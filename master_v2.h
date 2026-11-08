// V2 step 7: fights pay, the Master Ball and the Ultra Ball (Fedor). scene_v2.h includes this at its end, so it's
// only there with UI_V2 on.
//
// Every wild or Travel battle fought to the end, won or lost (not run from; buddy battles and the dungeon don't
// count), fills two bars a step (pet.fightPays()): ULTRA_FIGHTS of them earn an Ultra Ball, which triples the catch
// chance of a throw that hits, and MASTER_FIGHTS (Fedor: REALLY rare) a Master Ball, which always catches what it
// hits. They stack. The Ultra Ball's bar shows at a fight's end, both on the card's pages; a fight that earns one
// brings a moment of its own on the Caught screen's gold burst (a Master Ball's, when it earns both). On the catch
// the balls you hold sit in a row over the band, the Master Ball, the Ultra Ball, then the Poke Ball: tap one to throw
// it. An Ultra or a Master Ball that hits is used up; a miss doesn't use it (it comes back). Any ball that hits ends
// the catch, caught or not (a ball it breaks out of: it runs off, as in V1); the third Poke Ball missing ends it too,
// the others being optional (Fedor).

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
// ----- the Ultra Ball: a black cap with yellow stripes down its sides -----
static const char *const SPR_V2_ULTRA_BALL[18] = {
  "......kkkkkk......",
  "....kkyqqqqykk....",
  "...kwyyqqqqyyQk...",
  "..kqwyyqqqqyyQQk..",
  ".kqqqyykkkkyyQQQk.",
  ".kqqqykkkkkkyQQQk.",
  "kqqqqkkkwwkkkQQQQk",
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
static uint16_t v2UltraInk(char c) {
  switch (c) {
    case 'q': return C565(0x3a, 0x3a, 0x48);
    case 'Q': return C565(0x26, 0x26, 0x30);
    case 'y': return C565(0xf8, 0xd0, 0x40);
    case 'w': return UI_WHITE;
    case 's': return C565(0xaa, 0xb0, 0xbc);
    case 'S': return C565(0x78, 0x7e, 0x8c);
    default: return C565(0x1b, 0x1b, 0x25);
  }
}
// a ball turned deg about (ax, ay) of its sprite, that point at (px, py), cells of s px: drawMapTurned() in its colours
static void v2BallSprTurned(const char *const *spr, uint16_t (*ink)(char), int s, float deg, float px, float py, float ax,
                            float ay) {
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
        if (u >= 0 && v >= 0 && u < n && v < n && spr[(int)v][(int)u] != '.') k = spr[(int)v][(int)u];
      }
      if (k == runK) continue;
      if (runK >= 0) gfx->fillRect((int)px + runC * s, (int)py + cy * s, (cx - runC) * s, s, ink((char)runK));
      runC = cx;
      runK = k;
    }
  }
}
static void v2MasterBallTurned(int s, float deg, float px, float py, float ax, float ay) {
  v2BallSprTurned(SPR_V2_MASTER_BALL, v2MasterInk, s, deg, px, py, ax, ay);
}
static void v2UltraBallTurned(int s, float deg, float px, float py, float ax, float ay) {
  v2BallSprTurned(SPR_V2_ULTRA_BALL, v2UltraInk, s, deg, px, py, ax, ay);
}
static void v2MasterBall(int x, int y, int s) { v2MasterBallTurned(s, 0, x, y, 0, 0); }  // its top left at (x, y)
static void v2UltraBall(int x, int y, int s) { v2UltraBallTurned(s, 0, x, y, 0, 0); }

// ----- the ball thrown: a Poke Ball, an Ultra Ball or a Master Ball -----
#define V2_BALL_POKE 0
#define V2_BALL_ULTRA 1
#define V2_BALL_MASTER 2
static uint8_t v2Throw = V2_BALL_POKE;
static void v2SpecialBallTurned(int s, float deg, float px, float py, float ax, float ay) {  // (an Ultra or a Master Ball)
  if (v2Throw == V2_BALL_MASTER) v2MasterBallTurned(s, deg, px, py, ax, ay);
  else v2UltraBallTurned(s, deg, px, py, ax, ay);
}
static void v2SpecialBall(int x, int y, int s) { v2SpecialBallTurned(s, 0, x, y, 0, 0); }

// ----- the bars: the ball, how many you hold, the fights to the next -----
// the ball at (x, y) at 2x, the bar and "7/25" to its right, w in all; full: the fight that just earned one (25/25,
// before its moment), the ball it earned not counted yet
static void v2BallBar(bool master, int x, int y, int w, bool full) {
  if (master) v2MasterBall(x, y, 2);
  else v2UltraBall(x, y, 2);
  uint8_t n = master ? MASTER_FIGHTS : ULTRA_FIGHTS, held = master ? pet.masterBalls : pet.ultraBalls;
  uint8_t steps = full ? n : master ? pet.masterBar : pet.ultraBar;
  if (full && held) held--;
  if (held) drawCountBadge(x + 34, y + 4, held);
  char b[12];
  snprintf(b, sizeof(b), "%u/%u", steps, n);
  bool cjk = classicFontBegin();
  setSize(2);
  int tw = textW(b, 2), bx = x + 50, bw = w - 50 - tw - 10;
  gfx->fillRoundRect(bx, y + 11, bw, 15, 6, C565(0xd8, 0xd2, 0xbd));
  int fw = (bw - 4) * steps / n;
  if (fw > 2) gfx->fillRoundRect(bx + 2, y + 13, fw, 11, 5, master ? v2MasterInk('q') : C565(0xd0, 0xa0, 0x20));
  gfx->setTextColor(V2_INK_MUTED);
  setCur(x + w - tw, y + 11);
  printT(b);
  classicFontEnd(cjk);
}

// ----- a fight's end -----
static uint8_t v2MasterNew = 0;     // what this fight earned: 1 a Master Ball, 2 an Ultra Ball (battleFinish())
#define V2_MASTER_SHOW_MS 1500      // the end as usual first, then the Master Ball's moment
#define V2_MASTER_END_MS 6500       // ...until the catch or home (a tap from V2_MASTER_SKIP_MS)
#define V2_MASTER_SKIP_MS 3000
// t: since the end came up. The Ultra Ball's bar in a small panel under the lines (the nearer one); or, past
// V2_MASTER_SHOW_MS of a fight that earned a ball, the gold burst: its name on the ribbon, the ball big with sparkles,
// what it does, how many (a Master Ball's, when the fight earned both)
static void v2MasterEnd(uint32_t t) {
  if (!v2MasterNew || t < V2_MASTER_SHOW_MS) {
    v2Panel(126, 406, 340, 438, 10);  // (under the second line's box; inside the round edge)
    v2BallBar(false, 132, 404, 200, v2MasterNew & 2);
    return;
  }
  bool master = v2MasterNew & 1;
  char b[40], c[40];
  if (master) {
    v2Caps(T(S_MASTER_GOT), b, sizeof(b));
  } else {
    snprintf(c, sizeof(c), "%s!", T(S_ULTRA));
    v2Caps(c, b, sizeof(b));
  }
  v2CaughtBack(b);
  uint32_t now = millis();
  int bob = SNAP2((int)(4 * sinf(now * 0.004f)));
  v2PetShadow(CX, 262, 40);
  if (master) v2MasterBall(CX - 45, 150 + bob, 5);
  else v2UltraBall(CX - 45, 150 + bob, 5);
  drawCountBadge(CX + 46, 160 + bob, master ? pet.masterBalls : pet.ultraBalls);
  static const int16_t SPARK[6][2] = { { 140, 140 }, { 326, 150 }, { 120, 230 }, { 346, 226 }, { 178, 104 }, { 290, 100 } };
  scScreen();
  scGradeTod = 0;
  uint32_t beat = now / 300;
  for (int i = 0; i < 6; i++) {  // white pixel sparkles, twinkling in turn
    if ((beat + i) % 3 == 0) continue;
    scGridRect(SPARK[i][0], SPARK[i][1] - 8, SPARK[i][0] + 4, SPARK[i][1] + 12, 0xFFFFFF);
    scGridRect(SPARK[i][0] - 8, SPARK[i][1], SPARK[i][0] + 12, SPARK[i][1] + 4, 0xFFFFFF);
  }
  v2SmallPanel(T(master ? S_MASTER_HOW : S_ULTRA_HOW), 296);
}

// ----- the throw -----
// a ball that hits falls from there to the ground where the Pokemon stood, turning upright as it lands with a little
// bounce, and only then the count starts (Fedor): the shake's time runs from its landing
#define V2_CATCH_DROP_MS 700
#define V2_BALL_LOWER 14  // and lands this much below the Pokemon's feet, in front of where it stood, on its shadow
// the hint over the band, higher than V1's so it clears the balls (Fedor); a computer or the web aims with the arrow
// keys too, and says so, on two lines
#define V2_HINT_Y 318
static void v2ThrowHint() {
#ifdef BOARD_HOST
  const char *p = T(S_THROW_HINT_KEYS);
  char l[2][48];
  int n = 0, w = 0;
  for (; *p && n < 2; n++) {
    int skip, k = wrapNext(p, 24, skip);
    snprintf(l[n], sizeof(l[n]), "%.*s", k, p);
    setSize(2);
    if (textW(l[n], 2) > w) w = textW(l[n], 2);
    p += skip;
  }
  int y0 = V2_HINT_Y - 22 * (n - 1);
  v2Panel(CX - (w + 28) / 2, y0 - 7, CX + (w + 28) / 2, y0 + 22 * (n - 1) + 20, 8);
  gfx->setTextColor(UI_INK);
  for (int i = 0; i < n; i++) {
    setCur(centerX(l[i], 2), y0 + 22 * i);
    printT(l[i]);
  }
#else
  battleCenterLine(T(S_THROW_HINT), V2_HINT_Y);
#endif
}
// the band under the running Pokemon, with an Ultra or a Master Ball in hand: the balls you hold in a row, the Master
// Ball, the Ultra Ball, then the Poke Ball with its throws left (Fedor), each the button for its throw: a tap throws
// the one nearest it. Three in the row are drawn at 3x, so they stay inside the round screen
static bool v2MasterAiming() { return pet.masterBalls > 0 || pet.ultraBalls > 0; }
static int v2AimKinds(uint8_t *kind) {
  int n = 0;
  if (pet.masterBalls) kind[n++] = V2_BALL_MASTER;
  if (pet.ultraBalls) kind[n++] = V2_BALL_ULTRA;
  kind[n++] = V2_BALL_POKE;
  return n;
}
static int v2AimX(int i, int n) { return n == 3 ? CX - 117 + i * 90 : CX - 104 + i * 136; }  // ball i's left edge
static void v2MasterAimBalls(uint8_t throws) {
  uint8_t kind[3];
  int n = v2AimKinds(kind), s = n == 3 ? 3 : 4, y = n == 3 ? 378 : 372, w = 18 * s;
  for (int i = 0; i < n; i++) {
    int x = v2AimX(i, n);
    if (kind[i] == V2_BALL_MASTER) v2MasterBall(x, y, s);
    else if (kind[i] == V2_BALL_ULTRA) v2UltraBall(x, y, s);
    else drawMap(SPR_HOME_BALL, HOME_ICON_N, x, y, s, false);
    drawCountBadge(x + w - 4, y + 6, kind[i] == V2_BALL_MASTER ? pet.masterBalls : kind[i] == V2_BALL_ULTRA ? pet.ultraBalls : throws);
  }
}
static uint8_t v2BallAt(int16_t x) {
  if (!v2MasterAiming()) return V2_BALL_POKE;
  uint8_t kind[3];
  int n = v2AimKinds(kind), best = 0, bd = 9999, w = n == 3 ? 54 : 72;
  for (int i = 0; i < n; i++) {
    int d = abs(x - (v2AimX(i, n) + w / 2));
    if (d < bd) {
      bd = d;
      best = i;
    }
  }
  return kind[best];
}
