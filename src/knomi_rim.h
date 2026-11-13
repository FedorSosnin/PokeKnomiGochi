#pragma once
// V2: the Knomi's screen darkens softly into its rim, so the picture looks round and a little bubbly. It's what was
// kept of a CRT look tried on the Knomi (Fedor: "I dont think it works on such a small screen. The effect makes
// things harder to read and see. I do like the edges of the screen having a bit of shadow though. Makes it look
// bubbly."): the computers' CRT screen's rim (host/host_crt.cpp), done to the panel's pixels near the edge as the
// frame goes out (ScaledCanvas::flush()). The middle is left as it is; the corners the round screen hides are black.
#include <stdint.h>
#include <math.h>
#include <esp_heap_caps.h>

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

// a panel pixel (RGB565) darkened by k (0..255)
static inline __attribute__((always_inline)) uint16_t knomiRimPixel(uint16_t c, int k) {
  uint32_t r = ((c >> 11) * k) >> 8, g = (((c >> 5) & 63) * k) >> 8, b = ((c & 31) * k) >> 8;
  return (uint16_t)((r << 11) | (g << 5) | b);
}
