#pragma once
// V2: the Knomi's screen darkens softly into its rim, so the picture looks round and a little bubbly. It's what was
// kept of a CRT look tried on the Knomi (Fedor: "I dont think it works on such a small screen. The effect makes
// things harder to read and see. I do like the edges of the screen having a bit of shadow though. Makes it look
// bubbly."): the computers' CRT screen's rim (host/host_crt.cpp), done to the panel's pixels near the edge as the
// frame goes out (ScaledCanvas::flush()). The middle is left as it is; the corners the round screen hides are black.
#include <stdint.h>
#include <math.h>
#include <esp_heap_caps.h>
#include <string.h>
#define KNOMI_RIM 1  // (scene_v2.h: the need bars' labels go over it)

static uint8_t *gKnomiRim = nullptr;  // each panel pixel's darkening, 255 = none (PSRAM)
static int16_t gKnomiRimX0[256], gKnomiRimX1[256];  // each row's columns on the round screen (the rest is black)
static int16_t gKnomiRimI0[256], gKnomiRimI1[256];  // ...and those in the middle, left as they are

// the table, once (n: the panel's side); false if there's no room (then the frame goes out as it is)
static bool knomiRimBegin(int n) {
  if (gKnomiRim) return true;
  gKnomiRim = (uint8_t *)ps_malloc((size_t)n * n);
  if (!gKnomiRim) return false;
  // as host_crt.cpp's rim: a soft darkening n/30 wide, and the picture fading into a black edge 1.2% in
  const float R = n / 2.0f, er = R - 0.012f * n, sv = n / 30.0f * 1.41421356f, se = n / 160.0f * 1.41421356f;
  for (int y = 0; y < n; y++) {
    int x0 = n, x1 = 0, i0 = n, i1 = 0;
    for (int x = 0; x < n; x++) {
      float dx = x + 0.5f - R, dy = y + 0.5f - R, r = sqrtf(dx * dx + dy * dy);
      int v = (int)(0.5f * erfcf((r - R) / sv) * 0.5f * erfcf((r - er) / se) * 255 + 0.5f);
      gKnomiRim[y * n + x] = (uint8_t)v;
      if (v) x0 = x < x0 ? x : x0, x1 = x + 1;
      if (v == 255) i0 = x < i0 ? x : i0, i1 = x + 1;
    }
    if (i0 > i1) i0 = i1 = x0;  // (a row with no middle)
    gKnomiRimX0[y] = x0, gKnomiRimX1[y] = x1 > x0 ? x1 : x0, gKnomiRimI0[y] = i0, gKnomiRimI1[y] = i1;
  }
  return true;
}

// What's drawn over the shadow (Fedor: "make the food, energy, joy, and hyg lables be above the shadow. So they are
// very easy to see"): the canvas's 2 x 2 cells that the need bars' labels drew this frame (scene_v2.h), which the rim
// leaves as they are. Marked by comparing each label's box before and after it's drawn, so every letter and its dark
// edge count, in any language; a frame that draws none clears it (render()).
#define KNOMI_CELLS 233  // (the canvas's 466 in 2 x 2 cells, as the panel's pixels average them)
static uint32_t gKnomiLift[(KNOMI_CELLS * KNOMI_CELLS + 31) / 32];
static bool gKnomiLiftOn = false;
static uint16_t *gKnomiLiftBuf = nullptr;  // a label's box as it was before the label (PSRAM)
#define KNOMI_LIFT_MAX (160 * 160)
static inline bool knomiLifted(int cx, int cy) {
  uint32_t i = (uint32_t)cy * KNOMI_CELLS + cx;
  return (gKnomiLift[i >> 5] >> (i & 31)) & 1;
}
static void knomiLiftClear() {
  memset(gKnomiLift, 0, sizeof gKnomiLift);
  gKnomiLiftOn = true;
}
static void knomiLiftClip(int &x0, int &y0, int &x1, int &y1) {
  x0 = x0 < 0 ? 0 : x0, y0 = y0 < 0 ? 0 : y0, x1 = x1 > 465 ? 465 : x1, y1 = y1 > 465 ? 465 : y1;
}
// before drawing in the canvas's box x0..x1, y0..y1: keeps what's there (false: no room for it, nothing will lift)
static bool knomiLiftKeep(const uint16_t *fb, int x0, int y0, int x1, int y1) {
  knomiLiftClip(x0, y0, x1, y1);
  int w = x1 - x0 + 1, h = y1 - y0 + 1;
  if (w <= 0 || h <= 0 || w * h > KNOMI_LIFT_MAX) return false;
  if (!gKnomiLiftBuf) gKnomiLiftBuf = (uint16_t *)ps_malloc(KNOMI_LIFT_MAX * sizeof(uint16_t));
  if (!gKnomiLiftBuf) return false;
  for (int y = 0; y < h; y++) memcpy(gKnomiLiftBuf + y * w, fb + (y0 + y) * 466 + x0, w * sizeof(uint16_t));
  return true;
}
// ...and after: the cells where something changed are over the shadow
static void knomiLiftMark(const uint16_t *fb, int x0, int y0, int x1, int y1) {
  knomiLiftClip(x0, y0, x1, y1);
  int w = x1 - x0 + 1, h = y1 - y0 + 1;
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      if (fb[(y0 + y) * 466 + x0 + x] != gKnomiLiftBuf[y * w + x]) {
        uint32_t i = (uint32_t)((y0 + y) >> 1) * KNOMI_CELLS + ((x0 + x) >> 1);
        gKnomiLift[i >> 5] |= 1u << (i & 31);
      }
}

// a panel pixel (RGB565) darkened by k (0..255)
static inline __attribute__((always_inline)) uint16_t knomiRimPixel(uint16_t c, int k) {
  uint32_t r = ((c >> 11) * k) >> 8, g = (((c >> 5) & 63) * k) >> 8, b = ((c & 31) * k) >> 8;
  return (uint16_t)((r << 11) | (g << 5) | b);
}
