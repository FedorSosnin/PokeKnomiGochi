// Tests of the pet logic (pet.cpp) on top of the host shims.
#include "framework.h"
#include "shim/Arduino.h"
#include "../pet.h"
#include "../dex.h"
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
  p.feedCandy();
  CHECK_EQ(p.fullness, (uint8_t)60);
  CHECK_EQ(p.joy, (uint8_t)62);
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
  e.chooseStarter(4);
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

TEST(tick, good_care_for_12h_builds_defense) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.trDef, (uint8_t)0);
  for (int h = 0; h < 720; h++) {  // keep everything >= 40 for 12 h
    setStats(p, 100, 100, 100, 100);
    advance(p, 1);
  }
  CHECK_EQ(p.trDef, (uint8_t)1);
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
  p.ageMinutes = 254 * 60;
  uint8_t before = p.level();
  CHECK_EQ(before, (uint8_t)255);
  p.ageMinutes = 255 * 60;  // level 256 -> should keep rising (or saturate)
  CHECK_MSG(p.level() >= before, "the level should never drop with age");
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
  p.ageMinutes = 16 * 60;  // levels up: it is offered again
  CHECK(p.wantEvolveButton());
}

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

TEST(stats, egg_has_no_stats) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK_EQ(p.atkStat(), (uint16_t)0);
  CHECK_EQ(p.defStat(), (uint16_t)0);
  CHECK_EQ(p.speStat(), (uint16_t)0);
}

TEST(train, winning_trains_more_than_losing) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.battleResult(true), (uint8_t)BTL_WIN_TRAIN);
  CHECK_EQ(p.battleResult(false), (uint8_t)BTL_LOSS_TRAIN);
  CHECK_EQ(p.trAtk, (uint8_t)(BTL_WIN_TRAIN + BTL_LOSS_TRAIN));
  CHECK_EQ(p.battleWins, (uint16_t)1);
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
  CHECK(p.catchWild(25));
  CHECK(p.isRegistered(25));
  CHECK_MSG(!p.catchWild(25), "it is only new the first time");
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

TEST(visitor, day_is_saved_and_cheers_up_on_leaving) {
  Pet p;
  makePet(p, 4);
  p.setClock(20000UL * 86400 + 20 * 3600);  // 20:00: the hour of any day has already passed
  CHECK(p.visitorDueNow());
  p.visitorArrived();
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

TEST(egg, letting_go_gives_a_25_percent_shiny_chance) {
  CHECK_RANGE(shinyCount(CER_FAREWELL, 2000), 400, 600);
  CHECK_RANGE(shinyCount(CER_RELEASE, 2000), 400, 600);
  CHECK_MSG(shinyCount(CER_RUNAWAY, 2000) < 150, "a runaway does not get the prize");
}

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
  p.release();
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
  p.setClock(101u * 86400);
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)2);
}

TEST(streak, a_missed_day_resets) {
  Pet p;
  makePet(p, 4);
  p.setClock(100u * 86400);
  p.caress();
  p.setClock(101u * 86400);
  p.caress();
  p.setClock(105u * 86400);  // gap of days
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
    p.setClock(d * 86400);
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
  CHECK_MSG(p.medals == MED_FINAL, "the individual's medals are reset");
  CHECK_MSG(p.totalMedals == 2, "the player's total is kept");
}

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
  CHECK_STREQ(b.nick, "SPARKY");
  CHECK(b.isRegistered(25));
  CHECK(!b.awaitingStarter());
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
  p.setClock(86400UL * 1001);
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
