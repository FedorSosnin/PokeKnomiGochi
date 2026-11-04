#pragma once
#include <stdint.h>
// PokeRun: rules without hardware (test/test_minigames.cpp). Logical pixels of the 466 canvas and seconds.
// Difficulty is tuned with these constants.

#define RUN_GROUND_Y 340
#define RUN_PET_X 120
#define RUN_SPAWN_X 490.0f
#define RUN_SEEN_X 438.0f     // where the circle lets you see the ground-level obstacle

#define RUN_SPEED_START 230.0f
#define RUN_SPEED_GAIN 5.0f   // px/s gained each second
#define RUN_SPEED_MAX 760.0f

#define RUN_JUMP_V 620.0f
#define RUN_GRAVITY 1500.0f   // 0.83 s in the air, 128 px high
#define RUN_JUMP_BUFFER 0.12f // pressing just before landing also counts

#define RUN_GAP_START 1.6f    // seconds between obstacles
#define RUN_GAP_MIN 0.95f     // never less: there is always time to land and jump again
#define RUN_GAP_TIGHTEN 45.0f // seconds of play until reaching the minimum
#define RUN_DOUBLE_FROM 20.0f // from here on double rocks can appear

// collision box somewhat smaller than the drawing: grazing doesn't count
#define RUN_PET_HALF_W 15
#define RUN_FORGIVE 4

// the single ones come in turn at random; RUN_DOUBLE (two rocks) only later on
enum RunKind : uint8_t { RUN_ROCK, RUN_BUSH, RUN_DOUBLE, RUN_LOG, RUN_STUMP, RUN_MUSHROOM };
static const RunKind RUN_SINGLES[] = { RUN_ROCK, RUN_BUSH, RUN_LOG, RUN_STUMP, RUN_MUSHROOM };
#define RUN_SINGLE_KINDS 5
struct RunObstacle { float x; RunKind kind; bool on; };

struct RunState {
  float t, dist;       // play time and distance traveled
  float h, vy;         // height above ground and vertical speed
  float buffer;        // jump waiting to land
  float spawnIn;
  RunObstacle ob[4];
  uint32_t rng;
  bool dead, jumped;   // jumped: jumped in this step
};

// all of them no taller than the bush and no wider than the double: every jump clears any of them
inline int runObstacleW(RunKind k) {
  switch (k) {
    case RUN_BUSH: return 44;
    case RUN_DOUBLE: return 78;
    case RUN_LOG: return 64;
    case RUN_STUMP: return 36;
    case RUN_MUSHROOM: return 38;
    default: return 34;
  }
}
inline int runObstacleH(RunKind k) {
  switch (k) {
    case RUN_BUSH: return 44;
    case RUN_LOG: return 24;
    case RUN_STUMP: return 36;
    case RUN_MUSHROOM: return 40;
    default: return 30;
  }
}

inline float runSpeed(float t) {
  float v = RUN_SPEED_START + RUN_SPEED_GAIN * t;
  return v > RUN_SPEED_MAX ? RUN_SPEED_MAX : v;
}

inline float runMinGap(float t) {
  if (t >= RUN_GAP_TIGHTEN) return RUN_GAP_MIN;
  return RUN_GAP_START + (RUN_GAP_MIN - RUN_GAP_START) * t / RUN_GAP_TIGHTEN;
}

inline uint32_t runRand(RunState &s) {
  uint32_t x = s.rng ? s.rng : 1;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return s.rng = x;
}
inline float runRand01(RunState &s) { return (runRand(s) >> 8) / 16777216.0f; }

inline void runStart(RunState &s, uint32_t seed) {
  s = RunState();
  s.rng = seed;
  s.spawnIn = 1.2f;  // a moment to get oriented before the first
}

inline void runSpawn(RunState &s) {
  RunKind k = RUN_SINGLES[runRand(s) % RUN_SINGLE_KINDS];
  // doubles appear gradually: up to one in three
  float dbl = (s.t - RUN_DOUBLE_FROM) / 90.0f;
  if (dbl > 0.33f) dbl = 0.33f;
  if (dbl > 0 && runRand01(s) < dbl) k = RUN_DOUBLE;
  for (auto &o : s.ob)
    if (!o.on) {
      o.x = RUN_SPAWN_X;
      o.kind = k;
      o.on = true;
      break;
    }
  float gap = runMinGap(s.t) * (1.0f + 0.5f * runRand01(s));
  s.spawnIn = runObstacleW(k) / runSpeed(s.t) + gap;
}

inline bool runHits(const RunState &s, const RunObstacle &o) {
  if (!o.on) return false;
  int w = runObstacleW(o.kind), hh = runObstacleH(o.kind);
  return o.x < RUN_PET_X + RUN_PET_HALF_W - 2 && o.x + w > RUN_PET_X - RUN_PET_HALF_W + 2 && s.h < hh - RUN_FORGIVE;
}

// one physics step. A stall (flash save) must not kill: the step is clamped.
inline void runStep(RunState &s, float dt, bool jump) {
  s.jumped = false;
  if (s.dead) return;
  if (dt > 0.12f) dt = 0.12f;
  s.t += dt;
  float v = runSpeed(s.t);
  s.dist += v * dt;

  if (jump) s.buffer = RUN_JUMP_BUFFER;
  if (s.h > 0 || s.vy > 0) {
    s.vy -= RUN_GRAVITY * dt;
    s.h += s.vy * dt;
    if (s.h <= 0) { s.h = 0; s.vy = 0; }
  }
  if (s.h == 0 && s.buffer > 0) {
    s.vy = RUN_JUMP_V;
    s.h = 0.001f;
    s.buffer = 0;
    s.jumped = true;
  }
  if (s.buffer > 0) s.buffer -= dt;

  for (auto &o : s.ob) {
    if (!o.on) continue;
    o.x -= v * dt;
    if (o.x + runObstacleW(o.kind) < -20) o.on = false;
    else if (runHits(s, o)) s.dead = true;
  }
  s.spawnIn -= dt;
  if (s.spawnIn <= 0) runSpawn(s);
}

// the upcoming obstacle, or null
inline const RunObstacle *runNext(const RunState &s) {
  const RunObstacle *best = nullptr;
  for (auto &o : s.ob)
    if (o.on && o.x + runObstacleW(o.kind) > RUN_PET_X - RUN_PET_HALF_W && (!best || o.x < best->x)) best = &o;
  return best;
}

// seconds until touching the upcoming obstacle; negative if there is none
inline float runTimeToContact(const RunState &s) {
  const RunObstacle *o = runNext(s);
  if (!o) return -1;
  float d = o->x - (RUN_PET_X + RUN_PET_HALF_W);
  return d <= 0 ? 0 : d / runSpeed(s.t);
}

// automatic player for tests: jumps shortly before arriving, at any frame rate
inline bool runBotWantsJump(const RunState &s) {
  float c = runTimeToContact(s);
  return s.h == 0 && c > 0 && c < 0.22f;
}
