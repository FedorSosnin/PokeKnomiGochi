// Knomi Tower's balance, with the game's own battle rules (battle.h) and the tower's (tower.h): whole climbs for a few
// teams, and how far they get. Built and run by hand while tuning tower.h's numbers:
//   clang++ -std=c++17 -O2 -I test/shim -I src tools/tower_sim.cpp -o build/tower_sim && build/tower_sim
#include "Arduino.h"
#include "tower.h"
#include <stdio.h>

static uint32_t gSeed = 1;
static int rnd(int n) { gSeed = gSeed * 1103515245u + 12345u; return (int)((gSeed >> 16) % (uint32_t)n); }
static uint16_t hitFor(const BtlFighter &a, const BtlFighter &t, uint8_t mv, uint8_t roll) {
  return btlDamage(a.level, MOVE_TBL[mv], a.atk, t.def, DEX_TBL[a.dex].type, DEX_TBL[t.dex].type, roll);
}
static long gTaken = 0, gFights = 0;  // damage taken on the first floors, to see whether a strong team gets hit
static bool fight(BtlFighter &me, BtlFighter &foe, bool count) {
  BtlMoves mm = btlMovesFor(me.dex, me.level, foe.dex), fm = btlMovesFor(foe.dex, foe.level, me.dex);
  uint8_t pick = mm.ids[0];
  for (int i = 0; i < mm.count; i++)
    if (hitFor(me, foe, mm.ids[i], 100) > hitFor(me, foe, pick, 100)) pick = mm.ids[i];
  uint16_t hp0 = me.hp;
  for (int turn = 0; turn < 200; turn++) {
    bool first = btlPlayerFirst(me.spe, foe.spe, rnd(2));
    uint8_t fmv = fm.ids[btlWildMove(fm, rnd(100))];
    for (int k = 0; k < 2; k++) {
      bool mine = (k == 0) == first;
      BtlFighter &a = mine ? me : foe, &t = mine ? foe : me;
      uint16_t dmg = hitFor(a, t, mine ? pick : fmv, 85 + rnd(16));
      t.hp = dmg >= t.hp ? 0 : t.hp - dmg;
      if (!foe.hp || !me.hp) {
        if (count) gTaken += (hp0 - me.hp) * 100 / me.maxHp, gFights++;
        return foe.hp == 0;
      }
    }
  }
  return false;
}
static int score(const BtlFighter &m, const BtlFighter &foe) {
  return towerHint(DEX_TBL[m.dex].type, DEX_TBL[foe.dex].type) * 1000 + m.hp * 100 / m.maxHp;
}
static int climb(const int16_t *team, int n, uint8_t lvl, uint8_t stars, uint8_t ta, uint8_t td, uint8_t ts) {
  BtlFighter t[3];
  for (int i = 0; i < n; i++) t[i] = towerFriend(team[i], lvl, stars, ta, td, ts);
  int reached = 0;
  for (uint8_t step = 1; step <= TOWER_STEPS; step++) {
    BtlFighter foe = towerGuardian(step, towerGuardianDex(step, lvl, rnd(250)), lvl);
    for (;;) {
      int best = -1;
      for (int i = 0; i < n; i++)
        if (t[i].hp && (best < 0 || score(t[i], foe) > score(t[best], foe))) best = i;
      if (best < 0) return reached;
      if (fight(t[best], foe, step <= 3)) break;
      foe.hp = towerDrain(foe.hp, foe.maxHp);  // it drains the one it beat
    }
    reached = step;
    if (step < TOWER_STEPS)  // (a little back before Mewtwo too)
      for (int i = 0; i < n; i++) t[i].hp = towerHeal(t[i].hp, t[i].maxHp);
  }
  return reached;
}
int main() {
  struct P { const char *name; int16_t team[3]; int n; uint8_t lvl, st, a, d, s; } PS[] = {
    { "new: Charmander+Squirtle+Pikachu Lv15", { 4, 7, 25 }, 3, 15, 2, 30, 2, 20 },
    { "mid: Charizard+Blastoise+Raichu Lv45", { 6, 9, 26 }, 3, 45, 2, 80, 6, 60 },
    { "good: same, Lv70 3*", { 6, 9, 26 }, 3, 70, 3, 100, 10, 80 },
    { "strong: same, Lv100 3*", { 6, 9, 26 }, 3, 100, 3, 100, 15, 100 },
    { "strong: Gyarados+Jolteon+Dragonite", { 130, 135, 149 }, 3, 100, 3, 100, 15, 100 },
    { "strong: Mewtwo+Dragonite+Gengar", { 150, 149, 94 }, 3, 100, 3, 100, 15, 100 },
    { "strong: Charizard alone", { 6 }, 1, 100, 3, 100, 15, 100 },
  };
  printf("%-40s  1F  2F  3F  4F  5F  6F  7F  8F  L1  L2  (%% of climbs) hit%%/fight 1-3F\n", "");
  for (auto &p : PS) {
    gSeed = 7;
    gTaken = gFights = 0;
    int N = 1500, at[11] = { 0 };
    for (int i = 0; i < N; i++) {
      int r = climb(p.team, p.n, p.lvl, p.st, p.a, p.d, p.s);
      for (int k = 1; k <= r; k++) at[k]++;
    }
    printf("%-40s", p.name);
    for (int k = 1; k <= TOWER_STEPS; k++) printf(" %3d", at[k] * 100 / N);
    printf("   %3ld\n", gFights ? gTaken / gFights : 0);
  }
}
