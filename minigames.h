#pragma once
#include <stdint.h>
// Rules for the catch, the quiz and the daily visit, without hardware (test/test_minigames.cpp).

#define CATCH_BALLS 3         // throws per encounter
#define WILD_SHINY_PCT 1      // one wild Pokemon in 100 is shiny
#define RUN_FAIL_PCT 5        // one run in 20 from a wild battle fails: no running for the rest of that fight
#define CATCH_SHAKES 3
#define CATCH_SHAKE_MS 900    // one rock to the side, with its tone
// Three rings around the Pokemon: the closer to its middle the ball lands, the surer the catch
#define CATCH_RING_IN 12      // logical px: the small green circle, a sure catch
#define CATCH_RING_MID 28     // the second ring, about the Pokemon's body: most of the time
#define CATCH_RADIUS 48       // the outer ring: still a hit, but it rarely holds; past it, a miss
#define CATCH_CHANCE_IN 100
#define CATCH_CHANCE_MID 75
#define CATCH_CHANCE_OUT 25

// how likely a ball landing here is to hold (%), 0 for a miss
inline uint8_t catchChance(float ballX, float foeX) {
  float d = ballX - foeX;
  if (d < 0) d = -d;
  return d < CATCH_RING_IN ? CATCH_CHANCE_IN : d < CATCH_RING_MID ? CATCH_CHANCE_MID : d < CATCH_RADIUS ? CATCH_CHANCE_OUT : 0;
}

// a ball that hit: 0 = it holds (caught), or the shake (1..3) the Pokemon breaks out on
inline uint8_t catchBreakAt(uint8_t roll100, uint8_t roll3, uint8_t chance) {
  return roll100 < chance ? 0 : 1 + roll3 % CATCH_SHAKES;
}
#define QUIZ_ROUNDS 5

inline bool catchHit(float ballX, float foeX) {
  float d = ballX - foeX;
  return d > -CATCH_RADIUS && d < CATCH_RADIUS;
}

// exit a game: swipe from the bottom half upward
inline bool swipeUpQuit(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t ms) {
  int dx = x1 - x0;
  return y0 >= 233 && y0 - y1 >= 100 && dx > -90 && dx < 90 && ms < 1000;
}

// 0-23 to the 12-hour clock hour: midnight and noon are 12
inline int clock12Hour(int h24) { return h24 % 12 == 0 ? 12 : h24 % 12; }

// three distinct options with the answer at position pos. a and b are random species.
inline void quizChoices(int16_t answer, int16_t a, int16_t b, uint8_t pos, int16_t out[3]) {
  while (a == answer) a = a % 151 + 1;
  while (b == answer || b == a) b = b % 151 + 1;
  pos %= 3;
  out[pos] = answer;
  out[(pos + 1) % 3] = a;
  out[(pos + 2) % 3] = b;
}

// the visit arrives at a different time each day, between 9 and 18
inline uint8_t visitorHour(uint32_t day) { return 9 + ((day * 2654435761u) >> 28) % 10; }

inline bool visitorDue(uint32_t day, uint8_t hour, uint32_t lastDay) {
  return day != 0 && day != lastDay && hour >= visitorHour(day) && hour < 21;
}
