// V2 step 7: fights pay, the Master Ball and the Ultra Ball (Fedor). scene_v2.h includes this at its end, so it's
// only there with UI_V2 on.
//
// Every wild, Travel or Knomi Tower battle won (Fedor: "You should only get one for winning one"; until 2.1.2 a loss
// counted too; buddy battles and the dungeon don't count) fills two bars a step (pet.fightPays()): ULTRA_FIGHTS of them earn an Ultra Ball, which triples the catch
// chance of a throw that hits, and MASTER_FIGHTS (Fedor: REALLY rare) a Master Ball, which always catches what it
// hits. They stack. The bars show at a won fight's end, both on the card's pages; a fight that earns one
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
// before its moment), the ball it earned not counted yet. k: how much of this fight's step shows, 0 to 1 (its number
// counts it, in green, once it's all in)
static void v2BallBar(bool master, int x, int y, int w, bool full, float k = 1) {
  if (master) v2MasterBall(x, y, 2);
  else v2UltraBall(x, y, 2);
  uint8_t n = master ? MASTER_FIGHTS : ULTRA_FIGHTS, held = master ? pet.masterBalls : pet.ultraBalls;
  uint8_t steps = full ? n : master ? pet.masterBar : pet.ultraBar;
  if (full && held) held--;
  if (held) drawCountBadge(x + 34, y + 4, held);
  bool in = k >= 1 || !steps;
  char b[12];
  snprintf(b, sizeof(b), "%u/%u", in ? steps : steps - 1, n);
  bool cjk = classicFontBegin();
  setSize(2);
  int tw = textW("100/100", 2), bx = x + 50, bw = w - 50 - tw - 10;  // (room for the longest: the bars line up)
  gfx->fillRoundRect(bx, y + 11, bw, 15, 6, C565(0xd8, 0xd2, 0xbd));
  int fw = (int)((bw - 4) * (in ? steps : steps - 1 + (k < 0 ? 0 : k)) / n);
  if (fw > 2) gfx->fillRoundRect(bx + 2, y + 13, fw, 11, 5, master ? v2MasterInk('q') : C565(0xd0, 0xa0, 0x20));
  gfx->setTextColor(in && steps ? C565(0x3c, 0x8a, 0x4c) : V2_INK_MUTED);
  setCur(x + w - textW(b, 2), y + 11);
  printT(b);
  classicFontEnd(cjk);
}

// ----- a fight's end -----
static uint8_t v2MasterNew = 0;     // what this fight earned: 1 a Master Ball, 2 an Ultra Ball (battleFinish())
#define V2_BARS_SHOW_MS 1000        // the end's lines first, then the balls' bars come up in a panel
#define V2_BARS_IN_MS 180           // ...rising in as the scene dims
#define V2_BARS_FILL_MS 450         // ...then this fight's step fills in on each
#define V2_MASTER_SHOW_MS 2400      // a fight that earned a ball: its bar full, then the ball's moment
#define V2_MASTER_END_MS 7400       // ...until the catch or home (a tap from V2_MASTER_SKIP_MS)
#define V2_MASTER_SKIP_MS 3900
// the balls' bars over the dimmed fight, t since they came up: the Ultra Ball's over the Master Ball's in a panel in
// the middle, rising in, this fight's step filling in on both; gone with the fight (Fedor: "have them come up in a
// 'modal' show progress and go away"; there was room at the bottom for one bar, the Ultra Ball's)
static void v2BarsModal(uint32_t t) {
  float in = t >= V2_BARS_IN_MS ? 1 : t / (float)V2_BARS_IN_MS;
  v2DimFrame((int)(115 * in));
  int y0 = 178 + SNAP2((int)(16 * (1 - in)));
  v2Panel(V2_CARD_X0, y0, V2_CARD_X1, y0 + 110, 10);  // (as wide as the card's)
  float k = t < V2_BARS_IN_MS + 150 ? 0 : (t - V2_BARS_IN_MS - 150) / (float)V2_BARS_FILL_MS;
  v2BallBar(false, V2_CARD_L, y0 + 14, V2_CARD_R - V2_CARD_L, v2MasterNew & 2, k);
  v2BallBar(true, V2_CARD_L, y0 + 62, V2_CARD_R - V2_CARD_L, v2MasterNew & 1, k);
}
// t: since the end came up. Past V2_BARS_SHOW_MS the balls' bars; past V2_MASTER_SHOW_MS of a fight that earned a
// ball, the gold burst: its name on the ribbon, the ball big with sparkles, what it does, how many (a Master Ball's,
// when the fight earned both)
static void v2MasterEnd(uint32_t t) {
  if (!v2MasterNew || t < V2_MASTER_SHOW_MS) {
    if (t >= V2_BARS_SHOW_MS) v2BarsModal(t - V2_BARS_SHOW_MS);
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
// the band under the running Pokemon, with an Ultra or a Master Ball in hand: the balls you hold in a row. With one of
// them, it and then the Poke Ball with its throws left (Fedor); with both, the Poke Ball in the middle, the Master Ball
// left of it and the Ultra Ball right (Fedor: "the regular pokeball in the center"). Three in the row are drawn at 3x,
// so they stay inside the round screen
static bool v2MasterAiming() { return pet.masterBalls > 0 || pet.ultraBalls > 0; }
static int v2AimKinds(uint8_t *kind) {
  int n = 0;
  bool both = pet.masterBalls && pet.ultraBalls;
  if (pet.masterBalls) kind[n++] = V2_BALL_MASTER;
  if (both) kind[n++] = V2_BALL_POKE;
  if (pet.ultraBalls) kind[n++] = V2_BALL_ULTRA;
  if (!both) kind[n++] = V2_BALL_POKE;
  return n;
}
static int v2AimX(int i, int n) { return n == 3 ? CX - 117 + i * 90 : CX - 104 + i * 136; }  // ball i's left edge
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

// ----- choosing a ball, then THROW (Fedor: "Maybe clicking one selects it and there is a throw button to throw it"):
// a tap on a ball chooses it, lifted in a gold slot, and THROW with its name, in its colour (Fedor: "I like the color
// coded buttons"), throws it, over the row where the hint was. The hint is up in the sky (Fedor: "The ... text can go
// at the top in the sky area"). With the Poke Ball alone there's nothing to choose: its button is there (Fedor: "If
// there is only one type of ball available, throw poke ball should already be there") and a tap on the ball throws it
// too, as it always has. Each catch starts on the Poke Ball; a missed Ultra or Master Ball comes back and stays chosen,
// and when the last one is used, the Poke Ball is chosen again -----
static uint8_t v2Pick = V2_BALL_POKE;
static uint8_t v2PickNow() {
  if ((v2Pick == V2_BALL_MASTER && !pet.masterBalls) || (v2Pick == V2_BALL_ULTRA && !pet.ultraBalls)) v2Pick = V2_BALL_POKE;
  return v2Pick;
}
#define V2_HINT_Y 318     // where the hint was: a ball broken out of says so there (BP_BROKE)
#define V2_THROW_Y0 296  // THROW: where the hint was, between the running Pokemon and the balls
#define V2_THROW_Y1 342
#define V2_PICK_HINT_Y 72  // the hint's first line, in the sky
static const char *v2ThrowLabel(char *b, int n) {
  uint8_t k = v2PickNow();
  char t[48];
  snprintf(t, sizeof(t), T(S_THROW_FMT), T(k == V2_BALL_MASTER ? S_MASTER : k == V2_BALL_ULTRA ? S_ULTRA : S_POKE_BALL));
  return v2Caps(t, b, n);
}
static void v2ThrowRect(int &x0, int &x1) {  // as wide as its words need, from 256
  char b[48];
  setSize(2);
  int w = textW(v2ThrowLabel(b, sizeof(b)), 2) + 44;
  if (w < 256) w = 256;
  if (w > 330) w = 330;
  x0 = CX - w / 2;
  x1 = CX + w / 2;
}
static bool v2ThrowHit(int x, int y) {
  int x0, x1;
  v2ThrowRect(x0, x1);
  return x >= x0 - 6 && x <= x1 + 6 && y >= V2_THROW_Y0 - 6 && y <= V2_THROW_Y1 + 6;
}
// ball i of n in the row: its top left and its size
static void v2PickBallAt(int i, int n, int &x, int &y, int &w) {
  int s = n == 3 ? 3 : 4;
  w = 18 * s;
  x = n == 1 ? CX - 36 : v2AimX(i, n);
  y = n == 3 ? 378 : 372;
}
static int v2PickKinds(uint8_t *kind) {
  if (v2MasterAiming()) return v2AimKinds(kind);
  kind[0] = V2_BALL_POKE;
  return 1;
}
// the hint in a panel in the sky, wrapped to fit: choose and throw (or only throw), then how to aim
static int v2SpanW(const char *p, int n) {
  char b[96];
  snprintf(b, sizeof(b), "%.*s", n, p);
  return textW(b, 2);
}
static void v2PickHint(bool choose) {
#ifdef BOARD_HOST
  StrId aim = S_AIM_KEYS;  // a computer, the web: the arrow keys aim too
#else
  StrId aim = S_AIM_LEAN;
#endif
  char txt[192], l[4][96];
  snprintf(txt, sizeof(txt), "%s %s", T(choose ? S_PICK_THROW : S_TAP_THROW), T(aim));
  setSize(2);
  int n = 0, w = 0;
  const char *p = txt;
  while (*p && n < 4) {  // as many words as fit in 276 px (the first one whatever its width)
    int end = 0;
    for (;;) {
      int e = end;
      while (p[e] == ' ') e++;
      while (p[e] && p[e] != ' ') e++;
      if (end && v2SpanW(p, e) > 276) break;
      end = e;
      if (!p[e]) break;
    }
    snprintf(l[n], sizeof(l[n]), "%.*s", end, p);
    int tw = textW(l[n], 2);
    if (tw > w) w = tw;
    n++;
    p += end;
    while (*p == ' ') p++;
  }
  int y0 = V2_PICK_HINT_Y;
  v2Panel(CX - (w + 28) / 2, y0 - 7, CX + (w + 28) / 2, y0 + 22 * (n - 1) + 20, 8);
  gfx->setTextColor(UI_INK);
  for (int i = 0; i < n; i++) {
    setCur(centerX(l[i], 2), y0 + 22 * i);
    printT(l[i]);
  }
}
// the catch waiting for a throw: the hint, THROW, the balls with the chosen one in its slot
static void v2PickDraw(uint8_t throws) {
  uint8_t kind[3];
  int n = v2PickKinds(kind);
  uint8_t pick = v2PickNow();
  v2PickHint(n > 1);
  int x0, x1;
  v2ThrowRect(x0, x1);
  {
    V2_HOVER(x0, V2_THROW_Y0, x1 - x0, V2_THROW_Y1 - V2_THROW_Y0);  // under the mouse: lifted (hover_v2.h)
    char b[48];
    v2ThrowLabel(b, sizeof(b));
    uint16_t col = pick == V2_BALL_MASTER ? C565(0x8a, 0x5c, 0xc8) : pick == V2_BALL_ULTRA ? C565(0xf0, 0xc0, 0x4a) : C565(0xe8, 0x50, 0x3a);
    v2LabelButton(x0, V2_THROW_Y0, x1, V2_THROW_Y1, b, col, pick == V2_BALL_ULTRA ? UI_INK : UI_WHITE);
  }
  for (int i = 0; i < n; i++) {
    int x, y, w;
    v2PickBallAt(i, n, x, y, w);
    if (n > 1 && kind[i] == pick) {  // chosen: lifted, in a gold slot (a dark rim, gold, cream)
      y -= 8;
      gfx->fillRoundRect(x - 11, y - 11, w + 22, w + 22, 14, C565(0x3a, 0x4a, 0x5a));
      gfx->fillRoundRect(x - 9, y - 9, w + 18, w + 18, 12, C565(0xf0, 0xb8, 0x30));
      gfx->fillRoundRect(x - 5, y - 5, w + 10, w + 10, 9, C565(0xff, 0xf4, 0xd0));
    }
    V2_HOVER(x - 6, y - 6, w + 12, w + 12);
    if (kind[i] == V2_BALL_MASTER) v2MasterBall(x, y, w / 18);
    else if (kind[i] == V2_BALL_ULTRA) v2UltraBall(x, y, w / 18);
    else drawMap(SPR_HOME_BALL, HOME_ICON_N, x, y, w / 18, false);
    drawCountBadge(x + w - 4, y + 6, kind[i] == V2_BALL_MASTER ? pet.masterBalls : kind[i] == V2_BALL_ULTRA ? pet.ultraBalls : throws);
  }
}
// the throw of a ball, by a tap or a key
static void v2ThrowBall(uint8_t k) {
  v2Throw = k;
  sfxPlay(SFX_TAP);
  tiltZero();
  bBallX = CX;
  bBallT = millis();
  battleSetPhase(BP_THROW);
}
// a tap while it waits: THROW throws the chosen ball; a ball is chosen (the Poke Ball alone: thrown)
static void v2BallTap(int16_t x, int16_t y) {
  if (v2ThrowHit(x, y)) {
    v2ThrowBall(v2PickNow());
    return;
  }
  if (y < V2_THROW_Y1 + 10) return;
  if (!v2MasterAiming()) {
    v2ThrowBall(V2_BALL_POKE);
    return;
  }
  uint8_t k = v2BallAt(x);
  if (k != v2PickNow()) {
    v2Pick = k;
    sfxPlay(SFX_TAP);
  }
}
// the press preview's (press.h): THROW, or the ball under the finger
static bool v2PickPress(int16_t x, int16_t y, PressTarget &t) {
  static char b[48];  // (the target keeps the label's address)
  if (v2ThrowHit(x, y)) {
    int x0, x1;
    v2ThrowRect(x0, x1);
    return v2PressSet(t, x0, V2_THROW_Y0, x1 - x0, V2_THROW_Y1 - V2_THROW_Y0, v2ThrowLabel(b, sizeof(b)));
  }
  if (y < V2_THROW_Y1 + 10) return false;
  uint8_t kind[3];
  int n = v2PickKinds(kind);
  uint8_t k = v2BallAt(x);
  for (int i = 0; i < n; i++)
    if (kind[i] == k) {
      int bx, by, w;
      v2PickBallAt(i, n, bx, by, w);
      if (n > 1 && k == v2PickNow()) by -= 8;
      return v2PressSet(t, bx - 6, by - 6, w + 12, w + 12, T(k == V2_BALL_MASTER ? S_MASTER : k == V2_BALL_ULTRA ? S_ULTRA : S_POKE_BALL));
    }
  return false;
}
// the arrow keys' choice (a computer, the web): the next ball in the row, left or right
static void v2PickStep(int d) {
  uint8_t kind[3];
  int n = v2PickKinds(kind), i = 0;
  uint8_t pick = v2PickNow();
  while (i < n - 1 && kind[i] != pick) i++;
  i += d;
  if (i < 0 || i >= n) return;
  v2Pick = kind[i];
  sfxPlay(SFX_TAP);
}

#ifdef BOARD_HOST
// ----- on a computer, keys on the catch (Fedor: "you should be able to use space, or up arrow to throw the ball and
// arrow keys to steer it"): while it waits, the left and right arrows choose a ball and Space (the Knomi's button,
// knomiButtonTap()) or the up arrow throws the one chosen; in the air the left and right arrows steer it, as they tilt
// (battleThrowStep()). Not in the catch's first 300 ms, and only keys pressed while it's waiting -----
bool hostArrowUp();  // the arrows held (host_runtime.cpp)
bool hostArrowLeft();
bool hostArrowRight();
static bool v2KeyThrow() {
  if (!battleOpen || bPhase != BP_AIM || millis() - bPhaseT < 300) return false;
  v2ThrowBall(v2PickNow());
  return true;
}
static void v2AimKeys() {  // each frame while it waits for a throw
  static uint32_t aim = 0;
  static bool wasUp = true, wasL = true, wasR = true;
  bool up = hostArrowUp(), l = hostArrowLeft(), r = hostArrowRight();
  if (aim != bPhaseT) aim = bPhaseT, wasUp = wasL = wasR = true;  // a new wait: keys held from before do nothing
  if (l && !wasL) v2PickStep(-1);
  if (r && !wasR) v2PickStep(1);
  wasL = l;
  wasR = r;
  if (up && !wasUp) v2KeyThrow();
  wasUp = up;
}
#endif
