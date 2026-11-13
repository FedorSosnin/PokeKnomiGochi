# Buddy relay

Browsers can't send the Knomi's local Wi-Fi messages (UDP broadcasts on port 47115), so Catch 151 in a browser meets its buddies through this relay: a Cloudflare Worker with one Durable Object that passes the same 65-byte buddy messages (`buddy.h`) between devices on the same home network.

- **Grouping:** devices are grouped by public address: their IPv4, or their IPv6 /64 plus the IPv4 they report (computers and phones often connect over IPv6, Knomis over IPv4).
- **What it passes:** only well-formed buddy messages, at most 8 a second per device, 16 devices per group. It stores and logs nothing; every check on the messages stays on the devices.
- **Cost:** it tells each device how many others share its group (`{"n": 2}`), and a device that is alone sends nothing. A `ping` text keeps quiet connections open and is answered without waking the object. Cloudflare's free plan covers it; if it ever runs out, only relay buddies pause until the next day, and the games keep working.

- **Live count:** [`/stats`](https://catch151-relay.fedor-dd0.workers.dev/stats) says how many devices are connected right now, all kinds together: `{"online": 12}`. That's every Knomi on Wi-Fi, Mac and Windows app and open browser tab, so it counts players who are playing now, not everyone who has the game. It's only the number, refreshed at most once a minute.

Deploy with [Wrangler](https://developers.cloudflare.com/workers/wrangler/): `cd relay && wrangler deploy`.

**Two addresses, one lobby (since 2.2.0, the rename to Catch 151):** `pokeknomigochi-relay` (`wrangler.toml`) holds the
lobby, and the copies up to 2.1.4 connect to it. `catch151-relay` (`wrangler-catch151.toml`, deploy with
`wrangler deploy -c wrangler-catch151.toml`) runs the same code (`src/catch151.js`) and reaches the same lobby through
its `LOBBY` binding's `script_name`, so copies on either address meet. Never rename `wrangler.toml`'s Worker: that
would start a second, empty lobby.
