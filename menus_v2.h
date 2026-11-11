// V2 step 5: the menus. scene_v2.h includes this at its end, so it's only there with UI_V2 on.
//
// Every menu sits on the bedroom with the lights down (rooms_px.py bedroom(), rooms2_px.py dim()), its window
// showing the hour's sky. On it: a wooden name board for the title (sign()), white panels (v2Panel), V2
// buttons, blue pixel page buttons at the sides with dots above the X (arrows(), dots()), and the red pixel X
// (close_x()), always at (233, 422). The screens keep their V1 tap areas: where a mockup and V1 disagree, the
// picture moves to fit.

#define V2_CLOSE_Y 422
#define V2_BOARD_Y 34
#define V2_INK_MUTED C565(0x6a, 0x6f, 0x84)  // the panels' second line
#define V2_ON_DARK C565(0xd8, 0xdc, 0xf0)    // text straight on the dimmed room

// ----- the bedroom -----
static void scRR(int x0, int y0, int x1, int y1, Rgb c) { scRect(x0, y0, x1 - 1, y1 - 1, c); }  // the mockups' RR()
static void scGridRect(int x0, int y0, int x1, int y1, Rgb c) { scRR(scFl4(x0) * 4, scFl4(y0) * 4, scFl4(x1) * 4, scFl4(y1) * 4, c); }

// the room as bedroom() draws it; the window shows the hour's sky (tod: 0 day, 1 dusk, 2 night)
static void v2Bedroom(int tod) {
  const Rgb WALL = 0xF0E2C0, WALL_S = 0xE4D2A8, WAIN = 0xC89A68, WAIN_D = 0xA87A4C, BASE = 0x6A4428;
  const Rgb PLANK = 0xC8955C, PLANK_D = 0xA87648, PLANK_L = 0xDCAE78, WOOD = 0x8A5A30, WOOD_L = 0xB07A45, WOOD_D = 0x6A4428;
  scRR(0, 0, V2_W, 300, WALL);
  for (int x = 0; x < V2_W; x += 24) scRR(x, 0, x + 8, 250, WALL_S);  // striped wallpaper
  scRR(0, 250, V2_W, 300, WAIN);                                        // the wainscot, with panels
  for (int x = 8; x < V2_W; x += 56) {
    scRect(x, 258, x + 44, 261, WAIN_D);
    scRect(x, 287, x + 44, 290, WAIN_D);
    scRect(x, 258, x + 3, 290, WAIN_D);
    scRect(x + 41, 258, x + 44, 290, WAIN_D);
  }
  scRR(0, 250, V2_W, 254, 0xDCB082);
  scRR(0, 296, V2_W, 304, BASE);
  scRR(0, 304, V2_W, V2_W, PLANK);  // the floor
  for (int i = 0, y = 304; y < V2_W; i++, y += 20) {
    scRR(0, y, V2_W, y + 4, PLANK_D);
    for (int x = (i % 2) * 60 - 20; x < V2_W; x += 120) scRR(x, y, x + 4, y + 20, PLANK_D);
  }
  scRR(0, 304, V2_W, 308, PLANK_L);
  scOval(scE(73, 334, 393, 414), 0xA83C3C);  // the rug
  scOval(scE(89, 342, 377, 406), 0xC85050);
  scOval(scE(121, 354, 345, 394), 0xE8A050);
  scOval(scE(137, 358, 329, 390), 0xC85050);
  // the window, the hour's sky through it
  static const Rgb SKY0[3] = { 0x8FC8EA, 0xC75A4A, 0x0C1224 }, SKY1[3] = { 0xDCEEE6, 0xF6B46A, 0x233058 };
  static const int16_t EDGES[4] = { 88, 124, 160, 194 };
  scRR(172, 82, 294, 200, WOOD_D);
  Rgb sky[3] = { SKY0[tod], scMix(SKY0[tod], SKY1[tod], .5), SKY1[tod] };
  scBands(sky, EDGES, 3, 180, 286);
  if (tod == 2) {
    static const int16_t STAR[4][2] = { {196, 100}, {250, 118}, {270, 96}, {210, 150} };
    for (auto &s : STAR) scGridRect(s[0], s[1], s[0] + 4, s[1] + 4, 0xFFFFFF);
  } else {
    scCloud(SC_DAY, 196, 124, 3);
  }
  scOval(scE(180, 174, 300, 230), 0x8CC794);  // hills outside
  scRR(172, 194, 294, 200, WOOD_D);
  scRR(230, 88, 236, 194, WOOD_D);
  scRR(180, 138, 286, 144, WOOD_D);
  scRR(164, 200, 302, 210, 0xDCB082);  // the sill
  scRR(164, 206, 302, 210, 0xA87A4C);
  static const int16_t CURTAIN[2][2] = { {150, 180}, {286, 316} };
  for (auto &c : CURTAIN) {  // curtains with folds
    scRR(c[0], 74, c[1], 214, 0x6AA0D8);
    for (int x = c[0] + 6; x < c[1]; x += 10) scRR(x, 74, x + 4, 214, 0x4F84C0);
  }
  scRR(140, 68, 326, 76, WOOD);
  // the bed, left
  scRR(24, 196, 40, 320, WOOD);
  scRR(24, 196, 40, 200, WOOD_L);
  scRR(30, 210, 150, 232, WOOD);
  scRR(40, 232, 160, 300, 0x4F93C4);  // the blanket
  scRR(40, 232, 160, 240, 0x7CC4EA);
  for (int x = 56; x < 160; x += 24) scRR(x, 248, x + 8, 256, 0x7CC4EA);
  scRR(44, 222, 92, 240, 0xFFFFFF);  // the pillow
  scRR(44, 236, 92, 240, 0xD6E0EA);
  scRR(40, 300, 160, 312, WOOD);
  scRR(150, 260, 164, 322, WOOD);
  // the TV, right
  scRR(316, 266, 444, 280, WOOD);
  scRR(316, 266, 444, 270, WOOD_L);
  scRR(322, 280, 330, 312, WOOD_D);
  scRR(430, 280, 438, 312, WOOD_D);
  scRR(332, 200, 420, 266, 0x8A8FA6);
  scRR(332, 200, 420, 204, 0xAAB0BC);
  scRR(340, 208, 404, 256, 0x2A3A52);
  scRR(344, 212, 356, 220, 0x5A7AA0);  // the screen's glare
  scRR(408, 214, 416, 222, 0xE8503A);
  scRR(408, 230, 416, 238, 0x58B868);
  scRR(368, 194, 372, 200, 0x6A6F84);  // the aerial
  scRR(380, 190, 384, 200, 0x6A6F84);
  static const char *const BALL[7] = { "..kkk..", ".krrrk.", "krlrrrk", "kkkwkkk", "kwwwwwk", ".kwwwk.", "..kkk.." };
  static const ScKey BALL_K[4] = { { 'k', 0x1B1B25 }, { 'r', 0xE8503A }, { 'l', 0xF6A08A }, { 'w', 0xFFFFFF } };
  scSprite(BALL, 7, 388, 172, 4, BALL_K, 4);  // a Poke Ball sitting on top of the TV (Fedor)
  // a plant by the bed, a shelf of books over it, a poster
  scRR(172, 268, 196, 304, 0xC86A3C);
  scRR(172, 268, 196, 272, 0xE08A5A);
  static const int16_t LEAF[5][2] = { {168, 248}, {184, 240}, {196, 252}, {176, 232}, {190, 226} };
  for (auto &l : LEAF) scOval(scE(l[0] - 10, l[1] - 10, l[0] + 10, l[1] + 10), 0x58B868, 0x8FD6B4);
  scRR(36, 140, 140, 148, WOOD);
  static const Rgb BOOK[5] = { 0xE8503A, 0x4F93C4, 0xF8D990, 0x58B868, 0xC75F80 };
  for (int i = 0, x = 42; x < 136; i++, x += 12) scRR(x, 108 + (i % 3) * 4, x + 10, 140, BOOK[i % 5]);
  scRR(334, 104, 410, 170, 0x3A2414);  // the poster: a mountain at dawn
  scRR(338, 108, 406, 166, 0x9FD6F0);
  for (int i = 0; i < 14; i++) scRR(372 - i * 2, 130 + i * 2, 372 + i * 2 + 4, 132 + i * 2, 0x6C7190);
  scRR(338, 156, 406, 166, 0x58B868);
  scRR(366, 128, 378, 134, 0xFFFFFF);
}
// the room with the lights down: everything 58% of the way to #120e22 (scGrade's mode 3)
static void v2PaintMenuBg(int tod) {
  scGradeTod = 3;
  v2Bedroom(tod);
  scGradeTod = 0;
}
// what every menu sits on, from the scene buffer
static void v2MenuBackdrop() {
  uint8_t tod = v2HourTod();
  v2Cached(800 + tod, v2PaintMenuBg, tod);
}

// ----- what sits on it -----
// a wooden name board for a screen's title, centred, its top at y (sign()), in its wood's colours
static int v2SignIn(const char *s, int y, uint16_t wood, uint16_t lit) {
  setSize(2);
  int w = textW(s, 2) + 28, x0 = CX - w / 2;
  const uint16_t dark = C565(0x3a, 0x24, 0x14);
  gfx->fillRect(x0 - 3, y - 3, w + 7, 31, dark);
  gfx->fillRect(x0, y, w + 1, 25, wood);
  gfx->fillRect(x0, y, w + 1, 4, lit);
  gfx->fillRect(x0 + 5, y + 10, 4, 4, dark);  // its nails
  gfx->fillRect(x0 + w - 8, y + 10, 4, 4, dark);
  gfx->setTextColor(C565(0xff, 0xf3, 0xdc));
  setCur(centerX(s, 2), y + 6);
  printT(s);
  return x0 + w + 3;  // its right edge
}
static int v2Sign(const char *s, int y = V2_BOARD_Y) { return v2SignIn(s, y, C565(0x8a, 0x5a, 0x30), C565(0xb0, 0x7a, 0x45)); }
// a round button at (cx, cy), 2 px a pixel (one Knomi pixel each: its edge as round as the screen shows; they were
// 4 px cells, Fedor: "They dont look perfectly round and are a bit more pixelated that some of the other art"): a
// black ring all round (the X had a grey one: "The x button needs to have black surround, like the left and right
// arrows"), the disc lit along its top and shaded along its bottom, a glint, its white mark with a shadow under it.
// mark 0: an X; -1, 1: an arrow left, right. 52 px across, as before
static bool v2BtnMark(int mark, int dx, int dy) {
  if (mark == 0) return dx >= -11 && dx <= 11 && dy >= -11 && dy <= 11 && (abs(dx - dy) <= 3 || abs(dx + dy) <= 3);
  int x = (mark > 0 ? dx : -dx) - 1;  // pointing right, a pixel right of the middle: it looks centred
  return x >= -7 && x <= 9 && abs(dy) * 16 <= (9 - x) * 11 + 2;
}
static void v2RoundBtn(int cx, int cy, int mark, Rgb base, Rgb lite, Rgb dark) {
  cx = SNAP2(cx);
  cy = SNAP2(cy);
  V2_HOVER(cx - 26, cy - 26, 52, 52);  // under the mouse: lifted (hover_v2.h)
  const uint16_t cRing = sc565(0x1B1B25), cBase = sc565(base), cLite = sc565(lite), cDark = sc565(dark),
                 cGlint = sc565(scMix(lite, 0xFFFFFF, .7)), cMark = sc565(0xFFFFFF), cShadow = sc565(scMix(dark, 0x1B1B25, .35));
  for (int j = -13; j < 13; j++) {
    int dy = 2 * j + 1, runX = 0;
    uint16_t runC = 0;
    bool on = false;
    for (int i = -13; i <= 13; i++) {
      bool have = false;
      uint16_t c = 0;
      if (i < 13) {
        int dx = 2 * i + 1, rr = dx * dx + dy * dy;
        if (rr <= 26 * 26) {
          have = true;
          if (rr > 22 * 22) c = cRing;
          else if (v2BtnMark(mark, dx, dy)) c = cMark;
          else if (v2BtnMark(mark, dx - 2, dy - 2)) c = cShadow;
          else if ((dx == -9 && dy == -15) || (dx == -11 && dy == -13) || (dx == -13 && dy == -11) || (dx == -11 && dy == -11)) c = cGlint;
          else if (dy < -11 || (dy < -5 && rr > 18 * 18)) c = cLite;
          else if (dy > 11 || (dy > 5 && rr > 18 * 18)) c = cDark;
          else c = cBase;
        }
      }
      if (on && (!have || c != runC)) {
        gfx->fillRect(cx + 2 * runX, cy + 2 * j, 2 * (i - runX), 2, runC);
        on = false;
      }
      if (have && !on) on = true, runX = i, runC = c;
    }
  }
}
// the red X, always at (233, 422)
static void v2CloseX() { v2RoundBtn(CX, V2_CLOSE_Y, 0, 0xE8503A, 0xF58C78, 0xB8321F); }
// a page button: blue, an arrow (dir -1 left, 1 right)
static void v2PageArrow(int x, int y, int dir) { v2RoundBtn(x, y, dir < 0 ? -1 : 1, 0x4F93C4, 0x86CAEE, 0x336E9E); }
// which page this is, in words above the X (Fedor: dots didn't say how many there are): "Page 2 of 5", white with
// a dark shadow so it reads on whatever is behind, its top no lower than y 380 so it clears the X
static void v2PageOf(int n, int cur, int y = 386) {
  char b[32];
  snprintf(b, sizeof(b), T(S_PAGE_OF_FMT), cur + 1, n);
  setSize(2);
  int x = centerX(b, 2), ty = (y < 386 ? y : 386) - 6;  // (its top at 380 at most: clear of a panel to 372 and the X)
  gfx->setTextColor(C565(0x1b, 0x1b, 0x25));
  setCur(x + 2, ty + 2);
  printT(b);
  gfx->setTextColor(UI_WHITE);
  setCur(x, ty);
  printT(b);
}
// a V2 button with its label centred (button())
static void v2LabelButton(int x0, int y0, int x1, int y1, const char *lab, uint16_t col, uint16_t ink = UI_WHITE, int size = 2) {
  v2Button(x0, y0, x1 - x0, y1 - y0, col);
  gfx->setTextColor(ink);
  setSize(size);
  setCur((x0 + x1) / 2 - textW(lab, size) / 2, (y0 + y1) / 2 - 4 * size + 1);
  printT(lab);
}
// a switch: green with the knob to the right when on
// (72 x 36, the auto-update row's height; Fedor: "The auto-update button is too small"; it was 48 x 24): a dark
// rim, the knob white with a soft shadow under it
static void v2Switch(int x, int y, bool on) {
  gfx->fillRoundRect(x, y, 72, 36, 18, C565(0x3a, 0x4a, 0x5a));
  gfx->fillRoundRect(x + 2, y + 2, 68, 32, 16, on ? C565(0x58, 0xb8, 0x68) : C565(0xd8, 0xd2, 0xbd));
  int kx = on ? x + 54 : x + 18;
  gfx->fillCircle(kx, y + 20, 14, on ? C565(0x3c, 0x8a, 0x4c) : C565(0xb0, 0xa8, 0x94));
  gfx->fillCircle(kx, y + 18, 14, UI_WHITE);
}
// text twice, a Knomi pixel apart: bold (not for the CJK font, which has its own weight)
static void v2Bold(const char *s, int x, int y) {
  setCur(x, y);
  printT(s);
  if (!gCjkFont) {
    setCur(x + 2, y);
    printT(s);
  }
}


// ----- start over -----
// The time page's START OVER (every build that has the question, its box in layout_v2.h): it asks
// "Start over?" on the home screen, with what it erases, before anything goes.
// "Start over?" and what it erases, in a panel over the dimmed home; ERASE EVERYTHING (red) and KEEP
// PLAYING (green) fill V1's two buttons exactly, so their taps are V1's. ERASE EVERYTHING asks once more
// (Fedor): "Are you sure?" and "There's no going back.", with KEEP PLAYING on top this time, so a quick second
// tap where ERASE was keeps the game
static void v2ResetDialog() {
  dimFrame();
  v2Panel(73, 92, 393, 340, 12);
  const char *q = T(v2ResetSure ? S_RESET_SURE : S_RESET_Q);
  int qs = textW(q, 3) <= 300 ? 3 : 2;
  gfx->setTextColor(UI_INK);
  setSize(qs);
  setCur(centerX(q, qs), 108);
  printT(q);
  // the warning, wrapped by its width at the spaces, up to three lines
  const char *w = T(v2ResetSure ? S_RESET_NOUNDO : S_RESET_WARN);
  char line[96];
  setSize(2);
  gfx->setTextColor(UI_BAR_BAD);
  for (int ln = 0, y = qs == 3 ? 142 : 134; *w && ln < 3; ln++, y += 20) {
    int n = 0, fit = 0;
    for (;;) {  // the longest run of whole words that fits
      int e = n;
      while (w[e] == ' ') e++;
      while (w[e] && w[e] != ' ') e++;
      snprintf(line, sizeof(line), "%.*s", e, w);
      if (fit && textW(line, 2) > 290) break;
      fit = n = e;
      if (!w[e]) break;
    }
    snprintf(line, sizeof(line), "%.*s", fit, w);
    setCur(centerX(line, 2), y);
    printT(line);
    w += fit;
    while (*w == ' ') w++;
  }
  int erase = v2ResetSure ? 271 : 209, keep = v2ResetSure ? 209 : 271;
  v2LabelButton(96, erase, 370, erase + 46, T(S_RESET_GO), UI_BAR_BAD);
  v2LabelButton(96, keep, 370, keep + 46, T(S_RESET_KEEP), C565(0x58, 0xb8, 0x68));
}

// ----- settings -----
#ifdef BOARD_KNOMI_V2  // as V1's: the Knomi's own (its Wi-Fi and updates)
// the Knomi's settings page: Wi-Fi, updates and the auto-update switch in a panel (settings_dim())
static void v2Settings() {
  v2MenuBackdrop();
  v2Sign("Wi-Fi");  // (Settings is the first page's)
  v2Panel(80, 84, 386, 372, 10);  // the buttons fill V1's boxes exactly (96..370), so a press's ring hugs them
  setSize(2);
  gfx->setTextColor(UI_INK);
  setCur(centerX("Wi-Fi", 2), 100);
  printT("Wi-Fi");
  const char *ssid = clockWifiSsid();
  setCur(centerX(ssid[0] ? ssid : "---", 2), 124);
  printT(ssid[0] ? ssid : "---");
  uint8_t st = clockWifiStatus();
  gfx->setTextColor(st == 1 ? C565(0x3c, 0x8a, 0x4c) : st == 2 ? UI_BAR_BAD : V2_INK_MUTED);
  const char *stText = st == 1 ? T(S_WIFI_SYNCED) : st == 2 ? T(S_WIFI_NOCONN) : T(S_WIFI_TRYING);
  setCur(centerX(stText, 2), 148);
  printT(stText);
  const uint16_t blue = C565(0x4f, 0x93, 0xc4), green = C565(0x58, 0xb8, 0x68);
  v2LabelButton(99, SET_WIFI_BTN_Y + 3, 367, SET_WIFI_BTN_Y + SET_BTN_H - 3, T(S_WIFI_CHANGE), blue);
  OtaState os = otaState();
  char ub[48];
  if (otaChecking) strlcpy(ub, T(S_WIFI_SEARCH), sizeof(ub));
  else if (os == OTA_AVAILABLE) snprintf(ub, sizeof(ub), T(S_UPD_TO_FMT), otaNewVersion());
  else strlcpy(ub, T(S_UPD_CHECK), sizeof(ub));
  v2LabelButton(99, UPD_BTN_Y + 3, 367, UPD_BTN_Y + SET_BTN_H - 3, ub, os == OTA_AVAILABLE ? green : blue);
  int lw = textW(T(S_AUTO_UPDATE), 2), x0 = CX - (lw + 14 + 72) / 2;
  gfx->setTextColor(UI_INK);
  setCur(x0, AUTO_ROW_Y + (AUTO_ROW_H - 16) / 2);
  printT(T(S_AUTO_UPDATE));
  v2Switch(SNAP2(x0 + lw + 14), AUTO_ROW_Y, autoUpdate);
  if (!otaChecking && (os == OTA_LATEST || os == OTA_FAILED)) {
    const char *m = os == OTA_LATEST ? T(S_UPD_LATEST) : T(S_UPD_FAILED);
    gfx->setTextColor(os == OTA_LATEST ? C565(0x3c, 0x8a, 0x4c) : UI_BAR_BAD);
    setCur(centerX(m, 2), AUTO_ROW_Y + AUTO_ROW_H + 8);
    printT(m);
  }
  drawPageArrows(1, V2_CLOCK_PAGES, V2_PAGE_ARROW_Y);  // back to the time
  v2PageOf(V2_CLOCK_PAGES, 1);
  v2CloseX();
}
#endif

// ----- the card: profile, battle stats, training, progress -----
// One spacing for every page (Fedor: "another pass of spacing and consistency"): each panel as wide as the others,
// its words and bars 16 px in from its sides, as tall as what's in it plus 16 px above and below, and its bottom
// just over the page's number, so the panels line up as the pages turn. Lines are V2_CARD_ROW apart.
#define V2_CARD_X0 84
#define V2_CARD_X1 382
#define V2_CARD_L 100                // where the words and bars start
#define V2_CARD_R 366                // and where they end
#define V2_CARD_TX (V2_CARD_L + 26)  // the words after a line's icon
#define V2_CARD_BOT 372
#define V2_CARD_PAD 16
#define V2_CARD_ROW 32
// the top of a page's panel for h px of lines (the first one's top to the last one's bottom)
static int v2CardTop(int h) { return V2_CARD_BOT - h - 2 * V2_CARD_PAD; }
// that panel; it returns where the first line goes
static int v2CardPanel(int h) {
  int y0 = v2CardTop(h);
  v2Panel(V2_CARD_X0, y0, V2_CARD_X1, V2_CARD_BOT, 10);
  return y0 + V2_CARD_PAD;
}
// the card's one look for a bar: x0 to x1 at y, h tall, k of it filled in col
static void v2CardBar(int x0, int x1, int y, float k, uint16_t col, int h = 14) {
  gfx->fillRoundRect(x0, y, x1 - x0, h, 5, C565(0xd8, 0xd2, 0xbd));
  int fw = (int)((x1 - x0 - 4) * (k < 0 ? 0 : k > 1 ? 1 : k));
  if (fw > 2) gfx->fillRoundRect(x0 + 2, y + 2, fw, h - 4, 4, col);
}
// a text ending at the panel's inner edge
static void v2CardRight(const char *s, int y, uint16_t col) {
  setSize(2);
  gfx->setTextColor(col);
  setCur(V2_CARD_R - textW(s, 2), y);
  printT(s);
}
// a line in its two halves, where the language leaves two spaces between them ("STREAK %u  best %u",
// "%s   AGE %lud"): the first into a, the rest into b (b empty when there's no such gap)
static void v2SplitPair(const char *s, char *a, char *b, size_t n) {
  const char *gap = strstr(s, "  ");
  if (!gap) {
    strlcpy(a, s, n);
    b[0] = 0;
    return;
  }
  snprintf(a, n, "%.*s", (int)(gap - s), s);
  while (*gap == ' ') gap++;
  strlcpy(b, gap, n);
}
// a text wrapped at its spaces to w px, from y down (pitch px apart), up to n lines: each line centred, or from x
// when x >= 0. It returns how many lines it took; with draw false it only counts them
static int v2Wrapped(const char *s, int y, int w, int pitch, int n, int x = -1, bool draw = true) {
  char line[96];
  setSize(2);
  int ln = 0;
  for (; *s && ln < n; ln++, y += pitch) {
    int e = 0, fit = 0;
    for (;;) {  // the longest run of whole words that fits
      int k = e;
      while (s[k] == ' ') k++;
      while (s[k] && s[k] != ' ') k++;
      snprintf(line, sizeof(line), "%.*s", k, s);
      if (fit && textW(line, 2) > w) break;
      fit = e = k;
      if (!s[k]) break;
    }
    snprintf(line, sizeof(line), "%.*s", fit, s);
    if (draw) {
      setCur(x >= 0 ? x : centerX(line, 2), y);
      printT(line);
    }
    s += fit;
    while (*s == ' ') s++;
  }
  return ln;
}
static const uint16_t V2_STAT_COL[4] = { C565(0xe8, 0x50, 0x3a), C565(0x4f, 0x93, 0xc4), C565(0xf0, 0xc0, 0x4a), C565(0x8e, 0x8c, 0x96) };
// the widest of a page's four stat names at size 2: the bars start after it, so a long name (Japanese) never runs
// into its bar
static int v2LabW(const char *a, const char *b, const char *c, const char *d) {
  const char *s[4] = { a, b, c, d };
  setSize(2);
  int w = 0;
  for (int i = 0; i < 4; i++) {
    int t = textW(s[i], 2);
    if (t > w) w = t;
  }
  return w;
}
// a stat in a panel: its name, a bar after the widest name (labW), the number at the right (stat_row()); a
// boosted number is green
static void v2StatRow(int y, const char *lab, int v, int maxv, uint16_t col, int labW, bool boosted = false, bool maxed = false,
                      bool unknown = false) {
  setSize(2);
  gfx->setTextColor(UI_INK);
  setCur(V2_CARD_L, y);
  printT(lab);
  v2CardBar(V2_CARD_L + labW + 12, V2_CARD_R - textW("888", 2) - 12, y + 1, (float)(v < maxv ? v : maxv) / maxv, col);
  char num[8];
  if (maxed) strlcpy(num, "MAX", sizeof(num));
  else if (unknown) strlcpy(num, "?", sizeof(num));  // a friend on an older version doesn't send it
  else snprintf(num, sizeof(num), "%d", v);
  v2CardRight(num, y, maxed ? col : unknown ? V2_INK_MUTED : boosted ? C565(0x1e, 0x7d, 0x3c) : UI_INK);
}
// your Pokemon standing on the rug, its feet at y, as big as fits under the board (at most maxS)
static void v2CardMon(int feet, int maxH, uint8_t maxS) {
  if (!pmd.loaded) return;
  uint8_t s = pmdFitScale(pmd, PMD_IDLE, 220, maxH);
  gPmdForceScale = s < maxS ? s : maxS;
  v2PetShadow(CX, feet - 4, 44);
  drawPmdAct(PMD_IDLE, CX, feet, millis(), true, false, maxS);
  gPmdForceScale = 0;
}
// The Profile: its name alone on the board (Fedor: room for a long one), the level in white under it as the
// Pokedex has its number, with the species before it under a nickname ("PIKACHU Lv.22"); your Pokemon big on the
// rug; in the panel the streak by its flame (the best at the right), the bond by a heart with its bar, the
// favourite berry by its icon (the age at the right, or on a line of its own when the two would meet)
static void v2CardProfile() {
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  char head[32];
  snprintf(head, sizeof(head), "%s%s", pet.shiny ? "*" : "", nm);
  int right = v2Sign(head);
  drawMap(SPR_PENCIL, 10, right + 4, V2_BOARD_Y + 2, 2, false);  // the name can be changed: tap the board
  char sub[40];
  if (pet.nick[0]) snprintf(sub, sizeof(sub), T(S_NAME_FMT), "", dexName(pet.speciesId), pet.level());
  else snprintf(sub, sizeof(sub), T(S_LVL_FMT), pet.level());
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(centerX(sub, 2), V2_BOARD_Y + 35);
  printT(sub);
  cardNameBottom = V2_BOARD_Y + 54;  // (a tap on the line under the board renames too)
  int top = V2_BOARD_Y + 54;
  char line[48], streak[48], best[48], berryName[48], age[48];
  snprintf(line, sizeof(line), T(S_STREAK_FMT), pet.streak, pet.bestStreak);
  v2SplitPair(line, streak, best, sizeof(streak));
  const char *berry = !pet.berryKnown ? T(S_BERRY_UNK) : pet.lovesBerry(0) ? T(S_BERRY_RED) : pet.lovesBerry(1) ? T(S_BERRY_BLUE) : T(S_BERRY_GREEN);
  snprintf(line, sizeof(line), T(S_INFO_FMT), berry, (unsigned long)(pet.ageMinutes / 1440));
  v2SplitPair(line, berryName, age, sizeof(berryName));
  setSize(2);
  bool ageOwn = V2_CARD_TX + textW(berryName, 2) + 16 > V2_CARD_R - textW(age, 2);
  int h = (ageOwn ? 3 : 2) * V2_CARD_ROW + 16, feet = v2CardTop(h) - 10;
  v2CardMon(feet, feet - top - 6, 4);
  int y = v2CardPanel(h);
  gfx->fillTriangle(110, y + 1, 103, y + 14, 117, y + 14, UI_BAR_BAD);  // the streak, by its flame
  gfx->fillTriangle(110, y + 6, 106, y + 14, 114, y + 14, UI_BAR_WARN);
  gfx->setTextColor(UI_INK);
  setCur(V2_CARD_TX, y);
  printT(streak);
  v2CardRight(best, y, V2_INK_MUTED);
  y += V2_CARD_ROW;  // the bond, by a heart in its bar's pink
  const uint16_t pink = C565(0xf0, 0x8a, 0xa4);
  v2LifeHeart(110, y + 8, true, pink, C565(0xc8, 0x5a, 0x78));
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(V2_CARD_TX, y);
  printT(T(S_VIN));
  v2CardBar(V2_CARD_TX + textW(T(S_VIN), 2) + 12, V2_CARD_R, y + 1, (pet.bond > 100 ? 100 : pet.bond) / 100.0f, pink);
  y += V2_CARD_ROW;  // the favourite berry, by its icon, and the age
  if (pet.berryKnown) {
    static const char *const *const BERRY_ICON[3] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP };
    drawMap(BERRY_ICON[pet.lovesBerry(0) ? 0 : pet.lovesBerry(1) ? 1 : 2], HOME_ICON_N, 101, y - 1, 1, false);
  }
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(V2_CARD_TX, y);
  printT(berryName);
  if (ageOwn) {
    y += V2_CARD_ROW;
    gfx->setTextColor(V2_INK_MUTED);
    setCur(V2_CARD_TX, y);
    printT(age);
  } else {
    v2CardRight(age, y, V2_INK_MUTED);
  }
}
// Battle stats: your Pokemon, its four stats in bars, and its wins by the trophy
static void v2CardStats() {
  v2Sign(T(S_BATTLE));
  int bon = partyBoostOn() ? PARTY_STAT_BONUS : 0;  // friends around: what wild battles use
  int labW = v2LabW(T(S_STAT_ATK), T(S_STAT_DEF), T(S_STAT_SPE), T(S_STAT_WGT));
#ifdef BOARD_KNOMI_V2
  const int step = V2_CARD_ROW, h = 3 * step + 16 + 24 + 16;  // the four bars, then the wins
  int feet = v2CardTop(h) - 10;
  v2CardMon(feet, feet - V2_BOARD_Y - 34, 3);
  int y = v2CardPanel(h), wy = y + 3 * step + 16 + 24;
#else
  const int step = 24, y = 204, wy = 346;  // the battle button under the bars stays where it's pressed (300 to 340)
  v2CardMon(184, 184 - V2_BOARD_Y - 34, 3);
  v2Panel(V2_CARD_X0, 190, V2_CARD_X1, V2_CARD_BOT, 10);
#endif
  v2StatRow(y, T(S_STAT_ATK), pet.atkStat() + bon, 260, V2_STAT_COL[0], labW, bon > 0);
  v2StatRow(y + step, T(S_STAT_DEF), pet.defStat() + bon, 260, V2_STAT_COL[1], labW, bon > 0);
  v2StatRow(y + 2 * step, T(S_STAT_SPE), pet.speStat() + bon, 260, V2_STAT_COL[2], labW, bon > 0);
  v2StatRow(y + 3 * step, T(S_STAT_WGT), pet.weight, 100, V2_STAT_COL[3], labW);
#ifndef BOARD_KNOMI_V2
  v2LabelButton(112, 300, 354, 334, T(S_BTL_START), UI_BAR_BAD);  // (the Knomi has its own button)
#endif
  char wins[40];
  snprintf(wins, sizeof(wins), T(S_WINS_FMT), pet.battleWins);
  setSize(2);
  int ww = textW(wins, 2), x0 = CX - (ww + 32) / 2;
  drawMap(SPR_TROPHY, 12, x0, wy - 4, 2, false);
  gfx->setTextColor(UI_INK);
  setCur(x0 + 32, wy);
  printT(wins);
}
// Training: each stat's training in a bar, and how it's trained under it (a long line takes two: German, French)
static void v2CardTraining() {
  v2Sign(T(S_TRAINING));
  struct { const char *lab; uint8_t v; const char *how; bool maxes; } rows[4] = {
    { T(S_STAT_ATK), pet.trAtk, T(S_TRAIN_HOW_ATK), true }, { T(S_STAT_DEF), pet.trDef, T(S_TRAIN_HOW_DEF), true },
    { T(S_STAT_SPE), pet.trSpe, T(S_TRAIN_HOW_SPE), true }, { T(S_STAT_WGT), pet.weight, T(S_TRAIN_HOW_WGT), false } };
  const int w = V2_CARD_R - V2_CARD_L;
  int labW = v2LabW(rows[0].lab, rows[1].lab, rows[2].lab, rows[3].lab), lines[4], h = 3 * 16;  // (16 between them)
  for (int i = 0; i < 4; i++) {
    lines[i] = v2Wrapped(rows[i].how, 0, w, 20, 2, V2_CARD_L, false);
    h += 22 + 16 + (lines[i] - 1) * 20;
  }
  int y = v2CardPanel(h);
  for (int i = 0; i < 4; i++) {
    v2StatRow(y, rows[i].lab, rows[i].v, 100, V2_STAT_COL[i], labW, false, rows[i].maxes && rows[i].v >= 100);
    gfx->setTextColor(V2_INK_MUTED);
    v2Wrapped(rows[i].how, y + 22, w, 20, 2, V2_CARD_L);
    y += 22 + 16 + (lines[i] - 1) * 20 + 16;
  }
}
static void v2CardItem(int which);  // (repel_v2.h: an item's own page: 0 the Ultra Ball, 1 the Master Ball, 2 the Repel)
// Progress: the level big, its bar and what's left to the next; then evolution: when, and the slip-ups holding it back
static void v2CardProgress() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  v2Sign(T(S_PROGRESS));
  int y = v2CardPanel(32 + 12 + 18 + 10 + 16 + 24 + 16 + 8 + 16 + 8 + 16);
  char lv[10];
  snprintf(lv, sizeof(lv), T(S_LVL_FMT), pet.level());
  gfx->setTextColor(UI_INK);
  setSize(4);
  v2Bold(lv, centerX(lv, 4) - 1, y);
  bool top = pet.level() >= LEVEL_MAX;  // a level is an hour of play; full, and MAX, at the top
  uint8_t into = pet.ageMinutes % MINUTES_PER_LEVEL;
  v2CardBar(V2_CARD_L, V2_CARD_R, y + 44, top ? 1.0f : into / (float)MINUTES_PER_LEVEL, C565(0x58, 0xb8, 0x68), 18);
  char nx[32];
  if (top) strlcpy(nx, "MAX", sizeof(nx));
  else snprintf(nx, sizeof(nx), T(S_NEXT_LVL_FMT), MINUTES_PER_LEVEL - into, pet.level() + 1);
  gfx->setTextColor(V2_INK_MUTED);
  setSize(2);
  setCur(centerX(nx, 2), y + 72);
  printT(nx);
  setCur(centerX(T(S_EVO_LABEL), 2), y + 112);
  printT(T(S_EVO_LABEL));
  char evoBuf[32];
  const char *evo;
  uint16_t evoCol = UI_INK;
  if (d.evolvesTo == 0) {
    evo = T(S_FINAL_FORM);
  } else {
    int needed = d.evolveLevel + pet.careMistakes;
    if (pet.level() >= needed) {
      if (pet.lowestStat() >= 40) evo = T(S_EVO_READY), evoCol = C565(0x3c, 0x8a, 0x4c);
      else evo = T(S_EVO_BLOCKED), evoCol = UI_BAR_BAD;
    } else {
      snprintf(evoBuf, sizeof(evoBuf), T(S_EVO_IN_FMT), needed - pet.level());
      evo = evoBuf;
    }
  }
  gfx->setTextColor(evoCol);
  setCur(centerX(evo, 2), y + 136);
  printT(evo);
  char ms[24];  // the slip-ups, which hold evolution back
  snprintf(ms, sizeof(ms), T(S_MISTAKES_FMT), pet.careMistakes);
  gfx->setTextColor(pet.careMistakes > 0 ? UI_BAR_BAD : C565(0x3c, 0x8a, 0x4c));
  setCur(centerX(ms, 2), y + 160);
  printT(ms);
}
// the card's four pages (the medals' pages are still V1's: step 5b)
static void v2Card() {
  v2MenuBackdrop();
  if (cardPage == 0) v2CardProfile();
  else if (cardPage == 1) v2CardStats();
  else if (cardPage == 2) v2CardTraining();
  else if (cardPage == 3) v2CardProgress();
  else v2CardItem(cardPage - 4);  // the Ultra Ball, the Master Ball, then the Repel: a page each, after Progress
  drawPageArrows(cardPage, cardPageCount(), V2_PAGE_ARROW_Y);
  v2PageOf(cardPageCount(), cardPage);
  v2CloseX();
}

// ----- the clock and the time zone picker -----
// The time page keeps V1's controls where they were (the time zone button, setting the time, sound, the
// language, OK); the room, the board, a panel behind the time and V2 buttons are new. The version is size 2.
// V1's language pill is defined further down the sketch; the same values here (a different one would warn).
#define LANG_PILL_Y 296
#define LANG_PILL_H 30
#define LANG_PILL_X 336
#define LANG_PILL_W 96
static void v2SettingsRows();  // (about_v2.h)
// Settings' first page. Its time set (the network's or the computer's): the time, where it comes from, a row each for
// the sound and the language, START OVER (about_v2.h; the version is on the About page now). Being set by hand (a
// Knomi without Wi-Fi): V1's controls where they were
static void v2Clock(const char *langCode) {
  bool locked = clockLocked();
  v2MenuBackdrop();
  v2Sign(locked ? T(S_SETTINGS) : T(S_SET_TIME));
  v2Panel(72, locked ? 84 : 92, 394, locked ? V2_SET_PANEL_Y1 : 280, 10);
  gfx->setTextColor(UI_INK);
  if (locked) {  // network time or the computer's: shown as it runs
    uint32_t e = rtcEpoch();
    drawBigTime((e / 3600) % 24, (e / 60) % 60);
    v2SettingsRows();
  } else {
    drawBigTime(clockH, clockM);
    static const int16_t KX[4] = { 104, 170, 252, 318 };
    for (int k = 0; k < 4; k++) v2LabelButton(KX[k] + 3, 190 + 3, KX[k] + 55, 190 + 55, k % 2 ? "+" : "-", C565(0x4f, 0x93, 0xc4), UI_WHITE, 4);
    gfx->setTextColor(V2_INK_MUTED);
    setSize(2);
    setCur(120, 256);
    printT(T(S_HOUR));
    setCur(276, 256);
    printT(T(S_MIN));
  }
  if (!locked) {  // being set: sound, left, and the language, right, both one tap; OK
    bool snd = audioEnabled();
    v2LabelButton(37, LANG_PILL_Y + 3, 127, LANG_PILL_Y + LANG_PILL_H - 3, snd ? T(S_SND_ON) : T(S_SND_OFF),
                  snd ? C565(0x58, 0xb8, 0x68) : C565(0x8e, 0x8c, 0x96));
    char lp[10];
    snprintf(lp, sizeof(lp), "%s >", langCode);
    v2LabelButton(LANG_PILL_X + 3, LANG_PILL_Y + 3, LANG_PILL_X + LANG_PILL_W - 3, LANG_PILL_Y + LANG_PILL_H - 3, lp,
                  C565(0x4f, 0x93, 0xc4));
    v2LabelButton(136, 343, 330, 385, "OK", C565(0x58, 0xb8, 0x68), UI_WHITE, 4);  // (size 4: crisp on the Knomi)
  } else {
#ifdef BOARD_KNOMI_V2  // (the question is the Knomi's, a computer's too)
    {
      V2_HOVER(V2_RESET_X0, V2_RESET_Y0, V2_RESET_X1 - V2_RESET_X0, V2_RESET_Y1 - V2_RESET_Y0);
      v2LabelButton(V2_RESET_X0, V2_RESET_Y0, V2_RESET_X1, V2_RESET_Y1, T(S_RESET_BTN), UI_BAR_BAD);  // asks first
    }
#endif
    v2PageOf(V2_CLOCK_PAGES, 0);
  }
  drawPageArrows(0, V2_CLOCK_PAGES, V2_PAGE_ARROW_Y);  // on to the Knomi's Wi-Fi, or About
  v2CloseX();
}
#ifdef BOARD_KNOMI_V2  // as V1's
// the time zone picker, as V1 picks (one city at a time, the arrows, OK), in the V2 frame
static void v2TzPicker() {
  v2MenuBackdrop();
  v2Sign(T(S_TIME_ZONE));
  v2Panel(72, 92, 394, 286, 10);
  uint32_t utc = clockUtc() ? clockUtc() : rtcEpoch();
  uint32_t l = clockLocalIn(tzPickIdx, utc);
  gfx->setTextColor(UI_INK);
  drawBigTime((l / 3600) % 24, (l / 60) % 60);
  const char *city = TIME_ZONES[tzPickIdx].city;
  setSize(4);
  int cs = textW(city, 4) <= 300 ? 4 : 2;  // (an even size: on the Knomi each font pixel lands whole; 3 spread over a pixel and a half, Fedor: crisp as the battles)
  setSize(cs);
  setCur(centerX(city, cs), TZ_CITY_Y + (cs == 4 ? -4 : 4));
  printT(city);
  char off[16];
  fmtUtcOffset(off, sizeof(off), (int32_t)(l - utc));
  gfx->setTextColor(V2_INK_MUTED);
  setSize(2);
  setCur(centerX(off, 2), TZ_CITY_Y + 40);
  printT(off);
  drawPageArrows(tzPickIdx, TIME_ZONE_COUNT, TZ_ARROW_Y);
  v2LabelButton(136, 343, 330, 385, "OK", C565(0x58, 0xb8, 0x68), UI_WHITE, 4);  // (size 4: crisp on the Knomi)
  v2CloseX();
}
#endif

// ----- the Pokedex -----
// a grid's board (its title and count) and a white panel round its squares: the Pokemon stand straight on
// the white, no box each, which shows a shiny's sparkles best (Fedor)
static void v2GridHead(const char *title, const char *count, int x0, int y0, int cell, int rows) {
  char b[48];
  snprintf(b, sizeof(b), "%s  %s", title, count);
  v2Sign(b);
  v2Panel(x0 - 6, y0 - 6, x0 + 3 * cell + 6, y0 + rows * cell + 6, 10);
}
// an entry's board: its number and name ("???" until it's caught)
// an entry's name on the board ("???" until it's caught), its number in white under the board (Fedor)
static void v2DexBoard(int16_t dex, bool reg) {
  v2Sign(reg ? dexName(dex) : "???");
  char num[8];
  snprintf(num, sizeof(num), "N.%03d", dex);
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(centerX(num, 2), V2_BOARD_Y + 35);
  printT(num);
}
// An entry's Pokemon turns round, a full turn every 2 s (Fedor): eight views from the packs' walks, 45 degrees
// apart, each held 250 ms: facing you, then turning to show its left side, its back, its right side, standing
// still in each view (each walk's first frame), shinies too (Fedor). A pack without all the walks just faces you.
#define V2_SPIN_STEP_MS 250
static uint8_t v2SpinAct(const PmdMon &m, uint32_t now) {
  static const uint8_t VIEW[8] = { PMD_WALKD, PMD_WALKDL, PMD_WALKL, PMD_WALKUL, PMD_WALKU, PMD_WALKUR, PMD_WALKR, PMD_WALKDR };
  for (uint8_t a : VIEW)
    if (!m.has(a)) return PMD_IDLE;
  return VIEW[(now / V2_SPIN_STEP_MS) % 8];
}
// an entry's type chip, under its art on the room
static void v2TypeChip(int16_t dex) {
  const DexEntry &d = DEX_TBL[dex];
  setSize(2);
  const char *ty = TYPE_NAME[d.type];
  int w = textW(ty, 2) + 24;
  gfx->fillRoundRect(CX - w / 2, 250, w, 24, 8, d.accent);
  gfx->setTextColor(UI_WHITE);
  setCur(centerX(ty, 2), 255);
  printT(ty);
}
// under the art: the type chip on the room, then the next form and the fact in a panel (dex_entry())
static const char *v2Caps(const char *s, char *buf, int n);  // (further down)
// (in capitals, as the Game Boy's Pokedex wrote them: on the Knomi a lowercase letter is 5 of its pixels high, and
// it read soft beside the battles' capitals; Fedor: "make sure the text and buttons elsewhere is also as crisp. For
// example the text for Pokedex entries")
static void v2DexInfo(int16_t dex, bool withFact) {
  const DexEntry &d = DEX_TBL[dex];
  v2TypeChip(dex);
  setSize(2);
  v2Panel(60, 280, 406, withFact ? 386 : 306, 10);
  char line[40], caps[40];
  gfx->setTextColor(V2_INK_MUTED);
  if (d.evolvesTo) {  // the next form isn't shown until it's found
    snprintf(line, sizeof(line), "Lv.%u > %s", d.evolveLevel, pet.isRegistered(d.evolvesTo) ? dexName(d.evolvesTo) : "???");
    v2Caps(line, caps, sizeof(caps));
    setCur(centerX(caps, 2), 286);
    printT(caps);
  } else {
    v2Caps(T(S_FINAL_FORM), caps, sizeof(caps));
    setCur(centerX(caps, 2), 286);
    printT(caps);
  }
  if (!withFact) return;
  // the fact at most 25 letters a line, at least 23 px in from the panel's sides (Fedor: "add some padding to the
  // sides of the text so it doesnt come so close to the edges"; it was 28, 5 px), its lines in the middle of the room
  // under the next form's: 20 px apart, 18 for the four facts that take four lines
  gfx->setTextColor(UI_INK);
  int n = 0, skip;  // how many lines it takes
  for (const char *q = DEX_FACT[dex]; *q && n < 4; n++) {
    wrapNext(q, 25, skip);
    q += skip;
  }
  int step = n >= 4 ? 18 : 20, y = 306 + (78 - (n * step - (step - 16))) / 2;
  const char *p = DEX_FACT[dex];
  for (int i = 0; i < n && *p; i++, y += step) {
    int len = wrapNext(p, 25, skip);
    snprintf(line, sizeof(line), "%.*s", len, p);
    v2Caps(line, caps, sizeof(caps));
    setCur(centerX(caps, 2), y);
    printT(caps);
    p += skip;
  }
}

// ----- the trophy room (the achievements) and a medal's card -----
// All 25 medals as things in one room: the five zone badges in a case, the Pokedex statues (a Poke, Great, Ultra
// and Master Ball on marble) and a golden Pikachu under a glass dome (the shiny medal) on the top shelf, the level 100 cups on the next, three
// photos in gold frames on the lower wall and the pet's eight rosettes beside them. A medal not won yet is its shape
// in shadow. Tap for a card (medal_won(), medal_locked()): the room dimmed under a spotlight, the medal big on a
// plinth, its name on the board, how to win it and how far along you are, the page buttons stepping through all 25
// in the room's order. The things are pixel art in the game's style (tools/trophies.py, trophies_v2.h: Fedor, "they
// don't have the same art style as some of our icons or even the Pokemon sprites"), each drawn twice: small in the
// room, 2 px a pixel (one Knomi pixel each), and big on its card, 3 px a pixel, the size the food is drawn at.
#include "trophies_v2.h"
#define TR_WALL 0x8A5A3A
#define TR_WALL_D 0x6E452A
#define TR_WALL_L 0xA8744C
#define TR_GOLD 0xF0C04A
#define TR_GOLD_D 0xC08410
#define TR_SIL 0x4A2E1E    // a medal not won yet
#define TR_SIL_D 0x3A2214  // ...its outline
// the room's order (V2_TROPHY_ORDER) is in layout_v2.h: the taps and swipes need it early in the sketch
static bool v2Won(int medal, bool bare) { return !bare && pet.hasMedal(1UL << medal); }

// a medal's art, s px a pixel, the middle of its bottom edge at (x, base): in its colours once won, otherwise its
// shape in shadow (its outline a shade darker, no glass and no sparkles). Glass shows the room through it
static void trArt(const TrArt &a, int x, int base, int s, bool won, Rgb sil = TR_SIL, Rgb silD = TR_SIL_D) {
  int x0 = x - a.w * s / 2, y0 = base - a.h * s;
  for (int j = 0; j < a.h; j++) {
    const uint8_t *row = a.px + j * a.w;
    for (int i = 0; i < a.w; i++) {
      uint8_t k = row[i];
      if (!k) continue;
      int px = x0 + i * s, py = y0 + j * s;
      if (!won) {
        if (TR_PAL_A[k] < 255 || TR_PAL_WON[k]) continue;
        scRect(px, py, px + s - 1, py + s - 1, k == 1 ? silD : sil);
      } else if (TR_PAL_A[k] == 255) {
        scRect(px, py, px + s - 1, py + s - 1, TR_PAL[k]);
      } else {
        uint16_t c = sc565(scGrade(TR_PAL[k]));
        for (int yy = py + scDY; yy < py + s + scDY; yy++)
          for (int xx = px + scDX; xx < px + s + scDX; xx++)
            if (xx >= 0 && yy >= 0 && xx < scW && yy < scH) scFb[yy * scW + xx] = v2Blend(scFb[yy * scW + xx], c, TR_PAL_A[k]);
      }
    }
  }
}
// where each medal stands in the room: the middle of its bottom edge (V2_TROPHY_ORDER walks them in this order)
static void trSpot(int m, int &x, int &base) {
  static const int16_t STATUE_X[4] = { 118, 176, 234, 292 }, CUP_X[4] = { 127, 190, 257, 330 };
  if ((m >= 17 && m <= 21) || m == 25) {  // the badges, each in the middle of its place in the case (25: Knomi Tower's, last)
    x = 148 + (m == 25 ? 5 : m - 17) * 34;
    base = 80 + TR_ART_SMALL[m].h;  // (2 px a pixel: half its height under the middle, 80)
  } else if (m >= 9 && m <= 12) x = STATUE_X[m - 9], base = 170;
  else if (m == 8) x = 350, base = 170;
  else if (m == 13 || (m >= 22 && m <= 24)) x = CUP_X[m == 13 ? 0 : m - 21], base = 246;
  else if (m >= 14 && m <= 16) x = 72 + (m - 14) * 58, base = 308;  // three photos in a row
  else x = 268 + (m % 4) * 38 + (m < 4 ? 0 : 19), base = m < 4 ? 298 : 334;  // the rosettes: two rows of four, the
}                                                                            // second between the first's tails

// the room; bare: every medal in shadow (behind a medal's card)
static int8_t trHoverGroup = -1;  // the group under the mouse: left out of the painted room, drawn over it to lift
static void trMedal(int m, bool bare) {
  int x, base;
  trSpot(m, x, base);
  if ((m >= 17 && m <= 21) || m == 25) trArt(TR_ART_SMALL[m], x, base, 2, v2Won(m, bare), 0x4A1620, 0x3A1018);  // shadows in the velvet
  else trArt(TR_ART_SMALL[m], x, base, 2, v2Won(m, bare));
}
static void trRoom(bool bare) {
  scRR(0, 0, V2_W, 330, TR_WALL);
  for (int x = 0; x < V2_W; x += 40) {  // wood panelling
    scRR(x, 0, x + 4, 330, TR_WALL_D);
    scRR(x + 4, 0, x + 8, 330, TR_WALL_L);
  }
  scRR(0, 40, V2_W, 46, TR_GOLD_D);  // a gold picture rail
  scRR(0, 40, V2_W, 42, TR_GOLD);
  scRR(0, 324, V2_W, 332, 0x3A2414);
  scRR(0, 332, V2_W, V2_W, 0xA83C3C);  // red carpet, a gold edge
  for (int y = 340; y < V2_W; y += 16)
    for (int x = ((y / 16) % 2) * 16; x < V2_W; x += 32) scGridRect(x, y, x + 8, y + 8, 0x882C2C);
  scRR(0, 332, V2_W, 336, TR_GOLD_D);
  scRR(126, 56, 340, 104, 0x3A2414);  // the badges' case, velvet inside
  scRR(130, 60, 336, 100, 0x6E1E2A);
  scRR(130, 60, 336, 64, 0x8A2A38);
  for (int sy : { 170, 246 }) {  // two shelves
    scRR(84, sy, 382, sy + 8, 0x5A3A22);
    scRR(84, sy, 382, sy + 2, TR_WALL_L);
    scRR(92, sy + 8, 100, sy + 18, 0x3A2414);
    scRR(366, sy + 8, 374, sy + 18, 0x3A2414);
  }
  for (int i = 0; i < MED_COUNT; i++)
    if (v2TrophyGroup(V2_TROPHY_ORDER[i]) != trHoverGroup) trMedal(V2_TROPHY_ORDER[i], bare);
}
// a tap in the room opens the shelf of the group it's on (v2TrophyGroupAt(), layout_v2.h; Fedor, first: tapping the
// things themselves was odd, a tap anywhere opened the first card; then: "If you click on the trophy room, you get to
// sections"). V1's achMedalAt() asks this, and gets the group's first medal
static int v2TrophyAt(int16_t x, int16_t y) {
  return closeHit(x, y) ? -1 : V2_TROPHY_ORDER[V2_TR_GROUP_FIRST[v2TrophyGroupAt(x, y)]];
}

// what the won medals draw, a bit a pixel: where the room as it is differs from the bare room (every medal in
// shadow), so the gleam lights the medals and nothing behind them
static uint8_t *trGleamMask = nullptr;
// the room, from the scene buffer (painted again when the medals change); the count on a board on the carpet
static void v2PaintTrophyRoom(int) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  if (!trGleamMask) trGleamMask = (uint8_t *)ps_malloc(V2_W * V2_W / 8 + 1);
  if (trGleamMask && v2SceneBuf) {  // (v2Cached copies the room over the scene buffer straight after)
    trRoom(true);
    memcpy(v2SceneBuf, fb, V2_W * V2_W * sizeof(uint16_t));
    trRoom(false);
    memset(trGleamMask, 0, V2_W * V2_W / 8 + 1);
    for (int i = 0; i < V2_W * V2_W; i++)
      if (fb[i] != v2SceneBuf[i]) trGleamMask[i >> 3] |= 1 << (i & 7);
  } else {
    trRoom(false);
  }
}
// the gleam over the medals won (Fedor: "a gleam animation that swipes over them"): every 3.2 s a slanted band of
// light, a wide stripe and a thin one, crosses the room left to right in 0.9 s, in the room's 4 px cells
static void trGleam(uint32_t now) {
  uint32_t t = now % 3200;
  if (!trGleamMask || t >= 900) return;
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  int pos = -40 + (int)(t * (V2_W / 4 + 120) / 900);  // the band's place, in cells along x + y / 2
  for (int cy = 56 / 4; cy < 332 / 4; cy++)
    for (int cx = 0; cx < V2_W / 4; cx++) {
      int d = cx + cy / 2 - pos, a = d >= 0 && d < 3 ? 150 : d == 5 ? 100 : 0;  // the wide stripe, then the thin one
      if (!a) continue;
      for (int y = cy * 4; y < cy * 4 + 4; y++)
        for (int x = cx * 4; x < cx * 4 + 4 && x < V2_W; x++) {
          int i = y * V2_W + x;
          if (trGleamMask[i >> 3] & (1 << (i & 7))) fb[i] = v2Blend(fb[i], 0xFFFF, a);
        }
    }
}
static void v2TrophyRoom() {
  uint32_t won = 0;
  int got = 0;
  for (int m = 0; m < MED_COUNT; m++)
    if (pet.hasMedal(1UL << m)) won |= 1UL << m, got++;
  int hg = -1;
#ifdef BOARD_HOST
  for (int g = 0; g < V2_TR_GROUPS; g++) {
    int bx, by, bw, bh;
    v2TrophyGroupBox(g, bx, by, bw, bh);
    if (v2HoverIs(bx, by, bw, bh)) hg = g;
  }
#endif
  trHoverGroup = (int8_t)hg;
  v2Cached(0x40000000 | (hg + 1) << 25 | (int)won, v2PaintTrophyRoom, 0);
  if (hg >= 0) {  // under the mouse: its group over the room without it, so it can lift with its shadow (hover_v2.h)
    int bx, by, bw, bh;
    v2TrophyGroupBox(hg, bx, by, bw, bh);
    V2_HOVER_DIFF(bx, by, bw, bh);
    for (int i = 0; i < MED_COUNT; i++)
      if (v2TrophyGroup(V2_TROPHY_ORDER[i]) == hg) trMedal(V2_TROPHY_ORDER[i], false);
  }
  if (won) trGleam(millis());
  char b[32];
  snprintf(b, sizeof(b), T(S_MEDALS_FMT), got, MED_COUNT);
  v2SignIn(b, 350, C565(0x6e, 0x1e, 0x2a), C565(0x8a, 0x2a, 0x38));
  v2CloseX();
}

// ----- a group's shelf: the room a step closer -----
// Fedor: "the trophy room has no names. Then the grouped trophies should have the names. And then if you click into
// the trophy, then you have the whole view with the explanation". One of the room's five groups on two rows, a place
// each big enough for a finger: the things at the room's 2 px a pixel, from their cards' pictures, which have more in
// them; the badges, the statues and the cups on shelves, the photos and the rosettes on the wall; each
// with its name on a plate under it, gold once it's won and brown till then. The board says the group and how many
// of it you have, in gold when that's all of them. The side buttons and swipes step through the five, a tap on a
// thing opens its card, the X (or a swipe down) goes back to the room. The plates of a row are all one size, the
// biggest of them, a name on one line in the middle of its plate (Fedor: "make the Bond label as large as the others
// and center Bond in that badge").
static bool trShelved(int g) { return g <= 2; }  // the badges, the statues and the cups stand on shelves (Fedor: the
                                                 // badges too, not a case of their own); the photos and rosettes hang
static int8_t trShelfHover = -1;  // the thing under the mouse: left off the painted shelf, drawn over it to lift
static void trShelfThing(int g, int k) {
  int m = V2_TROPHY_ORDER[V2_TR_GROUP_FIRST[g] + k], x0, y0, w, h;
  v2ShelfCell(g, k, x0, y0, w, h);
  const TrArt &a = TR_ART_BIG[m];
  int base = trShelved(g) ? y0 + V2_TR_ART_H : y0 + V2_TR_ART_H / 2 + a.h;  // (2 px a pixel: its middle on the row's)
  trArt(a, x0 + w / 2, base, 2, pet.hasMedal(1UL << m));
}
static void v2PaintShelf(int g) {
  scRR(0, 0, V2_W, 376, TR_WALL);  // the wall: the room's planks, further apart
  for (int x = 24; x < V2_W; x += 60) {
    scRR(x, 0, x + 4, 376, TR_WALL_D);
    scRR(x + 4, 0, x + 8, 376, TR_WALL_L);
  }
  scRR(0, 372, V2_W, 380, 0x3A2414);  // the skirting, the carpet
  scRR(0, 380, V2_W, V2_W, 0xA83C3C);
  for (int y = 388; y < V2_W; y += 16)
    for (int x = ((y / 16) % 2) * 16; x < V2_W; x += 32) scGridRect(x, y, x + 8, y + 8, 0x882C2C);
  scRR(0, 380, V2_W, 384, TR_GOLD_D);
  int n = v2TrophyGroupSize(g);
  if (trShelved(g)) {  // a shelf under each row
    for (int r = 0; r < 2; r++) {
      int k0 = r ? V2_TR_ROW1[g] : 0, k1 = r ? n - 1 : V2_TR_ROW1[g] - 1, ax, ay, bx, w, h;
      v2ShelfCell(g, k0, ax, ay, w, h);
      v2ShelfCell(g, k1, bx, ay, w, h);
      int sy = ay + V2_TR_ART_H, x0 = ax + 2, x1 = bx + w - 2;
      scRR(x0, sy, x1, sy + 8, 0x5A3A22);
      scRR(x0, sy, x1, sy + 2, TR_WALL_L);
      scRR(x0 + 8, sy + 8, x0 + 16, sy + 16, 0x3A2414);
      scRR(x1 - 16, sy + 8, x1 - 8, sy + 16, 0x3A2414);
    }
  }
  for (int k = 0; k < n; k++)
    if (k != trShelfHover) trShelfThing(g, k);
}
// a medal's name on a plate, its top at y: gold once it's won, brown till then. Too wide for its place (maxW), it
// breaks at the space that leaves its longer line shortest (German's BINDUNG and ENDFORM side by side ran into each
// other: a plate keeps its margin only while it has the room)
struct TrPlate {
  char a[48], b[48];  // its lines (b empty: one line)
  int wa, wb, w, h;   // their widths, and the plate's own size
};
static void v2PlateFit(const char *s, int maxW, TrPlate &p) {
  char *a = p.a, *b = p.b;
  b[0] = 0;
  snprintf(a, sizeof(p.a), "%s", s);
  setSize(2);
  if (textW(a, 2) > maxW) {
    int best = -1, bestW = 9999;
    for (int i = 1; s[i]; i++) {
      if (s[i] != ' ') continue;
      snprintf(a, sizeof(p.a), "%.*s", i, s);
      int wa = textW(a, 2), wb = textW(s + i + 1, 2), wm = wa > wb ? wa : wb;
      if (wm < bestW) bestW = wm, best = i;
    }
    snprintf(a, sizeof(p.a), "%.*s", best > 0 ? best : (int)strlen(s), s);
    if (best > 0) snprintf(b, sizeof(p.b), "%s", s + best + 1);
  }
  p.wa = textW(a, 2);
  p.wb = b[0] ? textW(b, 2) : 0;
  int tw = p.wa > p.wb ? p.wa : p.wb;
  p.h = b[0] ? 42 : 24;
  p.w = tw + 14 < maxW + 4 ? tw + 14 : tw + 6 > maxW + 4 ? tw + 6 : maxW + 4;  // (no wider than its place, so two long
}                                                                          // names side by side don't run into each other)
// a plate w x h, its top at y, its lines in its middle
static void v2NamePlate(const TrPlate &p, int cx, int y, int w, int h, bool won) {
  const uint16_t dark = C565(0x3a, 0x24, 0x14);
  int x0 = cx - w / 2, ty = y + (h - (p.b[0] ? 34 : 16)) / 2 + 1;
  gfx->fillRect(x0 - 2, y - 2, w + 4, h + 4, dark);
  gfx->fillRect(x0, y, w, h, won ? C565(0xc0, 0x84, 0x10) : C565(0x5a, 0x3a, 0x22));
  gfx->fillRect(x0, y, w, 3, won ? C565(0xf0, 0xc0, 0x4a) : C565(0x7a, 0x5a, 0x3a));
  gfx->setTextColor(won ? C565(0xff, 0xf3, 0xdc) : C565(0xc8, 0xb0, 0x90));
  setCur(cx - p.wa / 2, ty);
  printT(p.a);
  if (p.b[0]) {
    setCur(cx - p.wb / 2, ty + 18);
    printT(p.b);
  }
}
// a group's name and how many of it you have: "ZONES 3/5" (its shelf's board, the room's press); how many it has
static int v2ShelfTitle(int g, char *b, int len) {
  int first = V2_TR_GROUP_FIRST[g], n = v2TrophyGroupSize(g), got = 0;
  for (int k = 0; k < n; k++)
    if (pet.hasMedal(1UL << V2_TROPHY_ORDER[first + k])) got++;
  static const StrId TITLE[V2_TR_GROUPS] = { S_ZONE_MEDALS_FMT, S_DEX_TITLE, S_LV100_FMT, S_TRG_BUDDIES_FMT, S_TRG_CARE_FMT };
  if (g == 1) snprintf(b, len, "%s %d/%d", T(S_DEX_TITLE), got, n);  // ("POKEDEX" has no count of its own)
  else snprintf(b, len, T(TITLE[g]), got, n);
  return got;
}
static void v2TrophyShelf(int g) {
  int first = V2_TR_GROUP_FIRST[g], n = v2TrophyGroupSize(g), got = 0;
  uint32_t won = 0;
  for (int k = 0; k < n; k++)
    if (pet.hasMedal(1UL << V2_TROPHY_ORDER[first + k])) won |= 1UL << k, got++;
  int hk = -1;
#ifdef BOARD_HOST
  for (int k = 0; k < n; k++) {
    int x0, y0, w, h;
    v2ShelfCell(g, k, x0, y0, w, h);
    if (v2HoverIs(x0 + 4, y0 + 2, w - 8, h - 6)) hk = k;
  }
#endif
  trShelfHover = (int8_t)hk;
  v2Cached(0x3E000000 | (hk + 1) << 16 | (g << 12) | won, v2PaintShelf, g);
  for (int r = 0; r < 2; r++) {  // the names, a row's plates all the size of its biggest, a gap kept between them (one
    int k0 = r ? V2_TR_ROW1[g] : 0, k1 = r ? n : V2_TR_ROW1[g], pw = 0, ph = 0;  // longer name gets a wider one)
    TrPlate pl[4];
    for (int k = k0; k < k1; k++) {
      int x0, y0, w, h;
      v2ShelfCell(g, k, x0, y0, w, h);
      TrPlate &p = pl[k - k0];
      v2PlateFit(medalName(V2_TROPHY_ORDER[first + k]), w - 8, p);
      if (p.w > pw) pw = p.w;
      if (p.h > ph) ph = p.h;
    }
    for (int k = k0; k < k1; k++) {
      int x0, y0, w, h;
      v2ShelfCell(g, k, x0, y0, w, h);
      const TrPlate &p = pl[k - k0];
      int tw = p.wa > p.wb ? p.wa : p.wb, pwk = pw < w - 10 ? pw : w - 10;
      if (pwk < tw + 2) pwk = tw + 2 < w - 6 ? tw + 2 : w - 6;
      V2_HOVER_DIFF(x0 + 4, y0 + 2, w - 8, h - 6);  // under the mouse: the thing and its plate lift (hover_v2.h)
      if (k == hk) trShelfThing(g, k);
      v2NamePlate(p, x0 + w / 2, y0 + V2_TR_ART_H + (trShelved(g) ? 20 : 6), pwk, ph, won & (1UL << k));
    }
  }
  char b[48];
  v2ShelfTitle(g, b, sizeof(b));
  if (got == n) v2SignIn(b, V2_BOARD_Y, C565(0xc0, 0x84, 0x10), C565(0xf0, 0xc0, 0x4a));
  else v2Sign(b);
  drawPageArrows(g, V2_TR_GROUPS, V2_PAGE_ARROW_Y);
  v2PageOf(V2_TR_GROUPS, g, 390);
  v2CloseX();
}

// how far along a medal not won yet is, where the game counts it
static bool v2MedalProgress(int m, int &cur, int &goal) {
  switch (m) {
    case 0: cur = pet.level(), goal = 10; return true;
    case 1: cur = pet.level(), goal = 25; return true;
    case 2: cur = pet.level(), goal = 50; return true;
    case 4: cur = pet.streak, goal = 7; return true;
    case 9: cur = pet.registeredCount(), goal = 10; return true;
    case 10: cur = pet.registeredCount(), goal = 50; return true;
    case 11: cur = pet.registeredCount(), goal = 100; return true;
    case 12: cur = pet.registeredCount(), goal = 151; return true;
    case 13: cur = pet.level(), goal = LEVEL_MAX; return true;
    case 22: cur = pet.lv100Count, goal = 3; return true;
    case 23: cur = pet.lv100Count, goal = 5; return true;
    case 24: cur = pet.lv100Count, goal = 10; return true;
  }
  return false;
}
// a medal's thing, big (3 px a pixel): standing on the plinth (its foot at base), or a badge, a photo or a rosette
// over it as if held up to the light
static void trBig(int m, bool won) {
  const TrArt &a = TR_ART_BIG[m];
  bool stands = (m >= 8 && m <= 13) || m >= 22;
  int base = stands ? 262 : 190 + a.h * 3 / 2;
  trArt(a, CX, base, 3, won, 0x63402A, 0x3A2214);  // (its shadow a shade lighter than the room's: the lights are down)
  if (won) {  // it shines: three sparkles round it, in its own pixels
    int x0 = CX - a.w * 3 / 2, x1 = x0 + a.w * 3, y0 = base - a.h * 3;
    const int16_t sp[3][3] = { {(int16_t)(x0 - 12), (int16_t)(y0 + 18), 3}, {(int16_t)(x1 + 9), (int16_t)(y0 + 6), 3},
                               {(int16_t)(x1 + 15), (int16_t)(y0 + a.h * 2), 2} };
    for (auto &q : sp) {
      int x = q[0], y = q[1], r = q[2] * 3;
      scRR(x - 1, y - r - 1, x + 2, y + r + 2, 0xFFFFFF);
      scRR(x - r - 1, y - 1, x + r + 2, y + 2, 0xFFFFFF);
      scRR(x - 2, y - 2, x + 3, y + 3, 0xFFF6C8);
    }
  }
}
// behind a card: the bare room, the lights down (scGrade's mode 4), a spotlight from above
static void v2PaintTrophyBack(int) {
  scGradeTod = 4;
  trRoom(true);
  scGradeTod = 0;
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const uint16_t warm = sc565(0xFFF3DC);
  for (int y = 40; y < 300; y++) {
    int half = 30 + (int)((y - 40) * .55);
    for (int x = CX - half; x < CX + half; x++)
      if (x >= 0 && x < V2_W) fb[y * V2_W + x] = v2Blend(fb[y * V2_W + x], warm, 26);
  }
}
static void v2MedalCard(int m) {
  bool won = pet.hasMedal(1UL << m);
  v2Cached(0x3F000000, v2PaintTrophyBack, 0);
  scScreen();
  scGradeTod = 0;
  scRR(173, 262, 293, 290, 0x5A3A22);  // the plinth
  scRR(173, 262, 293, 266, TR_WALL_L);
  trBig(m, won);
  if (!won) {
    gfx->setTextColor(C565(0x8a, 0x6a, 0x50));  // not yet: a question mark over its shadow
    setSize(4);
    setCur(centerX("?", 4), 186);
    printT("?");
  }
  if (won) v2SignIn(medalName(m), V2_BOARD_Y, C565(0xc0, 0x84, 0x10), C565(0xf0, 0xc0, 0x4a));
  else v2SignIn(medalName(m), V2_BOARD_Y, C565(0x5a, 0x3a, 0x22), C565(0x7a, 0x5a, 0x3a));
  v2Panel(72, 270, 394, 374, 10);  // (26 px over the mockup's, for its page line under it and a fourth line)
  // how to win it, wrapped by its width (three lines, or two over a progress bar), and under it whether it's won or
  // how far along: the lot together in the middle of the panel (Fedor: not stuck to its top and bottom edges)
  int cur, goal;
  bool prog = !won && v2MedalProgress(m, cur, goal);
  const char *w = medalHow(m);
  char lines[4][64];
  int nl = 0;
  setSize(2);
  // up to four lines, three over a bar (they were three and two: German, French, Italian and Portuguese lost their
  // last words, "Kummere dich 7 Tage am Stuck um dasselbe" without its Pokemon)
  while (*w && nl < (prog ? 3 : 4)) {
    int n = 0, fit = 0;
    for (;;) {
      int e = n;
      while (w[e] == ' ') e++;
      while (w[e] && w[e] != ' ') e++;
      snprintf(lines[nl], sizeof(lines[nl]), "%.*s", e, w);
      if (fit && textW(lines[nl], 2) > 306) break;
      fit = n = e;
      if (!w[e]) break;
    }
    snprintf(lines[nl++], sizeof(lines[0]), "%.*s", fit, w);
    w += fit;
    while (*w == ' ') w++;
  }
  int pitch = nl >= 4 || (prog && nl >= 3) ? 17 : 18;  // (17 px a line when it's that long)
  int statusH = prog ? 34 : 16, total = (nl ? nl * pitch + 8 : 0) + statusH;
  int y = 270 + (105 - total) / 2;
  gfx->setTextColor(UI_INK);
  for (int i = 0; i < nl; i++, y += pitch) {
    setCur(centerX(lines[i], 2), y);
    printT(lines[i]);
  }
  if (nl) y += 8;
  if (won) {
    gfx->setTextColor(C565(0x3c, 0x8a, 0x4c));
    setCur(centerX(T(S_MEDAL_GOT), 2), y);
    printT(T(S_MEDAL_GOT));
  } else if (prog) {
    if (cur > goal) cur = goal;
    gfx->fillRect(126, y, 214, 12, C565(0xd8, 0xd2, 0xbd));
    if (cur > 0) gfx->fillRect(128, y + 2, 210 * cur / goal, 8, C565(0x58, 0xb8, 0x68));
    char b[16];
    snprintf(b, sizeof(b), "%d / %d", cur, goal);
    gfx->setTextColor(V2_INK_MUTED);
    setCur(centerX(b, 2), y + 18);
    printT(b);
  } else {
    gfx->setTextColor(V2_INK_MUTED);
    setCur(centerX(T(S_MEDAL_TODO), 2), y);
    printT(T(S_MEDAL_TODO));
  }
  int g = v2TrophyGroup(m), pos = v2TrophyPos(m) - V2_TR_GROUP_FIRST[g], n = v2TrophyGroupSize(g);
  drawPageArrows(pos, n, V2_PAGE_ARROW_Y);
  v2PageOf(n, pos, 390);  // "Page 2 of 5": the medal's place in its group (the X goes back to the group's shelf)
  v2CloseX();
}

// ----- capitals -----
// The mockups write boards and cartridge titles in capitals. Latin letters only (Japanese and Korean have
// none); an accented small letter becomes its capital where the font has a clear one (Ñ, Ç, Ä, Ö, Ü, Å, Æ),
// otherwise the plain capital, as V1's capitals already do ("EVOLUCION").
static const char *v2Caps(const char *s, char *buf, int n) {
  int i = 0;
  for (; s[i] && i < n - 1; i++) {
    uint8_t c = (uint8_t)s[i];
    if (!gCjkFont) {
      if (c >= 'a' && c <= 'z') c -= 32;
      else if (c >= 0x80) switch (c) {  // the font's code page (CP437)
          case 0x81: c = 0x9A; break;
          case 0x84: c = 0x8E; break;
          case 0x86: c = 0x8F; break;
          case 0x87: c = 0x80; break;
          case 0x91: c = 0x92; break;
          case 0x94: c = 0x99; break;
          case 0xA4: c = 0xA5; break;
          case 0x83: case 0x85: case 0xA0: c = 'A'; break;
          case 0x82: case 0x88: case 0x89: case 0x8A: c = 'E'; break;
          case 0x8B: case 0x8C: case 0x8D: case 0xA1: c = 'I'; break;
          case 0x93: case 0x95: case 0xA2: c = 'O'; break;
          case 0x96: case 0x97: case 0xA3: c = 'U'; break;
          case 0x98: c = 'Y'; break;
        }
    }
    buf[i] = (char)c;
  }
  buf[i] = 0;
  return buf;
}

// ----- Travel: the round island (v2g.py travel_d()) and a zone's screen -----
// Five wedges round a Wild patch: the Field at the top, then the Cave, the Beach, the Mansion and the Forest
// clockwise, each in its zone's ground with its landmark and a name tag with how many you've caught there,
// dashed borders between them and the sea round the coast. One tap on a wedge opens that zone (as V1's tile
// did); Wild starts a wild battle. Fedor: "D is super cool!"
static const uint8_t V2_WEDGE_ZONE[5] = { Z_FIELD, Z_CAVE, Z_BEACH, Z_MANSION, Z_FOREST };
static const StrId V2_ZONE_NAMES[ZONE_COUNT] = { S_ZONE_FOREST, S_ZONE_BEACH, S_ZONE_CAVE, S_ZONE_MANSION, S_ZONE_FIELD };  // V1's ZONE_NAME, defined further down
static bool v2IsleLand(int x, int y) {  // single precision: the Knomi has hardware for it
  float a = atan2f(y - 233.f, x - 233.f), r = hypotf(x - 233.f, y - 233.f);
  return r < 198 + 8 * sinf(5 * a) + 5 * sinf(11 * a + 1);
}
static int v2IsleRegion(int x, int y) {  // 0..4: a wedge (V2_WEDGE_ZONE), 5: the Wild patch
  if (hypotf(x - 233.f, y - 233.f) < 60) return 5;
  float a = fmodf(atan2f(y - 233.f, x - 233.f) * 57.29578f + 90 + 36 + 720, 360);
  int w = (int)(a / 72);
  return w > 4 ? 4 : w;
}
// each 4 px cell's land (0x80) and region, worked out once and kept: the painter and the sea's motion use it
#define V2_ISLE_N (V2_W / 4 + 1)
#define V2_SEA 0x4F93C4
#define V2_SEA_L 0x7CC4EA
static uint8_t *v2IsleG = nullptr;
static bool v2IsleGrid() {
  if (v2IsleG) return true;
  v2IsleG = (uint8_t *)ps_malloc(V2_ISLE_N * V2_ISLE_N);
  if (!v2IsleG) return false;
  for (int j = 0; j < V2_ISLE_N; j++)
    for (int i = 0; i < V2_ISLE_N; i++) {
      int x = i * 4 + 2, y = j * 4 + 2;
      v2IsleG[j * V2_ISLE_N + i] = v2IsleLand(x, y) ? 0x80 | v2IsleRegion(x, y) : 0;
    }
  return true;
}
static uint8_t v2IsleCell(int i, int j) { return i < 0 || j < 0 || i >= V2_ISLE_N || j >= V2_ISLE_N ? 0 : v2IsleG[j * V2_ISLE_N + i]; }
static void v2IsleSpot(int k, int &x, int &y) {  // where wedge k's landmark stands, 140 out from the middle
  float a = (-90 + k * 72) * 0.0174533f;
  x = (int)(233 + 140 * cosf(a));
  y = (int)(233 + 140 * sinf(a)) - 4;
}
// The island without what moves: the waves, the pines, the palms and the tall grass are drawn every frame
// (v2IsleLife()), and the coast's foam and the mansion's windows over it.
static uint8_t v2IslePage = 0;  // V2: Travel's page: 0 the island, 1 Knomi Tower's islet (tower_v2.h)
static void twPierPaint();       // the pier off the Cave's coast, toward the tower (tower_v2.h)
static void v2PaintIsle(int) {
  static const Rgb FILL[6] = { 0x8FCE7A, 0x6C7190, 0xECDCAC, 0x2E2758, 0x3F8A4C, 0x5F9E3E };
  static const Rgb LIT[6] = { 0xA6DC90, 0x8A8FA6, 0xF8ECC4, 0x3E3470, 0x58B868, 0x79BD4F };
  const Rgb SEA = V2_SEA, SEA_L = V2_SEA_L;
  if (!v2IsleGrid()) return;
  auto at = [&](int i, int j) -> uint8_t { return v2IsleCell(i, j); };
  for (int j = 0; j * 4 < V2_W; j++)
    for (int i = 0; i * 4 < V2_W; i++) {
      int cx = i * 4, cy = j * 4;
      uint8_t c0 = at(i, j);
      if (!c0) {  // the sea, a lighter checker along the coast
        bool near = at(i, j - 1) || at(i, j + 1) || at(i - 1, j) || at(i + 1, j);
        scCell(cx, cy, near && (i + j) % 2 ? SEA_L : SEA);
        continue;
      }
      int z = c0 & 7;
      uint8_t r = at(i + 1, j), d = at(i, j + 1);
      bool edge = (r && (r & 7) != z) || (d && (d & 7) != z);
      bool coast = !(at(i, j - 1) && at(i, j + 1) && at(i - 1, j) && at(i + 1, j));
      Rgb c;
      if (coast) c = z != 2 ? 0xE2CF98 : 0xF8ECC4;  // a sandy coast (the Beach's paler)
      else if (edge) c = (i + j) % 2 == 0 ? 0x1B1B25 : scMix(FILL[z], 0x1B1B25, .4);  // dashed borders
      else if (!at(i, j - 2) || (at(i, j - 1) & 7) != z) c = LIT[z];  // lit along its top edge
      else c = FILL[z];
      scCell(cx, cy, c);
    }
  // each wedge's landmark (the Forest's pines and the Beach's palms sway, so they're drawn every frame)
  for (int k = 0; k < 5; k++) {
    int x, y;
    v2IsleSpot(k, x, y);
    switch (V2_WEDGE_ZONE[k]) {
      case Z_FOREST: break;
      case Z_FIELD: {
        scHouse(x - 24, y - 34, false);
        scFence(y + 2, x + 28, x + 70, 0x9A6F43, 0xB38A58, 12);
        static const int8_t FL[3][2] = { {-40, 4}, {-30, -6}, {40, -20} };
        for (int f = 0; f < 3; f++) scGridRect(x + FL[f][0], y + FL[f][1], x + FL[f][0] + 4, y + FL[f][1] + 4, SC_DAY.fl[f]);
        break;
      }
      case Z_CAVE: {
        static const int8_t PEAK[2][2] = { {-22, 44}, {20, 34} };
        for (auto &pk : PEAK) {
          int px = x + pk[0], h = pk[1];
          for (int cy = y - h; cy < y + 4; cy += 4) {
            float half = (cy - (y - h)) * .85f;
            for (int cx = scFl4((int)floorf(px - half)) * 4; cx <= (int)(px + half); cx += 4)
              scCell(cx, cy, cy < y - h + 12 ? 0xE8EEF5 : cx > px ? 0x4A4F68 : 0x5A5F7C);
          }
        }
        scOval(scE(x - 30, y - 14, x - 14, y + 4), 0x1B1B25);  // the cave's mouth
        scSprite(SPR_V2_CRYSTAL, 6, x + 4, y - 10, 2, SC_CRYSTAL_KEYS, 2);
        break;
      }
      case Z_MANSION: {
        scMansion(x - 58, y - 62, 2);
        static const int8_t ST[3][2] = { {50, -10}, {64, -4}, {-64, -6} };
        const ScKey stone[2] = { { 'o', 0x0D0B20 }, { 's', 0x5A5F7C } };
        for (int t = 0; t < 3; t++) scSprite(t % 3 == 1 ? SPR_V2_CROSS : SPR_V2_TOMB, 7, x + ST[t][0], y + ST[t][1], 2, stone, 2);
        break;
      }
      default: {  // the Beach: its crab (the palms sway)
        const ScKey crab[3] = { { 'r', 0xE86F5F }, { 'w', 0xFFFFFF }, { 'k', 0x1B1B25 } };
        scSprite(SPR_V2_CRAB, 6, x - 6, y + 2, 2, crab, 3);
      }
    }
  }
  twPierPaint();
}

// ----- the island's life (Fedor: "subtle motion. Water flowing, grass and trees swaying. Maybe lights in the
// mansion blinking on some of the rooms or turning off and on") -----
// The foam along the coast: five swells run round the island clockwise, a lap in 35 s (the light checker
// thickens where one passes and thins between); ten little waves drift round it the same way, a lap in 2 min.
struct V2Foam { uint8_t i, j; uint16_t ang; };
static V2Foam *v2Foam = nullptr;
static int v2FoamN = 0;
static void v2FoamList() {  // the sea's cells touching the land, once
  if (v2Foam || !v2IsleGrid()) return;
  for (int pass = 0; pass < 2; pass++) {
    int n = 0;
    for (int j = 0; j < V2_ISLE_N; j++)
      for (int i = 0; i < V2_ISLE_N; i++) {
        if (v2IsleCell(i, j) || !(v2IsleCell(i, j - 1) || v2IsleCell(i, j + 1) || v2IsleCell(i - 1, j) || v2IsleCell(i + 1, j))) continue;
        if (pass) {
          float a = atan2f(j * 4 + 2 - 233.f, i * 4 + 2 - 233.f);
          v2Foam[n] = { (uint8_t)i, (uint8_t)j, (uint16_t)((a < 0 ? a + 6.2831853f : a) * (65536 / 6.2831853f)) };
        }
        n++;
      }
    if (!pass) {
      v2Foam = (V2Foam *)ps_malloc(n * sizeof(V2Foam));
      if (!v2Foam) return;
      v2FoamN = n;
    }
  }
}
// a 2-step sway for something drawn in rows (its top `top` rows shifted by `dx`): the rest stays put
static void v2SwaySprite(const char *const *rows, int nr, int top, int x, int y, int s, int dx, const ScKey *k, int nk) {
  scSprite(rows, top, x + dx, y, s, k, nk);
  scSprite(rows + top, nr - top, x, y + top * s, s, k, nk);
}
// the mansion's windows, from its sprite (x, y, w, h in its cells, and whether it's lit in the sprite)
static const uint8_t V2_WIN[17][5] = { { 19, 12, 2, 2, 1 }, { 36, 12, 2, 2, 0 }, { 8, 14, 3, 4, 1 }, { 17, 19, 2, 3, 1 }, { 21, 19, 2, 3, 1 },
                                       { 35, 19, 2, 3, 1 }, { 39, 19, 2, 3, 1 }, { 8, 21, 3, 4, 0 }, { 46, 22, 2, 3, 1 }, { 50, 22, 2, 3, 0 },
                                       { 17, 25, 2, 3, 1 }, { 21, 25, 1, 3, 1 }, { 35, 25, 2, 3, 0 }, { 39, 25, 2, 3, 1 }, { 8, 27, 3, 3, 1 },
                                       { 46, 28, 2, 3, 1 }, { 50, 28, 2, 3, 1 } };
// some of them live their own lives: a light goes off for a while, a dark room lights up, one bulb flickers
static bool v2WinLit(int w, uint32_t now) {
  static const uint8_t LIVE[6] = { 1, 4, 7, 9, 13, 16 };
  for (int k = 0; k < 6; k++) {
    if (LIVE[k] != w) continue;
    uint32_t cyc = 9000 + k * 1300, c = (now + k * 3700u) % cyc;
    if (w == 4 && c < 700) return (c / 110) % 2;  // the flicker, now and then
    return c < cyc * (V2_WIN[w][4] ? 78u : 30u) / 100;
  }
  return V2_WIN[w][4];
}
static void v2IsleLife(uint32_t now) {
  scScreen();
  scGradeTod = 0;
  v2FoamList();
  float ph = (now % 7000) * (6.2831853f / 7000);
  for (int k = 0; k < v2FoamN; k++) {
    const V2Foam &c = v2Foam[k];
    float f = sinf(c.ang * (5 * 6.2831853f / 65536) - ph);
    bool lit = (c.i + c.j) % 2 ? f > -0.6f : f > 0.55f;
    scCell(c.i * 4, c.j * 4, lit ? V2_SEA_L : V2_SEA);
  }
  static const uint8_t WR[10] = { 221, 225, 219, 226, 222, 220, 225, 219, 224, 221 };
  float lap = (now % 120000) * (6.2831853f / 120000);
  for (int k = 0; k < 10 && v2IsleG; k++) {  // only on open sea: never on the land or the foam
    float a = k * 0.6283185f + (k * 37 % 11) * 0.05f + lap;
    int x = scFl4((int)(233 + WR[k] * cosf(a)) - 6) * 4, y = scFl4((int)(233 + WR[k] * sinf(a))) * 4;
    static const int8_t C[3][2] = { { 0, 0 }, { 4, -4 }, { 8, 0 } };
    bool open = true;
    for (auto &c : C) {
      int i = (x + c[0]) / 4, j = (y + c[1]) / 4;
      open = open && !v2IsleCell(i, j) && !v2IsleCell(i - 1, j) && !v2IsleCell(i + 1, j) && !v2IsleCell(i, j - 1) && !v2IsleCell(i, j + 1);
    }
    if (open)
      for (auto &c : C) scCell(x + c[0], y + c[1], V2_SEA_L);
  }
  // the Forest's pines sway, each on its own beat: their tops a cell one way, back, the other way, back
  int x, y;
  v2IsleSpot(4, x, y);
  static const int8_t PINE[7][2] = { { -30, -6 }, { -8, -14 }, { 14, -4 }, { 36, -12 }, { -18, 10 }, { 6, 8 }, { 28, 12 } };
  static const int8_t SWAY[4] = { 0, 1, 0, -1 };
  ScKey pk[5];
  scTreeKeys(pk, SC_DAY, 0x2F7A3C, SC_DAY.dark);
  for (int t = 0; t < 7; t++)
    v2SwaySprite(SPR_V2_PINE, 12, 5, x + PINE[t][0] - 15, y + PINE[t][1] - 30, 3, 3 * SWAY[((now + t * 410) / 700) % 4], pk, 5);
  // the Beach's palms lean on alternate beats (as on the walk: their top five rows a cell to the left)
  v2IsleSpot(2, x, y);
  ScKey palm[5];
  scPalmKeys(palm, SC_DAY);
  scSprite((now / 1100) % 2 ? V2_PALM_LEAN : SPR_V2_PALM, 12, x - 40, y - 46, 3, palm, 5);
  scSprite((now / 1100 + 1) % 2 ? V2_PALM_LEAN : SPR_V2_PALM, 12, x + 6, y - 40, 3, palm, 5);
  // the Wild patch's tall grass: the blades' tips sway
  static const int8_t TUFT[4][2] = { { -24, -10 }, { 0, -14 }, { -12, 4 }, { 14, 0 } };
  const ScKey wild[2] = { { 'G', 0x2F6A3A }, { 'g', 0x4F9A55 } };
  for (int t = 0; t < 4; t++)
    v2SwaySprite(SPR_V2_TALLGRASS, 5, 2, 233 + TUFT[t][0] - 12, 222 + TUFT[t][1], 2, 2 * SWAY[((now + t * 290) / 520) % 4], wild, 2);
  // the mansion's windows (it stands at its landmark, 2 px a cell)
  v2IsleSpot(3, x, y);
  for (int w = 0; w < 17; w++) {
    bool lit = v2WinLit(w, now);
    for (int cy = 0; cy < V2_WIN[w][3]; cy++)
      for (int cx = 0; cx < V2_WIN[w][2]; cx++) {
        int px = x - 58 + (V2_WIN[w][0] + cx) * 2, py = y - 62 + (V2_WIN[w][1] + cy) * 2;
        scRect(px, py, px + 1, py + 1, !lit ? 0x141024 : cx == 0 && cy == 0 ? 0xFFF0B0 : 0xF0C04A);
      }
  }
}
// a wedge's name tag: its name and the count, in a panel rimmed in the zone's colour
static void v2IsleTag(const char *name, const char *line2, int x, int y, Rgb col) {
  setSize(2);
  int w = textW(name, 2);
  if (textW(line2, 2) > w) w = textW(line2, 2);
  w += 16;
  V2_HOVER(x - w / 2, y, w, 44);  // under the mouse: lifted (hover_v2.h)
  uint16_t rim = sc565(col);
  gfx->fillRoundRect(x - w / 2 - 3, y - 3, w + 6, 50, 10, rim);
  gfx->fillRoundRect(x - w / 2, y, w, 44, 8, UI_WHITE);
  gfx->setTextColor(UI_INK);
  setCur(x - textW(name, 2) / 2, y + 6);
  printT(name);
  gfx->setTextColor(sc565(scMix(col, 0x1B1B25, .35)));
  setCur(x - textW(line2, 2) / 2, y + 26);
  printT(line2);
}
// tag k's two lines and where it sits: a wedge's name and count, or the Wild patch's "Wild Encounter" on two
// lines (one in Japanese and Korean)
static void v2IsleTagText(int k, char *l1, char *l2, int n, int &x, int &y) {
  if (k < 5) {
    float a = (-90 + k * 72) * 0.0174533f;
    uint8_t z = V2_WEDGE_ZONE[k];
    snprintf(l1, n, "%s", T(V2_ZONE_NAMES[z]));
    snprintf(l2, n, "%d/%d", pet.zoneCaught(z), zoneSize(z));
    x = (int)(233 + 120 * cosf(a));
    y = (int)(233 + 120 * sinf(a)) + 4;
    return;
  }
  snprintf(l1, n, "%s", T(S_WILD));  // Knomi Tower's plan (Fedor): "Wild" "All", the way the zones show a count
  snprintf(l2, n, "%s", T(S_ALL));
  x = 233;
  y = 240;
}
static void v2Travel2();  // the second page: Knomi Tower's islet (tower_v2.h)
static void v2Travel() {
  static const Rgb TAG[6] = { 0xF0C04A, 0x9A86D8, 0x4F93C4, 0xB070F0, 0x58B868, 0xE8503A };
  if (v2IslePage) {
    v2Travel2();
    return;
  }
  v2Cached(0x3D000000, v2PaintIsle, 0);
  v2IsleLife(millis());
  char l1[32], l2[32];
  for (int k = 0; k < 6; k++) {
    int x, y;
    v2IsleTagText(k, l1, l2, sizeof(l1), x, y);
    v2IsleTag(l1, l2, x, y, TAG[k]);
  }
  char b[32];
  v2SignIn(v2Caps(T(S_ISLAND), b, sizeof(b)), 18, C565(0xc0, 0x84, 0x10), C565(0xf0, 0xc0, 0x4a));  // Knomi Island (Fedor)
  v2PageArrow(430, V2_PAGE_ARROW_Y, 1);  // on to Knomi Tower, along the pier
  v2CloseX();
}
// the box round a pick's tag (a zone, or ZONE_COUNT for the Wild patch), for the press ring
static void v2IsleTagBox(int pick, int16_t &bx, int16_t &by, int16_t &bw) {
  int k = 5;
  for (int i = 0; i < 5; i++)
    if (V2_WEDGE_ZONE[i] == pick) k = i;
  char l1[32], l2[32];
  int x, y;
  v2IsleTagText(k, l1, l2, sizeof(l1), x, y);
  setSize(2);
  int w = textW(l1, 2) > textW(l2, 2) ? textW(l1, 2) : textW(l2, 2);
  bw = w + 16;
  bx = x - bw / 2;
  by = y;
}
// which pick a tap on the island is: a zone, the Wild patch (ZONE_COUNT), or the sea (-1)
static int v2IsleAt(int16_t x, int16_t y) {
  if (v2IslePage || !v2IsleLand(x, y)) return -1;
  int r = v2IsleRegion(x, y);
  return r == 5 ? ZONE_COUNT : V2_WEDGE_ZONE[r];
}
// behind a zone's grid: its scene, dimmed half way, so the Pokemon stand out (from the scene buffer)
static void v2PaintZoneBack(int key) {
  scPaint(key / 4, key % 4);
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const uint16_t tint = sc565(0x120E22);
  for (int i = 0; i < V2_W * V2_W; i++) fb[i] = v2Blend(fb[i], tint, 128);
}
static void v2ZoneBack(uint8_t zone) {
  uint8_t tod = zone == Z_CAVE || zone == Z_MANSION ? 0 : v2HourTod();
  v2Cached(0x3C000000 | (zone * 4 + tod), v2PaintZoneBack, zone * 4 + tod);
}

// ----- the Game Boy: Games and Play together (Fedor: "maybe it needs to be a gameboy in the background instead of a
// TV. Pokemon blue and red were gameboy games and that screen has a distinct color. The cartridges would also make
// more sense"; it was an old TV with snow) -----
// A Game Boy filling the round screen, close up: its grey body, the dark bezel with the two coloured lines and the
// red power light, the big olive screen with its dot matrix faint on it, and at the bottom the D-pad and the A and B
// buttons either side of the X. Painted once and kept (the TV's snow, new every second, is gone). On its screen, the
// cartridges in the colours the games came in, each label a little picture from its game: its colours and a
// starter, quick to draw (they were the games' own scenes at half size, and opening took seconds on the Knomi).
// One tap on a cartridge starts its game, as V1's windows did.
#define V2_GB_X0 40  // its screen: the cartridges sit inside it, 11 px in at least (Fedor: "so they sit in the screen
#define V2_GB_Y0 84  // and not over top of it")
#define V2_GB_X1 426
#define V2_GB_Y1 392
#define V2_LBL_W 79  // a label's picture (AW x AH)
#define V2_LBL_H 42
#define V2_GB_KEY 0x3A000000
static void v2PaintGb(int) {
  const uint16_t body = sc565(0xCDC9C2), bodyL = sc565(0xE2DED8), bodyD = sc565(0xA39E97), ink = sc565(0x1B1B25);
  gfx->fillScreen(body);
  gfx->fillRect(0, 0, V2_W, 14, bodyL);  // its top edge, lit, and the ridge under it
  gfx->fillRect(0, 14, V2_W, 4, bodyD);
  // the bezel: dark, its bottom right corner rounder, as the Game Boy's is
  const int bx0 = 22, by0 = 62, bx1 = 444, by1 = 408;
  gfx->fillRoundRect(bx0 - 4, by0 - 4, bx1 - bx0 + 8, by1 - by0 + 8, 26, ink);
  gfx->fillRoundRect(bx0, by0, bx1 - bx0, by1 - by0, 22, sc565(0x5E5C74));
  gfx->fillCircle(bx1 - 60, by1 - 60, 60, sc565(0x5E5C74));  // (the round corner's fill, then its rim)
  gfx->fillRect(bx1 - 60, by0 + 22, 60, by1 - by0 - 82, sc565(0x5E5C74));
  gfx->fillRect(bx0 + 22, by1 - 60, bx1 - bx0 - 82, 60, sc565(0x5E5C74));
  gfx->fillRect(bx0, by0, bx1 - bx0, 4, sc565(0x7A7890));  // lit along its top
  // the two lines across its top, a gap where the words would be
  for (int k = 0; k < 2; k++) {
    uint16_t c = k ? sc565(0x2A2A6E) : sc565(0xB83A78);
    gfx->fillRect(60, 70 + k * 6, 110, 4, c);
    gfx->fillRect(296, 70 + k * 6, 110, 4, c);
  }
  gfx->fillCircle(31, 214, 5, ink);  // the power light
  gfx->fillCircle(31, 214, 3, sc565(0xE0463A));
  gfx->fillRect(30, 212, 2, 2, sc565(0xFFB0A0));
  // the screen: olive, its dot matrix faint, a shadow along its top and left
  const uint16_t lcd = sc565(0xA6BA3C), dot = sc565(0x9CB036), sh = sc565(0x869C2C);
  gfx->fillRect(V2_GB_X0, V2_GB_Y0, V2_GB_X1 - V2_GB_X0, V2_GB_Y1 - V2_GB_Y0, lcd);
  for (int y = V2_GB_Y0; y < V2_GB_Y1; y += 4) gfx->fillRect(V2_GB_X0, y + 2, V2_GB_X1 - V2_GB_X0, 2, dot);
  for (int x = V2_GB_X0; x < V2_GB_X1; x += 4) gfx->fillRect(x + 2, V2_GB_Y0, 2, V2_GB_Y1 - V2_GB_Y0, dot);
  gfx->fillRect(V2_GB_X0, V2_GB_Y0, V2_GB_X1 - V2_GB_X0, 4, sh);
  gfx->fillRect(V2_GB_X0, V2_GB_Y0, 4, V2_GB_Y1 - V2_GB_Y0, sh);
  // the D-pad, the A and B buttons, down at the rim either side of the X, the round screen's edge cutting them, so
  // they read as the Game Boy's and not as buttons to press (Fedor)
  const uint16_t pad = sc565(0x2C2C3A), padL = sc565(0x4A4A5C);
  gfx->fillRoundRect(126, 458, 48, 16, 3, pad);  // (further off: "I find them a bit distracting", Fedor; only the
  gfx->fillRoundRect(142, 442, 16, 48, 3, pad);  // D-pad's top shows)
  gfx->fillRect(142, 442, 16, 4, padL);
  gfx->fillRect(126, 458, 4, 16, padL);
  gfx->fillCircle(150, 466, 4, padL);
  const uint16_t btn = sc565(0xB83A78), btnD = sc565(0x7E2452), btnL = sc565(0xE070A8);
  static const int16_t AB[2][2] = { { 314, 472 }, { 350, 454 } };
  for (auto &b : AB) {
    gfx->fillCircle(b[0], b[1] + 2, 15, btnD);
    gfx->fillCircle(b[0], b[1], 14, btn);
    gfx->fillRect(b[0] - 6, b[1] - 10, 6, 4, btnL);
  }
}
// a cartridge's title in capitals on up to three lines, each no wider than its label: broken at a line break
// in the title ("\n"), or at the space nearest the middle, or where two joined words meet ("Poke" "Dungeon"),
// or in Japanese where the kana change (hiragana to katakana); a part still too wide is broken again, and a
// word too wide on its own is cut in half
#define V2_CART_TXT_W 84  // 7 letters at size 2 (their ink is 82 px)
static int v2CartCut(const char *s, int n, bool joinedOnly) {  // where to break s[0..n), or -1
  int best = -1;
  for (int i = 1; i < n; i++) {
    uint8_t c = (uint8_t)s[i], p = (uint8_t)s[i - 1];
    bool cut = !gCjkFont && c >= 'A' && c <= 'Z' && p >= 'a' && p <= 'z';  // two joined words
    if (!joinedOnly && !cut) {
      cut = c == ' ';
      if (gCjkFont && c == 0xE3 && i >= 3 && i + 2 < n && (uint8_t)s[i - 3] == 0xE3) {  // two kana in a row
        int a = (((uint8_t)s[i - 2] & 0x3F) << 6) | ((uint8_t)s[i - 1] & 0x3F), b = (((uint8_t)s[i + 1] & 0x3F) << 6) | ((uint8_t)s[i + 2] & 0x3F);
        cut = a >= 0x040 && b >= 0x040 && a < 0x100 && b < 0x100 && (a < 0x0A0) != (b < 0x0A0);  // hiragana, katakana
      }
    }
    if (cut && (best < 0 || abs(2 * i - n) < abs(2 * best - n))) best = i;
  }
  return best;
}
static int v2CartLines(const char *title, char out[3][32]) {
  char part[4][32];
  int np = 0;
  setSize(2);  // (Japanese and Korean are measured in the font as it's set)
  const char *nl = strchr(title, '\n');
  if (nl) {
    snprintf(part[np++], 32, "%.*s", (int)(nl - title), title);
    snprintf(part[np++], 32, "%s", nl + 1);
  } else {
    snprintf(part[np++], 32, "%s", title);
  }
  for (int i = 0; i < np && np < 3; i++) {
    int n = (int)strlen(part[i]), cut, skip = 0;
    if (textW(part[i], 2) <= V2_CART_TXT_W) {  // it fits: only a one-line name of two joined words breaks ("PokeRun")
      if (np > 1 || (cut = v2CartCut(part[i], n, true)) < 0) continue;
    } else if ((cut = v2CartCut(part[i], n, false)) >= 0) {
      skip = part[i][cut] == ' ';
    } else {  // one word too wide: cut in half
      cut = (n + 1) / 2;
      if (gCjkFont)
        while (cut < n && ((uint8_t)part[i][cut] & 0xC0) == 0x80) cut++;  // on a character's first byte
    }
    for (int k = np; k > i + 1; k--) memcpy(part[k], part[k - 1], 32);
    snprintf(part[i + 1], 32, "%s", part[i] + cut + skip);
    part[i][cut] = 0;
    np++;
    i--;  // the first half again
  }
  for (int i = 0; i < np; i++) v2Caps(part[i], out[i], 32);
  return np;
}

// the same for a title in Latin letters only (PokeRun, PikaSurf, PokeDungeon): the classic font in every
// language, so they look the same in Japanese and Korean. Returns whether to put the language's font back.
static bool v2CartFont(const char *title) {
  for (const char *p = title; *p; p++)
    if ((uint8_t)*p >= 0x80) return false;
  return classicFontBegin();
}

// a cartridge (cart_label()), its top left at (x, y): the grey shell in 5 px cells with its notched corner, the
// grooves, a label well; the label in the game's colour with the title and the picture in a dark frame
static void v2CartRun(int x, int y, int cy, int cx0, int cx1, uint16_t c) {
  if (cx1 >= cx0) gfx->fillRect(x + cx0 * 5, y + cy * 5, (cx1 - cx0 + 1) * 5, 5, c);
}
// (21 x 26 cells: a cell narrower each side and one shorter than it was, so they stand apart; Fedor: "make the
// cartirages just a bit smaller so they dont touch each other". The label keeps its 85 px, for the titles)
static bool v2CartIn(int cx, int cy) { return cx >= 0 && cx < 21 && cy >= 0 && cy < 26 && cx - 17 <= cy; }
static void v2Cart(int x, int y, const uint16_t *pic, const char *title, Rgb shell) {
  V2_HOVER(x, y, 105, 130);  // under the mouse: lifted (hover_v2.h)
  // the shell in its game's colour (Fedor: "edit the colors of each cartridge to make them different colors like they
  // were"), its label a darker shade of it
  const Rgb col = scMix(shell, 0x1B1B25, .32);
  const uint16_t body = sc565(shell), shade = sc565(scMix(shell, 0x1B1B25, .22)), lite = sc565(scMix(shell, 0xFFFFFF, .35)),
                 groove = sc565(scMix(shell, 0x1B1B25, .12)), ink = sc565(0x1B1B25);
  for (int cy = -1; cy <= 26; cy++) {  // the shell and its dark outline, row by row in runs
    int run = 0;
    uint16_t runC = 0;
    bool on = false;
    for (int cx = -1; cx <= 22; cx++) {
      bool have = false;
      uint16_t c = 0;
      if (cx <= 21) {
        if (v2CartIn(cx, cy)) have = true, c = (cx == 0 || cy == 0) ? lite : (cx == 20 || cy == 25) ? shade : body;
        else if (v2CartIn(cx + 1, cy) || v2CartIn(cx - 1, cy) || v2CartIn(cx, cy + 1) || v2CartIn(cx, cy - 1)) have = true, c = ink;
      }
      if (on && (!have || c != runC)) v2CartRun(x, y, cy, run, cx - 1, runC), on = false;
      if (have && !on) on = true, run = cx, runC = c;
    }
  }
  v2CartRun(x, y, 1, 2, 12, groove);  // the grooves across the top
  v2CartRun(x, y, 4, 2, 12, lite);
  for (int cy = 2; cy <= 3; cy++) v2CartRun(x, y, cy, 1, 1, groove), v2CartRun(x, y, cy, 13, 13, lite);
  for (int cy = 1; cy <= 3; cy++) v2CartRun(x, y, cy, 14, 16, cy % 2 ? groove : lite);
  v2CartRun(x, y, 5, 1, 19, shade);  // the label's well: its edges (the label covers the rest)
  for (int cy = 6; cy <= 23; cy++) v2CartRun(x, y, cy, 1, 1, shade), v2CartRun(x, y, cy, 19, 19, lite);
  v2CartRun(x, y, 23, 2, 19, lite);
  v2CartRun(x, y, 24, 9, 11, shade);  // the arrow
  v2CartRun(x, y, 25, 10, 10, shade);
  const int lx0 = x + 10, ly0 = y + 30;
  gfx->fillRect(lx0, ly0, 85, 85, sc565(col));
  char lines[3][32];
  bool cjk = v2CartFont(title);
  int n = v2CartLines(title, lines);
  static const int8_t LY[3][3] = { { 12 }, { 4, 21 }, { 2, 18, 34 } };
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  for (int i = 0; i < n; i++) {
    setCur(x + (gCjkFont ? 52 : 53) - textW(lines[i], 2) / 2, ly0 + LY[n - 1][i]);  // the ink centred on the label
    printT(lines[i]);
  }
  classicFontEnd(cjk);
  int py = ly0 + (n == 3 ? 51 : 38), h = n == 3 ? 29 : V2_LBL_H;  // three lines leave room for less picture
  gfx->fillRect(lx0 + 2, py - 1, V2_LBL_W + 2, h + 2, sc565(scMix(col, 0x1B1B25, .35)));
  if (pic) gfx->draw16bitRGBBitmap(lx0 + 3, py, (uint16_t *)pic + (V2_LBL_H - h) * V2_LBL_W, V2_LBL_W, h);
}

// ----- the labels' pictures: starters in the games' backgrounds (Fedor: "they can lean towards a starter on the
// labels. No need to make a million of these") -----
// Eight: the five games in the cartridges' order, then Play together's three. Each is painted at the canvas's
// top left (not shown): the game's background at half size, then what stands in it at 1x. A set is painted when
// its menu opens and the hour's light (or Berry Drop's title) has changed since: one set all day, near enough.
static uint16_t *v2LblArt = nullptr;
static uint16_t *v2Lbl(int i) { return v2LblArt ? v2LblArt + i * V2_LBL_W * V2_LBL_H : nullptr; }
static bool v2LblStart() {
  if (!v2LblArt) v2LblArt = (uint16_t *)ps_malloc(8 * V2_LBL_W * V2_LBL_H * sizeof(uint16_t));
  return v2LblArt != nullptr;
}
static void v2LblKeep(int i) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer(), *a = v2Lbl(i);
  for (int j = 0; j < V2_LBL_H; j++) memcpy(a + j * V2_LBL_W, fb + j * V2_W, V2_LBL_W * sizeof(uint16_t));
}
// a Pokemon at 1x from its Pokedex thumbnail (its sprite's first idle frame), its feet at (cx, feet); sil: its
// silhouette
static void v2LblThumb(int16_t dex, int cx, int feet, bool sil) {
  const uint8_t *b = dex > 0 && dex <= 151 ? thumbs.get(dex) : nullptr;
  if (!b) return;
  int w = b[0], h = b[1], top = h, bot = -1;
  const uint8_t *d = b + 3 + b[2] * 2;
  for (int i = 0; i < w * h; i++)
    if (d[i] != 0xFF) {
      if (i / w < top) top = i / w;
      bot = i / w;
    }
  if (bot < 0) return;
  int bh = bot - top + 1;
  drawThumbFit(b, cx, feet - bh + bh / 2, 400, 400, 1, sil, 0, nullptr);
}
static void v2LblOval(int x0, int y0, int x1, int y1, Rgb col) {  // PIL's ellipse() in its box, near enough
  float cx = (x0 + x1 + 1) / 2.0f, cy = (y0 + y1 + 1) / 2.0f, rx = (x1 - x0 + 1) / 2.0f, ry = (y1 - y0 + 1) / 2.0f;
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++) {
      float dx = (x + 0.5f - cx) / rx, dy = (y + 0.5f - cy) / ry;
      if (dx * dx + dy * dy <= 1.0f && x >= 0 && y >= 0) ((uint16_t *)gfx->getFramebuffer())[y * V2_W + x] = sc565(col);
    }
}
// a label's backdrop, at its own size at the canvas's top left: a sky over the ground (ground 0: none), in 2 px steps
static void v2LblSky(Rgb top, Rgb bot, int groundY, Rgb ground, Rgb groundD) {
  for (int y = 0; y < V2_LBL_H; y += 2)
    gfx->fillRect(0, y, V2_LBL_W + 1, 2, sc565(y >= groundY && groundY ? (y < groundY + 4 ? ground : groundD) : scMix(top, bot, y / (float)V2_LBL_H)));
}
// rays from behind (Who's That?): a burst of lighter wedges
static void v2LblBurst(Rgb base, Rgb ray) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  for (int y = 0; y < V2_LBL_H; y += 2)
    for (int x = 0; x < V2_LBL_W; x += 2) {
      float a = atan2f(y - 24.0f, x - 36.0f);
      uint16_t c = sc565(((int)((a + 3.1416f) * 2.55f)) % 2 ? ray : base);
      fb[y * V2_W + x] = fb[y * V2_W + x + 1] = fb[(y + 1) * V2_W + x] = fb[(y + 1) * V2_W + x + 1] = c;
    }
}
static int16_t v2LblKey[2] = { -1, -1 };  // what each set was painted for (-1: not yet)
static void v2PaintGameLabels() {
  if (!v2LblStart()) return;
  // Berry Drop: a clearing, Bulbasaur under three falling berries. A title on three lines leaves room for the
  // picture's bottom 30 rows only: two berries, lower down
  char l[3][32];
  bool cjk = v2CartFont(gameLabel(0));
  int lo = v2CartLines(gameLabel(0), l) == 3 ? 13 : 0;
  classicFontEnd(cjk);
  int16_t key = lo ? 1 : 0;
  if (key == v2LblKey[0]) return;
  v2LblKey[0] = thumbs.loaded ? key : -1;  // (no thumbnails yet: painted again next time)
  v2LblSky(0x7FBDE6, 0xD8EEE8, 32, 0x79BD4F, 0x5A9A3A);
  v2LblThumb(1, 40, 41, false);
  static const char *const *const BERRY[3] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP };
  static const int8_t BXY[3][2] = { { 14, 2 }, { 58, 8 }, { 30, -4 } };
  for (int k = 0; k < (lo ? 2 : 3); k++) drawMap(BERRY[k], HOME_ICON_N, BXY[k][0], BXY[k][1] + lo, 1, false);
  v2LblKeep(0);
  // Who's That?: Pikachu's silhouette and a "?" on the blue burst
  v2LblBurst(0x2A7AD8, 0x4F9EEA);
  v2LblThumb(25, 36, 40, true);
  cjk = classicFontBegin();
  gfx->setTextColor(UI_WHITE);
  setSize(4);  // (even: crisp on the Knomi)
  setCur(56, 8);
  printT("?");
  classicFontEnd(cjk);
  v2LblKeep(1);
  // PokeRun: the forest's edge, Charmander over a log, speed lines behind it
  v2LblSky(0x9FD6F0, 0xD8EEE8, 30, 0x8A6A44, 0x6E5034);
  for (int x = 0; x < V2_LBL_W; x += 10) gfx->fillRect(x, 22 + (x / 10) % 2 * 2, 8, 8, sc565(0x3C8A4C));  // the pines' feet
  gfx->fillRect(50, 26, 19, 6, sc565(0x8A5A34));
  gfx->fillRect(50, 26, 19, 2, sc565(0xA87048));
  v2LblThumb(4, 32, 28, false);
  for (int k = 0; k < 3; k++) gfx->fillRect(4 + k * 4, 12 + k * 4, 10, 2, UI_WHITE);
  v2LblKeep(2);
  // PikaSurf: the sea, Pikachu on his board, a wake behind
  v2LblSky(0x7FBDE6, 0xBFE6F5, 20, 0x4F93C4, 0x3A78A8);
  for (int k = 0; k < 4; k++) gfx->fillRect(6 + k * 18, 28 + (k % 2) * 6, 10, 2, sc565(0x9FD6F0));
  v2LblOval(22, 33, 56, 40, 0xF5863D);  // the board
  v2LblOval(24, 33, 54, 36, 0xFFB070);
  v2LblThumb(25, 39, 36, false);
  for (int k = 0; k < 4; k++) gfx->fillRect(6 + k * 3, 38 - k, 5, 2, UI_WHITE);
  v2LblKeep(3);
  // PokeDungeon: a stone wall over flagstones, Squirtle by a berry
  v2LblSky(0x55516E, 0x55516E, 18, 0x9A94B0, 0x847E9C);
  for (int x = 0; x < V2_LBL_W; x += 12) gfx->fillRect(x, 6 + (x / 12) % 2 * 6, 10, 4, sc565(0x6E6A84));  // the wall's stones
  for (int x = 6; x < V2_LBL_W; x += 16) gfx->fillRect(x, 18, 2, 24, sc565(0x6E6A84));                  // the floor's joints
  v2LblThumb(7, 40, 40, false);
  drawMap(SPR_BERRY_ORAN, HOME_ICON_N, 60, 24, 1, false);
  v2LblKeep(4);
}
static void v2PaintPlayLabels() {
  if (!v2LblStart()) return;
  if (v2LblKey[1] == 0) return;
  v2LblKey[1] = thumbs.loaded ? 0 : -1;
  // Poke Ball Volley: the court, the ball over Charmander
  v2LblSky(0x7FBDE6, 0xD8EEE8, 30, 0x79BD4F, 0x5A9A3A);
  gfx->fillRect(38, 14, 2, 18, UI_WHITE);  // the net
  drawMap(SPR_HOME_BALL, HOME_ICON_N, 22, 4, 1, false);
  v2LblThumb(4, 60, 40, false);
  v2LblKeep(5);
  // Who's That? Showdown: Bulbasaur and Squirtle face to face on the pink burst
  v2LblBurst(0xC75F80, 0xE58AA8);
  v2LblThumb(1, 22, 40, false);
  v2LblThumb(7, 60, 40, false);
  v2LblKeep(6);
  // PikaSurf together: Squirtle alongside Pikachu
  v2LblSky(0x7FBDE6, 0xBFE6F5, 20, 0x4F93C4, 0x3A78A8);
  v2LblOval(22, 33, 56, 40, 0xF5863D);
  v2LblOval(24, 33, 54, 36, 0xFFB070);
  v2LblThumb(25, 39, 36, false);
  v2LblThumb(7, 62, 38, false);
  v2LblKeep(7);
}

// ----- the two Game Boys -----
static const int16_t V2_CART_XY[5][2] = { { 56, 100 }, { 180, 100 }, { 304, 100 }, { 118, 248 }, { 242, 248 } };
static const uint8_t V2_CART_GAME[5] = { 0, 1, 2, 4, 3 };  // V1's games in the cartridges' order (Berry Drop, Who's That?, PokeRun, PikaSurf, PokeDungeon)
// the colours the games came in: Green, Silver, Red, Yellow, Blue; Play together's Gold, Crystal, Yellow
static const Rgb V2_CART_COL[5] = { 0x3E9A52, 0xB4BAC8, 0xD8403A, 0xF2C230, 0x3A6FC8 };
static const int16_t V2_PLAY_XY[3][2] = { { 56, 150 }, { 180, 150 }, { 304, 150 } };
static const Rgb V2_PLAY_COL[3] = { 0xC9A43C, 0x5AC8D8, 0xF2C230 };
static void v2GbBack(const char *title) {
  char b[40];
  v2Cached(V2_GB_KEY, v2PaintGb, 0);
  v2Sign(v2Caps(title, b, sizeof(b)), 24);
}
static void v2GamesTv() {
  v2GbBack(T(S_GAMES));
  for (int k = 0; k < 5; k++) v2Cart(V2_CART_XY[k][0], V2_CART_XY[k][1], v2Lbl(k), gameLabel(V2_CART_GAME[k]), V2_CART_COL[k]);
  v2CloseX();
}
static void v2PlayTv() {
  v2GbBack(T(S_PLAY_TOGETHER));
  char w[48];
  snprintf(w, sizeof(w), T(S_PLAY_WITH_FMT), buddyName());
  gfx->setTextColor(sc565(0x2C4A18));  // dark green on the olive screen
  setSize(2);
  setCur(centerX(w, 2), 100);
  printT(w);
  static const StrId TITLE[3] = { S_CART_VOLLEY, S_CART_SHOWDOWN, S_GAME_SURF };
  for (int k = 0; k < 3; k++) v2Cart(V2_PLAY_XY[k][0], V2_PLAY_XY[k][1], v2Lbl(5 + k), T(TITLE[k]), V2_PLAY_COL[k]);
  v2CloseX();
}
// the cartridge under a tap (its outline counts), or -1
static int v2CartAt(int16_t x, int16_t y, const int16_t (*xy)[2], int n) {
  for (int k = 0; k < n; k++)
    if (x >= xy[k][0] - 5 && x < xy[k][0] + 110 && y >= xy[k][1] - 5 && y < xy[k][1] + 135) return k;
  return -1;
}
static int v2GameAt(int16_t x, int16_t y) {  // V1's game number
  int k = v2CartAt(x, y, V2_CART_XY, 5);
  return k < 0 ? -1 : V2_CART_GAME[k];
}
static int v2CartOfGame(int g) {
  for (int k = 0; k < 5; k++)
    if (V2_CART_GAME[k] == g) return k;
  return 0;
}

// ----- friends: a buddy's view, Party Boost, the visitor, trade (batch3.py friend_stats(), buddy_menu(),
// party_boost(), trade()) -----
static const uint16_t V2_HP_COL = C565(0x58, 0xb8, 0x68);
// another's Pokemon standing on the rug, its feet at y, as big as fits under the board (at most 3x)
static void v2FriendMon(PmdMon &mon, int feet) {
  if (!mon.loaded) return;
  uint8_t s = pmdFitScale(mon, PMD_IDLE, 220, feet - V2_BOARD_Y - 34);
  gPmdForceScale = s < 3 ? s : 3;
  v2PetShadow(CX, feet - 4, 44);
  drawPmdActM(mon, PMD_IDLE, CX, feet, millis(), true, false, 3);
  gPmdForceScale = 0;
}
// a friend's stats (friend_stats()): its name and level on the board, it on the rug, its four stats in a
// panel (one an older version doesn't send shows "?"); the caller adds the panel's last line at 342
static void v2FriendStats(PmdMon &mon, const char *name, bool shiny, uint8_t level, uint16_t hp, uint16_t atk, uint16_t def,
                          uint16_t spe) {
  v2MenuBackdrop();
  char head[40];
  snprintf(head, sizeof(head), T(S_NAME_FMT), shiny ? "*" : "", name, level);
  v2Sign(head);
  v2FriendMon(mon, 196);
  v2Panel(V2_CARD_X0, 202, V2_CARD_X1, 372, 10);  // (the card's width and its stat rows)
  int labW = v2LabW(T(S_STAT_HP), T(S_STAT_ATK), T(S_STAT_DEF), T(S_STAT_SPE));
  v2StatRow(214, T(S_STAT_HP), hp, 260, V2_HP_COL, labW, false, false, !hp);
  v2StatRow(244, T(S_STAT_ATK), atk, 260, V2_STAT_COL[0], labW, false, false, !atk);
  v2StatRow(274, T(S_STAT_DEF), def, 260, V2_STAT_COL[1], labW, false, false, !def);
  v2StatRow(304, T(S_STAT_SPE), spe, 260, V2_STAT_COL[2], labW, false, false, !spe);
}
static void v2FriendLine(const char *s, int x = -1) {
  gfx->setTextColor(V2_INK_MUTED);
  setSize(2);
  setCur(x < 0 ? centerX(s, 2) : x, 342);
  printT(s);
}
// a buddy's first page: its stats and how much of the Pokedex it has
static void v2BuddyStats(int caught) {
  const BuddyPeer &p = bPeers[bCur];
  v2FriendStats(buddyPmd[bCur], buddyName(), p.shiny, p.level, p.hp, p.atk, p.def, p.spe);
  char b[32];
  snprintf(b, sizeof(b), T(S_POKEDEX_FMT), caught);
  v2FriendLine(b);
  drawPageArrows(0, 2, V2_PAGE_ARROW_Y);
  v2PageOf(2, 0);
  v2CloseX();
}
// its second page: Battle, Trade and Play in a panel over the home, dimmed (buddy_menu()). The buttons fill
// V1's rows, where its taps are; Trade is grey while there's nothing to trade
#define V2_BUDDY_ROW_Y 172  // V1's BUDDY_ROW_Y, _STEP and _H, defined further down
#define V2_BUDDY_ROW_STEP 58
#define V2_BUDDY_ROW_H 48
static void v2BuddyActions(bool tradeOk) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const uint16_t tint = sc565(0x120E22);
  for (int i = 0; i < V2_W * V2_W; i++) fb[i] = v2Blend(fb[i], tint, 159);  // 62% toward the night tint
  v2Panel(90, 104, 376, 350, 12);
  char b[48];
  snprintf(b, sizeof(b), T(S_BUDDY_HERE_FMT), buddyName());
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(b, 2), 126);
  printT(b);
  static const StrId LAB[3] = { S_BUDDY_BATTLE, S_BUDDY_TRADE, S_PLAY_BTN };
  static const uint16_t COL[3] = { C565(0xe8, 0x50, 0x3a), C565(0x4f, 0x93, 0xc4), C565(0xc7, 0x5f, 0x80) };
  for (int i = 0; i < 3; i++) {
    int y = V2_BUDDY_ROW_Y + i * V2_BUDDY_ROW_STEP;
    bool off = i == 1 && !tradeOk;
    char cap[32];
    v2LabelButton(CX - 127, y + 3, CX + 127, y + V2_BUDDY_ROW_H - 3, v2Caps(T(LAB[i]), cap, sizeof(cap)),
                  off ? C565(0x8e, 0x8c, 0x96) : COL[i], off ? C565(0xd8, 0xd2, 0xbd) : UI_WHITE);
  }
  drawPageArrows(1, 2, V2_PAGE_ARROW_Y);
  v2PageOf(2, 1);
  v2CloseX();
}
// Party Boost (party_boost()): who's here, each a row that opens its stats, then what the boost gives. The rows
// are V1's, where its taps are (300 wide, as tall as V1 makes them for the party's size)
static void v2Party(const int8_t *who, int n, int y0, int step) {
  v2MenuBackdrop();
  char b[40];
  v2Sign(v2Caps(T(S_PARTY_BOOST), b, sizeof(b)));
  const int x0 = CX - 150, h = step - 6, by = y0 + n * step + 14;
  v2Panel(x0 - 12, y0 - 12, x0 + 312, by + 54, 10);
  setSize(2);
  for (int r = 0; r < n; r++) {
    int y = y0 + r * step;
    bool visitor = who[r] < 0;
    gfx->fillRoundRect(x0, y, 300, h, 8, C565(0xee, 0xf2, 0xf8));
    gfx->fillRect(x0 + 6, y, 288, 4, UI_WHITE);
    v2LblThumb(visitor ? visitDex : bPeers[who[r]].dex, x0 + 30, y + h - 5, false);
    char l2[24];  // its level, or how long the visitor stays
    if (visitor) snprintf(l2, sizeof(l2), T(S_VISIT_MIN_FMT), (unsigned)(pet.visitMin ? pet.visitMin : 1));
    else snprintf(l2, sizeof(l2), T(S_LVL_FMT), bPeers[who[r]].level);
    gfx->setTextColor(UI_INK);
    if (h >= 50) {  // two lines
      setCur(x0 + 58, y + 9);
      printT(partyRowName(r));
      gfx->setTextColor(V2_INK_MUTED);
      setCur(x0 + 58, y + 29);
      printT(l2);
    } else {  // a full party's shorter rows: the second on the right
      setCur(x0 + 58, y + h / 2 - 7);
      printT(partyRowName(r));
      gfx->setTextColor(V2_INK_MUTED);
      setCur(x0 + 268 - textW(l2, 2), y + h / 2 - 7);
      printT(l2);
    }
    gfx->fillTriangle(x0 + 278, y + h / 2 - 9, x0 + 288, y + h / 2, x0 + 278, y + h / 2 + 9, C565(0x3a, 0x4a, 0x5a));
  }
  const uint16_t green = C565(0x3c, 0x8a, 0x4c);
  char v[24];
  gfx->setTextColor(UI_INK);
  setCur(x0 + 12, by);
  printT(T(S_BOOST_ENERGY));
  snprintf(v, sizeof(v), "-%d%%", PARTY_ENERGY_SAVE_PCT);
  gfx->setTextColor(green);
  setCur(x0 + 288 - textW(v, 2), by);
  printT(v);
  snprintf(b, sizeof(b), "%s %s %s", T(S_STAT_ATK), T(S_STAT_DEF), T(S_STAT_SPE));
  gfx->setTextColor(UI_INK);
  setCur(x0 + 12, by + 28);
  printT(b);
  snprintf(v, sizeof(v), "+%d", PARTY_STAT_BONUS);
  gfx->setTextColor(green);
  setCur(x0 + 288 - textW(v, 2), by + 28);
  printT(v);
  v2CloseX();
}
// the visitor's stats: a wild one of its level; the last line, the candy it brought and how long it stays
static void v2VisitorStats(uint8_t lvl, uint16_t hp, uint16_t atk, uint16_t def, uint16_t spe) {
  v2FriendStats(visitPmd, dexName(visitDex), visitPmd.shiny, lvl, hp, atk, def, spe);
  char m[24], line[40];
  snprintf(m, sizeof(m), T(S_VISIT_MIN_FMT), (unsigned)(pet.visitMin ? pet.visitMin : 1));
  snprintf(line, sizeof(line), "x1   %s", m);
  setSize(2);
  int x = CX - (24 + textW(line, 2)) / 2;
  drawMap(SPR_CANDY, HOME_ICON_N, x, 336, 1, false);
  v2FriendLine(line, x + 24);
  v2CloseX();
}

// ----- trade -----
// picking what to give: your forms on white in a panel, 4 x 4 squares of 76 (V1's are 80, under the X: a tap
// in a V2 square is moved into V1's, v2TradeTap()); the page as dots and the side buttons
#define V2_TRADE_X 81
#define V2_TRADE_Y 70
#define V2_TRADE_CELL 76
static void v2TradePick(int page, int pages, const uint8_t *buddyReg) {
  v2MenuBackdrop();
  char b[48];
  v2Sign(v2Caps(T(S_TRADE_PICK), b, sizeof(b)));
  v2Panel(V2_TRADE_X - 6, V2_TRADE_Y - 6, V2_TRADE_X + 4 * V2_TRADE_CELL + 6, V2_TRADE_Y + 4 * V2_TRADE_CELL + 6, 10);
  for (int i = 0; i < 16; i++) {
    bool shiny;
    int16_t dex = pet.formAt(page * 16 + i, shiny);
    if (!dex) break;
    int x = V2_TRADE_X + (i % 4) * V2_TRADE_CELL, y = V2_TRADE_Y + (i / 4) * V2_TRADE_CELL;
    if (const uint8_t *t = thumbs.getForm(dex, shiny)) drawThumb(t, x - 2, y - 2, 2, !pet.canTrade(dex, shiny));  // (centred in 80)
    if (shiny) drawShinyMark(x, y, V2_TRADE_CELL);
    if (!buddyRegHas(buddyReg, dex)) gfx->fillCircle(x + 62, y + 62, 7, UI_BAR_OK);  // the buddy hasn't got it
  }
  drawPageArrows(page, pages, V2_PAGE_ARROW_Y);
  if (pages > 1) v2PageOf(pages, page);
  v2CloseX();
}
// a tap in a V2 square, moved to the middle of V1's (73 + 80 c, 72 + 80 r); one off the squares but where V1's
// reach (they're bigger), moved where V1 finds none. The board over them and the X keep theirs
static void v2TradeTap(int16_t &x, int16_t &y) {
  if (x < V2_TRADE_X || y < V2_TRADE_Y || x >= V2_TRADE_X + 4 * V2_TRADE_CELL || y >= V2_TRADE_Y + 4 * V2_TRADE_CELL) {
    if (y >= 66 && y < 396) x = 0;
    return;
  }
  x = 73 + (x - V2_TRADE_X) / V2_TRADE_CELL * 80 + 40;
  y = 72 + (y - V2_TRADE_Y) / V2_TRADE_CELL * 80 + 40;
}
// another Pokemon from its thumbnail at scale s, standing with its feet at (cx, feet), with a shadow
static void v2ThumbStand(const uint8_t *b, int cx, int feet, int s) {
  if (!b) return;
  int w = b[0], h = b[1], top = h, bot = -1;
  const uint8_t *d = b + 3 + b[2] * 2;
  for (int i = 0; i < w * h; i++)
    if (d[i] != 0xFF) {
      if (i / w < top) top = i / w;
      bot = i / w;
    }
  if (bot < 0) return;
  int bh = (bot - top + 1) * s;
  v2PetShadow(cx, feet - 4, 14 * s);
  drawThumbFit(b, cx, feet - bh + bh / 2, 400, 400, s, false, 0, nullptr);
}
// one side of the trade: what it is over it, it standing, its name on a tag, its stars
static void v2TradeSide(int cx, const char *label, int16_t dex, bool shiny, uint8_t stars) {
  gfx->setTextColor(V2_ON_DARK);
  setSize(2);
  setCur(cx - textW(label, 2) / 2, 68);
  printT(label);
  if (!dex) {  // not known yet
    setSize(6);  // (an even size: on the Knomi each font pixel lands whole; 3 spread over a pixel and a half, Fedor: crisp as the battles)
    gfx->setTextColor(UI_WHITE);
    setCur(cx - textW("?", 6) / 2, 114);
    printT("?");
    return;
  }
  v2ThumbStand(thumbs.getForm(dex, shiny), cx, 204, 3);
  if (shiny) drawShinyMark(cx - 62, 84, 124);
  const char *nm = dexName(dex);
  setSize(2);
  int w = textW(nm, 2) + 16;
  gfx->fillRoundRect(cx - w / 2 - 3, 207, w + 6, 30, 8, C565(0x3a, 0x4a, 0x5a));
  gfx->fillRoundRect(cx - w / 2, 210, w, 24, 6, UI_WHITE);
  gfx->setTextColor(UI_INK);
  setCur(cx - textW(nm, 2) / 2, 214);
  printT(nm);
  drawStars(cx, 252, 7, stars);
}
// what you give on the left, what you get on the right, swap arrows between; TRADE in V1's box (or the wait)
static void v2TradeReview(int16_t mine, bool mineShiny, uint8_t mineStars, int16_t theirs, bool theirsShiny, uint8_t theirsStars,
                          bool review) {
  v2MenuBackdrop();
  char b[40];
  v2Sign(v2Caps(T(S_BUDDY_TRADE), b, sizeof(b)));
  v2TradeSide(136, T(S_TRADE_GIVE), mine, mineShiny, mineStars);
  v2TradeSide(330, T(S_TRADE_GET), theirs, theirsShiny, theirsStars);
  for (int k = 0; k < 2; k++) {  // the swap arrows
    int yy = 132 + k * 22, s = k ? -1 : 1, tip = s > 0 ? 262 : 204;
    gfx->fillRect(206, yy - 2, 54, 5, UI_WHITE);
    gfx->fillTriangle(tip + 8 * s, yy, tip - 2 * s, yy - 8, tip - 2 * s, yy + 8, UI_WHITE);
  }
  if (review) {
    v2LabelButton(CX - 107, 279, CX + 107, 329, b, C565(0x58, 0xb8, 0x68));  // (the board's TRADE)
  } else {
    v2Panel(CX - 110, 296, CX + 110, 336, 8);
    gfx->setTextColor(V2_INK_MUTED);
    setSize(2);
    setCur(centerX(T(S_BUDDY_WAITING), 2), 308);
    printT(T(S_BUDDY_WAITING));
  }
  v2CloseX();
}
// done: what came, big on the rug, and "Got EEVEE!" on a green banner
static void v2TradeDone(int16_t dex, bool shiny) {
  v2MenuBackdrop();
  v2ThumbStand(thumbs.getForm(dex, shiny), CX, 330, 4);
  if (shiny) drawShinyMark(CX - 84, 150, 168);
  char b[44];
  snprintf(b, sizeof(b), T(S_TRADE_GOT_FMT), dexName(dex));
  setSize(2);
  int w = textW(b, 2) + 40;
  v2LabelButton(CX - w / 2, 352, CX + w / 2, 392, b, C565(0x58, 0xb8, 0x68));
  v2CloseX();  // it waits for this (it went by itself after 3.5 s)
}

// ----- the keyboards: a nickname and a Wi-Fi password (batch3.py keyboard(), wifi_list()) -----
// "As big as the screen allows" (Fedor): QWERTY, 10 keys of 37 x 52 a row, four rows edge to edge from y 120.
// Letters (small, or capitals with shift, which locks), numbers and symbols, more symbols; shift and
// backspace wide on the third row, the layer key, SPACE and OK along the bottom. Between them all, the
// printable ASCII, as V1's layers. The typed text at size 3 in a panel above, its title over it.
static const char *const V2_KB[4][3] = {
  { "qwertyuiop", "asdfghjkl", "zxcvbnm" },
  { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" },
  { "1234567890", "-/:;()$&@", ".,?!'\"#%" },
  { "[]{}#%^*+=", "_\\|~<>`\"'", ".,?!'\"#%" },
};
#define V2_KW 36  // (with the gap, 40 a key: every key's edges on even pixels, whole Knomi pixels)
#define V2_KGAP 4
#define V2_KH 52
#define V2_KY0 120
#define V2_KSTEP (V2_KH + 6)
#define V2_KWIDE 52
// a key: a character (32..126), or one of these
#define V2K_SHIFT 1  // shift (letters), or the other symbols' key
#define V2K_BKSP 2
#define V2K_LAYER 3  // 123, or back to ABC
#define V2K_OK 5
static int v2KbRowX(int n, int extra) { return SNAP2(233 - (n * V2_KW + (n - 1) * V2_KGAP + extra) / 2); }
// every key of layer L with its box, in drawing order; returns how many (at most 40)
struct V2Key { int16_t x0, y0, x1, y1; int8_t code; };
static int v2KbKeys(int L, V2Key *k) {
  int n = 0;
  for (int r = 0; r < 2; r++) {
    const char *row = V2_KB[L][r];
    int len = (int)strlen(row), x = v2KbRowX(len, 0), y = V2_KY0 + r * V2_KSTEP;
    for (int i = 0; i < len; i++, x += V2_KW + V2_KGAP) k[n++] = { (int16_t)x, (int16_t)y, (int16_t)(x + V2_KW), (int16_t)(y + V2_KH), (int8_t)row[i] };
  }
  const char *row = V2_KB[L][2];
  int len = (int)strlen(row), y = V2_KY0 + 2 * V2_KSTEP, x = v2KbRowX(len, 2 * (V2_KWIDE + V2_KGAP));
  k[n++] = { (int16_t)x, (int16_t)y, (int16_t)(x + V2_KWIDE), (int16_t)(y + V2_KH), V2K_SHIFT };
  x += V2_KWIDE + V2_KGAP;
  for (int i = 0; i < len; i++, x += V2_KW + V2_KGAP) k[n++] = { (int16_t)x, (int16_t)y, (int16_t)(x + V2_KW), (int16_t)(y + V2_KH), (int8_t)row[i] };
  k[n++] = { (int16_t)x, (int16_t)y, (int16_t)(x + V2_KWIDE), (int16_t)(y + V2_KH), V2K_BKSP };
  y += V2_KSTEP;
  k[n++] = { 40, (int16_t)y, 120, (int16_t)(y + V2_KH), V2K_LAYER };
  k[n++] = { 126, (int16_t)y, 318, (int16_t)(y + V2_KH), ' ' };
  k[n++] = { 324, (int16_t)y, 426, (int16_t)(y + V2_KH), V2K_OK };
  return n;
}
// the key under (x, y) in layer L (the gaps between keys count as their nearest key's), or -1
static int v2KbAt(int L, int x, int y, V2Key *box = nullptr) {
  V2Key k[40];
  int n = v2KbKeys(L, k);
  for (int i = 0; i < n; i++)
    if (x >= k[i].x0 - 2 && x <= k[i].x1 + 2 && y >= k[i].y0 - 3 && y <= k[i].y1 + 3) {
      if (box) *box = k[i];
      return k[i].code;
    }
  return -1;
}
// the layer a layer key leads to: shift turns letters small and capital (and the symbols' key swaps the two
// symbol layers); the bottom key goes from letters to numbers, and back
static int v2KbNext(int L, int code) {
  if (code == V2K_SHIFT) return L < 2 ? 1 - L : 5 - L;
  return L < 2 ? 2 : 0;
}
// a key in the V2 button style (key()): a dark rim, a lit stripe along its top; held: a bar under its label
static void v2Key(const V2Key &k, const char *lab, uint16_t col, uint16_t ink, int size, bool held) {
  int w = k.x1 - k.x0, h = k.y1 - k.y0;  // (even, from even pixels: its edges whole Knomi pixels)
  V2_HOVER(k.x0, k.y0, w, h);  // under the mouse: lifted (hover_v2.h)
  gfx->fillRoundRect(k.x0 - 2, k.y0 - 2, w + 4, h + 4, 6, C565(0x3a, 0x4a, 0x5a));
  gfx->fillRoundRect(k.x0, k.y0, w, h, 6, col);
  gfx->fillRect(k.x0 + 4, k.y0 + 2, w - 8, 2, lerp565(col, UI_WHITE, 8, 16));
  if (held) gfx->fillRect(k.x0 + 8, k.y1 - 8, w - 16, 2, UI_WHITE);
  bool cjk = classicFontBegin();  // the keys' letters and arrows are the classic font's in every language
  gfx->setTextColor(ink);
  setSize(size);
  setCur((k.x0 + k.x1) / 2 - textW(lab, size) / 2 + 1, (k.y0 + k.y1) / 2 - 4 * size + 1);
  printT(lab);
  classicFontEnd(cjk);
}
// the whole keyboard: its title, what's been typed (the end of it, if it's long), the keys of layer L; hot:
// the key under the finger, lit, and big over the text (the finger hides the real one); ready: OK is green
static void v2Keyboard(int L, const char *title, const char *typed, int hot, bool ready) {
  v2MenuBackdrop();
  gfx->setTextColor(C565(0xff, 0xf3, 0xdc));
  setSize(2);
  setCur(centerX(title, 2), 36);
  printT(title);
  v2Panel(84, 58, 382, 104, 8);
  bool cjk = classicFontBegin();
  size_t len = strlen(typed);
  const char *show = len > 15 ? typed + len - 15 : typed;  // 15 letters at size 3 fill the panel
  char line[20];
  snprintf(line, sizeof(line), "%s_", show);
  gfx->setTextColor(UI_INK);
  setSize(4);
  int ts = textW(line, 4) <= 266 ? 4 : 2;  // (an even size: on the Knomi each font pixel lands whole; 3 spread over a pixel and a half, Fedor: crisp as the battles)
  setSize(ts);
  setCur(100, ts == 4 ? 66 : 74);
  printT(line);
  classicFontEnd(cjk);
  V2Key k[40];
  int n = v2KbKeys(L, k);
  const uint16_t grey = C565(0xaa, 0xb0, 0xbc), blue = C565(0x4f, 0x93, 0xc4);
  for (int i = 0; i < n; i++) {
    char lab[8] = { (char)k[i].code, 0 };
    bool lit = k[i].code == hot;
    switch (k[i].code) {
      case V2K_SHIFT:
        if (L < 2) v2Key(k[i], "\x18", lit || L == 1 ? blue : grey, UI_WHITE, 4, L == 1);  // the font's up arrow
        else v2Key(k[i], L == 2 ? "#+=" : "123", lit ? blue : grey, UI_WHITE, 2, false);
        break;
      case V2K_BKSP: v2Key(k[i], "\x1b", lit ? blue : grey, UI_WHITE, 4, false); break;  // its left arrow
      case V2K_LAYER: v2Key(k[i], L < 2 ? "123" : "ABC", lit ? blue : grey, UI_WHITE, 2, false); break;
      case V2K_OK: v2Key(k[i], "OK", lit ? blue : ready ? C565(0x58, 0xb8, 0x68) : grey, UI_WHITE, 2, false); break;
      case ' ': v2Key(k[i], "SPACE", lit ? blue : UI_WHITE, lit ? UI_WHITE : C565(0x8e, 0x8c, 0x96), 2, false); break;
      default: v2Key(k[i], lab, lit ? blue : UI_WHITE, lit ? UI_WHITE : UI_INK, 4, false);  // (size 4: 3 spread each font pixel over a Knomi pixel and a half, Fedor: "very pixelated")
    }
  }
  if (hot > 0) {  // what the finger is on, big, where the text was
    char big[8] = { (char)hot, 0 };
    if (hot == ' ') strlcpy(big, "SPACE", sizeof(big));
    else if (hot == V2K_SHIFT) strlcpy(big, L < 2 ? "\x18" : L == 2 ? "#+=" : "123", sizeof(big));
    else if (hot == V2K_BKSP) strlcpy(big, "\x1b", sizeof(big));
    else if (hot == V2K_LAYER) strlcpy(big, L < 2 ? "123" : "ABC", sizeof(big));
    else if (hot == V2K_OK) strlcpy(big, "OK", sizeof(big));
    int s = strlen(big) > 3 ? 4 : 6;
    bool c2 = classicFontBegin();
    setSize(s);
    int w = textW(big, s) + 36;
    gfx->fillRoundRect(CX - w / 2, 30, w, 84, 14, blue);
    gfx->setTextColor(UI_WHITE);
    setCur(CX - textW(big, s) / 2 + s / 2, 72 - 4 * s);
    printT(big);
    classicFontEnd(c2);
  }
  v2CloseX();
}

// the nickname's keyboard: it starts in capitals for an empty name, and turns small after the first letter
static uint8_t v2NameLayer = 0;
static void v2NameKbOpen() { v2NameLayer = nameLen ? 0 : 1; }
static void v2NameKb() {
  char title[24];
  v2Keyboard(v2NameLayer, v2Caps(T(S_NAME), title, sizeof(title)), nameBuf, -1, true);
}
static void v2NameKbTap(int16_t x, int16_t y) {
  if (closeHit(x, y)) {  // the X: out without renaming (V1 had only OK)
    sfxPlay(SFX_TAP);
    kbOpen = false;
    return;
  }
  int c = v2KbAt(v2NameLayer, x, y);
  if (c < 0) return;
  sfxPlay(SFX_TAP);
  if (c == V2K_SHIFT || c == V2K_LAYER) {
    v2NameLayer = v2KbNext(v2NameLayer, c);
  } else if (c == V2K_BKSP) {
    if (nameLen) nameBuf[--nameLen] = 0;
  } else if (c == V2K_OK) {
    pet.rename(nameBuf);
    kbOpen = false;
  } else if (nameLen < sizeof(nameBuf) - 1) {
    nameBuf[nameLen++] = (char)c;
    nameBuf[nameLen] = 0;
    if (nameLen == 1 && v2NameLayer == 1) v2NameLayer = 0;  // a capital first, then small letters
  }
  screenKeyValid = false;  // (the layer isn't in the screen's key)
}
// what a key says in the press preview over the finger (words for the arrows: the CJK font has none)
static const char *v2KeyLabel(int c, int L, char *buf) {
  if (c == V2K_SHIFT) return L < 2 ? (L == 1 ? "abc" : "ABC") : L == 2 ? "#+=" : "123";
  if (c == V2K_BKSP) return "<-";
  if (c == V2K_LAYER) return L < 2 ? "123" : "ABC";
  if (c == V2K_OK) return "OK";
  if (c == ' ') return "SPACE";
  buf[0] = (char)c;
  buf[1] = 0;
  return buf;
}

#ifdef BOARD_KNOMI_V2  // the Knomi's own Wi-Fi setup
// the Wi-Fi list (wifi_list()): the networks found, strongest first, in rows in a panel (V1's rows, where its taps
// are: 5 of 48 from y 88); the one in use is blue. SCAN under them, the page buttons at the sides, the X cancels
#define V2_WIFI_ROW_Y 88  // V1's WIFI_ROW_Y, _H and WIFI_ROWS, defined further down
#define V2_WIFI_ROW_H 48
static void v2WifiList(int page, int n, const char *(*net)(int), const char *cur) {
  v2MenuBackdrop();
  v2Sign("WI-FI");
  int pages = (n + 4) / 5;
  if (pages < 1) pages = 1;
  v2Panel(86, 76, 380, 374, 10);  // (its page line under it)
  setSize(2);
  for (int r = 0; r < 5; r++) {
    int idx = page * 5 + r;
    if (idx >= n) break;
    int y = V2_WIFI_ROW_Y + r * V2_WIFI_ROW_H;
    bool inUse = cur[0] && !strcmp(cur, net(idx));
    gfx->fillRoundRect(98, y, 270, V2_WIFI_ROW_H - 6, 8, inUse ? C565(0x4f, 0x93, 0xc4) : C565(0xee, 0xf2, 0xf8));
    char line[21];
    strlcpy(line, net(idx), sizeof(line));  // 20 letters fill the row
    bool cjk = classicFontBegin();  // network names are bytes, not the language's
    gfx->setTextColor(inUse ? UI_WHITE : UI_INK);
    setCur(112, y + 13);
    printT(line);
    classicFontEnd(cjk);
  }
  v2LabelButton(179, 333, 287, 367, "SCAN", C565(0x8e, 0x8c, 0x96));  // V1's middle box (its sides turn pages too)
  drawPageArrows(page, pages, V2_PAGE_ARROW_Y);
  if (pages > 1) v2PageOf(pages, page, 388);
  v2CloseX();
}
// the password's keyboard: "< network" over it goes back to the list, as the X does
static void v2WifiKbTap(int16_t x, int16_t y) {
  if (y < 56 || closeHit(x, y)) {
    sfxPlay(SFX_TAP);
    wifiSetupBackToList();
    return;
  }
  int c = v2KbAt(wifiKbLayer, x, y);
  if (c < 0) return;
  if (c == V2K_SHIFT || c == V2K_LAYER) wifiKbLayer = v2KbNext(wifiKbLayer, c);
  else if (c == V2K_BKSP) wifiSetupDelete();
  else if (c != V2K_OK) wifiSetupType((char)c);
  else if (wifiSetupCanSubmit()) {
    sfxPlay(SFX_TAP);
    wifiSetupSubmit();
  } else {
    sfxPlay(SFX_DENY);  // fewer than 8 characters
  }
}
// what the settings page shows while the Wi-Fi is being set: the list, the keyboard, or how it's going
static void v2WifiScreen(uint8_t ws) {
  if (ws == WS_LIST) {
    v2WifiList(wifiListPage, wifiSetupNetCount(), wifiSetupNet, clockWifiSsid());
    return;
  }
  if (ws == WS_PASS) {
    char title[40];
    snprintf(title, sizeof(title), "< %.24s", wifiSetupSsid());
    v2Keyboard(wifiKbLayer, title, wifiSetupPass(), wifiKbHover, wifiSetupCanSubmit());
    return;
  }
  v2MenuBackdrop();
  v2Sign("WI-FI");
  const char *msg = ws == WS_SCAN ? T(S_WIFI_SEARCH) : ws == WS_OK ? T(S_WIFI_SAVED) : ws == WS_FAIL ? T(S_WIFI_KEPT) : T(S_WIFI_TRYING);
  setSize(2);
  int w = textW(msg, 2) + 40;
  v2Panel(CX - w / 2, 204, CX + w / 2, 252, 10);
  gfx->setTextColor(ws == WS_OK ? C565(0x3c, 0x8a, 0x4c) : ws == WS_FAIL ? UI_BAR_BAD : UI_INK);
  setCur(centerX(msg, 2), 220);
  printT(msg);
}
#ifdef BOARD_HOST
// the screenshot tool's way in (the host has no Wi-Fi setup of its own): 0 the list, 1 to 4 the keyboard's layers
static const char *v2HostNet(int i) {
  static const char *const N[7] = { "Home-5G", "Home", "Coffee Shop", "Neighbor", "PokeCenter", "Library guest", "Garage" };
  return i >= 0 && i < 7 ? N[i] : "";
}
void v2HostWifiShot(int which) {
  if (which == 0) v2WifiList(0, 7, v2HostNet, "Home-5G");
  else v2Keyboard(which - 1, "< Home-5G", "pika#25", which == 3 ? '#' : -1, which != 4);
}
#endif
#endif

// ----- the games' ends, their how-to cards, Who's That? and a catch -----
static void v2DimFrame(int a256) {  // everything a256/256 of the way to the night tint
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const uint16_t tint = sc565(0x120E22);
  for (int i = 0; i < V2_W * V2_W; i++) fb[i] = v2Blend(fb[i], tint, a256);
}
// a game's end (Fedor: "a Play Again! button to play again and our X to close"): PLAY AGAIN! in green, the
// red X under it; the end waits for one of them. Its place is fixed, so a finger finds it at every game's end
#define V2_AGAIN_X0 116
#define V2_AGAIN_X1 350
#define V2_AGAIN_H 46
#define V2_AGAIN_Y 264   // on a game of yours (v2GameResult())
#define V2_AGAIN_YB 302  // after a game with a buddy (v2BuddyResult()), the Repel row between it and the X
static void v2AgainButton(int y) {
  char b[32];
  v2LabelButton(V2_AGAIN_X0, y, V2_AGAIN_X1, y + V2_AGAIN_H, v2Caps(T(S_PLAY_AGAIN), b, sizeof(b)), C565(0x58, 0xb8, 0x68));
  v2CloseX();
}
static void v2RepelRow(int y, int x0, int x1);  // (repel_v2.h: how far along the next Repel is)
// where the Repel row is on the end whose PLAY AGAIN! is at by: drawn there, and a tap there opens the Repel's page
static void v2RepelRowAt(int by, int &x0, int &y, int &x1) {
  if (by == V2_AGAIN_YB) x0 = 104, y = 357, x1 = 362;  // as wide as the score panel, clear of the X
  else x0 = 96, y = 338, x1 = 370;                      // between the panel and the X, as wide as the panel
}
static void v2RepelRowFor(int by) {
  int x0, y, x1;
  v2RepelRowAt(by, x0, y, x1);
  v2RepelRow(y, x0, x1);
}
// a game's result (rooms2_px.py game_over()): the game behind, dimmed 45%, its name on a board; in a panel a
// line on what happened (the dungeon's faint), the score big, the record (or NEW RECORD! in gold) and the
// happiness it brought, in green, all of it in the room over PLAY AGAIN!, which is at the panel's foot
static void v2GameResult(const char *title, const char *top, const char *big, bool newHi, const char *record, const char *joy) {
  v2DimFrame(115);
  char b[40];
  v2Sign(v2Caps(title, b, sizeof(b)), 70);
  int bs = textW(big, 4) <= 250 ? 4 : 3, h = (top ? 28 : 0) + 8 * bs + 12 + (record ? 26 : 0) + 16;
  v2Panel(96, 112, 370, V2_AGAIN_Y + V2_AGAIN_H + 16, 12);
  int y = 112 + (V2_AGAIN_Y - 112 - h) / 2;
  setSize(2);
  if (top) {
    gfx->setTextColor(UI_INK);
    setCur(centerX(top, 2), y);
    printT(top);
    y += 28;
  }
  gfx->setTextColor(UI_INK);
  setSize(bs);
  setCur(centerX(big, bs), y);
  printT(big);
  y += 8 * bs + 12;
  setSize(2);
  if (record) {
    const char *r = newHi ? T(S_NEW_RECORD) : record;
    gfx->setTextColor(newHi ? C565(0xc0, 0x84, 0x10) : V2_INK_MUTED);
    setCur(centerX(r, 2), y);
    printT(r);
    y += 26;
  }
  gfx->setTextColor(C565(0x3c, 0x8a, 0x4c));
  setCur(centerX(joy, 2), y);
  printT(joy);
  v2AgainButton(V2_AGAIN_Y);
  v2RepelRowFor(V2_AGAIN_Y);  // the Repel this play counts toward
}

// a game's how-to card (rooms2_px.py howto()): its name on a board, how to play in a panel, READY, GO! where
// V1's button is (where its tap is); the whole card drops in and flies off as V1's did, (cx, cy) its middle
static void v2IntroCard(int cx, int cy, bool dim, const char *title, const char *how) {
  if (dim) v2DimFrame(115);
  int dx = cx - CX, dy = cy - CY;
  char b[40];
  setSize(2);
  v2Caps(title, b, sizeof(b));
  int w = textW(b, 2) + 28, bx = cx - w / 2, by = cy - 170;  // the board, moving with the card (v2Sign() stays put)
  const uint16_t dark = C565(0x3a, 0x24, 0x14);
  gfx->fillRect(bx - 3, by - 3, w + 7, 31, dark);
  gfx->fillRect(bx, by, w + 1, 25, C565(0x8a, 0x5a, 0x30));
  gfx->fillRect(bx, by, w + 1, 4, C565(0xb0, 0x7a, 0x45));
  gfx->fillRect(bx + 5, by + 10, 4, 4, dark);
  gfx->fillRect(bx + w - 8, by + 10, 4, 4, dark);
  gfx->setTextColor(C565(0xff, 0xf3, 0xdc));
  setCur(cx - textW(b, 2) / 2, by + 6);
  printT(b);
  v2Panel(86 + dx, cy - 128, 380 + dx, cy + 140, 12);
  int n = drawWrappedAt(how, 2, 264, cx, 0, 22, false);
  gfx->setTextColor(UI_INK);
  drawWrappedAt(how, 2, 264, cx, cy - 110 + (176 - (n * 22 - 6)) / 2, 22, true);
  char go[32];
  v2LabelButton(CX - 117 + dx, cy + 79, CX + 117 + dx, cy + 125, v2Caps(T(S_READY_GO), go, sizeof(go)), C565(0x58, 0xb8, 0x68));
}

// Who's That? (batch3.py quiz_play()): the question, or the answer once it's shown, on a board; the round in a
// small panel under it; the names as white panels where V1's are, green for the answer, red for a wrong tap
// a white panel edged in slate, as the round's (y: its top inside the edge)
static void v2Pill(int x, int y, int w) {
  gfx->fillRoundRect(x - 2, y - 2, w + 4, 28, 8, C565(0x3a, 0x4a, 0x5a));
  gfx->fillRoundRect(x, y, w, 24, 6, UI_WHITE);
}
// the round's panel with the countdown beside it (Fedor: "a 10 second count down on each screen"): a clock and
// the seconds left, red for the last three. The two side by side, centred
static void v2RoundClock(const char *round, int y, int secs) {
  setSize(2);
  char c[4];
  snprintf(c, sizeof(c), "%d", secs < 0 ? 0 : secs > 99 ? 99 : secs);
  int w1 = textW(round, 2) + 24, w2 = 16 + 6 + textW(c, 2) + 20, gap = 10;
  int x1 = CX - (w1 + gap + w2) / 2, x2 = x1 + w1 + gap;
  v2Pill(x1, y, w1);
  gfx->setTextColor(UI_INK);
  setCur(x1 + 12, y + 5);
  printT(round);
  v2Pill(x2, y, w2);
  uint16_t ink = secs <= 3 ? UI_BAR_BAD : UI_INK;
  int cx = x2 + 18, cy = y + 12;  // the clock: a ring and two hands
  gfx->fillCircle(cx, cy, 8, ink);
  gfx->fillCircle(cx, cy, 6, UI_WHITE);
  gfx->fillRect(cx - 1, cy - 5, 2, 6, ink);
  gfx->fillRect(cx - 1, cy - 1, 5, 2, ink);
  gfx->setTextColor(ink);
  setCur(x2 + 32, y + 5);
  printT(c);
}
static void v2QuizHead(const char *q, int round, int rounds, int secs = -1) {
  char b[48];
  v2Sign(v2Caps(q, b, sizeof(b)), 40);
  snprintf(b, sizeof(b), "%d / %d", round, rounds);
  if (secs >= 0) {  // while it asks: the seconds left beside the round
    v2RoundClock(b, 76, secs);
    return;
  }
  setSize(2);
  int w = textW(b, 2) + 24;
  gfx->fillRoundRect(CX - w / 2 - 2, 74, w + 4, 28, 8, C565(0x3a, 0x4a, 0x5a));
  gfx->fillRoundRect(CX - w / 2, 76, w, 24, 6, UI_WHITE);
  gfx->setTextColor(UI_INK);
  setCur(centerX(b, 2), 81);
  printT(b);
}
static void v2QuizOption(int x, int y, int w, int h, const char *name, uint16_t bg) {
  V2_HOVER(x, y, w, h);  // under the mouse: lifted (hover_v2.h)
  if (bg == UI_WHITE) {
    gfx->fillRoundRect(x - 3, y - 3, w + 6, h + 6, 10, C565(0x3a, 0x4a, 0x5a));
    gfx->fillRoundRect(x, y, w, h, 8, UI_WHITE);
  } else {
    v2Button(x, y, w, h, bg);
  }
  gfx->setTextColor(bg == UI_WHITE ? UI_INK : UI_WHITE);
  setSize(2);
  setCur(x + w / 2 - textW(name, 2) / 2, y + h / 2 - 7);
  printT(name);
}

// a catch (rooms_px.py caught_screen(), approved): a gold burst, a red ribbon with the news, sparkles round the
// Pokemon, its stars as pixel stars, what it means for your Pokedex in a panel; the X
// a red ribbon with the news across the top, its ends folded and notched (the caught and showdown screens)
static void v2Ribbon(const char *news) {
  setSize(2);
  int w = textW(news, 2) + 28;
  if (w < 240) w = 240;
  const uint16_t fold = C565(0x88, 0x2c, 0x2c);
  for (int sx = -1; sx <= 1; sx += 2) {
    int ex = CX + sx * w / 2;
    gfx->fillRect(sx < 0 ? ex - 20 : ex, 58, 21, 29, fold);
    gfx->fillTriangle(ex + sx * 20, 58, ex + sx * 30, 58, ex + sx * 20, 72, fold);
    gfx->fillTriangle(ex + sx * 20, 72, ex + sx * 30, 86, ex + sx * 20, 86, fold);
  }
  gfx->fillRect(CX - w / 2 - 3, 52, w + 6, 39, C565(0x3a, 0x24, 0x14));
  gfx->fillRect(CX - w / 2, 55, w, 33, C565(0xc8, 0x50, 0x50));
  gfx->fillRect(CX - w / 2, 55, w, 5, C565(0xe8, 0x70, 0x70));
  gfx->setTextColor(UI_WHITE);
  setCur(centerX(news, 2), 64);
  printT(news);
}
static void v2CaughtBack(const char *news) {
  v2Cached(114, scBurst, 2);
  v2Ribbon(news);
}
static void v2CaughtStars(uint8_t n, int lines) {
  static const char *const STAR[7] = { "...k...", "..kfk..", "kkkfkkk", "kfffffk", ".kfffk.", ".kfkfk.", "kk...kk" };
  scScreen();
  scGradeTod = 0;
  for (int i = 0; i < 3; i++) {
    const ScKey k[2] = { { 'k', 0x8A5A10 }, { 'f', (Rgb)(i < n ? 0xFFD95E : 0xFFFFFF) } };
    scSprite(STAR, 7, 196 + i * 28, 250, 3, k, 2);
  }
  static const int16_t SPARK[6][2] = { { 150, 140 }, { 318, 150 }, { 130, 250 }, { 336, 236 }, { 180, 100 }, { 290, 104 } };
  for (auto &p : SPARK) {  // white pixel sparkles
    scGridRect(p[0], p[1] - 8, p[0] + 4, p[1] + 12, 0xFFFFFF);
    scGridRect(p[0] - 8, p[1], p[0] + 12, p[1] + 4, 0xFFFFFF);
  }
  if (lines > 1) v2Panel(46, 282, 420, 350, 10);  // the lines go at 292 (and 320), as V1's: up to 30 letters
  else v2Panel(92, 282, 374, 318, 10);
}

// ----- the buddy games: the score, Showdown's round, how it ended (batch3.py volley_play(), showdown_win()) -----
// the score in a white panel at y: you and the buddy, in capitals
static void v2ScorePanel(int mine, int theirs, int y) {
  char b[48], c[48];
  snprintf(b, sizeof(b), "%s %u : %u %s", T(S_SD_YOU_LBL), mine, theirs, buddyName());
  v2Caps(b, c, sizeof(c));
  setSize(2);
  int w = textW(c, 2) + 28;
  gfx->fillRoundRect(CX - w / 2 - 3, y - 3, w + 6, 34, 10, C565(0x3a, 0x4a, 0x5a));
  gfx->fillRoundRect(CX - w / 2, y, w, 28, 8, UI_WHITE);
  gfx->setTextColor(UI_INK);
  setCur(centerX(c, 2), y + 6);
  printT(c);
}
// a line in a small white panel (the round, who got it, waiting...)
static void v2SmallPanel(const char *s, int y) {
  setSize(2);
  int w = textW(s, 2) + 24;
  gfx->fillRoundRect(CX - w / 2 - 2, y - 2, w + 4, 28, 8, C565(0x3a, 0x4a, 0x5a));
  gfx->fillRoundRect(CX - w / 2, y, w, 24, 6, UI_WHITE);
  gfx->setTextColor(UI_INK);
  setCur(centerX(s, 2), y + 5);
  printT(s);
}
// how a buddy game ended (showdown_win()): the news on the ribbon, yours and theirs face to face with VS
// between, the score in a panel with who's who under it (unit: PikaSurf's "s"), then PLAY AGAIN! and the X.
// dim: over the court or the water
static void v2BuddyResult(const char *news, int mine, int theirs, PmdMon &them, bool dim, const char *unit = "") {
  if (dim) v2DimFrame(115);
  char c[48];
  v2Ribbon(v2Caps(news, c, sizeof(c)));
  PmdMon *two[2] = { &pmd, &them };
  for (int k = 0; k < 2; k++) {
    PmdMon &m = *two[k];
    if (!m.loaded) continue;
    int x = k ? 330 : 140;
    uint8_t s = pmdFitScale(m, PMD_IDLE, 150, 120);
    gPmdForceScale = s < 4 ? s : 4;
    v2PetShadow(x, 208, 40);
    bool shiny = m.shiny;
    m.shiny = false;  // (no sparkles over the ribbon)
    drawPmdActM(m, PMD_IDLE, x, 212, millis(), true, false, 4);
    m.shiny = shiny;
    gPmdForceScale = 0;
  }
  v2Panel(104, 220, 362, 290, 12);
  char l[12];
  char r[12];
  char b[28];
  snprintf(l, sizeof(l), "%u%s", mine, unit);
  snprintf(r, sizeof(r), "%u%s", theirs, unit);
  snprintf(b, sizeof(b), "%s : %s", l, r);
  bool cjk = classicFontBegin();  // VS and the numbers in the pixel font in every language
  gfx->setTextColor(UI_WHITE);
  setSize(4);
  v2Bold("VS", centerX("VS", 4), 126);  // (an even size: on the Knomi each font pixel lands whole; 3 spread over a pixel and a half, Fedor: crisp as the battles)
  int ss = textW(b, 4) <= 230 ? 4 : 2;  // a long time a size down
  gfx->setTextColor(UI_INK);
  setSize(ss);
  int sx = centerX(b, ss), lx = sx + textW(l, ss) / 2, rx = sx + textW(b, ss) - textW(r, ss) / 2;
  setCur(sx, 230 + (4 - ss) * 4);
  printT(b);
  classicFontEnd(cjk);
  v2Caps(T(S_SD_YOU_LBL), c, sizeof(c));  // who's who, under each number, kept inside the panel
  const char *nm = buddyName();
  setSize(2);
  gfx->setTextColor(V2_INK_MUTED);
  int cw = textW(c, 2), nw = textW(nm, 2), cx0 = lx - cw / 2, nx0 = rx - nw / 2;
  if (cx0 < 116) cx0 = 116;
  if (nx0 + nw > 350) nx0 = 350 - nw;
  setCur(cx0, 268);
  printT(c);
  setCur(nx0, 268);
  printT(nm);
  v2AgainButton(V2_AGAIN_YB);
  v2RepelRowFor(V2_AGAIN_YB);  // the Repel this play counts toward
}
// PikaSurf with a buddy, both out (V1's words over the water): as the other games with a buddy, the times
// for the score
static void v2SurfDuelResult() {
  int out = surfDuelOutcome((uint16_t)(surf.t * 10), surfTheir10);
  char b[48];
  if (out > 0) strlcpy(b, T(S_SD_WIN), sizeof(b));
  else if (out < 0) snprintf(b, sizeof(b), T(S_SD_LOSE_FMT), buddyName());
  else strlcpy(b, T(S_SD_DRAW), sizeof(b));
  v2BuddyResult(b, (int)surf.t, surfTheir10 / 10, surfPika, true, "s");
}

// The end on show, if any: where its PLAY AGAIN! is (0: no end on show), and since when it's been up
static int v2GameEnd(uint32_t *since = nullptr) {
  uint32_t t = 0;
  int y = V2_AGAIN_Y;
  if (gameOpen && gameOverUntil) t = gameOverUntil - 3500;  // (Berry Drop's end lasted 3.5 s in V1)
  else if (quizOpen && quizPhase == QP_END) t = quizT;
  else if (runOpen && runOverT) t = runOverT;
  else if (dungeonOpen && dnOverT) t = dnOverT;
  else if (surfOpen && !surfDuel && surfOverT) t = surfOverT;
  else {
    y = V2_AGAIN_YB;
    if (sdOpen && sdPhase == SD_DONE) t = sdT;
    else if (vbOpen && vbPhase == VB_DONE) t = vbT;
    else if (surfOpen && surfDuel && surfDuelDoneT) t = surfDuelDoneT;
    else return 0;
  }
  if (since) *since = t;
  return y;
}
// What a tap on it means: 1 PLAY AGAIN!, 2 the X, 3 the Repel row, 0 nothing. Not in its first 800 ms, and only for a finger put
// down after it came up: one steering or jumping as the game ended lifts on nothing
static int v2GameEndTap(int16_t x, int16_t y) {
  uint32_t since;
  int by = v2GameEnd(&since);
  if (!by || millis() - since < 800 || (int32_t)(tStart - since) < 0) return 0;
  if (x >= V2_AGAIN_X0 - 8 && x <= V2_AGAIN_X1 + 8 && y >= by - 8 && y <= by + V2_AGAIN_H + 8) return 1;
  int rx0, ry, rx1;
  v2RepelRowAt(by, rx0, ry, rx1);
  if (x >= rx0 && x <= rx1 && y >= ry - 4 && y <= ry + 32) return 3;
  return closeHit(x, y) ? 2 : 0;
}
// A game with a buddy closes as its end did by itself: home, or Play together after PikaSurf (with back)
static void v2BuddyGameClose(bool back) {
  if (sdOpen) sdClose();
  if (vbOpen) vbClose();
  if (surfOpen && surfDuel) {
    if (back) {
      surfDuelEnd();
    } else {
      closeSurf();
      buddyEndSession();
    }
  }
}
// What they do. PLAY AGAIN! starts a game of yours again at once (no card: you've just played it); after a game
// with a buddy it asks them again, from home, as an invitation does. The X goes where the end went by itself:
// back to the games, home after a game with a buddy, Play together after PikaSurf with one. The Repel row closes
// the game and opens the card on the Repel's page (Fedor: "Clicking on repel progress after a mini game should take
// you to the repel screen"); its X goes home, as the card's does
static void v2GameEndDo(int k) {
  sfxPlay(SFX_TAP);
  if (k == 3) {
    if (sdOpen || vbOpen || surfDuel) {
      v2BuddyGameClose(false);
    } else {
      gameOpen = false;
      if (quizOpen) closeQuiz();
      if (runOpen) closeRun();
      if (dungeonOpen) closeDungeon();
      if (surfOpen) closeSurf();
    }
    cardOpen = true;
    cardAch = false;
    cardPage = 6;  // the Repel's, after the Ultra Ball's and the Master Ball's (CARD_PAGES)
    return;
  }
  bool again = k == 1;
  if (sdOpen || vbOpen || surfDuel) {
    uint8_t kind = sdOpen ? BK_SHOWDOWN : vbOpen ? BK_VOLLEY : BK_SURF;
    v2BuddyGameClose(!again);
    if (!again) return;
    if (!bPeers[bCur].on) buddySetNote(S_BUDDY_LEFT_FMT);  // they've gone
    else if (!pet.isEgg() && !pet.sleeping && !pet.ceremony) buddyInvite(kind);
    return;
  }
  int g = gameOpen ? 0 : quizOpen ? 1 : runOpen ? 2 : dungeonOpen ? 3 : 4;
  gameOpen = false;
  if (quizOpen) closeQuiz();
  if (runOpen) closeRun();
  if (dungeonOpen) closeDungeon();
  if (surfOpen) closeSurf();
  if (again) {
    if (g == 0) startGame();
    else if (g == 1) startQuiz();
    else if (g == 2) startRun();
    else if (g == 3) startDungeon();
    else startSurf();
  }
  if (!gameOpen && !quizOpen && !runOpen && !dungeonOpen && !surfOpen) gameEndToMenu();  // (it may not start: asleep)
}

// ----- the home's dialogs and moments (rooms2_px.py release_dialog(), batch3.py medal_banner(), bath(), evolving()) -----
// V1's colours as V2's: grey buttons get white words
static uint16_t v2BtnCol(uint16_t c, uint16_t &ink) {
  if (c == UI_TRACK) {
    ink = UI_WHITE;
    return C565(0x8e, 0x8c, 0x96);
  }
  if (c == UI_BAR_BAD) return C565(0xe8, 0x50, 0x3a);
  if (c == UI_BAR_OK) return C565(0x58, 0xb8, 0x68);
  if (c == UI_BAR_WARN) return C565(0xf0, 0xc0, 0x4a);
  return c;
}
// a question over the dimmed home with two V2 buttons in V1's boxes, where its taps are: evolve or keep, say
// goodbye or stay, take care of it or let it go, a buddy's invitation
static void v2ChoiceDialog(const char *q, const char *o1, uint16_t c1, uint16_t t1, const char *o2, uint16_t c2, uint16_t t2) {
  dimFrame();
  v2Panel(73, 156, 393, 344, 12);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(q, 2), 178);
  printT(q);
  char a[40], b[40];
  c1 = v2BtnCol(c1, t1);
  c2 = v2BtnCol(c2, t2);
  v2LabelButton(96, 209, 370, 255, v2Caps(o1, a, sizeof(a)), c1, t1);
  v2LabelButton(96, 271, 370, 317, v2Caps(o2, b, sizeof(b)), c2, t2);
}
// "Release GENGAR?" (release_dialog()): under it, what it means ("It goes back to the wild. This cannot be
// undone."), then RELEASE in red and KEEP in green (Fedor)
static void v2ReleaseDialog(const char *q) {
  dimFrame();
  v2Panel(84, 140, 382, 330, 12);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(q, 2), 160);
  printT(q);
  gfx->setTextColor(V2_INK_MUTED);
  drawWrappedAt(T(S_RELEASE_WARN), 2, 252, CX, 192, 20, true);
  char a[24];
  char b[24];
  v2LabelButton(V2_REL_X0, V2_REL_Y0, V2_REL_X1, V2_REL_Y1, v2Caps(T(S_RELEASE_BTN), a, sizeof(a)), C565(0xe8, 0x50, 0x3a));
  v2LabelButton(V2_KEEP_X0, V2_REL_Y0, V2_KEEP_X1, V2_REL_Y1, v2Caps(T(S_KEEP_BTN), b, sizeof(b)), C565(0x58, 0xb8, 0x68));
}
// the evolve, goodbye and runaway buttons (v2b-evolve.png): V2 buttons under the pet, breathing a little; a long
// line goes on two
static void v2CtaButton(int x, int y, int w, int h, const char *s, uint16_t col, uint16_t ink, int period) {
  int p = SNAP2((int)(2.5f * sinf(millis() * 6.2831853f / period)));
  v2Button(x - p, y - p, w + 2 * p, h + 2 * p, col);
  char c[64], l1[64];
  v2Caps(s, c, sizeof(c));
  setSize(2);
  gfx->setTextColor(ink);
  if (textW(c, 2) <= w - 24) {
    setCur(centerX(c, 2), y + h / 2 - 7);
    printT(c);
    return;
  }
  int n = (int)strlen(c), cut = -1;
  for (int i = 1; i < n; i++)
    if (c[i] == ' ' && (cut < 0 || abs(2 * i - n) < abs(2 * cut - n))) cut = i;
  if (cut < 0) cut = n;
  snprintf(l1, sizeof(l1), "%.*s", cut, c);
  setCur(centerX(l1, 2), y + h / 2 - 19);
  printT(l1);
  if (cut < n) {
    setCur(centerX(c + cut + 1, 2), y + h / 2 + 3);
    printT(c + cut + 1);
  }
}

// a new medal (medal_banner()): a pixel medal on a red and blue ribbon, the news in a panel under it: "MEDAL!"
// in gold and its name big; white pixel sparkles twinkle once it has settled. A streak's has a flame. dy: as
// it slides in (V1's)
static void v2MedalBanner(const char *l1, const char *l2, bool medal, int dy) {
  scScreen();
  scGradeTod = 0;
  const uint16_t red = C565(0xc8, 0x50, 0x50), blue = C565(0x4f, 0x93, 0xc4);
  for (int k = 0; k < 2; k++) {  // the ribbon
    int x0 = 205 + 28 * k, t = 104 + dy, b = 150 + dy;
    uint16_t c = k ? blue : red;
    gfx->fillRect(x0, t, 29, b - t - 10, c);
    gfx->fillTriangle(x0, b - 10, x0 + 14, b - 10, x0, b, c);
    gfx->fillTriangle(x0 + 14, b - 10, x0 + 28, b - 10, x0 + 28, b, c);
  }
  scOval(scE(197, 126 + dy, 269, 198 + dy), 0xC08410);
  scOval(scE(203, 132 + dy, 263, 192 + dy), 0xF0C04A, 0xFFD95E);
  if (medal) {
    static const char *const STAR[7] = { "...k...", "..kfk..", "kkkfkkk", "kfffffk", ".kfffk.", ".kfkfk.", "kk...kk" };
    const ScKey k[2] = { { 'k', 0x8A5A10 }, { 'f', 0xFFFFFF } };
    scSprite(STAR, 7, 223, 152 + dy, 3, k, 2);
  } else {
    static const char *const FLAME[8] = { "...r...", "..rr...", "..rrr..", ".rryrr.", ".ryyyr.", "rryyyrr", ".ryyyr.", "..rrr.." };
    const ScKey k[2] = { { 'r', 0xE8503A }, { 'y', 0xFFE060 } };
    scSprite(FLAME, 8, 223, 150 + dy, 3, k, 2);
  }
  v2Panel(104, 204 + dy, 362, 266 + dy, 10);
  gfx->setTextColor(C565(0xc0, 0x84, 0x10));
  setSize(2);
  setCur(centerX(l1, 2), 213 + dy);
  printT(l1);
  int s = textW(l2, 3) <= 240 ? 3 : 2;
  gfx->setTextColor(UI_INK);
  setSize(s);
  v2Bold(l2, centerX(l2, s), (s == 3 ? 234 : 238) + dy);
  if (dy) return;
  static const int16_t SP[4][2] = { { 180, 148 }, { 290, 138 }, { 300, 190 }, { 166, 194 } };
  uint32_t beat = millis() / 350;
  for (int i = 0; i < 4; i++) {
    if ((beat + i) % 4 == 0) continue;  // one resting at a time: they twinkle
    scGridRect(SP[i][0], SP[i][1] - 8, SP[i][0] + 4, SP[i][1] + 12, 0xFFFFFF);
    scGridRect(SP[i][0] - 8, SP[i][1], SP[i][0] + 12, SP[i][1] + 4, 0xFFFFFF);
  }
}

// the bath (bath()): soap bubbles as pixel bubbles, and the V2 rubber duck facing its Pokemon
static void v2Bubble(int x, int y, int r) {
  scScreen();
  scGradeTod = 0;
  scOval(scE(x - r, y - r, x + r, y + r), 0xD6ECF8, 0xFFFFFF);
  scGridRect(x - r / 2, y - r / 2, x - r / 2 + 4, y - r / 2 + 4, 0xFFFFFF);
}
static void v2BathDuck(int x, int y, bool faceLeft) {  // its 14 x 12 at 3x; it looks right as drawn
  scScreen();
  scGradeTod = 0;
  for (int j = 0; j < 12; j++)
    for (int i = 0; i < 14; i++) {
      char ch = SPR_V2_DUCK[j][faceLeft ? 13 - i : i];
      Rgb c = ch == 'k' ? 0x1B1B25 : ch == 'y' ? 0xFFD95E : ch == 'Y' ? 0xE0B860 : ch == 'o' ? 0xF5863D : ch == 'w' ? 0xFFFFFF : SC_NONE;
      if (c != SC_NONE) scRect(x + i * 3, y + j * 3, x + i * 3 + 2, y + j * 3 + 2, c);
    }
}

// evolving (evolving()): the whole screen goes dark, pale rays turn slowly round it, a gold glow swells and
// shrinks round its Pokemon, which blinks white between the old form and the new, faster and faster (as V1), white
// pixel sparkles twinkle round it, and the header says it in white; V1's white flash ends it
static uint16_t *v2RayAng = nullptr;  // each 4 px cell's angle round the middle, in tenths of a degree
static void v2EvolveFX(PmdMon &oldForm, float t, const char *name, const char *lv) {
  uint32_t now = millis();
  const int cx = CX, cy = PET_GROUND - 50;
  v2DimFrame(141);  // 55%
  if (!v2RayAng) {
    v2RayAng = (uint16_t *)ps_malloc(117 * 117 * sizeof(uint16_t));
    if (v2RayAng)
      for (int j = 0; j < 117; j++)
        for (int i = 0; i < 117; i++) {
          float a = atan2f(j * 4 + 2 - (float)cy, i * 4 + 2 - (float)cx) * 572.9578f;
          v2RayAng[j * 117 + i] = (uint16_t)(a < 0 ? a + 3600 : a);
        }
  }
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const uint16_t pale = sc565(0xFFF0B0);
  int rot = (int)((now / 4) % 240);  // the rays turn: one pair of 12 degrees every 960 ms
  for (int j = 0; j < 117 && v2RayAng; j++)
    for (int i = 0; i < 117; i++) {
      int dx = i * 4 + 2 - cx, dy = j * 4 + 2 - cy;
      if (dx * dx + dy * dy < 40 * 40 || ((v2RayAng[j * 117 + i] + rot) / 120) % 2) continue;
      for (int y = j * 4; y < j * 4 + 4 && y < V2_W; y++)
        for (int x = i * 4; x < i * 4 + 4 && x < V2_W; x++) fb[y * V2_W + x] = v2Blend(fb[y * V2_W + x], pale, 72);  // 28%
    }
  scScreen();
  scGradeTod = 0;
  int g = (int)(4 * sinf(now * 0.008f)) * 2;  // the glow breathes
  scOval(scE(cx - 80 - g, cy - 80 - g, cx + 80 + g, cy + 80 + g), 0xF0C04A);
  scOval(scE(cx - 68 - g, cy - 68 - g, cx + 68 + g, cy + 68 + g), 0xFFD95E);
  int period = 60 + (int)(220 * (1.0f - t));
  bool showOld = t < 0.9f && oldForm.loaded && ((now / period) % 2) == 0;
  if (showOld) drawPmdActM(oldForm, PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  else drawPmdAct(PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  for (int y = cy - 180; y < PET_GROUND + 4; y++)  // the silhouette is drawn in ink: it shines white instead
    for (int x = cx - 150; x < cx + 150; x++)
      if (y >= 0 && fb[y * V2_W + x] == INK_K) fb[y * V2_W + x] = UI_WHITE;
  static const int16_t SP[5][2] = { { 150, 160 }, { 320, 150 }, { 130, 300 }, { 336, 290 }, { 233, 132 } };
  for (int i = 0; i < 5; i++) {
    if ((now / 260 + i * 2) % 5 == 0) continue;
    scGridRect(SP[i][0], SP[i][1] - 10, SP[i][0] + 4, SP[i][1] + 14, 0xFFFFFF);
    scGridRect(SP[i][0] - 10, SP[i][1], SP[i][0] + 14, SP[i][1] + 4, 0xFFFFFF);
  }
  gfx->setTextColor(UI_WHITE);  // the header, in white over it all
  setSize(2);
  setCur(centerX(lv, 2), V2_LEVEL_Y);
  printT(lv);
  v2NameText(name, V2_NAME_Y);  // as at home
  gfx->setTextColor(C565(0xff, 0xf0, 0xb0));
  setSize(2);
  setCur(centerX(T(S_EVOLVING), 2), V2_STATUS_Y);
  printT(T(S_EVOLVING));
  if (t > 0.9f) gfx->fillCircle(cx, cy, (int)(300 * (t - 0.9f) / 0.1f), UI_WHITE);  // the flash
}

// ----- the side buttons' and the X's press previews, the Pokedex entries' side buttons, the trophy room's previews -----
// (the sketch's press preview asks these, pressTargetAt())
static bool v2PressSet(PressTarget &t, int x, int y, int w, int h, const char *label) {
  t = { (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, label, nullptr };
  return true;
}
// V2: the side buttons and the X, where a screen has them: a preview like every other button's (Fedor: "some of the
// buttons, like the left and right keys and the x button in some screens doesnt have an overlay that pops up on
// click"). pages < 2: no side buttons; cy: where its X is
bool v2PressNav(int16_t x, int16_t y, PressTarget &t, int page, int pages, int arrowY, bool hasX, int cy) {
  if (pages > 1)
    if (int d = pageArrowAt(x, y, page, pages, arrowY))
      return v2PressSet(t, CX + d * pageArrowDx(arrowY) - CLOSE_R, arrowY - CLOSE_R, 2 * CLOSE_R, 2 * CLOSE_R, d < 0 ? "<" : ">");
  return hasX && closeHitAt(x, y, cy) && v2PressSet(t, CX - CLOSE_R, V2_CLOSE_Y - CLOSE_R, 2 * CLOSE_R, 2 * CLOSE_R, "X");
}
static char v2PressBuf[52];
// V2: an entry's side buttons step through the entries its grid shows (Fedor: "Add left and right arrows to pokedex
// entries on the main pokedex and zone pokedex's. That way a player can go through all the pokemon one by one"): the
// Pokedex's 151 in order, or its zone's own; the grid behind follows to the entry's page, so going back lands there
void v2DexEntryPos(int16_t dex, int &pos, int &n) {
  if (zoneScreen && zoneDetail) {
    n = zoneSize(zoneCur);
    for (pos = 0; pos < n && zoneMember(zoneCur, pos) != dex; pos++) {}
    if (pos >= n) pos = 0;
  } else {
    n = 151;
    pos = dex - 1;
  }
}
void v2DexEntryStep(int d) {
  bool z = zoneScreen && zoneDetail;
  int pos, n;
  v2DexEntryPos(z ? zoneDetail : galleryDetail, pos, n);
  if (pos + d < 0 || pos + d >= n) return;
  int16_t dex = z ? zoneMember(zoneCur, pos + d) : pos + d + 1;
  if (!dex) return;
  if (z) {
    zoneDetail = dex;
    zonePage = (pos + d) / DEXG_PAGE;  // (ZONE_PAGE, as the sketch has it further down)
  } else {
    galleryDetail = dex;
    galleryPage = (dex - 1) / DEXG_PAGE;
    galleryDirty = true;
  }
  dexDetailLoad(dex);
}
bool v2DexEntryArrowTap(int16_t x, int16_t y) {
  int pos, n;
  v2DexEntryPos(zoneScreen && zoneDetail ? zoneDetail : galleryDetail, pos, n);
  int d = pageArrowAt(x, y, pos, n, V2_PAGE_ARROW_Y);
  if (!d) return false;
  sfxPlay(SFX_TAP);
  v2DexEntryStep(d);
  return true;
}
// V2: an entry's side buttons and its X
bool v2PressDexEntry(int16_t x, int16_t y, PressTarget &t) {
  int pos, n;
  v2DexEntryPos(zoneScreen && zoneDetail ? zoneDetail : galleryDetail, pos, n);
  return v2PressNav(x, y, t, pos, n, V2_PAGE_ARROW_Y, true);
}
// V2: the trophy room's (Fedor: "The medal room needs to have hover states since they are buttons. Its hard to know
// what is clickable"). A medal's card: its side buttons, its X. A shelf: its side buttons, its X, each thing with its
// name. The room: its X, each of its five groups outlined where it stands, its name and how many of it you have
bool v2PressTrophies(int16_t x, int16_t y, PressTarget &t) {
  if (achDetail >= 0) {
    int g = v2TrophyGroup(achDetail);
    return v2PressNav(x, y, t, v2TrophyPos(achDetail) - V2_TR_GROUP_FIRST[g], v2TrophyGroupSize(g), V2_PAGE_ARROW_Y, true);
  }
  if (achGroup >= 0) {
    if (v2PressNav(x, y, t, achGroup, V2_TR_GROUPS, V2_PAGE_ARROW_Y, true)) return true;
    int m = v2ShelfAt(achGroup, x, y), x0, y0, w, h;
    if (m < 0) return false;
    v2ShelfCell(achGroup, v2TrophyPos(m) - V2_TR_GROUP_FIRST[achGroup], x0, y0, w, h);
    return v2PressSet(t, x0 + 4, y0 + 2, w - 8, h - 6, medalName(m));
  }
  if (v2PressNav(x, y, t, 0, 0, 0, true)) return true;
  int g = v2TrophyGroupAt(x, y), bx, by, bw, bh;
  if (g < 0) return false;
  v2TrophyGroupBox(g, bx, by, bw, bh);
  v2ShelfTitle(g, v2PressBuf, sizeof(v2PressBuf));
  return v2PressSet(t, bx, by, bw, bh, v2PressBuf);
}
