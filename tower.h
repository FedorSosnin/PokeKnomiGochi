#pragma once
#include <stdint.h>
#include "battle.h"

// Knomi Tower's rules, without hardware or game state (tested in test/test_tower.cpp). The plan, the mockups and
// the battle simulator these numbers came from: side-session-notes .../knomi-tower/ (PLAN.md, sim/cfg_t3.py).
//
// A climb starts at 1F every visit, with your Pokemon and 2 friends from the Pokedex (at your Pokemon's level and
// training). Each of the 8 floors has one guardian of its type; the Legend at the top has two fights, a guardian
// of Ice, Fighting, Ghost or Dragon and then Mewtwo. The guardians match your level (a floor's offset on top), so
// raising a Pokemon for longer gives no edge by level: training, stars and choosing who fights do. HP carries over
// from fight to fight, with a little back between floors. Lose all three and the climb is over.

#define TOWER_FLOORS 8      // 1F..8F, one guardian each
#define TOWER_LEGEND 9      // the Legend: its two fights are steps 9 and 10
#define TOWER_STEPS 10      // fights in a whole climb
#define TOWER_TEAM 3        // your Pokemon and two friends
#define TOWER_ENERGY_MIN 90 // a climb needs a full bar...
#define TOWER_FIGHT_ENERGY 9  // ...and each fight won uses this much: a whole climb uses it all
#define TOWER_HEAL_PCT 20   // HP back between floors, for each one still standing
#define DEX_MEWTWO 150

// floor n (1..8) at [n]; [9] is the Legend
static const uint8_t TOWER_TYPE[TOWER_LEGEND + 1] = { 0, TY_ROCK, TY_WATER, TY_ELECTRIC, TY_GRASS, TY_POISON, TY_PSYCHIC, TY_FIRE, TY_GROUND, TY_NORMAL };
static const uint8_t TOWER_LVL_BONUS[TOWER_LEGEND + 1] = { 0, 0, 0, 1, 1, 2, 2, 2, 3, 3 };
static const uint8_t TOWER_TRAIN[TOWER_LEGEND + 1] = { 0, 28, 40, 50, 58, 66, 74, 82, 88, 90 };
static const uint8_t TOWER_GENE[TOWER_LEGEND + 1] = { 0, 94, 96, 98, 100, 101, 102, 104, 106, 106 };
static const uint8_t TOWER_LEGEND_TYPES[4] = { TY_ICE, TY_FIGHTING, TY_GHOST, TY_DRAGON };

// the floor a step of the climb is on: steps 1..8 are the floors, 9 and 10 the Legend
inline uint8_t towerFloorOf(uint8_t step) { return step > TOWER_FLOORS ? TOWER_LEGEND : step; }

// a gate's reward the first time it's passed: Ultra Balls (4F: 1, 8F: 2); the Legend gives a Master Ball instead
inline uint8_t towerGateUltras(uint8_t floor) { return floor == 4 ? 1 : floor == 8 ? 2 : 0; }
inline bool towerIsGate(uint8_t floor) { return floor == 4 || floor == 8 || floor == TOWER_LEGEND; }

inline uint16_t towerBst(int16_t d) { const DexEntry &e = DEX_TBL[d]; return e.bHp + e.bAtk + e.bDef + e.bSpe; }

// the guardian's species: one of the three strongest of the type that its level allows, legendaries never (roll
// picks among them). With none allowed yet (an Ice guardian at a low level), the base form with the most strength.
inline int16_t towerSpecies(uint8_t type, uint8_t lvl, uint8_t roll) {
  int16_t top[3] = { 0, 0, 0 };
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    const DexEntry &e = DEX_TBL[d];
    if (e.type != type || e.rarity == R_LEGENDARY || btlMinLevel(d) > lvl) continue;
    for (int k = 0; k < 3; k++) {
      if (!top[k] || towerBst(d) > towerBst(top[k])) {
        for (int j = 2; j > k; j--) top[j] = top[j - 1];
        top[k] = d;
        break;
      }
    }
  }
  int n = top[2] ? 3 : top[1] ? 2 : top[0] ? 1 : 0;
  if (n) return top[roll % n];
  int16_t best = 0;  // nothing allowed at this level: the strongest of the type, whatever its level
  for (int16_t d = 1; d <= DEX_COUNT; d++)
    if (DEX_TBL[d].type == type && DEX_TBL[d].rarity != R_LEGENDARY && (!best || towerBst(d) > towerBst(best))) best = d;
  return best ? best : 1;
}

inline uint8_t towerLevel(uint8_t step, uint8_t petLvl) {
  int l = petLvl + TOWER_LVL_BONUS[towerFloorOf(step)];
  return l > BTL_LEVEL_CAP ? BTL_LEVEL_CAP : l < 2 ? 2 : (uint8_t)l;
}

// the guardian's species at a step (roll: any number; the Legend's first fight takes its type from it)
inline int16_t towerGuardianDex(uint8_t step, uint8_t petLvl, uint8_t roll) {
  if (step == TOWER_STEPS) return DEX_MEWTWO;
  uint8_t lvl = towerLevel(step, petLvl);
  if (step == TOWER_LEGEND) return towerSpecies(TOWER_LEGEND_TYPES[roll % 4], lvl, roll / 4);
  return towerSpecies(TOWER_TYPE[step], lvl, roll);
}

inline BtlFighter towerGuardian(uint8_t step, int16_t dex, uint8_t petLvl) {
  uint8_t fl = towerFloorOf(step), lvl = towerLevel(step, petLvl), tr = TOWER_TRAIN[fl], g = TOWER_GENE[fl];
  const DexEntry &d = DEX_TBL[dex];
  BtlFighter f;
  f.dex = dex;
  f.level = lvl;
  f.hp = f.maxHp = btlMaxHp(d.bHp, lvl);
  f.atk = btlStat(d.bAtk, g, lvl, tr);
  f.def = btlStat(d.bDef, g, lvl, tr);
  f.spe = btlStat(d.bSpe, g, lvl, tr);
  return f;
}

// a friend's genes from the stars you caught it with (the middle of each star's band)
inline uint8_t towerStarGene(uint8_t stars) { return stars >= 3 ? 107 : stars == 2 ? 100 : 93; }

// a friend from the Pokedex: your Pokemon's level and training, its own stars
inline BtlFighter towerFriend(int16_t dex, uint8_t lvl, uint8_t stars, uint8_t trAtk, uint8_t trDef, uint8_t trSpe) {
  const DexEntry &d = DEX_TBL[dex];
  uint8_t g = towerStarGene(stars);
  BtlFighter f;
  f.dex = dex;
  f.level = lvl;
  f.hp = f.maxHp = btlMaxHp(d.bHp, lvl);
  f.atk = btlStat(d.bAtk, g, lvl, trAtk);
  f.def = btlStat(d.bDef, g, lvl, trDef);
  f.spe = btlStat(d.bSpe, g, lvl, trSpe);
  return f;
}

// the lead screen's hint, from the type chart both ways: 1 Strong, -1 Weak, 0 neither
inline int8_t towerHint(uint8_t mine, uint8_t theirs) {
  int s = (int)btlEffect(mine, theirs) - (int)btlEffect(theirs, mine);
  return s > 0 ? 1 : s < 0 ? -1 : 0;
}

// HP back between floors (never past full)
inline uint16_t towerHeal(uint16_t hp, uint16_t maxHp) {
  if (!hp) return 0;
  uint32_t v = hp + (uint32_t)maxHp * TOWER_HEAL_PCT / 100;
  return v > maxHp ? maxHp : (uint16_t)v;
}

// the average of the three training stats, for the "Climb over" bars
inline uint8_t towerTrainAvg(uint8_t a, uint8_t d, uint8_t s) { return (uint8_t)(((int)a + d + s) / 3); }
