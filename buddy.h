#pragma once
#include <stdint.h>
#include <string.h>

// Buddy: otro PokeKnomiGochi en la misma wifi. Mensajes UDP con formato fijo y reglas sin
// hardware (test/test_buddy.cpp); la radio vive en buddy_net.cpp. Nada de lo que llega por la
// red se da por bueno: se valida todo antes de usarlo. Solo viaja la especie, el nivel, el
// apodo y la Pokedex de la mascota.

#define BUDDY_PORT 47115
#define BUDDY_VER 1
#define BUDDY_HELLO_MS 4000UL
#define BUDDY_TIMEOUT_MS 15000UL  // sin saludos durante este tiempo, se ha ido
#define BUDDY_RESEND_MS 400UL     // UDP puede perder paquetes: se repite hasta tener respuesta
#define BUDDY_REPLY_MS 20000UL    // respuesta a una invitacion o jugada, antes de rendirse
#define BUDDY_MAX_STAT 1000

enum BuddyType : uint8_t {
  BM_HELLO = 1, BM_INVITE, BM_ACCEPT, BM_DECLINE, BM_FIGHTER, BM_MOVE, BM_OFFER, BM_CONFIRM, BM_LEAVE, BM_END
};
enum BuddyKind : uint8_t { BK_BATTLE = 1, BK_TRADE };

struct __attribute__((packed)) BuddyMsg {
  char magic[4];
  uint8_t ver, type;
  uint32_t from, to, session;  // to = 0: para todos
  uint8_t kind, ack;           // ack = 1: es una respuesta, no pide otra (evita el ping-pong)
  uint8_t turn, move;
  int16_t dex;
  uint8_t level, shiny;
  uint16_t hp, atk, def, spe;
  char nick[12];
  uint8_t reg[19];             // Pokedex, 1 bit por especie
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

// un paquete de la red a BuddyMsg, solo si tiene sentido. Limpia el apodo: se pinta en pantalla.
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
    if (*c < 32 || *c > 126) {  // lo que no sea ASCII imprimible no llega a la pantalla
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

// de momento un solo buddy: el primero que se vea, hasta que se vaya
struct BuddyPeer {
  bool on = false;
  uint32_t id = 0, seen = 0;
  int16_t dex = 0;
  uint8_t level = 1, shiny = 0;
  char nick[12] = "";
  uint8_t reg[19] = {};
};

// true si acaba de llegar
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

// combate entre dos: las dos placas sacan los mismos numeros de la misma semilla (la sesion)
inline uint32_t pvpRand(uint32_t &r) {
  uint32_t x = r ? r : 1;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  r = x;
  return x;
}

inline uint8_t pvpRoll(uint32_t &r) { return 85 + pvpRand(r) % 16; }

// quien ataca antes, visto desde esta placa. Consume siempre una tirada, y en un empate de
// velocidad la tirada elige por id, asi que las dos placas se ponen de acuerdo.
inline bool pvpMineFirst(uint16_t mySpe, uint16_t foeSpe, uint32_t myId, uint32_t foeId, uint32_t &r) {
  bool coin = pvpRand(r) & 1;
  if (mySpe != foeSpe) return mySpe > foeSpe;
  return (myId < foeId) == coin;
}
