#pragma once
#include <stdint.h>

// Automatic rotation of the Knomi screen. rot = quarter turns clockwise of the image over
// the panel. The game always sees a straight canvas: here only pixels, touches and tilt are translated.
// Accelerometer axes relative to the unrotated panel: +X to the right, +Y down; the axis that
// points up reads +1 g.
#define ROT_MIN_G 0.6f       // below this it is nearly flat: it stays as it was
#define ROT_MARGIN_G 0.2f    // on a diagonal it doesn't change either, so it doesn't flicker between two sides
#define ROT_STABLE_MS 600UL  // it must hold this way for a while

#ifdef BOARD_HOST
inline uint8_t gPanelRot = 0;  // a window doesn't rotate
#else
inline uint8_t gPanelRot = 1;  // at boot: in the right hand, with the cable on the left
#endif

inline uint8_t rotFromGravity(float x, float y, uint8_t cur) {
  float ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
  float top = ax > ay ? ax : ay, gap = ax > ay ? ax - ay : ay - ax;
  if (top < ROT_MIN_G || gap < ROT_MARGIN_G) return cur;
  if (ax > ay) return x > 0 ? 1 : 3;
  return y > 0 ? 2 : 0;
}

struct RotFilter {
  int8_t candidate = -1;
  uint32_t since = 0;
  void reset() { candidate = -1; }
  // true if it must switch to *rot: the new side has already been still for ROT_STABLE_MS
  bool update(float x, float y, uint32_t now, uint8_t *rot) {
    uint8_t want = rotFromGravity(x, y, *rot);
    if (want == *rot) {
      candidate = -1;
      return false;
    }
    if (candidate != want) {
      candidate = want;
      since = now;
      return false;
    }
    if (now - since < ROT_STABLE_MS) return false;
    *rot = want;
    candidate = -1;
    return true;
  }
};

// where pixel (x, y) of the image lands on the panel: index = base + x * dx + y * dy
struct RotBlit { int32_t base, dx, dy; };
inline RotBlit rotBlit(uint8_t rot, int32_t size) {
  switch (rot & 3) {
    case 1: return { size - 1, size, -1 };
    case 2: return { size * size - 1, -1, -size };
    case 3: return { (size - 1) * size, -size, 1 };
    default: return { 0, 1, size };
  }
}

// from a panel point (px, py) to the image point seen there
inline void rotTouch(uint8_t rot, int32_t px, int32_t py, int32_t size, int32_t *x, int32_t *y) {
  switch (rot & 3) {
    case 1: *x = py; *y = size - 1 - px; break;
    case 2: *x = size - 1 - px; *y = size - 1 - py; break;
    case 3: *x = size - 1 - py; *y = px; break;
    default: *x = px; *y = py; break;
  }
}

// tilting to the left of the image is negative, whichever side is acting as up
inline float rotTilt(const float g[3], uint8_t rot) {
  switch (rot & 3) {
    case 1: return -g[1];
    case 2: return g[0];
    case 3: return g[1];
    default: return -g[0];
  }
}
