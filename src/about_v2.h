// V2: Settings' first page tidied, and the About page, its last (scene_v2.h includes this at its end, so it's only
// there with UI_V2 on). Fedor: "We also need to make an about page with my logo on it but in a pixel art style to
// match the game. Maybe a way to get to the repo but also my youtube channel. We can move the version to this area
// too. Will give us room to make the clock, sound, Language, and start over screen look better. It seems all over the
// place right now."
#include "about_logo.h"  // the 3D Print SOS logo as pixel art (tools/about_assets.py)
#include "about_qr.h"    // the two links as QR codes, for the Knomi
#include "brand_logo.h"  // the game's logo as pixel art (tools/brand_assets.py)

#ifdef BOARD_HOST
bool hostOpenUrl(const char *url);  // a computer, the web: in the browser (false: nothing to open it with)
#endif

// ----- the settings page: the time, where it comes from (the Knomi: its time zone, a tap to change it), then a row
// each for the sound (a switch) and the language (its own name, a tap for the next), START OVER under the panel -----
// each language by its own name, in its own letters
static const char *const V2_LANG_NAMES[LANG_COUNT] = { "Espa\244ol", "English", "Fran\207ais", "Deutsch", "Italiano",
                                                       "Portugu\210s", "日本語", "한국어" };
#define V2_SET_TZ_Y0 158     // the time zone's button, under the time (the Knomi)
#define V2_SET_TZ_Y1 192
#define V2_SET_LINE_Y 204    // a line between the time and the rows
#define V2_SET_ROW_X0 92     // the rows: a tap anywhere on one is its control's
#define V2_SET_ROW_X1 374
#if HAS_MUSIC  // a computer and the web: a row more, for the music, the rows closer and no line over them
#define V2_SND_Y 190         // the Sound row: its switch's top (72 x 36)
#define V2_MUS_Y 232         // the Music row: its switch's
#define V2_LANG_Y 274        // the Language row: its button's top
#else
#define V2_SND_Y 214         // the Sound row: its switch's top (72 x 36)
#define V2_LANG_Y 262        // the Language row: its button's top
#endif
#define V2_SND_SW_X 292
#define V2_LANG_BTN_X0 210
#define V2_LANG_BTN_X1 370
#define V2_SET_ROW_H 36

#ifdef BOARD_KNOMI_V2  // (the time zones are the Knomi's, and a computer's: the Waveshare has none)
static void v2SetTzRect(int &x0, int &x1, char *b, int n) {  // as wide as the city needs
  int i = clockTzIndex();
  snprintf(b, n, "%s >", TIME_ZONES[i >= 0 ? i : TIME_ZONE_DEFAULT].city);
  setSize(2);
  int w = textW(b, 2) + 48;
  if (w < 180) w = 180;
  x0 = CX - w / 2;
  x1 = CX + w / 2;
}
#endif
#if HAS_MUSIC
// ----- a level slider (a computer and the web: SOUND's, all of it, and the music's): a track with four notches, 0 off,
// then 1, 2 and 3, and a knob showing its number (Fedor: "Maybe a slider with 4 notches. From 0 to 3"). A tap on a
// notch moves it there; a finger sliding along it moves the choice, the press preview showing the number, and it
// lands where the finger lifts. Off, it comes back on at the level it had
#define V2_SL_X0 212   // the first notch (0); each 48 further
#define V2_SL_STEP 48
#define V2_SL_R 14     // the knob
static const char *const V2_SL_NUM[4] = { "0", "1", "2", "3" };

static void v2Slider(int y, uint8_t lv) {
  int cy = y + V2_SET_ROW_H / 2, kx = V2_SL_X0 + V2_SL_STEP * lv;
  const int x0 = V2_SL_X0 - 8, x1 = V2_SL_X0 + 3 * V2_SL_STEP + 8;
  gfx->fillRoundRect(x0, cy - 7, x1 - x0, 14, 7, C565(0x3a, 0x4a, 0x5a));
  gfx->fillRoundRect(x0 + 2, cy - 5, x1 - x0 - 4, 10, 5, C565(0xd8, 0xd2, 0xbd));
  if (lv) gfx->fillRoundRect(x0 + 2, cy - 5, kx - x0 - 2, 10, 5, C565(0x58, 0xb8, 0x68));
  for (int i = 0; i < 4; i++)
    gfx->fillCircle(V2_SL_X0 + V2_SL_STEP * i, cy, 3, i <= lv && lv ? C565(0x3c, 0x8a, 0x4c) : C565(0xb0, 0xa8, 0x94));
  V2_HOVER(kx - 22, y, 44, V2_SET_ROW_H);
  gfx->fillCircle(kx, cy + 2, V2_SL_R, lv ? C565(0x3c, 0x8a, 0x4c) : C565(0xb0, 0xa8, 0x94));  // (its shadow)
  gfx->fillCircle(kx, cy, V2_SL_R, C565(0x3a, 0x4a, 0x5a));
  gfx->fillCircle(kx, cy, V2_SL_R - 2, UI_WHITE);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(kx - textW(V2_SL_NUM[lv], 2) / 2 + 2, cy - 6);  // (the digit's ink in the middle of the knob)
  printT(V2_SL_NUM[lv]);
}
// the notch at x (-1: none: the row's name)
static int v2SliderAt(int16_t x) {
  int i = (x - V2_SL_X0 + V2_SL_STEP / 2) / V2_SL_STEP;
  return x < V2_SL_X0 - V2_SL_STEP / 2 || i < 0 ? -1 : i > 3 ? 3 : i;
}
// ...its press preview: the notch, and its number
static bool v2SliderPress(int16_t x, int y, PressTarget &t) {
  int i = v2SliderAt(x);
  return i >= 0 && v2PressSet(t, V2_SL_X0 + V2_SL_STEP * i - 22, y, 44, V2_SET_ROW_H, V2_SL_NUM[i]);
}
#endif
static void v2SetLabel(const char *s, int y) {
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(V2_SET_ROW_X0 + 8, y + V2_SET_ROW_H / 2 - 7);
  printT(s);
}
// below the time (v2Clock() draws the panel and the time)
static void v2SettingsRows() {
  char b[40];
  setSize(2);
#ifdef BOARD_KNOMI_V2
  if (clockTzUi) {
    int x0, x1;
    v2SetTzRect(x0, x1, b, sizeof(b));
    V2_HOVER(x0, V2_SET_TZ_Y0, x1 - x0, V2_SET_TZ_Y1 - V2_SET_TZ_Y0);
    v2LabelButton(x0, V2_SET_TZ_Y0, x1, V2_SET_TZ_Y1, b, C565(0x4f, 0x93, 0xc4));
  } else
#endif
  {
    gfx->setTextColor(C565(0x3c, 0x8a, 0x4c));
#if HAS_MUSIC
    setCur(centerX(T(S_SET_AUTO), 2), V2_SET_TZ_Y0 + 8);
#else
    setCur(centerX(T(S_SET_AUTO), 2), V2_SET_TZ_Y0 + 9);
#endif
    printT(T(S_SET_AUTO));
  }
#if !HAS_MUSIC
  gfx->fillRect(V2_SET_ROW_X0 + 2, V2_SET_LINE_Y, V2_SET_ROW_X1 - V2_SET_ROW_X0 - 4, 2, C565(0xd8, 0xd2, 0xbd));
#endif
  v2SetLabel(T(S_SOUND), V2_SND_Y);
#if HAS_MUSIC
  v2Slider(V2_SND_Y, audioEnabled() ? soundVolume() : 0);
  v2SetLabel(T(S_MUSIC), V2_MUS_Y);
  v2Slider(V2_MUS_Y, musicEnabled() ? musicVolume() : 0);
#else
  {
    V2_HOVER(V2_SND_SW_X, V2_SND_Y, 72, V2_SET_ROW_H);
    v2Switch(V2_SND_SW_X, V2_SND_Y, audioEnabled());
  }
#endif
  v2SetLabel(T(S_LANGUAGE), V2_LANG_Y);
  snprintf(b, sizeof(b), "%s >", V2_LANG_NAMES[gLang]);
  V2_HOVER(V2_LANG_BTN_X0, V2_LANG_Y, V2_LANG_BTN_X1 - V2_LANG_BTN_X0, V2_SET_ROW_H);
  v2LabelButton(V2_LANG_BTN_X0, V2_LANG_Y, V2_LANG_BTN_X1, V2_LANG_Y + V2_SET_ROW_H, b, C565(0x4f, 0x93, 0xc4));
}
static bool v2InRow(int16_t x, int16_t y, int y0) {
  return x >= V2_SET_ROW_X0 && x <= V2_SET_ROW_X1 && y >= y0 - 6 && y <= y0 + V2_SET_ROW_H + 6;
}
// a tap on the page (its X and START OVER have been asked first): every tap here is the page's
static void v2SettingsTap(int16_t x, int16_t y) {
#ifdef BOARD_KNOMI_V2
  if (clockTzUi && y >= V2_SET_TZ_Y0 - 6 && y <= V2_SET_TZ_Y1 + 6) {
    char b[40];
    int x0, x1;
    v2SetTzRect(x0, x1, b, sizeof(b));
    if (x >= x0 - 6 && x <= x1 + 6) {
      sfxPlay(SFX_TAP);
      tzPickerOpen();
    }
    return;
  }
#endif
#if HAS_MUSIC
  if (v2InRow(x, y, V2_SND_Y)) {  // (the tap sounds at the level it sets)
    int k = v2SliderAt(x);
    if (k == 0) audioSetEnabled(false);
    else if (k > 0) soundSetVolume((uint8_t)k);
    if (k >= 0 && audioEnabled()) sfxPlay(SFX_TAP);
    return;
  }
  if (v2InRow(x, y, V2_MUS_Y)) {
    int k = v2SliderAt(x);
    if (k == 0) musicSetEnabled(false);
    else if (k > 0) musicSetVolume((uint8_t)k);
    if (k >= 0) sfxPlay(SFX_TAP);
    return;
  }
#else
  if (v2InRow(x, y, V2_SND_Y)) {
    audioSetEnabled(!audioEnabled());
    if (audioEnabled()) sfxPlay(SFX_TAP);  // confirms when turned on
    return;
  }
#endif
  if (v2InRow(x, y, V2_LANG_Y)) {
    setLang((Lang)((gLang + 1) % LANG_COUNT));
    applyLangFont();  // the font changes with the language
    sfxPlay(SFX_TAP);
  }
}
// the press preview's (press.h)
static bool v2SettingsPress(int16_t x, int16_t y, PressTarget &t) {
  static char b[40];
#ifdef BOARD_KNOMI_V2
  if (clockTzUi && y >= V2_SET_TZ_Y0 - 6 && y <= V2_SET_TZ_Y1 + 6) {
    int x0, x1;
    v2SetTzRect(x0, x1, b, sizeof(b));
    return x >= x0 - 6 && x <= x1 + 6 && v2PressSet(t, x0, V2_SET_TZ_Y0, x1 - x0, V2_SET_TZ_Y1 - V2_SET_TZ_Y0, b);
  }
#endif
#if HAS_MUSIC
  if (v2InRow(x, y, V2_SND_Y)) return v2SliderPress(x, V2_SND_Y, t);
  if (v2InRow(x, y, V2_MUS_Y)) return v2SliderPress(x, V2_MUS_Y, t);
#else
  if (v2InRow(x, y, V2_SND_Y)) return v2PressSet(t, V2_SND_SW_X, V2_SND_Y, 72, V2_SET_ROW_H, T(S_SOUND));
#endif
  if (v2InRow(x, y, V2_LANG_Y)) {
    snprintf(b, sizeof(b), "%s >", V2_LANG_NAMES[gLang]);
    return v2PressSet(t, V2_LANG_BTN_X0, V2_LANG_Y, V2_LANG_BTN_X1 - V2_LANG_BTN_X0, V2_SET_ROW_H, b);
  }
#ifdef BOARD_KNOMI_V2  // START OVER (it asks with the Knomi's own question)
  if (x >= V2_RESET_X0 && x <= V2_RESET_X1 && y >= V2_RESET_Y0 && y <= V2_RESET_Y1)
    return v2PressSet(t, V2_RESET_X0, V2_RESET_Y0, V2_RESET_X1 - V2_RESET_X0, V2_RESET_Y1 - V2_RESET_Y0, T(S_RESET_BTN));
#endif
  return false;
}

// ----- the coin: the logo spinning like one (Fedor: "Can is spin around like a coin? with a gold gleam?"). It spins
// two turns, quick then slowing, its milled gold edge showing as it turns and the light catching it each time it
// faces you; as it lands a gold gleam sweeps across and a sparkle twinkles, and the gleam comes back every 6 s. It
// spins each time the page opens, and on a tap (Fedor: "If it reacts to touch, then it has to spin for the first time
// on load to hint that"). Drawn a logo pixel at a time, 2x2 on the screen, straight into the frame -----
#define V2_COIN_Y 62            // its top (the logo at 2x: ABOUT_LOGO_N, 72)
#define V2_COIN_PI 3.14159265f   // (M_PI isn't everywhere: Windows' compiler)
#define V2_COIN_SPIN_MS 1500
#define V2_COIN_GLEAM_MS 640
#define V2_COIN_GLEAM_EVERY 6000
static uint32_t v2CoinSpinT = 0;  // when its spin began
static uint32_t v2CoinSeenT = 0;  // the last frame it was drawn: a gap, and the page has just opened
static uint8_t v2AboutQr = 0;     // a link's QR code over the page (the Knomi): 1 GitHub, 2 YouTube
static bool v2AboutQrOpen() { return v2AboutQr != 0; }

static inline void v2CoinPx(uint16_t *fb, int x, int y, uint16_t c) {  // a logo pixel: 2x2 on the screen
  if (x < 0 || y < 0 || x + 1 >= V2_W || y + 1 >= V2_W) return;
  uint16_t *p = fb + y * V2_W + x;
  p[0] = p[1] = p[V2_W] = p[V2_W + 1] = c;
}
static inline uint16_t v2CoinMix(uint16_t c, int r2, int g2, int b2, float a) {  // c toward (r2, g2, b2) by a
  int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
  r += (int)((r2 - r) * a);
  g += (int)((g2 - g) * a);
  b += (int)((b2 - b) * a);
  return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}
static void v2CoinSparkle(uint16_t *fb, int sx, int sy, int size) {  // a four-pointed star: 2 big, 1 small
  if (!size) return;
  int arm = size > 1 ? 4 : 2;
  for (int d = -arm; d <= arm; d++) {
    bool mid = d >= -1 && d <= 1;
    v2CoinPx(fb, sx + 2 * d, sy, mid ? sc565(0xFFFFF0) : sc565(0xFFE282));
    v2CoinPx(fb, sx, sy + 2 * d, mid ? sc565(0xFFFFF0) : sc565(0xFFE282));
  }
  if (size > 1)
    for (int d = -1; d <= 1; d += 2) {
      v2CoinPx(fb, sx + 2 * d, sy + 2 * d, sc565(0xFFFFF0));
      v2CoinPx(fb, sx + 2 * d, sy - 2 * d, sc565(0xFFFFF0));
    }
}
static void v2Coin(uint32_t now) {
  if (now - v2CoinSeenT > 300) {  // just opened: it spins (a hint it can be spun), no link's code over it
    v2CoinSpinT = now;
    v2AboutQr = 0;
  }
  v2CoinSeenT = now;
  const int N = ABOUT_LOGO_N;
  const float cl = (N - 1) / 2.0f, R = N * 0.369f + 5;  // the logo's middle; the disc to the rope's edge
  float theta = 0, flash = 0, gleam = -1000;
  int spark = 0, spark2 = 0;
  uint32_t t = now - v2CoinSpinT;
  if (t < V2_COIN_SPIN_MS) {  // two turns, slowing (ease out)
    float p = t / (float)V2_COIN_SPIN_MS, e = 1 - (1 - p) * (1 - p) * (1 - p);
    theta = e * 4 * V2_COIN_PI;
    float ph = fmodf(theta, V2_COIN_PI) / V2_COIN_PI, m = ph < 1 - ph ? ph : 1 - ph;
    flash = 0.35f - 3.0f * m;  // facing you: the light catches it
    if (flash < 0) flash = 0;
  } else {  // landed: the gleam now, then every 6 s
    uint32_t g = (t - V2_COIN_SPIN_MS) % V2_COIN_GLEAM_EVERY;
    if (g < V2_COIN_GLEAM_MS) {
      int i = g * 16 / V2_COIN_GLEAM_MS;
      gleam = -64 + i * 8.5f;
      spark = i >= 6 && i <= 9 ? 2 : (i == 5 || i == 10) ? 1 : 0;
      spark2 = i == 12 || i == 13;
    }
  }
  float c = cosf(theta), s = sinf(theta), k = fabsf(c), T = 5 * fabsf(s);
  int side = s * c >= 0 ? 1 : -1;  // which side its edge shows on
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const int x0 = CX - N, y0 = V2_COIN_Y;  // the logo's top left on the screen (2x)
  const uint16_t eD = sc565(0xB08630), eL = sc565(0xF2C858), eK = sc565(0x5A3E12);  // its edge: milled gold, outlined
  // the edge: the disc's outline squashed, T px deep to one side
  if (T >= 1)
    for (int y = 0; y < N; y++) {
      float dy = y - cl;
      if (fabsf(dy) > R) continue;
      float half = sqrtf(R * R - dy * dy) * k;
      int a = (int)lroundf(cl + side * half), b = (int)lroundf(cl + side * (half + T));
      int lo = a < b ? a : b, hi = a < b ? b : a;
      for (int x = lo; x <= hi; x++) {
        bool rim = x == b || fabsf(dy) > R - 1.5f;
        v2CoinPx(fb, x0 + 2 * x, y0 + 2 * y, rim ? eK : ((y / 2) % 2 ? eL : eD));
      }
    }
  // the face: each column the logo's, squashed (both sides show it the right way round), a little darker turned away
  if (k > 0.02f) {
    float dark = 0.75f + 0.25f * k;
    for (int x = -8; x < N + 8; x++) {
      float u = (x - cl) / k;
      int sx = (int)lroundf(cl + u);
      if (sx < 0 || sx >= N) continue;
      for (int y = 0; y < N; y++) {
        uint8_t i = ABOUT_LOGO[y * N + sx];
        if (!i) continue;
        uint16_t px = ABOUT_LOGO_PAL[i];
        if (gleam > -999) {  // a slanted band of light: white-hot in the middle, gold either side
          float d = u + (y - cl) * 0.55f - gleam, ad = fabsf(d);
          if (ad <= 1.5f) px = v2CoinMix(px, 255, 252, 232, 0.85f);
          else if (ad <= 6) px = v2CoinMix(px, 255, 214, 96, 0.55f * (1 - (ad - 1.5f) / 5));
        }
        if (flash > 0) px = v2CoinMix(px, 255, 230, 150, flash);
        if (dark < 0.99f) px = v2CoinMix(px, 0, 0, 0, 1 - dark);
        v2CoinPx(fb, x0 + 2 * x, y0 + 2 * y, px);
      }
    }
  }
  v2CoinSparkle(fb, x0 + 2 * (N - 14), y0 + 2 * 12, spark);  // upper right of the rope
  v2CoinSparkle(fb, x0 + 2 * 12, y0 + 2 * (N - 22), spark2);  // and a small one lower left, after
}
static bool v2CoinHit(int16_t x, int16_t y) {
  int dx = x - CX, dy = y - (V2_COIN_Y + ABOUT_LOGO_N);
  return dx * dx + dy * dy <= 92 * 92;
}

// ----- the About page: the coin, then a panel with the game's logo and its version, who made it, and the links: on a
// computer or the web they open in the browser; the Knomi can't, so it shows the link's QR code for a phone to scan.
// Fedor picked the logo in the panel, the panel "a bit bigger", the who-made-it line "a bit less wide", then "the
// 3DPrintSOS coin a bit smaller and our new logo larger": the coin 72 px (it was 96), the logo drawn larger -----
#define V2_ABOUT_PANEL_X0 66   // (between the page arrows: they end at x 62 and start at 404, down to y 254)
#define V2_ABOUT_PANEL_X1 400
#define V2_ABOUT_PANEL_Y0 212  // (just under the coin)
#define V2_ABOUT_PANEL_Y1 374  // (just over "Page 2 of 2")
#define V2_ABOUT_LOGO_Y 220    // the logo's top: 2 px a logo pixel
#define V2_ABOUT_VER_Y (V2_ABOUT_LOGO_Y + 2 * BRAND_LOGO_H + 8)  // the version, under it
#define V2_ABOUT_MADE_Y (V2_ABOUT_VER_Y + 18)
#define V2_ABOUT_BTN_Y0 330
#define V2_ABOUT_BTN_Y1 366
// the game's logo (tools/brand_assets.py), s screen pixels a logo pixel, its middle at cx and its top at y: each row's
// runs of one colour, one rectangle each
static void v2BrandLogo(int cx, int y, int s) {
  int x0 = SNAP2(cx - BRAND_LOGO_W * s / 2);
  y = SNAP2(y);
  for (int j = 0; j < BRAND_LOGO_H; j++) {
    const uint8_t *r = BRAND_LOGO + j * BRAND_LOGO_W;
    for (int i = 0; i < BRAND_LOGO_W;) {
      int k = i + 1;
      while (k < BRAND_LOGO_W && r[k] == r[i]) k++;
      if (r[i]) gfx->fillRect(x0 + i * s, y + j * s, (k - i) * s, s, BRAND_LOGO_PAL[r[i]]);
      i = k;
    }
  }
}
#define V2_GH_X0 100  // GITHUB
#define V2_GH_X1 228
#define V2_YT_X0 238  // YOUTUBE
#define V2_YT_X1 366
static const uint16_t V2_GH_COL = C565(0x2f, 0x35, 0x3c), V2_YT_COL = C565(0xe0, 0x2a, 0x22);
static void v2AboutQrDraw() {
  dimFrame();
  bool yt = v2AboutQr == 2;
  int n = yt ? QR_YOUTUBE_N : QR_GITHUB_N, s = 6, w = n * s, x0 = CX - w / 2, y0 = 116;  // 4 modules clear all round
  const char *const *q = yt ? QR_YOUTUBE : QR_GITHUB;
  v2Panel(CX - 150, 58, CX + 150, y0 + w + 56, 12);
  setSize(2);
  gfx->setTextColor(UI_INK);
  const char *title = yt ? "YouTube: 3D Print SOS" : "GitHub: PokeKnomiGochi";
  setCur(centerX(title, 2), 74);
  printT(title);
  for (int r = 0; r < n; r++)
    for (int c = 0; c < n; c++)
      if (q[r][c] == '1') gfx->fillRect(x0 + c * s, y0 + r * s, s, s, C565(0x1b, 0x1b, 0x25));
  gfx->setTextColor(V2_INK_MUTED);
  setCur(centerX(T(S_SCAN_PHONE), 2), y0 + w + 26);
  printT(T(S_SCAN_PHONE));
  v2CloseX();
}
static void v2About() {
  uint32_t now = millis();
  v2MenuBackdrop();
  char sign[32];
  v2Sign(v2Caps(T(S_ABOUT), sign, sizeof(sign)));
  v2Coin(now);
  v2Panel(V2_ABOUT_PANEL_X0, V2_ABOUT_PANEL_Y0, V2_ABOUT_PANEL_X1, V2_ABOUT_PANEL_Y1, 10);
  setSize(2);
  const char *v = "v" FW_VERSION;
  v2BrandLogo(CX, V2_ABOUT_LOGO_Y, 2);
  gfx->setTextColor(V2_INK_MUTED);
  setCur(centerX(v, 2), V2_ABOUT_VER_Y);
  printT(v);
  setCur(centerX(T(S_MADE_BY), 2), V2_ABOUT_MADE_Y);
  printT(T(S_MADE_BY));
  {
    V2_HOVER(V2_GH_X0, V2_ABOUT_BTN_Y0, V2_GH_X1 - V2_GH_X0, V2_ABOUT_BTN_Y1 - V2_ABOUT_BTN_Y0);
    v2LabelButton(V2_GH_X0, V2_ABOUT_BTN_Y0, V2_GH_X1, V2_ABOUT_BTN_Y1, "GITHUB", V2_GH_COL);
  }
  {
    V2_HOVER(V2_YT_X0, V2_ABOUT_BTN_Y0, V2_YT_X1 - V2_YT_X0, V2_ABOUT_BTN_Y1 - V2_ABOUT_BTN_Y0);
    v2LabelButton(V2_YT_X0, V2_ABOUT_BTN_Y0, V2_YT_X1, V2_ABOUT_BTN_Y1, "YOUTUBE", V2_YT_COL);
  }
  drawPageArrows(V2_ABOUT_PAGE, V2_CLOCK_PAGES, V2_PAGE_ARROW_Y);
  v2PageOf(V2_CLOCK_PAGES, V2_ABOUT_PAGE);
  v2CloseX();
  if (v2AboutQr) v2AboutQrDraw();
}
static int v2AboutLinkAt(int16_t x, int16_t y) {  // 1 GITHUB, 2 YOUTUBE, 0 neither
  if (y < V2_ABOUT_BTN_Y0 - 6 || y > V2_ABOUT_BTN_Y1 + 6) return 0;
  if (x >= V2_GH_X0 - 4 && x <= V2_GH_X1 + 4) return 1;
  if (x >= V2_YT_X0 - 4 && x <= V2_YT_X1 + 4) return 2;
  return 0;
}
// a tap: over a QR code, anywhere puts it away; the X closes Settings; the coin spins; a link opens (or its code)
static void v2AboutTap(int16_t x, int16_t y) {
  if (v2AboutQr) {
    sfxPlay(SFX_TAP);
    v2AboutQr = 0;
    return;
  }
  if (closeHit(x, y)) {
    sfxPlay(SFX_TAP);
    clockOpen = false;
    return;
  }
  if (v2CoinHit(x, y)) {
    sfxPlay(SFX_TAP);
    v2CoinSpinT = millis();
    return;
  }
  if (int k = v2AboutLinkAt(x, y)) {
    sfxPlay(SFX_TAP);
#ifdef BOARD_HOST
    if (hostOpenUrl(k == 1 ? ABOUT_URL_GITHUB : ABOUT_URL_YOUTUBE)) return;  // in the browser
#endif
    v2AboutQr = k;  // the Knomi: its code, for a phone
  }
}
static bool v2AboutPress(int16_t x, int16_t y, PressTarget &t) {
  if (v2AboutQr) return closeHit(x, y) && v2PressSet(t, CX - CLOSE_R, CLOSE_Y - CLOSE_R, 2 * CLOSE_R, 2 * CLOSE_R, "X");
  int k = v2AboutLinkAt(x, y);
  if (k == 1) return v2PressSet(t, V2_GH_X0, V2_ABOUT_BTN_Y0, V2_GH_X1 - V2_GH_X0, V2_ABOUT_BTN_Y1 - V2_ABOUT_BTN_Y0, "GITHUB");
  if (k == 2) return v2PressSet(t, V2_YT_X0, V2_ABOUT_BTN_Y0, V2_YT_X1 - V2_YT_X0, V2_ABOUT_BTN_Y1 - V2_ABOUT_BTN_Y0, "YOUTUBE");
  return false;
}
