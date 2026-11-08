// V2 step 7: fights pay, the Master Ball (Fedor). scene_v2.h includes this at its end, so it's only there with
// UI_V2 on.
//
// Every wild or Travel battle fought to the end, won or lost (not run from; buddy battles and the dungeon don't
// count), fills a bar a step; 25 earn a Master Ball (pet.masterFight()), which always catches what it hits. They
// stack. The bar shows at a fight's end and on the card's Progress page; the fight that fills it brings a moment of
// its own, on the Caught screen's gold burst. On the catch, with one in hand, the Master Ball sits left of the Poke
// Ball: tap it to throw it. A hit always holds and uses it up; a miss doesn't use it (it comes back). Only the three
// Poke Balls count (Fedor): the catch ends when the third one fails (a miss, or it breaks out) or anything catches it,
// so a Master Ball can be thrown any time before then, even after a Poke Ball it broke out of.

// ----- the ball: purple, two pink spots, a white M, as the Poke Ball's 18 x 18 sprite -----
static const char *const SPR_V2_MASTER_BALL[18] = {
  "......kkkkkk......",
  "....kkwqqqqwkk....",
  "...kmmwwqqwwmmk...",
  "..kqmmwqwwqwmmQk..",
  ".kqqqqqkkkkqqqQQk.",
  ".kqqqqkkkkkkqqQQk.",
  "kqqqqkkkwwkkkqQQQk",
  "kkkkkkkwwwskkkkkkk",
  "kkkkkkkwwsskkkkkkk",
  "kwwwwkkksskkkwwwSk",
  "kwwwwwkkkkkkwwwSSk",
  ".kwwwwwkkkkwwwwSk.",
  ".kwwwwwwwwwwwwSSk.",
  "..kwwwwwwwwwwSSk..",
  "...kSwwwwwwwSSk...",
  "....kkSSSSSSkk....",
  "......kkkkkk......",
  "..................",
};
static uint16_t v2MasterInk(char c) {
  switch (c) {
    case 'q': return C565(0x78, 0x48, 0xb8);
    case 'Q': return C565(0x4e, 0x2c, 0x80);
    case 'm': return C565(0xe8, 0x5a, 0xa8);
    case 'w': return UI_WHITE;
    case 's': return C565(0xaa, 0xb0, 0xbc);
    case 'S': return C565(0x78, 0x7e, 0x8c);
    default: return C565(0x1b, 0x1b, 0x25);
  }
}
// turned deg about (ax, ay) of the sprite, that point at (px, py), cells of s px: drawMapTurned() in its colours
static void v2MasterBallTurned(int s, float deg, float px, float py, float ax, float ay) {
  const int n = HOME_ICON_N;
  float a = deg * 0.0174533f, ca = cosf(a), sa = sinf(a), reach = 0;
  for (int i = 0; i < 4; i++) reach = fmaxf(reach, hypotf(((i & 1) ? n : 0) - ax, ((i & 2) ? n : 0) - ay));
  int cells = (int)reach + 1;
  for (int cy = -cells; cy < cells; cy++) {
    int runC = 0, runK = -1;
    for (int cx = -cells; cx <= cells; cx++) {
      int k = -1;
      if (cx < cells) {
        float dx = cx + 0.5f, dy = cy + 0.5f;
        float u = dx * ca + dy * sa + ax, v = dy * ca - dx * sa + ay;
        if (u >= 0 && v >= 0 && u < n && v < n && SPR_V2_MASTER_BALL[(int)v][(int)u] != '.') k = SPR_V2_MASTER_BALL[(int)v][(int)u];
      }
      if (k == runK) continue;
      if (runK >= 0) gfx->fillRect((int)px + runC * s, (int)py + cy * s, (cx - runC) * s, s, v2MasterInk((char)runK));
      runC = cx;
      runK = k;
    }
  }
}
static void v2MasterBall(int x, int y, int s) { v2MasterBallTurned(s, 0, x, y, 0, 0); }  // its top left at (x, y)

// ----- the bar: the ball, how many you hold, the steps to the next -----
// the ball at (x, y) at 2x, the bar and "7/25" to its right, w in all; full: the fight that just earned one (25/25,
// before its moment), the ball it earned not counted yet
static void v2MasterBar(int x, int y, int w, bool full) {
  v2MasterBall(x, y, 2);
  uint8_t held = full ? pet.masterBalls - 1 : pet.masterBalls, steps = full ? MASTER_FIGHTS : pet.masterBar;
  if (held) drawCountBadge(x + 34, y + 4, held);
  char b[12];
  snprintf(b, sizeof(b), "%u/%u", steps, MASTER_FIGHTS);
  bool cjk = classicFontBegin();
  setSize(2);
  int tw = textW(b, 2), bx = x + 50, bw = w - 50 - tw - 10;
  gfx->fillRoundRect(bx, y + 11, bw, 15, 6, C565(0xd8, 0xd2, 0xbd));
  int fw = (bw - 4) * steps / MASTER_FIGHTS;
  if (fw > 2) gfx->fillRoundRect(bx + 2, y + 13, fw, 11, 5, v2MasterInk('q'));
  gfx->setTextColor(V2_INK_MUTED);
  setCur(x + w - tw, y + 11);
  printT(b);
  classicFontEnd(cjk);
}

// ----- a fight's end -----
static bool v2MasterNew = false;    // this fight earned one (battleFinish())
static bool v2MasterThrow = false;  // the ball thrown is the Master Ball
#define V2_MASTER_SHOW_MS 1500      // the end as usual first, then the Master Ball's moment
#define V2_MASTER_END_MS 6500       // ...until the catch or home (a tap from V2_MASTER_SKIP_MS)
#define V2_MASTER_SKIP_MS 3000
// t: since the end came up. The bar in a small panel under the lines; or, past V2_MASTER_SHOW_MS of the fight that
// earned one, the gold burst, "MASTER BALL!" on the ribbon, the ball big with sparkles, what it does, how many
static void v2MasterEnd(uint32_t t) {
  if (!v2MasterNew || t < V2_MASTER_SHOW_MS) {
    v2Panel(126, 406, 340, 438, 10);  // (under the second line's box; inside the round edge)
    v2MasterBar(132, 404, 200, v2MasterNew);
    return;
  }
  char b[40];
  v2CaughtBack(v2Caps(T(S_MASTER_GOT), b, sizeof(b)));
  uint32_t now = millis();
  int bob = SNAP2((int)(4 * sinf(now * 0.004f)));
  v2PetShadow(CX, 262, 40);
  v2MasterBall(CX - 45, 150 + bob, 5);
  drawCountBadge(CX + 46, 160 + bob, pet.masterBalls);
  static const int16_t SPARK[6][2] = { { 140, 140 }, { 326, 150 }, { 120, 230 }, { 346, 226 }, { 178, 104 }, { 290, 100 } };
  scScreen();
  scGradeTod = 0;
  uint32_t beat = now / 300;
  for (int i = 0; i < 6; i++) {  // white pixel sparkles, twinkling in turn
    if ((beat + i) % 3 == 0) continue;
    scGridRect(SPARK[i][0], SPARK[i][1] - 8, SPARK[i][0] + 4, SPARK[i][1] + 12, 0xFFFFFF);
    scGridRect(SPARK[i][0] - 8, SPARK[i][1], SPARK[i][0] + 12, SPARK[i][1] + 4, 0xFFFFFF);
  }
  v2SmallPanel(T(S_MASTER_HOW), 296);
}

// ----- the throw -----
// the band under the running Pokemon, with one in hand: the Master Ball (how many) on the left and the Poke Ball (its
// throws left) on the right (Fedor), each the button for its throw: a tap on the left half throws the Master Ball
static bool v2MasterAiming() { return pet.masterBalls > 0; }
static void v2MasterAimBalls(uint8_t throws) {
  v2MasterBall(CX - 104, 372, 4);
  drawCountBadge(CX - 36, 378, pet.masterBalls);
  drawMap(SPR_HOME_BALL, HOME_ICON_N, CX + 32, 372, 4, false);
  drawCountBadge(CX + 100, 378, throws);
}
