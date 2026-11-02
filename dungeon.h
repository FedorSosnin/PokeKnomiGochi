#pragma once
#include <stdint.h>
#include "battle.h"

// PokeDungeon: reglas por turnos sin hardware (test/test_dungeon.cpp). El jugador da un paso y
// luego actua cada rival. Entrar en una casilla ocupada es atacar con el mejor ataque contra ese
// rival, con la misma formula que el combate, asi que entrenar a la mascota se nota aqui.

#define DN_W 21
#define DN_H 15
#define DN_ROOMS 6            // 3 x 2 sectores de 7 x 7
#define DN_MONS 8
#define DN_BERRIES 3
#define DN_ALERT 4            // casillas a las que un rival te persigue
#define DN_HP_MULT 4          // aguantar varias peleas seguidas, no una
#define DN_BERRY_PCT 35       // vida que cura una baya
#define DN_FLOOR_PCT 25       // vida que se recupera al bajar

enum DnTile : uint8_t { DN_WALL, DN_FLOOR, DN_STAIRS };

struct DnMon {
  BtlFighter f;
  int8_t x, y;
  bool on;
};

struct DnItem { int8_t x, y; bool on; };

// lo que paso en el ultimo turno, para pintarlo
struct DnHit {
  bool mine;
  int8_t mon;       // el rival que golpea o recibe
  uint8_t move;     // indice de MOVE_TBL
  uint16_t dmg;
  uint8_t eff;
  bool ko;
};

struct DnState {
  uint8_t map[DN_H][DN_W];
  BtlFighter me;
  int8_t x, y, faceX;
  uint8_t floor;
  bool dead, ate, descended;
  uint16_t kos;
  uint32_t rng;
  DnMon mon[DN_MONS];
  DnItem berry[DN_BERRIES];
  DnHit hits[DN_MONS + 1];
  uint8_t hitCount;
};

inline uint32_t dnRand(DnState &s, uint32_t n) {  // xorshift: partidas reproducibles en las pruebas
  uint32_t x = s.rng;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  s.rng = x;
  return n ? x % n : 0;
}

inline bool dnWalkable(const DnState &s, int x, int y) {
  return x >= 0 && y >= 0 && x < DN_W && y < DN_H && s.map[y][x] != DN_WALL;
}

inline int dnMonAt(const DnState &s, int x, int y) {
  for (int i = 0; i < DN_MONS; i++)
    if (s.mon[i].on && s.mon[i].x == x && s.mon[i].y == y) return i;
  return -1;
}

inline int dnBerryAt(const DnState &s, int x, int y) {
  for (int i = 0; i < DN_BERRIES; i++)
    if (s.berry[i].on && s.berry[i].x == x && s.berry[i].y == y) return i;
  return -1;
}

// mas hondo, rivales de mas nivel: la profundidad es lo que aprieta
inline uint8_t dnFoeLevel(uint8_t myLevel, uint8_t floor) {
  int l = (int)myLevel - 5 + floor / 2;
  return l < 2 ? 2 : (l > BTL_LEVEL_CAP ? BTL_LEVEL_CAP : (uint8_t)l);
}

inline int16_t dnPickSpecies(DnState &s, uint8_t lvl) {
  for (int i = 0; i < 40; i++) {
    int16_t d = 1 + dnRand(s, DEX_COUNT);
    if (DEX_TBL[d].rarity != R_LEGENDARIO && btlMinLevel(d) <= lvl) return d;
  }
  return 16;  // Pidgey: forma base, sale a cualquier nivel
}

struct DnRoom { int8_t x, y, w, h; };

inline void dnCarve(DnState &s, int x0, int y0, int x1, int y1) {  // pasillo en L
  for (int x = x0; x != x1; x += x1 > x0 ? 1 : -1) s.map[y0][x] = DN_FLOOR;
  for (int y = y0; y != y1; y += y1 > y0 ? 1 : -1) s.map[y][x1] = DN_FLOOR;
  s.map[y1][x1] = DN_FLOOR;
}

inline void dnRoomSpot(DnState &s, const DnRoom &r, int8_t &x, int8_t &y) {
  x = r.x + dnRand(s, r.w);
  y = r.y + dnRand(s, r.h);
}

// una planta: seis salas unidas en serpiente, asi que siempre se llega a la escalera
inline void dnNewFloor(DnState &s) {
  for (int y = 0; y < DN_H; y++)
    for (int x = 0; x < DN_W; x++) s.map[y][x] = DN_WALL;

  DnRoom rooms[DN_ROOMS];
  static const uint8_t SNAKE[DN_ROOMS] = { 0, 1, 2, 5, 4, 3 };  // 0 1 2 arriba, 3 4 5 abajo
  for (int i = 0; i < DN_ROOMS; i++) {
    DnRoom &r = rooms[i];
    r.w = 3 + dnRand(s, 3);
    r.h = 3 + dnRand(s, 3);
    r.x = (i % 3) * 7 + 1 + dnRand(s, 6 - r.w);
    r.y = (i / 3) * 7 + 1 + dnRand(s, 6 - r.h);
    for (int y = r.y; y < r.y + r.h; y++)
      for (int x = r.x; x < r.x + r.w; x++) s.map[y][x] = DN_FLOOR;
  }
  for (int i = 0; i + 1 < DN_ROOMS; i++) {
    const DnRoom &a = rooms[SNAKE[i]], &b = rooms[SNAKE[i + 1]];
    dnCarve(s, a.x + a.w / 2, a.y + a.h / 2, b.x + b.w / 2, b.y + b.h / 2);
  }

  // se empieza en un extremo de la serpiente y la escalera esta en el otro
  bool flip = dnRand(s, 2);
  const DnRoom &first = rooms[flip ? 3 : 0], &last = rooms[flip ? 0 : 3];
  dnRoomSpot(s, first, s.x, s.y);
  int8_t sx, sy;
  dnRoomSpot(s, last, sx, sy);
  s.map[sy][sx] = DN_STAIRS;

  for (auto &m : s.mon) m.on = false;
  for (auto &b : s.berry) b.on = false;

  uint8_t lvl = dnFoeLevel(s.me.level, s.floor);
  int count = 2 + s.floor / 3;
  if (count > DN_MONS) count = DN_MONS;
  for (int i = 0; i < count; i++) {
    const DnRoom &r = rooms[SNAKE[1 + dnRand(s, DN_ROOMS - 1)]]; 
    int8_t x, y;
    dnRoomSpot(s, r, x, y);
    int dist = (x > s.x ? x - s.x : s.x - x) + (y > s.y ? y - s.y : s.y - y);
    if (dist < 4 || dnMonAt(s, x, y) >= 0 || s.map[y][x] == DN_STAIRS) continue;
    s.mon[i].f = btlMakeWild(dnPickSpecies(s, lvl), lvl, btlWildGene(dnRand(s, 16)));
    s.mon[i].x = x;
    s.mon[i].y = y;
    s.mon[i].on = true;
  }
  int berries = 1 + dnRand(s, 2);
  for (int i = 0; i < berries; i++) {
    int8_t x, y;
    dnRoomSpot(s, rooms[dnRand(s, DN_ROOMS)], x, y);
    if (s.map[y][x] == DN_STAIRS || (x == s.x && y == s.y) || dnMonAt(s, x, y) >= 0 || dnBerryAt(s, x, y) >= 0) continue;
    s.berry[i] = { x, y, true };
  }
}

// me ya trae nivel y estadisticas; la vida se multiplica aqui
inline void dnStart(DnState &s, const BtlFighter &me, uint32_t seed) {
  s.me = me;
  s.me.maxHp = me.maxHp * DN_HP_MULT;
  s.me.hp = s.me.maxHp;
  s.floor = 1;
  s.dead = s.ate = s.descended = false;
  s.kos = 0;
  s.hitCount = 0;
  s.faceX = 1;
  s.rng = seed ? seed : 1;
  dnNewFloor(s);
}

// el jugador no elige ataque: sale el que mas dana a ese rival
inline uint8_t dnBestMove(const DnState &s, const BtlFighter &foe) {
  BtlMoves ms = btlMovesFor(s.me.dex, s.me.level, foe.dex);
  uint8_t best = ms.ids[0];
  uint16_t top = 0;
  for (uint8_t i = 0; i < ms.count; i++) {
    uint16_t d = btlDamage(s.me.level, MOVE_TBL[ms.ids[i]], s.me.atk, foe.def, DEX_TBL[s.me.dex].type, DEX_TBL[foe.dex].type, 100);
    if (d > top) { top = d; best = ms.ids[i]; }
  }
  return best;
}

inline void dnAddHit(DnState &s, bool mine, int mon, uint8_t move, uint16_t dmg, uint8_t eff, bool ko) {
  if (s.hitCount <= DN_MONS) s.hits[s.hitCount++] = { mine, (int8_t)mon, move, dmg, eff, ko };
}

inline void dnFoesAct(DnState &s) {
  for (int i = 0; i < DN_MONS && !s.dead; i++) {
    DnMon &m = s.mon[i];
    if (!m.on) continue;
    int dx = s.x - m.x, dy = s.y - m.y;
    int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    if (adx + ady == 1) {
      BtlMoves ms = btlMovesFor(m.f.dex, m.f.level, s.me.dex);
      uint8_t id = ms.ids[btlWildMove(ms, dnRand(s, 100))];
      uint8_t mt = DEX_TBL[m.f.dex].type, mine = DEX_TBL[s.me.dex].type;
      uint16_t d = btlDamage(m.f.level, MOVE_TBL[id], m.f.atk, s.me.def, mt, mine, 85 + dnRand(s, 16));
      s.me.hp = d >= s.me.hp ? 0 : s.me.hp - d;
      s.dead = s.me.hp == 0;
      dnAddHit(s, false, i, id, d, btlEffect(MOVE_TBL[id].type, mine), s.dead);
    } else if (adx + ady <= DN_ALERT) {
      int8_t sx = dx > 0 ? 1 : (dx < 0 ? -1 : 0), sy = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
      bool horizFirst = adx >= ady;
      for (int t = 0; t < 2; t++) {
        bool h = (t == 0) == horizFirst;
        int nx = m.x + (h ? sx : 0), ny = m.y + (h ? 0 : sy);
        if ((nx != m.x || ny != m.y) && dnWalkable(s, nx, ny) && dnMonAt(s, nx, ny) < 0 && !(nx == s.x && ny == s.y)) {
          m.x = nx;
          m.y = ny;
          break;
        }
      }
    }
  }
}

// un paso del jugador (dx, dy en -1..1, uno de los dos a 0). false si no gasta el turno (pared).
inline bool dnStep(DnState &s, int8_t dx, int8_t dy) {
  if (s.dead) return false;
  int nx = s.x + dx, ny = s.y + dy;
  if (!dnWalkable(s, nx, ny)) return false;
  s.hitCount = 0;
  s.ate = s.descended = false;
  if (dx) s.faceX = dx;

  int target = dnMonAt(s, nx, ny);
  if (target >= 0) {
    DnMon &m = s.mon[target];
    uint8_t id = dnBestMove(s, m.f);
    uint8_t ft = DEX_TBL[m.f.dex].type;
    uint16_t d = btlDamage(s.me.level, MOVE_TBL[id], s.me.atk, m.f.def, DEX_TBL[s.me.dex].type, ft, 85 + dnRand(s, 16));
    m.f.hp = d >= m.f.hp ? 0 : m.f.hp - d;
    bool ko = m.f.hp == 0;
    dnAddHit(s, true, target, id, d, btlEffect(MOVE_TBL[id].type, ft), ko);
    if (ko) { m.on = false; s.kos++; }
  } else {
    s.x = nx;
    s.y = ny;
    if (s.map[ny][nx] == DN_STAIRS) {  // al bajar nadie te sigue
      if (s.floor < 255) s.floor++;
      uint32_t heal = (uint32_t)s.me.maxHp * DN_FLOOR_PCT / 100;
      s.me.hp = s.me.hp + heal > s.me.maxHp ? s.me.maxHp : s.me.hp + heal;
      s.descended = true;
      dnNewFloor(s);
      return true;
    }
    int b = dnBerryAt(s, nx, ny);
    if (b >= 0) {
      s.berry[b].on = false;
      uint32_t heal = (uint32_t)s.me.maxHp * DN_BERRY_PCT / 100;
      s.me.hp = s.me.hp + heal > s.me.maxHp ? s.me.maxHp : s.me.hp + heal;
      s.ate = true;
    }
  }
  dnFoesAct(s);
  return true;
}

// primer paso del camino mas corto a la escalera, pasando por encima de los rivales (se les
// ataca al llegar). Para las pruebas y para ver partidas largas en la placa.
inline bool dnBotDir(const DnState &s, int8_t &outX, int8_t &outY) {
  static int8_t fromX[DN_H][DN_W], fromY[DN_H][DN_W];
  static uint8_t seen[DN_H][DN_W];
  static uint16_t queue[DN_W * DN_H];
  for (int y = 0; y < DN_H; y++)
    for (int x = 0; x < DN_W; x++) seen[y][x] = 0;
  static const int8_t DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };
  int head = 0, tail = 0;
  queue[tail++] = s.y * DN_W + s.x;
  seen[s.y][s.x] = 1;
  while (head < tail) {
    int cy = queue[head] / DN_W, cx = queue[head] % DN_W;
    head++;
    if (s.map[cy][cx] == DN_STAIRS) {
      while (fromX[cy][cx] != s.x || fromY[cy][cx] != s.y) {
        int px = fromX[cy][cx], py = fromY[cy][cx];
        cx = px;
        cy = py;
      }
      outX = cx - s.x;
      outY = cy - s.y;
      return true;
    }
    for (int d = 0; d < 4; d++) {
      int nx = cx + DX[d], ny = cy + DY[d];
      if (!dnWalkable(s, nx, ny) || seen[ny][nx]) continue;
      seen[ny][nx] = 1;
      fromX[ny][nx] = cx;
      fromY[ny][nx] = cy;
      queue[tail++] = ny * DN_W + nx;
    }
  }
  return false;
}
