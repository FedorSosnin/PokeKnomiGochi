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
#define TOWER_DRAIN_PCT 30  // a guardian keeps its HP from one of yours to the next, and drains the one it beats
#define DEX_MEWTWO 150
#define TOWER_CHEER_MAX 3   // friends cheer after a lost climb: up to this many in a row...
#define TOWER_CHEER_PCT 10  // ...each this much more HP for the whole team, until a new best floor

// by step: floor n (1..8) at [n]; [9] the Legend's first fight, [10] Mewtwo (its own numbers: its base stats are the
// game's biggest, so the same training as the rest made it all but unbeatable)
static const uint8_t TOWER_TYPE[TOWER_LEGEND + 1] = { 0, TY_ROCK, TY_WATER, TY_ELECTRIC, TY_GRASS, TY_POISON, TY_PSYCHIC, TY_FIRE, TY_GROUND, TY_NORMAL };
static const uint8_t TOWER_LVL_BONUS[TOWER_STEPS + 1] = { 0, 0, 0, 1, 1, 2, 2, 2, 3, 2, 0 };
// (Fedor's target: a stacked team, Lv 100, three 3-star Pokemon, ATK and SPD trained full, DEF 25 (a daily player's:
// DEF grows 1 a 6 h of good care, so 60 is very rare), the right types and leads, clears the whole tower about 1 time in 4; tools/tower_sim.cpp and the climbs in test/test_tower.cpp check it)
static const uint8_t TOWER_TRAIN[TOWER_STEPS + 1] = { 0, 28, 40, 50, 56, 66, 72, 80, 86, 82, 58 };
static const uint8_t TOWER_GENE[TOWER_STEPS + 1] = { 0, 94, 96, 98, 100, 101, 102, 104, 106, 104, 100 };
// a guardian is a boss: more HP than its level gives, % (Fedor: "way too easy ... I got those pokeballs without even
// being hit"), so a fight lasts long enough to hurt
static const uint8_t TOWER_HP_PCT[TOWER_STEPS + 1] = { 0, 110, 115, 120, 125, 140, 165, 175, 190, 105, 100 };
static const uint8_t TOWER_LEGEND_TYPES[4] = { TY_ICE, TY_FIGHTING, TY_GHOST, TY_DRAGON };

// the floor a step of the climb is on: steps 1..8 are the floors, 9 and 10 the Legend
inline uint8_t towerFloorOf(uint8_t step) { return step > TOWER_FLOORS ? TOWER_LEGEND : step; }

// the gates, the only floors that pay, and only the first time (Fedor: "only getting one ultra ball at 8f, Nothing for
// 4F. This should be a legit challenge"): 8F an Ultra Ball, the Legend a Master Ball
inline uint8_t towerGateUltras(uint8_t floor) { return floor == 8 ? 1 : 0; }
inline bool towerIsGate(uint8_t floor) { return floor == 8 || floor == TOWER_LEGEND; }

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
  int l = petLvl + TOWER_LVL_BONUS[step];
  return l > BTL_LEVEL_CAP ? BTL_LEVEL_CAP : l < 2 ? 2 : (uint8_t)l;
}

// the guardian's species at a step (roll: any number; the Legend's first fight takes only its type from it: the
// strongest of that type, never a Dratini at the top)
inline int16_t towerGuardianDex(uint8_t step, uint8_t petLvl, uint8_t roll) {
  if (step == TOWER_STEPS) return DEX_MEWTWO;
  uint8_t lvl = towerLevel(step, petLvl);
  if (step == TOWER_LEGEND) return towerSpecies(TOWER_LEGEND_TYPES[roll % 4], lvl, 0);  // the Legend: the type's strongest
  return towerSpecies(TOWER_TYPE[step], lvl, roll);
}

// true when neither can hurt the other: none of either one's moves does anything to the other (a Normal Pokemon
// and a Ghost: the fight would never end)
inline bool towerNoHarm(int16_t a, uint8_t aLvl, int16_t b) {
  BtlMoves m = btlMovesFor(a, aLvl, b);
  for (int i = 0; i < m.count; i++)
    if (btlEffect(MOVE_TBL[m.ids[i]].type, DEX_TBL[b].type)) return false;
  return true;
}
inline bool towerStalemate(int16_t a, uint8_t aLvl, int16_t b, uint8_t bLvl) {
  return towerNoHarm(a, aLvl, b) && towerNoHarm(b, bLvl, a);
}

// the Legend's first guardian for this team: the rolled type, or the next of the four that none of the team is
// stuck against (a team with a Normal Pokemon never meets a Ghost it can't touch and that can't touch it)
inline int16_t towerLegendDex(uint8_t petLvl, uint8_t roll, const int16_t *team, uint8_t n, uint8_t teamLvl) {
  uint8_t gl = towerLevel(TOWER_LEGEND, petLvl);
  for (uint8_t k = 0; k < 4; k++) {
    int16_t d = towerGuardianDex(TOWER_LEGEND, petLvl, (uint8_t)(roll % 4 + k));
    bool stuck = false;
    for (uint8_t i = 0; i < n; i++) stuck = stuck || towerStalemate(team[i], teamLvl, d, gl);
    if (!stuck) return d;
  }
  return towerGuardianDex(TOWER_LEGEND, petLvl, roll);
}

inline BtlFighter towerGuardian(uint8_t step, int16_t dex, uint8_t petLvl) {
  uint8_t lvl = towerLevel(step, petLvl), tr = TOWER_TRAIN[step], g = TOWER_GENE[step];
  const DexEntry &d = DEX_TBL[dex];
  BtlFighter f;
  f.dex = dex;
  f.level = lvl;
  uint32_t hp = (uint32_t)btlMaxHp(d.bHp, lvl) * TOWER_HP_PCT[step] / 100;
  f.hp = f.maxHp = hp > 65535 ? 65535 : (uint16_t)hp;
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

// a guardian that beat one of yours drains it: this much of its own HP back (never past full)
inline uint16_t towerDrain(uint16_t hp, uint16_t maxHp) {
  uint32_t v = hp + (uint32_t)maxHp * TOWER_DRAIN_PCT / 100;
  return v > maxHp ? maxHp : (uint16_t)v;
}

// the average of the three training stats, for the "Climb over" bars
inline uint8_t towerTrainAvg(uint8_t a, uint8_t d, uint8_t s) { return (uint8_t)(((int)a + d + s) / 3); }

// ---- friends cheer (Fedor: kids slowly break through, strong players never see it): each climb lost in a row since
// the last new best floor gives the next climb's team TOWER_CHEER_PCT more HP, up to TOWER_CHEER_MAX times
inline uint16_t towerCheerHp(uint16_t hp, uint8_t cheer) {
  if (cheer > TOWER_CHEER_MAX) cheer = TOWER_CHEER_MAX;
  uint32_t v = (uint32_t)hp * (100 + TOWER_CHEER_PCT * cheer) / 100;
  return v > 65535 ? 65535 : (uint16_t)v;
}

// ---- the climb's end says what to try next (Fedor: "a kid who loses at 7F should know exactly what to try")
enum : uint8_t { TOWER_TIP_WEAK, TOWER_TIP_NOSTRONG, TOWER_TIP_ATK, TOWER_TIP_DEF, TOWER_TIP_SPD };
// the types Strong against a guardian's (the lead screen's hint), the best first: up to 2, how many
inline uint8_t towerStrongTypes(uint8_t foe, uint8_t out[2]) {
  uint8_t n = 0;
  for (int pass = 0; pass < 2 && n < 2; pass++)  // first those that hit it hard and take little back, then the rest
    for (uint8_t t = 0; t < TY_COUNT && n < 2; t++) {
      if (towerHint(t, foe) <= 0 || (n && out[0] == t)) continue;
      bool best = btlEffect(t, foe) > 2 && btlEffect(foe, t) <= 2;
      if (best == (pass == 0)) out[n++] = t;
    }
  return n;
}
// what the end screen says: the one beaten last was Weak against it, or nobody was Strong (both: try a type that is),
// or else the training the team is shortest of
inline uint8_t towerTip(int16_t lastDex, int16_t foeDex, const int16_t *team, uint8_t n, uint8_t ta, uint8_t td, uint8_t ts) {
  uint8_t foe = DEX_TBL[foeDex].type, strong[2];
  if (towerStrongTypes(foe, strong)) {
    if (lastDex && towerHint(DEX_TBL[lastDex].type, foe) < 0) return TOWER_TIP_WEAK;
    bool any = false;
    for (uint8_t i = 0; i < n; i++) any = any || towerHint(DEX_TBL[team[i]].type, foe) > 0;
    if (!any) return TOWER_TIP_NOSTRONG;
  }
  return td <= ta && td <= ts ? TOWER_TIP_DEF : ts <= ta ? TOWER_TIP_SPD : TOWER_TIP_ATK;
}

// ---- challenges, once the Legend is beaten (Fedor: the hard part is opt-in, not in a kid's way): a whole climb won...
#define TOWER_CH_NOFAINT 1  // ...with no one fainting
#define TOWER_CH_ONESTAR 2  // ...with two 1-star friends
#define TOWER_CH_BASE 4     // ...with no evolved Pokemon on the team
#define TOWER_CH_ALL 7
inline bool towerBaseForm(int16_t d) {  // nothing evolves into it (Eevee's three come from Eevee)
  if (d >= 134 && d <= 136) return false;
  for (int16_t e = 1; e <= DEX_COUNT; e++)
    if (DEX_TBL[e].evolvesTo == d) return false;
  return true;
}
// the challenges a whole climb won meets: faints, the team (your Pokemon first), the friends' stars (stars[1], [2])
inline uint8_t towerChallenges(uint8_t faints, const int16_t *team, const uint8_t *stars, uint8_t n) {
  uint8_t got = faints ? 0 : TOWER_CH_NOFAINT;
  if (n == TOWER_TEAM && stars[1] == 1 && stars[2] == 1) got |= TOWER_CH_ONESTAR;
  bool base = n > 0;
  for (uint8_t i = 0; i < n; i++) base = base && towerBaseForm(team[i]);
  if (base) got |= TOWER_CH_BASE;
  return got;
}
