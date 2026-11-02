// Battle rules (battle.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../battle.h"
#include <stdlib.h>

TEST(battle, gen_1_type_chart) {
  CHECK_EQ((int)btlEffect(TY_FIRE, TY_GRASS), 4);
  CHECK_EQ((int)btlEffect(TY_WATER, TY_FIRE), 4);
  CHECK_EQ((int)btlEffect(TY_GRASS, TY_WATER), 4);
  CHECK_EQ((int)btlEffect(TY_FIRE, TY_WATER), 1);
  CHECK_EQ((int)btlEffect(TY_NORMAL, TY_ROCK), 1);
  CHECK_EQ((int)btlEffect(TY_NORMAL, TY_GHOST), 0);
  CHECK_EQ((int)btlEffect(TY_ELECTRIC, TY_GROUND), 0);
  CHECK_EQ((int)btlEffect(TY_GHOST, TY_PSYCHIC), 0);  // the famous gen 1 bug
  CHECK_EQ((int)btlEffect(TY_ICE, TY_DRAGON), 4);
  CHECK_EQ((int)btlEffect(TY_NORMAL, TY_NORMAL), 2);
}

TEST(battle, full_chart_has_no_odd_values) {
  for (int a = 0; a < TY_COUNT; a++)
    for (int d = 0; d < TY_COUNT; d++) {
      int e = btlEffect(a, d);
      CHECK(e == 0 || e == 1 || e == 2 || e == 4);
    }
}

static const char *moveName(int16_t dex, uint8_t lvl, uint8_t slot) {
  return MOVE_TBL[btlMovesFor(dex, lvl, 1).ids[slot]].name;
}

static bool sameName(const char *a, const char *b) { return strcmp(a, b) == 0; }

TEST(battle, each_species_uses_its_red_blue_moves) {
  CHECK_EQ((int)btlMovesFor(102, 20, 1).count, 1);  // Exeggcute only has BARRAGE at that level
  CHECK(sameName(moveName(102, 20, 0), "BARRAGE"));
  CHECK_EQ((int)btlMovesFor(1, 5, 4).count, 1);
  CHECK(sameName(moveName(1, 5, 0), "TACKLE"));
  CHECK(sameName(moveName(1, 21, 0), "TACKLE"));
  CHECK(sameName(moveName(1, 21, 1), "VINE WHIP"));
  CHECK(sameName(moveName(4, 30, 0), "SLASH"));
  CHECK(sameName(moveName(4, 30, 1), "EMBER"));  // it is not left without a move of its own type
  CHECK(sameName(moveName(25, 1, 0), "THUNDERSHOCK"));
}

TEST(battle, no_damaging_moves_uses_struggle) {
  CHECK(sameName(moveName(63, 50, 0), "STRUGGLE"));  // Abra only has TELEPORT
  CHECK(sameName(moveName(129, 14, 0), "STRUGGLE"));  // Magikarp: SPLASH does nothing
  CHECK(sameName(moveName(129, 15, 0), "TACKLE"));
}

TEST(battle, keeps_what_was_learned_before_evolving) {
  CHECK(sameName(moveName(11, 8, 0), "TACKLE"));  // Metapod, from when it was Caterpie
  CHECK(sameName(moveName(14, 8, 0), "POISON STING"));
}

TEST(battle, ditto_copies_the_rivals_moves) {
  BtlMoves m = btlMovesFor(DEX_DITTO, 30, 25);
  BtlMoves p = btlMovesFor(25, 30, 1);
  CHECK_EQ((int)m.ids[0], (int)p.ids[0]);
  CHECK_EQ((int)m.ids[1], (int)p.ids[1]);
  CHECK(sameName(MOVE_TBL[btlMovesFor(DEX_DITTO, 30, DEX_DITTO).ids[0]].name, "STRUGGLE"));
}

TEST(battle, new_move_announced_only_once) {
  BtlMoves one = btlMovesFor(1, 5, 4), two = btlMovesFor(1, 13, 4);
  CHECK_EQ((int)btlMoveToAnnounce(1, one), -1);  // new pet: nothing to announce
  CHECK_EQ((int)btlMoveToAnnounce(1, two), (int)two.newest);
  CHECK_EQ((int)btlMoveToAnnounce(2, two), -1);  // old save that already had two
  CHECK_EQ((int)btlMoveToAnnounce(btlMovesSeenValue(two), two), -1);
  CHECK_EQ((int)btlMoveToAnnounce(btlMovesSeenValue(one), two), (int)two.newest);
  CHECK(sameName(MOVE_TBL[two.newest].name, "VINE WHIP"));
}

TEST(battle, moves_fit_on_the_button) {
  for (size_t i = 0; i < sizeof(MOVE_TBL) / sizeof(MOVE_TBL[0]); i++) CHECK(strlen(MOVE_TBL[i].name) <= 13);
}

TEST(battle, damage_effectiveness_and_stab) {
  BtlMove ember = { "EMBER", TY_FIRE, 60 };
  uint16_t base = btlDamage(20, ember, 50, 50, TY_NORMAL, TY_NORMAL, 100);
  uint16_t stab = btlDamage(20, ember, 50, 50, TY_FIRE, TY_NORMAL, 100);
  uint16_t sup = btlDamage(20, ember, 50, 50, TY_NORMAL, TY_GRASS, 100);
  uint16_t weak = btlDamage(20, ember, 50, 50, TY_NORMAL, TY_WATER, 100);
  CHECK(stab > base);
  CHECK_EQ((int)sup, (int)base * 2);
  CHECK(weak < base);
  CHECK(weak >= 1);
  CHECK_EQ((int)btlDamage(20, BtlMove{ "TACKLE", TY_NORMAL, 40 }, 50, 50, TY_NORMAL, TY_GHOST, 100), 0);
}

TEST(battle, level_cap_and_nearby_rival) {
  CHECK_EQ((int)btlCapLevel(999), BTL_LEVEL_CAP);
  CHECK_EQ((int)btlWildLevel(1, 0), 2);
  CHECK_EQ((int)btlWildLevel(100, 3), 100);
  for (int r = 0; r < 4; r++) {
    int l = btlWildLevel(50, r);
    CHECK(l >= 47 && l <= 50);
  }
}

// a whole battle with the rival's AI; the player uses their best move
static bool simulate(int16_t mine, int16_t foeDex, uint8_t lvl, uint8_t trained) {
  const DexEntry &d = DEX_TBL[mine];
  BtlFighter me;
  me.dex = mine;
  me.level = lvl;
  me.hp = me.maxHp = btlMaxHp(d.bHp, lvl);
  me.atk = btlStat(d.bAtk, 100, lvl, trained);
  me.def = btlStat(d.bDef, 100, lvl, 0);
  me.spe = btlStat(d.bSpe, 100, lvl, 0);
  BtlFighter foe = btlMakeWild(foeDex, btlWildLevel(lvl, rand() % 4), btlWildGene(rand() % 16));
  uint8_t myT = d.type, foeT = DEX_TBL[foeDex].type;
  BtlMoves myMoves = btlMovesFor(mine, lvl, foeDex), foeMoves = btlMovesFor(foeDex, foe.level, mine);
  for (int turn = 0; turn < 200; turn++) {
    uint16_t best = 0;
    for (int m = 0; m < myMoves.count; m++) {
      uint16_t x = btlDamage(lvl, MOVE_TBL[myMoves.ids[m]], me.atk, foe.def, myT, foeT, 85 + rand() % 16);
      if (x > best) best = x;
    }
    BtlMove fm = MOVE_TBL[foeMoves.ids[btlWildMove(foeMoves, rand() % 100)]];
    uint16_t back = btlDamage(foe.level, fm, foe.atk, me.def, foeT, myT, 85 + rand() % 16);
    bool first = btlPlayerFirst(me.spe, foe.spe, rand() % 2);
    if (first) {
      foe.hp = best >= foe.hp ? 0 : foe.hp - best;
      if (!foe.hp) return true;
    }
    me.hp = back >= me.hp ? 0 : me.hp - back;
    if (!me.hp) return false;
    if (!first) {
      foe.hp = best >= foe.hp ? 0 : foe.hp - best;
      if (!foe.hp) return true;
    }
  }
  return false;
}

static int winRate(int16_t mine, uint8_t lvl, uint8_t trained) {
  srand(1234);
  int wins = 0, n = 0;
  for (int16_t foe = 1; foe <= 151; foe++)
    for (int i = 0; i < 10; i++) {
      if (DEX_TBL[foe].rarity == R_LEGENDARY || btlMinLevel(foe) > lvl) continue;
      n++;
      if (simulate(mine, foe, lvl, trained)) wins++;
    }
  return wins * 100 / n;
}

TEST(battle, balance_neither_a_gift_nor_impossible) {
  for (int16_t mine : {1, 4, 7}) {
    int fresh = winRate(mine, 5, 0);
    printf("  #%d: level 5 untrained %d%%, level 30 +40 STR %d%%\n", mine, fresh, winRate(mine, 30, 40));
    CHECK_MSG(fresh >= 40 && fresh <= 85, "a new pet must be able to win and lose");
  }
}

// an evolved pet at its normal level, with moderate training
TEST(battle, evolving_makes_a_difference) {
  for (int16_t mine : {3, 6, 9}) {
    int r = winRate(mine, 40, 30);
    printf("  #%d: level 40 +30 STR %d%%\n", mine, r);
    CHECK(r >= 60);
  }
}

TEST(battle, training_makes_a_difference) {
  CHECK(winRate(4, 30, 60) > winRate(4, 30, 0));
}
