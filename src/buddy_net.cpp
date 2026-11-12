#include "buddy_net.h"

#ifdef BOARD_KNOMI_V2
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NetworkClientSecure.h>
#include <esp_mac.h>
#include <esp_random.h>
#include "ota_roots.h"

static WiFiUDP udp;
static bool listening = false;
static uint32_t peerId = 0;
static IPAddress peerIp;

uint32_t buddyNetId() {
  static uint32_t id = 0;
  if (!id) {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    id = ((uint32_t)mac[2] << 24 | (uint32_t)mac[3] << 16 | (uint32_t)mac[4] << 8 | mac[5]) | 1;
  }
  return id;
}

// ---- the relay: the same messages over a WebSocket, so browsers on the same home network
// (which can't use the Wi-Fi messages) meet this Knomi too. A message may arrive both ways; the
// game already copes with repeats, since the Wi-Fi ones are resent until answered. ----
#define RELAY_FIRST_MS 20000UL   // after boot: Wi-Fi and the clock first
#define RELAY_RETRY_MS 10000UL   // after a failure, doubling up to RELAY_RETRY_MAX
#define RELAY_RETRY_MAX 300000UL
static NetworkClientSecure *rc = nullptr;
static bool relayOn = false;
static int relayOthers = 0;  // the relay's {"n": ...}: alone, nothing is sent
static uint32_t relayT = 0, relayWait = RELAY_FIRST_MS, relayPingT = 0;
static bool relayHeld = false;  // buddyNetRelayHold: closed for a Wi-Fi update

static void relayClose(bool failed) {
  if (rc) {
    rc->stop();
    delete rc;
    rc = nullptr;
  }
  if (failed) relayWait = relayWait < RELAY_RETRY_MS ? RELAY_RETRY_MS : (relayWait * 2 > RELAY_RETRY_MAX ? RELAY_RETRY_MAX : relayWait * 2);
  relayOn = false;
  relayOthers = 0;
  relayT = millis();
}

// one WebSocket frame from us: always masked, as the standard asks of clients; short ones only
static bool wsSend(uint8_t opcode, const uint8_t *data, size_t len) {
  if (!rc || len > 125) return false;
  uint8_t f[6 + 125];
  uint32_t m = esp_random();
  f[0] = 0x80 | opcode;
  f[1] = 0x80 | len;
  memcpy(f + 2, &m, 4);
  for (size_t i = 0; i < len; i++) f[6 + i] = data[i] ^ f[2 + (i & 3)];
  return rc->write(f, 6 + len) == 6 + len;
}

// Connects and upgrades to a WebSocket. Holds the loop for a second or two (the TLS handshake):
// only at boot and after the connection drops, with a growing pause between tries.
static void relayConnect() {
  rc = new NetworkClientSecure;
  rc->setCACert(OTA_ROOTS);
  rc->setHandshakeTimeout(8);
  if (!rc->connect(BUDDY_RELAY_HOST, 443)) {
    Serial.println("buddy: relay unreachable");
    relayClose(true);
    return;
  }
  uint8_t raw[16];
  esp_fill_random(raw, sizeof(raw));
  static const char *B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  char key[25];
  for (int i = 0, o = 0; i < 16; i += 3) {
    uint32_t v = raw[i] << 16 | (i + 1 < 16 ? raw[i + 1] << 8 : 0) | (i + 2 < 16 ? raw[i + 2] : 0);
    key[o++] = B64[(v >> 18) & 63];
    key[o++] = B64[(v >> 12) & 63];
    key[o++] = i + 1 < 16 ? B64[(v >> 6) & 63] : '=';
    key[o++] = i + 2 < 16 ? B64[v & 63] : '=';
    key[o] = 0;
  }
  rc->printf("GET " BUDDY_RELAY_PATH " HTTP/1.1\r\nHost: " BUDDY_RELAY_HOST "\r\nUpgrade: websocket\r\n"
             "Connection: Upgrade\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n"
             "User-Agent: PokeKnomiGochi/2\r\n\r\n", key);  // /2: steps aside for Wi-Fi updates (1.31 on)
  rc->setTimeout(4000);
  String status = rc->readStringUntil('\n');
  if (!status.startsWith("HTTP/1.1 101")) {
    Serial.printf("buddy: relay refused (%s)\n", status.c_str());
    relayClose(true);
    return;
  }
  while (rc->connected()) {  // the rest of the headers, up to the blank line
    String h = rc->readStringUntil('\n');
    if (h.length() <= 1) break;
  }
  rc->setTimeout(1000);
  relayOn = true;
  relayWait = RELAY_RETRY_MS;
  relayPingT = millis();
  Serial.println("buddy: relay connected");
}

static void relayTick(bool wifi) {
  uint32_t now = millis();
  if (!wifi || relayHeld) {
    if (rc) relayClose(false);
    return;
  }
  if (!rc && now - relayT >= relayWait) relayConnect();
  if (relayOn && now - relayPingT >= BUDDY_RELAY_PING_MS) {
    relayPingT = now;
    if (!wsSend(0x1, (const uint8_t *)"ping", 4)) relayClose(true);
  }
  if (rc && !rc->connected()) relayClose(true);
}

static bool relayPoll(BuddyMsg &out) {
  for (int k = 0; k < 4 && relayOn && rc->available() >= 2; k++) {
    uint8_t h[2];
    rc->read(h, 2);
    uint8_t op = h[0] & 0x0f;
    size_t len = h[1] & 0x7f;
    if (len == 126) {
      uint8_t e[2];
      if (rc->readBytes(e, 2) != 2) { relayClose(true); return false; }
      len = (size_t)e[0] << 8 | e[1];
    }
    if (len > 512 || len == 127 || (h[1] & 0x80)) {  // nothing that big comes from the relay, and it never masks
      relayClose(true);
      return false;
    }
    uint8_t buf[513];  // + the end of a text frame
    if (rc->readBytes(buf, len) != len) {
      relayClose(true);
      return false;
    }
    if (op == 0x8) {  // the relay is closing
      relayClose(true);
      return false;
    }
    if (op == 0x9) {  // ping: answered with the same data
      wsSend(0xA, buf, len);
      continue;
    }
    if (op == 0x1) {  // {"n": others}; "pong" and anything else are ignored
      buf[len] = 0;
      const char *n = strstr((const char *)buf, "\"n\":");
      if (n) relayOthers = atoi(n + 4);
      continue;
    }
    if (op != 0x2 || !buddyParse(buf, len, out) || out.from == buddyNetId()) continue;
    if (out.to != 0 && out.to != buddyNetId()) continue;
    return true;
  }
  return false;
}

bool buddyNetUp() {
  bool up = WiFi.status() == WL_CONNECTED;
  if (up && !listening) listening = udp.begin(BUDDY_PORT);
  else if (!up && listening) {
    udp.stop();
    listening = false;
    peerIp = IPAddress();
  }
  relayTick(up);
  return up && listening;
}

bool buddyNetPoll(BuddyMsg &out) {
  if (!listening) return false;
  // doesn't get stuck on garbage: at most a few packets per call
  for (int i = 0; i < 4; i++) {
    int len = udp.parsePacket();
    if (len <= 0) break;
    uint8_t buf[sizeof(BuddyMsg) + 1];
    int got = udp.read(buf, sizeof(buf));
    if (got != len) continue;
    if (!buddyParse(buf, got, out) || out.from == buddyNetId()) continue;
    if (out.to != 0 && out.to != buddyNetId()) continue;
    if (out.from == peerId) peerIp = udp.remoteIP();
    return true;
  }
  return relayPoll(out);
}

void buddyNetSend(const BuddyMsg &m) {
  if (!listening) return;
  bool direct = m.to != 0 && m.to == peerId && peerIp != IPAddress();
  udp.beginPacket(direct ? peerIp : WiFi.broadcastIP(), BUDDY_PORT);
  udp.write((const uint8_t *)&m, sizeof(m));
  udp.endPacket();
  if (relayOn && relayOthers > 0 && !wsSend(0x2, (const uint8_t *)&m, sizeof(m))) relayClose(true);
}

void buddyNetSetPeer(uint32_t id) {
  if (id != peerId) peerIp = IPAddress();
  peerId = id;
}

void buddyNetRelayHold(bool hold) {
  if (hold == relayHeld) return;
  relayHeld = hold;
  if (hold) {
    if (rc) relayClose(false);  // frees its TLS memory now
  } else {
    relayT = millis();
    relayWait = 2000;  // back soon after
  }
}

#else

uint32_t buddyNetId() { return 1; }
void buddyNetRelayHold(bool) {}
bool buddyNetUp() { return false; }
bool buddyNetPoll(BuddyMsg &) { return false; }
void buddyNetSend(const BuddyMsg &) {}
void buddyNetSetPeer(uint32_t) {}

#endif
