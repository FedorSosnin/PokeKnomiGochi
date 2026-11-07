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

// The bars' empty track is the dark see-through tint everywhere. The band under the icons is too, except
// over dark scenes (night, asleep, the Cave, the Mansion), where it would vanish: there it's white and
// fainter, as in the night mockups.
static bool v2DarkScene() { return gNight || gV2Dark; }
#define V2_RIM_A 46    // 18%
#define V2_TRACK_A 56  // 22%

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
  const uint16_t tcol = V2_TRACK;
  const int ra = V2_RIM_A, ta = V2_TRACK_A;
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
  bool dark = v2DarkScene();
  uint16_t col = dark ? UI_WHITE : V2_TRACK;
  int ra = dark ? 26 : V2_RIM_A, ta = dark ? 31 : V2_TRACK_A;  // white at 10% and 12%
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

// ---------- the text box and the food picker ----------

#define V2_RIM C565(0x3a, 0x4a, 0x5a)   // panels' and buttons' dark rim
#define V2_LINE C565(0xa8, 0xd8, 0xe8)  // the soft line inside a panel
#define V2_SEAT C565(0xe4, 0xea, 0xf2)  // a round seat for something to pick

// The V2 panel, a Pokemon-style text box: white, a 3 px dark rim, a soft 2 px line inside. x1 and y1 are
// inside it, as in the mockups.
static void v2Panel(int x0, int y0, int x1, int y1, int r) {
  int w = x1 - x0 + 1, h = y1 - y0 + 1;
  gfx->fillRoundRect(x0 - 3, y0 - 3, w + 6, h + 6, r + 2, V2_RIM);
  gfx->fillRoundRect(x0, y0, w, h, r, UI_WHITE);
  int ri = r - 3 < 2 ? 2 : r - 3;
  gfx->drawRoundRect(x0 + 3, y0 + 3, w - 6, h - 6, ri, V2_LINE);
  gfx->drawRoundRect(x0 + 4, y0 + 4, w - 8, h - 8, ri > 1 ? ri - 1 : 1, V2_LINE);
}

// a count on a red disc with a dark rim (the candies left today)
static void v2CountBadge(int cx, int cy, int n) {
  char b[8];
  snprintf(b, sizeof(b), "%d", n);
  gfx->fillCircle(cx, cy, 12, V2_RIM);
  gfx->fillCircle(cx, cy, 10, UI_BAR_BAD);
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(cx - textW(b, 2) / 2 + 1, cy - 7);
  printT(b);
}

// The food picker: a text-box bubble out of the bag, its tail pointing at it, each food on a round seat
// (the candy, while there are any left today, with its count)
static void v2DrawFoodPicker(int count) {
  static const char *const *const FOOD[4] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_CANDY };
  int x0 = V2_FEED_X0, y0 = V2_FEED_Y0, x1 = x0 + count * V2_FEED_CELL, y1 = y0 + V2_FEED_H;
  gfx->fillTriangle(118, y1, 146, y1, 124, y1 + 16, V2_RIM);  // the tail's rim, under the box's
  v2Panel(x0, y0, x1, y1, 14);
  gfx->fillTriangle(121, y1 - 3, 143, y1 - 3, 125, y1 + 10, UI_WHITE);  // its inside, over the rim
  for (int i = 0; i < count; i++) {
    int cx = x0 + V2_FEED_CELL / 2 + V2_FEED_CELL * i, cy = y0 + 36;
    gfx->fillCircle(cx, cy, V2_FEED_SEAT_R, V2_SEAT);
    drawMap(FOOD[i], HOME_ICON_N, cx - 27, cy - 27, 3, false);
  }
  if (count == 4) v2CountBadge(x0 + V2_FEED_CELL / 2 + V2_FEED_CELL * 3 + 22, y0 + 8, pet.candiesLeft());
}

// ---------- the scenes: the home's backdrop, in the Pokemon's Travel zone ----------
// All on a 4 px grid (one 8-bit pixel = 2 Knomi pixels), drawn the way the V2 mockups' scripts draw them
// (pixelhills.py, zones_px.py, mansion2.py; dusk and night as batch3.py's render_tod). Colours are
// 0xRRGGBB and mixed in 8-bit RGB, as there. Nothing in a scene moves yet, so it is drawn once into a
// buffer and copied in every frame; it's drawn again only when the zone or the time of day changes.

typedef uint32_t Rgb;
#define V2_W 466
#define Z_MEADOW 5  // after the five zones: the egg's
static inline uint16_t sc565(Rgb c) { return ((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x1F); }
static inline Rgb scRgb(int r, int g, int b) {
  r = r < 0 ? 0 : r > 255 ? 255 : r;
  g = g < 0 ? 0 : g > 255 ? 255 : g;
  b = b < 0 ? 0 : b > 255 ? 255 : b;
  return (Rgb)r << 16 | (Rgb)g << 8 | (Rgb)b;
}
static inline Rgb scMix(Rgb a, Rgb b, double t) {  // the mockups' lerp()
  int ar = a >> 16, ag = (a >> 8) & 255, ab = a & 255, br = b >> 16, bg = (b >> 8) & 255, bb = b & 255;
  return scRgb((int)(ar + (br - ar) * t), (int)(ag + (bg - ag) * t), (int)(ab + (bb - ab) * t));
}

static uint16_t *scFb;      // where the scene is drawn
static uint8_t scGradeTod;  // 0: colours as given; 1, 2: a day scene's land at dusk or at night
static Rgb scGrade(Rgb c) {
  if (!scGradeTod) return c;
  int r = c >> 16, g = (c >> 8) & 255, b = c & 255;
  if (scGradeTod == 2) return scRgb((int)(r * .30 + 8), (int)(g * .36 + 12), (int)(b * .55 + 30));  // about a third of the light, toward blue
  return scMix(scRgb((int)(r * .94), (int)(g * .90), (int)(b * .84)), 0xFFAA6E, .16);                 // a warm wash
}
// a rectangle with both corners inside it, as PIL draws them
static void scRect(int x0, int y0, int x1, int y1, Rgb c) {
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > V2_W - 1) x1 = V2_W - 1;
  if (y1 > V2_W - 1) y1 = V2_W - 1;
  if (x0 > x1 || y0 > y1) return;
  uint16_t v = sc565(scGrade(c));
  for (int y = y0; y <= y1; y++) {
    uint16_t *p = scFb + y * V2_W + x0;
    for (int x = x0; x <= x1; x++) *p++ = v;
  }
}
static inline void scCell(int x, int y, Rgb c) { scRect(x, y, x + 3, y + 3, c); }

struct ScKey { char ch; Rgb c; };
static void scSprite(const char *const *rows, int nr, int x, int y, int s, const ScKey *keys, int nk) {
  for (int j = 0; j < nr; j++)
    for (int i = 0; rows[j][i]; i++)
      for (int k = 0; k < nk; k++)
        if (keys[k].ch == rows[j][i]) {
          scRect(x + i * s, y + j * s, x + i * s + s - 1, y + j * s + s - 1, keys[k].c);
          break;
        }
}

// a scene's colours (the mockups' PX entries)
struct ScPal { Rgb top, bot, far, mid, near, dark, trunk, cloud, cloudSh, sun, halo; double lit; Rgb fl[3]; };
static const ScPal SC_DAY = { 0x7FBDE6, 0xD8EEE8, 0xA7D7B0, 0x86C58E, 0x6DB673, 0x3F7E52, 0x7E5530,
                              0xFFFFFF, 0xCFE3EF, 0xFFE48A, 0xFFF3C8, .28, { 0xFFFFFF, 0xFFD95E, 0xF08AA4 } };
static const ScPal SC_NIGHT = { 0x0C1224, 0x233058, 0x25364C, 0x1E2E40, 0x1A2A36, 0x101C26, 0x1A1A22,
                                0x2E3858, 0x252E4A, 0xE8EEF5, 0x2A3456, .10, { 0x5A6A8A, 0x6A6A5A, 0x6A5A72 } };
static const ScPal SC_DUSK_SKY = { 0xC75A4A, 0xF6B46A, 0, 0, 0, 0, 0, 0xFFD9B8, 0xE89A7C, 0xFFF1C8, 0xFFD29A, .22, { 0, 0, 0 } };

// the role colours of a tree, pine, palm or rock standing on `base`
static void scTreeKeys(ScKey *k, const ScPal &p, Rgb base, Rgb dark) {
  k[0] = { '1', dark };
  k[1] = { '2', scMix(base, 0xFFFFFF, p.lit + .1) };
  k[2] = { '3', scMix(base, dark, .25) };
  k[3] = { '4', scMix(base, dark, .6) };
  k[4] = { '5', p.trunk };
}
static void scRole(const char *const *rows, int nr, int x, int y, int s, const ScPal &p, Rgb base) {
  ScKey k[5];
  scTreeKeys(k, p, base, p.dark);
  scSprite(rows, nr, x, y, s, k, 5);
}

// ----- sky, sun and moon, clouds -----
static void scSky(Rgb top, Rgb bot) {
  Rgb c[3] = { top, scMix(top, bot, .5), bot };
  const int edge[4] = { 0, 124, 196, 300 };
  for (int i = 0; i < 3; i++) scRect(0, edge[i], V2_W, edge[i + 1], c[i]);
  for (int i = 1; i <= 2; i++)  // two rows of checker each side of where the bands meet
    for (int y = edge[i] - 8; y < edge[i] + 8; y += 4)
      for (int x = 0; x < V2_W; x += 4)
        if ((x / 4 + y / 4) % 2 == 0) scCell(x, y, y < edge[i] ? c[i] : c[i - 1]);
}
static void scDisc(int x, int y, int r, Rgb col, int ox, int oy) {
  for (int cy = y - r - 4; cy < y + r + 4; cy += 4)
    for (int cx = x - r - 4; cx < x + r + 4; cx += 4) {
      int dx = cx + 2 - x - ox, dy = cy + 2 - y - oy;
      if (dx * dx + dy * dy <= r * r) scCell(cx / 4 * 4, cy / 4 * 4, col);
    }
}
static const int16_t SC_STARS[10][2] = { {70, 130}, {118, 96}, {160, 158}, {342, 74}, {334, 196},
                                         {404, 120}, {96, 200}, {268, 152}, {392, 176}, {196, 140} };  // clear of the name
// the sun in its halo, fading out in a checker; at night a crescent moon and square stars
static void scSun(const ScPal &p, bool night, int x, int y) {
  Rgb mid = scMix(p.top, p.bot, .5);
  if (night) {
    for (auto &s : SC_STARS) scCell(s[0] / 4 * 4, s[1] / 4 * 4, 0xFFFFFF);
    scDisc(x, y, 22, p.sun, 0, 0);
    scDisc(x, y, 18, mid, 10, -8);
    return;
  }
  scDisc(x, y, 34, p.halo, 0, 0);
  for (int cy = y - 34; cy < y + 34; cy += 4)
    for (int cx = x - 34; cx < x + 34; cx += 4) {
      int r2 = (cx + 2 - x) * (cx + 2 - x) + (cy + 2 - y) * (cy + 2 - y);
      if (r2 > 28 * 28 && r2 <= 34 * 34 && (cx / 4 + cy / 4) % 2) scCell(cx / 4 * 4, cy / 4 * 4, mid);
    }
  scDisc(x, y, 24, p.sun, 0, 0);
}
static void scCloud(const ScPal &p, int x, int y, int s) {
  const ScKey k[2] = { { 'x', p.cloud }, { 's', p.cloudSh } };
  scSprite(SPR_V2_CLOUD, 5, x, y, s, k, 2);
}

// ----- hills -----
struct ScEll { int16_t x0, y0, x1, y1; };
struct ScLayer { Rgb col; const ScEll *e; uint8_t n; };
static const ScEll SC_FAR[3] = { { -120, 214, 230, 500 }, { 200, 200, 600, 520 }, { 20, 226, 446, 330 } };  // [2]: the Mansion's rise
static const ScEll SC_MID[2] = { { -200, 246, 300, 600 }, { 240, 256, 700, 620 } };
static const ScEll SC_NEAR[1] = { { -400, 276, 866, 1100 } };
static const ScEll SC_DUNES[2] = { { -200, 262, 300, 600 }, { 240, 270, 700, 620 } };
static const ScEll SC_SHORE[1] = { { -400, 284, 866, 1100 } };
static bool scInEll(float x, float y, const ScEll &e) {
  float rx = (e.x1 - e.x0) / 2.0f, ry = (e.y1 - e.y0) / 2.0f;
  float a = (x - (e.x0 + rx)) / rx, b = (y - (e.y0 + ry)) / ry;
  return a * a + b * b <= 1;
}
static int scLay(const ScLayer *L, int nl, float x, float y) {
  for (int i = nl - 1; i >= 0; i--)
    for (int k = 0; k < L[i].n; k++)
      if (scInEll(x, y, L[i].e[k])) return i + 1;
  return 0;
}
// Stepped hills, each with a lit top row and a checker fringe under it. from = 1 draws them all; 2 draws
// the nearer ones again, over the trees and houses that stand on the far ones.
static void scHills(const ScLayer *L, int nl, double lit, int from) {
  Rgb litc[3], half[3];
  for (int i = 0; i < nl; i++) {
    litc[i] = scMix(L[i].col, 0xFFFFFF, lit);
    half[i] = scMix(L[i].col, 0xFFFFFF, lit / 2);
  }
  for (int cy = 192; cy < V2_W; cy += 4)  // the hills all start below y 200
    for (int cx = 0; cx < V2_W; cx += 4) {
      int l = scLay(L, nl, cx + 2, cy + 2);
      if (l < from) continue;
      Rgb c = L[l - 1].col;
      if (scLay(L, nl, cx + 2, cy + 2 - 4) != l) c = litc[l - 1];
      else if (scLay(L, nl, cx + 2, cy + 2 - 8) != l && (cx / 4 + cy / 4) % 2 == 0) c = half[l - 1];
      scCell(cx, cy, c);
    }
}

// ----- details -----
static void scGrass(const ScPal &p, const int16_t (*pts)[2], int n) {
  const ScKey k[2] = { { 'G', p.dark }, { 'g', scMix(p.near, p.dark, .35) } };
  for (int i = 0; i < n; i++) scSprite(SPR_V2_TALLGRASS, 5, pts[i][0], pts[i][1], 3, k, 2);
}
static void scFlowers(const int16_t (*pts)[2], int n, const Rgb *cols, int nc) {
  for (int i = 0; i < n; i++) scCell(pts[i][0] / 4 * 4, pts[i][1] / 4 * 4, cols[i % nc]);
}
static void scFence(int y, int x0, int x1, Rgb post, Rgb rail, int step) {
  for (int x = x0; x < x1; x += step) {
    scRect(x, y - 16, x + 3, y + 3, post);
    scRect(x, y - 16, x + 3, y - 13, scMix(post, 0xFFFFFF, .3));
  }
  scRect(x0, y - 12, x1, y - 9, rail);
  scRect(x0, y - 4, x1, y - 1, rail);
}
static void scHouse(int x, int y, bool lit) {
  const ScKey k[8] = { { 'k', 0x1B1B25 }, { 'r', 0xE8503A }, { 'R', 0xB53224 }, { 'w', 0xFFFFFF },
                       { 'b', 0x7CC4EA }, { 'B', 0x4F93C4 }, { 'C', 0x7E5530 }, { 'y', 0xF8D990 } };
  scSprite(SPR_V2_HOUSE, 12, x, y, 3, k, 8);
  if (!lit) return;
  uint8_t g = scGradeTod;  // at night its windows light up, warm and ungraded
  scGradeTod = 0;
  const ScKey w[2] = { { 'b', 0xF0C04A }, { 'B', 0xC08410 } };
  scSprite(SPR_V2_HOUSE, 12, x, y, 3, w, 2);
  scGradeTod = g;
}

// ----- the zones -----
static ScPal scFieldPal() {
  ScPal p = SC_DAY;
  p.top = 0xA8DCF0; p.bot = 0xE2F2EA; p.far = 0xA6DC90; p.mid = 0x8FCE7A; p.near = 0x79BD4F; p.dark = 0x4F8A32;
  return p;
}
static ScPal scForestPal() {
  ScPal p = SC_DAY;
  p.top = 0x9FD6F0; p.bot = 0xDCEFE4; p.far = 0x5FA36C; p.mid = 0x3F8A4C; p.near = 0x4F9A55; p.dark = 0x1F4A2A; p.lit = .18;
  return p;
}
static ScPal scBeachPal() {
  ScPal p = SC_DAY;
  p.near = 0xE2CF98; p.mid = 0xECDCAC; p.dark = 0xB89A5E; p.trunk = 0x8A5A30;
  return p;
}
static ScPal scCavePal() {
  ScPal p = SC_DAY;
  p.top = 0x201C2C; p.bot = 0x2F2A3F; p.far = 0x3A3450; p.mid = 0x494258; p.near = 0x544B63; p.dark = 0x1A1624; p.lit = .12;
  return p;
}
static ScPal scMansionPal() {
  ScPal p = SC_NIGHT;
  p.top = 0x140F2C; p.bot = 0x2A2350; p.far = 0x241E40; p.mid = 0x1E2034; p.near = 0x25283A; p.dark = 0x0D0B20; p.lit = .12;
  p.cloud = 0x3A3260; p.cloudSh = 0x2C2650;
  return p;
}

// the three hill layers every zone shares (the Mansion adds its rise to the far ones)
static void scStdHills(const ScPal &p, int from, bool rise = false) {
  ScLayer L[3] = { { p.far, SC_FAR, (uint8_t)(rise ? 3 : 2) }, { p.mid, SC_MID, 2 }, { p.near, SC_NEAR, 1 } };
  scHills(L, 3, p.lit, from);
}

static const int16_t SC_GRASS6[6][2] = { {58, 316}, {112, 300}, {330, 300}, {380, 318}, {40, 344}, {414, 342} };
static const int16_t SC_FLOWERS5[5][2] = { {168, 324}, {296, 330}, {150, 352}, {318, 356}, {232, 346} };

static void scLandMeadow(const ScPal &p) {
  static const int16_t trees[5][2] = { {64, 196}, {100, 206}, {326, 194}, {362, 186}, {398, 200} };
  scStdHills(p, 1);
  for (auto &t : trees) scRole(SPR_V2_TREE, 10, t[0], t[1], 3, p, p.mid);
  scStdHills(p, 2);
  scGrass(p, SC_GRASS6, 6);
  scFlowers(SC_FLOWERS5, 5, p.fl, 3);
}

static void scLandField(const ScPal &p, bool night) {
  static const int16_t trees[3][2] = { {326, 194}, {362, 186}, {398, 200} };
  const ScKey bird[1] = { { 'k', 0x18181F } };
  scSprite(SPR_V2_BIRD_UP, 3, 196, 150, 3, bird, 1);
  scSprite(SPR_V2_BIRD_UP, 3, 226, 140, 3, bird, 1);
  scStdHills(p, 1);
  scHouse(50, 186, night);
  for (auto &t : trees) scRole(SPR_V2_TREE, 10, t[0], t[1], 3, p, p.mid);
  scStdHills(p, 2);
  scFence(262, 0, 160, 0x9A6F43, 0xB38A58, 24);  // the Field tile's wooden fence
  scFence(266, 312, 466, 0x9A6F43, 0xB38A58, 24);
  scGrass(p, SC_GRASS6, 6);
  scFlowers(SC_FLOWERS5, 5, p.fl, 3);
}

static void scLandForest(const ScPal &p) {
  static const int16_t big[4][2] = { {14, 246}, {52, 258}, {384, 254}, {418, 244} };
  static const int16_t grass[4][2] = { {112, 300}, {330, 300}, {40, 344}, {414, 342} };
  static const int16_t flowers[3][2] = { {150, 330}, {300, 336}, {180, 352} };
  const Rgb pink = 0xE9587A;
  scStdHills(p, 1);
  for (int tx = 0; tx < V2_W; tx += 26) {  // a wall of pines on the far hills, open behind the pet
    if (tx > 150 && tx < 300) continue;
    scRole(SPR_V2_PINE, 12, tx - 10, 178 + (tx * 37 % 18), 3, p, p.far);
  }
  scStdHills(p, 2);
  for (auto &t : big) scRole(SPR_V2_PINE, 12, t[0], t[1], 4, p, p.mid);
  scGrass(p, grass, 4);
  scFlowers(flowers, 3, &pink, 1);
}

static void scLandBeach(const ScPal &p) {
  static const int16_t glint[25][2] = {  // where the mockup's waves glint (its random.Random(3))
    {120, 236}, {300, 236}, {276, 236}, {64, 236}, {188, 236}, {308, 248}, {240, 248}, {320, 248}, {296, 248},
    {32, 248}, {308, 260}, {4, 260}, {464, 260}, {428, 260}, {240, 260}, {132, 272}, {280, 272}, {116, 272},
    {96, 272}, {364, 272}, {240, 284}, {276, 284}, {428, 284}, {280, 284}, {240, 284} };
  const Rgb sea = 0x4F96C4, seaHi = 0xBFE6F5;
  scRect(0, 224, V2_W, 300, sea);
  scRect(0, 224, V2_W, 227, scMix(sea, 0xFFFFFF, .35));  // the horizon
  for (auto &g : glint) scRect(g[0], g[1], g[0] + 11, g[1] + 3, g[1] < 260 ? seaHi : scMix(seaHi, sea, .4));
  ScLayer L[2] = { { p.mid, SC_DUNES, 2 }, { p.near, SC_SHORE, 1 } };
  scHills(L, 2, p.lit, 1);
  ScKey palm[5];  // palms on the dunes, a size up so they read at the Knomi's size
  scTreeKeys(palm, p, 0x58B868, 0x2F6A3A);
  scSprite(SPR_V2_PALM, 12, 34, 224, 4, palm, 5);
  scSprite(SPR_V2_PALM, 12, 368, 236, 4, palm, 5);
  scHills(L, 2, p.lit, 2);
  const ScKey shell[3] = { { 'p', 0xF08AA4 }, { 'w', 0xFFFFFF }, { 'o', 0xF5863D } };
  scSprite(SPR_V2_STARFISH, 4, 96, 318, 3, shell, 3);
  scSprite(SPR_V2_SHELL, 3, 350, 324, 3, shell, 3);
  scSprite(SPR_V2_STARFISH, 4, 60, 346, 3, shell, 3);
  scSprite(SPR_V2_SHELL, 3, 396, 344, 3, shell, 3);
  const ScKey crab[3] = { { 'r', 0xE86F5F }, { 'w', 0xFFFFFF }, { 'k', 0x1B1B25 } };
  scSprite(SPR_V2_CRAB, 6, 300, 332, 3, crab, 3);
}

static void scCave() {
  const ScPal p = scCavePal();
  scRect(0, 0, V2_W, V2_W, p.top);
  for (int cy = 0; cy < 300; cy += 4)  // the back wall: a lighter hollow in the middle
    for (int cx = 0; cx < V2_W; cx += 4) {
      float a = (cx + 2 - 233) / 150.0f, b = (cy + 2 - 190) / 130.0f, r = a * a + b * b;
      if (r < 1) scCell(cx, cy, (r > .45f || (r > .35f && (cx / 4 + cy / 4) % 2)) ? p.bot : 0x3A3450);
    }
  static const int16_t drip[6][3] = { {40, 70, 120}, {110, 40, 80}, {170, 30, 60}, {300, 36, 70}, {360, 44, 96}, {430, 70, 130} };
  for (auto &s : drip)  // stalactites, lit on their left edge
    for (int cy = 0; cy < s[2]; cy += 4) {
      float half = s[1] / 2.0f * (1 - (float)cy / s[2]);
      for (int cx = (int)(s[0] - half) / 4 * 4; cx <= (int)(s[0] + half); cx += 4)
        scCell(cx, cy, cx < s[0] - half + 4 ? 0x5A5270 : 0x494258);
    }
  scStdHills(p, 1);
  const ScKey cr[2] = { { 't', 0x8FF0E0 }, { 'T', 0x3FB8A8 } };
  static const int16_t back[4][3] = { {70, 196, 3}, {96, 206, 2}, {350, 192, 3}, {378, 204, 2} };
  static const int16_t front[4][3] = { {60, 300, 4}, {90, 316, 3}, {374, 304, 4}, {404, 324, 3} };
  for (auto &c : back) scSprite(SPR_V2_CRYSTAL, 6, c[0], c[1], c[2], cr, 2);
  scStdHills(p, 2);
  for (auto &c : front) scSprite(SPR_V2_CRYSTAL, 6, c[0], c[1], c[2], cr, 2);
  scRole(SPR_V2_ROCK, 5, 150, 330, 3, p, p.near);
  scRole(SPR_V2_ROCK, 5, 300, 336, 3, p, p.near);
  static const int16_t glint[6][2] = { {120, 150}, {340, 140}, {200, 230}, {260, 210}, {150, 260}, {320, 250} };
  for (int k = 0; k < 6; k++) scCell(glint[k][0] / 4 * 4, glint[k][1] / 4 * 4, k % 2 ? 0x8FF0E0 : 0x3FB8A8);
}

// MANSION_PAL in sprites_v2.h has no I, J or O: a letter isn't its index
static Rgb scMansionCol(char c) {
  switch (c) {
    case 'A': return 0x3D3262; case 'B': return 0x2C2448; case 'C': return 0x342A55; case 'D': return 0x1E1838;
    case 'E': return 0x4E4280; case 'F': return 0x7A6AA8; case 'G': return 0x2A2244; case 'H': return 0x4A2E1A;
    case 'K': return 0x0D0B20; case 'L': return 0xF0C04A; case 'M': return 0x141024; case 'N': return 0xFFF0B0;
    case 'P': return 0xB070F0;
  }
  return 0;
}
// the big mansion at 4x: outlined (every empty cell touching it is ink), then moonlight on its left edges
static void scMansion(int x, int y) {
  const int MW = 58, MH = 34, s = 4;
  auto at = [&](int cx, int cy) -> char {
    if (cx < 0 || cy < 0 || cx >= MW || cy >= MH) return '.';
    return SPR_V2_MANSION[cy][cx];
  };
  for (int cy = 0; cy < MH; cy++)  // as the mockup: the outline stays inside the sprite's grid
    for (int cx = 0; cx < MW; cx++)
      if (at(cx, cy) == '.' && (at(cx + 1, cy) != '.' || at(cx - 1, cy) != '.' || at(cx, cy + 1) != '.' || at(cx, cy - 1) != '.'))
        scRect(x + cx * s, y + cy * s, x + cx * s + s - 1, y + cy * s + s - 1, 0x0D0B20);
  for (int cy = 0; cy < MH; cy++)
    for (int cx = 0; cx < MW; cx++) {
      char c = at(cx, cy);
      if (c == '.') continue;
      Rgb col = ((c == 'A' || c == 'C') && at(cx - 1, cy) == '.') ? 0x5A4C8A : scMansionCol(c);
      scRect(x + cx * s, y + cy * s, x + cx * s + s - 1, y + cy * s + s - 1, col);
    }
}

static void scMansionZone() {
  const ScPal p = scMansionPal();
  scSky(p.top, p.bot);
  scSun(p, true, 372, 128);
  scCloud(p, 60, 150, 3);
  scStdHills(p, 1, true);
  scMansion(233 - 58 * 2, 230 - 34 * 4);  // centred behind the pet, its walls standing on the rise
  const ScKey bare[1] = { { '1', p.dark } };
  scSprite(SPR_V2_BARE, 11, 28, 170, 4, bare, 1);
  scStdHills(p, 2);
  static const int16_t stones[7][3] = { {352, 258, 3}, {378, 252, 3}, {404, 262, 3}, {428, 256, 3}, {364, 286, 4}, {402, 290, 4}, {436, 284, 3} };
  const ScKey stone[2] = { { 'o', 0x0D0B20 }, { 's', 0x5A5F7C } };
  for (int i = 0; i < 7; i++)  // the graveyard down the right slope
    if (i % 3 == 1) scSprite(SPR_V2_CROSS, 7, stones[i][0], stones[i][1], stones[i][2], stone, 2);
    else scSprite(SPR_V2_TOMB, 7, stones[i][0], stones[i][1], stones[i][2], stone, 2);
  scFence(312, 340, 466, 0x0D0B20, 0x0D0B20, 18);  // behind an iron fence
  for (int x = 340; x < 466; x += 18) scRect(x + 1, 292, x + 2, 295, 0x0D0B20);
  static const int16_t grass[2][2] = { {40, 330}, {120, 304} };
  scGrass(p, grass, 2);
}

// Draws a whole scene on the canvas. tod: 0 day, 1 dusk, 2 night. The Cave and the Mansion look the same at
// every hour; the others are drawn by day with their land graded, under a dusk or a night sky.
static void scPaint(uint8_t zone, uint8_t tod) {
  scFb = (uint16_t *)gfx->getFramebuffer();
  scGradeTod = 0;
  if (zone == Z_CAVE) { scCave(); return; }
  if (zone == Z_MANSION) { scMansionZone(); return; }
  ScPal p = zone == Z_FIELD ? scFieldPal() : zone == Z_FOREST ? scForestPal() : zone == Z_BEACH ? scBeachPal() : SC_DAY;
  if (tod == 0) {  // the zone's own sky
    scSky(p.top, p.bot);
    if (zone == Z_FIELD) {
      scSun(p, false, 110, 134);
      scCloud(p, 260, 110, 4);
      scCloud(p, 330, 182, 3);
    } else if (zone == Z_FOREST) {
      scSun(p, false, 352, 128);
      scCloud(p, 118, 104, 4);
    } else {
      scSun(p, false, 352, 128);
      scCloud(p, 116, 104, 4);
      scCloud(p, 292, 184, 4);
    }
  } else {  // the dusk or night sky, the same over every zone
    const ScPal &s = tod == 2 ? SC_NIGHT : SC_DUSK_SKY;
    scSky(s.top, s.bot);
    scSun(s, tod == 2, 352, 128);
    scCloud(s, 116, 104, 4);
    scCloud(s, 292, 184, 4);
  }
  scGradeTod = tod;
  if (zone == Z_FIELD) scLandField(p, tod == 2);
  else if (zone == Z_FOREST) scLandForest(p);
  else if (zone == Z_BEACH) scLandBeach(p);
  else scLandMeadow(p);
  scGradeTod = 0;
}

// the home's scene: from the buffer, drawn again only when the zone or the time of day changes
static uint16_t *v2SceneBuf = nullptr;
static int v2SceneKey = -1;
static void v2DrawScene(uint8_t zone, uint8_t tod) {
  if (zone == Z_CAVE || zone == Z_MANSION) tod = 0;
  int key = zone * 4 + tod;
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  if (!v2SceneBuf) v2SceneBuf = (uint16_t *)ps_malloc(V2_W * V2_W * sizeof(uint16_t));
  if (key == v2SceneKey && v2SceneBuf) {
    memcpy(fb, v2SceneBuf, V2_W * V2_W * sizeof(uint16_t));
    return;
  }
  scPaint(zone, tod);  // without the buffer (no memory) it's drawn every frame
  if (v2SceneBuf) {
    memcpy(v2SceneBuf, fb, V2_W * V2_W * sizeof(uint16_t));
    v2SceneKey = key;
  }
}

// the home's zone (an egg's is the meadow) and its time of day: night while it sleeps or after 8 pm,
// dusk in the hours either side of the day
static uint8_t v2HomeZone() { return pet.isEgg() ? Z_MEADOW : zoneOf(pet.speciesId); }
static uint8_t v2HomeTod() {
  int h = sceneHour();
  if (pet.sleeping || h < 6 || h >= 20) return 2;
  return (h < 8 || h >= 18) ? 1 : 0;
}

// a soft shadow under the pet, on the 4 px grid, wherever it stands
static void v2PetShadow(int cx, int cy, int rx) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  int a = gNight || gV2Dark ? 90 : 77;  // 35% at night, 30% by day
  for (int y = (cy - 8) / 4 * 4; y <= cy + 8; y += 4)
    for (int x = (cx - rx - 4) / 4 * 4; x <= cx + rx + 4; x += 4) {
      float dx = (x + 2 - cx) / (float)rx, dy = (y + 2 - cy) / 7.0f;
      if (dx * dx + dy * dy > 1) continue;
      for (int yy = y; yy < y + 4 && yy < V2_W; yy++)
        for (int xx = x; xx < x + 4 && xx < V2_W; xx++)
          if (xx >= 0 && yy >= 0) fb[yy * V2_W + xx] = v2Blend(fb[yy * V2_W + xx], 0, a);
    }
}

// a name over a dark scene: the Pokemon's own colour, brightened (Gengar's comes out lilac)
static uint16_t v2LightAccent(uint16_t c) {
  int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3, m = r > g ? (r > b ? r : b) : (g > b ? g : b);
  if (m < 1) m = 1;
  Rgb s = scMix(scRgb(r * 255 / m, g * 255 / m, b * 255 / m), 0xFFFFFF, .25);
  return sc565(s);
}
