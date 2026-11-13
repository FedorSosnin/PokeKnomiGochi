// The drop shadow under the Mac and Windows apps' round window (Fedor: "lets try adding a drop shadow under the app on
// mac and pc. I think that will add to the effect"): a soft dark disc a little smaller than the ball, a little lower,
// in a window of its own under the app's that clicks pass through. Header only: both apps draw it themselves.
#pragma once
#include <math.h>
#include <stdint.h>

// its look, as parts of the ball's side at rest (frame included)
#define HOST_SHADOW_R 0.47f      // the disc's radius
#define HOST_SHADOW_DROP 0.045f  // how far below the ball's middle
#define HOST_SHADOW_SOFT 0.04f   // how soft its edge (the blur's sigma)
#define HOST_SHADOW_ALPHA 0.42f  // how dark at its darkest

// the shadow window's margin round the ball's square at rest, as a part of its side: room for the drop and the blur
static inline float hostShadowMargin(float side) { return side * (HOST_SHADOW_DROP + 3 * HOST_SHADOW_SOFT) + 2; }

// its alpha, n x n bytes (rows from the top), for a ball side px across in the middle of the square
static inline void hostShadowMask(uint8_t *alpha, int n, float side) {
  const float c = n / 2.0f, cy = c + side * HOST_SHADOW_DROP, r = side * HOST_SHADOW_R;
  const float k = 1.0f / (side * HOST_SHADOW_SOFT * 1.41421356f);
  for (int y = 0; y < n; y++)
    for (int x = 0; x < n; x++) {
      float dx = x + 0.5f - c, dy = y + 0.5f - cy, d = sqrtf(dx * dx + dy * dy);
      float a = HOST_SHADOW_ALPHA * 0.5f * erfcf((d - r) * k);  // a disc, blurred
      alpha[y * n + x] = (uint8_t)(a * 255 + 0.5f);
    }
}
