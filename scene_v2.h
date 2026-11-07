#pragma once
// V2, the new look: the home screen's header, need bars, icon band and icons (step 1 of the V2 build).
// PokeKnomiGochi.ino includes this after its drawing helpers, and only with UI_V2 on: it draws with them
// (gfx, setSize, printT, drawMap...) and reads the sketch's state (pet, gNight). Positions are in
// layout_v2.h and colours in palettes_v2.h.
#include "host/glcdfont.h"  // the classic 5x7 font's pixels, for the labels turned along the bars

// s blended over d, a = 0..256
static inline uint16_t v2Blend(uint16_t d, uint16_t s, int a) {
  int dr = d >> 11, dg = (d >> 5) & 63, db = d & 31;
  int sr = s >> 11, sg = (s >> 5) & 63, sb = s & 31;
  return (uint16_t)(((dr + (((sr - dr) * a) >> 8)) << 11) | ((dg + (((sg - dg) * a) >> 8)) << 5) |
                    (db + (((sb - db) * a) >> 8)));
}

// one 2x2 cell (a Knomi pixel), painted (a = 256) or blended
static inline void v2Cell(uint16_t *fb, int x, int y, uint16_t col, int a) {
  if (x < 0 || y < 0 || x > 464 || y > 464) return;
  uint16_t *p = fb + y * 466 + x;
  if (a >= 256) { p[0] = p[1] = p[466] = p[467] = col; return; }
  p[0] = v2Blend(p[0], col, a); p[1] = v2Blend(p[1], col, a);
  p[466] = v2Blend(p[466], col, a); p[467] = v2Blend(p[467], col, a);
}

// A ring segment round the screen's centre: from angle a0 to a1 (degrees, 0 = right, clockwise, less than
// half a turn), r_in..r_out, with round ends centred on its middle radius at a0 and a1.
struct V2Seg {
  float c0, s0, c1, s1;      // the two end directions
  float e0x, e0y, e1x, e1y;  // the round ends' centres, relative to the screen's centre
  float ri2, ro2, h2;
  bool on;                   // false: empty (a1 <= a0)
};
static V2Seg v2Seg(float rIn, float rOut, float a0, float a1) {
  V2Seg g;
  const float D = 0.01745329f;
  g.on = a1 > a0;
  g.c0 = cosf(a0 * D); g.s0 = sinf(a0 * D); g.c1 = cosf(a1 * D); g.s1 = sinf(a1 * D);
  float rm = (rIn + rOut) / 2, h = (rOut - rIn) / 2;
  g.e0x = rm * g.c0; g.e0y = rm * g.s0; g.e1x = rm * g.c1; g.e1y = rm * g.s1;
  g.ri2 = rIn * rIn; g.ro2 = rOut * rOut; g.h2 = h * h;
  return g;
}
// (fx, fy) relative to the screen's centre, r2 = fx^2 + fy^2
static inline bool v2In(const V2Seg &g, float fx, float fy, float r2) {
  if (!g.on) return false;
  if (r2 >= g.ri2 && r2 <= g.ro2 && g.c0 * fy - g.s0 * fx >= 0 && fx * g.s1 - fy * g.c1 >= 0) return true;
  float dx = fx - g.e0x, dy = fy - g.e0y;
  if (dx * dx + dy * dy <= g.h2) return true;
  dx = fx - g.e1x; dy = fy - g.e1y;
  return dx * dx + dy * dy <= g.h2;
}
// the 2 px grid box that holds a segment drawn out to rOut
static void v2SegBox(float rOut, float a0, float a1, float h, int &x0, int &y0, int &x1, int &y1) {
  const float D = 0.01745329f;
  float rm = rOut - h;
  float xa = rm * cosf(a0 * D), ya = rm * sinf(a0 * D), xb = rm * cosf(a1 * D), yb = rm * sinf(a1 * D);
  float xmin = fminf(xa, xb) - h, xmax = fmaxf(xa, xb) + h, ymin = fminf(ya, yb) - h, ymax = fmaxf(ya, yb) + h;
  for (int q = 0; q <= 720; q += 90)  // a right, down, left or up the arc passes through
    if (q > a0 && q < a1) {
      float cx = rOut * cosf(q * D), cy = rOut * sinf(q * D);
      xmin = fminf(xmin, cx); xmax = fmaxf(xmax, cx); ymin = fminf(ymin, cy); ymax = fmaxf(ymax, cy);
    }
  x0 = ((int)(CX + xmin) - 2) & ~1; y0 = ((int)(CY + ymin) - 2) & ~1;
  x1 = (int)(CX + xmax) + 2; y1 = (int)(CY + ymax) + 2;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > 464) x1 = 464;
  if (y1 > 464) y1 = 464;
}

// Over dark scenes (night, asleep; later the Cave and the Mansion) the dark see-through tint
// disappears, so the track and the band go white and fainter there.
static bool v2DarkScene() { return gNight; }
static uint16_t v2TrackCol() { return v2DarkScene() ? UI_WHITE : V2_TRACK; }
static int v2RimA() { return v2DarkScene() ? 15 : 46; }    // 18% (6% white)
static int v2TrackA() { return v2DarkScene() ? 20 : 56; }  // 22% (8% white)

// ---------- the need bars ----------

// one character of the classic font at size s, its middle at (px, py), turned so its baseline runs along
// the unit vector (tx, ty) (its top to the left of it). Each canvas pixel takes the font pixel under its
// centre: on the 2 px grid a letter at 45 degrees falls apart, while this way the Knomi's 2x2 averaging
// smooths the turned edges.
static void v2GlyphTurned(uint16_t *fb, uint8_t ch, int s, float px, float py, float tx, float ty, uint16_t col) {
  const int R = s * 5 + 2;
  for (int y = (int)py - R; y <= (int)py + R; y++) {
    if (y < 0 || y > 465) continue;
    for (int x = (int)px - R; x <= (int)px + R; x++) {
      if (x < 0 || x > 465) continue;
      float dx = x + 0.5f - px, dy = y + 0.5f - py;
      float u = (dx * tx + dy * ty) / s + 2.5f, v = (dy * tx - dx * ty) / s + 3.5f;  // back into the font
      if (u < 0 || v < 0 || u >= 5 || v >= 8) continue;
      if (GLCD_FONT[ch * 5 + (int)u] >> (int)v & 1) fb[y * 466 + x] = col;
    }
  }
}

// the bytes of the UTF-8 character at s
static int v2Utf8Len(const char *s) {
  uint8_t c = (uint8_t)*s;
  return c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
}

// A bar's label, centred on it along its middle radius. Latin letters turn with the bar, their tops to the
// rim (bottom to top on the left, top to bottom on the right). Japanese and Korean stand upright, one
// under the other, as they're written in columns anyway.
static void v2BarLabel(uint16_t *fb, const char *lab, float midDeg, uint16_t col) {
  const float D = 0.01745329f, rm = (V2_BAR_RIN + V2_BAR_ROUT) / 2.0f;
  if (gCjkFont) {
    int n = 0;
    for (const char *p = lab; *p; p += v2Utf8Len(p)) n++;
    const float adv = 18;  // a kana or hangul at size 2 is 16 px
    float phi = midDeg * D - adv * n / rm / 2;
    gfx->setTextColor(col);
    setSize(2);
    for (const char *p = lab; *p;) {
      int len = v2Utf8Len(p);
      char one[5] = {};
      memcpy(one, p, len);
      p += len;
      float a = phi + adv / 2 / rm;
      phi += adv / rm;
      int cx = (int)(CX + rm * cosf(a)), cy = (int)(CY + rm * sinf(a));
      setCur(cx - textW(one, 2) / 2, cy - 8);
      printT(one);
    }
    return;
  }
  const int s = 2, adv = 6 * s;
  int n = strlen(lab);
  float phi = midDeg * D - (float)adv * n / rm / 2;
  for (int i = 0; i < n; i++) {
    float a = phi + (i + 0.5f) * adv / rm;
    v2GlyphTurned(fb, (uint8_t)lab[i], s, CX + rm * cosf(a), CY + rm * sinf(a), -sinf(a), cosf(a), col);
  }
}

// One need bar on the rim: a soft rim, the see-through track, and the fill in the status colour from the
// bar's lower end (a0 on the left, a1 on the right). All three in one pass over the bar's box.
static void v2Bar(uint16_t *fb, const V2Bar &b, uint8_t val, const char *lab, bool dim) {
  float a0 = b.a0, a1 = b.a1, span = (a1 - a0) * val / 100;
  float f0 = b.leftSide ? a0 : a1 - span, f1 = b.leftSide ? a0 + span : a1;
  uint16_t fill = val >= 50 ? V2_BAR_OK : val >= 25 ? V2_BAR_WARN : V2_BAR_BAD;
  if (dim) fill = v2Blend(fill, C565(0x1e, 0x26, 0x46), 102);  // asleep: 40% toward the night
  V2Seg rim = v2Seg(V2_BAR_RIN - V2_BAR_RIM, V2_BAR_ROUT + V2_BAR_RIM, a0, a1);
  V2Seg track = v2Seg(V2_BAR_RIN, V2_BAR_ROUT, a0, a1);
  V2Seg full = v2Seg(V2_BAR_RIN, V2_BAR_ROUT, f0, f1);
  uint16_t tcol = v2TrackCol();
  int ra = v2RimA(), ta = v2TrackA();
  int x0, y0, x1, y1;
  v2SegBox(V2_BAR_ROUT + V2_BAR_RIM, a0, a1, (V2_BAR_ROUT - V2_BAR_RIN) / 2.0f + V2_BAR_RIM, x0, y0, x1, y1);
  for (int y = y0; y <= y1; y += 2) {
    float fy = y + 1 - CY;
    for (int x = x0; x <= x1; x += 2) {
      float fx = x + 1 - CX, r2 = fx * fx + fy * fy;
      if (!v2In(rim, fx, fy, r2)) continue;
      if (v2In(full, fx, fy, r2)) { v2Cell(fb, x, y, fill, 256); continue; }
      v2Cell(fb, x, y, tcol, ra);
      if (v2In(track, fx, fy, r2)) v2Cell(fb, x, y, tcol, ta);
    }
  }
  v2BarLabel(fb, lab, (a0 + a1) / 2, UI_WHITE);
}

static void v2DrawBars() {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  bool dim = pet.sleeping;
  v2Bar(fb, V2_BARS[0], pet.fullness, T(S_BAR_FOOD), dim);
  v2Bar(fb, V2_BARS[1], pet.joy, T(S_BAR_JOY), dim);
  v2Bar(fb, V2_BARS[2], pet.energy, T(S_BAR_ENE), dim);
  v2Bar(fb, V2_BARS[3], pet.hygiene, T(S_BAR_HYG), dim);
}

// ---------- the icons ----------

// drawMap in one flat colour (an icon in shadow)
static void v2MapFlat(const char *const *map, int n, int x, int y, int s, uint16_t col) {
  x = SNAP2(x);
  y = SNAP2(y);
  for (int r = 0; r < n; r++) {
    const char *row = map[r];
    forRuns(n, [&](int c) { return charKey(row[c], true); },
            [&](int c0, int len, int) { gfx->fillRect(x + c0 * s, y + r * s, len * s, s, col); });
  }
}

// The band under the five icons: one curved see-through strip, its round ends on the bag and the drop.
// It takes the place of the shadows under them.
static void v2DrawBand() {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  V2Seg rim = v2Seg(V2_BAND_RIN - 3, V2_BAND_ROUT + 3, V2_BAND_A0, V2_BAND_A1);
  V2Seg band = v2Seg(V2_BAND_RIN, V2_BAND_ROUT, V2_BAND_A0, V2_BAND_A1);
  uint16_t col = v2TrackCol();
  int ra = v2RimA(), ta = v2TrackA();
  int x0, y0, x1, y1;
  v2SegBox(V2_BAND_ROUT + 3, V2_BAND_A0, V2_BAND_A1, (V2_BAND_ROUT - V2_BAND_RIN) / 2.0f + 3, x0, y0, x1, y1);
  for (int y = y0; y <= y1; y += 2) {
    float fy = y + 1 - CY;
    for (int x = x0; x <= x1; x += 2) {
      float fx = x + 1 - CX, r2 = fx * fx + fy * fy;
      if (!v2In(rim, fx, fy, r2)) continue;
      v2Cell(fb, x, y, col, ra);
      if (v2In(band, fx, fy, r2)) v2Cell(fb, x, y, col, ta);
    }
  }
}

// The icons on their circle. Asleep, all but the moon are slate shadows: ink would vanish on the band.
static void v2DrawIcons() {
  for (int i = 0; i < BTN_COUNT; i++) {
    int x = buttons[i].cx - BTN_HALF, y = buttons[i].cy - BTN_HALF;
    if (pet.sleeping && i != BTN_LIGHT) v2MapFlat(buttons[i].icon, HOME_ICON_N, x, y, 3, C565(0x3a, 0x4a, 0x5a));
    else drawMap(buttons[i].icon, HOME_ICON_N, x, y, 3, false);
  }
}

// ---------- the header ----------

// The clock on top, then the level, the name (big and bold) and the status. No battery (the Knomi has none)
// and no streak flame (it's on the card).
static void v2Header(const char *name, uint16_t nameColor, const char *msg, const char *level) {
  static uint32_t clockAt = 0;
  static char clk[12] = "";
  uint32_t now = millis();
  if (!clockAt || now - clockAt >= 1000) {  // once a second: on the Waveshare the clock is read over I2C
    clockAt = now ? now : 1;
    uint32_t e = rtcEpoch();
    int h = (e / 3600) % 24;
    if (e) snprintf(clk, sizeof(clk), "%d:%02d %s", clock12Hour(h), (int)((e / 60) % 60), h < 12 ? "AM" : "PM");
    else clk[0] = 0;  // no time yet: no clock
  }
  gfx->setTextColor(inkColor());
  if (clk[0]) {
    bool cjk = classicFontBegin();  // digits and AM/PM in the classic font, as on the clock screen
    setSize(2);
    setCur(centerX(clk, 2), V2_CLOCK_Y);
    printT(clk);
    classicFontEnd(cjk);
  }
  if (level) {
    setSize(2);
    setCur(centerX(level, 2), V2_LEVEL_Y);
    printT(level);
  }
  gfx->setTextColor(nameColor);
  int ns = 3;
  if (gCjkFont) {  // as V1: Japanese and Korean draw sizes 2 and 3 alike, so the name goes up a size
    setSize(4);
    if (textW(name, 4) <= 260) ns = 4;
  }
  setSize(ns);
  int nx = centerX(name, ns), ny = ns == 4 ? V2_NAME_Y - 4 : V2_NAME_Y;
  setCur(nx, ny);
  printT(name);
  if (!gCjkFont) {  // twice, a Knomi pixel apart: bold (setCur snaps to even x, so nx + 1 could land on nx)
    setCur(SNAP2(nx) + 2, ny);
    printT(name);
  }
  gfx->setTextColor(inkColor());
  setSize(2);
  setCur(centerX(msg, 2), V2_STATUS_Y);
  printT(msg);
}
