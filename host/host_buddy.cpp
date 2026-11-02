// What buddy_net.cpp is on the Knomi: the same UDP messages on BUDDY_PORT, broadcast until the
// buddy's address is known, then straight to it. Plain BSD sockets, never blocks.
#include "buddy_net.h"
#include "host_internal.h"
#include <Preferences.h>
#include <random>

#if defined(__EMSCRIPTEN__)  // a browser has no UDP: the game sees nobody

uint32_t buddyNetId() { return 1; }
bool buddyNetUp() { return false; }
bool buddyNetPoll(BuddyMsg &) { return false; }
void buddyNetSend(const BuddyMsg &) {}
void buddyNetSetPeer(uint32_t) {}
void hostBuddyNetConfig(bool, uint16_t, const char *, uint16_t) {}

#else

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <Arduino.h>
#include <vector>

#define IFACE_REFRESH_MS 5000UL  // Wi-Fi comes and goes: the list of networks is read again this often
#define BIND_RETRY_MS 5000UL

static bool gEnabled = true, gConfigured = false;
static uint16_t gListenPort = BUDDY_PORT, gSendPort = BUDDY_PORT;
static in_addr_t gFixedTarget = 0;  // tests: one address instead of the networks' broadcasts

static int sock = -1;
static uint32_t bindT = 0, ifaceT = 0;
static bool bindTried = false, ifaceRead = false;
static std::vector<in_addr_t> broadcasts, ownAddrs;  // network order
static uint32_t peerId = 0;
static in_addr_t peerIp = 0;

uint32_t buddyNetId() {
  static uint32_t id = 0;
  if (!id) {
    // the Knomi uses its MAC; here a random number kept with the save, so it survives restarts
    Preferences p;
    p.begin("buddy");
    id = p.getUInt("id");
    if (!id) {
      std::random_device rd;
      id = (rd() | 1) & 0x7fffffffu;  // odd like the Knomi's, never 0
      p.putUInt("id", id);
    }
    p.end();
  }
  return id;
}

static void closeSock() {
  if (sock >= 0) close(sock);
  sock = -1;
  peerIp = 0;
}

void hostBuddyNetConfig(bool enabled, uint16_t listenPort, const char *target, uint16_t sendPort) {
  closeSock();
  gEnabled = enabled;
  gListenPort = listenPort;
  gSendPort = sendPort;
  gFixedTarget = target ? inet_addr(target) : 0;
  bindTried = ifaceRead = false;
  gConfigured = true;
}

// POKEMACGOCHI_BUDDY_PORT=n: test copies that only see each other, never the Knomis on the network
static void readEnv() {
  gConfigured = true;
  const char *e = getenv("POKEMACGOCHI_BUDDY_PORT");
  int port = e ? atoi(e) : 0;
  if (port > 0 && port < 65536) gListenPort = gSendPort = (uint16_t)port;
}

static bool openSock() {
  int s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) return false;
  int one = 1;
  // REUSEPORT: two copies on one Mac share the port, and both hear every broadcast
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
  setsockopt(s, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
  fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
  fcntl(s, F_SETFD, FD_CLOEXEC);
  sockaddr_in a = {};
  a.sin_family = AF_INET;
  a.sin_port = htons(gListenPort);
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s, (sockaddr *)&a, sizeof(a)) != 0) {
    fprintf(stderr, "buddy: cannot listen on UDP %u: %s\n", gListenPort, strerror(errno));
    close(s);
    return false;
  }
  sock = s;
  return true;
}

// IPv4 networks that are up and can broadcast (Wi-Fi, Ethernet), and this machine's own addresses
static void readInterfaces() {
  broadcasts.clear();
  ownAddrs.clear();
  if (gFixedTarget) {
    broadcasts.push_back(gFixedTarget);
    return;
  }
  ifaddrs *list = nullptr;
  if (getifaddrs(&list) != 0) return;
  for (ifaddrs *i = list; i; i = i->ifa_next) {
    if (!i->ifa_addr || i->ifa_addr->sa_family != AF_INET || !(i->ifa_flags & IFF_UP)) continue;
    ownAddrs.push_back(((sockaddr_in *)i->ifa_addr)->sin_addr.s_addr);
    if ((i->ifa_flags & (IFF_LOOPBACK | IFF_POINTOPOINT)) || !(i->ifa_flags & IFF_BROADCAST) || !i->ifa_broadaddr) continue;
    in_addr_t b = ((sockaddr_in *)i->ifa_broadaddr)->sin_addr.s_addr;
    bool dup = false;
    for (in_addr_t x : broadcasts) dup |= x == b;
    if (b && !dup) broadcasts.push_back(b);
  }
  freeifaddrs(list);
}

bool buddyNetUp() {
  if (!gConfigured) readEnv();
  if (!gEnabled) return false;
  uint32_t now = millis();
  if (!ifaceRead || now - ifaceT > IFACE_REFRESH_MS) {
    ifaceRead = true;
    ifaceT = now;
    readInterfaces();
  }
  bool up = !broadcasts.empty();
  if (up && sock < 0 && (!bindTried || now - bindT > BIND_RETRY_MS)) {
    bindTried = true;
    bindT = now;
    openSock();
  } else if (!up && sock >= 0) {
    closeSock();  // off the network: like the Knomi losing Wi-Fi
  }
  return up && sock >= 0;
}

bool buddyNetPoll(BuddyMsg &out) {
  if (sock < 0) return false;
  // doesn't get stuck on garbage: at most a few packets per call
  for (int i = 0; i < 4; i++) {
    uint8_t buf[sizeof(BuddyMsg) + 1];
    sockaddr_in from = {};
    socklen_t fromLen = sizeof(from);
    ssize_t got = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr *)&from, &fromLen);
    if (got < 0) return false;  // EWOULDBLOCK: nothing more for now
    if (!buddyParse(buf, (size_t)got, out) || out.from == buddyNetId()) continue;
    if (out.to != 0 && out.to != buddyNetId()) continue;
    if (out.from == peerId) peerIp = from.sin_addr.s_addr;
    return true;
  }
  return false;
}

static bool isOwnAddr(in_addr_t ip) {
  if ((ntohl(ip) >> 24) == 127) return true;
  for (in_addr_t a : ownAddrs)
    if (a == ip) return true;
  return false;
}

static void sendTo(const BuddyMsg &m, in_addr_t ip) {
  sockaddr_in a = {};
  a.sin_family = AF_INET;
  a.sin_port = htons(gSendPort);
  a.sin_addr.s_addr = ip;
  if (sendto(sock, &m, sizeof(m), 0, (sockaddr *)&a, sizeof(a)) < 0) {
    static uint32_t warnT = 0;  // e.g. macOS Local Network permission refused: says so now and then, not every packet
    if (!warnT || millis() - warnT > 60000UL) {
      warnT = millis() | 1;
      fprintf(stderr, "buddy: cannot send to %s: %s\n", inet_ntoa(a.sin_addr), strerror(errno));
    }
  }
}

void buddyNetSend(const BuddyMsg &m) {
  if (sock < 0) return;
  // A buddy on this same machine shares the port, and a direct packet would reach only one of the
  // two sockets (maybe our own): it keeps getting broadcasts, which reach both.
  if (m.to != 0 && m.to == peerId && peerIp && !isOwnAddr(peerIp)) {
    sendTo(m, peerIp);
    return;
  }
  for (in_addr_t b : broadcasts) sendTo(m, b);
}

void buddyNetSetPeer(uint32_t id) {
  if (id != peerId) peerIp = 0;
  peerId = id;
}

#endif
