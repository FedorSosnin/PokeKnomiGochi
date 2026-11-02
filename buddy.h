#pragma once
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

// for now a single buddy: the first one seen, until it leaves
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
