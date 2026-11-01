// Reglas de Dungeon Dive (dungeon.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "shim/Preferences.h"
#include "../dungeon.h"
#include "../pet.h"
#include <algorithm>
#include <stdio.h>
#include <vector>

static BtlFighter fighter(int16_t dex, uint8_t lvl, uint8_t trained) {
  const DexEntry &d = DEX_TBL[dex];
  BtlFighter f;
  f.dex = dex;
  f.level = lvl;
  f.hp = f.maxHp = btlMaxHp(d.bHp, lvl);
  f.atk = btlStat(d.bAtk, 100, lvl, trained);
  f.def = btlStat(d.bDef, 100, lvl, 0);
  f.spe = btlStat(d.bSpe, 100, lvl, 0);
  return f;
}

// juega con el camino mas corto hasta caer; devuelve la planta alcanzada
static int botDepth(int16_t dex, uint8_t lvl, uint8_t trained, uint32_t seed) {
  static DnState s;
  dnStart(s, fighter(dex, lvl, trained), seed);
  for (int i = 0; i < 6000 && !s.dead; i++) {
    int8_t dx = 0, dy = 0;
    if (!dnBotDir(s, dx, dy) || !dnStep(s, dx, dy)) return -1;
  }
  return s.floor;
}

static int medianDepth(int16_t dex, uint8_t lvl, uint8_t trained) {
  std::vector<int> d;
  for (uint32_t i = 1; i <= 101; i++) d.push_back(botDepth(dex, lvl, trained, i * 2654435761u));
  std::sort(d.begin(), d.end());
  return d[50];
}

TEST(dungeon, siempre_se_llega_a_la_escalera) {
  static DnState s;
  for (uint32_t seed = 1; seed <= 500; seed++) {
    dnStart(s, fighter(1, 20, 0), seed * 2654435761u);
    int8_t dx, dy;
    CHECK(dnWalkable(s, s.x, s.y));
    CHECK(dnBotDir(s, dx, dy));
    for (int x = 0; x < DN_W; x++) CHECK(s.map[0][x] == DN_WALL && s.map[DN_H - 1][x] == DN_WALL);
    for (int y = 0; y < DN_H; y++) CHECK(s.map[y][0] == DN_WALL && s.map[y][DN_W - 1] == DN_WALL);
  }
}

TEST(dungeon, los_rivales_salen_lejos_y_en_el_suelo) {
  static DnState s;
  for (uint32_t seed = 1; seed <= 200; seed++) {
    dnStart(s, fighter(25, 30, 0), seed * 2654435761u);
    for (auto &m : s.mon) {
      if (!m.on) continue;
      CHECK(dnWalkable(s, m.x, m.y));
      CHECK(abs(m.x - s.x) + abs(m.y - s.y) >= 4);
      CHECK(DEX_TBL[m.f.dex].rarity != R_LEGENDARIO);
      CHECK(btlMinLevel(m.f.dex) <= m.f.level);
    }
  }
}

TEST(dungeon, la_pared_no_gasta_el_turno) {
  static DnState s;
  dnStart(s, fighter(1, 20, 0), 7);
  for (auto &m : s.mon) m.on = false;
  s.map[s.y][s.x + 1] = DN_WALL;
  int x = s.x;
  CHECK(!dnStep(s, 1, 0));
  CHECK_EQ((int)s.x, x);
}

TEST(dungeon, atacar_entrando_en_la_casilla_del_rival) {
  static DnState s;
  dnStart(s, fighter(4, 30, 0), 11);
  for (auto &m : s.mon) m.on = false;
  s.map[s.y][s.x + 1] = DN_FLOOR;
  s.mon[0].f = btlMakeWild(1, 10, 80);
  s.mon[0].x = s.x + 1;
  s.mon[0].y = s.y;
  s.mon[0].on = true;
  int x = s.x;
  uint16_t hp = s.mon[0].f.hp;
  CHECK(dnStep(s, 1, 0));
  CHECK_EQ((int)s.x, x);
  CHECK_EQ(s.hits[0].mine, true);
  CHECK(!s.mon[0].on || s.mon[0].f.hp < hp);
  CHECK_MSG(strcmp(MOVE_TBL[s.hits[0].move].name, "EMBER") == 0 || strcmp(MOVE_TBL[s.hits[0].move].name, "SLASH") == 0,
            "usa su mejor ataque");
}

TEST(dungeon, el_rival_adyacente_golpea) {
  static DnState s;
  dnStart(s, fighter(4, 30, 0), 11);
  for (auto &m : s.mon) m.on = false;
  s.mon[0].f = btlMakeWild(1, 30, 80);
  s.mon[0].x = s.x;
  s.mon[0].y = s.y;
  s.mon[0].on = true;  // misma casilla no; se recoloca abajo
  s.map[s.y][s.x + 1] = DN_FLOOR;
  s.mon[0].x = s.x + 1;
  uint16_t hp = s.me.hp;
  dnFoesAct(s);
  CHECK(s.me.hp < hp);
}

TEST(dungeon, la_baya_cura_y_la_escalera_baja) {
  static DnState s;
  dnStart(s, fighter(7, 20, 0), 3);
  for (auto &m : s.mon) m.on = false;
  s.me.hp = 1;
  s.map[s.y][s.x + 1] = DN_FLOOR;
  s.berry[0] = { (int8_t)(s.x + 1), s.y, true };
  CHECK(dnStep(s, 1, 0));
  CHECK(s.ate);
  CHECK(s.me.hp > 1);
  uint16_t hp = s.me.hp;
  for (auto &m : s.mon) m.on = false;
  s.map[s.y][s.x + 1] = DN_STAIRS;
  CHECK(dnStep(s, 1, 0));
  CHECK(s.descended);
  CHECK_EQ((int)s.floor, 2);
  CHECK(s.me.hp > hp);
}

TEST(dungeon, entrenar_y_evolucionar_llega_mas_hondo) {
  int fresh = medianDepth(1, 5, 0), mid = medianDepth(2, 20, 20), strong = medianDepth(3, 40, 40), wrecked = medianDepth(129, 5, 0);
  printf("  medianas: nuevo %d, medio %d, fuerte %d, magikarp %d\n", fresh, mid, strong, wrecked);
  CHECK_RANGE(fresh, 2, 12);
  CHECK_RANGE(mid, 4, 20);
  CHECK_RANGE(strong, 5, 30);
  CHECK(strong >= fresh);
  CHECK_MSG(medianDepth(1, 20, 60) > medianDepth(1, 20, 0), "entrenar se nota");
}

TEST(dungeon, el_record_se_guarda_y_lo_viejo_no_cambia) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.chooseStarter(1);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_EQ((int)p.dungeonHi, 0);
  p.dungeonResult(4);
  p.dungeonResult(2);
  CHECK_EQ((int)p.dungeonHi, 4);
  Pet q;
  q.begin();
  CHECK_EQ((int)q.dungeonHi, 4);
}
