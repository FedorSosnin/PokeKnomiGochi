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

// The icons on their circle. Asleep, all but the moon are slate shadows: ink would vanish on the band. With the
// backpack open, all but the bag are (the band over them has no tail pointing at the bag).
static void v2RepelBagBadge(int cx, int cy);  // (repel_v2.h, below: the Repel)
static bool v2RepelSeat();
static void v2RepelSeatDraw(int cx, int cy);
static void v2DrawIcons() {
  bool bag = feedMenuUntil != 0;  // the backpack open: all but the bag go dim, as they do while it sleeps (Fedor)
  for (int i = 0; i < BTN_COUNT; i++) {
    int x = buttons[i].cx - BTN_HALF, y = buttons[i].cy - BTN_HALF;
    if ((pet.sleeping && i != BTN_LIGHT) || (bag && i != 0)) v2MapFlat(buttons[i].icon, HOME_ICON_N, x, y, 3, C565(0x3a, 0x4a, 0x5a));
    else drawMap(buttons[i].icon, HOME_ICON_N, x, y, 3, false);
  }
  v2RepelBagBadge(buttons[0].cx + 22, buttons[0].cy - 22);  // a Repel running: the minutes left, on the bag
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

// The backpack's seats: the foods (the candy while there are any left today), then the Repel's while you hold one
// or one runs, right of the candy, along an arc over the icons, centred under your Pokemon (Fedor), so it stays in
// view and what you tap is seen dropping onto it
static int v2FeedFoods() { return pet.canHaveCandy() ? 4 : 3; }
static int v2FeedCount() { return v2FeedFoods() + (v2RepelSeat() ? 1 : 0); }
// seat i's direction from the seats' circle's centre (degrees, 0 right, 90 down): the first on the left
static float v2FeedDeg(int i) { return 90 + (v2FeedCount() - 1 - 2 * i) * (V2_FEED_STEP * 28.64789f / V2_FEED_R); }
static void v2FeedSeat(int i, int &cx, int &cy) {  // seat i's centre, on the 2 px grid
  float a = v2FeedDeg(i) * 0.01745329f;
  cx = SNAP2(CX + (int)lroundf(V2_FEED_R * cosf(a)));
  cy = SNAP2(V2_FEED_CY + (int)lroundf(V2_FEED_R * sinf(a)));
}
static int v2FeedAt(int16_t x, int16_t y) {  // the seat under a tap (within half a step of its centre), or -1
  for (int i = 0, n = v2FeedCount(); i < n; i++) {
    int cx, cy;
    v2FeedSeat(i, cx, cy);
    if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= (V2_FEED_STEP / 2) * (V2_FEED_STEP / 2)) return i;
  }
  return -1;
}
#ifdef BOARD_HOST
void v2HostFeedSeat(int i, int *x, int *y) { v2FeedSeat(i, *x, *y); }  // (for the screenshot tool)
#endif
// The food picker: a band in the bag's own green over the icons' band, curved as it is (Fedor: the straight
// bubble covered your Pokemon), with the panels' dark rim and a lighter green line inside; each item on a soft
// green seat (the candy with its count left today, the Repel with how many and its minutes while one runs)
static void v2DrawFoodPicker(int) {
  static const char *const *const FOOD[4] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_CANDY };
  int foods = v2FeedFoods(), count = v2FeedCount();
  float a0 = v2FeedDeg(count - 1), a1 = v2FeedDeg(0);  // its round ends centred on the last seat and the first
  const uint16_t green = C565(0x58, 0xb8, 0x68), line = C565(0x8f, 0xd6, 0x9a), seat = C565(0xa6, 0xd9, 0xae);
  V2Seg rim = v2Seg(V2_FEED_RIN - 3, V2_FEED_ROUT + 3, a0, a1), band = v2Seg(V2_FEED_RIN, V2_FEED_ROUT, a0, a1);
  V2Seg lineOut = v2Seg(V2_FEED_RIN + 3, V2_FEED_ROUT - 3, a0, a1), lineIn = v2Seg(V2_FEED_RIN + 6, V2_FEED_ROUT - 6, a0, a1);
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  int fx0, fy0, lx, ly;  // the box it fits in: the end seats and round them, down to the rim under the middle
  v2FeedSeat(0, fx0, fy0);
  v2FeedSeat(count - 1, lx, ly);
  int x0 = (fx0 - 40) & ~1, y0 = ((fy0 < ly ? fy0 : ly) - 40) & ~1, x1 = lx + 40, y1 = V2_FEED_CY + V2_FEED_ROUT + 4;
  if (x0 < 0) x0 = 0;
  if (x1 > 464) x1 = 464;
  if (y1 > 464) y1 = 464;
  for (int y = y0; y <= y1; y += 2) {
    float fy = y + 1 - V2_FEED_CY;
    for (int x = x0; x <= x1; x += 2) {
      float fx = x + 1 - CX, r2 = fx * fx + fy * fy;
      if (!v2In(rim, fx, fy, r2)) continue;
      uint16_t c = !v2In(band, fx, fy, r2) ? V2_RIM : v2In(lineOut, fx, fy, r2) && !v2In(lineIn, fx, fy, r2) ? line : green;
      v2Cell(fb, x, y, c, 256);
    }
  }
  int candyX = 0, candyY = 0;
  for (int i = 0; i < count; i++) {
    int cx, cy;
    v2FeedSeat(i, cx, cy);
    gfx->fillCircle(cx, cy, V2_FEED_SEAT_R, seat);
    if (i < foods) drawMap(FOOD[i], HOME_ICON_N, cx - 27, cy - 27, 3, false);
    else v2RepelSeatDraw(cx, cy);
    if (i == 3) candyX = cx, candyY = cy;
  }
  if (foods == 4) v2CountBadge(candyX + 22, candyY - 28, pet.candiesLeft());
}

// ---------- the scenes: the home's backdrop, in the Pokemon's Travel zone ----------
// All on a 4 px grid (one 8-bit pixel = 2 Knomi pixels), drawn the way the V2 mockups' scripts draw them
// (pixelhills.py, zones_px.py, mansion2.py; dusk and night as batch3.py's render_tod). Colours are
// 0xRRGGBB and mixed in 8-bit RGB, as there. Nothing in a scene moves yet, so it is drawn once into a
// buffer and copied in every frame; it's drawn again only when the zone or the time of day changes. While
// the Pokemon walks (step 4, walk_v2.h) the layers are drawn into strips of their own instead.

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

static uint16_t *scFb;      // where the scene is drawn: the screen, or a walk layer's strip (walk_v2.h)
static int scW = V2_W, scH = V2_W, scDX = 0, scDY = 0;  // scFb is scW x scH; a screen point lands at x + scDX, y + scDY
static bool scKeyed = false;  // drawing a strip: never its "nothing here" colour (V2_STRIP_KEY)
#define V2_STRIP_KEY 0xF81F   // magenta, which no scene uses
static void scScreen() {      // draw on the screen
  scFb = (uint16_t *)gfx->getFramebuffer();
  scW = scH = V2_W;
  scDX = scDY = 0;
  scKeyed = false;
}
static uint8_t scGradeTod;  // 0: colours as given; 1, 2: a day scene's land at dusk or at night; 3, 4: a menu's room, dimmed
static Rgb scGrade(Rgb c) {
  if (!scGradeTod) return c;
  if (scGradeTod == 3) return scMix(c, 0x120E22, .58);  // the lights down (rooms2_px.py dim())
  if (scGradeTod == 4) return scMix(c, 0x120E22, .62);  // ...a little further, behind a medal's card
  int r = c >> 16, g = (c >> 8) & 255, b = c & 255;
  if (scGradeTod == 2) return scRgb((int)(r * .30 + 8), (int)(g * .36 + 12), (int)(b * .55 + 30));  // about a third of the light, toward blue
  return scMix(scRgb((int)(r * .94), (int)(g * .90), (int)(b * .84)), 0xFFAA6E, .16);                 // a warm wash
}
// a rectangle with both corners inside it, as PIL draws them
static void scRect(int x0, int y0, int x1, int y1, Rgb c) {
  x0 += scDX, x1 += scDX, y0 += scDY, y1 += scDY;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > scW - 1) x1 = scW - 1;
  if (y1 > scH - 1) y1 = scH - 1;
  if (x0 > x1 || y0 > y1) return;
  uint16_t v = sc565(scGrade(c));
  if (scKeyed && v == V2_STRIP_KEY) v ^= 0x20;
  for (int y = y0; y <= y1; y++) {
    uint16_t *p = scFb + y * scW + x0;
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
// Is (x, y) inside the oval in box e: ((x - cx) / rx)^2 + ((y - cy) / ry)^2 <= 1, as the mockups test it.
// Worked in whole numbers (everything doubled, so the centre is whole), which is exact; only a point exactly
// on the edge is left to floating point, rounded as Python rounds it there.
static bool scInEll(int x, int y, const ScEll &e) {
  int32_t X = 2 * x - (e.x0 + e.x1), Y = 2 * y - (e.y0 + e.y1), RX = e.x1 - e.x0, RY = e.y1 - e.y0;
  int64_t l = (int64_t)(X * X) * (RY * RY) + (int64_t)(Y * Y) * (RX * RX), r = (int64_t)(RX * RX) * (RY * RY);
  if (l != r) return l < r;
  volatile double a = (double)X / RX, b = (double)Y / RY;  // volatile: each step rounded on its own, as there
  volatile double a2 = a * a, b2 = b * b;
  volatile double s = a2 + b2;
  return s <= 1.0;
}
static int scLay(const ScLayer *L, int nl, int x, int y) {
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
  int y0 = -scDY > 192 ? -scDY / 4 * 4 : 192, y1 = scH - scDY;  // the hills all start below y 200
  for (int cy = y0; cy < y1; cy += 4)
    for (int cx = -scDX; cx < scW - scDX; cx += 4) {  // a strip's margins are whole cells
      int l = scLay(L, nl, cx + 2, cy + 2);
      if (l < from) continue;
      Rgb c = L[l - 1].col;
      if (scLay(L, nl, cx + 2, cy + 2 - 4) != l) c = litc[l - 1];
      else if (scLay(L, nl, cx + 2, cy + 2 - 8) != l && (((cx >> 2) + (cy >> 2)) & 1) == 0) c = half[l - 1];
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

static const int16_t SC_FIELD_TREES[3][2] = { {326, 194}, {362, 186}, {398, 200} };
static void scLandField(const ScPal &p, bool night) {
  const ScKey bird[1] = { { 'k', 0x18181F } };
  scSprite(SPR_V2_BIRD_UP, 3, 196, 150, 3, bird, 1);
  scSprite(SPR_V2_BIRD_UP, 3, 226, 140, 3, bird, 1);
  scStdHills(p, 1);
  scHouse(50, 186, night);
  for (auto &t : SC_FIELD_TREES) scRole(SPR_V2_TREE, 10, t[0], t[1], 3, p, p.mid);
  scStdHills(p, 2);
  scFence(262, 0, 160, 0x9A6F43, 0xB38A58, 24);  // the Field tile's wooden fence
  scFence(266, 312, 466, 0x9A6F43, 0xB38A58, 24);
  scGrass(p, SC_GRASS6, 6);
  scFlowers(SC_FLOWERS5, 5, p.fl, 3);
}

static const int16_t SC_FOREST_BIG[4][2] = { {14, 246}, {52, 258}, {384, 254}, {418, 244} };
#define SC_FOREST_PINK 0xE9587A
// the far hills' top at x (the highest of their two ovals there), or 999 where neither reaches
static int scFarTop(int x) {
  int top = 999;
  for (int k = 0; k < 2; k++) {
    const ScEll &e = SC_FAR[k];
    double cx = (e.x0 + e.x1) / 2.0, rx = (e.x1 - e.x0) / 2.0, cy = (e.y0 + e.y1) / 2.0, ry = (e.y1 - e.y0) / 2.0;
    double d = (x - cx) / rx;
    if (d * d < 1) {
      int t = (int)(cy - ry * sqrt(1 - d * d));
      if (t < top) top = t;
    }
  }
  return top;
}
// a wall of pines on the far hills, open behind the pet; each stands at least 4 px into the hill under its trunk
// (Fedor: one hung in the air where the hill dips, left of the pet)
static void scPineWall(const ScPal &p) {
  for (int tx = 0; tx < V2_W; tx += 26) {
    if (tx > 150 && tx < 300) continue;
    int y = 178 + (tx * 37 % 18), top = scFarTop(tx + 8);  // (its trunk: the middle of its 36 px)
    if (top < 999 && y + 36 < top + 4) y = top + 4 - 36;
    scRole(SPR_V2_PINE, 12, tx - 10, y, 3, p, p.far);
  }
}
static void scLandForest(const ScPal &p) {
  static const int16_t grass[4][2] = { {112, 300}, {330, 300}, {40, 344}, {414, 342} };
  static const int16_t flowers[3][2] = { {150, 330}, {300, 336}, {180, 352} };
  const Rgb pink = SC_FOREST_PINK;
  scStdHills(p, 1);
  scPineWall(p);
  scStdHills(p, 2);
  for (auto &t : SC_FOREST_BIG) scRole(SPR_V2_PINE, 12, t[0], t[1], 4, p, p.mid);
  scGrass(p, grass, 4);
  scFlowers(flowers, 3, &pink, 1);
}

#define SC_SEA 0x4F96C4
#define SC_SEA_HI 0xBFE6F5
static void scPalmKeys(ScKey *k, const ScPal &p) { scTreeKeys(k, p, 0x58B868, 0x2F6A3A); }
static void scLandBeach(const ScPal &p) {
  static const int16_t glint[25][2] = {  // where the mockup's waves glint (its random.Random(3))
    {120, 236}, {300, 236}, {276, 236}, {64, 236}, {188, 236}, {308, 248}, {240, 248}, {320, 248}, {296, 248},
    {32, 248}, {308, 260}, {4, 260}, {464, 260}, {428, 260}, {240, 260}, {132, 272}, {280, 272}, {116, 272},
    {96, 272}, {364, 272}, {240, 284}, {276, 284}, {428, 284}, {280, 284}, {240, 284} };
  const Rgb sea = SC_SEA, seaHi = SC_SEA_HI;
  scRect(0, 224, V2_W, 300, sea);
  scRect(0, 224, V2_W, 227, scMix(sea, 0xFFFFFF, .35));  // the horizon
  for (auto &g : glint) scRect(g[0], g[1], g[0] + 11, g[1] + 3, g[1] < 260 ? seaHi : scMix(seaHi, sea, .4));
  ScLayer L[2] = { { p.mid, SC_DUNES, 2 }, { p.near, SC_SHORE, 1 } };
  scHills(L, 2, p.lit, 1);
  ScKey palm[5];  // palms on the dunes, a size up so they read at the Knomi's size
  scPalmKeys(palm, p);
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

static const ScKey SC_CRYSTAL_KEYS[2] = { { 't', 0x8FF0E0 }, { 'T', 0x3FB8A8 } };
static void scCaveWall(const ScPal &p) {  // the back wall and the stalactites
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
}
static void scCaveBack() {  // crystals on the far floor
  static const int16_t back[4][3] = { {70, 196, 3}, {96, 206, 2}, {350, 192, 3}, {378, 204, 2} };
  for (auto &c : back) scSprite(SPR_V2_CRYSTAL, 6, c[0], c[1], c[2], SC_CRYSTAL_KEYS, 2);
}
static void scCaveGlints() {  // glints in the dark
  static const int16_t glint[6][2] = { {120, 150}, {340, 140}, {200, 230}, {260, 210}, {150, 260}, {320, 250} };
  for (int k = 0; k < 6; k++) scCell(glint[k][0] / 4 * 4, glint[k][1] / 4 * 4, k % 2 ? 0x8FF0E0 : 0x3FB8A8);
}
static void scCave() {
  const ScPal p = scCavePal();
  scCaveWall(p);
  scStdHills(p, 1);
  static const int16_t front[4][3] = { {60, 300, 4}, {90, 316, 3}, {374, 304, 4}, {404, 324, 3} };
  scCaveBack();
  scStdHills(p, 2);
  for (auto &c : front) scSprite(SPR_V2_CRYSTAL, 6, c[0], c[1], c[2], SC_CRYSTAL_KEYS, 2);
  scRole(SPR_V2_ROCK, 5, 150, 330, 3, p, p.near);
  scRole(SPR_V2_ROCK, 5, 300, 336, 3, p, p.near);
  scCaveGlints();
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
static void scMansion(int x, int y, int s = 4) {
  const int MW = 58, MH = 34;
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

static void scMansionFar(const ScPal &p) {  // on the rise: the mansion and a bare tree
  scMansion(233 - 58 * 2, 230 - 34 * 4);  // centred behind the pet, its walls standing on the rise
  const ScKey bare[1] = { { '1', p.dark } };
  scSprite(SPR_V2_BARE, 11, 28, 170, 4, bare, 1);
}
static void scGraveyard(int dx, int dy) {  // down the right slope, behind an iron fence
  static const int16_t stones[7][3] = { {352, 258, 3}, {378, 252, 3}, {404, 262, 3}, {428, 256, 3}, {364, 286, 4}, {402, 290, 4}, {436, 284, 3} };
  const ScKey stone[2] = { { 'o', 0x0D0B20 }, { 's', 0x5A5F7C } };
  for (int i = 0; i < 7; i++)
    if (i % 3 == 1) scSprite(SPR_V2_CROSS, 7, stones[i][0] + dx, stones[i][1] + dy, stones[i][2], stone, 2);
    else scSprite(SPR_V2_TOMB, 7, stones[i][0] + dx, stones[i][1] + dy, stones[i][2], stone, 2);
  scFence(312 + dy, 340 + dx, 466 + dx, 0x0D0B20, 0x0D0B20, 18);
  for (int x = 340; x < 466; x += 18) scRect(x + 1 + dx, 292 + dy, x + 2 + dx, 295 + dy, 0x0D0B20);
}
static void scMansionZone() {
  const ScPal p = scMansionPal();
  scSky(p.top, p.bot);
  scSun(p, true, 372, 128);
  scCloud(p, 60, 150, 3);
  scStdHills(p, 1, true);
  scMansionFar(p);
  scStdHills(p, 2);
  scGraveyard(0, 0);
  static const int16_t grass[2][2] = { {40, 330}, {120, 304} };
  scGrass(p, grass, 2);
}

// Draws a whole scene on the canvas. tod: 0 day, 1 dusk, 2 night. The Cave and the Mansion look the same at
// every hour; the others are drawn by day with their land graded, under a dusk or a night sky.
static ScPal scZonePal(uint8_t zone) {
  return zone == Z_FIELD ? scFieldPal() : zone == Z_FOREST ? scForestPal() : zone == Z_BEACH ? scBeachPal()
       : zone == Z_CAVE ? scCavePal() : zone == Z_MANSION ? scMansionPal() : SC_DAY;
}
static void scPaint(uint8_t zone, uint8_t tod) {
  scScreen();
  scGradeTod = 0;
  if (zone == Z_CAVE) { scCave(); return; }
  if (zone == Z_MANSION) { scMansionZone(); return; }
  ScPal p = scZonePal(zone);
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

// A whole-screen backdrop from the buffer: painted again only when its key changes. The home's scene is
// one (its key is zone * 4 + tod) and the games' backgrounds are others, so one buffer serves them all.
// Without the buffer (no memory) it's painted every frame.
static uint16_t *v2SceneBuf = nullptr;
static int v2SceneKey = -1;
static void v2Cached(int key, void (*paint)(int), int arg, int rows = V2_W) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  if (!v2SceneBuf) v2SceneBuf = (uint16_t *)ps_malloc(V2_W * V2_W * sizeof(uint16_t));
  if (key == v2SceneKey && v2SceneBuf) {
    memcpy(fb, v2SceneBuf, rows * V2_W * sizeof(uint16_t));
    return;
  }
  scScreen();
  scGradeTod = 0;
  paint(arg);
  scGradeTod = 0;
  if (v2SceneBuf) {
    memcpy(v2SceneBuf, fb, V2_W * V2_W * sizeof(uint16_t));
    v2SceneKey = key;
  }
}

// the home's scene
static void scPaintKey(int key) { scPaint(key / 4, key % 4); }
static void v2DrawScene(uint8_t zone, uint8_t tod) {
  if (zone == Z_CAVE || zone == Z_MANSION) tod = 0;
  v2Cached(zone * 4 + tod, scPaintKey, zone * 4 + tod);
}

// the time of day by the clock: night after 8 pm and before 6 am, dusk in the two hours either side of
// the day (0 day, 1 dusk, 2 night)
static uint8_t v2HourTod() {
  int h = sceneHour();
  if (h < 6 || h >= 20) return 2;
  return (h < 8 || h >= 18) ? 1 : 0;
}
// the home's zone (an egg's is the meadow) and its time of day: night too while it sleeps
static uint8_t v2HomeZone() { return pet.isEgg() ? Z_MEADOW : zoneOf(pet.speciesId); }
static uint8_t v2HomeTod() { return pet.sleeping ? 2 : v2HourTod(); }

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

// ---------- step 3: the games' backgrounds ----------
// The home scenes' style (the 4 px grid, dithered bands, lit edges), drawn as the mockups' games_px.py
// draws them; PokeDungeon's tiles are rooms_px.py's and the battle's bases rooms2_px.py's (battle_hud).
// The game layouts don't move: where a mockup and the game differ (PokeRun's ground, Volley's court, where
// the fighters stand), the picture is moved to fit the game.

#define SC_NONE 0xFF000000u  // no colour: an oval without a lit top row or without an underside

// the mockups' floor division by 4 and modulo, which round down for negative numbers too
static inline int scFl4(int v) { return v >= 0 ? v / 4 : -((3 - v) / 4); }
static inline int scMod(int a, int m) {
  int r = a % m;
  return r < 0 ? r + m : r;
}
static inline ScEll scE(int x0, int y0, int x1, int y1) { return { (int16_t)x0, (int16_t)y0, (int16_t)x1, (int16_t)y1 }; }

// flat bands from x0 to x1, with two rows of checker each side of where two meet (dither_bands)
static void scBands(const Rgb *cols, const int16_t *edges, int n, int x0, int x1) {
  for (int i = 0; i < n; i++) scRect(x0, edges[i], x1, edges[i + 1], cols[i]);
  for (int i = 1; i < n; i++)
    for (int y = edges[i] - 8; y < edges[i] + 8; y += 4)
      for (int x = scFl4(x0) * 4; x < x1; x += 4)
        if ((scFl4(x) + scFl4(y)) % 2 == 0) scCell(x, y, y < edges[i] ? cols[i] : cols[i - 1]);
}

// an oval on the grid (e is its box), with a lit top row and a darker underside when given (ell_cells)
static void scOval(const ScEll &e, Rgb col, Rgb lit = SC_NONE, Rgb under = SC_NONE) {
  int y0 = scFl4(e.y0) * 4, x0 = scFl4(e.x0) * 4;  // cells off the screen are skipped: the grid stays the same
  int y1 = e.y1 + 4 < V2_W ? e.y1 + 4 : V2_W, x1 = e.x1 + 4 < V2_W ? e.x1 + 4 : V2_W;
  for (int cy = y0 < 0 ? 0 : y0; cy < y1; cy += 4)
    for (int cx = x0 < 0 ? 0 : x0; cx < x1; cx += 4) {
      if (!scInEll(cx + 2, cy + 2, e)) continue;
      Rgb c = col;
      if (lit != SC_NONE && !scInEll(cx + 2, cy - 2, e)) c = lit;
      else if (under != SC_NONE && !scInEll(cx + 2, cy + 10, e)) c = under;
      scCell(cx, cy, c);
    }
}

// a patch of tall grass growing out of ground of colour base
static void scTall(int x, int y, Rgb base) {
  const ScKey k[2] = { { 'G', SC_DAY.dark }, { 'g', scMix(base, SC_DAY.dark, .35) } };
  scSprite(SPR_V2_TALLGRASS, 5, x, y, 3, k, 2);
}

// a game's sky in three bands: its own colours by day, the dusk or the night sky's otherwise, with stars at
// night (they and the sky are never graded)
static void scGameSky(const Rgb *day, const int16_t *edges, uint8_t tod) {
  Rgb c[3] = { day[0], day[1], day[2] };
  if (tod) {
    const ScPal &s = tod == 2 ? SC_NIGHT : SC_DUSK_SKY;
    c[0] = s.top;
    c[1] = scMix(s.top, s.bot, .5);
    c[2] = s.bot;
  }
  scBands(c, edges, 3, 0, V2_W);
  if (tod == 2)
    for (auto &s : SC_STARS) scCell(s[0] / 4 * 4, s[1] / 4 * 4, 0xFFFFFF);
}
static ScPal scSkyPal(uint8_t tod) {  // the day palette with the clouds of the hour
  ScPal p = SC_DAY;
  if (tod) {
    const ScPal &s = tod == 2 ? SC_NIGHT : SC_DUSK_SKY;
    p.cloud = s.cloud;
    p.cloudSh = s.cloudSh;
  }
  return p;
}

// Berry Drop: a forest clearing framed by two big trees, a leafy canopy along the top with vines hanging
// from it, pale pines far off, a grassy floor (g_tilt). At dusk and at night it's graded like the home
// scenes, under that hour's sky.
static void scTilt(int tod) {
  static const Rgb sky[3] = { 0x9FD6F0, 0xC6E8EC, 0xE2F2EA };
  static const int16_t edge[4] = { 0, 150, 250, 466 };
  scGameSky(sky, edge, tod);
  scGradeTod = tod;
  const ScKey pale[5] = { { '1', 0x8CC49A }, { '2', 0xB6DCBC }, { '3', 0xA2D2AC }, { '4', 0x8CC49A }, { '5', 0x8CC49A } };
  for (int tx = -10; tx < V2_W; tx += 30) scSprite(SPR_V2_PINE, 12, tx, 250 + scMod(tx * 13, 20), 4, pale, 5);
  scOval(scE(-200, 330, 666, 700), 0x6DB673, 0x9AD69C);  // the clearing's floor
  const Rgb bark = 0x7E5530, barkD = 0x5A3A20, barkL = 0xA07040;
  static const int16_t trunk[2][2] = { { 20, 84 }, { 382, 446 } };
  for (auto &t : trunk)  // two trunks, lit on the left, with grooves in the bark
    for (int cy = 60; cy < 420; cy += 4)
      for (int cx = t[0]; cx < t[1]; cx += 4)
        scCell(cx, cy, cx < t[0] + 8 ? barkL : (cx / 4 % 4 == 0 || cx > t[1] - 8) ? barkD : bark);
  static const ScEll canopy[7] = { { -40, -60, 140, 120 }, { 60, -80, 220, 70 }, { 180, -90, 300, 50 }, { 250, -80, 410, 70 },
                                   { 330, -60, 510, 120 }, { -30, 40, 120, 150 }, { 350, 40, 500, 150 } };
  for (auto &e : canopy) scOval(e, 0x3F8A4C, SC_NONE, 0x2F6A3A);
  for (int i = 0; i < 5; i++) scOval(scE(canopy[i].x0 + 20, canopy[i].y0 + 10, canopy[i].x1 - 30, canopy[i].y1 - 30), 0x58B868);
  static const int16_t vine[4][2] = { { 120, 150 }, { 150, 100 }, { 316, 130 }, { 346, 170 } };
  for (auto &v : vine) {  // vines with their leaves
    for (int cy = 40; cy < 40 + v[1]; cy += 4) scCell(v[0], cy, 0x2F6A3A);
    for (int cy = 52; cy < 40 + v[1]; cy += 16) {
      scCell(v[0] + 4, cy, 0x58B868);
      scCell(v[0] - 4, cy + 8, 0x58B868);
    }
  }
  static const int16_t grass[4][2] = { { 96, 380 }, { 140, 400 }, { 300, 392 }, { 344, 376 } };
  for (auto &g : grass) scTall(g[0], g[1], 0x6DB673);
  scGradeTod = 0;
}

// PokeRun: a dithered sky, two rows of pines (a pale far row, a big near row), a dirt track with a lit grass
// edge, pebbles (g_run). The game's ground line is RUN_GROUND_Y, 18 px under the mockup's track, so the land
// moves down with it. The sky, its clouds and the hills behind come from the buffer; the pines and the track
// are drawn every frame, each sliding at its own speed as the run goes on (as V1's two rows of pines did).
#define V2_RUN_DY (RUN_GROUND_Y - 322)
static void scRunSky(int tod) {
  static const Rgb sky[3] = { 0x8FCBE8, 0xB8E0EA, 0xDCF0E8 };
  static const int16_t edge[4] = { 0, 120, 210, 466 };
  scGameSky(sky, edge, tod);
  ScPal p = scSkyPal(tod);
  scCloud(p, 80, 120, 4);
  scCloud(p, 320, 96, 3);
  scGradeTod = tod;
  scOval(scE(-100, 236 + V2_RUN_DY, 260, 420 + V2_RUN_DY), 0xA7D7B0, 0xC4E6C8);
  scOval(scE(200, 226 + V2_RUN_DY, 600, 420 + V2_RUN_DY), 0xA7D7B0, 0xC4E6C8);
  scGradeTod = 0;
}
static const int16_t V2_PEBBLES[40][3] = {  // where the mockup's random.Random(2) put them; 1: a dark one
  {440,460,0},{432,348,1},{44,352,0},{184,448,1},{84,436,0},{412,424,1},{436,380,0},{128,416,1},{108,416,0},{16,416,1},
  {348,360,0},{220,420,1},{200,444,0},{368,452,1},{260,460,0},{188,408,1},{224,404,0},{136,456,1},{16,452,0},{12,388,1},
  {236,460,0},{160,456,1},{192,396,0},{456,452,1},{268,360,0},{284,364,1},{120,368,0},{12,364,1},{164,364,0},{68,404,1},
  {260,388,0},{260,428,1},{284,364,0},{456,396,1},{404,392,0},{376,408,1},{464,456,0},{388,388,1},{404,416,0},{180,388,1} };
static void scRunLand(float dist, uint8_t tod) {
  scGradeTod = tod;
  ScKey farK[5], nearK[5];
  scTreeKeys(farK, SC_DAY, 0x86C58E, SC_DAY.dark);
  scTreeKeys(nearK, SC_DAY, 0x5F9E68, SC_DAY.dark);
  int of = SNAP2((int)(dist * 0.2f)), on = SNAP2((int)(dist * 0.45f)), og = SNAP2((int)dist);  // on even pixels
  for (int k = (of + 10) / 34 - 2;; k++) {  // the far row: a pine every 34 px, its height by where it stands
    int wx = -10 + 34 * k, x = wx - of;
    if (x >= V2_W) break;
    if (x > -40) scSprite(SPR_V2_PINE, 12, x, 222 + V2_RUN_DY + scMod(wx * 7, 16), 4, farK, 5);
  }
  for (int k = (on + 30) / 70 - 2;; k++) {  // the near row, every 70 px and half as big again
    int wx = -30 + 70 * k, x = wx - on;
    if (x >= V2_W) break;
    if (x > -60) scSprite(SPR_V2_PINE, 12, x, 248 + V2_RUN_DY + scMod(wx * 11, 12), 6, nearK, 5);
  }
  const int gy = 322 + V2_RUN_DY;
  scRect(0, gy, V2_W, V2_W, 0x8A5A34);        // the track,
  scRect(0, gy, V2_W, gy + 3, 0x79BD4F);      // its grass edge: a lit row,
  scRect(0, gy + 4, V2_W, gy + 7, 0x5F9E3E);  // a darker one,
  for (int c = scFl4(og) - 1; c * 4 - og < V2_W; c++)  // and a row of checker into the dirt (the mockup's at y 330)
    if ((c + 330 / 4) % 2) scCell(c * 4 - og, gy + 8, 0x5F9E3E);
  for (auto &b : V2_PEBBLES) {  // pebbles, coming round again every 468 px
    int x = scMod(b[0] - og, 468);
    Rgb c = b[2] ? 0x6E4428 : 0xA87048;
    scCell(x, b[1] + V2_RUN_DY, c);
    if (x > 462) scCell(x - 468, b[1] + V2_RUN_DY, c);
  }
  scGradeTod = 0;
}
static void v2RunScene(float dist) {
  uint8_t tod = v2HourTod();
  v2Cached(104 + tod, scRunSky, tod);
  scScreen();
  scRunLand(dist, tod);
}

// PikaSurf: open water from above, in deep and shallow bands with checker seams, stepped wave crests and
// specks (g_surf). It rushes up the screen: the bands at half the water's speed and the crests at its speed,
// as V1's swells and streaks did. Drawn every frame, graded at dusk and at night.
static const int16_t V2_CRESTS[26][2] = {  // the mockup's random.Random(4): 26 crests, then 30 specks
  {120,152},{52,368},{200,244},{76,44},{32,8},{204,280},{148,408},{388,28},{112,264},{272,184},{140,396},{88,420},{52,132},
  {108,12},{424,328},{412,132},{408,136},{96,84},{156,148},{320,444},{372,436},{432,448},{188,44},{432,308},{172,340},{196,256} };
static const int16_t V2_SPECKS[30][2] = {
  {124,88},{124,240},{140,44},{416,440},{280,428},{152,0},{464,148},{292,360},{448,156},{432,388},{260,96},{208,216},{304,144},{220,228},{80,116},
  {156,132},{416,408},{20,40},{20,236},{320,140},{264,272},{328,240},{356,172},{72,344},{100,32},{208,100},{324,320},{224,140},{92,180},{220,380} };
static void v2SurfWater(float dist, uint8_t tod) {
  scScreen();
  scGradeTod = tod;
  const Rgb band[2] = { 0x2F78C8, 0x3F8AD8 };  // deep, shallow: each 116 px
  int ob = SNAP2((int)(dist / 2)), oc = SNAP2((int)dist);
  for (int b = ob / 116; b * 116 - ob < V2_W; b++) scRect(0, b * 116 - ob, V2_W, b * 116 - ob + 115, band[b & 1]);
  for (int b = ob / 116; b * 116 - 8 - ob < V2_W; b++)  // the seams, the checker moving with the water
    for (int wy = b * 116 - 8; wy < b * 116 + 8; wy += 4)
      for (int x = 0; x < V2_W; x += 4)
        if ((x / 4 + scFl4(wy)) % 2 == 0) scCell(x, wy - ob, wy < b * 116 ? band[b & 1] : band[(b - 1) & 1]);
  static const int8_t crest[5][3] = { { 0, 4, 0 }, { 4, 0, 1 }, { 8, 0, 1 }, { 12, 0, 1 }, { 16, 4, 0 } };  // a stepped arc
  for (auto &c : V2_CRESTS)
    for (int y = scMod(c[1] - oc, 468), k = 0; k < 2; k++, y -= 468)  // coming round again every 468 px
      for (auto &d : crest) scCell(c[0] + d[0], y + d[1], d[2] ? 0xBFE6F5 : 0x8FCBEF);
  for (auto &s : V2_SPECKS)
    for (int y = scMod(s[1] - oc, 468), k = 0; k < 2; k++, y -= 468) scCell(s[0], y, 0x6FB2EC);
  scGradeTod = 0;
}

// Volley: a mown court from above in striped greens, chalk lines, a net with a checker mesh, packed clay
// either side with grit in it (g_volley). Moved onto the game's court: the side lines on its walls (VB_LEFT
// and VB_RIGHT), the net's tape on VB_TOP, where the ball crosses (44 px under the mockup's), the attack line
// where the game has it.
static const int16_t V2_GRIT[160][3] = {  // the mockup's random.Random(9); 1: a light one
  {236,312,1},{188,136,0},{68,92,0},{440,344,1},{0,172,0},{256,236,0},{456,308,1},{40,168,0},{280,312,0},{356,20,1},
  {372,192,0},{84,360,0},{228,368,1},{216,80,0},{84,120,0},{24,56,1},{64,256,0},{444,300,0},{32,396,1},{352,196,0},
  {404,380,0},{452,52,1},{148,104,0},{344,112,0},{368,404,1},{212,452,0},{44,392,0},{136,104,1},{200,140,0},{172,412,0},
  {20,100,1},{360,452,0},{0,448,0},{208,28,1},{456,192,0},{432,248,0},{68,12,1},{120,216,0},{376,464,0},{432,56,1},
  {304,0,0},{60,388,0},{296,100,1},{456,436,0},{100,168,0},{4,456,1},{40,68,0},{276,8,0},{256,40,1},{292,252,0},
  {272,100,0},{212,32,1},{200,96,0},{324,388,0},{44,356,1},{296,72,0},{88,308,0},{368,20,1},{24,136,0},{284,348,0},
  {308,436,1},{76,380,0},{136,376,0},{292,16,1},{60,444,0},{360,204,0},{460,120,1},{80,312,0},{256,20,0},{376,184,1},
  {432,344,0},{264,300,0},{288,396,1},{352,44,0},{444,176,0},{56,440,1},{448,296,0},{184,228,0},{104,204,1},{100,292,0},
  {380,8,0},{192,408,1},{308,168,0},{0,220,0},{56,108,1},{112,224,0},{136,164,0},{416,44,1},{156,320,0},{144,52,0},
  {264,400,1},{28,12,0},{192,396,0},{284,412,1},{216,252,0},{176,360,0},{396,124,1},{288,376,0},{32,216,0},{328,116,1},
  {48,388,0},{204,260,0},{188,440,1},{284,188,0},{68,340,0},{148,84,1},{352,428,0},{448,132,0},{12,24,1},{412,88,0},
  {132,84,0},{60,308,1},{72,4,0},{24,28,0},{228,104,1},{400,196,0},{148,312,0},{4,408,1},{180,144,0},{72,244,0},
  {284,140,1},{68,44,0},{380,284,0},{148,356,1},{60,32,0},{212,228,0},{72,360,1},{284,388,0},{404,196,0},{300,452,1},
  {408,84,0},{336,364,0},{140,392,1},{304,448,0},{308,252,0},{176,48,1},{24,140,0},{268,64,0},{88,408,1},{320,420,0},
  {40,80,0},{396,404,1},{112,436,0},{4,100,0},{88,240,1},{232,300,0},{284,276,0},{196,40,1},{88,280,0},{244,300,0},
  {132,272,1},{32,332,0},{12,296,0},{296,452,1},{220,276,0},{156,268,0},{268,272,1},{328,464,0},{404,32,0},{448,328,1} };
static void scCourt(int tod) {
  scGradeTod = tod;
  scRect(0, 0, V2_W, V2_W, 0xC8643C);
  for (auto &g : V2_GRIT) scCell(g[0], g[1], g[2] ? 0xD87A50 : 0xA84E30);
  const int L = VB_LEFT - 2, R = VB_RIGHT + 2, dy = 44;  // the court's outer edges; the mockup's net, moved down
  static const Rgb stripe[6] = { 0x3C8A4C, 0x4A9A58, 0x3C8A4C, 0x4A9A58, 0x3C8A4C, 0x4A9A58 };
  static const int16_t edge[7] = { 52 + 44, 120 + 44, 188 + 44, 256 + 44, 324 + 44, 392 + 44, 466 };
  scBands(stripe, edge, 6, L, R);
  scRect(L, edge[0], L + 4, V2_W, 0xFFFFFF);  // the side lines,
  scRect(R - 4, edge[0], R, V2_W, 0xFFFFFF);
  scRect(L, edge[0], R, edge[0] + 4, 0xFFFFFF);  // the line under the net
  scRect(L, 196, R, 200, 0xFFFFFF);              // and the attack line
  for (int cy = 28 + dy; cy < 52 + dy; cy += 4)  // the net
    for (int cx = L - 12; cx < R + 14; cx += 4) scCell(cx, cy, (cx / 4 + cy / 4) % 2 ? 0x1B1B25 : 0xE8E8F0);
  scRect(L - 12, 24 + dy, R + 14, 27 + dy, 0xFFFFFF);
  scRect(L - 16, 20 + dy, L - 9, 60 + dy, 0x8A8FA6);
  scRect(R + 10, 20 + dy, R + 17, 60 + dy, 0x8A8FA6);
  scGradeTod = 0;
}

// Who's That?: the classic burst of rays in two blues, stepped on the grid, with a lighter middle and a
// checker edge round it (g_quiz). Showdown has the same burst in pinks. It doesn't change with the hour.
static void scBurst(int pink) {  // 0 blue (Who's That?), 1 pink (Showdown), 2 gold (a catch)
  static const Rgb A[3] = { 0x3F8FD8, 0xC84874, 0xE8A23C }, B[3] = { 0x5AA8EC, 0xD8608A, 0xF0C04A }, G[3] = { 0x8CC8F4, 0xEE9AB4, 0xFFF0B0 };
  const Rgb a = A[pink], b = B[pink], glow = G[pink];
  for (int cy = 0; cy < V2_W; cy += 4)
    for (int cx = 0; cx < V2_W; cx += 4) {
      int dx = cx + 2 - 233, dy = cy + 2 - 200, r2 = dx * dx + dy * dy;
      double ang = atan2((double)dy, (double)dx) * 57.29577951308232;  // in degrees, as math.degrees
      if (ang < 0) ang += 360;
      Rgb c = (int)(ang / 15) % 2 ? a : b;
      if (r2 < 110 * 110 || (r2 < 126 * 126 && (cx / 4 + cy / 4) % 2)) c = glow;
      scCell(cx, cy, c);
    }
}

// The battle's backdrop: its zone's scene (a wild battle's is the meadow, as g_battle draws it), and two
// grassy bases where the game stands the two fighters, lit along the top with a dark underside under them
// (battle_hud, which has them in the Forest). The catch has the scene alone: there it runs about.
static int16_t v2BtlFoe[2], v2BtlMe[2];  // where the fighters' feet are: set by the battle
static void scMeadowBattle(uint8_t tod) {
  const ScPal &p = SC_DAY;
  const ScPal &s = tod == 2 ? SC_NIGHT : tod == 1 ? SC_DUSK_SKY : SC_DAY;
  scSky(s.top, s.bot);
  scSun(s, tod == 2, 110, 120);
  scCloud(s, 300, 90, 3);
  scGradeTod = tod;
  static const ScEll nearB[1] = { { -400, 250, 866, 1100 } };  // the near hill comes up higher than at home
  ScLayer L[3] = { { p.far, SC_FAR, 2 }, { p.mid, SC_MID, 2 }, { p.near, nearB, 1 } };
  scHills(L, 3, p.lit, 1);
  static const int16_t trees[2][2] = { { 40, 196 }, { 76, 206 } };  // off to the left, clear of the foe's base
  for (auto &t : trees) scRole(SPR_V2_TREE, 10, t[0], t[1], 3, p, p.mid);
  scHills(L, 3, p.lit, 2);
  scGradeTod = 0;
}
// a base under feet at (x, y): hw either side, up above them and down below (as battle_hud has them), in the
// zone's colours (the Forest's and the meadow's are the mockups'; the others have their ground's colour, lit
// and shaded the same way)
static void scBase(int8_t zone, uint8_t tod, int x, int y, int hw, int up, int down, bool mine) {
  Rgb fill = 0x74BF74, lit = 0x8CD08A, under = 0x3F7E52;  // the meadow's
  if (zone == Z_FOREST) fill = 0x4F9A55, lit = 0x74BF74, under = 0x234A30;
  else if (zone >= 0) {
    ScPal p = zone == Z_FIELD ? scFieldPal() : zone == Z_BEACH ? scBeachPal() : zone == Z_CAVE ? scCavePal() : scMansionPal();
    fill = p.near;
    lit = scMix(p.near, 0xFFFFFF, .25);
    under = scMix(p.near, p.dark, .85);
  }
  scGradeTod = zone == Z_CAVE || zone == Z_MANSION ? 0 : tod;
  ScEll e = scE(x - hw, y - up, x + hw, y + down);
  scOval(scE(e.x0, e.y0 + 8, e.x1, e.y1 + 8), under);
  scOval(e, fill, lit);
  if (zone < 0) {  // the meadow's: a lighter middle and grass at its foot (where g_battle has them)
    scOval(scE(e.x0 + 18, e.y0 + 8, e.x1 - 18, e.y1 - 6), 0x86C98A);
    scTall(e.x0 + (mine ? 14 : 12), e.y1 - (mine ? 16 : 10), fill);
    scTall(e.x1 - (mine ? 22 : 20), e.y1 - (mine ? 22 : 14), fill);
  }
  scGradeTod = 0;
}
static void scBattle(int key) {  // key: (zone + 1) * 8 + tod * 2 + bases
  int8_t zone = key / 8 - 1;
  uint8_t tod = key / 2 % 4;
  if (zone < 0) scMeadowBattle(tod);
  else scPaint(zone, zone == Z_CAVE || zone == Z_MANSION ? 0 : tod);
  if (!(key & 1)) return;
  scBase(zone, tod, v2BtlFoe[0], v2BtlFoe[1], 80, 24, 16, false);
  scBase(zone, tod, v2BtlMe[0], v2BtlMe[1], 100, 28, 22, true);
}
// zone: the battle's Travel zone, or -1 for a wild battle (the meadow); bases: false for the catch
static void v2BattleScene(int zone, bool bases, int foeX, int foeY, int meX, int meY) {
  v2BtlFoe[0] = foeX, v2BtlFoe[1] = foeY, v2BtlMe[0] = meX, v2BtlMe[1] = meY;
  int key = (zone + 1) * 8 + v2HourTod() * 2 + (bases ? 1 : 0);
  v2Cached(200 + key, scBattle, key);
}

static void v2TiltScene() { uint8_t t = v2HourTod(); v2Cached(100 + t, scTilt, t); }
static void v2CourtScene() { uint8_t t = v2HourTod(); v2Cached(108 + t, scCourt, t); }
static void v2BurstScene(bool pink) { v2Cached(pink ? 113 : 112, scBurst, pink ? 1 : 0); }

// PokeDungeon's tiles (dn_tile in rooms_px.py): flagstones lit along the top and left and shaded on the
// bottom and right, now and then a crack or moss; walls dark on top with specks and, where floor lies under
// them, a face of bricks lit along its top, a few with a flickering torch; the stairs as a stairwell going down to
// the warm, flickering light of the floor below (Fedor: in the dungeon's look, and standing out).
static void v2DnTile(uint8_t t, bool face, int tx, int ty, int sx, int sy) {
  scScreen();
  auto rr = [](int x0, int y0, int x1, int y1, Rgb c) { scRect(x0, y0, x1 - 1, y1 - 1, c); };  // x1 and y1 outside it
  const int T = DN_TILE;
  if (t == DN_WALL) {
    rr(sx, sy, sx + T, sy + T, 0x24202C);
    for (int k = 0; k < 3; k++) {
      int x = sx + (tx * 17 + k * 23) % 56 / 4 * 4, y = sy + (ty * 13 + k * 19) % 40 / 4 * 4;
      rr(x, y, x + 4, y + 4, 0x2E2A38);
    }
    if (!face) return;
    rr(sx, sy + 40, sx + T, sy + T, 0x4A4252);
    rr(sx, sy + 40, sx + T, sy + 44, 0x5E5468);
    static const int8_t course[2][3] = { { 48, 0, 8 }, { 56, 12, 6 } };  // the mortar: y, where the joints start, their height
    for (auto &c : course) {
      rr(sx, sy + c[0], sx + T, sy + c[0] + 2, 0x3A3442);
      for (int bx = c[1]; bx < T; bx += 24) rr(sx + bx, sy + c[0] - c[2], sx + bx + 2, sy + c[0], 0x3A3442);
    }
    if ((tx * 3 + ty * 5) % 7 == 0) {  // a torch on this face, its flame a little shorter every other beat
      int x = sx + 28, y = sy + 44, low = ((millis() + (tx * 7 + ty) * 97) / 170) % 2 ? 4 : 0;
      rr(x, y, x + 8, y + 16, 0x6A4428);
      rr(x - 4, y - 12 + low, x + 12, y, 0xF5863D);
      rr(x, y - 16 + low, x + 8, y - 4, 0xFFD95E);
    }
    return;
  }
  rr(sx, sy, sx + T, sy + T, 0x62584C);
  rr(sx, sy, sx + T, sy + 4, 0x7A6E60);
  rr(sx, sy, sx + 4, sy + T, 0x7A6E60);
  rr(sx + T - 4, sy, sx + T, sy + T, 0x4E463C);
  rr(sx, sy + T - 4, sx + T, sy + T, 0x4E463C);
  int h = (tx * 7 + ty * 13) % 5;
  if (h == 0) {  // a crack
    rr(sx + 20, sy + 28, sx + 36, sy + 32, 0x4E463C);
    rr(sx + 32, sy + 32, sx + 36, sy + 40, 0x4E463C);
  }
  if (h == 2) {  // moss
    rr(sx + 40, sy + 16, sx + 48, sy + 20, 0x6E8A4A);
    rr(sx + 44, sy + 12, sx + 48, sy + 16, 0x8AA860);
  }
  if (t == DN_STAIRS) {  // the way down, in the dungeon's own look and standing out (Fedor): a stairwell with a lip
                         // of the walls' stone, four steps going down into it, narrower and warmer as they go, and
                         // the light of the floor below rising from it, flickering as the torches do
    bool hi = ((millis() + (tx * 7 + ty * 3) * 61) / 170) % 2;
    rr(sx + 4, sy + 4, sx + 60, sy + 60, 0x4A4252);   // the lip
    rr(sx + 4, sy + 4, sx + 60, sy + 8, 0x5E5468);    // lit along its top
    rr(sx + 56, sy + 8, sx + 60, sy + 60, 0x3A3442);  // its right side in shade
    rr(sx + 8, sy + 56, sx + 56, sy + 60, hi ? 0x8A5A38 : 0x6E4A34);  // its near edge, lit from the stairwell
    rr(sx + 8, sy + 8, sx + 56, sy + 56, 0x16121C);   // the stairwell
    rr(sx + 12, sy + 40, sx + 52, sy + 56, hi ? 0xF5863D : 0xD66F2E);  // the light from below
    rr(sx + 20, sy + 44, sx + 44, sy + 56, hi ? 0xFFD95E : 0xF5B04A);
    rr(sx + 28, sy + 48, sx + 36, sy + 56, hi ? 0xFFF1B0 : 0xFFD95E);
    static const Rgb TREAD[4] = { 0x9A8E7E, 0xA88C68, 0xBC8C52, 0xD49442 }, RISER[4] = { 0x5E5448, 0x6C5442, 0x80583A, 0x985E30 };
    for (int i = 0; i < 4; i++) {  // each a tread and its riser
      int x0 = sx + 8 + i * 4, x1 = sx + 56 - i * 4, y = sy + 8 + i * 8;
      rr(x0, y, x1, y + 4, TREAD[i]);
      rr(x0, y + 4, x1, y + 8, RISER[i]);
    }
  }
}

// the dungeon's floor and HP in one dark panel at the top (the approved mockup, with its numbers at size 2)
static void v2DnHud(uint8_t floor, int hp, int maxHp) {
  gfx->fillRoundRect(131, 15, 205, 59, 10, C565(0x0d, 0x0b, 0x20));
  gfx->fillRoundRect(134, 18, 199, 53, 8, C565(0x1e, 0x20, 0x34));
  gfx->drawRoundRect(137, 21, 193, 47, 5, C565(0x5a, 0x5f, 0x7c));
  gfx->drawRoundRect(138, 22, 191, 45, 4, C565(0x5a, 0x5f, 0x7c));
  char b[16];
  snprintf(b, sizeof(b), "B%uF", floor);
  setSize(2);
  gfx->setTextColor(C565(0xf0, 0xc0, 0x4a));
  setCur(148, 26);
  printT(b);
  int hx = 148 + textW(b, 2) + 12;
  if (hx < 200) hx = 200;
  gfx->setTextColor(UI_WHITE);
  setCur(hx, 26);
  printT("HP");
  int bx = hx + 28, bw = 318 - bx;  // the bar, to the panel's right
  gfx->fillRect(bx, 28, bw, 12, C565(0x0d, 0x0b, 0x20));
  float pct = maxHp ? (float)hp / maxHp : 0;
  int w = (int)((bw - 4) * pct);
  if (w > 0) gfx->fillRect(bx + 2, 30, w, 8, pct > 0.5f ? UI_BAR_OK : (pct > 0.2f ? UI_BAR_WARN : UI_BAR_BAD));
  snprintf(b, sizeof(b), "%d/%d", hp, maxHp);
  gfx->setTextColor(C565(0xd8, 0xdc, 0xf0));
  setCur(bx + bw / 2 - textW(b, 2) / 2, 48);
  printT(b);
}

// a V2 button: a dark rim, its colour, a lit stripe along the top (button() in the mockups)
static void v2Button(int x, int y, int w, int h, uint16_t col) {
  gfx->fillRoundRect(x - 3, y - 3, w + 6, h + 6, 10, V2_RIM);
  gfx->fillRoundRect(x, y, w, h, 8, col);
  gfx->fillRect(x + 6, y + 3, w - 12, 4, v2Blend(col, UI_WHITE, 90));  // 35% lighter
}

// Berry Drop's lives: a small heart on the 2 px grid (18 x 16, centred on cx, cy), red while the life is
// left, grey once it's gone
static const char *const V2_LIFE_HEART[8] = {
  "..kk.kk..", ".krrkrrk.", "krwrrrrrk", "krrrrrrRk", ".krrrrRk.", "..krrRk..", "...krk...", "....k....",
};
static void v2LifeHeart(int cx, int cy, bool full) {
  int x0 = SNAP2(cx - 9), y0 = SNAP2(cy - 8);
  for (int r = 0; r < 8; r++)
    for (int c = 0; V2_LIFE_HEART[r][c]; c++) {
      char ch = V2_LIFE_HEART[r][c];
      if (ch == '.') continue;
      uint16_t col = ch == 'k'   ? UI_INK
                     : !full     ? C565(0xa9, 0xb2, 0xbe)
                     : ch == 'w' ? UI_WHITE
                     : ch == 'R' ? C565(0xb5, 0x32, 0x24)
                                 : UI_BAR_BAD;
      gfx->fillRect(x0 + c * 2, y0 + r * 2, 2, 2, col);
    }
}

// a line of text over a scene (the battle has no band under it now): in its own text box
static bool gV2LineBox = false;  // battleCenterLine boxes its lines while this is on
static void v2LineBox(const char *s, int y) {
  int w = textW(s, 2) + 28;
  v2Panel(CX - w / 2, y - 7, CX + w / 2, y + 20, 8);
}

#ifdef BOARD_HOST
// For the screenshot tool: one background on its own at an hour (0 day, 1 dusk, 2 night), to set beside
// its mockup. 0 Berry Drop, 1 PokeRun, 2 PikaSurf, 3 Volley, 4 Who's That?, 5 Showdown, 6 the wild battle,
// 7 the Forest battle, 8 the Field's catch, 9 a dungeon room.
void v2HostBackdrop(int which, int tod) {
  scScreen();
  v2SceneKey = -1;
  scGradeTod = 0;
  switch (which) {
    case 0: scTilt(tod); break;
    case 1: scRunSky(tod); scRunLand(0, tod); break;
    case 2: v2SurfWater(0, tod); break;
    case 3: scCourt(tod); break;
    case 4: scBurst(0); break;
    case 5: scBurst(1); break;
    case 6: v2BtlFoe[0] = 330, v2BtlFoe[1] = 186, v2BtlMe[0] = 128, v2BtlMe[1] = 322; scBattle(0 * 8 + tod * 2 + 1); break;
    case 7: v2BtlFoe[0] = 330, v2BtlFoe[1] = 186, v2BtlMe[0] = 128, v2BtlMe[1] = 322; scBattle(1 * 8 + tod * 2 + 1); break;
    case 8: scBattle((Z_FIELD + 1) * 8 + tod * 2); break;
    case 9: {
      static const char *const ROOM[8] = { "#########", "#...#...#", "#.......#", "#..##...#", "#.....>.#", "##.#....#", "#...#...#", "#########" };
      scRect(0, 0, V2_W, V2_W, 0x24202C);
      int ox = 233 - 32 - 3 * 64, oy = 233 - 32 - 3 * 64 + 16;
      for (int ty = 0; ty < 8; ty++)
        for (int tx = 0; tx < 9; tx++) {
          char c = ROOM[ty][tx];
          bool face = c == '#' && ty + 1 < 8 && ROOM[ty + 1][tx] != '#';
          v2DnTile(c == '#' ? DN_WALL : c == '>' ? DN_STAIRS : DN_FLOOR, face, tx, ty, ox + tx * 64, oy + ty * 64);
        }
      break;
    }
  }
  scGradeTod = 0;
}
#endif

// The poops on the grass (Fedor: V2's own, not V1's): three coils and a curled tip, outlined like the icons, lit
// from the upper left, at 2x
static const char *const SPR_V2_POOP[18] = {
  "..................",
  ".........kkk......",
  "........khwCk.....",
  ".......kkwcCk.....",
  ".....kkwccCkk.....",
  "....khwcccccCk....",
  "....kcccccccCCk...",
  "....kkCCCCCCCk....",
  "...khwccccccCCk...",
  "..kwwcccccccccCk..",
  "..kcccccccccCCCk..",
  ".kkCCCCCCCCCCCCkk.",
  "khwwccccccccccCCCk",
  "kwcccccccccccccCCk",
  ".kcccccccccccCCCk.",
  "..kkCCCCCCCCCCkk..",
  "....kkkkkkkkkk....",
  "..................",
};
static void v2PoopIcon(int x, int y) {  // its top left at (x, y)
  static const ScKey K[5] = { { 'k', 0x1B1B25 }, { 'c', 0x9A6034 }, { 'C', 0x6B3F22 }, { 'w', 0xC88B52 }, { 'h', 0xE8C08A } };
  scScreen();
  scGradeTod = 0;
  scSprite(SPR_V2_POOP, 18, x, y, 2, K, 5);
}

#include "walk_v2.h"   // step 4: the walk
#include "menus_v2.h"  // step 5: the menus
#include "starter_v2.h"  // step 6: the starter screen
#include "master_v2.h"   // step 7: fights pay, the Master Ball
#include "repel_v2.h"    // the Repel
#include "radial_v2.h"   // the radial menu: a hold on the Pokemon
