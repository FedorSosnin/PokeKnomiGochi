// Press preview geometry (press.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../press.h"

static bool insideCircle(int x, int y) {
  int dx = x - 233, dy = y - 233;
  return dx * dx + dy * dy <= 231 * 231;
}
static bool boxInside(PressBox b) {
  return insideCircle(b.x, b.y) && insideCircle(b.x + b.w, b.y) && insideCircle(b.x, b.y + b.h) &&
         insideCircle(b.x + b.w, b.y + b.h);
}

TEST(press, copy_goes_above_the_finger) {
  PressBox b = pressPopupBox(233, 300, 160, 70);
  CHECK(b.y + b.h <= 300 - PRESS_GAP);
  CHECK_EQ(b.x, 233 - 80);
  CHECK(boxInside(b));
}

TEST(press, near_the_top_it_goes_below) {
  PressBox b = pressPopupBox(233, 90, 160, 70);  // games menu title row / Wi-Fi back
  CHECK(b.y >= 90 + PRESS_GAP);
  CHECK(boxInside(b));
}

TEST(press, stays_inside_the_round_edge) {
  // feed button bottom left, clock "-" on the left, a key in the right column
  int16_t fingers[][2] = { { 140, 390 }, { 104, 220 }, { 400, 360 }, { 60, 233 }, { 420, 120 }, { 233, 440 } };
  for (auto &f : fingers) {
    PressBox b = pressPopupBox(f[0], f[1], 200, 80);
    CHECK(boxInside(b));
  }
}

TEST(press, too_wide_is_centered) {
  PressBox b = pressPopupBox(100, 330, 420, 60);
  CHECK_EQ(b.x, 233 - 210);
}

TEST(press, flick_vs_slide) {
  CHECK(pressIsFlick(0, -120, 200));    // swipe up to exit
  CHECK(pressIsFlick(150, 10, 300));    // turn a page
  CHECK(!pressIsFlick(0, -120, 600));   // slow slide: choosing
  CHECK(!pressIsFlick(30, 20, 100));    // a tap that wobbled
}

TEST(press, soft_targets_give_way_to_swipes) {
  CHECK(!pressSoftShows(0, 0, 50));      // just landed: a swipe may be starting
  CHECK(pressSoftShows(5, -8, 200));     // resting on the buddy: preview it
  CHECK(!pressSoftShows(60, 0, 200));    // sliding sideways: a page swipe
  CHECK(!pressSoftShows(0, 90, 600));    // a slow drag down: still a swipe
  CHECK(pressSoftMoved(-31, 0));
  CHECK(!pressSoftMoved(30, -30));       // a wobble
}
