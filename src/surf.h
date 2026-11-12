#pragma once
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
// PikaSurf: rules without hardware (test/test_minigames.cpp). Logical pixels of the 466 canvas and seconds.
// Your Pokemon surfs down the screen on a board, near the top, and the water rushes up past it. Rocks come
// up from the bottom in rows across three lanes, one or two to a row (never all three), faster and closer
// together as in PokeRun. Pikachu surfs alongside and never touches one; after a little while he is just
// faster, pulls ahead and is gone off the bottom. One rock and the run is over.
#define SURF_CX 233
#define SURF_PET_Y 150         // where yours rides: the middle of its board
#define SURF_LANE_DX 100       // the lanes: SURF_CX - 100, SURF_CX, SURF_CX + 100
#define SURF_X_MIN 100         // how far to the sides yours can go
#define SURF_X_MAX 366
#define SURF_SPAWN_Y 600.0f    // rows start well below the screen, so Pikachu sees them coming even far ahead...
#define SURF_GONE_Y -50.0f     // ...and are gone above it
#define SURF_SPEED_START 200.0f  // px/s the water rushes up
#define SURF_SPEED_GAIN 5.0f     // gained each second, as in PokeRun
#define SURF_SPEED_MAX 640.0f
#define SURF_GAP_START 1.5f      // seconds between rows
#define SURF_GAP_MIN 0.8f
#define SURF_GAP_TIGHTEN 45.0f   // seconds of play until the minimum
#define SURF_DOUBLE_FROM 12.0f   // from here on a row can block two lanes, up to half of them
#define SURF_MOVE_V 259.0f       // how fast yours crosses at full tilt (Berry Drop's: 22 px every 85 ms)
#define SURF_REACT 0.3f          // time to see a row and start moving, on top of the crossing
#define SURF_ROCK_HALF_W 36      // a rock (its pixel art at 5x, 70 px): two side by side leave no room to sit
#define SURF_ROCK_HALF_H 24      // between them, so the way through is always an open lane
#define SURF_PET_HALF_W 18       // yours on its board: what can touch a rock (the board is long: its nose counts)
#define SURF_PET_HALF_H 30
#define SURF_PIKA_HALF_W 14      // Pikachu on his smaller board
#define SURF_PIKA_HALF_H 22
#define SURF_ROWS 4
#define SURF_FIXED_DT (1.0f / 120)  // the game steps in these, whatever the frame rate: two boards in a duel,
                                    // started from the same seed, then see the very same rows
// Pikachu: beside yours at first, then pulling ahead until he is out of sight (about 40 s in)
#define SURF_PIKA_DX 76          // his place beside yours
#define SURF_PIKA_PULL_FROM 10.0f
#define SURF_PIKA_PULL_V 12.0f   // px/s he gains on you...
#define SURF_PIKA_DASH_Y 400.0f  // ...three times that from here on, on his way out of sight
#define SURF_PIKA_LOOK 340.0f    // how far ahead he looks for the next row
#define SURF_PIKA_V 640.0f       // how fast he crosses
#define SURF_PIKA_GONE_Y 520.0f
#define SURF_SEEN_Y 446          // a rock whose top is lower than this is still under the screen's edge

struct SurfRow {
  float y;
  uint8_t lanes;     // the blocked ones: bit 0 the left lane, 1 the middle, 2 the right
  int8_t jitter[3];  // the row a little off the lanes' middles (all its rocks alike: the gaps never widen)
  bool on;
};
struct SurfState {
  float t, dist;     // play time and how far the water has gone by
  float x;           // yours
  float spawnIn, sinceRow;
  uint8_t lastSafe;  // the lanes the last row left open (bits as in SurfRow)
  SurfRow rows[SURF_ROWS];
  float pikaX, pikaY;
  int8_t pikaSide;   // which side of yours he likes to ride
  uint32_t rng;
  bool dead;
};

inline float surfSpeed(float t) {
  float v = SURF_SPEED_START + SURF_SPEED_GAIN * t;
  return v > SURF_SPEED_MAX ? SURF_SPEED_MAX : v;
}

inline float surfMinGap(float t) {
  if (t >= SURF_GAP_TIGHTEN) return SURF_GAP_MIN;
  return SURF_GAP_START + (SURF_GAP_MIN - SURF_GAP_START) * t / SURF_GAP_TIGHTEN;
}

inline int surfLaneX(int lane) { return SURF_CX + (lane - 1) * SURF_LANE_DX; }
inline int surfRockX(const SurfRow &r, int lane) { return surfLaneX(lane) + r.jitter[lane]; }

inline uint32_t surfRand(SurfState &s) {
  uint32_t x = s.rng ? s.rng : 1;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return s.rng = x;
}
inline float surfRand01(SurfState &s) { return (surfRand(s) >> 8) / 16777216.0f; }

inline void surfStart(SurfState &s, uint32_t seed) {
  s = SurfState();
  s.rng = seed;
  s.x = SURF_CX;
  s.spawnIn = 1.4f;  // a moment to find your feet before the first
  s.lastSafe = 7;
  s.pikaX = SURF_CX + SURF_PIKA_DX;
  s.pikaY = SURF_PET_Y;
  s.pikaSide = 1;
}

// the longest crossing (in lanes) from any lane the last row left open to the given lane
inline int surfReach(uint8_t lastSafe, int lane) {
  int far = 0;
  for (int i = 0; i < 3; i++)
    if (lastSafe >> i & 1) far = abs(i - lane) > far ? abs(i - lane) : far;
  return far;
}

inline void surfSpawn(SurfState &s) {
  SurfRow *slot = nullptr;
  for (auto &r : s.rows)
    if (!r.on) { slot = &r; break; }
  if (!slot) { s.spawnIn = 0.1f; return; }  // all four still on screen: wait a moment
  // a double appears gradually, up to half the rows; the opening it leaves must be one yours can still
  // reach from wherever the last row let it through, in the time between the two
  float dbl = (s.t - SURF_DOUBLE_FROM) / 60.0f;
  if (dbl > 0.5f) dbl = 0.5f;
  uint8_t lanes;
  if (dbl > 0 && surfRand01(s) < dbl) {
    int open = surfRand(s) % 3;
    float need = SURF_REACT + surfReach(s.lastSafe, open) * SURF_LANE_DX / SURF_MOVE_V;
    if (need > s.sinceRow) open = 1;  // the middle is at most one lane from anywhere
    need = SURF_REACT + surfReach(s.lastSafe, open) * SURF_LANE_DX / SURF_MOVE_V;
    if (need > s.sinceRow) lanes = 1 << (surfRand(s) % 3);  // not even that: a single rock instead
    else lanes = 7 & ~(1 << open);
  } else {
    lanes = 1 << (surfRand(s) % 3);
  }
  slot->on = true;
  slot->y = SURF_SPAWN_Y;
  slot->lanes = lanes;
  int8_t j = (int8_t)((int)(surfRand(s) % 17) - 8);
  for (int i = 0; i < 3; i++) slot->jitter[i] = j;
  s.lastSafe = 7 & ~lanes;
  s.sinceRow = 0;
  s.spawnIn = surfMinGap(s.t) * (1.0f + 0.4f * surfRand01(s));
}

// does a body at x (half width hw), level with the row, touch one of its rocks?
inline bool surfRowHitsAt(const SurfRow &r, float x, int hw) {
  for (int i = 0; i < 3; i++)
    if ((r.lanes >> i & 1) && fabsf(surfRockX(r, i) - x) < SURF_ROCK_HALF_W + hw - 2) return true;
  return false;
}

inline bool surfHits(const SurfState &s, const SurfRow &r) {
  if (!r.on || fabsf(r.y - SURF_PET_Y) >= SURF_ROCK_HALF_H + SURF_PET_HALF_H) return false;
  return surfRowHitsAt(r, s.x, SURF_PET_HALF_W);
}

// the next row coming up at y (not yet past a body halfH tall), within reach of sight, or null
inline const SurfRow *surfNextRow(const SurfState &s, float y, float look, int halfH) {
  const SurfRow *best = nullptr;
  for (auto &r : s.rows)
    if (r.on && r.y > y - (SURF_ROCK_HALF_H + halfH) && r.y < y + look && (!best || r.y < best->y)) best = &r;
  return best;
}

// the middle of the open lane nearest x, in that row
inline float surfOpenNear(const SurfRow &r, float x) {
  float best = x, bestD = 1e9f;
  for (int i = 0; i < 3; i++) {
    if (r.lanes >> i & 1) continue;
    float d = fabsf(surfLaneX(i) - x);
    if (d < bestD) { bestD = d; best = (float)surfLaneX(i); }
  }
  return best;
}

// does going from x0 to x1 pass a rock of the row?
inline bool surfCrosses(const SurfRow &r, float x0, float x1, int hw) {
  float lo = x0 < x1 ? x0 : x1, hi = x0 < x1 ? x1 : x0;
  for (int i = 0; i < 3; i++) {
    if (!(r.lanes >> i & 1)) continue;
    float rx = (float)surfRockX(r, i);
    if (rx + SURF_ROCK_HALF_W + hw > lo && rx - SURF_ROCK_HALF_W - hw < hi) return true;
  }
  return false;
}

// Pikachu: rides beside yours, or makes for the opening of the row he is about to meet; never touches a rock
inline void surfPikaStep(SurfState &s, float dt) {
  if (s.t > SURF_PIKA_PULL_FROM) s.pikaY += (s.pikaY < SURF_PIKA_DASH_Y ? 1 : 3) * SURF_PIKA_PULL_V * dt;
  if (s.pikaY > SURF_PIKA_GONE_Y) return;  // out of sight ahead
  float home = s.x + s.pikaSide * SURF_PIKA_DX;
  if (home > SURF_X_MAX || home < SURF_X_MIN) {  // no room on that side: the other
    s.pikaSide = -s.pikaSide;
    home = s.x + s.pikaSide * SURF_PIKA_DX;
  }
  float want = home < SURF_X_MIN ? SURF_X_MIN : home > SURF_X_MAX ? SURF_X_MAX : home;
  if (const SurfRow *r = surfNextRow(s, s.pikaY, SURF_PIKA_LOOK, SURF_PIKA_HALF_H)) {
    if (surfRowHitsAt(*r, s.pikaX, SURF_PIKA_HALF_W + 4)) want = surfOpenNear(*r, s.pikaX);  // in the way: the nearest opening
    else if (surfCrosses(*r, s.pikaX, want, SURF_PIKA_HALF_W + 4)) want = s.pikaX;  // safe where he is: he stays, never across one
  }
  float step = SURF_PIKA_V * dt, d = want - s.pikaX;
  float nx = s.pikaX + (d > step ? step : d < -step ? -step : d);
  for (auto &r : s.rows)  // never into a rock level with him: he holds his line until it has gone by
    if (r.on && fabsf(r.y - s.pikaY) < SURF_ROCK_HALF_H + SURF_PIKA_HALF_H + 4 &&
        surfRowHitsAt(r, nx, SURF_PIKA_HALF_W + 2) && !surfRowHitsAt(r, s.pikaX, SURF_PIKA_HALF_W + 2))
      return;
  s.pikaX = nx;
}

// one step: tilt -1..1 steers yours. A stall (flash save) must not kill: the step is clamped.
inline void surfStep(SurfState &s, float dt, float tilt) {
  if (s.dead) return;
  if (dt > 0.12f) dt = 0.12f;
  s.t += dt;
  s.sinceRow += dt;
  float v = surfSpeed(s.t);
  s.dist += v * dt;
  s.x += tilt * SURF_MOVE_V * dt;
  if (s.x < SURF_X_MIN) s.x = SURF_X_MIN;
  if (s.x > SURF_X_MAX) s.x = SURF_X_MAX;
  for (auto &r : s.rows) {
    if (!r.on) continue;
    r.y -= v * dt;
    if (r.y < SURF_GONE_Y) r.on = false;
    else if (surfHits(s, r)) s.dead = true;
  }
  s.spawnIn -= dt;
  if (s.spawnIn <= 0) surfSpawn(s);
  surfPikaStep(s, dt);
}

// how far the water has gone by after t seconds (the speed's integral): what a duel compares
inline float surfDistAt(float t) {
  float tMax = (SURF_SPEED_MAX - SURF_SPEED_START) / SURF_SPEED_GAIN;  // when it stops speeding up
  if (t <= tMax) return SURF_SPEED_START * t + SURF_SPEED_GAIN * t * t / 2;
  return surfDistAt(tMax) + SURF_SPEED_MAX * (t - tMax);
}

// A duel with a buddy (both on the same course): who lasted longer, in tenths of a second as they
// travel; 1 yours, -1 theirs, 0 a draw
inline int surfDuelOutcome(uint16_t mine10, uint16_t theirs10) {
  return mine10 > theirs10 ? 1 : mine10 < theirs10 ? -1 : 0;
}

// automatic player for tests and the screenshots: makes for the opening of the next row, at full tilt
inline float surfBotTilt(const SurfState &s) {
  const SurfRow *r = surfNextRow(s, SURF_PET_Y, 600, SURF_PET_HALF_H);
  if (!r || !surfRowHitsAt(*r, s.x, SURF_PET_HALF_W + 4)) return 0;
  float d = surfOpenNear(*r, s.x) - s.x;
  float t = d / 30.0f;
  return t > 1 ? 1 : t < -1 ? -1 : t;
}
