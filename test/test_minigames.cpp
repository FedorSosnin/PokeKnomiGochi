// Catching, quiz and daily visit (minigames.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../minigames.h"
#include "../pokerun.h"
#include "../pet.h"
#include "../dex.h"

TEST(catching, only_a_direct_throw_hits) {
  CHECK(catchHit(233, 233));
  CHECK(catchHit(233 + CATCH_RADIUS - 1, 233));
  CHECK(!catchHit(233 + CATCH_RADIUS, 233));
  CHECK(!catchHit(100, 233));
}

TEST(quiz, three_distinct_options_including_the_answer) {
  for (int answer = 1; answer <= 151; answer += 7)
    for (int a = 1; a <= 151; a += 11)
      for (int b = 1; b <= 151; b += 13)
        for (int pos = 0; pos < 3; pos++) {
          int16_t o[3];
          quizChoices(answer, a, b, pos, o);
          CHECK_EQ((int)o[pos], answer);
          CHECK(o[0] != o[1] && o[1] != o[2] && o[0] != o[2]);
          for (int i = 0; i < 3; i++) CHECK(o[i] >= 1 && o[i] <= 151);
        }
}

TEST(visitor, once_a_day_and_in_daytime) {
  uint32_t day = 20000;
  uint8_t h = visitorHour(day);
  CHECK(h >= 9 && h <= 18);
  CHECK(!visitorDue(day, h - 1, 0));
  CHECK(visitorDue(day, h, 0));
  CHECK_MSG(!visitorDue(day, h, day), "does not repeat on the same day");
  CHECK(!visitorDue(day, 21, 0));
  CHECK(visitorDue(day + 1, 20, day));
  CHECK_MSG(!visitorDue(0, 12, 5), "with no valid hour nobody comes");
}

TEST(visitor, hour_varies_between_days) {
  bool differs = false;
  for (uint32_t d = 20000; d < 20030; d++) differs = differs || visitorHour(d) != visitorHour(20000);
  CHECK(differs);
}

TEST(quit, only_from_bottom_to_top) {
  CHECK(swipeUpQuit(233, 400, 233, 200, 300));
  CHECK_MSG(!swipeUpQuit(233, 200, 233, 40, 300), "from the top does not count");
  CHECK_MSG(!swipeUpQuit(233, 200, 233, 400, 300), "downward does not count");
  CHECK_MSG(!swipeUpQuit(233, 400, 233, 340, 300), "a shifted tap does not count");
  CHECK_MSG(!swipeUpQuit(100, 400, 300, 200, 300), "diagonal does not count");
  CHECK_MSG(!swipeUpQuit(233, 400, 233, 200, 1500), "too slow does not count");
}

static float runBotGame(uint32_t seed, float maxT, float frame) {
  RunState s;
  runStart(s, seed);
  while (!s.dead && s.t < maxT) runStep(s, frame, runBotWantsJump(s));
  return s.t;
}

TEST(pokerun, without_jumping_you_crash_soon) {
  RunState s;
  runStart(s, 7);
  while (!s.dead && s.t < 10) runStep(s, 0.045f, false);
  CHECK(s.dead);
  CHECK(s.t > 2.0f);  // there is time to get oriented
}

TEST(pokerun, it_is_always_passable) {
  for (float frame : { 0.03f, 0.045f, 0.085f })
    for (uint32_t seed = 1; seed <= 100; seed++)
      CHECK(runBotGame(seed * 97, 300, frame) >= 300);
}

TEST(pokerun, gets_faster_with_a_cap) {
  CHECK(runSpeed(30) > runSpeed(0));
  CHECK(runSpeed(60) > runSpeed(30));
  CHECK(runSpeed(1000) == RUN_SPEED_MAX);
  CHECK(runMinGap(0) > runMinGap(30));
  CHECK(runMinGap(1000) == RUN_GAP_MIN);
}

TEST(pokerun, challenge_arrives_between_30_and_60_seconds) {
  // margin from seeing the obstacle until touching it: generous at first, tight later
  float seen0 = (RUN_SEEN_X - RUN_PET_X) / runSpeed(0), seen30 = (RUN_SEEN_X - RUN_PET_X) / runSpeed(30),
        seen60 = (RUN_SEEN_X - RUN_PET_X) / runSpeed(60);
  CHECK(seen0 > 1.2f);
  CHECK(seen30 > 0.75f);
  CHECK(seen60 < 0.65f);
}

TEST(pokerun, a_stall_does_not_kill) {
  RunState s;
  runStart(s, 7);
  float x;
  for (int i = 0; i < 40; i++) runStep(s, 0.045f, false);
  x = s.dist;
  runStep(s, 5.0f, false);
  CHECK(s.dist - x < runSpeed(s.t) * 0.13f);
}

TEST(pokerun, record_is_saved_and_old_saves_unchanged) {
  Pet p;
  p.begin();
  p.chooseStarter(1);
  p.eggTap(); p.eggTap(); p.eggTap();
  p.gameHi = 130;
  p.runResult(48);
  CHECK_EQ(p.runHi, (uint16_t)48);
  CHECK_MSG(p.gameHi == 130, "does not touch the Tilt Catch record");
  p.runResult(20);
  CHECK_EQ(p.runHi, (uint16_t)48);
}

TEST(clock, 12_hour_clock) {
  CHECK_EQ(clock12Hour(0), 12);
  CHECK_EQ(clock12Hour(1), 1);
  CHECK_EQ(clock12Hour(11), 11);
  CHECK_EQ(clock12Hour(12), 12);
  CHECK_EQ(clock12Hour(13), 1);
  CHECK_EQ(clock12Hour(23), 11);
}

TEST(catch_, the_shakes_hold_as_often_as_the_chance) {
  for (uint8_t chance : { CATCH_CHANCE_IN, CATCH_CHANCE_MID, CATCH_CHANCE_OUT }) {
    int held = 0, at[4] = {};
    for (int roll = 0; roll < 100; roll++)
      for (int r3 = 0; r3 < 3; r3++) {
        uint8_t b = catchBreakAt(roll, r3, chance);
        CHECK(b <= CATCH_SHAKES);
        at[b]++;
        held += b == 0;
      }
    CHECK_EQ(held, chance * 3);
    CHECK(at[1] == at[2] && at[2] == at[3]);  // a break-out on any of the three shakes alike
  }
}

// the closer to the middle, the surer: the green circle always holds, the second ring three times in four
TEST(catch_, the_rings_reward_a_good_aim) {
  CHECK_EQ(catchChance(233, 233), CATCH_CHANCE_IN);
  CHECK_EQ(catchChance(233 + CATCH_RING_IN - 1, 233), CATCH_CHANCE_IN);
  CHECK_EQ(catchChance(233 - CATCH_RING_IN, 233), CATCH_CHANCE_MID);
  CHECK_EQ(catchChance(233 + CATCH_RING_MID, 233), CATCH_CHANCE_OUT);
  CHECK_EQ(catchChance(233 - CATCH_RADIUS + 1, 233), CATCH_CHANCE_OUT);
  CHECK_EQ(catchChance(233 + CATCH_RADIUS, 233), 0);
  CHECK(catchChance(233 + CATCH_RADIUS, 233) == 0 && !catchHit(233 + CATCH_RADIUS, 233));  // the same edge
}
