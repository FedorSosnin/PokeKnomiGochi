// Zonas de Combate (zones.h, zones_art.h, Pet::pickZoneSpecies).
#include "framework.h"
#include "shim/Arduino.h"
#include "shim/Preferences.h"
#include "../pet.h"
#include "../zones.h"
#include "../zones_art.h"
#include "../battle.h"
#include <set>

static void zoneCatchAllBut(Pet &p, uint8_t z, int16_t keep) {
  for (int i = 0; i < zoneSize(z); i++) {
    int16_t d = zoneMember(z, i);
    if (d != keep && d != ZONE_LEGEND[z]) p.catchWild(d);
  }
}

TEST(Zones, cubren_las_151_una_vez) {
  std::set<int16_t> seen;
  int total = 0;
  for (uint8_t z = 0; z < ZONE_COUNT; z++) {
    int n = zoneSize(z);
    total += n;
    for (int i = 0; i < n; i++) {
      int16_t d = zoneMember(z, i);
      CHECK(d >= 1 && d <= DEX_COUNT);
      CHECK(zoneOf(d) == z);
      CHECK(seen.insert(d).second);
    }
    CHECK_EQ(zoneMember(z, n), 0);
  }
  CHECK_EQ(total, 151);
}

TEST(Zones, un_legendario_al_final_de_cada_una) {
  std::set<int16_t> legends;
  for (uint8_t z = 0; z < ZONE_COUNT; z++) {
    int16_t l = ZONE_LEGEND[z];
    CHECK(DEX_TBL[l].rarity == R_LEGENDARIO);
    CHECK(zoneOf(l) == z);
    CHECK_EQ(zoneMember(z, zoneSize(z) - 1), l);
    legends.insert(l);
  }
  CHECK_EQ((int)legends.size(), (int)ZONE_COUNT);
}

TEST(Zones, tamanos_del_plan) {
  CHECK_EQ(zoneSize(Z_FOREST), 41);
  CHECK_EQ(zoneSize(Z_BEACH), 30);
  CHECK_EQ(zoneSize(Z_CAVE), 31);
  CHECK_EQ(zoneSize(Z_MANSION), 21);
  CHECK_EQ(zoneSize(Z_FIELD), 28);
}

TEST(Zones, el_rival_es_de_la_zona_y_respeta_el_nivel) {
  Pet p;
  for (uint8_t z = 0; z < ZONE_COUNT; z++)
    for (uint8_t lvl : { 5, 20, 60 })
      for (int i = 0; i < 400; i++) {
        int16_t d = p.pickZoneSpecies(z, lvl);
        CHECK(zoneOf(d) == z);
        CHECK(btlMinLevel(d) <= lvl);
        CHECK(d != ZONE_LEGEND[z]);
      }
}

TEST(Zones, prefiere_lo_que_falta) {
  Pet p;
  zoneCatchAllBut(p, Z_BEACH, 129);  // solo falta Magikarp
  int hits = 0, N = 2000;
  for (int i = 0; i < N; i++) hits += p.pickZoneSpecies(Z_BEACH, 60) == 129;
  CHECK(hits > N * (ZONE_NEW_PCT - 10) / 100);
}

TEST(Zones, legendario_solo_con_la_zona_completa) {
  Pet p;
  zoneCatchAllBut(p, Z_MANSION, 92);  // falta Gastly: el legendario no sale
  for (int i = 0; i < 2000; i++) CHECK(p.pickZoneSpecies(Z_MANSION, 60) != 150);
  p.catchWild(92);
  int hits = 0, N = 2000;
  for (int i = 0; i < N; i++) hits += p.pickZoneSpecies(Z_MANSION, 60) == 150;
  CHECK(hits > N * (ZONE_LEGEND_NEW_PCT - 8) / 100 && hits < N * (ZONE_LEGEND_NEW_PCT + 8) / 100);
  p.catchWild(150);  // atrapado: raro, pero no desaparece
  hits = 0;
  for (int i = 0; i < N; i++) hits += p.pickZoneSpecies(Z_MANSION, 60) == 150;
  CHECK(hits > 0 && hits < N * (ZONE_LEGEND_SEEN_PCT + 5) / 100);
}

TEST(Zones, el_escenario_cabe_y_cada_uno_es_distinto) {
  static uint16_t art[ZONE_COUNT][ZONE_ART * ZONE_ART];
  for (uint8_t z = 0; z < ZONE_COUNT; z++) {
    zoneArtDraw(art[z], z);
    std::set<uint16_t> colors(art[z], art[z] + ZONE_ART * ZONE_ART);
    CHECK((int)colors.size() >= 6);
    for (uint8_t o = 0; o < z; o++) CHECK(memcmp(art[z], art[o], sizeof(art[z])) != 0);
  }
}
