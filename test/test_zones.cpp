// Battle zones (zones.h, zones_art.h, Pet::pickZoneSpecies).
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

TEST(Zones, cover_all_151_exactly_once) {
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

TEST(Zones, one_legendary_at_the_end_of_each) {
  std::set<int16_t> legends;
  for (uint8_t z = 0; z < ZONE_COUNT; z++) {
    int16_t l = ZONE_LEGEND[z];
    CHECK(DEX_TBL[l].rarity == R_LEGENDARY);
    CHECK(zoneOf(l) == z);
    CHECK_EQ(zoneMember(z, zoneSize(z) - 1), l);
    legends.insert(l);
  }
  CHECK_EQ((int)legends.size(), (int)ZONE_COUNT);
}

TEST(Zones, planned_sizes) {
  CHECK_EQ(zoneSize(Z_FOREST), 41);
  CHECK_EQ(zoneSize(Z_BEACH), 30);
  CHECK_EQ(zoneSize(Z_CAVE), 31);
  CHECK_EQ(zoneSize(Z_MANSION), 21);
  CHECK_EQ(zoneSize(Z_FIELD), 28);
}

TEST(Zones, rival_belongs_to_zone_and_respects_level) {
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

TEST(Zones, prefers_what_is_missing) {
  Pet p;
  zoneCatchAllBut(p, Z_BEACH, 129);  // only Magikarp is missing
  int hits = 0, N = 2000;
  for (int i = 0; i < N; i++) hits += p.pickZoneSpecies(Z_BEACH, 60) == 129;
  CHECK(hits > N * (ZONE_NEW_PCT - 10) / 100);
}

TEST(Zones, legendary_only_with_complete_zone) {
  Pet p;
  zoneCatchAllBut(p, Z_MANSION, 92);  // Gastly is missing: the legendary does not appear
  for (int i = 0; i < 2000; i++) CHECK(p.pickZoneSpecies(Z_MANSION, 60) != 150);
  p.catchWild(92);
  int hits = 0, N = 2000;
  for (int i = 0; i < N; i++) hits += p.pickZoneSpecies(Z_MANSION, 60) == 150;
  CHECK(hits > N * (ZONE_LEGEND_NEW_PCT - 8) / 100 && hits < N * (ZONE_LEGEND_NEW_PCT + 8) / 100);
  p.catchWild(150);  // caught: rare, but does not disappear
  hits = 0;
  for (int i = 0; i < N; i++) hits += p.pickZoneSpecies(Z_MANSION, 60) == 150;
  CHECK(hits > 0 && hits < N * (ZONE_LEGEND_SEEN_PCT + 5) / 100);
}

TEST(Zones, scene_fits_and_each_is_distinct) {
  static uint16_t art[ZONE_COUNT][ZONE_ART * ZONE_ART];
  for (uint8_t z = 0; z < ZONE_COUNT; z++) {
    zoneArtDraw(art[z], z);
    std::set<uint16_t> colors(art[z], art[z] + ZONE_ART * ZONE_ART);
    CHECK((int)colors.size() >= 6);
    for (uint8_t o = 0; o < z; o++) CHECK(memcmp(art[z], art[o], sizeof(art[z])) != 0);
  }
}

// every species can be caught through the zones: each is in one, and shows up there once
// your Pokemon is strong enough (the wild level reaches its evolution level)
TEST(Zones, all_151_can_be_caught) {
  int maxFoe = 0;
  for (int l = 1; l <= 100; l++)
    for (int r = 0; r < 4; r++) {
      int f = btlWildLevel(btlCapLevel(l), r);
      if (f > maxFoe) maxFoe = f;
    }
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    CHECK(zoneOf(d) < ZONE_COUNT);
    CHECK_MSG(btlMinLevel(d) <= maxFoe, "a species no wild battle reaches");
  }
  for (uint8_t z = 0; z < ZONE_COUNT; z++) CHECK_EQ((int)zoneOf(ZONE_LEGEND[z]), (int)z);
  // and the picker really hands each one out: catch everything a zone offers until it's full
  Pet p;
  p.speciesId = 25;
  p.noSave = true;
  for (uint8_t z = 0; z < ZONE_COUNT; z++)
    for (int i = 0; i < 20000 && p.zoneCaught(z) < zoneSize(z); i++) p.catchWild(p.pickZoneSpecies(z, 100));
  CHECK_EQ((int)p.registeredCount(), 151);
}

TEST(Zones, a_full_zone_earns_its_medal) {
  Pet p;
  p.speciesId = 25;
  p.noSave = true;
  zoneCatchAllBut(p, Z_CAVE, 0);  // all but the legendary
  CHECK(!p.hasMedal(MED_ZONE_CAVE));
  p.catchWild(ZONE_LEGEND[Z_CAVE]);
  CHECK(p.hasMedal(MED_ZONE_CAVE));
  CHECK(!p.hasMedal(MED_ZONE_FOREST));
  CHECK_EQ(MED_ZONE_FOREST, 1UL << MED_FIRST_ZONE);
  CHECK_EQ(MED_ZONE_FIELD, 1UL << (MED_FIRST_ZONE + Z_FIELD));
  CHECK_EQ(MED_FIRST_ZONE + ZONE_COUNT, MED_COUNT);
}
