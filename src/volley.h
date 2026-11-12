#pragma once
#include <stdint.h>
#include <math.h>

// Poke Ball Volley, the physics (tested on the PC). Each player's screen is their half of the
// court: their Pokemon at the bottom is the paddle, the sides are walls, and the top edge is the
// net. A ball that flies off the top comes in at the top of the buddy's screen, mirrored (they
// face each other). It speeds up a little every moment it flies and keeps most of it after a point, so a
// match to VB_WIN points lasts about a minute.
#define VB_WIN 3            // points to win
#define VB_LEFT 66          // the walls: inside the round screen from the net to the paddle
#define VB_RIGHT 400
#define VB_TOP 70           // the net: past it the ball is on the buddy's side
#define VB_PADDLE_Y 330     // where the ball meets the paddle
#define VB_SPIN_DEG_MS 0.15f // the ball in flight turns slowly: under half a turn a second
#define VB_GROUND_Y 392     // where the Pokemon stands: on the field, below the horizon, as in Berry Drop
#define VB_PADDLE_HALF 46   // half the paddle's width (the Pokemon)
#define VB_BALL_R 32        // the ball's radius on screen: touching the paddle with its edge counts
#define VB_SPEED0 160.0f    // px/s at the first serve
#define VB_SPEED_MAX 700.0f
#define VB_RAMP 10.0f        // px/s faster every second it flies: a smooth, steady speed-up
#define VB_KEEP 0.7f       // after a point, the serve keeps this much of the speed it reached
#define VB_CURVE 30.0f      // px/s^2 of sideways curve at full spin and the first serve's speed
#define VB_CURVE_GROW 1.2f  // each return curves at least this much more than the last: rallies end
#define VB_SPIN_MIN 0.35f   // every shot curves at least this much: standing still doesn't win

struct VbBall {
  float x, y, vx, vy;  // px and px/s; vy < 0 goes up, toward the net
  float ax;            // the curve: sideways px/s^2 from the spin
};

inline float vbSpeed(const VbBall &b) { return sqrtf(b.vx * b.vx + b.vy * b.vy); }

// the curve for a spin of -1..1 (never less than VB_SPIN_MIN either way); stronger as it speeds
// up, so the ball bends about as far across the court however fast it flies
inline float vbCurve(float spin, float tie, float speed) {
  if (spin > -VB_SPIN_MIN && spin < VB_SPIN_MIN) spin = (spin != 0 ? spin : tie) < 0 ? -VB_SPIN_MIN : VB_SPIN_MIN;
  float r = speed / VB_SPEED0;
  return spin * VB_CURVE * r * r;
}

// a serve from your paddle, straight-ish up toward the net
inline VbBall vbServe(float paddleX, float speed, float lean) {
  VbBall b;
  b.x = paddleX;
  b.y = VB_PADDLE_Y - 8;
  b.vx = speed * 0.35f * lean;  // lean -1..1: which way it goes
  b.vy = -sqrtf(speed * speed - b.vx * b.vx);
  b.ax = vbCurve(lean, paddleX < (VB_LEFT + VB_RIGHT) / 2 ? 1 : -1, speed);  // toward the far side
  return b;
}

enum VbEvent : uint8_t { VB_NONE, VB_WALL, VB_HIT, VB_MISSED, VB_OVER_NET };

// one step: walls, the paddle, a miss, or over the net to the buddy
inline VbEvent vbStep(VbBall &b, float dt, float paddleX) {
  // it speeds up a little all the time; the curve only turns it
  float s = vbSpeed(b) + VB_RAMP * dt;
  if (s > VB_SPEED_MAX) s = VB_SPEED_MAX;
  float vmax = s * 0.8f;  // it bends, it doesn't fly flat
  b.vx += b.ax * dt;
  if (b.vx > vmax) b.vx = vmax;
  if (b.vx < -vmax) b.vx = -vmax;
  b.vy = (b.vy < 0 ? -1 : 1) * sqrtf(s * s - b.vx * b.vx);
  b.x += b.vx * dt;
  b.y += b.vy * dt;
  VbEvent ev = VB_NONE;
  // a wall takes the spin off the other way, so the ball doesn't hug it
  if (b.x < VB_LEFT) { b.x = 2 * VB_LEFT - b.x; b.vx = -b.vx; b.ax = -b.ax; ev = VB_WALL; }
  if (b.x > VB_RIGHT) { b.x = 2 * VB_RIGHT - b.x; b.vx = -b.vx; b.ax = -b.ax; ev = VB_WALL; }
  if (b.vy < 0 && b.y < VB_TOP) return VB_OVER_NET;
  if (b.vy > 0 && b.y >= VB_PADDLE_Y) {
    float off = b.x - paddleX;
    if (off > -(VB_PADDLE_HALF + VB_BALL_R / 2) && off < VB_PADDLE_HALF + VB_BALL_R / 2) {
      // back up, angled by where it hit the paddle (the edges send it sideways)
      float k = off / (VB_PADDLE_HALF + VB_BALL_R / 2);  // -1..1
      b.vx = s * 0.6f * k;
      b.vy = -sqrtf(s * s - b.vx * b.vx);
      float was = fabsf(b.ax) * VB_CURVE_GROW;
      b.ax = vbCurve(k, b.vx != 0 ? b.vx : -b.ax, s);  // the edges put the most spin on it
      if (fabsf(b.ax) < was) b.ax = b.ax < 0 ? -was : was;
      b.y = 2 * VB_PADDLE_Y - b.y;
      return VB_HIT;
    }
    if (b.y > VB_PADDLE_Y + 40) return VB_MISSED;  // past the paddle: a point for the buddy
  }
  return ev;
}

// over the net: how it comes into the buddy's screen (they face you: left and right swap)
inline VbBall vbArrive(float x, float vx, float vy, float ax) {
  VbBall b;
  b.x = (VB_LEFT + VB_RIGHT) - x;
  b.vx = -vx;
  b.ax = -ax;  // still curving, mirrored like the rest
  b.vy = vy < 0 ? -vy : vy;  // coming down toward their paddle
  b.y = VB_TOP;
  return b;
}

// the speed the next serve starts at, after a point
inline float vbNextServeSpeed(float reached) {
  float s = reached * VB_KEEP;
  return s < VB_SPEED0 ? VB_SPEED0 : s;
}
