#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Buddy: another PokeKnomiGochi on the same wifi. UDP messages with a fixed format and rules without
// hardware (test/test_buddy.cpp); the radio lives in buddy_net.cpp. Nothing that arrives over the
// network is taken as good: everything is validated before use. Only the species, level,
// nickname and Pokedex of the pet travel.

#define BUDDY_PORT 47115
#define BUDDY_VER 1
#define BUDDY_HELLO_MS 4000UL
#define BUDDY_TIMEOUT_MS 15000UL  // no greetings for this long, it has gone
#define BUDDY_RESEND_MS 400UL     // UDP can lose packets: repeats until there is a reply
#define BUDDY_REPLY_MS 20000UL    // reply to an invitation or move, before giving up
#define BUDDY_MAX_STAT 1000
// The relay (relay/ on Cloudflare): lets browsers, which can't use these Wi-Fi messages, meet
// everyone else. The same messages travel over one WebSocket; see relay/src/index.js.
#define BUDDY_RELAY_HOST "pokeknomigochi-relay.fedor-dd0.workers.dev"
#define BUDDY_RELAY_PATH "/ws"
#define BUDDY_RELAY_PING_MS 45000UL  // keeps a quiet connection open (the relay answers "pong")

enum BuddyType : uint8_t {
  BM_HELLO = 1, BM_INVITE, BM_ACCEPT, BM_DECLINE, BM_FIGHTER, BM_MOVE, BM_OFFER, BM_CONFIRM, BM_LEAVE, BM_END
};
enum BuddyKind : uint8_t { BK_BATTLE = 1, BK_TRADE };

struct __attribute__((packed)) BuddyMsg {
  char magic[4];
  uint8_t ver, type;
  uint32_t from, to, session;  // to = 0: for everyone
  uint8_t kind, ack;           // ack = 1: it is a reply, doesn't ask for another (avoids ping-pong)
  uint8_t turn, move;
  int16_t dex;
  uint8_t level, shiny;
  uint16_t hp, atk, def, spe;
  char nick[12];
  uint8_t reg[19];             // Pokedex, 1 bit per species
};

// The wire format: these bytes in this order, little-endian, on the ESP32 and on the Mac alike.
// Changing them breaks talking to boards on older firmware.
static_assert(sizeof(BuddyMsg) == 65, "BuddyMsg wire size");
static_assert(offsetof(BuddyMsg, from) == 6 && offsetof(BuddyMsg, kind) == 18 && offsetof(BuddyMsg, dex) == 22 &&
                  offsetof(BuddyMsg, hp) == 26 && offsetof(BuddyMsg, nick) == 34 && offsetof(BuddyMsg, reg) == 46,
              "BuddyMsg wire layout");

inline void buddyInit(BuddyMsg &m, BuddyType type, uint32_t from, uint32_t to, uint32_t session) {
  memset(&m, 0, sizeof(m));
  memcpy(m.magic, "PKG1", 4);
  m.ver = BUDDY_VER;
  m.type = type;
  m.from = from;
  m.to = to;
  m.session = session;
}

inline bool buddyDexOk(int16_t d) { return d >= 1 && d <= 151; }

// a network packet to BuddyMsg, only if it makes sense. Cleans the nickname: it is drawn on screen.
inline bool buddyParse(const uint8_t *buf, size_t len, BuddyMsg &m) {
  if (len != sizeof(BuddyMsg)) return false;
  memcpy(&m, buf, sizeof(BuddyMsg));
  if (memcmp(m.magic, "PKG1", 4) != 0 || m.ver != BUDDY_VER) return false;
  if (m.type < BM_HELLO || m.type >= BM_END || m.from == 0) return false;
  if (m.type == BM_HELLO || m.type == BM_FIGHTER || m.type == BM_OFFER || m.type == BM_CONFIRM) {
    if (!buddyDexOk(m.dex)) return false;
  }
  if ((m.type == BM_HELLO || m.type == BM_FIGHTER) && (m.level < 1 || m.level > 100)) return false;
  if (m.type == BM_FIGHTER) {
    if (m.hp < 1 || m.def < 1) return false;
    if (m.hp > BUDDY_MAX_STAT || m.atk > BUDDY_MAX_STAT || m.def > BUDDY_MAX_STAT || m.spe > BUDDY_MAX_STAT) return false;
  }
  if (m.type == BM_INVITE && m.kind != BK_BATTLE && m.kind != BK_TRADE) return false;
  if (m.type == BM_MOVE && m.move > 1) return false;
  m.nick[sizeof(m.nick) - 1] = 0;
  for (char *c = m.nick; *c; c++) {
    if (*c < 32 || *c > 126) {  // anything that isn't printable ASCII doesn't reach the screen
      m.nick[0] = 0;
      break;
    }
  }
  m.shiny = m.shiny ? 1 : 0;
  return true;
}

inline bool buddyRegHas(const uint8_t *reg, int16_t dex) {
  return buddyDexOk(dex) && (reg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
}

#define BUDDY_MAX 4  // buddies at once: with your own Pokemon, a party of 5 on screen

// one buddy on the network, kept while its greetings keep coming
struct BuddyPeer {
  bool on = false;
  uint32_t id = 0, seen = 0;
  int16_t dex = 0;
  uint8_t level = 1, shiny = 0;
  char nick[12] = "";
  uint8_t reg[19] = {};
};

// true if it has just arrived
inline bool buddyPeerHello(BuddyPeer &p, const BuddyMsg &m, uint32_t now) {
  if (p.on && p.id != m.from) return false;
  bool arrived = !p.on;
  p.on = true;
  p.id = m.from;
  p.seen = now;
  p.dex = m.dex;
  p.level = m.level;
  p.shiny = m.shiny;
  memcpy(p.nick, m.nick, sizeof(p.nick));
  memcpy(p.reg, m.reg, sizeof(p.reg));
  return arrived;
}

inline bool buddyPeerGone(const BuddyPeer &p, uint32_t now) { return p.on && now - p.seen > BUDDY_TIMEOUT_MS; }

// every buddy around: the slot of this id, -1 if it isn't one
inline int buddyFind(const BuddyPeer *ps, uint32_t id) {
  for (int i = 0; i < BUDDY_MAX; i++)
    if (ps[i].on && ps[i].id == id) return i;
  return -1;
}

inline int buddyCount(const BuddyPeer *ps) {
  int n = 0;
  for (int i = 0; i < BUDDY_MAX; i++) n += ps[i].on;
  return n;
}

// a greeting: updates its buddy, or gives it a free slot. The slot, or -1 when all are taken
// (the fifth buddy waits until one leaves). *arrived = it is new.
inline int buddyHello(BuddyPeer *ps, const BuddyMsg &m, uint32_t now, bool *arrived) {
  int i = buddyFind(ps, m.from);
  if (i < 0)
    for (int j = 0; j < BUDDY_MAX && i < 0; j++)
      if (!ps[j].on) i = j;
  if (i < 0) return -1;
  *arrived = buddyPeerHello(ps[i], m, now);
  return i;
}

// Where each Pokemon stands on the main screen when others are there (n = buddies on screen, or 1
// for the daily visitor). Yours always stands in the middle, a size bigger than the rest, so it is
// never mistaken for a buddy of the same species; the others stand at the sides, smaller the more
// there are. Alone, yours walks around as always.
#define PARTY_MINE_X 233
inline int16_t partyBuddyX(int n, int k) {  // k = 1..n: the buddies in slot order
  static const int16_t ONE[] = { 368 }, TWO[] = { 98, 368 }, THREE[] = { 108, 332, 410 }, FOUR[] = { 56, 136, 330, 410 };
  const int16_t *xs = n <= 1 ? ONE : n == 2 ? TWO : n == 3 ? THREE : FOUR;
  int i = k - 1;
  if (i < 0) i = 0;
  if (i > (n < 1 ? 0 : n > 4 ? 3 : n - 1)) i = n < 1 ? 0 : (n > 4 ? 3 : n - 1);
  return xs[i];
}
inline uint8_t partyScale(int n, bool mine) {
  if (n <= 0) return 5;  // alone: as always
  if (mine) return n <= 2 ? 4 : 3;
  return n <= 2 ? 3 : 2;
}
inline int16_t partyHalf(int n) { return n <= 2 ? 56 : 38; }   // half the ground a buddy takes
inline int16_t partyTall(int n) { return n <= 2 ? 110 : 80; }  // how tall it stands, for touches

// battle between two: both boards draw the same numbers from the same seed (the session)
inline uint32_t pvpRand(uint32_t &r) {
  uint32_t x = r ? r : 1;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  r = x;
  return x;
}

inline uint8_t pvpRoll(uint32_t &r) { return 85 + pvpRand(r) % 16; }

// who attacks first, seen from this board. Always consumes one roll, and on a speed tie
// the roll chooses by id, so both boards agree.
inline bool pvpMineFirst(uint16_t mySpe, uint16_t foeSpe, uint32_t myId, uint32_t foeId, uint32_t &r) {
  bool coin = pvpRand(r) & 1;
  if (mySpe != foeSpe) return mySpe > foeSpe;
  return (myId < foeId) == coin;
}
