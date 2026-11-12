#pragma once
#include <stdint.h>
#include "dex.h"

// Battle Zones. Assigned by hand per habitat, not by type: each species has only one type and
// none is Flying, so type is no use for putting the birds in the field or Zubat
// in the cave. Each zone has a legendary, found on any Search but seldom.
enum : uint8_t { Z_FOREST, Z_BEACH, Z_CAVE, Z_MANSION, Z_FIELD, ZONE_COUNT };

static const uint8_t ZONE_OF[DEX_COUNT] = {
  0, 0, 0, 4, 4, 4, 1, 1, 1, 0,  // 1
  0, 0, 0, 0, 0, 4, 4, 4, 4, 4,  // 11
  4, 4, 0, 0, 0, 0, 2, 2, 0, 0,  // 21
  0, 0, 0, 0, 2, 2, 0, 0, 4, 4,  // 31
  2, 2, 0, 0, 0, 0, 0, 0, 0, 2,  // 41
  2, 0, 0, 1, 1, 0, 0, 4, 4, 1,  // 51
  1, 1, 3, 3, 3, 2, 2, 2, 0, 0,  // 61
  0, 1, 1, 2, 2, 2, 4, 4, 1, 1,  // 71
  3, 3, 4, 4, 4, 1, 1, 3, 3, 1,  // 81
  1, 3, 3, 3, 2, 3, 3, 1, 1, 3,  // 91
  3, 0, 0, 2, 2, 2, 2, 4, 3, 3,  // 101
  2, 2, 4, 0, 4, 1, 1, 1, 1, 1,  // 111
  1, 3, 0, 1, 3, 2, 0, 4, 1, 1,  // 121
  1, 3, 4, 1, 4, 4, 3, 2, 2, 2,  // 131
  2, 2, 4, 1, 4, 2, 2, 2, 2, 3,  // 141
  0,  // 151
};

static const uint8_t ZONE_LEGEND[ZONE_COUNT] = { 151, 144, 146, 150, 145 };  // Mew, Articuno, Moltres, Mewtwo, Zapdos

// A zone's Pokemon come in three kinds. The legendary can turn up on any Search, from the first,
// but only about one in a hundred. The others are drawn by weight: a common first form most often,
// its evolved forms half as often, and a rare one (and what it evolves into) a third as often again.
// Searches lean a little toward the common ones still missing, never toward the rare or the legend:
// filling a zone takes finding them. (V2 leans toward the rare ones too: below.)
#define ZONE_LEGEND_PERMIL 10    // the legendary: 10 Searches in 1000
#define ZONE_W_COMMON 6          // weights of the rest
#define ZONE_W_COMMON_EVO 3
#define ZONE_W_RARE 2
#define ZONE_W_RARE_EVO 1
#define ZONE_NEW_PCT 40          // how often a Search looks only among the common ones you're missing
// V2: a Search leans toward the ones you're missing as a Wild Encounter does, rares too (each one left out of
// that look 60 times in 100, as there), so searching the right zone is never the slow way to find one
// (Fedor looked for Jynx at the Beach for three days; Wild Encounters found her at once)
#define ZONE_V2_NEW_PCT 70       // how often a Search looks only among the ones you're missing
#define ZONE_V2_RARE_SKIP 60     // and how often a missing rare one sits that look out

inline uint8_t zoneOf(int16_t dex) { return ZONE_OF[dex - 1]; }

// the first form of a species' line (Eevee for its three)
inline int16_t lineBase(int16_t d) {
  if (d > DEX_EEVEE && d <= DEX_EEVEE + 3) return DEX_EEVEE;
  for (bool moved = true; moved;) {
    moved = false;
    for (int16_t s = 1; s <= DEX_COUNT; s++)
      if (DEX_TBL[s].evolvesTo == d) { d = s; moved = true; break; }
  }
  return d;
}

// rare: its line starts with a rare Pokemon (Scyther, Lapras, Dratini and on to Dragonite...)
inline bool zoneIsRare(int16_t d) { return DEX_TBL[lineBase(d)].rarity == R_RARE; }

// how often a Search turns it up, against the others of its zone (the legendary is apart)
inline uint8_t zoneWeight(int16_t d) {
  bool evolved = lineBase(d) != d;
  if (zoneIsRare(d)) return evolved ? ZONE_W_RARE_EVO : ZONE_W_RARE;
  return evolved ? ZONE_W_COMMON_EVO : ZONE_W_COMMON;
}

inline int zoneSize(uint8_t z) {
  int n = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) n += zoneOf(d) == z;
  return n;
}

// the i-th of the zone in Pokedex order, with the legendary always last. 0 = none
inline int16_t zoneMember(uint8_t z, int i) {
  for (int16_t d = 1; d <= DEX_COUNT; d++)
    if (zoneOf(d) == z && d != ZONE_LEGEND[z] && i-- == 0) return d;
  return i == 0 ? ZONE_LEGEND[z] : 0;
}
