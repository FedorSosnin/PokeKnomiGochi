// What buddy_net.cpp is on the Knomi: the same UDP messages on BUDDY_PORT, broadcast until the
// buddy's address is known, then straight to it. Plain BSD sockets (Winsock on Windows), never blocks. Plus the relay
// (relay/ on Cloudflare): the front end keeps a WebSocket to it and hands the messages over, so
// browsers, which have no UDP, meet everyone else too. A message may arrive both ways; the game
// already copes with repeats, since the UDP ones are resent until answered.
#include "buddy_net.h"
#include "host.h"
#include "host_internal.h"
#include <Preferences.h>
#include <Arduino.h>
#include <random>
#include <string.h>

#define RELAY_QUEUE 16

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

// ---- the relay: the front end calls these on the game's thread ----
static HostRelaySend gRelaySend = nullptr;
static int gRelayOthers = 0;
static uint8_t gRelayQ[RELAY_QUEUE][sizeof(BuddyMsg)];
static int gRelayHead = 0, gRelayCount = 0;

void hostRelayConnected(HostRelaySend send) {
  gRelaySend = send;
  if (!send) gRelayOthers = 0;
}
void hostRelayPeers(int n) { gRelayOthers = n > 0 ? n : 0; }
void hostRelayReceived(const uint8_t *data, int len) {
  if (len != (int)sizeof(BuddyMsg) || gRelayCount == RELAY_QUEUE) return;  // full: dropped, like a lost packet
  memcpy(gRelayQ[(gRelayHead + gRelayCount++) % RELAY_QUEUE], data, sizeof(BuddyMsg));
}

static bool relayPoll(BuddyMsg &out) {
  while (gRelayCount) {
    const uint8_t *b = gRelayQ[gRelayHead];
    gRelayHead = (gRelayHead + 1) % RELAY_QUEUE;
    gRelayCount--;
    if (!buddyParse(b, sizeof(BuddyMsg), out) || out.from == buddyNetId()) continue;
    if (out.to != 0 && out.to != buddyNetId()) continue;
    return true;
  }
  return false;
}

static void relaySend(const BuddyMsg &m) {
  // alone on the relay: nobody to tell. Otherwise every greeting, as on the Wi-Fi: with fewer, one
  // late greeting is enough for a buddy to count as gone (BUDDY_TIMEOUT_MS)
  if (!gRelaySend || !gRelayOthers) return;
  gRelaySend((const uint8_t *)&m, sizeof(m));
}

#if defined(__EMSCRIPTEN__)  // a browser has no UDP: only the relay

static bool udpUp() { return false; }
static bool udpPoll(BuddyMsg &) { return false; }
static void udpSend(const BuddyMsg &) {}
static void udpSetPeer(uint32_t) {}
void hostBuddyNetConfig(bool, uint16_t, const char *, uint16_t) {}

#else

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#ifdef _WIN32  // Winsock: the same calls, a few different names
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#pragma push_macro("INPUT")  // Arduino's pin mode is a struct name in Windows' headers
#undef INPUT
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma pop_macro("INPUT")
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
typedef uint32_t in_addr_t;
#define closeSocket closesocket
static const char *sockError() {
  static char msg[16];
  snprintf(msg, sizeof(msg), "error %d", WSAGetLastError());
  return msg;
}
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define closeSocket close
static const char *sockError() { return strerror(errno); }
#endif

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

static void closeSock() {
  if (sock >= 0) closeSocket(sock);
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
#ifdef _WIN32
  static bool wsa = false;
  if (!wsa) {
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d) != 0) return false;
    wsa = true;
  }
#endif
  int s = (int)socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) return false;
  int one = 1;
  // REUSEPORT: two copies on one Mac share the port, and both hear every broadcast (on Windows REUSEADDR does that)
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
  setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char *)&one, sizeof(one));
#ifdef _WIN32
  u_long nb = 1;
  ioctlsocket(s, FIONBIO, &nb);
  BOOL no = FALSE;  // a packet to a buddy that has gone would otherwise turn up as an error on the next receive
  DWORD ret = 0;
  WSAIoctl(s, SIO_UDP_CONNRESET, &no, sizeof(no), NULL, 0, &ret, NULL, NULL);
#else
  setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
  fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
  fcntl(s, F_SETFD, FD_CLOEXEC);
#endif
  sockaddr_in a = {};
  a.sin_family = AF_INET;
  a.sin_port = htons(gListenPort);
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s, (sockaddr *)&a, sizeof(a)) != 0) {
    fprintf(stderr, "buddy: cannot listen on UDP %u: %s\n", gListenPort, sockError());
    closeSocket(s);
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
#ifdef _WIN32
  ULONG len = 16384;
  std::vector<uint8_t> mem;
  IP_ADAPTER_ADDRESSES *list;
  ULONG r;
  do {
    mem.resize(len);
    list = (IP_ADAPTER_ADDRESSES *)mem.data();
    r = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, list, &len);
  } while (r == ERROR_BUFFER_OVERFLOW);
  if (r != NO_ERROR) return;
  for (IP_ADAPTER_ADDRESSES *i = list; i; i = i->Next) {
    if (i->OperStatus != IfOperStatusUp) continue;
    bool canBroadcast = i->IfType != IF_TYPE_SOFTWARE_LOOPBACK && i->IfType != IF_TYPE_TUNNEL && i->IfType != IF_TYPE_PPP;
    for (IP_ADAPTER_UNICAST_ADDRESS *u = i->FirstUnicastAddress; u; u = u->Next) {
      if (u->Address.lpSockaddr->sa_family != AF_INET) continue;
      in_addr_t ip = ((sockaddr_in *)u->Address.lpSockaddr)->sin_addr.s_addr;
      ownAddrs.push_back(ip);
      int bits = u->OnLinkPrefixLength;
      if (!canBroadcast || bits <= 0 || bits >= 31) continue;
      in_addr_t b = ip | htonl(0xffffffffu >> bits);
      bool dup = false;
      for (in_addr_t x : broadcasts) dup |= x == b;
      if (!dup) broadcasts.push_back(b);
    }
  }
#else
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
#endif
}

static bool udpUp() {
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

static bool udpPoll(BuddyMsg &out) {
  if (sock < 0) return false;
  // doesn't get stuck on garbage: at most a few packets per call
  for (int i = 0; i < 4; i++) {
    uint8_t buf[sizeof(BuddyMsg) + 1];
    sockaddr_in from = {};
    socklen_t fromLen = sizeof(from);
    int got = (int)recvfrom(sock, (char *)buf, sizeof(buf), 0, (sockaddr *)&from, &fromLen);
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
  if (sendto(sock, (const char *)&m, sizeof(m), 0, (sockaddr *)&a, sizeof(a)) < 0) {
    static uint32_t warnT = 0;  // e.g. macOS Local Network permission refused: says so now and then, not every packet
    if (!warnT || millis() - warnT > 60000UL) {
      warnT = millis() | 1;
      fprintf(stderr, "buddy: cannot send to %s: %s\n", inet_ntoa(a.sin_addr), sockError());
    }
  }
}

static void udpSend(const BuddyMsg &m) {
  if (sock < 0) return;
  // A buddy on this same machine shares the port, and a direct packet would reach only one of the
  // two sockets (maybe our own): it keeps getting broadcasts, which reach both.
  if (m.to != 0 && m.to == peerId && peerIp && !isOwnAddr(peerIp)) {
    sendTo(m, peerIp);
    return;
  }
  for (in_addr_t b : broadcasts) sendTo(m, b);
}

static void udpSetPeer(uint32_t id) {
  if (id != peerId) peerIp = 0;
  peerId = id;
}

#endif

// ---- what the game sees: both ways as one ----
bool buddyNetUp() {
  bool udp = udpUp();
  return udp || gRelaySend;
}

bool buddyNetPoll(BuddyMsg &out) { return udpPoll(out) || relayPoll(out); }

void buddyNetSend(const BuddyMsg &m) {
  udpSend(m);
  relaySend(m);
}

void buddyNetSetPeer(uint32_t id) { udpSetPeer(id); }
