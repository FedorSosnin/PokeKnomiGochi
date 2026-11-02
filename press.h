#pragma once
#include <math.h>
#include <stdint.h>

// Press preview, like the Wi-Fi keyboard: while the finger is down, the control under it is
// outlined and a large copy floats beside the finger (which covers the real one). The tap
// fires on lift, at the control under the finger at that moment, so sliding to the neighbour
// changes the choice. Made for big fingers on a 1.28" screen. Only geometry here (tested on the PC).
#define PRESS_GAP 56         // a fingertip covers ~50 logical px: the copy goes beyond it
#define PRESS_EDGE 20        // margin from the round edge
#define PRESS_FLICK_MS 350   // faster than this and longer than PRESS_FLICK_PX: still a swipe
#define PRESS_FLICK_PX 80

// the control under the finger
struct PressTarget {
  int16_t x, y, w, h;       // the drawn control, to outline it
  const char *label;        // the large copy: text...
  const char *const *icon;  // ...or a 16x16 icon
};

struct PressBox {
  int16_t x, y, w, h;
};

// where the w x h copy goes for a finger at (fx, fy): above it, below when there is no room
// at the top, and always inside the round screen (center c, radius r)
inline PressBox pressPopupBox(int16_t fx, int16_t fy, int16_t w, int16_t h, int16_t c = 233, int16_t r = 231) {
  const int16_t top = c - r + PRESS_EDGE, bottom = c + r - PRESS_EDGE;
  int16_t y = fy - PRESS_GAP - h;
  if (y < top) y = fy + PRESS_GAP;  // finger near the top: below it
  if (y + h > bottom) y = bottom - h;
  if (y < top) y = top;
  // the corners must stay inside the circle: the row farthest from the center limits it
  int32_t dy = y + h - c > c - y ? y + h - c : c - y;
  int32_t lim = (int32_t)r - PRESS_EDGE;
  int16_t half = dy < lim ? (int16_t)sqrtf((float)(lim * lim - dy * dy)) : 0;
  int16_t x = fx - w / 2;
  if (w > 2 * half) x = c - w / 2;  // wider than the chord: centered is the best it can do
  else if (x < c - half) x = c - half;
  else if (x + w > c + half) x = c + half - w;
  return { x, y, w, h };
}

// a quick, long flick is still a swipe (pages, swipe up to exit) even if it ends on a control
inline bool pressIsFlick(int dx, int dy, uint32_t dt) {
  return dt < PRESS_FLICK_MS && (int32_t)dx * dx + (int32_t)dy * dy > PRESS_FLICK_PX * PRESS_FLICK_PX;
}
