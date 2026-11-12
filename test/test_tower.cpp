// Knomi Tower's rules (tower.h), and a simulator of whole climbs with the game's own battle rules, so a later
// change to the numbers can't make the Legend free or the first gate a walk for a new Pokemon.
#include "framework.h"
#include "shim/Arduino.h"
#include "../src/tower.h"
#include <stdlib.h>

TEST(tower, floors_types_and_gates) {
  CHECK_EQ((int)TOWER_TYPE[1], (int)TY_ROCK);
  CHECK_EQ((int)TOWER_TYPE[8], (int)TY_GROUND);
  CHECK(towerIsGate(TOWER_LEGEND));  // only the Legend pays (a Master Ball); the fights fill the balls' bars (Fedor)
  CHECK(!towerIsGate(1) && !towerIsGate(4) && !towerIsGate(8));
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

TEST(tower, the_legend_never_sends_one_your_team_cant_touch) {
  // Tauros (Normal) and Gengar (Ghost): neither can hurt the other, so that fight would never end
  CHECK(towerStalemate(128, 100, 94, 102));
  CHECK(!towerStalemate(6, 100, 94, 102));
  const int16_t normals[3] = { 128, 143, 6 };
  for (uint8_t roll = 0; roll < 8; roll++) {
    int16_t d = towerLegendDex(100, roll, normals, 3, 100);
    CHECK(d >= 1 && d <= DEX_COUNT);
    for (int i = 0; i < 3; i++) CHECK(!towerStalemate(normals[i], 100, d, towerLevel(TOWER_LEGEND, 100)));
  }
  const int16_t fire[3] = { 6, 9, 3 };  // a team with nothing stuck meets the rolled type
  for (uint8_t roll = 0; roll < 4; roll++) CHECK_EQ((int)towerLegendDex(100, roll, fire, 3, 100), (int)towerGuardianDex(TOWER_LEGEND, 100, roll));
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
  me.hp = 0;  // (a fight that never ends counts as lost, so a climb can't loop)
  return false;
}
static int simScore(const BtlFighter &m, const BtlFighter &foe) {  // what the lead screen's hint says, then HP
  return towerMargin(m, foe) * 1000 + m.hp * 100 / m.maxHp;  // (a sensible player: the least bad when all are Weak)
}
// how far a climb gets: 0 none, 1..8 floors, 9 the Legend's first fight, 10 the whole tower
static int simClimb(const int16_t team[3], uint8_t lvl, uint8_t stars, uint8_t ta, uint8_t td, uint8_t ts) {
  BtlFighter t[3];
  for (int i = 0; i < 3; i++) t[i] = towerFriend(team[i], lvl, stars, ta, td, ts);
  int reached = 0;
  for (uint8_t step = 1; step <= TOWER_STEPS; step++) {
    int16_t fd = towerGuardianDex(step, lvl, rnd(250));
    if (step == TOWER_LEGEND) fd = towerLegendDex(lvl, (uint8_t)rnd(250), team, 3, lvl);
    BtlFighter foe = towerGuardian(step, fd, lvl);
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
  // Fedor: a stacked team (Lv 100, three 3-star Pokemon, ATK and SPD full, DEF 25: a daily player's) that plays it well clears it
  // about 1 time in 4: hard, but not impossible
  gSeed = 13;
  const int16_t stacked[3] = { 130, 135, 149 };  // Gyarados, Jolteon, Dragonite
  int top = simPct(stacked, 100, 3, 100, 25, 100, TOWER_STEPS);
  CHECK(top >= 12 && top <= 45);
  const int16_t team[3] = { 6, 9, 26 };  // less DEF, a weaker mix: rarely
  CHECK(simPct(team, 100, 3, 100, 15, 100, TOWER_STEPS) <= 15);
  int eight = simPct(team, 100, 3, 100, 15, 100, 8);
  CHECK(eight >= 10 && eight <= 70);  // 8F is no walk either
}

TEST(tower, friends_cheer_adds_hp_up_to_three_times) {
  CHECK_EQ((int)towerCheerHp(100, 0), 100);
  CHECK_EQ((int)towerCheerHp(100, 2), 100 + 2 * TOWER_CHEER_PCT);
  CHECK_EQ((int)towerCheerHp(100, 9), 100 + TOWER_CHEER_PCT * TOWER_CHEER_MAX);  // never past the most
}

TEST(tower, the_hint_comes_from_the_fight_not_just_the_types) {
  // Rhydon (Ground) against Kabutops (Rock): Ground beats Rock, but Kabutops' Water move and its speed win (Fedor)
  BtlFighter rhydon = towerFriend(112, 100, 3, 100, 25, 100), kabutops = towerGuardian(1, 141, 100);
  CHECK(towerMatchup(rhydon, kabutops) <= 0);
  BtlFighter blastoise = towerFriend(9, 100, 3, 100, 25, 100), charizard = towerFriend(6, 100, 3, 100, 25, 100), golem = towerGuardian(1, 76, 100);
  CHECK_EQ((int)towerMatchup(blastoise, golem), 1);
  CHECK_EQ((int)towerMatchup(charizard, golem), -1);
  BtlFighter tired = blastoise;
  tired.hp = 1;  // the HP each has now counts
  CHECK_EQ((int)towerMatchup(tired, golem), -1);
}

TEST(tower, the_end_says_which_types_to_bring) {
  uint8_t t[2];
  CHECK(towerStrongTypes(76, 100, t) >= 1);  // Golem
  CHECK(t[0] == TY_WATER || t[0] == TY_GRASS || t[0] == TY_GROUND || t[0] == TY_FIGHTING);
  int n = towerStrongTypes(141, 100, t);  // Kabutops has a Water move: never Ground or Fire
  for (int k = 0; k < n; k++) CHECK(t[k] != TY_GROUND && t[k] != TY_FIRE);
  for (int16_t d = 1; d <= DEX_COUNT; d++) {  // whatever it names hits that one hard, and takes no hard hit back
    uint8_t m = towerStrongTypes(d, 100, t);
    BtlMoves fm = btlMovesFor(d, 100, 0);
    for (uint8_t k = 0; k < m; k++) {
      CHECK(btlEffect(t[k], DEX_TBL[d].type) > 2);
      for (int i = 0; i < fm.count; i++) CHECK(btlEffect(MOVE_TBL[fm.ids[i]].type, t[k]) <= 2);
    }
  }
}

TEST(tower, the_tip_fits_how_the_climb_was_lost) {
  BtlFighter golem = towerGuardian(1, 76, 100), fire[3], mixed[3];
  const int16_t f[3] = { 6, 5, 4 }, m[3] = { 6, 4, 9 };  // three Fire, or two Fire and Blastoise
  for (int i = 0; i < 3; i++) fire[i] = towerFriend(f[i], 100, 3, 100, 25, 100), mixed[i] = towerFriend(m[i], 100, 3, 100, 25, 100);
  CHECK_EQ((int)towerTip(&fire[0], golem, fire, 3, 100, 25, 100), (int)TOWER_TIP_WEAK);      // the last one was Weak
  CHECK_EQ((int)towerTip(nullptr, golem, fire, 3, 100, 25, 100), (int)TOWER_TIP_NOSTRONG);   // nobody was Strong
  CHECK_EQ((int)towerTip(&mixed[2], golem, mixed, 3, 100, 25, 100), (int)TOWER_TIP_DEF);     // the right one lost: training
  CHECK_EQ((int)towerTip(&mixed[2], golem, mixed, 3, 10, 25, 100), (int)TOWER_TIP_ATK);
  CHECK_EQ((int)towerTip(&mixed[2], golem, mixed, 3, 100, 25, 5), (int)TOWER_TIP_SPD);
}

TEST(tower, challenges_after_the_legend) {
  CHECK(towerBaseForm(4) && towerBaseForm(133) && towerBaseForm(25) && towerBaseForm(1));
  CHECK(!towerBaseForm(5) && !towerBaseForm(6) && !towerBaseForm(134) && !towerBaseForm(26));
  const int16_t base[3] = { 4, 7, 1 }, big[3] = { 6, 7, 1 };
  const uint8_t ones[3] = { 3, 1, 1 }, mix[3] = { 3, 2, 1 };
  CHECK_EQ((int)towerChallenges(0, base, ones, 3), TOWER_CH_ALL);
  CHECK_EQ((int)towerChallenges(1, big, mix, 3), 0);
  CHECK_EQ((int)towerChallenges(0, big, mix, 3), TOWER_CH_NOFAINT);
  CHECK_EQ((int)towerChallenges(2, big, ones, 3), TOWER_CH_ONESTAR);
  CHECK_EQ((int)towerChallenges(0, base, ones, 2), TOWER_CH_NOFAINT | TOWER_CH_BASE);  // two 1-star friends need three
}
