// The CRT screen (host_crt.h): the mockup Fedor picked (scratch fx3.py), in the game's pixels so any size matches it.
#include "host_crt.h"
#include "host.h"
#include <math.h>
#include <string.h>
#include <vector>

namespace {

const int C = HOST_CANVAS_SIZE;    // 466
const int Q = (C + 3) / 4;         // the halo's quarter-size grid (117)
const float K = 0.045f;            // the glass's bulge: the middle magnified, the rim squeezed (0.11 at first: Fedor,
                                   // "lessen the distortion but just a tiny bit. On medium size it can be a bit overwhelming";
                                   // 0.09 in 2.1.2, then half: "I just want this lite effect as on")
const float SHRINK = 0.965f;       // the picture stops short of the rim (the tube's black edge)
const float SAT = 1.18f, CON = 1.08f;  // CRT colour: richer, deeper blacks
const float BLOOM = 0.11f;         // the glow on bright pixels (2.5 game pixels wide; 2.1.2's was twice it)
const float HAL_R = 0.10f, HAL_G = 0.075f, HAL_B = 0.05f;  // the halo in the glass: wide and warm (13 game pixels; also half)

// per frame
uint8_t gAdj[C * C * 3];           // the picture in CRT colour
uint8_t gBeam[C * 2 * C * 3];      // ...soft along its lines, on a half-pixel grid across (for the convergence)
uint8_t gBeamL[C * 2 * C];         // its brightness, which sets each line's width
uint8_t gGlowB[C * C * 3], gGlowH[C * C * 3];  // what the glow and the halo add to each canvas pixel (both smooth:
                                               // read at the nearest one)
int gSide = 0;
float gStep = 1, gPhase = 0;       // canvas pixels per screen pixel; the lines' phase (see hostCrtPrepare)
int gSub = 1;
// per side: through the curved glass
std::vector<int16_t> gMapX, gMapY; // the canvas point under each screen pixel, x16 (-1: off the picture)
std::vector<uint8_t> gShade, gGlass;  // the rim's darkening and the tube's black edge; the glass's reflection
float gLut[65][64];                // a line's light: exp(-(d/w)^2) * 1.2 by distance from its middle and brightness
bool gLutMade = false;

inline uint8_t clamp8(float v) { return v <= 0 ? 0 : v >= 255 ? 255 : (uint8_t)(v + 0.5f); }

// Whites and light greys stay apart (Fedor: "Some of the white and light gray get completely washed out on CRT
// mode"). The glow and the halo fade out as a pixel nears white, by its brightness cubed, so they still light the
// dark and middle tones as before. And a colour that runs past 255 is scaled down whole, keeping its tint, instead
// of each channel clipping on its own toward white.
inline float glowFade(const float *v) {
  float l = (0.3f * v[0] + 0.59f * v[1] + 0.11f * v[2]) / 255;
  if (l > 1) l = 1;
  return 1 - l * l * l;
}
inline void fitWhole(float *v) {
  float t = fmaxf(v[0], fmaxf(v[1], v[2]));
  if (t > 255) {
    float k = 255 / t;
    v[0] *= k, v[1] *= k, v[2] *= k;
  }
}

// a Gaussian blur as three box blurs (as Pillow's), edges extended, on a planar w x h float image
void boxesForGauss(float sigma, int sizes[3]) {
  float wIdeal = sqrtf(12 * sigma * sigma / 3 + 1);
  int wl = (int)floorf(wIdeal);
  if (wl % 2 == 0) wl--;
  int wu = wl + 2;
  float mIdeal = (12 * sigma * sigma - 3 * wl * wl - 4 * 3 * wl - 3 * 3) / (-4.0f * wl - 4);
  int m = (int)lroundf(mIdeal);
  for (int i = 0; i < 3; i++) sizes[i] = i < m ? wl : wu;
}
void boxLine(const float *s, float *d, int n, int step, int r) {  // one row or column
  if (r <= 0) {
    for (int i = 0; i < n; i++) d[i * step] = s[i * step];
    return;
  }
  const float k = 1.0f / (2 * r + 1);
  float acc = 0;
  for (int i = -r; i <= r; i++) acc += s[(i < 0 ? 0 : i >= n ? n - 1 : i) * step];
  for (int i = 0; i < n; i++) {
    d[i * step] = acc * k;
    int a = i + r + 1, b = i - r;
    acc += s[(a >= n ? n - 1 : a) * step] - s[(b < 0 ? 0 : b) * step];
  }
}
void gauss(float *buf, int w, int h, float sigma) {
  static std::vector<float> tmp;
  tmp.resize((size_t)w * h);
  int sz[3];
  boxesForGauss(sigma, sz);
  for (int p = 0; p < 3; p++) {
    int r = (sz[p] - 1) / 2;
    for (int y = 0; y < h; y++) boxLine(buf + (size_t)y * w, tmp.data() + (size_t)y * w, w, 1, r);
    for (int x = 0; x < w; x++) boxLine(tmp.data() + x, buf + x, h, w, r);
  }
}

// the glass for a side: where each screen pixel looks into the canvas, the rim, the reflection
void makeSide(int side) {
  gSide = side;
  size_t n = (size_t)side * side;
  gMapX.assign(n, -1);
  gMapY.assign(n, -1);
  gShade.assign(n, 0);
  gGlass.assign(n, 0);
  const float R = side / 2.0f;
  std::vector<float> vig(n), edge(n), refl(n), arc(n);
  const float er = R - 0.012f * side;  // the picture's edge, inside the rim
  const float rx0 = 0.08f * side, ry0 = 0.04f * side, rx1 = 0.70f * side, ry1 = 0.42f * side;  // the reflection
  const float rcx = (rx0 + rx1) / 2, rcy = (ry0 + ry1) / 2, ra = (rx1 - rx0) / 2, rb = (ry1 - ry0) / 2;
  const float arcR = 0.45f * side, arcW = 0.012f * side;  // a thin bright arc along the upper rim
  for (int y = 0; y < side; y++)
    for (int x = 0; x < side; x++) {
      size_t i = (size_t)y * side + x;
      float px = x + 0.5f, py = y + 0.5f, dx = px - R, dy = py - R, r = sqrtf(dx * dx + dy * dy);
      vig[i] = r <= R ? 1.0f : 0.0f;
      edge[i] = r <= er ? 1.0f : 0.0f;
      float ex = (px - rcx) / ra, ey = (py - rcy) / rb;
      refl[i] = ex * ex + ey * ey <= 1 ? 34.0f : 0.0f;
      float ang = atan2f(dy, dx) * 57.29578f;
      if (ang < 0) ang += 360;
      arc[i] = (r <= arcR && r >= arcR - arcW && ang >= 200 && ang <= 285) ? 70.0f : 0.0f;
      // the bulge: the screen pixel at radius r (0..1) shows the canvas at r * (1 - K + K r^2) / SHRINK
      float nx = dx / R, ny = dy / R, f = (1 - K + K * (nx * nx + ny * ny)) / SHRINK;
      float sx = (nx * f + 1) * C / 2, sy = (ny * f + 1) * C / 2;
      if (r <= R && sx >= 0 && sy >= 0 && sx < C && sy < C) {
        gMapX[i] = (int16_t)(sx * 16);
        gMapY[i] = (int16_t)(sy * 16);
      }
    }
  gauss(vig.data(), side, side, side / 30.0f);   // the rim darkens softly...
  gauss(edge.data(), side, side, side / 160.0f); // ...and the picture fades into the tube's black edge
  gauss(refl.data(), side, side, side / 14.0f);
  gauss(arc.data(), side, side, side / 160.0f);
  for (size_t i = 0; i < n; i++) {
    gShade[i] = clamp8(vig[i] * edge[i] * 255);
    gGlass[i] = clamp8(refl[i] + arc[i]);
  }
  gStep = (float)C / side;
  // at under ~1.75 screen pixels a canvas pixel, a line has too few rows to show its beam: each row averages four
  // samples of it, a quarter line on, so one row lands on the beam and the next on the gap (as 240 lines on 480)
  gSub = gStep > 1 / 1.75f ? 4 : 1;
  gPhase = gSub > 1 ? 0.25f : 0;
}

}  // namespace

void hostCrtPrepare(const uint8_t *rgba, int side) {
  if (!gLutMade) {
    for (int di = 0; di <= 64; di++)
      for (int li = 0; li < 64; li++) {
        float d = di * 2.0f / 64, L = (li + 0.5f) / 64, w = 0.85f + 1.35f * L;
        gLut[di][li] = expf(-(d / w) * (d / w)) * 1.2f;
      }
    gLutMade = true;
  }
  if (side != gSide) makeSide(side);
  // CRT colour (as Pillow's Color then Contrast): saturation round each pixel's grey, contrast round the
  // picture's mean grey, whites a touch warm
  static uint8_t sat[C * C * 3];
  double sum = 0;
  for (int i = 0; i < C * C; i++) {
    const uint8_t *p = rgba + i * 4;
    float L = (p[0] * 299 + p[1] * 587 + p[2] * 114) / 1000.0f;
    uint8_t *s = sat + i * 3;
    for (int c = 0; c < 3; c++) s[c] = clamp8(L + (p[c] - L) * SAT);
    sum += (s[0] * 299 + s[1] * 587 + s[2] * 114) / 1000.0;
  }
  float mean = (float)(int)(sum / (C * C) + 0.5);
  for (int i = 0; i < C * C; i++) {
    uint8_t *s = sat + i * 3, *a = gAdj + i * 3;
    for (int c = 0; c < 3; c++) a[c] = clamp8(mean + (s[c] - mean) * CON);
    a[0] = (uint8_t)(a[0] * 1.02f > 255 ? 255 : a[0] * 1.02f);
    a[2] = (uint8_t)(a[2] * 0.96f);
  }
  // the beam, soft along the line: on a half-pixel grid, each point 55% itself and 22.5% each neighbour
  for (int y = 0; y < C; y++)
    for (int x2 = 0; x2 < 2 * C; x2++) {
      int xm = x2 - 1 < 0 ? 0 : x2 - 1, xp = x2 + 1 >= 2 * C ? 2 * C - 1 : x2 + 1;
      const uint8_t *m = gAdj + (y * C + (xm >> 1)) * 3, *c0 = gAdj + (y * C + (x2 >> 1)) * 3, *p = gAdj + (y * C + (xp >> 1)) * 3;
      uint8_t *o = gBeam + ((size_t)y * 2 * C + x2) * 3;
      for (int c = 0; c < 3; c++) o[c] = clamp8(0.55f * c0[c] + 0.225f * (m[c] + p[c]));
      gBeamL[(size_t)y * 2 * C + x2] = clamp8(0.3f * o[0] + 0.59f * o[1] + 0.11f * o[2]);
    }
  // the glow (2.5 game pixels, made on a half-size grid) and the halo (13, on a quarter-size one): both soft, so
  // small grids do, brought back to the canvas's size with a smooth stretch
  const int H = C / 2;  // 233
  static float half[3][H * H], quart[3][Q * Q], rowH[3][H], rowQ[3][Q];
  static int upH0[C], upH1[C], upQ0[C], upQ1[C];
  static float upHw[C], upQw[C];
  static bool upMade = false;
  if (!upMade) {
    for (int i = 0; i < C; i++) {
      float p = (i + 0.5f) * H / C - 0.5f, q = (i + 0.5f) * Q / C - 0.5f;
      if (p < 0) p = 0;
      if (q < 0) q = 0;
      upH0[i] = (int)p, upH1[i] = upH0[i] + 1 < H ? upH0[i] + 1 : upH0[i], upHw[i] = p - upH0[i];
      upQ0[i] = (int)q, upQ1[i] = upQ0[i] + 1 < Q ? upQ0[i] + 1 : upQ0[i], upQw[i] = q - upQ0[i];
    }
    upMade = true;
  }
  for (int y = 0; y < H; y++)
    for (int x = 0; x < H; x++) {
      const uint8_t *a0 = gAdj + ((2 * y) * C + 2 * x) * 3, *a1 = a0 + C * 3;
      for (int c = 0; c < 3; c++) half[c][y * H + x] = (a0[c] + a0[3 + c] + a1[c] + a1[3 + c]) * 0.25f;
    }
  for (int y = 0; y < Q; y++)
    for (int x = 0; x < Q; x++) {
      int y0 = 2 * y, y1 = 2 * y + 1 < H ? 2 * y + 1 : 2 * y, x0 = 2 * x, x1 = 2 * x + 1 < H ? 2 * x + 1 : 2 * x;
      if (y0 > H - 1) y0 = y1 = H - 1;
      if (x0 > H - 1) x0 = x1 = H - 1;
      for (int c = 0; c < 3; c++)
        quart[c][y * Q + x] = (half[c][y0 * H + x0] + half[c][y0 * H + x1] + half[c][y1 * H + x0] + half[c][y1 * H + x1]) * 0.25f;
    }
  for (int c = 0; c < 3; c++) {
    gauss(half[c], H, H, 1.25f);
    gauss(quart[c], Q, Q, 13.0f / 4);
  }
  const float hk[3] = {HAL_R, HAL_G, HAL_B};
  for (int y = 0; y < C; y++) {
    for (int c = 0; c < 3; c++) {
      const float *h0 = half[c] + upH0[y] * H, *h1 = half[c] + upH1[y] * H;
      for (int x = 0; x < H; x++) rowH[c][x] = h0[x] + (h1[x] - h0[x]) * upHw[y];
      const float *q0 = quart[c] + upQ0[y] * Q, *q1 = quart[c] + upQ1[y] * Q;
      for (int x = 0; x < Q; x++) rowQ[c][x] = q0[x] + (q1[x] - q0[x]) * upQw[y];
    }
    uint8_t *gb = gGlowB + y * C * 3, *gh = gGlowH + y * C * 3;
    for (int x = 0; x < C; x++)
      for (int c = 0; c < 3; c++) {
        float b = rowH[c][upH0[x]] + (rowH[c][upH1[x]] - rowH[c][upH0[x]]) * upHw[x];
        float h = rowQ[c][upQ0[x]] + (rowQ[c][upQ1[x]] - rowQ[c][upQ0[x]]) * upQw[x];
        gb[x * 3 + c] = (uint8_t)(int)(b * BLOOM);
        gh[x * 3 + c] = (uint8_t)(int)(h * hk[c]);
      }
  }
}

void hostCrtRows(uint8_t *out, int stride, bool bgra, int y0, int y1, uint32_t frame) {
  const int side = gSide;
  const int B2 = 2 * C;
  for (int y = y0; y < y1 && y < side; y++) {
    uint8_t *o = out + (size_t)y * stride;
    uint32_t seed = (uint32_t)y * 2654435761u ^ frame * 40503u;
    for (int x = 0; x < side; x++, o += 4) {
      size_t i = (size_t)y * side + x;
      float pic[3] = {0, 0, 0};
      if (gMapX[i] >= 0) {
        float sx = gMapX[i] / 16.0f, sy = gMapY[i] / 16.0f;
        int ix2 = (int)(sx * 2), iy = (int)sy;
        if (ix2 > B2 - 1) ix2 = B2 - 1;
        if (iy > C - 1) iy = C - 1;
        const uint8_t *row = gBeam + (size_t)iy * B2 * 3;
        int ir = ix2 > 0 ? ix2 - 1 : 0, ib = ix2 < B2 - 1 ? ix2 + 1 : ix2;  // convergence: red and blue a hair apart
        float cr = row[ir * 3], cg = row[ix2 * 3 + 1], cb = row[ib * 3 + 2];
        int li = gBeamL[(size_t)iy * B2 + ix2] >> 2;
        float f = 0;
        for (int k = 0; k < gSub; k++) {  // the line's light where this pixel sits in it
          float yy = gSub > 1 ? sy + ((k + 0.5f) / gSub - 0.5f) * gStep : sy;
          float t = yy / 2 + gPhase;
          t -= floorf(t);
          int di = (int)(fabsf(t - 0.5f) * 4 * 32 + 0.5f);
          f += gLut[di > 64 ? 64 : di][li];
        }
        f /= gSub;
        int gi = ((int)sy * C + (int)sx) * 3;
        if (gi > (C * C - 1) * 3) gi = (C * C - 1) * 3;
        float line[3] = {cr * f, cg * f, cb * f};
        fitWhole(line);  // whites and light greys stay apart (glowFade)
        float w = glowFade(line);
        for (int c = 0; c < 3; c++) line[c] += gGlowB[gi + c] * w;
        w = glowFade(line);
        for (int c = 0; c < 3; c++) pic[c] = line[c] + gGlowH[gi + c] * w;
        fitWhole(pic);
      }
      float sh = gShade[i] / 255.0f, a = gGlass[i] / 255.0f;
      // grain: about 3 levels either way, new every frame
      seed ^= seed << 13;
      seed ^= seed >> 17;
      seed ^= seed << 5;
      float g = (((seed & 0xFF) + ((seed >> 8) & 0xFF) + ((seed >> 16) & 0xFF)) / 765.0f - 0.5f) * 11.5f;
      static const float glass[3] = {235, 240, 255};
      uint8_t v[3];
      for (int c = 0; c < 3; c++) v[c] = clamp8(pic[c] * sh * (1 - a) + glass[c] * a + g);
      if (bgra) {
        o[0] = v[2];
        o[1] = v[1];
        o[2] = v[0];
      } else {
        o[0] = v[0];
        o[1] = v[1];
        o[2] = v[2];
      }
      o[3] = 255;
    }
  }
}

bool hostCrtPoint(float x, float y, int side, float &cx, float &cy) {
  float R = side / 2.0f, nx = (x - R) / R, ny = (y - R) / R;
  float f = (1 - K + K * (nx * nx + ny * ny)) / SHRINK;
  cx = (nx * f + 1) * C / 2;
  cy = (ny * f + 1) * C / 2;
  return cx >= 0 && cy >= 0 && cx < C && cy < C;
}
