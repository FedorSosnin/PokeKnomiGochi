// Knomi Tower's rules (tower.h), and a simulator of whole climbs with the game's own battle rules, so a later
// change to the numbers can't make the Legend free or the first gate a walk for a new Pokemon.
#include "framework.h"
#include "shim/Arduino.h"
#include "../tower.h"
#include <stdlib.h>

TEST(tower, floors_types_and_gates) {
  CHECK_EQ((int)TOWER_TYPE[1], (int)TY_ROCK);
  CHECK_EQ((int)TOWER_TYPE[8], (int)TY_GROUND);
  CHECK(towerIsGate(8) && towerIsGate(TOWER_LEGEND));
  CHECK(!towerIsGate(1) && !towerIsGate(4) && !towerIsGate(7));
  CHECK_EQ((int)towerGateUltras(4), 0);  // 4F pays nothing: the balls are a real challenge (Fedor)
  CHECK_EQ((int)towerGateUltras(8), 1);
  CHECK_EQ((int)towerGateUltras(5), 0);
  CHECK_EQ((int)towerFloorOf(9), TOWER_LEGEND);
  CHECK_EQ((int)towerFloorOf(10), TOWER_LEGEND);
  CHECK_EQ(TOWER_FIGHT_ENERGY * TOWER_STEPS, TOWER_ENERGY_MIN);  // a whole climb uses the whole bar
}

TEST(tower, guardians_match_your_level_and_are_never_legendary) {
  for (int lvl = 5; lvl <= 100; lvl += 5)
    for (uint8_t step = 1; step < TOWER_STEPS; step++)
      for (uint8_t roll = 0; roll < 12; roll++) {
        int16_t d = towerGuardianDex(step, lvl, roll);
        CHECK(d >= 1 && d <= DEX_COUNT);
        CHECK(DEX_TBL[d].rarity != R_LEGENDARY);
        if (step <= TOWER_FLOORS) CHECK_EQ((int)DEX_TBL[d].type, (int)TOWER_TYPE[step]);
        int gl = towerLevel(step, lvl);
        CHECK(gl >= lvl && gl <= lvl + 3);
      }
  CHECK_EQ((int)towerGuardianDex(TOWER_STEPS, 50, 7), DEX_MEWTWO);
  for (uint8_t roll = 0; roll < 8; roll++) CHECK(towerGuardianDex(TOWER_LEGEND, 100, roll) != 147);  // never Dratini at the top
}

TEST(tower, a_low_level_meets_base_forms) {
  // a Lv 15 climb meets Pikachu, not Raichu, on the Electric floor
  for (uint8_t roll = 0; roll < 6; roll++) CHECK(towerGuardianDex(3, 14, roll) != 26);
}

TEST(tower, friends_take_your_training_and_their_stars) {
  BtlFighter a = towerFriend(6, 50, 3, 100, 10, 80), b = towerFriend(6, 50, 1, 100, 10, 80);
  CHECK(a.atk > b.atk && a.def > b.def && a.spe > b.spe);
  CHECK_EQ((int)a.hp, (int)b.hp);
  BtlFighter c = towerFriend(6, 50, 3, 0, 0, 0);
  CHECK_EQ((int)(a.atk - c.atk), 100);
}

TEST(tower, guardians_are_bosses_and_drain) {
  BtlFighter a = towerGuardian(5, 66, 60);
  CHECK(a.maxHp > btlMaxHp(DEX_TBL[66].bHp, towerLevel(5, 60)));  // more HP than its level gives
  CHECK_EQ((int)towerDrain(10, 100), 10 + TOWER_DRAIN_PCT);
  CHECK_EQ((int)towerDrain(90, 100), 100);
}

TEST(tower, hints_and_healing) {
  CHECK_EQ((int)towerHint(TY_FIRE, TY_GRASS), 1);
  CHECK_EQ((int)towerHint(TY_WATER, TY_GRASS), -1);
  CHECK_EQ((int)towerHint(TY_NORMAL, TY_NORMAL), 0);
  CHECK_EQ((int)towerHeal(0, 100), 0);  // the fainted stay fainted
  CHECK_EQ((int)towerHeal(50, 100), 70);
  CHECK_EQ((int)towerHeal(95, 100), 100);
}

// ---- whole climbs, as sim/runs.py does them
struct SimMon { BtlFighter f; };
static uint32_t gSeed = 1;
static int rnd(int n) { gSeed = gSeed * 1103515245u + 12345u; return (int)((gSeed >> 16) % (uint32_t)n); }

static uint16_t hitFor(const BtlFighter &a, const BtlFighter &t, uint8_t mv, uint8_t roll) {
  return btlDamage(a.level, MOVE_TBL[mv], a.atk, t.def, DEX_TBL[a.dex].type, DEX_TBL[t.dex].type, roll);
}
static bool simFight(BtlFighter &me, BtlFighter &foe) {
  BtlMoves mm = btlMovesFor(me.dex, me.level, foe.dex), fm = btlMovesFor(foe.dex, foe.level, me.dex);
  uint8_t pick = mm.ids[0];
  for (int i = 0; i < mm.count; i++)
    if (hitFor(me, foe, mm.ids[i], 100) > hitFor(me, foe, pick, 100)) pick = mm.ids[i];
  for (int turn = 0; turn < 200; turn++) {
    bool first = btlPlayerFirst(me.spe, foe.spe, rnd(2));
    uint8_t fmv = fm.ids[btlWildMove(fm, rnd(100))];
    for (int k = 0; k < 2; k++) {
      bool mine = (k == 0) == first;
      BtlFighter &a = mine ? me : foe, &t = mine ? foe : me;
      uint16_t dmg = hitFor(a, t, mine ? pick : fmv, 85 + rnd(16));
      t.hp = dmg >= t.hp ? 0 : t.hp - dmg;
      if (!foe.hp) return true;
      if (!me.hp) return false;
    }
  }
  return false;
}
static int simScore(const BtlFighter &m, const BtlFighter &foe) {  // what the lead screen's hint says, then HP
  return towerHint(DEX_TBL[m.dex].type, DEX_TBL[foe.dex].type) * 1000 + m.hp * 100 / m.maxHp;
}
// how far a climb gets: 0 none, 1..8 floors, 9 the Legend's first fight, 10 the whole tower
static int simClimb(const int16_t team[3], uint8_t lvl, uint8_t stars, uint8_t ta, uint8_t td, uint8_t ts) {
  BtlFighter t[3];
  for (int i = 0; i < 3; i++) t[i] = towerFriend(team[i], lvl, stars, ta, td, ts);
  int reached = 0;
  for (uint8_t step = 1; step <= TOWER_STEPS; step++) {
    BtlFighter foe = towerGuardian(step, towerGuardianDex(step, lvl, rnd(250)), lvl);
    for (;;) {
      int best = -1;
      for (int i = 0; i < 3; i++)
        if (t[i].hp && (best < 0 || simScore(t[i], foe) > simScore(t[best], foe))) best = i;
      if (best < 0) return reached;
      if (simFight(t[best], foe)) break;
      foe.hp = towerDrain(foe.hp, foe.maxHp);  // it keeps its HP, and drains the one it beat
    }
    reached = step;
    if (step < TOWER_STEPS)
      for (auto &m : t) m.hp = towerHeal(m.hp, m.maxHp);
  }
  return reached;
}
static int simPct(const int16_t team[3], uint8_t lvl, uint8_t stars, uint8_t ta, uint8_t td, uint8_t ts, int atLeast) {
  int n = 400, ok = 0;
  for (int i = 0; i < n; i++) ok += simClimb(team, lvl, stars, ta, td, ts) >= atLeast;
  return ok * 100 / n;
}

TEST(tower, a_new_team_stops_before_the_first_gate) {
  gSeed = 11;
  const int16_t team[3] = { 4, 7, 25 };  // Charmander, Squirtle, Pikachu at Lv 15, a little training
  CHECK(simPct(team, 15, 2, 30, 2, 20, 1) >= 80);  // 1F is a first win
  CHECK(simPct(team, 15, 2, 30, 2, 20, 4) <= 10);  // Gate 1 isn't
}

TEST(tower, a_mid_team_passes_gate_1_but_not_gate_2) {
  gSeed = 12;
  const int16_t team[3] = { 6, 9, 26 };
  CHECK(simPct(team, 45, 2, 80, 6, 60, 4) >= 20);
  CHECK(simPct(team, 45, 2, 80, 6, 60, 8) <= 10);
}

TEST(tower, the_legend_is_hard_even_for_the_best) {
  gSeed = 13;
  const int16_t team[3] = { 6, 9, 26 };
  int top = simPct(team, 100, 3, 100, 15, 100, TOWER_STEPS);
  CHECK(top >= 3 && top <= 30);
  CHECK(simPct(team, 100, 3, 100, 15, 100, 8) >= 20);
  CHECK(simPct(team, 100, 3, 100, 15, 100, 8) <= 80);  // 8F is no walk either
}
