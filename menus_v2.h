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
// the red pixel X, always at (233, 422): a disc on the 4 px grid, a dark rim, a lit top row, an X of cells
static void v2CloseX() {
  static const char *const X[5] = { "xx...xx", ".xx.xx.", "..xxx..", ".xx.xx.", "xx...xx" };
  static const ScKey K[1] = { { 'x', 0xFFFFFF } };
  scScreen();
  scGradeTod = 0;
  scOval(scE(CX - 26, V2_CLOSE_Y - 26, CX + 26, V2_CLOSE_Y + 26), 0x3A4A5A);
  scOval(scE(CX - 22, V2_CLOSE_Y - 22, CX + 22, V2_CLOSE_Y + 22), 0xE8503A, 0xF08070, 0xC03A28);
  scSprite(X, 5, CX - 14, V2_CLOSE_Y - 10, 4, K, 1);
}
// a page button: a blue pixel disc, a lit top row, a stepped arrow (dir -1 left, 1 right)
static void v2PageArrow(int x, int y, int dir) {
  static const char *const L[5] = { "...x", "..xx", ".xxx", "..xx", "...x" }, *const R[5] = { "x...", "xx..", "xxx.", "xx..", "x..." };
  static const ScKey K[1] = { { 'x', 0xFFFFFF } };
  scScreen();
  scGradeTod = 0;
  scOval(scE(x - 24, y - 24, x + 24, y + 24), 0x1B1B25);
  scOval(scE(x - 20, y - 20, x + 20, y + 20), 0x4F93C4, 0x7CC4EA, 0x3A78A8);
  scSprite(dir < 0 ? L : R, 5, x - 8 + (dir > 0 ? 2 : -2), y - 10, 4, K, 1);
}
// page dots above the X: white for the page on show
static void v2Dots(int n, int cur, int y = 386) {
  for (int i = 0; i < n; i++) gfx->fillCircle(CX - (n - 1) * 9 + i * 18, y, 4, i == cur ? UI_WHITE : V2_INK_MUTED);
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
static void v2Switch(int x, int y, bool on) {
  gfx->fillRoundRect(x, y, 48, 24, 12, on ? C565(0x58, 0xb8, 0x68) : C565(0xd8, 0xd2, 0xbd));
  gfx->fillCircle(on ? x + 36 : x + 12, y + 12, 10, UI_WHITE);
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

// which version this is, quietly, in a panel
static void v2Version(int y) {
  char ver[28];
  snprintf(ver, sizeof(ver), "PokeKnomiGochi v%s", FW_VERSION);
  gfx->setTextColor(V2_INK_MUTED);
  setSize(2);
  setCur(centerX(ver, 2), y);
  printT(ver);
}

// ----- start over -----
// The time page's START OVER (every build that has the question, its box in layout_v2.h): it asks
// "Start over?" on the home screen, with what it erases, before anything goes.
// "Start over?" and what it erases, in a panel over the dimmed home; ERASE EVERYTHING (red) and KEEP
// PLAYING (green) fill V1's two buttons exactly, so their taps are V1's
static void v2ResetDialog() {
  dimFrame();
  v2Panel(73, 92, 393, 340, 12);
  const char *q = T(S_RESET_Q);
  int qs = textW(q, 3) <= 300 ? 3 : 2;
  gfx->setTextColor(UI_INK);
  setSize(qs);
  setCur(centerX(q, qs), 108);
  printT(q);
  // the warning, wrapped by its width at the spaces, up to three lines
  const char *w = T(S_RESET_WARN);
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
  v2LabelButton(96, 209, 370, 255, T(S_RESET_GO), UI_BAR_BAD);
  v2LabelButton(96, 271, 370, 317, T(S_RESET_KEEP), C565(0x58, 0xb8, 0x68));
}

// ----- settings -----
#ifdef BOARD_KNOMI_V2  // as V1's: the Knomi's own (its Wi-Fi and updates)
// the Knomi's settings page: Wi-Fi, updates and the auto-update switch in a panel (settings_dim())
static void v2Settings() {
  v2MenuBackdrop();
  v2Sign(T(S_SETTINGS));
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
  int lw = textW(T(S_AUTO_UPDATE), 2), x0 = CX - (lw + 14 + 48) / 2;
  gfx->setTextColor(UI_INK);
  setCur(x0, AUTO_ROW_Y + (AUTO_ROW_H - 16) / 2);
  printT(T(S_AUTO_UPDATE));
  v2Switch(x0 + lw + 14, AUTO_ROW_Y + (AUTO_ROW_H - 24) / 2, autoUpdate);
  if (!otaChecking && (os == OTA_LATEST || os == OTA_FAILED)) {
    const char *m = os == OTA_LATEST ? T(S_UPD_LATEST) : T(S_UPD_FAILED);
    gfx->setTextColor(os == OTA_LATEST ? C565(0x3c, 0x8a, 0x4c) : UI_BAR_BAD);
    setCur(centerX(m, 2), AUTO_ROW_Y + AUTO_ROW_H + 8);
    printT(m);
  }
  if (clockTzUi) v2Version(AUTO_ROW_Y + AUTO_ROW_H + 30);  // which version this is (the time page has no room)
  drawPageArrows(1, 2, V2_PAGE_ARROW_Y);  // back to the time
  v2Dots(2, 1);
  v2CloseX();
}
#endif

// ----- the card: profile, battle stats, training, progress -----
static const uint16_t V2_STAT_COL[4] = { C565(0xe8, 0x50, 0x3a), C565(0x4f, 0x93, 0xc4), C565(0xf0, 0xc0, 0x4a), C565(0x8e, 0x8c, 0x96) };
// a stat in a panel: its name, a bar, the number on the right (stat_row()); a boosted number is green
static void v2StatRow(int y, const char *lab, int v, int maxv, uint16_t col, bool boosted = false, bool maxed = false) {
  setSize(2);
  gfx->setTextColor(UI_INK);
  setCur(108, y);
  printT(lab);
  gfx->fillRoundRect(162, y + 1, 139, 13, 4, C565(0xd8, 0xd2, 0xbd));
  int fw = 134 * (v < maxv ? v : maxv) / maxv;
  if (fw > 2) gfx->fillRoundRect(164, y + 3, fw, 9, 3, col);
  char num[8];
  if (maxed) strlcpy(num, "MAX", sizeof(num));
  else snprintf(num, sizeof(num), "%d", v);
  gfx->setTextColor(maxed ? col : boosted ? C565(0x1e, 0x7d, 0x3c) : UI_INK);
  setCur(358 - textW(num, 2), y);
  printT(num);
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
static void v2CardProfile() {
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  char head[32];
  snprintf(head, sizeof(head), T(S_NAME_FMT), pet.shiny ? "*" : "", nm, pet.level());
  int right = v2Sign(head);
  drawMap(SPR_PENCIL, 10, right + 4, V2_BOARD_Y + 2, 2, false);  // the name can be changed: tap the board
  cardNameBottom = V2_BOARD_Y + 32;
  int top = V2_BOARD_Y + 34;
  if (pet.nick[0]) {  // its species under a nickname
    char par[32];
    snprintf(par, sizeof(par), "(%s)", dexName(pet.speciesId));
    gfx->setTextColor(V2_ON_DARK);
    setSize(2);
    setCur(centerX(par, 2), V2_BOARD_Y + 34);
    printT(par);
    cardNameBottom = top = V2_BOARD_Y + 54;
  }
  v2CardMon(290, 290 - top - 6, 4);
  v2Panel(92, 300, 374, 374, 10);
  // the streak with its flame
  gfx->fillTriangle(110, 312, 104, 324, 116, 324, UI_BAR_BAD);
  gfx->fillTriangle(110, 317, 107, 324, 113, 324, UI_BAR_WARN);
  char rl[40];
  snprintf(rl, sizeof(rl), T(S_STREAK_FMT), pet.streak, pet.bestStreak);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(124, 310);
  printT(rl);
  // the bond
  setCur(104, 332);
  printT(T(S_VIN));
  int bx = 104 + textW(T(S_VIN), 2) + 12;
  if (bx < 160) bx = 160;
  gfx->fillRect(bx, 334, 300 - bx, 10, C565(0xd8, 0xd2, 0xbd));
  int bw = (296 - bx) * (pet.bond > 100 ? 100 : pet.bond) / 100;
  if (bw > 0) gfx->fillRect(bx + 2, 336, bw, 6, C565(0xf0, 0x8a, 0xa4));
  // the favourite berry and the age
  const char *berry = !pet.berryKnown ? T(S_BERRY_UNK) : pet.lovesBerry(0) ? T(S_BERRY_RED) : pet.lovesBerry(1) ? T(S_BERRY_BLUE) : T(S_BERRY_GREEN);
  char info[48];
  snprintf(info, sizeof(info), T(S_INFO_FMT), berry, (unsigned long)(pet.ageMinutes / 1440));
  int x = 104;
  if (pet.berryKnown) {
    static const char *const *const BERRY_ICON[3] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP };
    drawMap(BERRY_ICON[pet.lovesBerry(0) ? 0 : pet.lovesBerry(1) ? 1 : 2], HOME_ICON_N, 100, 350, 1, false);
    x = 124;
  }
  gfx->setTextColor(UI_INK);
  setCur(x, 354);
  printT(info);
}
static void v2CardStats() {
  v2Sign(T(S_BATTLE));
  v2CardMon(184, 184 - V2_BOARD_Y - 34, 3);
  v2Panel(92, 190, 374, 372, 10);
  int bon = partyBoostOn() ? PARTY_STAT_BONUS : 0;  // friends around: what wild battles use
#ifdef BOARD_KNOMI_V2
  const int step = 33;
#else
  const int step = 24;  // room for the battle button under the bars (the Knomi has its own button)
#endif
  v2StatRow(204, T(S_STAT_ATK), pet.atkStat() + bon, 260, V2_STAT_COL[0], bon > 0);
  v2StatRow(204 + step, T(S_STAT_DEF), pet.defStat() + bon, 260, V2_STAT_COL[1], bon > 0);
  v2StatRow(204 + 2 * step, T(S_STAT_SPE), pet.speStat() + bon, 260, V2_STAT_COL[2], bon > 0);
  v2StatRow(204 + 3 * step, T(S_STAT_WGT), pet.weight, 100, V2_STAT_COL[3]);
#ifndef BOARD_KNOMI_V2
  v2LabelButton(112, 300, 354, 334, T(S_BTL_START), UI_BAR_BAD);
#endif
  char wins[40];
  snprintf(wins, sizeof(wins), T(S_WINS_FMT), pet.battleWins);
  setSize(2);
  int ww = textW(wins, 2), x0 = CX - (ww + 32) / 2;
#ifdef BOARD_KNOMI_V2
  const int wy = 344;
#else
  const int wy = 346;
#endif
  drawMap(SPR_TROPHY, 12, x0, wy - 4, 2, false);
  gfx->setTextColor(UI_INK);
  setCur(x0 + 32, wy);
  printT(wins);
}
static void v2CardTraining() {
  v2Sign(T(S_TRAINING));
  v2Panel(92, 70, 374, 372, 10);
  struct { const char *lab; uint8_t v; const char *how; bool maxes; } rows[4] = {
    { T(S_STAT_ATK), pet.trAtk, T(S_TRAIN_HOW_ATK), true }, { T(S_STAT_DEF), pet.trDef, T(S_TRAIN_HOW_DEF), true },
    { T(S_STAT_SPE), pet.trSpe, T(S_TRAIN_HOW_SPE), true }, { T(S_STAT_WGT), pet.weight, T(S_TRAIN_HOW_WGT), false } };
  for (int i = 0; i < 4; i++) {
    int y = 84 + i * 70;
    v2StatRow(y, rows[i].lab, rows[i].v, 100, V2_STAT_COL[i], false, rows[i].maxes && rows[i].v >= 100);
    gfx->setTextColor(V2_INK_MUTED);
    setCur(108, y + 22);
    printT(rows[i].how);
  }
}
static void v2CardProgress() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  v2Sign(T(S_PROGRESS));
  v2Panel(92, 70, 374, 372, 10);
  char lv[10];
  snprintf(lv, sizeof(lv), T(S_LVL_FMT), pet.level());
  gfx->setTextColor(UI_INK);
  setSize(4);
  v2Bold(lv, centerX(lv, 4) - 1, 88);
  bool top = pet.level() >= LEVEL_MAX;  // a level is an hour of play; full, and MAX, at the top
  uint8_t into = pet.ageMinutes % MINUTES_PER_LEVEL;
  gfx->fillRoundRect(118, 136, 231, 17, 6, C565(0xd8, 0xd2, 0xbd));
  int fw = top ? 226 : 226 * into / MINUTES_PER_LEVEL;
  if (fw > 2) gfx->fillRoundRect(120, 138, fw, 13, 5, C565(0x58, 0xb8, 0x68));
  char nx[32];
  if (top) strlcpy(nx, "MAX", sizeof(nx));
  else snprintf(nx, sizeof(nx), T(S_NEXT_LVL_FMT), MINUTES_PER_LEVEL - into, pet.level() + 1);
  gfx->setTextColor(V2_INK_MUTED);
  setSize(2);
  setCur(centerX(nx, 2), 162);
  printT(nx);
  setCur(centerX(T(S_EVO_LABEL), 2), 206);
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
  setCur(centerX(evo, 2), 230);
  printT(evo);
  char ms[24];  // the slip-ups, which hold evolution back
  snprintf(ms, sizeof(ms), T(S_MISTAKES_FMT), pet.careMistakes);
  gfx->setTextColor(pet.careMistakes > 0 ? UI_BAR_BAD : C565(0x3c, 0x8a, 0x4c));
  setCur(centerX(ms, 2), 286);
  printT(ms);
}
// the card's four pages (the medals' pages are still V1's: step 5b)
static void v2Card() {
  v2MenuBackdrop();
  if (cardPage == 0) v2CardProfile();
  else if (cardPage == 1) v2CardStats();
  else if (cardPage == 2) v2CardTraining();
  else v2CardProgress();
  drawPageArrows(cardPage, cardPageCount(), V2_PAGE_ARROW_Y);
  v2Dots(cardPageCount(), cardPage);
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
static void v2Clock(const char *langCode) {
  bool locked = clockLocked();
  v2MenuBackdrop();
  v2Sign(locked ? T(S_CLOCK) : T(S_SET_TIME));
  v2Panel(72, 92, 394, 280, 10);
  gfx->setTextColor(UI_INK);
  if (locked) {  // network time or the computer's: shown as it runs, with where it comes from
    uint32_t e = rtcEpoch();
    drawBigTime((e / 3600) % 24, (e / 60) % 60);
    const char *st = clockTzUi ? T(S_SET_BY_WIFI) : T(S_SET_AUTO);
    gfx->setTextColor(C565(0x3c, 0x8a, 0x4c));
    setSize(2);
    setCur(centerX(st, 2), clockTzUi ? TIME_STATUS_Y : TIME_STATUS_Y + 16);
    printT(st);
    if (!clockTzUi) v2Version(240);  // in the panel (with the time zone there, it's on the settings page)
#ifdef BOARD_KNOMI_V2
    if (clockTzUi) {
      gfx->setTextColor(V2_INK_MUTED);
      setCur(centerX(T(S_TIME_ZONE), 2), TZ_LABEL_Y);
      printT(T(S_TIME_ZONE));
      char b[32];
      int i = clockTzIndex();
      snprintf(b, sizeof(b), "%s >", TIME_ZONES[i >= 0 ? i : TIME_ZONE_DEFAULT].city);
      v2LabelButton(TZ_BTN_X + 3, TZ_BTN_Y + 3, TZ_BTN_X + TZ_BTN_W - 3, TZ_BTN_Y + TZ_BTN_H - 3, b, C565(0x4f, 0x93, 0xc4));
    }
#endif
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
  // sound, left, and the language, right, both one tap
  bool snd = audioEnabled();
  v2LabelButton(37, LANG_PILL_Y + 3, 127, LANG_PILL_Y + LANG_PILL_H - 3, snd ? T(S_SND_ON) : T(S_SND_OFF),
                snd ? C565(0x58, 0xb8, 0x68) : C565(0x8e, 0x8c, 0x96));
  char lp[10];
  snprintf(lp, sizeof(lp), "%s >", langCode);
  v2LabelButton(LANG_PILL_X + 3, LANG_PILL_Y + 3, LANG_PILL_X + LANG_PILL_W - 3, LANG_PILL_Y + LANG_PILL_H - 3, lp,
                C565(0x4f, 0x93, 0xc4));
  if (!locked) {
    v2LabelButton(136, 343, 330, 385, "OK", C565(0x58, 0xb8, 0x68), UI_WHITE, 3);
  } else {
#ifdef BOARD_KNOMI_V2
    v2LabelButton(V2_RESET_X0, V2_RESET_Y0, V2_RESET_X1, V2_RESET_Y1, T(S_RESET_BTN), UI_BAR_BAD);  // asks first
#endif
#if defined(BOARD_KNOMI_V2) && !defined(BOARD_HOST)
    v2Dots(2, 0);  // the settings are the next page (the Knomi's own)
#endif
  }
#if defined(BOARD_KNOMI_V2) && !defined(BOARD_HOST)
  drawPageArrows(0, 2, V2_PAGE_ARROW_Y);  // on to the settings
#endif
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
  setSize(3);
  setCur(centerX(city, 3), TZ_CITY_Y);
  printT(city);
  char off[16];
  fmtUtcOffset(off, sizeof(off), (int32_t)(l - utc));
  gfx->setTextColor(V2_INK_MUTED);
  setSize(2);
  setCur(centerX(off, 2), TZ_CITY_Y + 40);
  printT(off);
  drawPageArrows(tzPickIdx, TIME_ZONE_COUNT, TZ_ARROW_Y);
  v2LabelButton(136, 343, 330, 385, "OK", C565(0x58, 0xb8, 0x68), UI_WHITE, 3);
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
// under the art: the type chip on the room, then the next form and the fact in a panel (dex_entry())
static void v2DexInfo(int16_t dex, bool withFact) {
  const DexEntry &d = DEX_TBL[dex];
  setSize(2);
  const char *ty = TYPE_NAME[d.type];
  int w = textW(ty, 2) + 24;
  gfx->fillRoundRect(CX - w / 2, 250, w, 24, 8, d.accent);
  gfx->setTextColor(UI_WHITE);
  setCur(centerX(ty, 2), 255);
  printT(ty);
  v2Panel(60, 280, 406, withFact ? 386 : 306, 10);
  char line[40];
  gfx->setTextColor(V2_INK_MUTED);
  if (d.evolvesTo) {  // the next form isn't shown until it's found
    snprintf(line, sizeof(line), "Lv.%u > %s", d.evolveLevel, pet.isRegistered(d.evolvesTo) ? dexName(d.evolvesTo) : "???");
    setCur(centerX(line, 2), 286);
    printT(line);
  } else {
    setCur(centerX(T(S_FINAL_FORM), 2), 286);
    printT(T(S_FINAL_FORM));
  }
  if (!withFact) return;
  gfx->setTextColor(UI_INK);
  const char *p = DEX_FACT[dex];
  for (int y = 308; *p && y <= 368; y += 20) {
    int skip, n = wrapNext(p, 28, skip);
    snprintf(line, sizeof(line), "%.*s", n, p);
    setCur(centerX(line, 2), y);
    printT(line);
    p += skip;
  }
}

// ----- the trophy room (the achievements) and a medal's card -----
// All 25 medals as things in one room (rooms_px.py trophy_room()): the five zone badges in a case, the Pokedex
// statues and the shiny gem on the top shelf, the level 100 cups on the next, three frames on the left wall
// and the pet's eight rosettes on the right. A medal not won yet is its shape in shadow. Tap one for its card
// (medal_won(), medal_locked()): the room dimmed under a spotlight, the medal big on a plinth, its name on the
// board, how to win it and how far along you are, the page buttons stepping through all 25 in the room's order.
#define TR_WALL 0x8A5A3A
#define TR_WALL_D 0x6E452A
#define TR_WALL_L 0xA8744C
#define TR_GOLD 0xF0C04A
#define TR_GOLD_D 0xC08410
#define TR_GOLD_L 0xFFF0B0
#define TR_SIL 0x4A2E1E  // a medal not won yet
#define TR_INK 0x1B1B25
// the room's order (V2_TROPHY_ORDER) is in layout_v2.h: the taps and swipes need it early in the sketch
static bool v2Won(int medal, bool bare) { return !bare && pet.hasMedal(1UL << medal); }

// a rosette: a disc and a white middle over two tails (k: its size; the room's is 1)
static void trRosette(int x, int y, Rgb col, bool won, int k) {
  Rgb c = won ? col : TR_SIL, c2 = won ? scMix(col, TR_INK, .25) : TR_SIL;
  scRR(x - 7 * k, y + 6 * k, x - k, y + 26 * k, c2);
  scRR(x + k, y + 6 * k, x + 7 * k, y + 26 * k, c2);
  if (k == 1) {  // the tails' notches, in the wall's colour (the big one ends square)
    scRR(x - 7, y + 22, x - 4, y + 26, TR_WALL);
    scRR(x + 4, y + 22, x + 7, y + 26, TR_WALL);
  }
  scOval(scE(x - 13 * k, y - 13 * k, x + 13 * k, y + 13 * k), c, won ? scMix(c, 0xFFFFFF, .4) : SC_NONE);
  scOval(scE(x - 6 * k, y - 6 * k, x + 6 * k, y + 6 * k), won ? 0xFFFFFF : 0x5A3A28);
}
// a cup h tall on its plinth, its foot at base, with stars on it
static void trCup(int x, int base, int h, bool won, int stars) {
  Rgb g = won ? TR_GOLD : TR_SIL, gd = won ? TR_GOLD_D : TR_SIL;
  double w = h * 0.55;
  scRR((int)(x - w * .6), base - 8, (int)(x + w * .6), base, gd);  // the plinth
  scRR(x - 3, (int)(base - h * .45), x + 3, base - 8, gd);         // the stem
  int rows = (int)(h * .55) / 4;
  for (int i = 0; i < rows; i++) {  // the bowl, narrowing down
    int yy = base - h + i * 4;
    double t = i / (h * .55 / 4), half = w / 2 * (1 - t * t * .7);
    scRR(scFl4((int)floor(x - half)) * 4, yy, scFl4((int)floor(x + half)) * 4 + 4, yy + 4, g);
  }
  scRR((int)(x - w / 2 - 6), base - h + 4, (int)(x - w / 2), base - h + 16, gd);  // the handles
  scRR((int)(x + w / 2), base - h + 4, (int)(x + w / 2 + 6), base - h + 16, gd);
  if (won) scRR((int)(x - w / 2 + 4), base - h + 4, (int)(x - w / 2 + 8), base - h + 16, TR_GOLD_L);
  for (int s = 0; s < stars; s++) {
    int sx = x - (stars - 1) * 4 + s * 8;
    scGridRect(sx, base - h + 20, sx + 4, base - h + 24, won ? 0xFFF3DC : 0x5A3A28);
  }
}
// a Poke Ball statue on a pedestal (the Pokedex medals), tinted on top
static void trBall(int x, int base, Rgb tint, bool won, int k) {
  Rgb top = won ? tint : TR_SIL, bot = won ? 0xFFFFFF : TR_SIL, band = won ? TR_INK : 0x3A2214;
  ScEll e = scE(x - 16 * k, base - 40 * k, x + 16 * k, base - 8 * k);
  scRR(x - 16 * k, base - 8 * k, x + 16 * k, base, won ? 0x5A5F7C : TR_WALL_D);
  scOval(e, bot);
  for (int cy = scFl4(e.y0) * 4; cy < base - 24 * k; cy += 4)  // the top half in the tint
    for (int cx = scFl4(e.x0) * 4; cx < e.x1; cx += 4)
      if (scInEll(cx + 2, cy + 2, e)) scCell(cx, cy, top);
  scRR(x - 16 * k, base - 26 * k, x + 16 * k, base - 22 * k, band);
  scOval(scE(x - 5 * k, base - 29 * k, x + 5 * k, base - 19 * k), bot);
  if (won) scGridRect(x - 10 * k, base - 36 * k, x - 6 * k, base - 32 * k, scMix(top, 0xFFFFFF, .5));
}
// the shiny gem under its glass dome
static void trGem(int x, int base, bool won, int k) {
  scRR(x - 18 * k, base - 6 * k, x + 18 * k, base, won ? 0x5A5F7C : TR_WALL_D);
  scOval(scE(x - 18 * k, base - 44 * k, x + 18 * k, base - 4 * k), won ? scMix(0xBFE6F5, TR_WALL, .55) : TR_WALL_D);
  Rgb c = won ? 0xF08AA4 : TR_SIL;
  static const int8_t WID[6] = { 4, 10, 14, 10, 6, 2 };
  for (int i = 0; i < 6; i++)
    scRR(x - WID[i] * k, base - (34 - i * 4) * k, x + WID[i] * k, base - (30 - i * 4) * k, i ? c : scMix(c, 0xFFFFFF, .5));
  if (won) {
    static const int8_t SP[3][2] = { {-14, -40}, {10, -22}, {12, -42} };
    for (auto &s : SP) scGridRect(x + s[0] * k, base + s[1] * k, x + s[0] * k + 4, base + s[1] * k + 4, 0xFFFFFF);
  }
}
// a zone's badge, 7 x 7 cells (BADGE_SHAPES), in the sprites' own colours
static void trBadge(int zone, int x, int y, bool won, int cell) {
  static const char *const SHAPE[5][7] = {
    { "...g...", "..ggg..", ".ggGgg.", "ggggGgg", ".ggGgg.", "...t...", "...t..." },  // the Forest
    { ".......", ".b...b.", "bBb.bBb", "b.bBb.b", ".......", "bBb.bBb", "b.bBb.b" },  // the Beach
    { "...t...", "..tTt..", "..tTt..", ".ttTtt.", ".tTTTt.", "tTTTTTt", "......." },  // the Cave
    { "..ppp..", ".ppppp.", ".pwpwp.", ".ppppp.", ".ppppp.", "pp.p.pp", "p..p..p" },  // the Mansion
    { "...f...", ".f.f.f.", "..fff..", "fffYfff", "..fff..", ".f.f.f.", "...f..." },  // the Field
  };
  for (int r = 0; r < 7; r++)
    for (int c = 0; c < 7; c++) {
      char ch = SHAPE[zone][r][c];
      if (ch == '.') continue;
      Rgb col = 0x4A1620;  // in shadow
      if (won) switch (ch) {
        case 'g': col = 0x58B868; break; case 'G': col = 0x3C8A4C; break; case 't': col = 0x8FD6B4; break;
        case 'T': col = 0x3FB8A8; break; case 'b': col = 0x7CC4EA; break; case 'B': col = 0x4F93C4; break;
        case 'p': col = 0xF08AA4; break; case 'w': col = 0xFFFFFF; break; case 'f': col = 0xFFD95E; break;
        default: col = 0xE0B860; break;  // 'Y'
      }
      scRect(x + c * cell, y + r * cell, x + c * cell + cell - 1, y + r * cell + cell - 1, col);
    }
}
// a framed picture: two friends (Buddy), five (Party), two with a console (Playmate)
static void trFrame(int x, int y, int kind, bool won, int k) {
  scRR(x, y, x + 52 * k, y + 44 * k, won ? TR_GOLD_D : 0x3A2414);
  scRR(x + 4 * k, y + 4 * k, x + 48 * k, y + 40 * k, won ? 0x9FD6F0 : 0x2A1A12);
  if (!won) return;
  scRR(x + 4 * k, y + 30 * k, x + 48 * k, y + 40 * k, 0x79BD4F);
  static const Rgb COL[5] = { 0x6F5AA8, 0xF8D038, 0xE8503A, 0x4F93C4, 0x58B868 };
  int n = kind == 1 ? 5 : 2;
  for (int i = 0; i < n; i++) {
    int cx = x + (10 + i * (32 / (n - 1))) * k;
    scRR(cx - 3 * k, y + 20 * k, cx + 5 * k, y + 30 * k, COL[i]);
  }
  if (kind == 2) scRR(x + 20 * k, y + 10 * k, x + 32 * k, y + 18 * k, 0x3A4A5A);
}
static const Rgb TR_ROSETTE[8] = { 0x58B868, 0x4F93C4, 0x6F5AA8, 0xE8503A, 0xF5863D, 0xF08AA4, TR_GOLD, 0x8FD6B4 };
static const Rgb TR_STATUE[4] = { 0xC08450, 0xAAB0BC, TR_GOLD, 0xE8503A };
static const uint8_t TR_CUP_H[4] = { 34, 40, 46, 54 }, TR_CUP_STARS[4] = { 1, 3, 5, 0 };
static int trCupIndex(int medal) { return medal == 13 ? 0 : medal - 21; }  // MED_LV100, then LV100_3/5/10 (bits 22-24)

// the room; bare: every medal in shadow (behind a medal's card)
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
  for (int i = 0; i < 5; i++) trBadge(i, 150 + i * 40, 66, v2Won(17 + i, bare), 4);
  for (int sy : { 170, 246 }) {  // two shelves
    scRR(84, sy, 382, sy + 8, 0x5A3A22);
    scRR(84, sy, 382, sy + 2, TR_WALL_L);
    scRR(92, sy + 8, 100, sy + 18, 0x3A2414);
    scRR(366, sy + 8, 374, sy + 18, 0x3A2414);
  }
  for (int i = 0; i < 4; i++) trBall(112 + i * 56, 170, TR_STATUE[i], v2Won(9 + i, bare), 1);
  trGem(344, 170, v2Won(8, bare), 1);
  for (int i = 0; i < 4; i++) trCup(126 + i * 72, 246, TR_CUP_H[i], v2Won(i ? 21 + i : 13, bare), TR_CUP_STARS[i]);
  static const int16_t FRAME[3][2] = { {56, 262}, {114, 262}, {56, 308} };
  for (int i = 0; i < 3; i++) trFrame(FRAME[i][0], FRAME[i][1], i, v2Won(14 + i, bare), 1);
  for (int i = 0; i < 8; i++) trRosette(296 + (i % 4) * 34, 270 + (i / 4) * 40, TR_ROSETTE[i], v2Won(i, bare), 1);
}
// each medal's place in the room, for a tap: a box round its thing, at least as big as the spacing allows
static bool trBox(int medal, int &x0, int &y0, int &x1, int &y1) {
  if (medal >= 17 && medal <= 21) { int x = 150 + (medal - 17) * 40; x0 = x - 6, y0 = 56, x1 = x + 34, y1 = 104; return true; }
  if (medal >= 9 && medal <= 12) { int x = 112 + (medal - 9) * 56; x0 = x - 26, y0 = 118, x1 = x + 26, y1 = 178; return true; }
  if (medal == 8) { x0 = 318, y0 = 118, x1 = 370, y1 = 178; return true; }
  if (medal == 13 || (medal >= 22 && medal <= 24)) { int x = 126 + trCupIndex(medal) * 72; x0 = x - 34, y0 = 186, x1 = x + 34, y1 = 254; return true; }
  if (medal >= 14 && medal <= 16) {
    static const int16_t FRAME[3][2] = { {56, 262}, {114, 262}, {56, 308} };
    x0 = FRAME[medal - 14][0], y0 = FRAME[medal - 14][1], x1 = x0 + 52, y1 = y0 + 44;
    return true;
  }
  if (medal >= 0 && medal <= 7) { int x = 296 + (medal % 4) * 34, y = 270 + (medal / 4) * 40; x0 = x - 17, y0 = y - 16, x1 = x + 17, y1 = y + 24; return true; }
  return false;
}
static int v2TrophyAt(int16_t x, int16_t y) {
  for (int m = 0; m < MED_COUNT; m++) {
    int x0, y0, x1, y1;
    if (trBox(m, x0, y0, x1, y1) && x >= x0 && x <= x1 && y >= y0 && y <= y1) return m;
  }
  return -1;
}

// the room, from the scene buffer (painted again when the medals change); the count on a board on the carpet
static void v2PaintTrophyRoom(int) { trRoom(false); }
static void v2TrophyRoom() {
  uint32_t won = 0;
  int got = 0;
  for (int m = 0; m < MED_COUNT; m++)
    if (pet.hasMedal(1UL << m)) won |= 1UL << m, got++;
  v2Cached(0x40000000 | (int)won, v2PaintTrophyRoom, 0);
  char b[32];
  snprintf(b, sizeof(b), T(S_MEDALS_FMT), got, MED_COUNT);
  v2SignIn(b, 350, C565(0x6e, 0x1e, 0x2a), C565(0x8a, 0x2a, 0x38));
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
// a medal's thing, big, standing on the plinth (its foot at base)
static void trBig(int m, bool won) {
  const int x = CX, base = 262;
  if (m >= 17 && m <= 21) trBadge(m - 17, x - 42, base - 100, won, 12);
  else if (m >= 9 && m <= 12) trBall(x, base, TR_STATUE[m - 9], won, 3);
  else if (m == 8) trGem(x, base, won, 3);
  else if (m == 13 || (m >= 22 && m <= 24)) {
    int i = trCupIndex(m), h = TR_CUP_H[i] * 27 / 10;
    trCup(x, base, h > 130 ? 130 : h, won, TR_CUP_STARS[i]);
  } else if (m >= 14 && m <= 16) trFrame(x - 78, base - 140, m - 14, won, 3);
  else trRosette(x, base - 100, TR_ROSETTE[m], won, 3);
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
  if (won) {  // it shines
    static const int16_t SP[3][2] = { {180, 150}, {290, 140}, {300, 210} };
    for (auto &s : SP) {
      scGridRect(s[0], s[1] - 8, s[0] + 4, s[1] + 12, 0xFFFFFF);
      scGridRect(s[0] - 8, s[1], s[0] + 12, s[1] + 4, 0xFFFFFF);
    }
  } else {
    gfx->setTextColor(C565(0x8a, 0x6a, 0x50));  // not yet: a question mark over its shadow
    setSize(4);
    setCur(centerX("?", 4), 186);
    printT("?");
  }
  if (won) v2SignIn(medalName(m), V2_BOARD_Y, C565(0xc0, 0x84, 0x10), C565(0xf0, 0xc0, 0x4a));
  else v2SignIn(medalName(m), V2_BOARD_Y, C565(0x5a, 0x3a, 0x22), C565(0x7a, 0x5a, 0x3a));
  v2Panel(72, 296, 394, 384, 10);
  // how to win it, wrapped by its width: three lines, or two over a progress bar
  int cur, goal;
  bool prog = !won && v2MedalProgress(m, cur, goal);
  const char *w = medalHow(m);
  char line[64];
  gfx->setTextColor(UI_INK);
  setSize(2);
  int y = 302;
  for (int ln = 0; *w && ln < (prog ? 2 : 3); ln++, y += 18) {
    int n = 0, fit = 0;
    for (;;) {
      int e = n;
      while (w[e] == ' ') e++;
      while (w[e] && w[e] != ' ') e++;
      snprintf(line, sizeof(line), "%.*s", e, w);
      if (fit && textW(line, 2) > 306) break;
      fit = n = e;
      if (!w[e]) break;
    }
    snprintf(line, sizeof(line), "%.*s", fit, w);
    setCur(centerX(line, 2), y);
    printT(line);
    w += fit;
    while (*w == ' ') w++;
  }
  if (won) {
    gfx->setTextColor(C565(0x3c, 0x8a, 0x4c));
    setCur(centerX(T(S_MEDAL_GOT), 2), 362);
    printT(T(S_MEDAL_GOT));
  } else if (prog) {
    if (cur > goal) cur = goal;
    gfx->fillRect(126, 342, 214, 12, C565(0xd8, 0xd2, 0xbd));
    if (cur > 0) gfx->fillRect(128, 344, 210 * cur / goal, 8, C565(0x58, 0xb8, 0x68));
    char b[16];
    snprintf(b, sizeof(b), "%d / %d", cur, goal);
    gfx->setTextColor(V2_INK_MUTED);
    setCur(centerX(b, 2), 362);
    printT(b);
  } else {
    gfx->setTextColor(V2_INK_MUTED);
    setCur(centerX(T(S_MEDAL_TODO), 2), 362);
    printT(T(S_MEDAL_TODO));
  }
  int pos = v2TrophyPos(m), group = 0;
  for (int g = 0; g < 5; g++)
    if (pos >= V2_TROPHY_GROUP[g]) group = g;
  drawPageArrows(pos, MED_COUNT, V2_PAGE_ARROW_Y);
  v2Dots(5, group, 390);
  v2CloseX();
}
