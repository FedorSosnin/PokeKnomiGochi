#include "buddy_net.h"

#ifdef BOARD_KNOMI_V2
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_mac.h>

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

bool buddyNetUp() {
  bool up = WiFi.status() == WL_CONNECTED;
  if (up && !listening) listening = udp.begin(BUDDY_PORT);
  else if (!up && listening) {
    udp.stop();
    listening = false;
    peerIp = IPAddress();
  }
  return up && listening;
}

bool buddyNetPoll(BuddyMsg &out) {
  if (!listening) return false;
  // doesn't get stuck on garbage: at most a few packets per call
  for (int i = 0; i < 4; i++) {
    int len = udp.parsePacket();
    if (len <= 0) return false;
    uint8_t buf[sizeof(BuddyMsg) + 1];
    int got = udp.read(buf, sizeof(buf));
    if (got != len) continue;
    if (!buddyParse(buf, got, out) || out.from == buddyNetId()) continue;
    if (out.to != 0 && out.to != buddyNetId()) continue;
    if (out.from == peerId) peerIp = udp.remoteIP();
    return true;
  }
  return false;
}

void buddyNetSend(const BuddyMsg &m) {
  if (!listening) return;
  bool direct = m.to != 0 && m.to == peerId && peerIp != IPAddress();
  udp.beginPacket(direct ? peerIp : WiFi.broadcastIP(), BUDDY_PORT);
  udp.write((const uint8_t *)&m, sizeof(m));
  udp.endPacket();
}

void buddyNetSetPeer(uint32_t id) {
  if (id != peerId) peerIp = IPAddress();
  peerId = id;
}

#else

uint32_t buddyNetId() { return 1; }
bool buddyNetUp() { return false; }
bool buddyNetPoll(BuddyMsg &) { return false; }
void buddyNetSend(const BuddyMsg &) {}
void buddyNetSetPeer(uint32_t) {}

#endif
