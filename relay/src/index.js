// The buddy relay: lets Catch 151 in a browser (which can't send the Knomi's local Wi-Fi
// messages) meet Knomis, Macs and other browsers on the same home network.
//
// It only passes along the game's own buddy messages (65 bytes, "PKG1", see buddy.h) between
// devices that share an internet address, as devices on one home Wi-Fi do. It stores nothing:
// battles, trades and every check on the messages stay on the devices, exactly as on the Wi-Fi.
//
//   wss://<relay>/ws?v4=<the device's public IPv4, when it connects over IPv6>
//
// A computer or phone usually connects over IPv6, where every device has its own address, while a
// Knomi uses the home's shared IPv4: devices look their IPv4 up and send it, and are grouped by it.
// Text frames from the relay tell each device how many others share its group: {"n": 2}. A device
// that is alone sends nothing, which keeps the relay within Cloudflare's free plan.
//
//   https://<relay>/stats  ->  {"online": 12}
//
// How many devices are connected right now, all of them together (Knomis on Wi-Fi, the Mac and
// Windows apps, open browser tabs). Only the number: nothing about who or where.

const MSG_SIZE = 65;
const MAX_PER_GROUP = 16;  // a home has a few devices: more is someone filling a group on purpose
const RATE_PER_S = 8;      // the game sends a few messages a second at most, during a battle
const BURST = 16;
const STATS_CACHE_S = 60;  // the count is kept a minute: looking often doesn't wake the lobby each time

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === '/ws') {
      if (request.headers.get('Upgrade') !== 'websocket') return new Response('expected a websocket', { status: 426 });
      const keys = groupKeys(request.headers.get('CF-Connecting-IP') || '', url.searchParams.get('v4'));
      if (!keys.length) return new Response('no address', { status: 400 });
      const lobby = env.LOBBY.get(env.LOBBY.idFromName('lobby'));
      const fwd = new Request(request);
      fwd.headers.set('X-Groups', keys.join(','));
      return lobby.fetch(fwd);
    }
    if (url.pathname === '/stats') {
      const cache = caches.default;
      const key = new Request(url.origin + '/stats');
      let res = await cache.match(key);
      if (!res) {
        const lobby = env.LOBBY.get(env.LOBBY.idFromName('lobby'));
        const { online } = await (await lobby.fetch(new Request(url.origin + '/stats'))).json();
        res = new Response(JSON.stringify({ online }), {
          headers: {
            'content-type': 'application/json',
            'cache-control': `public, max-age=${STATS_CACHE_S}`,
            'access-control-allow-origin': '*',  // a dashboard on another site can read it
          },
        });
        await cache.put(key, res.clone());
      }
      return res;
    }
    return new Response('Catch 151 buddy relay. See https://github.com/FedorSosnin/Catch151\n', {
      headers: { 'content-type': 'text/plain' },
    });
  },
};

const V4 = /^(25[0-5]|2[0-4]\d|1?\d?\d)(\.(25[0-5]|2[0-4]\d|1?\d?\d)){3}$/;

// the groups a device belongs to: its IPv4, or its IPv6 /64 (one home network) plus the IPv4 it
// reports. A reported IPv4 is only taken from IPv6 connections, which can't show it themselves.
export function groupKeys(ip, claimed) {
  if (V4.test(ip)) return ['4:' + ip];
  if (!ip.includes(':')) return [];
  const keys = ['6:' + expand6(ip).slice(0, 4).join(':')];
  if (claimed && V4.test(claimed)) keys.push('4:' + claimed);
  return keys;
}

function expand6(ip) {
  const [head, tail = ''] = ip.split('::');
  const a = head ? head.split(':') : [];
  const b = tail ? tail.split(':') : [];
  const fill = ip.includes('::') ? Array(8 - a.length - b.length).fill('0') : [];
  return [...a, ...fill, ...b].map((h) => h.toLowerCase().replace(/^0+(?=.)/, ''));
}

export function validMessage(data) {
  if (!(data instanceof ArrayBuffer) || data.byteLength !== MSG_SIZE) return false;
  const b = new Uint8Array(data);
  return b[0] === 0x50 && b[1] === 0x4b && b[2] === 0x47 && b[3] === 0x31;  // "PKG1"
}

// One object holds every connection (with WebSocket hibernation, idle ones cost nothing). Each
// socket is tagged with its groups, so finding who shares a group is a lookup, not a scan.
export class Lobby {
  constructor(ctx) {
    this.ctx = ctx;
    // devices keep their connection open with a "ping" now and then; answered here without waking
    // the object, so keeping a quiet connection open costs nothing
    ctx.setWebSocketAutoResponse(new WebSocketRequestResponsePair('ping', 'pong'));
  }

  async fetch(request) {
    if (new URL(request.url).pathname === '/stats')  // each socket once, whatever its groups
      return Response.json({ online: this.ctx.getWebSockets().length });
    const groups = (request.headers.get('X-Groups') || '').split(',').filter(Boolean);
    for (const g of groups)
      if (this.ctx.getWebSockets(g).length >= MAX_PER_GROUP) return new Response('group full', { status: 429 });
    const pair = new WebSocketPair();
    const [client, server] = Object.values(pair);
    this.ctx.acceptWebSocket(server, groups);
    server.serializeAttachment({ groups, tokens: BURST, at: Date.now() });
    this.announce(groups);
    return new Response(null, { status: 101, webSocket: client });
  }

  // everyone sharing a group with ws (itself and one that is leaving excluded), each once
  others(ws, groups, gone) {
    const seen = new Set();
    for (const g of groups) for (const o of this.ctx.getWebSockets(g)) if (o !== ws && o !== gone) seen.add(o);
    return seen;
  }

  // tells every socket in these groups how many others it can reach now
  announce(groups, gone) {
    const touched = new Set();
    for (const g of groups) for (const s of this.ctx.getWebSockets(g)) if (s !== gone) touched.add(s);
    for (const s of touched) {
      const a = s.deserializeAttachment();
      try { s.send(JSON.stringify({ n: this.others(s, a.groups, gone).size })); } catch (e) {}
    }
  }

  async webSocketMessage(ws, data) {
    if (!validMessage(data)) return;  // text and anything else: ignored
    const a = ws.deserializeAttachment();
    const now = Date.now();
    a.tokens = Math.min(BURST, a.tokens + ((now - a.at) / 1000) * RATE_PER_S);
    a.at = now;
    if (a.tokens < 1) return;  // too fast: dropped, like a lost Wi-Fi packet
    a.tokens -= 1;
    ws.serializeAttachment(a);
    for (const o of this.others(ws, a.groups)) {
      try { o.send(data); } catch (e) {}
    }
  }

  async webSocketClose(ws) {
    const a = ws.deserializeAttachment();
    try { ws.close(); } catch (e) {}
    if (a) this.announce(a.groups, ws);
  }

  async webSocketError(ws) {
    await this.webSocketClose(ws);
  }
}
