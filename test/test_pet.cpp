// Tests of the pet logic (pet.cpp) on top of the host shims.
#include "framework.h"
#include "shim/Arduino.h"
#include "../src/pet.h"
#include "../src/dex.h"
#include "../src/zones.h"
#include "../src/battle.h"
#include "../host/dex_json.h"  // what the KnomiDex page says about training
#include <string.h>

// ---------------------------------------------------------------- helpers
static void advance(Pet &p, uint32_t minutes) {
  for (uint32_t i = 0; i < minutes; i++) {
    mockAdvanceMillis(PET_TICK_MS);
    p.update(millis());
  }
}

// a freshly hatched pet of the requested species, with a clean NVS
static void makePet(Pet &p, int16_t dex) {
  mockNvsReset();
  mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(dex);
  p.eggTap();
  p.eggTap();
  p.eggTap();  // 3 taps = immediate hatch
}

// the clock moves to epoch and the game lived that long too (a new day needs the hours lived)
static void liveTo(Pet &p, uint32_t epoch) {
  if (epoch > p.lastSeenEpoch && p.lastSeenEpoch) p.lifeMin += (epoch - p.lastSeenEpoch) / 60;
  p.setClock(epoch);
}

static void setStats(Pet &p, uint8_t f, uint8_t j, uint8_t e, uint8_t h) {
  p.fullness = f; p.joy = j; p.energy = e; p.hygiene = h;
}

// ---------------------------------------------------------------- egg
TEST(egg, first_game_asks_for_starter) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK(p.isEgg());
  CHECK(p.awaitingStarter());
  CHECK_EQ(p.eggCracks(), (uint8_t)0);
  CHECK_EQ(p.registeredCount(), (uint16_t)0);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
}

// v1.3 regression: the egg hatched on its own after 3 min while the player
// was still choosing a starter, and the choice was lost
TEST(egg, does_not_hatch_while_choosing_starter) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK(p.awaitingStarter());
  advance(p, 600);  // 10 game hours
  CHECK_MSG(p.isEgg(), "the egg must NOT hatch during the choice");
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  p.chooseStarter(7);
  CHECK(!p.awaitingStarter());
  advance(p, 3);
  CHECK(!p.isEgg());
  CHECK_EQ(p.speciesId, (int16_t)7);
}

#if !UI_V2  // 1.51's egg: V2 has none (the starter or the pick is yours at once: v2pick below)
TEST(egg, hatches_on_its_own_after_3_minutes) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.chooseStarter(1);
  advance(p, 2);
  CHECK(p.isEgg());
  advance(p, 1);
  CHECK(!p.isEgg());
  CHECK_EQ(p.speciesId, (int16_t)1);
}

TEST(egg, three_taps_hatch) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.chooseStarter(4);
  p.eggTap();
  CHECK_EQ(p.eggCracks(), (uint8_t)1);
  CHECK(p.isEgg());
  p.eggTap();
  CHECK_EQ(p.eggCracks(), (uint8_t)2);
  CHECK(p.isEgg());
  p.eggTap();
  CHECK(!p.isEgg());
  CHECK_EQ(p.speciesId, (int16_t)4);
}
#endif

TEST(egg, hatching_rolls_genes_and_resets_the_individual) {
  Pet p;
  makePet(p, 4);
  CHECK_RANGE(p.geneAtk, 90, 110);
  CHECK_RANGE(p.geneDef, 90, 110);
  CHECK_RANGE(p.geneSpe, 90, 110);
  CHECK_EQ(p.trAtk, (uint8_t)0);
  CHECK_EQ(p.trDef, (uint8_t)0);
  CHECK_EQ(p.trSpe, (uint8_t)0);
  CHECK_EQ(p.bond, (uint8_t)0);
  CHECK_EQ(p.berryKnown, false);
  CHECK_STREQ(p.nick, "");
  CHECK_MSG(p.isRegistered(4), "raising = registering in the pokedex");
  CHECK_EQ(p.registeredCount(), (uint16_t)1);
}

TEST(egg, tapping_egg_does_nothing_once_hatched) {
  Pet p;
  makePet(p, 4);
  uint8_t cracks = p.eggCracks();
  p.eggTap();
  CHECK_EQ(p.eggCracks(), cracks);
  CHECK_EQ(p.speciesId, (int16_t)4);
}

TEST(egg, egg_rarity_is_valid) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK_RANGE(p.eggRarity(), (uint8_t)R_COMMON, (uint8_t)R_LEGENDARY);
}

// ---------------------------------------------------------------- food
TEST(feed, normal_berry_adds_25_and_caps_at_100) {
  Pet p;
  makePet(p, 4);
  p.fullness = 50;
  uint8_t notLoved = (uint8_t)((p.speciesId + 1) % 3);
  p.feedBerry(notLoved);
  CHECK_EQ(p.fullness, (uint8_t)75);
  p.feedBerry(notLoved);
  CHECK_EQ(p.fullness, (uint8_t)100);  // 100 instead of 105
}

TEST(feed, favorite_berry_adds_more_and_gives_heart) {
  Pet p;
  makePet(p, 4);  // 4 % 3 == 1 -> it likes the blue one
  CHECK(p.lovesBerry(1));
  CHECK(!p.lovesBerry(0));
  CHECK(!p.lovesBerry(2));
  p.fullness = 10;
  p.joy = 10;
  p.feedBerry(1);
  CHECK_EQ(p.fullness, (uint8_t)45);
  CHECK_EQ(p.joy, (uint8_t)20);
  CHECK(p.berryKnown);
  CHECK(p.showHeart());
  CHECK(p.eating());
}

TEST(feed, candy_fattens) {
  Pet p;
  makePet(p, 4);
  p.fullness = 50;
  p.joy = 50;
  p.energy = 20;
  p.feedCandy();
  CHECK_EQ(p.fullness, (uint8_t)60);
  CHECK_EQ(p.joy, (uint8_t)62);
  CHECK_EQ(p.energy, (uint8_t)100);  // and it refills energy
  CHECK_EQ(p.weight, (uint8_t)WGT_CANDY);
}

TEST(feed, every_meal_adds_weight_and_overeating_more) {
  Pet p;
  makePet(p, 4);
  p.fullness = 30;
  p.feedBerry(0);
  CHECK_EQ(p.weight, (uint8_t)WGT_BERRY);
  p.fullness = WGT_FULL_AT;  // already full: it eats anyway, and it shows
  p.feedBerry(0);
  CHECK_EQ(p.weight, (uint8_t)(2 * WGT_BERRY + WGT_OVERFULL));
}

TEST(feed, a_day_of_overfeeding_makes_it_chubby) {
  Pet p;
  makePet(p, 4);
  for (int h = 0; h < 8; h++) {  // a full berry every 20 minutes, from full
    for (int i = 0; i < 3; i++) {
      p.fullness = 90;
      p.energy = 100;
      p.feedBerry(0);
      advance(p, 20);
    }
  }
  CHECK_MSG(p.weight > 60, "fed while full all day: over 60 says it's chubby");
}

TEST(feed, asleep_or_egg_does_not_eat) {
  Pet p;
  makePet(p, 4);
  p.fullness = 40;
  p.toggleLight();  // off to sleep
  CHECK(p.sleeping);
  p.feedBerry(1);
  p.feedCandy();
  CHECK_EQ(p.fullness, (uint8_t)40);

  Pet e;
  mockNvsReset();
  e.begin();
#if !UI_V2
  e.chooseStarter(4);  // (V2: the egg is only the wait for a starter)
#endif
  e.fullness = 40;
  e.feedBerry(0);
  CHECK_EQ(e.fullness, (uint8_t)40);
}

// ---------------------------------------------------------------- tick
TEST(tick, awake_drops_food_2_and_energy_1_per_minute) {
  Pet p;
  makePet(p, 4);
  setStats(p, 100, 100, 100, 100);
  p.poops = 0;
  mockForceRandom(99);  // random(100) < 15 is false: no poop
  advance(p, 10);
  mockClearForcedRandom();
  CHECK_EQ(p.fullness, (uint8_t)80);
  CHECK_EQ(p.energy, (uint8_t)90);
  CHECK_EQ(p.hygiene, (uint8_t)90);
  CHECK_EQ(p.joy, (uint8_t)90);
  CHECK_EQ(p.ageMinutes, (uint32_t)10);
  CHECK_EQ(p.poops, (uint8_t)0);
}

TEST(tick, bars_never_drop_below_zero) {
  Pet p;
  makePet(p, 4);
  setStats(p, 5, 3, 2, 1);
  advance(p, 30);
  CHECK_EQ(p.fullness, (uint8_t)0);
  CHECK_EQ(p.joy, (uint8_t)0);
  CHECK_EQ(p.energy, (uint8_t)0);
  CHECK_EQ(p.hygiene, (uint8_t)0);
}

TEST(tick, party_boost_cuts_the_energy_drain_by_a_quarter) {
  Pet p;
  makePet(p, 4);
  setStats(p, 100, 100, 100, 100);
  p.poops = 0;
  p.partyBoost = true;  // a buddy or the visitor around
  mockForceRandom(99);
  advance(p, 40);
  mockClearForcedRandom();
  CHECK_EQ(p.energy, (uint8_t)70);    // 30 of 40 minutes: a quarter less
  CHECK_EQ(p.fullness, (uint8_t)20);  // the rest drops as usual
  p.weight = 60;                      // overweight: still twice the usual, a quarter less
  p.energy = 100;
  setStats(p, 100, 100, p.energy, 100);
  mockForceRandom(99);
  advance(p, 20);
  mockClearForcedRandom();
  CHECK_EQ(p.energy, (uint8_t)70);    // 2 x 15 of 20
}

TEST(tick, sleeping_restores_energy_with_floors) {
  Pet p;
  makePet(p, 4);
  setStats(p, 100, 100, 10, 100);
  p.toggleLight();
  advance(p, 20);
  CHECK_MSG(p.energy == 100, "while sleeping, energy rises 6/min up to 100");
  CHECK_MSG(p.fullness > 80, "while sleeping, hunger drops ~4x slower");
  advance(p, 2000);  // sleeping a very long time: the floors protect
  CHECK_MSG(p.fullness >= 30, "food floor while sleeping = 30");
  CHECK_MSG(p.joy >= 35, "joy floor while sleeping = 35");
  CHECK_MSG(p.hygiene >= 45, "hygiene floor while sleeping = 45");
  CHECK_EQ(p.careMistakes, (uint8_t)0);
}

TEST(tick, overweight_burns_off_by_itself) {
  Pet p;
  makePet(p, 4);
  p.weight = 30;
  advance(p, 60);
  CHECK_EQ(p.weight, (uint8_t)20);  // -1 every 6 minutes
}

TEST(tick, neglect_counts_once_per_hour) {
  Pet p;
  makePet(p, 4);
  setStats(p, 0, 0, 0, 0);
  advance(p, 1);
  CHECK_EQ(p.careMistakes, (uint8_t)1);
  advance(p, 50);
  CHECK_MSG(p.careMistakes == 1, "the cooldown avoids counting the same neglect every minute");
  advance(p, 11);
  CHECK_EQ(p.careMistakes, (uint8_t)2);
}

// v1.4 regression: neglect subtracted 3 bond every 30 min and it was impossible
// to recover; now it subtracts 1 and never leaves it at 0
TEST(tick, neglect_cools_bond_without_wiping_it) {
  Pet p;
  makePet(p, 4);
  p.bond = 50;
  setStats(p, 0, 0, 0, 0);
  advance(p, 1);
  CHECK_EQ(p.bond, (uint8_t)49);
  p.bond = 1;
  advance(p, 61);
  CHECK_MSG(p.bond == 1, "the bond does not fall below 1");
}

TEST(tick, good_care_for_6h_builds_defense) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.trDef, (uint8_t)0);
  for (int m = 0; m < GOOD_CARE_MIN - 1; m++) {  // keep everything >= 40 for 6 h
    setStats(p, 100, 100, 100, 100);
    advance(p, 1);
  }
  CHECK_EQ(p.trDef, (uint8_t)0);
  advance(p, 1);
  CHECK_EQ(p.trDef, (uint8_t)1);
}

TEST(tick, sleep_counts_toward_defense) {
  Pet p;
  makePet(p, 4);
  for (int m = 0; m < GOOD_CARE_MIN / 2; m++) {
    setStats(p, 100, 100, 100, 100);
    advance(p, 1);
  }
  p.sleeping = true;  // the other half asleep, with needs dipping overnight as they do
  setStats(p, 35, 35, 20, 50);
  advance(p, GOOD_CARE_MIN / 2);
  CHECK_MSG(p.trDef == 1, "hours asleep count as good care");
}

TEST(tick, short_dips_under_40_are_forgiven) {
  Pet p;
  makePet(p, 4);
  for (int m = 0; m < 100; m++) { setStats(p, 100, 100, 100, 100); advance(p, 1); }
  for (int m = 0; m < CARE_GRACE_MIN; m++) { setStats(p, 30, 100, 100, 100); advance(p, 1); }  // hungry for a while
  CHECK_MSG(p.goodTicks == 100 + CARE_GRACE_MIN, "a dip shorter than the grace keeps the streak");
  for (int m = 0; m < 5; m++) { setStats(p, 100, 100, 100, 100); advance(p, 1); }
  for (int m = 0; m <= CARE_GRACE_MIN; m++) { setStats(p, 30, 100, 100, 100); advance(p, 1); }
  CHECK_MSG(p.goodTicks == 0, "left under 40 longer than the grace: the streak starts over");
}

TEST(tick, defense_streak_survives_a_restart) {
  Pet p;
  makePet(p, 4);
  for (int m = 0; m < 200; m++) { setStats(p, 100, 100, 100, 100); advance(p, 1); }
  p.flushSave();
  Pet q;
  q.begin();
  CHECK_EQ(q.goodTicks, (uint16_t)200);
}

// ---------------------------------------------------------------- levels
TEST(level, one_level_per_game_hour) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.level(), (uint8_t)1);
  p.ageMinutes = 59;
  CHECK_EQ(p.level(), (uint8_t)1);
  p.ageMinutes = 60;
  CHECK_EQ(p.level(), (uint8_t)2);
  p.ageMinutes = 24 * 60;
  CHECK_EQ(p.level(), (uint8_t)25);
}

// Fixed in v1.8: level() was uint8_t and wrapped around to 0 after ~10.6 days
TEST(level, does_not_overflow_with_long_lived_pets) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 99 * 60;
  uint16_t before = p.level();
  CHECK_EQ(before, (uint16_t)LEVEL_MAX);
  p.ageMinutes = 255 * 60;  // a uint8_t level would wrap at 256: it stays at the top instead
  CHECK_EQ(p.level(), before);
}

// ---------------------------------------------------------------- evolution
TEST(evolve, requires_level_and_good_care) {
  Pet p;
  makePet(p, 4);  // CHARMANDER evolves at level 16
  p.ageMinutes = 14 * 60;  // level 15
  CHECK(!p.canEvolveNow());
  p.ageMinutes = 15 * 60;  // level 16
  CHECK(p.canEvolveNow());
  p.fullness = 39;  // a bar below 40 blocks it
  CHECK(!p.canEvolveNow());
  p.fullness = 40;
  CHECK(p.canEvolveNow());
}

TEST(evolve, each_neglect_delays_one_level) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 15 * 60;  // level 16
  p.careMistakes = 1;
  CHECK(!p.canEvolveNow());
  p.ageMinutes = 16 * 60;  // level 17
  CHECK(p.canEvolveNow());
}

// Fixed in v1.8: the cast to uint8_t of (evolveLevel + careMistakes) overflowed
TEST(evolve, many_neglects_do_not_advance_evolution) {
  Pet p;
  makePet(p, 4);       // CHARMANDER: evolves at level 16
  p.careMistakes = 250;  // 250 neglects: it should be very far from evolving
  p.ageMinutes = 9 * 60;  // level 10
  CHECK_MSG(!p.canEvolveNow(), "with 250 neglects it cannot evolve at level 10");
}

TEST(evolve, does_not_evolve_asleep_as_egg_or_during_ceremony) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 20 * 60;
  CHECK(p.canEvolveNow());
  p.toggleLight();
  CHECK(!p.canEvolveNow());
  p.toggleLight();
  p.startFarewell();
  CHECK(!p.canEvolveNow());

  Pet e;
  mockNvsReset();
  e.begin();
  CHECK(!e.canEvolveNow());
}

TEST(evolve, transforms_registers_and_animates) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 20 * 60;
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)5);
  CHECK_EQ(p.prevSpeciesId, (int16_t)4);
  CHECK(p.isRegistered(5));
  CHECK(p.evolving());
  CHECK_RANGE(p.evolveT(), 0.0f, 1.0f);
  mockAdvanceMillis(EVOLVE_ANIM_MS + 1);
  CHECK(!p.evolving());
}

TEST(evolve, final_form_does_not_evolve) {
  Pet p;
  makePet(p, 6);  // CHARIZARD
  p.ageMinutes = 200 * 60;
  CHECK(!p.canEvolveNow());
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)6);
}

TEST(evolve, eevee_branches_into_134_136) {
  for (int seed = 1; seed <= 12; seed++) {
    randomSeed(seed);
    Pet p;
    makePet(p, DEX_EEVEE);
    p.ageMinutes = 40 * 60;
    CHECK(p.canEvolveNow());
    p.evolve();
    CHECK_RANGE(p.speciesId, (int16_t)134, (int16_t)136);
    CHECK(p.isRegistered(p.speciesId));
  }
}

TEST(evolve, evolve_button_is_postponed_when_declined) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 15 * 60;  // level 16
  CHECK(p.wantEvolveButton());
  p.declineEvolve();
  CHECK_MSG(!p.wantEvolveButton(), "'keep form' removes the button");
  CHECK_MSG(p.canEvolveNow(), "but the evolution is still available");
  p.ageMinutes = 16 * 60;  // an hour later
#if UI_V2
  CHECK_MSG(!p.wantEvolveButton(), "V2: kept is kept (Fedor): an hour doesn't bring the button back");
  CHECK_MSG(p.canEvolveNow(), "holding it still offers EVOLVE");
#else
  CHECK(p.wantEvolveButton());  // it is offered again
#endif
}

TEST(evolve, declined_at_level_100_is_offered_again_an_hour_later) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 150 * 60;  // level 100 and staying there
  CHECK(p.wantEvolveButton());
  p.declineEvolve();
  CHECK(!p.wantEvolveButton());
  p.ageMinutes += 60;
#if UI_V2
  CHECK_MSG(!p.wantEvolveButton(), "V2: not even at the top level");
#else
  CHECK_MSG(p.wantEvolveButton(), "the level can't rise any more, but an hour still brings it back");
#endif
}

#if UI_V2
TEST(evolve, v2_kept_form_stays_kept_through_a_restart_and_holding_it_still_evolves) {
  Pet p;
  makePet(p, 25);  // PIKACHU, which some would rather keep than have a RAICHU (Fedor)
  p.ageMinutes = 40 * 60;
  CHECK(p.wantEvolveButton());
  mockSetMillis(1000);
  p.declineEvolve();
  CHECK_EQ(p.evoKept, (int16_t)25);
  CHECK_MSG(p.evoHintUntil == 7000, "the home says how to evolve it later, for 6 s");
  p.flushSave();
  Pet q;
  q.begin();
  CHECK_MSG(q.evoKept == 25 && !q.wantEvolveButton(), "a restart doesn't ask again (it used to: not saved)");
  CHECK_MSG(q.canEvolveNow(), "the hold's EVOLVE is there");
  q.evolve();
  CHECK_EQ(q.speciesId, (int16_t)26);
}

TEST(evolve, v2_the_next_form_and_a_new_pokemon_are_asked_again) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 15 * 60;  // level 16
  p.declineEvolve();
  p.evolve();  // held, then EVOLVE
  CHECK_EQ(p.speciesId, (int16_t)5);
  p.ageMinutes = 35 * 60;  // CHARMELEON's level 36
  CHECK_MSG(p.wantEvolveButton(), "CHARMELEON is asked once, as any form");
  p.declineEvolve();
  CHECK(!p.wantEvolveButton());
  p.newEgg();
  p.chooseStarter(5);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_MSG(p.evoKept == 0, "a new Pokemon, even of a kept form, is asked about its own");
  p.ageMinutes = 35 * 60;
  CHECK(p.wantEvolveButton());
}
#endif

// ---------------------------------------------------------------- stats
TEST(stats, formula_base_by_genes_plus_level_and_training) {
  Pet p;
  makePet(p, 4);  // CHARMANDER: atk 52, def 43, spe 65
  p.geneAtk = p.geneDef = p.geneSpe = 100;
  p.trAtk = p.trDef = p.trSpe = 0;
  p.ageMinutes = 0;  // level 1
  CHECK_EQ(p.atkStat(), (uint16_t)(52 + 1));
  CHECK_EQ(p.defStat(), (uint16_t)(43 + 1));
  CHECK_EQ(p.speStat(), (uint16_t)(65 + 1));
  p.ageMinutes = 9 * 60;  // level 10
  p.trAtk = 20;
  CHECK_EQ(p.atkStat(), (uint16_t)(52 + 10 + 20));
  p.geneAtk = 110;
  CHECK_EQ(p.atkStat(), (uint16_t)(52 * 110 / 100 + 10 + 20));
}

// the KnomiDex page tells how training is earned (host/dex_json.h, into tools/dex.json): it's what your Pokemon gets
TEST(stats, knomidex_says_how_training_is_earned) {
  Pet p;
  makePet(p, 4);
  for (const DexTrainGame &g : DEX_TRAIN_GAMES) {
    for (uint8_t times = 1; times <= 4; times++) {  // so many of its unit, so many times its training
      p.trSpe = 0;
      uint16_t n = (uint16_t)g.per * times;
      switch (g.kind) {
        case DEX_TRAIN_BERRY: p.playResult((uint8_t)n); break;
        case DEX_TRAIN_RUN: p.runResult(n); break;
        case DEX_TRAIN_SURF: p.surfResult(n); break;
        case DEX_TRAIN_DUNGEON: p.dungeonResult((uint8_t)n); break;
      }
      CHECK_MSG(p.trSpe == g.gain * times, g.name);
    }
  }
  p.trAtk = 0;
  CHECK_EQ(p.battleResult(true), (uint8_t)BTL_WIN_TRAIN);
  p.trSpe = DEX_TRAIN_MAX - 1;
  p.playResult(200);
  CHECK_EQ(p.trSpe, (uint8_t)DEX_TRAIN_MAX);  // (where it stops)
  CHECK_EQ(Pet::starsFromGenes(DEX_GENE_MIN, DEX_GENE_MIN, DEX_GENE_MIN), (uint8_t)1);
  CHECK_EQ(Pet::starsFromGenes(DEX_GENE_MAX, DEX_GENE_MAX, DEX_GENE_MAX), (uint8_t)3);
}

TEST(stats, egg_has_no_stats) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK_EQ(p.atkStat(), (uint16_t)0);
  CHECK_EQ(p.defStat(), (uint16_t)0);
  CHECK_EQ(p.speStat(), (uint16_t)0);
}

TEST(train, only_a_win_trains) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.battleResult(true), (uint8_t)BTL_WIN_TRAIN);
  CHECK_EQ(p.battleResult(false), (uint8_t)0);
  CHECK_EQ(p.trAtk, (uint8_t)BTL_WIN_TRAIN);
  CHECK_EQ(p.battleWins, (uint16_t)1);
}

// a loss earns nothing (no training, bond or day of care) but still tires it out
TEST(train, a_loss_earns_nothing_but_tires) {
  Pet p;
  makePet(p, 4);
  p.energy = 90;
  uint8_t bond = p.bond, atk = p.trAtk;
  uint16_t streak = p.streak;
  p.battleResult(false);
  CHECK_EQ(p.trAtk, atk);
  CHECK_EQ(p.bond, bond);
  CHECK_EQ(p.streak, streak);
  CHECK(p.energy <= BTL_LOSS_ENERGY);
}

TEST(train, battle_reports_what_really_rises) {
  Pet p;
  makePet(p, 4);
  p.trAtk = 98;
  CHECK_EQ(p.battleResult(true), (uint8_t)2);
  CHECK_EQ(p.trAtk, (uint8_t)100);
  CHECK_EQ(p.battleResult(true), (uint8_t)0);
}

TEST(train, winning_tires_and_burns_weight) {
  Pet p;
  makePet(p, 4);
  p.weight = 40;
  p.energy = 80;
  p.fullness = 80;
  p.battleResult(true);
  CHECK_EQ(p.energy, (uint8_t)(80 - BTL_ENERGY_COST));
  CHECK_EQ(p.fullness, (uint8_t)75);
  CHECK_EQ(p.weight, (uint8_t)35);
  p.energy = 3;  // the floor protects it from reaching zero
  p.battleResult(true);
  CHECK_EQ(p.energy, (uint8_t)3);
}

TEST(train, losing_exhausts_and_forces_sleep) {
  Pet p;
  makePet(p, 4);
  p.energy = 90;
  p.battleResult(false);
  CHECK_EQ(p.energy, (uint8_t)BTL_LOSS_ENERGY);
  CHECK(p.energy < BTL_MIN_ENERGY);  // it cannot fight again like this
}

TEST(train, losing_is_not_neglect_but_ignoring_it_is) {
  Pet p;
  makePet(p, 4);
  p.fullness = p.joy = p.hygiene = 90;
  p.battleResult(false);
  uint8_t before = p.careMistakes;
  advance(p, BTL_LOSS_GRACE_MIN - 2);
  CHECK_MSG(p.careMistakes == before, "grace period to put it to bed");
  advance(p, 5);
  CHECK_MSG(p.careMistakes == before + 1, "ignoring the exhausted pet does count");
}

TEST(train, winning_does_not_register_until_caught) {
  Pet p;
  makePet(p, 4);
  p.battleResult(true);
  CHECK(!p.isRegistered(25));
  CHECK(p.catchWild(25) == CR_NEW);
  CHECK(p.isRegistered(25));
  CHECK_MSG(p.catchWild(25) != CR_NEW, "it is only new the first time");
}

TEST(train, shiny_pet_does_not_catch_rivals_as_shiny) {
  Pet p;
  makePet(p, 4);
  p.shiny = true;
  p.catchWild(25);
  CHECK(p.isRegistered(25));
  CHECK(!p.isShinyRegistered(25));
}

TEST(train, test_mode_never_reaches_nvs) {
  Pet p;
  makePet(p, 4);
  p.noSave = true;
  p.trAtk = 77;
  p.catchWild(99);
  p.flushSave();
  Pet q;
  q.begin();
  CHECK_MSG(q.trAtk != 77, "test training is not saved");
  CHECK_MSG(!q.isRegistered(99), "the test capture is not saved");
}

TEST(visitor, stays_an_hour_across_sleep_and_restarts) {
  Pet p;
  makePet(p, 4);
  p.setClock(20000UL * 86400 + 20 * 3600);
  p.visitorArrived(16);
  CHECK_EQ(p.visitMin, (uint8_t)VISIT_MINUTES);
  for (int i = 0; i < 20; i++) CHECK(!p.visitorMinute());
  Pet q;  // a restart: the visitor and its 40 minutes are still there
  q.begin();
  q.setClock(20000UL * 86400 + 21 * 3600);
  CHECK_EQ(q.visitDex, (int16_t)16);
  CHECK_EQ(q.visitMin, (uint8_t)(VISIT_MINUTES - 20));
  CHECK(q.visitorResumable());
  CHECK(!q.visitorDueNow());          // not a second visit the same day
  q.sleeping = true;
  CHECK(!q.visitorResumable());       // it steps out while yours sleeps...
  q.sleeping = false;
  CHECK(q.visitorResumable());        // ...and comes back
  for (int i = 0; i < VISIT_MINUTES - 21; i++) CHECK(!q.visitorMinute());
  CHECK(q.visitorMinute());           // the last minute: its hour is up
  q.visitorLeft();
  CHECK(!q.visitorResumable());
  CHECK_EQ(q.visitDex, (int16_t)0);
}

TEST(visitor, leftover_minutes_end_with_the_day) {
  Pet p;
  makePet(p, 4);
  p.setClock(20000UL * 86400 + 20 * 3600);
  p.visitorArrived(16);
  liveTo(p, 20001UL * 86400 + 20 * 3600);  // the next day
  CHECK(!p.visitorResumable());
  CHECK_EQ(p.visitMin, (uint8_t)0);
  CHECK(p.visitorDueNow());                  // today's own visit instead
}

TEST(visitor, day_is_saved_and_cheers_up_on_leaving) {
  Pet p;
  makePet(p, 4);
  p.setClock(20000UL * 86400 + 20 * 3600);  // 20:00: the hour of any day has already passed
  CHECK(p.visitorDueNow());
  p.visitorArrived(16);
  CHECK(!p.visitorDueNow());
  Pet q;
  q.begin();
  CHECK_MSG(q.visitorDay == p.visitorDay, "rebooting does not bring another visit");
  p.joy = 50;
  p.visitorLeft();
  CHECK_EQ(p.joy, (uint8_t)55);
}

TEST(quiz, getting_it_right_cheers_up_more) {
  Pet p;
  makePet(p, 4);
  p.joy = 40;
  p.quizResult(0);
  CHECK_EQ(p.joy, (uint8_t)43);
  p.joy = 40;
  p.quizResult(5);
  CHECK_EQ(p.joy, (uint8_t)58);
  CHECK_MSG(!p.showHeart(), "half right: no heart");
  p.joy = 40;
  p.quizResult(10);  // all ten silhouettes
  CHECK_EQ(p.joy, (uint8_t)73);
  CHECK_MSG(p.showHeart(), "all right: a heart");
}

TEST(train, rival_respects_evolution_levels_and_legendaries) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 2000; i++) {
    int16_t d = p.pickWildSpecies(5);
    CHECK(d >= 1 && d <= 151);
    CHECK_MSG(DEX_TBL[d].rarity != R_LEGENDARY, "legendary at level 5");
    CHECK_MSG(d != 3 && d != 6 && d != 9, "final form at level 5");
  }
}

TEST(train, rival_prefers_what_is_missing_from_the_pokedex) {
  Pet p;
  makePet(p, 4);
  for (int16_t d = 1; d <= 75; d++) p.catchWild(d);
  int fresh = 0;
  for (int i = 0; i < 1000; i++)
    if (!p.isRegistered(p.pickWildSpecies(50))) fresh++;
  CHECK_MSG(fresh > 600, "at least 70% look for new species");
}

TEST(play, minigame_trains_speed_and_saves_record) {
  Pet p;
  makePet(p, 4);
  p.joy = 10;
  p.weight = 50;
  p.playResult(20);
  CHECK_EQ(p.trSpe, (uint8_t)4);
  CHECK_EQ(p.joy, (uint8_t)45);   // +5 +30 for scoring more than 15
  CHECK_EQ(p.weight, (uint8_t)10);  // 50 - 20*2
  CHECK_EQ(p.gameHi, (uint16_t)20);
  p.playResult(5);
  CHECK_MSG(p.gameHi == 20, "a worse result does not lower the record");
}

TEST(play, playing_raises_joy_and_tires) {
  Pet p;
  makePet(p, 4);
  setStats(p, 50, 50, 50, 50);
  p.play();
  CHECK_EQ(p.joy, (uint8_t)75);
  CHECK_EQ(p.energy, (uint8_t)40);
  CHECK_EQ(p.fullness, (uint8_t)45);
  CHECK(p.showHeart());
}

TEST(care, cleaning_removes_poop_and_maxes_hygiene) {
  Pet p;
  makePet(p, 4);
  p.poops = 3;
  p.hygiene = 10;
  p.clean();
  CHECK_EQ(p.poops, (uint8_t)0);
  CHECK_EQ(p.hygiene, (uint8_t)100);
}

TEST(care, petting_raises_joy_and_bond) {
  Pet p;
  makePet(p, 4);
  p.joy = 50;
  p.caress();
  CHECK_EQ(p.joy, (uint8_t)55);
  CHECK_EQ(p.bond, (uint8_t)1);
  CHECK(p.showHeart());
}

TEST(care, bond_has_a_daily_cap) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 50; i++) p.play();  // +2 bond each time
  CHECK_MSG(p.bond <= 20, "cap of 20 bond points per day");
  CHECK_EQ(p.bond, (uint8_t)20);
}

TEST(mood, reflects_the_state) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 80);
  CHECK_EQ((int)p.mood(), (int)MOOD_HAPPY);
  p.joy = 20;
  CHECK_EQ((int)p.mood(), (int)MOOD_SAD);
  p.joy = 80;
  p.feedBerry(0);
  CHECK_EQ((int)p.mood(), (int)MOOD_EATING);
  mockAdvanceMillis(EAT_ANIM_MS + 1);
  p.toggleLight();
  CHECK_EQ((int)p.mood(), (int)MOOD_SLEEPING);
}

// ---------------------------------------------------------------- pokedex
TEST(dexreg, bitmap_covers_1_to_151) {
  Pet p;
  makePet(p, 4);
  memset(p.dexReg, 0, sizeof(p.dexReg));
  CHECK_EQ(p.registeredCount(), (uint16_t)0);
  for (int16_t d = 1; d <= 151; d++) {
    p.dexReg[(d - 1) >> 3] |= (uint8_t)(1 << ((d - 1) & 7));
    CHECK(p.isRegistered(d));
  }
  CHECK_EQ(p.registeredCount(), (uint16_t)151);
}

TEST(dexreg, out_of_range_is_not_registered) {
  Pet p;
  makePet(p, 4);
  memset(p.dexReg, 0xFF, sizeof(p.dexReg));
  CHECK(!p.isRegistered(0));
  CHECK(!p.isRegistered(-1));
  CHECK(!p.isRegistered(152));
  CHECK(!p.isRegistered(32767));
  CHECK_EQ(p.registeredCount(), (uint16_t)151);
}

TEST(dexreg, incomplete_line_is_detected) {
  Pet p;
  makePet(p, 1);  // BULBASAUR -> IVYSAUR -> VENUSAUR
  CHECK(p.lineHasUnregistered(1));
  p.dexReg[(2 - 1) >> 3] |= (uint8_t)(1 << ((2 - 1) & 7));
  p.dexReg[(3 - 1) >> 3] |= (uint8_t)(1 << ((3 - 1) & 7));
  CHECK_MSG(!p.lineHasUnregistered(1), "line 1-2-3 complete");
}

TEST(dexreg, eevee_branch_counts_all_three) {
  Pet p;
  makePet(p, DEX_EEVEE);
  CHECK(p.lineHasUnregistered(DEX_EEVEE));
  for (int16_t d = 134; d <= 136; d++)
    p.dexReg[(d - 1) >> 3] |= (uint8_t)(1 << ((d - 1) & 7));
  CHECK(!p.lineHasUnregistered(DEX_EEVEE));
}

TEST(egg, draw_always_gives_a_valid_species) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 400; i++) {
    randomSeed(i * 7919 + 1);
    int16_t d = p.pickEggSpecies();
    CHECK_RANGE(d, (int16_t)1, (int16_t)151);
    CHECK_MSG(DEX_TBL[d].rarity != R_EVO, "an egg never produces an evolved form");
  }
}

TEST(egg, first_egg_is_a_classic_starter) {
  for (int seed = 1; seed <= 20; seed++) {
    randomSeed(seed);
    mockNvsReset();
    Pet p;
    p.begin();
    int16_t d = p.pickEggSpecies();
    bool classic = false;
    for (int i = 0; i < NUM_CLASSIC_DEX; i++)
      if (CLASSIC_DEX[i] == d) classic = true;
    CHECK_MSG(classic, "without a pokedex, only classic starters come up");
  }
}

static void hatchNow(Pet &p) { p.eggTap(); p.eggTap(); p.eggTap(); }

TEST(egg, after_farewell_or_release_egg_is_chosen_from_pokedex) {
  Pet p;
  makePet(p, 25);
  p.catchWild(102);
  p.startFarewell();
  p.newEgg();
  CHECK(p.awaitingEggPick());
  CHECK(!p.awaitingStarter());
  CHECK_MSG(!p.chooseEgg(150), "what is not in the pokedex cannot be chosen");
  CHECK(p.awaitingEggPick());
  CHECK(p.chooseEgg(102));
  CHECK(!p.awaitingEggPick());
  hatchNow(p);
  CHECK_EQ(p.speciesId, (int16_t)102);

  p.release();
  p.newEgg();
  CHECK(p.awaitingEggPick());
}

TEST(egg, caring_in_time_cancels_the_runaway) {
  Pet p;
  makePet(p, 25);
  p.dbgRunawayReady();
  CHECK(p.canRunawayNow());
  p.rescue();
  CHECK(!p.canRunawayNow());
  CHECK_EQ(p.ceremony, (uint8_t)CER_NONE);
  CHECK(p.fullness > 0);
  CHECK(p.joy > 0);
  CHECK_EQ(p.hygiene, (uint8_t)100);
  CHECK_EQ(p.speciesId, (int16_t)25);
}

static int shinyCount(uint8_t ending, int rounds) {
  int shinies = 0;
  Pet p;
  makePet(p, 25);
  for (int i = 0; i < rounds; i++) {
    randomSeed(i + 1);
    if (ending == CER_FAREWELL) p.startFarewell();
    else if (ending == CER_RELEASE) p.release();
    else p.startRunaway();
    p.newEgg();
    if (p.awaitingEggPick()) p.chooseEgg(25);
    hatchNow(p);
    if (p.shiny) shinies++;
  }
  return shinies;
}

#if UI_V2
TEST(v2pick, no_shiny_roll_a_shiny_only_when_picked) {  // V2 (Fedor): "no shiny odds"
  CHECK_EQ(shinyCount(CER_FAREWELL, 500), 0);
  CHECK_EQ(shinyCount(CER_RELEASE, 500), 0);
  CHECK_EQ(shinyCount(CER_RUNAWAY, 500), 0);
}
#else
TEST(egg, letting_go_gives_a_25_percent_shiny_chance) {
  CHECK_RANGE(shinyCount(CER_FAREWELL, 2000), 400, 600);
  CHECK_RANGE(shinyCount(CER_RELEASE, 2000), 400, 600);
  CHECK_MSG(shinyCount(CER_RUNAWAY, 2000) < 150, "a runaway does not get the prize");
}
#endif

TEST(egg, after_a_runaway_there_is_no_choice) {
  Pet p;
  makePet(p, 25);
  p.startRunaway();
  p.newEgg();
  CHECK(!p.awaitingEggPick());
  CHECK(!p.chooseEgg(25));
}

TEST(egg, first_game_is_still_the_starter) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK(p.awaitingStarter());
  CHECK(!p.awaitingEggPick());
}

TEST(egg, choice_stops_time_and_survives_a_reboot) {
  Pet p;
  makePet(p, 25);
#if UI_V2
  p.startFarewell();  // (V2: a goodbye opens the Pokedex pick; before one, it's the starters: v2pick below)
#else
  p.release();
#endif
  p.newEgg();
  uint32_t age = p.ageMinutes;
  mockSetMillis(10 * PET_TICK_MS);
  p.update(10 * PET_TICK_MS);
  CHECK_EQ(p.ageMinutes, age);
  CHECK(p.isEgg());
  Pet again;
  again.begin();
  CHECK(again.awaitingEggPick());
}

TEST(egg, registered_list_is_in_order) {
  Pet p;
  makePet(p, 25);
  p.catchWild(102);
  p.catchWild(4);
  CHECK_EQ(p.registeredAt(0), (int16_t)4);
  CHECK_EQ(p.registeredAt(1), (int16_t)25);
  CHECK_EQ(p.registeredAt(2), (int16_t)102);
  CHECK_EQ(p.registeredAt(3), (int16_t)0);
}

TEST(egg, only_base_forms_can_be_chosen) {
  Pet p;
  makePet(p, 25);
  p.catchWild(26);   // Raichu
  p.catchWild(102);
  p.catchWild(134);  // Vaporeon, without Eevee
  bool shiny = true;
  CHECK_EQ(p.eggChoiceCount(), (uint16_t)2);
  CHECK_EQ(p.eggChoiceAt(0, shiny), (int16_t)25);
  CHECK(!shiny);
  CHECK_EQ(p.eggChoiceAt(1, shiny), (int16_t)102);
  CHECK_EQ(p.eggChoiceAt(2, shiny), (int16_t)0);
  p.startFarewell();
  p.newEgg();
  CHECK_MSG(!p.chooseEgg(26), "an evolved form cannot be chosen");
  CHECK_MSG(!p.chooseEgg(134), "nor one whose base form is missing");
  CHECK(p.chooseEgg(25));
}

TEST(egg, a_shiny_in_the_pokedex_can_be_chosen_and_hatches_shiny) {
  Pet p;
  makePet(p, 25);
  p.catchWild(102, true);  // a shiny Exeggcute only
  p.catchWild(26, true);   // a shiny Raichu: evolved, so not offered
  bool shiny = false;
  CHECK_EQ(p.eggChoiceCount(), (uint16_t)2);
  CHECK_EQ(p.eggChoiceAt(1, shiny), (int16_t)102);
  CHECK(shiny);
  p.startFarewell();
  p.newEgg();
  CHECK_MSG(!p.chooseEgg(102, false), "the regular form isn't in the Pokedex");
  CHECK(p.chooseEgg(102, true));
  hatchNow(p);
  CHECK_EQ(p.speciesId, (int16_t)102);
  CHECK(p.shiny);
}

TEST(egg, runaway_penalizes_rarity) {
  Pet p;
  makePet(p, 4);
  p.lastEnd = CER_RUNAWAY;
  for (int seed = 1; seed <= 60; seed++) {
    randomSeed(seed);
    int16_t d = p.pickEggSpecies();
    CHECK_MSG(DEX_TBL[d].rarity == R_COMMON, "after a runaway only commons come up");
  }
}

#if UI_V2
// ---------------------------------------------------------------- V2: the starters, then the Pokedex (Fedor)
// "For the first game, we get the 4 starters and we will get the 4 starters UNTIL a player has kept a pokemon
// for 4 days and was offered to swap it or keep it. In which case they can now choose from any non evolved
// pokemon from their pikadex."
TEST(v2pick, the_starters_until_a_goodbye_is_offered) {
  Pet p;
  makePet(p, 25);
  CHECK(!p.canPickFromDex());
  p.catchWild(102);
  p.release();
  p.newEgg();
  CHECK(p.awaitingStarter());
  CHECK(!p.awaitingEggPick());
  CHECK_MSG(!p.chooseEgg(102), "no Pokedex pick yet");
  p.chooseStarter(7);
  CHECK_MSG(!p.isEgg(), "a starter is yours at once: no egg");
  CHECK_EQ(p.speciesId, (int16_t)7);
  p.startRunaway();
  p.newEgg();
  CHECK(p.awaitingStarter());
}

TEST(v2pick, a_goodbye_offered_opens_the_pokedex_for_good) {
  Pet p;
  makePet(p, 26);  // Raichu: a final form
  p.catchWild(102);
  p.ageMinutes = FAREWELL_AGE_MIN - 2;
  setStats(p, 90, 90, 90, 90);
  advance(p, 1);
  CHECK(!p.canPickFromDex());
  advance(p, 1);
  CHECK_MSG(p.canFarewellNow() && p.canPickFromDex(), "offered on its 4th day");
  p.declineFarewell();  // "stay together": it was offered all the same
  Pet again;
  again.begin();
  CHECK_MSG(again.canPickFromDex(), "kept in the save");
  again.startRunaway();
  again.newEgg();
  CHECK_MSG(again.awaitingEggPick() && !again.awaitingStarter(), "from now on the Pokedex, even after a runaway");
  CHECK(again.chooseEgg(102));
  CHECK_MSG(!again.isEgg(), "the pick is yours at once");
  CHECK_EQ(again.speciesId, (int16_t)102);
  CHECK(!again.shiny);
}

TEST(v2pick, a_shiny_starter_only_if_you_own_its_shiny) {
  Pet p;
  makePet(p, 25);
  p.release();
  p.newEgg();
  p.chooseStarter(4, true);  // no shiny Charmander in the Pokedex
  CHECK_EQ(p.speciesId, (int16_t)4);
  CHECK(!p.shiny);
  p.catchWild(1, true, 3);  // a 3-star shiny Bulbasaur
  p.release();
  p.newEgg();
  p.chooseStarter(1, true);
  CHECK(p.shiny);
  CHECK_EQ(p.petStars(), (uint8_t)3);
}

// ---------------------------------------------------------------- V2: fights pay, the Master Ball (Fedor)
// V2: a Pokemon's days go by the calendar from the day it came (Fedor: "Lets make sure it counts days for real"),
// its card's age and its goodbye's "Thank you for N days" both; a save from before guesses the day from its age
TEST(v2days, they_go_by_the_calendar) {
  Pet p;
  makePet(p, 7);
  p.setClock(20000UL * 86400 + 20 * 3600);  // it came at 8 pm
  CHECK_EQ(p.daysHad(), (uint32_t)0);
  liveTo(p, 20001UL * 86400 + 9 * 3600);    // 9 am the next morning: its second day
  CHECK_EQ(p.daysHad(), (uint32_t)1);
  liveTo(p, 20010UL * 86400 + 9 * 3600);
  CHECK_EQ(p.daysHad(), (uint32_t)10);      // "Thank you for 11 days", where its minutes lived made 9
  CHECK(p.ageMinutes / 1440 < 10);
  p.flushSave();
  Pet again;
  again.begin();
  CHECK_EQ(again.daysHad(), (uint32_t)10);  // kept in the save
  again.metDay = 0;                         // a save from before: by its age, back from today
  again.ageMinutes = 3 * 1440 + 5;
  again.setClock(20010UL * 86400 + 10 * 3600);  // (which saves)
  Pet old;
  old.begin();
  CHECK_EQ(old.daysHad(), (uint32_t)3);
}

TEST(v2master, a_hundred_fights_earn_a_master_ball) {
  Pet p;
  makePet(p, 25);
  CHECK_EQ(p.masterBalls, (uint8_t)0);
  for (int i = 1; i < MASTER_FIGHTS; i++) CHECK_MSG(!p.masterFight(), "not yet");
  CHECK_EQ(p.masterBar, (uint8_t)(MASTER_FIGHTS - 1));
  CHECK_MSG(p.masterFight(), "the 100th earns one");
  CHECK_EQ(p.masterBalls, (uint8_t)1);
  CHECK_EQ(p.masterBar, (uint8_t)0);
  p.masterFight();
  p.flushSave();  // (the fight's own result would save it)
  Pet again;
  again.begin();
  CHECK_MSG(again.masterBalls == 1 && again.masterBar == 1, "kept in the save");
}

TEST(v2master, a_ball_is_used_once_and_they_stay_through_a_goodbye) {
  Pet p;
  makePet(p, 25);
  for (int i = 0; i < 2 * MASTER_FIGHTS; i++) p.masterFight();
  CHECK_EQ(p.masterBalls, (uint8_t)2);
  CHECK(p.useMasterBall());
  CHECK_EQ(p.masterBalls, (uint8_t)1);
  p.startFarewell();
  p.newEgg();
  CHECK(p.chooseEgg(25));
  CHECK_MSG(p.masterBalls == 1, "the trainer's, not the Pokemon's");
  CHECK(p.useMasterBall());
  CHECK_MSG(!p.useMasterBall(), "none left");
}

TEST(v2master, they_stack_to_99) {
  Pet p;
  makePet(p, 25);
  for (int i = 0; i < 101 * MASTER_FIGHTS; i++) p.masterFight();
  CHECK_EQ(p.masterBalls, (uint8_t)MASTER_MAX);
}

// ---------------------------------------------------------------- V2: the Ultra Ball (Fedor)
TEST(v2ultra, twenty_five_fights_earn_an_ultra_ball_and_a_hundred_a_master_ball) {
  Pet p;
  makePet(p, 25);
  for (int i = 1; i < ULTRA_FIGHTS; i++) CHECK_MSG(p.fightPays() == 0, "not yet");
  CHECK_MSG(p.fightPays() == 2, "the 25th earns an Ultra Ball");
  CHECK_EQ(p.ultraBalls, (uint8_t)1);
  CHECK_EQ(p.ultraBar, (uint8_t)0);
  CHECK_EQ(p.masterBar, (uint8_t)ULTRA_FIGHTS);
  for (int i = ULTRA_FIGHTS + 1; i < MASTER_FIGHTS; i++) p.fightPays();
  CHECK_MSG(p.fightPays() == 3, "the 100th earns both");
  CHECK_EQ(p.ultraBalls, (uint8_t)(MASTER_FIGHTS / ULTRA_FIGHTS));
  CHECK_EQ(p.masterBalls, (uint8_t)1);
  p.fightPays();
  p.flushSave();
  Pet again;
  again.begin();
  CHECK_MSG(again.ultraBalls == 4 && again.ultraBar == 1 && again.masterBalls == 1 && again.masterBar == 1, "kept in the save");
}

TEST(v2ultra, an_ultra_ball_is_used_once) {
  Pet p;
  makePet(p, 25);
  CHECK_MSG(!p.useUltraBall(), "none yet");
  for (int i = 0; i < ULTRA_FIGHTS; i++) p.fightPays();
  CHECK(p.useUltraBall());
  CHECK_EQ(p.ultraBalls, (uint8_t)0);
  CHECK_MSG(!p.useUltraBall(), "none left");
}

// ---------------------------------------------------------------- V2: the Repel (Fedor)
static int dexPowerOf(int16_t d) { return DEX_TBL[d].bHp + DEX_TBL[d].bAtk + DEX_TBL[d].bDef + DEX_TBL[d].bSpe; }

TEST(v2repel, minigame_seconds_earn_one) {
  Pet p;
  makePet(p, 25);
  CHECK(!p.repelPlay((REPEL_PLAY_S - 1) * 1000UL));
  CHECK_EQ(p.repelHeld, (uint8_t)0);
  CHECK(!p.repelPlay(999));
  CHECK_MSG(p.repelPlay(1), "the millisecond that makes the last second");
  CHECK_EQ(p.repelHeld, (uint8_t)1);
  CHECK_EQ(p.repelPlaySec, (uint16_t)0);
}

TEST(v2repel, ten_minutes_now_and_a_twenty_minute_save_keeps_its_play) {
  CHECK_EQ(REPEL_PLAY_S, 600);  // (Fedor: 10 minutes of minigames, from 20)
  Pet a;
  makePet(a, 25);
  a.repelPlay(100 * 1000UL);
  a.flushSave();
  Preferences pr;  // a save from when a Repel took 20 minutes: 15:00 played toward one
  pr.begin(SAVE_NS, false);
  pr.putUShort("rpls", 900);
  pr.end();
  Pet b;
  b.begin();
  CHECK_MSG(b.repelHeld == 1 && b.repelPlaySec == 300, "10 of its 15 minutes are a Repel, 5 carry on");
  pr.begin(SAVE_NS, false);
  pr.putUShort("rpls", 5000);  // more than any firmware saved
  pr.putUChar("rpl", 0);
  pr.end();
  Pet c;
  c.begin();
  CHECK_MSG(c.repelHeld == 0 && c.repelPlaySec == 0, "nonsense starts again");
}

TEST(v2repel, one_runs_ten_minutes_and_cant_be_doubled) {
  Pet p;
  makePet(p, 25);
  CHECK_MSG(!p.useRepel(), "none held");
  p.repelPlay(2UL * REPEL_PLAY_S * 1000);
  CHECK_EQ(p.repelHeld, (uint8_t)2);
  CHECK(p.useRepel());
  CHECK(p.repelOn());
  CHECK_EQ(p.repelMinutesLeft(), (uint8_t)REPEL_MIN);
  CHECK_MSG(!p.useRepel(), "one already running");
  CHECK_EQ(p.repelHeld, (uint8_t)1);
  advance(p, REPEL_MIN - 1);
  CHECK(p.repelOn());
  advance(p, 1);
  CHECK_MSG(!p.repelOn(), "REPEL_MIN of the game's minutes");
  advance(p, 5);  // (five minutes on, the minutes lived are due to be saved; a reboot also applies those it missed)
  p.flushSave();
  Pet again;
  again.begin();
  CHECK_MSG(again.repelHeld == 1 && !again.repelOn(), "kept in the save");
}

TEST(v2repel, the_wild_keeps_weaker_ones_away_shinies_untouched) {
  Pet p;
  makePet(p, 94);  // Gengar
  int weaker = 0;
  for (int seed = 1; seed <= 300; seed++) {
    randomSeed(seed);
    if (dexPowerOf(p.pickWildSpecies(37)) < dexPowerOf(94)) weaker++;
  }
  CHECK_MSG(weaker > 0, "without one, weaker ones come");
  p.repelPlay(REPEL_PLAY_S * 1000UL);
  CHECK(p.useRepel());
  for (int seed = 1; seed <= 300; seed++) {
    randomSeed(seed);
    int16_t d = p.pickWildSpecies(37);
    CHECK_MSG(dexPowerOf(d) >= dexPowerOf(94), "a Repel keeps weaker ones away");
  }
}

TEST(v2repel, an_incomplete_zone_evens_out_and_a_complete_one_keeps_weaker_away) {
  Pet p;
  makePet(p, 94);
  const uint8_t z = Z_FOREST;
  const int16_t legend = ZONE_LEGEND[z];
  int seen[152] = {}, eligible = 0;
  p.repelPlay(REPEL_PLAY_S * 1000UL);
  CHECK(p.useRepel());
  for (int i = 0; i < 6000; i++) {
    randomSeed(i + 1);
    seen[p.pickZoneSpecies(z, 60)]++;
  }
  int lo = 1 << 30, hi = 0;
  for (int16_t d = 1; d <= 151; d++)
    if (zoneOf(d) == z && d != legend && btlMinLevel(d) <= 60) {
      eligible++;
      lo = seen[d] < lo ? seen[d] : lo;
      hi = seen[d] > hi ? seen[d] : hi;
    }
  CHECK(eligible > 5);
  CHECK_MSG(lo > 0 && hi < lo * 2, "about as likely, each of them");
  for (int16_t d = 1; d <= 151; d++)
    if (zoneOf(d) == z) p.catchWild(d);
  CHECK(p.zoneComplete(z));
  for (int seed = 1; seed <= 300; seed++) {
    randomSeed(seed);
    int16_t d = p.pickZoneSpecies(z, 60);
    CHECK_MSG(d == legend || dexPowerOf(d) >= dexPowerOf(94), "complete: as in the wild");
  }
}

TEST(v2pick, an_older_saves_goodbye_counts_and_its_egg_hatches) {
  mockNvsReset();
  mockSetMillis(0);
  Preferences n;
  n.begin(SAVE_NS, false);
  n.putBool("init", true);
  n.putShort("dexn", -1);   // an egg, picked in 1.51 and not hatched
  n.putShort("eggT2", 4);
  n.putUChar("lend", CER_FAREWELL);
  uint8_t reg[19] = { 0x09 };  // Bulbasaur and Charmander in the Pokedex
  n.putBytes("dexreg", reg, sizeof(reg));
  n.end();
  Pet p;
  p.begin();
  CHECK_MSG(p.canPickFromDex(), "it said goodbye in 1.51");
  CHECK(p.isEgg());
  p.update(millis());
  CHECK_MSG(!p.isEgg(), "V2 has no egg: it hatches");
  CHECK_EQ(p.speciesId, (int16_t)4);
}
#endif

// ---------------------------------------------------------------- streak
TEST(streak, first_care_of_the_day_counts) {
  Pet p;
  makePet(p, 4);
  p.setClock(100u * 86400);
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)1);
  CHECK_EQ(p.bestStreak, (uint16_t)1);
  p.caress();
  CHECK_MSG(p.streak == 1, "the second care of the same day does not count");
  liveTo(p, 101u * 86400);
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)2);
}

TEST(streak, a_missed_day_resets) {
  Pet p;
  makePet(p, 4);
  p.setClock(100u * 86400);
  p.caress();
  liveTo(p, 101u * 86400);
  p.caress();
  liveTo(p, 105u * 86400);  // gap of days
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)1);
  CHECK_MSG(p.bestStreak == 2, "the best streak is kept");
}

TEST(streak, no_clock_means_no_streak) {
  Pet p;
  makePet(p, 4);
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)0);
}

TEST(streak, celebrates_milestones) {
  Pet p;
  makePet(p, 4);
  for (uint32_t d = 100; d < 103; d++) {
    liveTo(p, d * 86400);
    p.caress();
  }
  CHECK_EQ(p.streak, (uint16_t)3);
  CHECK_MSG(p.showMilestone(), "3-day milestone celebrated");
  CHECK_EQ(p.lastMilestone, (uint16_t)3);
}

TEST(bonus, streak_and_bond_improve_the_egg) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.careBonus(), 0);
  p.streak = 30;
  p.bond = 100;
  CHECK_EQ(p.careBonus(), 14);
  p.streak = 300;  // the streak is capped at 30
  CHECK_EQ(p.careBonus(), 14);
}

// ---------------------------------------------------------------- medals
TEST(medals, level_10_gives_a_medal) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.hasMedal(MED_LV10));
  p.ageMinutes = 9 * 60 - 1;
  advance(p, 1);
  CHECK(p.hasMedal(MED_LV10));
  CHECK(p.showMedal());
}

// no overweight, level 5 and zero neglects
TEST(medals, fit_reaches_level_5) {
  Pet p;
  makePet(p, 4);
  p.weight = 0;
  p.ageMinutes = 4 * 60 - 1;
  advance(p, 1);
  CHECK(p.hasMedal(MED_FIT));
  CHECK_EQ(p.totalMedals, (uint16_t)1);

  Pet q;
  makePet(q, 4);
  q.careMistakes = 1;   // one neglect rules it out
  q.ageMinutes = 4 * 60 - 1;
  advance(q, 1);
  CHECK(!q.hasMedal(MED_FIT));
}

TEST(medals, each_new_medal_adds_to_the_total) {
  Pet p;
  makePet(p, 4);
  p.weight = 0;
  p.ageMinutes = 9 * 60 - 1;
  advance(p, 1);  // crosses level 10: MED_LV10 + MED_FIT at once
  CHECK(p.hasMedal(MED_LV10));
  CHECK(p.hasMedal(MED_FIT));
  CHECK_MSG(p.totalMedals == 2, "the total counts each bit earned, not each event");
}

TEST(medals, final_form_at_birth) {
  Pet p;
  makePet(p, 143);  // SNORLAX is already a final form
  CHECK(p.hasMedal(MED_FINAL));
  CHECK_EQ(p.totalMedals, (uint16_t)1);
}

TEST(medals, favorite_berry_gives_a_medal) {
  Pet p;
  makePet(p, 4);
  p.feedBerry(1);  // its favorite
  CHECK(p.berryKnown);
  advance(p, 1);
  CHECK(p.hasMedal(MED_BERRY));
}

TEST(medals, total_accumulates_across_pets) {
  Pet p;
  makePet(p, 143);
  CHECK_EQ(p.totalMedals, (uint16_t)1);
  p.newEgg();
  p.chooseStarter(144);  // ARTICUNO, another final form
  p.eggTap(); p.eggTap(); p.eggTap();
#if UI_V2
  CHECK_MSG(p.medals == MED_FINAL && p.totalMedals == 1, "V2: a care medal is earned once, kept through the next");
#else
  CHECK_MSG(p.medals == MED_FINAL, "the individual's medals are reset");
  CHECK_MSG(p.totalMedals == 2, "the player's total is kept");
#endif
}

#if UI_V2
TEST(medals, v2_a_save_from_before_gets_back_the_care_medals_its_records_prove) {
  Pet a;
  makePet(a, 4);
  a.flushSave();
  Preferences pr;  // 2.0: a new Pokemon wiped them, but a 7-day streak and a level 100 were had before
  pr.begin(SAVE_NS, false);
  pr.putUShort("medal", 0);
  pr.putUShort("bstrk", 9);
  pr.putUInt("trmed32", MED_LV100);
  pr.putUChar("l100n", 1);
  pr.end();
  Pet b;
  b.begin();
  uint32_t back = MED_STREAK7 | MED_LV10 | MED_LV25 | MED_LV50;
  CHECK_MSG((b.medals & back) == back, "the streak and the levels' medals are back");
  CHECK_MSG(!(b.medals & (MED_BOND | MED_FIT | MED_BERRY)), "nothing the records can't prove");
  CHECK_MSG(!b.showMedal() && !b.newMedal, "quietly: they were won already");
  b.ageMinutes = 30 * 60;
  advance(b, 1);
  CHECK_MSG(!(b.newMedal & back), "and not won again on the way back up");
}
#endif

TEST(medals, trainer_medals_come_from_the_dex_and_outlive_the_pet) {
  Pet p;
  makePet(p, 4);
  for (int d = 1; d <= 10; d++) p.dexReg[(d - 1) >> 3] |= (uint8_t)(1 << ((d - 1) & 7));
  p.dexShinyReg[0] |= 1;
  advance(p, 1);
  CHECK(p.hasMedal(MED_DEX10));
  CHECK(p.hasMedal(MED_SHINY));
  CHECK(!p.hasMedal(MED_DEX50));
  CHECK(!p.hasMedal(MED_LV100));
  uint16_t total = p.totalMedals;
  p.newEgg();
  p.chooseStarter(144);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_MSG(p.hasMedal(MED_DEX10) && p.hasMedal(MED_SHINY), "the trainer's medals are kept");
  CHECK_MSG(p.totalMedals == total + 1, "only the new pet's own medal counts again");
}

TEST(medals, level_100_is_a_trainer_medal) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 99 * MINUTES_PER_LEVEL;
  advance(p, 1);
  CHECK_EQ(p.level(), (uint16_t)100);
  CHECK(p.hasMedal(MED_LV100));
  CHECK(p.trainerMedals & MED_LV100);
}

TEST(medals, level_stops_at_100) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 400 * MINUTES_PER_LEVEL;  // an old one, from before the cap
  CHECK_EQ(p.level(), (uint16_t)LEVEL_MAX);
  advance(p, 1);
  CHECK_EQ(p.level(), (uint16_t)LEVEL_MAX);
}

TEST(medals, raising_3_to_level_100_earns_a_medal) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 3; i++) {
    p.ageMinutes = 99 * MINUTES_PER_LEVEL;
    advance(p, 1);
    advance(p, 1);  // still the same Pokemon: it counts once
    CHECK_EQ(p.lv100Count, (uint8_t)(i + 1));
    CHECK(p.hasMedal(MED_LV100_3) == (i == 2));
    p.newEgg();
    p.chooseStarter(4);
    p.eggTap(); p.eggTap(); p.eggTap();
  }
  CHECK(!p.hasMedal(MED_LV100_5));
  p.flushSave();
  Pet again;
  again.begin();
  CHECK_EQ(again.lv100Count, (uint8_t)3);
  CHECK(again.hasMedal(MED_LV100_3));
}

// ---------------------------------------------------------------- stars
TEST(stars, a_catch_rolls_1_to_3_with_the_odds) {
  int n[4] = { 0 };
  for (int i = 0; i < 10000; i++) {
    uint8_t s = Pet::rollStars();
    CHECK(s >= 1 && s <= 3);
    n[s]++;
  }
  CHECK_RANGE(n[1], 4500, 5500);  // 50%
  CHECK_RANGE(n[2], 3000, 4000);  // 35%
  CHECK_RANGE(n[3], 1100, 1900);  // 15%
}

TEST(stars, genes_bands) {
  CHECK_EQ(Pet::starsFromGenes(90, 90, 90), (uint8_t)1);
  CHECK_EQ(Pet::starsFromGenes(96, 96, 97), (uint8_t)1);   // 96.3 rounds to 96
  CHECK_EQ(Pet::starsFromGenes(97, 97, 97), (uint8_t)2);
  CHECK_EQ(Pet::starsFromGenes(103, 103, 104), (uint8_t)2);
  CHECK_EQ(Pet::starsFromGenes(104, 104, 104), (uint8_t)3);
  CHECK_EQ(Pet::starsFromGenes(110, 110, 110), (uint8_t)3);
}

TEST(stars, a_new_form_comes_in_with_its_stars) {
  Pet p;
  makePet(p, 4);
  CHECK(p.catchWild(25, false, 2) == CR_NEW);
  CHECK_EQ(p.starsOf(25, false), (uint8_t)2);
  CHECK_EQ(p.starsOf(25, true), (uint8_t)0);  // the shiny one isn't there
}

TEST(stars, a_better_catch_replaces_yours_and_a_worse_one_goes_free) {
  Pet p;
  makePet(p, 4);
  p.catchWild(25, false, 1);
  CHECK(p.catchWild(25, false, 3) == CR_BETTER);
  CHECK_EQ(p.starsOf(25, false), (uint8_t)3);
  CHECK(p.catchWild(25, false, 3) == CR_KEPT);  // the same
  CHECK(p.catchWild(25, false, 2) == CR_KEPT);  // worse
  CHECK_EQ(p.starsOf(25, false), (uint8_t)3);
}

TEST(stars, shiny_and_regular_are_compared_apart) {
  Pet p;
  makePet(p, 4);
  p.catchWild(25, false, 3);
  CHECK(p.catchWild(25, true, 1) == CR_NEW);  // no shiny one yet: new, whatever the regular has
  CHECK_EQ(p.starsOf(25, true), (uint8_t)1);
  CHECK(p.catchWild(25, true, 2) == CR_BETTER);
  CHECK_EQ(p.starsOf(25, false), (uint8_t)3);
}

TEST(stars, an_egg_hatches_in_the_band_of_the_form_picked) {
  for (uint8_t want = 1; want <= 3; want += 2) {
    for (int trial = 0; trial < 20; trial++) {
      Pet p;
      makePet(p, 4);
      p.catchWild(25, false, want);
      p.catchWild(25, false, want);  // (the same stars: stays)
      p.startFarewell();
      p.newEgg();
      CHECK(p.chooseEgg(25, false));
      p.eggTap(); p.eggTap(); p.eggTap();
      CHECK_EQ(p.speciesId, (int16_t)25);
      int lo = want == 1 ? 90 : 104, hi = want == 1 ? 96 : 110;
      CHECK(p.geneAtk >= lo && p.geneAtk <= hi && p.geneDef >= lo && p.geneDef <= hi && p.geneSpe >= lo && p.geneSpe <= hi);
      CHECK_EQ(p.petStars(), want);
      CHECK_EQ(p.starsOf(25, false), want);
    }
  }
}

TEST(stars, raising_never_lowers_a_forms_stars) {
  Pet p;
  makePet(p, 4);
  p.geneAtk = p.geneDef = p.geneSpe = 90;  // a 1-star Charmander
  p.catchWild(5, false, 3);                // a 3-star Charmeleon caught already
  p.ageMinutes = 20 * 60;
  setStats(p, 80, 80, 80, 80);
  CHECK(p.canEvolveNow());
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)5);
  CHECK_EQ(p.starsOf(5, false), (uint8_t)3);  // kept, not lowered to the pet's 1
}

TEST(stars, a_trade_clears_the_given_form_and_keeps_the_higher_received) {
  Pet p;
  makePet(p, 4);
  p.catchWild(25, false, 2);
  p.catchWild(7, false, 3);
  CHECK(!p.tradeAway(25, 7, false, false, 1));  // gives Pikachu, gets a 1-star Squirtle it has with 3
  CHECK_EQ(p.starsOf(25, false), (uint8_t)0);
  CHECK_EQ(p.starsOf(7, false), (uint8_t)3);
  CHECK(p.tradeAway(7, 133, false, false, 0));  // an older buddy sends 0: rolled
  uint8_t s = p.starsOf(133, false);
  CHECK(s >= 1 && s <= 3);
}

TEST(stars, an_old_save_gets_every_form_rated_once) {
  Pet a;
  makePet(a, 4);
  a.catchWild(25, false, 2);
  a.catchWild(7, true, 3);
  a.rename("OLD");  // saves everything
  a.flushSave();
  Preferences pr;  // a save from before the stars
  pr.begin(SAVE_NS, false);
  pr.remove("dexstr");
  pr.end();
  Pet b;
  b.begin();
  CHECK_EQ(b.starsOf(4, false), b.petStars());  // the one being raised: by its genes
  uint8_t s25 = b.starsOf(25, false), s7 = b.starsOf(7, true);
  CHECK(s25 >= 1 && s25 <= 3 && s7 >= 1 && s7 <= 3);
  CHECK_EQ(b.starsOf(1, false), (uint8_t)0);  // not in the Pokedex: none
  b.flushSave();
  Pet c;
  c.begin();
  CHECK_EQ(c.starsOf(25, false), s25);  // not rolled again
  CHECK_EQ(c.starsOf(7, true), s7);
}

// ---------------------------------------------------------------- ceremonies
TEST(ceremony, farewell_only_in_final_form_and_after_3_days) {
  Pet p;
  makePet(p, 6);  // CHARIZARD, final form
  p.ageMinutes = FAREWELL_AGE_MIN - 1;
  CHECK(!p.canFarewellNow());
  p.ageMinutes = FAREWELL_AGE_MIN;
  CHECK(p.canFarewellNow());

  Pet q;
  makePet(q, 4);  // CHARMANDER is not a final form
  q.ageMinutes = FAREWELL_AGE_MIN * 3;
  CHECK(!q.canFarewellNow());
}

TEST(ceremony, staying_together_is_remembered) {
  Pet p;
  makePet(p, 6);
  p.ageMinutes = FAREWELL_AGE_MIN;
  CHECK(p.wantFarewellButton());
  p.declineFarewell();
  CHECK(!p.wantFarewellButton());
  CHECK(p.canFarewellNow());  // still possible: holding the pet offers it
  p.ageMinutes += 3 * 1440;
  CHECK(!p.wantFarewellButton());  // not again by itself, days later...
  Pet q;
  q.begin();
  CHECK(!q.wantFarewellButton());  // ...nor after a restart
}

TEST(ceremony, farewell_leaves_a_new_egg) {
  Pet p;
  makePet(p, 6);
  p.ageMinutes = FAREWELL_AGE_MIN;
  p.startFarewell();
  CHECK_EQ(p.ceremony, (uint8_t)CER_FAREWELL);
  CHECK_RANGE(p.ceremonyT(), 0.0f, 1.0f);
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
  CHECK(p.isEgg());
  CHECK_EQ(p.ceremony, (uint8_t)CER_NONE);
  CHECK_EQ(p.lastEnd, (uint8_t)CER_FAREWELL);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
}

TEST(ceremony, running_away_needs_an_hour_of_total_neglect) {
  Pet p;
  makePet(p, 4);
  setStats(p, 0, 0, 0, 0);
  CHECK(!p.canRunawayNow());
  advance(p, RUNAWAY_TICKS - 1);
  CHECK(!p.canRunawayNow());
  advance(p, 1);
  CHECK(p.canRunawayNow());
}

TEST(ceremony, a_single_care_saves_from_abandonment) {
  Pet p;
  makePet(p, 4);
  p.dbgRunawayReady();
  CHECK(p.canRunawayNow());
  p.feedBerry(0);
  advance(p, 1);
  CHECK_MSG(!p.canRunawayNow(), "eating resets the abandonment counter");
}

TEST(ceremony, no_interaction_during_the_ceremony) {
  Pet p;
  makePet(p, 4);
  setStats(p, 50, 50, 50, 50);
  p.poops = 2;
  p.startRunaway();
  p.feedBerry(0);
  p.feedCandy();
  p.play();
  p.clean();
  p.caress();
  p.toggleLight();
  CHECK_EQ(p.fullness, (uint8_t)50);
  CHECK_EQ(p.joy, (uint8_t)50);
  CHECK_EQ(p.poops, (uint8_t)2);
  CHECK(!p.sleeping);
}

TEST(ceremony, releasing_marks_the_ending_as_release) {
  Pet p;
  makePet(p, 4);
  p.release();
  CHECK_EQ(p.ceremony, (uint8_t)CER_RELEASE);
  CHECK_EQ(p.lastEnd, (uint8_t)CER_RELEASE);
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
  CHECK(p.isEgg());
}

TEST(ceremony, an_egg_cannot_be_bid_farewell_or_released) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.startFarewell();
  p.release();
  p.startRunaway();
  CHECK_EQ(p.ceremony, (uint8_t)CER_NONE);
}

// ---------------------------------------------------------------- offline
TEST(offline, applies_time_off_with_a_floor_of_15) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 600 * 60);  // 600 minutes switched off
  CHECK_EQ(p.ageMinutes, (uint32_t)600);
  CHECK_MSG(p.fullness == 15, "offline food floor = 15");
  CHECK_MSG(p.energy == 15, "offline energy floor = 15");
  CHECK_MSG(p.joy == 15, "offline joy floor = 15");
  CHECK_MSG(p.hygiene == 15, "offline hygiene floor = 15");
  CHECK_MSG(p.careMistakes == 0, "no neglects accumulate during absence");
  CHECK_EQ(p.poops, (uint8_t)2);  // one poop every 4 h
}

TEST(offline, two_week_cap) {
  Pet p;
  makePet(p, 4);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 90u * 86400);  // 90 days
  CHECK_EQ(p.ageMinutes, (uint32_t)(14u * 24 * 60));
}

TEST(offline, under_two_minutes_does_nothing) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 90);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  CHECK_EQ(p.fullness, (uint8_t)80);
}

#if !UI_V2  // V2 has no egg to hatch while away
TEST(offline, egg_hatches_while_you_are_away) {
  Pet p;
  makePet(p, 4);
  p.newEgg();  // there is a pokedex already: no starter prompt
  CHECK(!p.awaitingStarter());
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 600);  // 10 minutes: still waiting for the choice
  CHECK(p.isEgg());
  p.chooseEgg(4);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 600);
  CHECK(!p.isEgg());
  CHECK_EQ(p.speciesId, (int16_t)4);
}
#endif

TEST(offline, sleeping_rests_while_off_too) {
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 20, 90);
  p.toggleLight();
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 120 * 60);
  CHECK_EQ(p.energy, (uint8_t)100);
  CHECK_MSG(p.fullness >= 30, "sleep floors apply offline too");
  CHECK_MSG(p.poops == 0, "sleeping does not make a mess");
}

TEST(offline, no_clock_applies_nothing) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.syncClock(0);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  CHECK_EQ(p.fullness, (uint8_t)80);
}

// ---------------------------------------------------------------- time: only the hours lived count
#define DAY(d, h, m) ((uint32_t)(d) * 86400 + (h) * 3600 + (m) * 60)
#define UTC0 1790000000UL  // some network time (UTC), autumn 2026

TEST(time, setting_the_clock_late_does_not_make_a_day) {
  Pet p;
  makePet(p, 4);
  p.setClock(DAY(1000, 10, 0));
  p.feedCandy();
  p.feedCandy();
  CHECK(!p.canHaveCandy());
  p.setClock(DAY(1000, 23, 59));  // the time screen, set to just before midnight
  advance(p, 2);
  p.noteTime(DAY(1001, 0, 1));
  CHECK_EQ(p.dayNumber(), (uint32_t)1000);
  CHECK_MSG(!p.canHaveCandy(), "no new candies: the day began 10 h ago, not 12");
  advance(p, 118);  // 12 hours lived since the day began at midnight
  CHECK_EQ(p.dayNumber(), (uint32_t)1001);
  CHECK(p.canHaveCandy());
}

TEST(time, doing_it_again_and_again_gives_at_most_two_days_a_day) {
  Pet p;
  makePet(p, 4);
  p.setClock(DAY(1000, 0, 0));
  uint32_t clock = DAY(1000, 0, 0);
  for (int i = 0; i < 24 * 60; i++) {  // a day of play, the clock set to 23:59 every minute
    clock = (clock / 86400) * 86400 + 23 * 3600 + 59 * 60;
    p.setClock(clock);
    advance(p, 1);
    clock += 60;
    p.noteTime(clock);
  }
  CHECK_MSG(p.dayNumber() <= 1002, "12 hours lived per day: no more than 2 days in 24 hours");
}

TEST(time, the_clock_going_back_keeps_the_day) {
  Pet p;
  makePet(p, 4);
  p.setClock(DAY(1000, 12, 0));
  p.feedCandy();
  p.feedCandy();
  p.setClock(DAY(999, 12, 0));  // a day back
  CHECK_EQ(p.dayNumber(), (uint32_t)1000);
  CHECK_MSG(!p.canHaveCandy(), "going back doesn't refill the candies");
}

TEST(time, the_streak_survives_network_time_putting_the_date_back) {
  Pet p;
  makePet(p, 4);
  p.setClock(DAY(1000, 9, 0));
  p.caress();
  liveTo(p, DAY(1001, 9, 0));
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)2);
  p.setClock(DAY(1000, 9, 5));  // the real date comes back a day behind
  p.caress();
  liveTo(p, DAY(1001, 9, 0));
  p.caress();
  liveTo(p, DAY(1002, 9, 0));
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)3);  // not reset to 1: no gap
}

TEST(time, skipping_ahead_needs_the_days_lived) {
  Pet p;
  makePet(p, 4);
  p.setClock(DAY(1000, 12, 0));
  p.caress();
  p.lifeMin += 13 * 60;  // half a day more lived...
  p.setClock(DAY(1003, 12, 0));  // ...but the clock says three days
  CHECK_EQ(p.dayNumber(), (uint32_t)1001);  // one day, so the streak goes on rather than breaking
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)2);
}

TEST(time, an_old_save_starts_today_and_the_next_day_at_midnight) {
  Pet p;
  makePet(p, 4);
  p.setClock(DAY(1000, 21, 0));
  p.flushSave();
  Preferences pr;  // as a save from before the game counted its days
  pr.begin(SAVE_NS, false);
  pr.remove("gday");
  pr.remove("dlife");
  pr.remove("life");
  pr.end();
  Pet q;
  q.begin();
  q.syncClock(DAY(1000, 21, 1));
  CHECK_EQ(q.dayNumber(), (uint32_t)1000);
  advance(q, 3 * 60);  // to just past midnight: the day is taken to have begun at midnight
  q.noteTime(DAY(1001, 0, 1));
  CHECK_EQ(q.dayNumber(), (uint32_t)1001);
}

TEST(time, network_time_applies_the_time_off) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.trustedTime(UTC0, DAY(1000, 8, 0));  // the first one only starts counting
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  advance(p, 10);  // 10 minutes on
  uint32_t age = p.ageMinutes;
  p.trustedTime(UTC0 + 70 * 60, DAY(1000, 9, 10));  // off for an hour
  CHECK_EQ(p.ageMinutes, age + 60);
  CHECK_EQ(p.fullness, (uint8_t)15);
}

TEST(time, what_a_monotonic_clock_applied_is_not_applied_twice) {
  Pet p;
  makePet(p, 4);
  p.trustedTime(UTC0, DAY(1000, 8, 0));
  p.awayMinutes(30);  // the computer slept half an hour
  uint32_t age = p.ageMinutes;
  p.trustedTime(UTC0 + 30 * 60, DAY(1000, 8, 30));
  CHECK_EQ(p.ageMinutes, age);
}

TEST(time, a_clock_or_zone_change_is_no_time_away) {
  Pet p;
  makePet(p, 4);
  p.trustedTime(UTC0, DAY(1000, 10, 0));
  p.clockTrusted = true;
  uint32_t age = p.ageMinutes;
  p.trustedTime(UTC0 + 60, DAY(1001, 1, 1));  // 15 hours east, a minute later
  CHECK_EQ(p.ageMinutes, age);  // a minute: under the two that count
  CHECK_EQ(p.dayNumber(), (uint32_t)1000);  // 10 h lived since midnight: not tomorrow yet
}

TEST(time, a_save_from_before_network_time_measures_from_its_saved_time) {
  Pet p;
  makePet(p, 4);
  p.setClock(DAY(1000, 8, 0));  // saved at 8:00, no network time counted yet
  Pet q;
  q.begin();
  q.trustedTime(UTC0, DAY(1000, 18, 0));  // back at 18:00
  CHECK_EQ(q.ageMinutes, p.ageMinutes + 600);
  q.trustedTime(UTC0 + 120, DAY(1000, 18, 2));  // from here on, by network time
  CHECK_EQ(q.ageMinutes, p.ageMinutes + 602);
}

TEST(time, a_seed_clock_measures_nothing) {
  Pet p;
  makePet(p, 4);
  p.setClock(DAY(500, 0, 0));  // the made-up seed of a Knomi that never had the time
  p.trustedTime(UTC0, DAY(1000, 8, 0), false);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
}

TEST(time, the_count_is_saved) {
  Pet p;
  makePet(p, 4);
  p.trustedTime(UTC0, DAY(1000, 8, 0));
  advance(p, 5);
  p.flushSave();
  Pet q;
  q.begin();
  CHECK_EQ(q.lifeMin, p.lifeMin);
  CHECK_EQ(q.dayNumber(), (uint32_t)1000);
  uint32_t age = q.ageMinutes;
  q.trustedTime(UTC0 + 65 * 60, DAY(1000, 9, 5));  // an hour off after those 5 minutes
  CHECK_EQ(q.ageMinutes, age + 60);
}

// ---------------------------------------------------------------- saving
TEST(save, state_survives_a_reboot) {
  Pet a;
  makePet(a, 25);  // PIKACHU
  a.ageMinutes = 1234;
  a.streak = 9;
  a.bestStreak = 11;
  a.bond = 44;
  a.trAtk = 17;
  a.weight = 33;
  a.gameHi = 21;
  a.runHi = 48;
  a.surfHi = 52;
  a.rename("SPARKY");  // rename persists the whole state

  Pet b;
  b.begin();  // same "NVS": simulates a reboot
  CHECK_EQ(b.speciesId, (int16_t)25);
  CHECK_EQ(b.ageMinutes, (uint32_t)1234);
  CHECK_EQ(b.streak, (uint16_t)9);
  CHECK_EQ(b.bestStreak, (uint16_t)11);
  CHECK_EQ(b.bond, (uint8_t)44);
  CHECK_EQ(b.trAtk, (uint8_t)17);
  CHECK_EQ(b.weight, (uint8_t)33);
  CHECK_EQ(b.gameHi, (uint16_t)21);
  CHECK_EQ(b.runHi, (uint16_t)48);
  CHECK_EQ(b.surfHi, (uint16_t)52);
  CHECK_STREQ(b.nick, "SPARKY");
  CHECK(b.isRegistered(25));
  CHECK(!b.awaitingStarter());
}

// trainer medals past the 16th (MED_PLAYMATE) survive a reboot, and a save from before they
// existed (only the 16-bit "trmedal") still loads every medal it had
TEST(save, trainer_medals_past_16_and_older_saves) {
  Pet a;
  makePet(a, 25);
  a.trainerMedals = MED_SHINY | MED_BUDDY | MED_PLAYMATE;
  a.rename("MEDALS");  // rename persists the whole state
  a.flushSave();
  Pet b;
  b.begin();
  CHECK_EQ(b.trainerMedals, (uint32_t)(MED_SHINY | MED_BUDDY | MED_PLAYMATE));
  CHECK(b.hasMedal(MED_PLAYMATE));

  Preferences p;  // an older firmware's save: no 32-bit key
  p.begin(SAVE_NS, false);
  p.remove("trmed32");
  p.putUShort("trmedal", MED_DEX10 | MED_PARTY);
  p.end();
  Pet c;
  c.begin();
  CHECK_EQ(c.trainerMedals, (uint32_t)(MED_DEX10 | MED_PARTY));
}

TEST(save, showdown_rewards_both_and_gives_the_medal_once) {
  Pet a;
  makePet(a, 25);
  a.joy = 40;
  a.showdownResult(3, 2);  // won
  CHECK(a.joy > 40 + 12);
  CHECK(a.hasMedal(MED_PLAYMATE));
  uint16_t total = a.totalMedals;
  a.showdownResult(1, 4);  // lost: still fun, no second medal
  CHECK_EQ(a.totalMedals, total);
}

TEST(save, nickname_is_truncated_to_11_characters) {
  Pet p;
  makePet(p, 4);
  p.rename("VERY-LONG-NAME-THAT-WONT-FIT");
  CHECK_EQ((int)strlen(p.nick), 11);
  CHECK_STREQ(p.nick, "VERY-LONG-N");
}

TEST(save, factory_reset_leaves_everything_as_new) {
  Pet a;
  makePet(a, 4);
  a.factoryReset();
  Pet b;
  b.begin();
  CHECK(b.isEgg());
  CHECK(b.awaitingStarter());
  CHECK_EQ(b.registeredCount(), (uint16_t)0);
}

TEST(save, autosave_is_flagged_and_flushed) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.savePending());
  advance(p, 5);
  CHECK_MSG(p.savePending(), "every 5 ticks a save is left pending");
  p.flushSave();
  CHECK(!p.savePending());
}

TEST(joy, only_three_pets_per_hour_raise_joy) {
  Pet p;
  makePet(p, 4);
  p.joy = 50;
  for (int i = 0; i < 10; i++) p.caress();
  CHECK_EQ(p.joy, (uint8_t)65);
  CHECK(p.showHeart());  // it still shows gratitude
  mockSetMillis(millis() + PET_JOY_WINDOW_MS);
  p.caress();
  CHECK_EQ(p.joy, (uint8_t)70);
}

TEST(joy, the_visitor_brings_a_third_candy) {
  Pet p;
  makePet(p, 4);
  p.setClock(86400UL * 1000 + 20 * 3600);
  p.feedCandy();
  p.feedCandy();
  CHECK(!p.canHaveCandy());
  p.visitorArrived(16);
  CHECK(p.canHaveCandy());   // its candy
  p.feedCandy();
  CHECK(!p.canHaveCandy());
  Pet q;  // saved
  q.begin();
  q.setClock(86400UL * 1000 + 21 * 3600);
  CHECK(!q.canHaveCandy());
  liveTo(q, 86400UL * 1001 + 20 * 3600);  // the next day: 2 again, until its visitor comes
  CHECK(q.canHaveCandy());
  q.feedCandy();
  q.feedCandy();
  CHECK(!q.canHaveCandy());
}

TEST(joy, two_candies_a_day) {
  Pet p;
  makePet(p, 4);
  p.setClock(86400UL * 1000);
  p.fullness = p.joy = 10;
  CHECK(p.canHaveCandy());
  p.feedCandy();
  p.feedCandy();
  CHECK(!p.canHaveCandy());
  p.feedCandy();
  CHECK_EQ(p.fullness, (uint8_t)30);
  CHECK_EQ(p.joy, (uint8_t)34);
  CHECK_EQ(p.weight, (uint8_t)(2 * WGT_CANDY));
  Pet q;
  q.begin();
  CHECK_MSG(!q.canHaveCandy(), "rebooting does not hand out candies");
  liveTo(p, 86400UL * 1001);
  CHECK(p.canHaveCandy());
  p.feedCandy();
  CHECK_EQ(p.candyToday, (uint8_t)1);
}

TEST(play, tilt_catch_feeds_and_only_costs_energy) {
  Pet p;
  makePet(p, 4);
  p.fullness = 50;
  p.energy = 80;
  p.playResult(4);
  CHECK_EQ(p.fullness, (uint8_t)55);
  CHECK_EQ(p.energy, (uint8_t)68);
}

TEST(trade, yours_leaves_theirs_arrives) {
  Pet p;
  makePet(p, 25);  // raising a Pikachu
  p.catchWild(1);
  p.catchWild(4);
  CHECK(p.isRegistered(25) && p.isRegistered(1) && p.isRegistered(4));
  CHECK(p.tradeAway(1, 7));  // Bulbasaur for Squirtle: new
  CHECK(!p.isRegistered(1));
  CHECK(p.isRegistered(7));
  CHECK(!p.tradeAway(4, 7));  // Charmander for a Squirtle it has: not new, Charmander still gone
  CHECK(!p.isRegistered(4));
  CHECK(p.isRegistered(7));
}

TEST(trade, the_one_you_raise_stays) {
  Pet p;
  makePet(p, 25);
  CHECK(p.isRegistered(25));
  CHECK_EQ(p.tradeableCount(), (uint16_t)0);  // only the partner: nothing to trade
  CHECK(!p.canTrade(25));
  CHECK(!p.tradeAway(25, 7));
  CHECK(p.isRegistered(25));
  CHECK(!p.isRegistered(7));
  p.catchWild(1);
  CHECK_EQ(p.tradeableCount(), (uint16_t)1);
  CHECK(p.tradeAway(1, 1) == false);  // the same species both ways: kept, nothing new
  CHECK(p.isRegistered(1));
}

TEST(shiny, the_pokedex_keeps_both_forms) {
  Pet p;
  makePet(p, 1);
  CHECK(p.catchWild(25, true) == CR_NEW);  // a shiny Pikachu first
  CHECK(p.isRegistered(25));
  CHECK(p.hasForm(25, true) && !p.hasForm(25, false));
  CHECK(p.catchWild(25) == CR_NEW);  // then a regular one: new too
  CHECK(p.hasForm(25, true) && p.hasForm(25, false));
  CHECK(p.catchWild(25) != CR_NEW && p.catchWild(25, true) != CR_NEW);  // each only once
  CHECK(p.hasMedal(MED_SHINY));
  CHECK_EQ(p.formCount(), (uint16_t)3);  // Bulbasaur, Pikachu, shiny Pikachu
  bool sh;
  CHECK_EQ(p.formAt(1, sh), (int16_t)25);
  CHECK(!sh);
  CHECK_EQ(p.formAt(2, sh), (int16_t)25);
  CHECK(sh);
  CHECK_EQ(p.formAt(3, sh), (int16_t)0);
}

TEST(shiny, forms_survive_a_restart) {
  Pet p;
  makePet(p, 1);
  p.catchWild(25, true);
  p.catchWild(4);
  p.catchWild(4, true);
  Pet q;
  q.begin();
  CHECK(q.hasForm(25, true) && !q.hasForm(25, false));
  CHECK(q.hasForm(4, true) && q.hasForm(4, false));
}

TEST(trade, a_shiny_is_traded_on_its_own) {
  Pet p;
  makePet(p, 1);
  p.catchWild(25);
  p.catchWild(25, true);
  CHECK(p.canTrade(25, true) && p.canTrade(25, false));
  CHECK(p.tradeAway(25, 7, true, false));  // the shiny Pikachu for a Squirtle
  CHECK(p.hasForm(25, false) && !p.hasForm(25, true));  // the regular stays
  CHECK(p.tradeAway(25, 4, false, true));  // the regular for a shiny Charmander
  CHECK(!p.isRegistered(25));
  CHECK(p.hasForm(4, true) && !p.hasForm(4, false));
  CHECK(p.tradeAway(7, 7, false, true));  // the same species, the other form: a real swap
  CHECK(p.hasForm(7, true) && !p.hasForm(7, false));
  CHECK(!p.tradeAway(7, 7, true, true));  // the very same both ways: nothing
}

TEST(trade, only_the_form_you_raise_stays) {
  Pet p;
  makePet(p, 25);  // a regular Pikachu
  p.catchWild(25, true);
  CHECK(!p.canTrade(25, false));
  CHECK(p.canTrade(25, true));  // the shiny one can go
  CHECK_EQ(p.tradeableCount(), (uint16_t)1);
}

#if UI_V2
#include "../src/tower.h"
// ---------------------------------------------------------------- Knomi Tower (pet.cpp's part)
TEST(tower, friends_cheer_after_lost_climbs_until_a_new_best) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 5; i++) p.towerLost();
  CHECK_EQ((int)p.towerCheer, TOWER_CHEER_MAX);  // up to three
  p.towerFloorWon(1);
  CHECK_EQ((int)p.towerCheer, 0);  // a new best: it starts over
  p.towerLost();
  p.towerFloorWon(1);  // not a new best: it stays
  CHECK_EQ((int)p.towerCheer, 1);
  p.towerSave();
  Pet q;
  q.begin();
  CHECK_EQ((int)q.towerCheer, 1);  // kept
  p.towerBest = TOWER_LEGEND;
  p.towerCheer = 0;
  p.towerLost();
  CHECK_EQ((int)p.towerCheer, 0);  // once the Legend is beaten, no more cheering (the challenges are its own)
}

TEST(tower, challenges_are_kept_and_new_only_once) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ((int)p.towerChalDone(TOWER_CH_NOFAINT), TOWER_CH_NOFAINT);
  CHECK_EQ((int)p.towerChalDone(TOWER_CH_NOFAINT), 0);
  CHECK_EQ((int)p.towerChalDone(TOWER_CH_NOFAINT | TOWER_CH_BASE), TOWER_CH_BASE);
  Pet q;
  q.begin();
  CHECK_EQ((int)q.towerChal, TOWER_CH_NOFAINT | TOWER_CH_BASE);
}
#endif
