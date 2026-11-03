// Poke Ball Volley physics (volley.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../volley.h"

TEST(volley, walls_bounce_the_ball_back) {
  VbBall b = { VB_LEFT + 2.0f, 200, -100, 50, 0 };
  CHECK_EQ((int)vbStep(b, 0.1f, 233), (int)VB_WALL);
  CHECK(b.vx > 0 && b.x >= VB_LEFT);
}

TEST(volley, the_paddle_sends_it_back) {
  VbBall b = { 233, VB_PADDLE_Y - 2.0f, 0, 200, 0 };
  CHECK_EQ((int)vbStep(b, 0.05f, 233), (int)VB_HIT);
  CHECK(b.vy < 0);
  VbBall e = { 233 + VB_PADDLE_HALF - 4.0f, VB_PADDLE_Y - 2.0f, 0, 200, 0 };  // near the edge: sideways
  vbStep(e, 0.05f, 233);
  CHECK(e.vx > 0);
}

TEST(volley, a_ball_past_the_paddle_is_a_point) {
  VbBall b = { 100, VB_PADDLE_Y - 2.0f, 0, 300, 0 };
  VbEvent ev = VB_NONE;
  for (int i = 0; i < 40 && ev != VB_MISSED; i++) ev = vbStep(b, 0.02f, 330);  // paddle far away
  CHECK_EQ((int)ev, (int)VB_MISSED);
}

TEST(volley, over_the_net_it_arrives_mirrored) {
  VbBall b = vbServe(150, VB_SPEED0, 0.5f);
  VbEvent ev = VB_NONE;
  for (int i = 0; i < 500 && ev != VB_OVER_NET; i++) ev = vbStep(b, 0.01f, 150);
  CHECK_EQ((int)ev, (int)VB_OVER_NET);
  VbBall a = vbArrive(b.x, b.vx, b.vy, b.ax);
  CHECK(fabsf(a.x - ((VB_LEFT + VB_RIGHT) - b.x)) < 0.01f);
  CHECK(a.vy > 0);                      // coming down on their side
  CHECK(fabsf(a.vx + b.vx) < 0.01f);    // left and right swap
  CHECK(fabsf(a.ax + b.ax) < 0.01f);    // and so does the curve
}

TEST(volley, every_shot_curves) {
  VbBall b = { 233, VB_PADDLE_Y - 2.0f, 0, 200, 0 };  // dead center on the paddle
  vbStep(b, 0.02f, 233);
  CHECK(fabsf(b.ax) > 1);
  CHECK(fabsf(vbServe(233, VB_SPEED0, 0).ax) > 1);
  CHECK(fabsf(vbCurve(0.1f, 1, VB_SPEED_MAX)) > fabsf(vbCurve(0.1f, 1, VB_SPEED0)));  // more when fast
}

// both players stand still in the middle: the curve has to beat one of them soon
TEST(volley, standing_in_the_middle_loses) {
  VbBall b = vbServe(233, VB_SPEED0, 0);
  int crossings = 0;
  VbEvent ev = VB_NONE;
  for (int i = 0; i < 20000 && ev != VB_MISSED; i++) {
    ev = vbStep(b, 0.008f, 233);
    if (ev == VB_OVER_NET) {
      b = vbArrive(b.x, b.vx, b.vy, b.ax);
      crossings++;
    }
  }
  CHECK_EQ((int)ev, (int)VB_MISSED);
  CHECK(crossings < 15);  // about 20 seconds at most
}

// a smooth ramp: no jump at a hit, a little faster every second, never past the top speed
TEST(volley, it_speeds_up_slowly_and_smoothly) {
  VbBall b = { 233, 200, 0, VB_SPEED0, 0 };
  vbStep(b, 1.0f, 233);
  CHECK(fabsf(vbSpeed(b) - (VB_SPEED0 + VB_RAMP)) < 0.5f);
  VbBall h = { 233, VB_PADDLE_Y - 2.0f, 0, 300, 0 };
  vbStep(h, 0.001f, 233);  // a hit doesn't make it jump
  CHECK(vbSpeed(h) < 300.5f);
  VbBall c = { 233, 200, 0, VB_SPEED0, 400 };  // a strong curve turns it without speeding it up
  vbStep(c, 0.1f, 233);
  CHECK(vbSpeed(c) < VB_SPEED0 + VB_RAMP * 0.1f + 0.5f);
  VbBall f = { 233, 200, 0, VB_SPEED_MAX, 0 };
  vbStep(f, 1.0f, 233);
  CHECK(vbSpeed(f) <= VB_SPEED_MAX + 0.5f);
  CHECK(vbNextServeSpeed(VB_SPEED_MAX) > VB_SPEED0);  // the match stays quicker after each point
  CHECK_EQ(vbNextServeSpeed(VB_SPEED0), VB_SPEED0);
}

// two players who chase the ball at `finger` px/s and look again every `react` seconds; returns
// how long a match takes, in seconds (the point banners included), or 999 if it never ends
static float playMatch(float finger, float react) {
  float pad[2] = { 233, 233 }, look[2] = { 0, 0 }, to[2] = { 233, 233 }, speed0 = VB_SPEED0, secs = 0;
  int score[2] = { 0, 0 }, side = 0;
  VbBall b = vbServe(pad[side], speed0, 0);
  while (score[0] < VB_WIN && score[1] < VB_WIN && secs < 999) {
    float dt = 0.008f, st = finger * dt;
    secs += dt;
    if ((look[side] -= dt) <= 0) {
      look[side] = react;
      to[side] = b.vy > 0 ? b.x : 233;
    }
    float &p = pad[side];
    p += to[side] > p + st ? st : to[side] < p - st ? -st : to[side] - p;
    VbEvent ev = vbStep(b, dt, p);
    if (ev == VB_OVER_NET) {
      b = vbArrive(b.x, b.vx, b.vy, b.ax);
      side ^= 1;
    } else if (ev == VB_MISSED) {
      score[side ^ 1]++;
      secs += 3;  // the point banner and the next serve
      speed0 = vbNextServeSpeed(vbSpeed(b));
      b = vbServe(pad[side], speed0, 0);
    }
  }
  return secs;
}

// tilting a Knomi: about 200-350 px/s and a third of a second to react; about a minute a match
TEST(volley, a_match_lasts_about_a_minute) {
  for (float hand = 200; hand <= 350; hand += 75) {
    float s = playMatch(hand, 0.3f);
    CHECK(s > 35 && s < 90);
  }
}

// a quick finger on a touch screen still finishes in a couple of minutes
TEST(volley, quick_players_still_finish) { CHECK(playMatch(500, 0.2f) < 180); }
