// V2: Knomi Tower (Fedor's gym, "tower style: you move up only by winning"). menus_v2.h includes this at its end.
// The rules are tower.h's; the plan and its mockups are in the planning notes (knomi-tower/PLAN.md, mockups/views,
// scripts/pages.py, views.py, tower3.py), and these painters follow them.
//
// Travel gets a second page: the map panned east across the water, where a pier off the island's Cave coast
// becomes a bridge to the tower's islet, Mewtwo's shadow hovering over its top. Tapping the tower opens it: the
// whole tower at sunset, your badges on its floors, CLIMB! at its foot. A climb is your Pokemon and 2 friends from
// the Pokedex, 8 floors and the Legend, on one full energy bar.
#include "tower.h"

// ===================================================================== art helpers (tower3.py, views.py, pages.py)
#define TW_INK 0x1B1B25
#define TW_ST 0xA49AB4    // stone
#define TW_ST_L 0xC8C0D4  // ...lit
#define TW_ST_D 0x7A7090  // ...shaded
#define TW_ST_M 0x8A8098  // ...mortar
#define TW_HALL_LEGEND 15  // the Legend's hall (a type is 0..14)

// each floor's colours (TYPES in tower3.py): [floor] = fill, light, dark; [9] the Legend
static const Rgb TW_COL[TOWER_LEGEND + 1][3] = {
  { 0, 0, 0 },
  { 0xA08A58, 0xC8B080, 0x6E5C38 },  // rock
  { 0x3F8FD8, 0x7CC0F0, 0x2A62A0 },  // water
  { 0xF0C030, 0xFFE27A, 0xB88A10 },  // electric
  { 0x4FA04F, 0x8AD06A, 0x2F6E34 },  // grass
  { 0x9A5AB8, 0xC08AD8, 0x6A3A88 },  // poison
  { 0xE05A90, 0xF59AB8, 0xA83A68 },  // psychic
  { 0xF07030, 0xFFA868, 0xB04A18 },  // fire
  { 0xC08A48, 0xE0B878, 0x8A5A28 },  // ground
  { 0x9A6AD8, 0xD0B0F8, 0x5A3A98 },  // the Legend
};
// 7 x 7 marks, 'x' ink, 'o' light: [floor], [9] the Legend's crown
static const char *const TW_MARK[TOWER_LEGEND + 1][7] = {
  { "", "", "", "", "", "", "" },
  { "..xxx..", ".xooox.", "xoooxxx", "xooxxxx", "xxxxxxx", ".xxxxx.", "..xxx.." },
  { "...x...", "..xox..", ".xooox.", "xoooxxx", "xooxxxx", ".xxxxx.", "..xxx.." },
  { "...xxx.", "..xox..", ".xox...", "xooxxx.", "..xox..", ".xox...", "xx....." },
  { "....xxx", "..xxoox", ".xooxox", "xoxoxox", "xoxxox.", "xxoxx..", "x......" },
  { "..xxx..", ".xooox.", "xoxoxox", "xoooooo", ".xxxxx.", ".x.x.x.", "......." },
  { ".xxxxx.", "xoooooo", "xoxxxox", "xoxoxox", "xoxxxox", "xoooooo", ".xxxxx." },
  { "...x...", "..xox..", ".xox.x.", ".xooxox", "xoooxox", "xooooox", ".xxxxx." },
  { "......x", ".....xx", "..x.xox", ".xoxoox", "xooxoox", "xoooxox", "xxxxxxx" },
  { "x..x..x", "xoxoxox", "xooooox", "xooooox", "xxxxxxx", "x.....x", "xxxxxxx" },
};

// a soft light on what's drawn: each 4 px cell pulled toward col, the outer part on a checker (glow())
static void twGlow(int cx, int cy, int r, Rgb col, int a256) {
  uint16_t c = sc565(col);
  for (int y = scFl4(cy - r) * 4; y < cy + r; y += 4)
    for (int x = scFl4(cx - r) * 4; x < cx + r; x += 4) {
      if (x < 0 || y < 0 || x > V2_W - 4 || y > V2_W - 4) continue;
      float d = hypotf(x + 2 - cx, y + 2 - cy);
      if (d >= r) continue;
      if (d > r * 0.6f && (scFl4(x) + scFl4(y)) % 2) continue;
      int a = (int)(a256 * (1 - d / r));
      for (int j = 0; j < 4; j++) {
        uint16_t *p = scFb + (y + j) * V2_W + x;
        for (int i = 0; i < 4; i++) p[i] = v2Blend(p[i], c, a);
      }
    }
}

// bricks on the grid: 2-cell courses, offset each course; lit on the left, shaded on the right (stone_wall())
static void twStone(int x0, int y0, int x1, int y1, bool dark = false) {
  Rgb st = dark ? 0x6A6080 : TW_ST, lt = dark ? 0x847A98 : TW_ST_L, sh = dark ? 0x4A4258 : TW_ST_D, mo = dark ? 0x584E6C : TW_ST_M;
  for (int cy = y0; cy < y1; cy += 4) {
    int course = (cy - y0) / 8;
    for (int cx = x0; cx < x1; cx += 4) {
      Rgb c = st;
      bool mortar = (cy - y0) % 8 == 0 || ((cx - x0) / 4 + course * 3) % 6 == 0;
      if (mortar) c = mo;
      else if (((cx - x0) / 4 + course * 2) % 7 == 3) c = scMix(st, sh, .35);
      if (cx < x0 + 8) c = mortar ? scMix(lt, mo, .5) : lt;
      if (cx >= x1 - 12) c = mortar ? scMix(sh, mo, .5) : sh;
      scCell(cx, cy, c);
    }
  }
}

// a floor's badge: a crest in its colours with its mark inside, s px a cell (shield())
static void twShield(int cx, int top, int s, uint8_t floor, bool won) {
  static const char *const ROWS[14] = { "..xxxxxxxxx..", ".xLLLLLLLLLx.", "xLLccccccccdx", "xLccccccccccx", "xLccccccccccx",
                                        "xLccccccccccx", "xLccccccccccx", "xLccccccccccx", ".xLcccccccdx.", ".xLcccccccdx.",
                                        "..xLcccccdx..", "...xLcccdx...", "....xxdxx....", ".....xxx....." };
  Rgb col = won ? TW_COL[floor][0] : 0x3A3046, lit = won ? TW_COL[floor][1] : 0x4A4058, dk = won ? TW_COL[floor][2] : 0x2A2236;
  const ScKey k[4] = { { 'x', TW_INK }, { 'L', lit }, { 'c', col }, { 'd', dk } };
  scSprite(ROWS, 14, cx - 13 * s / 2, top, s, k, 4);
  const ScKey m[2] = { { 'x', won ? (Rgb)0xFFFFFF : (Rgb)0x5A5068 }, { 'o', won ? lit : (Rgb)0x4A4058 } };
  scSprite(TW_MARK[floor], 7, cx - 7 * s / 2, top + 3 * s, s, m, 2);
}

// the game's own Ultra and Master Balls (master_v2.h), s px a cell
static void twBall(bool master, int x, int y, int s) {
  const ScKey u[7] = { { 'q', 0x3A3A48 }, { 'Q', 0x262630 }, { 'y', 0xF8D040 }, { 'w', 0xFFFFFF }, { 's', 0xAAB0BC }, { 'S', 0x787E8C }, { 'k', TW_INK } };
  const ScKey mk[7] = { { 'q', 0x7848B8 }, { 'Q', 0x4E2C80 }, { 'm', 0xE85AA8 }, { 'w', 0xFFFFFF }, { 's', 0xAAB0BC }, { 'S', 0x787E8C }, { 'k', TW_INK } };
  scSprite(master ? SPR_V2_MASTER_BALL : SPR_V2_ULTRA_BALL, 18, x, y, s, master ? mk : u, 7);
}

// a small wooden plaque with a label ("3F"), lit when won
static void twPlaque(int x, int y, const char *s, bool won) {
  setSize(2);
  int w = textW(s, 2) + 10;
  gfx->fillRect(x - 2, y - 2, w + 4, 22, sc565(TW_INK));
  gfx->fillRect(x, y, w, 18, sc565(won ? 0x5A3A22 : 0x3A2A1E));
  gfx->fillRect(x, y, w, 2, sc565(won ? 0x8A5A30 : 0x4A3626));
  gfx->setTextColor(sc565(won ? 0xFFF3DC : 0xA08A70));
  setCur(x + 5, y + 3);
  printT(s);
}

// a plank walk: dark rails, planks across, posts down into the water (planks())
#define TW_PIER_Y 288
static void twPlanks(int x0, int x1, int y) {
  for (int cx = scFl4(x0) * 4; cx < x1; cx += 4) {
    for (int cy = y; cy < y + 20; cy += 4) {
      Rgb c = (cy == y || cy == y + 16) ? 0x6A4428 : (scMod(cx / 4, 3) == 0 ? 0xD8A468 : scMod(cx / 4, 3) == 1 ? 0xC8955C : 0xA87648);
      scCell(cx, cy, c);
    }
    if (scMod(cx / 4, 6) == 0) {
      scCell(cx, y + 20, 0x4A2E18);
      scCell(cx, y + 24, 0x3A6A98);
    }
  }
}

// Travel's first page: the pier off the Cave's coast, at the height the bridge has on page 2
// (it starts on the sand at the left of every row it covers, so no sea shows between it and the coast: Fedor, "The dock
// has a bit of a water leak")
static int twPierX0() {
  static int x0 = -1;
  if (x0 < 0) {
    x0 = V2_W;
    for (int y = TW_PIER_Y; y < TW_PIER_Y + 28; y += 4) {
      int x = V2_W - 4;
      while (x > 233 && !v2IsleLand(x + 2, y + 2)) x -= 4;
      if (x < x0) x0 = x;
    }
    x0 = scFl4(x0 - 8) * 4;
  }
  return x0;
}
static bool twOnPier(int x, int y) { return y >= TW_PIER_Y && y < TW_PIER_Y + 28 && x >= twPierX0(); }
static void twPierPaint() { twPlanks(twPierX0(), V2_W, TW_PIER_Y); }

// ===================================================================== Travel, page 2: the tower's islet
// The islet alone, in the middle, its walkway coming in from the left edge at the pier's height: with the pages
// sliding, page 1's pier leaves by the right edge as this walkway comes in by the left, one long walk out to the tower
// (Fedor: "one island with a long walkway to the tower")
#define TW_IX 233                   // the islet's middle
#define TW_IY 256                   // (low enough that Mewtwo's shadow clears the title plate)
#define TW_IR 112
static bool twOnIslet(int x, int y) {
  float a = atan2f(y - TW_IY, x - TW_IX);
  return hypotf((x - TW_IX) * 1.05f, y - TW_IY) < TW_IR + 7 * sinf(5 * a + 2) + 4 * sinf(9 * a);
}
static uint8_t twIsleCell(int i, int j) {  // page 2's cells: the islet (6), or sea
  int x = i * 4 + 2, y = j * 4 + 2;
  if (i < 0 || j < 0 || x >= V2_W || y >= V2_W) return 0;
  return twOnIslet(x, y) ? 0x80 | 6 : 0;
}
// the tower in the map's style: five storeys (lit windows for the floors won), gold gate ledges with pennants,
// torches at the door, battlements on top (grand_tower()); returns the roof's y
static int twMapTower(int cx, int base, uint8_t best) {
  static const uint8_t ST[5][3] = { { 56, 24, 0 }, { 52, 22, 0 }, { 48, 22, 0 }, { 44, 22, 1 }, { 40, 20, 0 } };  // half, h, gold (8F's gate)
  int y = base;
  for (int n = 0; n < 5; n++) {
    int half = ST[n][0], h = ST[n][1], top = y - h;
    twStone(cx - half, top, cx + half, y);
    scRect(cx - half - 4, top, cx - half - 1, y - 1, TW_INK);
    scRect(cx + half, top, cx + half + 3, y - 1, TW_INK);
    if (n == 0) {  // the door: an arch of dark wood, two torches
      static const char *const DOOR[5] = { "..xxxx..", ".xbbbbx.", "xbbwbbbx", "xbbwbbbx", "xbbwbbbx" };
      const ScKey dk[3] = { { 'x', TW_INK }, { 'b', 0x6A4428 }, { 'w', 0x3A2414 } };
      scSprite(DOOR, 5, cx - 16, y - 20, 4, dk, 3);
      static const char *const TORCH[4] = { ".f.", "fyf", ".x.", ".x." };
      const ScKey tk[3] = { { 'f', 0xF07030 }, { 'y', 0xFFE27A }, { 'x', TW_INK } };
      for (int tx : { cx - 30, cx + 26 }) {
        twGlow(tx + 2, y - 16, 16, 0xFFB050, 128);
        scSprite(TORCH, 4, tx - 4, y - 22, 3, tk, 3);
      }
    } else {  // two windows: lit for the floors won (two floors a storey)
      static const char *const WIN[4] = { ".xx.", "xwwx", "xwwx", "xxxx" };
      for (int k = 0; k < 2; k++) {
        int wx = cx + (k ? half / 2 - 4 : -half / 2 - 4);
        bool on = true;
        (void)best;
        if (on) twGlow(wx + 6, top + 10, 14, 0xFFE27A, 115);
        const ScKey wk[2] = { { 'x', TW_INK }, { 'w', on ? (Rgb)0xFFE27A : (Rgb)0x2A2236 } };
        scSprite(WIN, 4, wx - 2, top + 3, 3, wk, 2);
      }
    }
    scRect(cx - half - 8, y, cx + half + 7, y + 3, TW_INK);  // the ledge
    scRect(cx - half - 4, y - 4, cx + half + 3, y - 1, ST[n][2] ? 0xF0C04A : 0xD8D0E4);
    if (ST[n][2]) {  // a gate: a gold pennant at each end of its ledge
      static const char *const PR[3] = { "xx..", "xxxx", "xx.." }, *const PL[3] = { "..xx", "xxxx", "..xx" };
      const ScKey pk[1] = { { 'x', 0xF0C04A } };
      scRect(cx - half - 8, y - 28, cx - half - 5, y - 1, TW_INK);
      scSprite(PL, 3, cx - half - 20, y - 28, 4, pk, 1);
      scRect(cx + half + 4, y - 28, cx + half + 7, y - 1, TW_INK);
      scSprite(PR, 3, cx + half + 8, y - 28, 4, pk, 1);
    }
    y = top - 4;
  }
  int half = 44;  // the roof: a ledge and battlements
  scRect(cx - half - 4, y - 4, cx + half + 3, y + 3, TW_INK);
  scRect(cx - half, y - 4, cx + half - 1, y - 1, TW_ST_L);
  for (int bx = cx - half; bx < cx + half; bx += 16) {
    scRect(bx - 4, y - 16, bx + 7, y - 5, TW_INK);
    scRect(bx, y - 16, bx + 3, y - 5, TW_ST_L);
    scRect(bx + 4, y - 16, bx + 7, y - 5, TW_ST);
  }
  return y;
}
static int twMapRoof = 0;  // where the roof is, for Mewtwo's shadow over it (page 2's life)
static void twPaintIsle2(int best) {
  static const Rgb FILL[7] = { 0x8FCE7A, 0x6C7190, 0xECDCAC, 0x2E2758, 0x3F8A4C, 0x5F9E3E, 0x8A8098 };
  static const Rgb LIT[7] = { 0xA6DC90, 0x8A8FA6, 0xF8ECC4, 0x3E3470, 0x58B868, 0x79BD4F, 0xA49AB4 };
  for (int j = 0; j * 4 < V2_W; j++)
    for (int i = 0; i * 4 < V2_W; i++) {
      int cx = i * 4, cy = j * 4;
      uint8_t c0 = twIsleCell(i, j);
      if (!c0) {
        bool near = twIsleCell(i, j - 1) || twIsleCell(i, j + 1) || twIsleCell(i - 1, j) || twIsleCell(i + 1, j);
        scCell(cx, cy, near && (i + j) % 2 ? V2_SEA_L : V2_SEA);
        continue;
      }
      int z = c0 & 7;
      uint8_t r = twIsleCell(i + 1, j), d = twIsleCell(i, j + 1);
      bool edge = (r && (r & 7) != z) || (d && (d & 7) != z);
      bool coast = !(twIsleCell(i, j - 1) && twIsleCell(i, j + 1) && twIsleCell(i - 1, j) && twIsleCell(i + 1, j));
      Rgb c;
      if (coast) c = z != 2 ? 0xE2CF98 : 0xF8ECC4;
      else if (edge) c = (i + j) % 2 == 0 ? 0x1B1B25 : scMix(FILL[z], 0x1B1B25, .4);
      else if (!twIsleCell(i, j - 2) || (twIsleCell(i, j - 1) & 7) != z) c = LIT[z];
      else c = FILL[z];
      scCell(cx, cy, c);
    }
  // the islet: a grassy top, a cliff on its south face, a ring of boulders
  scOval(scE(TW_IX - 84, TW_IY - 70, TW_IX + 84, TW_IY + 62), 0x5F9E3E, 0x79BD4F);
  for (int cy = 0; cy < V2_W; cy += 4)
    for (int cx = 0; cx < V2_W; cx += 4)
      if (twOnIslet(cx + 2, cy + 2) && !twOnIslet(cx + 2, cy + 14)) scCell(cx, cy, scMod(cx / 4, 3) ? 0x5A5068 : 0x4A4258);
  for (int k = 0; k < 10; k++) {
    float a = k * 0.63f + (k * 37 % 10) * 0.03f, rr = TW_IR - 14;
    int rx = TW_IX + (int)(rr * cosf(a)), ry = TW_IY + (int)(rr * .92f * sinf(a));
    if (abs(ry - (TW_PIER_Y + 10)) < 24 && rx < TW_IX) continue;  // the bridge's landing stays clear
    scOval(scE(rx - 8, ry - 6, rx + 8, ry + 6), 0x6A6478, 0xA49AB4);
  }
  // the bridge (page 1's pier) and a flagstone path to the door
  int x1 = 4;
  while (!twOnIslet(x1, TW_PIER_Y + 10) && x1 < V2_W) x1 += 4;
  twPlanks(0, x1 + 8, TW_PIER_Y);
  int base = TW_IY + 54;
  for (int cx = x1 + 8; cx < TW_IX - 8; cx += 4)
    for (int cy = TW_PIER_Y; cy < TW_PIER_Y + 20; cy += 4) scCell(cx, cy, ((cx / 8 + cy / 8) % 2) ? TW_ST_L : TW_ST);
  for (int cy = base; cy < TW_PIER_Y + 20; cy += 4)
    for (int cx = TW_IX - 16; cx < TW_IX + 16; cx += 4) scCell(cx, cy, ((cx / 8 + cy / 8) % 2) ? TW_ST_L : TW_ST);
  // rocks out in the water, each in foam; gulls
  static const int16_t ROCK[4][3] = { { 396, 330, 10 }, { 92, 372, 8 }, { 392, 150, 7 }, { 84, 160, 6 } };
  for (auto &r : ROCK) {
    scOval(scE(r[0] - r[2] - 6, r[1] - r[2] + 2, r[0] + r[2] + 6, r[1] + r[2] + 6), 0xBFE0F0);
    scOval(scE(r[0] - r[2], r[1] - r[2], r[0] + r[2], r[1] + r[2]), 0x6A6478, 0xA49AB4, 0x4A4458);
  }
  static const char *const GULL[3] = { "x...x", ".x.x.", "..x.." };
  const ScKey gk[1] = { { 'x', 0xFFFFFF } };
  static const int16_t GULLS[4][2] = { { 100, 104 }, { 124, 88 }, { 356, 392 }, { 380, 230 } };
  for (auto &g : GULLS) scSprite(GULL, 3, g[0], g[1], 2, gk, 1);
  twMapRoof = twMapTower(TW_IX, base, (uint8_t)best);
}

// a thumbnail (thumbs.bin) standing with its feet at (cx, feet), s px a pixel; sil: one colour, a shadow
static void twThumb(const uint8_t *b, int cx, int feet, int s, Rgb sil = SC_NONE, bool grey = false) {
  if (!b) return;
  int w = b[0], h = b[1], n = b[2];
  const uint8_t *pal = b + 3, *d = pal + n * 2;
  int l = w, r = -1, bot = -1;
  for (int i = 0; i < w * h; i++)
    if (d[i] != 0xFF) {
      if (i % w < l) l = i % w;
      if (i % w > r) r = i % w;
      bot = i / w;
    }
  if (bot < 0) return;
  int x0 = SNAP2(cx - (l + r + 1) * s / 2), y0 = SNAP2(feet - (bot + 1) * s);
  uint16_t sc = sil != SC_NONE ? sc565(sil) : 0;
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      uint8_t k = d[y * w + x];
      if (k == 0xFF) continue;
      uint16_t c = sil != SC_NONE ? sc : (uint16_t)(pal[k * 2] | (pal[k * 2 + 1] << 8));
      if (grey) c = v2Blend(c, sc565(0xAAB0BC), 170);
      gfx->fillRect(x0 + x * s, y0 + y * s, s, s, c);
    }
}

// Travel's second page (pages.py page2): drawn every frame, Mewtwo's shadow bobbing over the tower
static void v2Travel2() {
  v2Cached(0x3D100000 + pet.towerBest, twPaintIsle2, pet.towerBest);
  scScreen();
  uint32_t now = millis();
  int bob = SNAP2((int)lroundf(-4 + 4 * sinf(now * (6.2831853f / 2400))));  // it hovers: 0 to 8 px up and back
  int feet = twMapRoof - 22 + bob;
  twGlow(TW_IX, feet - 42, 76, 0xC8A0F8, 190);
  static const int8_t SP[4][2] = { { -58, -70 }, { 54, -54 }, { -40, -100 }, { 40, -92 } };
  for (int i = 0; i < 4; i++) {  // sparkles, twinkling in turn
    if ((now / 400 + i) % 4 == 0) continue;
    int gx = TW_IX + SP[i][0], gy = feet + SP[i][1];
    scGridRect(gx, gy - 8, gx + 4, gy + 12, 0xF0E0FF);
    scGridRect(gx - 8, gy, gx + 12, gy + 4, 0xF0E0FF);
  }
  const uint8_t *mt = thumbs.getForm(DEX_MEWTWO, false);
  for (int k = 0; k < 4; k++)  // the same purple rim as on the tower screen
    twThumb(mt, TW_IX + (k == 0 ? -2 : k == 1 ? 2 : 0), feet + (k == 2 ? -2 : k == 3 ? 2 : 0), 3, 0x9A6AD8);
  twThumb(mt, TW_IX, feet, 3, 0x2A1A4C);
  char cnt[24];  // your best climb: the highest floor won, the Legend, or nothing yet
  if (pet.towerBest >= TOWER_LEGEND) snprintf(cnt, sizeof(cnt), "%s", T(S_LEGEND));
  else if (pet.towerBest) snprintf(cnt, sizeof(cnt), T(S_TWR_BEST_FMT), pet.towerBest);
  else snprintf(cnt, sizeof(cnt), "- -");
  v2IsleTag(T(S_TOWER), cnt, TW_IX, TW_IY + 62, 0x9A6AD8);
  v2PageArrow(36, V2_PAGE_ARROW_Y, -1);
  char b[32];
  v2SignIn(v2Caps(T(S_TOWER_TITLE), b, sizeof(b)), 18, C565(0x5a, 0x3a, 0x98), C565(0x8a, 0x6a, 0xd0));
  v2CloseX();
}

// ===================================================================== the climb
enum TwView : uint8_t { TW_TOWER, TW_HOW, TW_TIRED, TW_PICK, TW_TEAM, TW_LEAD, TW_DRAIN, TW_WON, TW_OVER, TW_LEGEND, TW_MASTER, TW_CHAL };
static TwView twView = TW_TOWER;
struct TwMember { int16_t dex; bool shiny, isPet; BtlFighter f; };
static TwMember twTeam[TOWER_TEAM];
static uint8_t twTeamN = 0, twStep = 0, twGot = 0, twFoeRoll = 0;
static int16_t twFoeDex = 0, twLegendFirst = 0;
static BtlFighter twFoe;             // the guardian now: its HP stays from one of yours to the next (Fedor)
static int8_t twDrained = -1;        // the one it just drained (TW_DRAIN), and its HP before and after
static uint16_t twDrainFrom = 0, twDrainTo = 0;
static uint32_t twDrainT0 = 0;
static int8_t twFighting = -1;   // the member in the fight now
static bool twRan = false;
static int16_t twBeatBy = 0;     // who ended the climb, and where
static uint8_t twBeatStep = 0;
static int16_t twLastDex = 0;    // the last of yours it beat (the end screen's tip)
static uint8_t twFaints = 0;     // this climb's faints (a challenge)
static bool twWasMaster = false; // the Legend was beaten before this climb (challenges count only then)
static uint8_t twChalNew = 0;    // the challenges this climb did for the first time (TW_CHAL)
static int16_t twChalX0 = 0, twChalY0 = 0;  // where the tower screen's challenge medals are (a tap opens them)
static PmdMon twPmd[2];          // the friends' sprites, for their fights
static uint8_t twPickPage = 0, twPickN = 0;
static int16_t twPickDex[2];
static bool twPickShiny[2];

PmdMon &towerMePmd(PmdMon &own) {  // the sprite of whoever fights for you (renderBattle)
  if (bTower && twFighting > 0 && twPmd[twFighting - 1].loaded) return twPmd[twFighting - 1];
  return own;
}
uint8_t towerHall() { return twStep >= 1 && twStep <= TOWER_FLOORS ? twStep : TOWER_LEGEND; }  // the hall of the fight now

static uint8_t twLevel() { return btlCapLevel(pet.level()); }

// the forms you can pick as friends: every one in the Pokedex but the one you're raising
static int twFormCount() { return pet.formCount(); }
static int16_t twFormAt(int i, bool &shiny) { return pet.formAt(i, shiny); }
static bool twAllowed(int16_t dex, bool shiny) {
  return !(dex == pet.speciesId && shiny == pet.shiny) && btlMinLevel(dex) <= twLevel();
}
static bool twPicked(int16_t dex, bool shiny) {
  for (int i = 0; i < twPickN; i++)
    if (twPickDex[i] == dex && twPickShiny[i] == shiny) return true;
  return false;
}

// the guardian of the step now (rolled once a step)
static void twRollFoe() {
  twFoeRoll = (uint8_t)random(250);
  twFoeDex = towerGuardianDex(twStep, twLevel(), twFoeRoll);
  if (twStep == TOWER_LEGEND) {  // never one the team can't touch and that can't touch it (Normal and Ghost)
    int16_t team[TOWER_TEAM];
    for (int i = 0; i < twTeamN; i++) team[i] = twTeam[i].dex;
    twFoeDex = towerLegendDex(twLevel(), twFoeRoll, team, twTeamN, twLevel());
  }
  twFoe = towerGuardian(twStep, twFoeDex, twLevel());
  if (twStep == TOWER_LEGEND) twLegendFirst = twFoeDex;
}

static void twStartClimb() {
  uint8_t lvl = twLevel();
  twTeamN = 0;
  twTeam[twTeamN++] = { pet.speciesId, pet.shiny, true, petFighter(lvl) };
  for (int i = 0; i < twPickN && twTeamN < TOWER_TEAM; i++) {
    int16_t d = twPickDex[i];
    bool sh = twPickShiny[i];
    twTeam[twTeamN] = { d, sh, false, towerFriend(d, lvl, pet.starsOf(d, sh), pet.trAtk, pet.trDef, pet.trSpe) };
    twPmd[twTeamN - 1].load(d, sh);
    twTeamN++;
    pet.towerPick[i] = d;
    pet.towerPickShiny[i] = sh;
  }
  if (pet.towerCheer)  // friends cheer after lost climbs: more HP for everyone (tower.h)
    for (int i = 0; i < twTeamN; i++) twTeam[i].f.hp = twTeam[i].f.maxHp = towerCheerHp(twTeam[i].f.maxHp, pet.towerCheer);
  pet.towerSave();
  twStep = 1;
  twRan = false;
  twFaints = 0;
  twLastDex = 0;
  twChalNew = 0;
  twWasMaster = pet.towerBest >= TOWER_LEGEND;
  twRollFoe();
  twView = TW_LEAD;
  Serial.printf("tower: climb with %d, Lv %u\n", twTeamN, lvl);
}

static void twEndClimb() {
  twPmd[0].unload();
  twPmd[1].unload();
  twFighting = -1;
}

void towerOpen() {  // from Travel's second page
  zoneScreen = ZS_TOWER;
  twView = TW_TOWER;
  twEndClimb();
  sfxPlay(SFX_TAP);
}

static bool twPickReady();
static void twClimbTapped() {
  if (pet.sleeping || pet.isEgg() || pet.ceremony) { sfxPlay(SFX_DENY); return; }
  sfxPlay(SFX_TAP);
  if (!pet.towerHowSeen) { twView = TW_HOW; return; }
  if (pet.energy < TOWER_ENERGY_MIN) { twView = TW_TIRED; return; }
  // the friends picked last time, if they can still come
  twPickN = 0;
  for (int i = 0; i < 2; i++)
    if (pet.towerPick[i] && pet.hasForm(pet.towerPick[i], pet.towerPickShiny[i]) && twAllowed(pet.towerPick[i], pet.towerPickShiny[i]) &&
        !twPicked(pet.towerPick[i], pet.towerPickShiny[i])) {
      twPickDex[twPickN] = pet.towerPick[i];
      twPickShiny[twPickN++] = pet.towerPickShiny[i];
    }
  twPickPage = 0;
  twView = twPickReady() ? TW_TEAM : TW_PICK;  // last time's team, or nobody to pick: straight to the team
}

// a Knomi Tower fight: me is the member of your team who fights (its HP as the climb left it), foe the
// floor's guardian. The battle is the usual one, in the floor's hall (here, not in the sketch: V1 keeps its tokens)
static void startTowerFight(const BtlFighter &me, const BtlFighter &foe) {
  if (battleOpen || !pmd.loaded) return;
  wildPmd.load(foe.dex, false);
  if (!wildPmd.loaded) return;
  bTower = true;
  bZone = -1;
  bFromIsland = false;
  bFoeShiny = false;
  bMe = me;
  partyBoostFighter(bMe);
  bFoe = foe;
  bMeMoves = btlMovesFor(bMe.dex, bMe.level, foe.dex);
  bFoeMoves = btlMovesFor(foe.dex, foe.level, bMe.dex);
  bShownMe = bMe.hp;
  bShownFoe = bFoe.hp;
  bWon = false;
  bGain = 0;
  battleOpen = true;
  battleSetPhase(BP_INTRO);
  sfxPlay(SFX_PLAY);
  Serial.printf("tower: #%d Lv.%u vs #%d Lv.%u\n", bMe.dex, bMe.level, foe.dex, foe.level);
}
static void twFight(int i) {
  if (i < 0 || i >= twTeamN || !twTeam[i].f.hp) return;
  twFighting = i;
  startTowerFight(twTeam[i].f, twFoe);  // (as the last fight left it)
  if (!battleOpen) twFighting = -1;
}

// a fight's end (closeBattle): HP back to the team, then the next step, a badge, or the end
void towerFightEnded(bool won, bool ran) {
  zoneScreen = ZS_TOWER;
  if (twFighting < 0) { twView = TW_TOWER; return; }
  TwMember &m = twTeam[twFighting];
  m.f.hp = bMe.hp > m.f.maxHp ? m.f.maxHp : bMe.hp;  // (Party Boost only lifts its stats, not its HP)
  bool petFought = m.isPet;
  twFighting = -1;
  if (ran) {  // RUN always works here, and ends the climb (no loss)
    twBeatBy = twFoeDex, twBeatStep = twStep, twRan = true;
    twEndClimb();
    twView = TW_OVER;
    return;
  }
  if (!won) {  // it beat one of yours: it keeps its HP, and drains that one (Fedor: "a screen where his health goes
               // up and one of my pokemon cards gets crossed out")
    int beaten = &m - twTeam;
    m.f.hp = 0;
    twFaints++;
    twLastDex = m.dex;
    twFoe.hp = bFoe.hp;
    bool any = false;
    for (int i = 0; i < twTeamN; i++) any |= twTeam[i].f.hp > 0;
    if (any) {
      twDrained = beaten;
      twDrainFrom = twFoe.hp;
      twFoe.hp = towerDrain(twFoe.hp, twFoe.maxHp);
      twDrainTo = twFoe.hp;
      twDrainT0 = millis();
      sfxPlay(SFX_DENY);
      twView = TW_DRAIN;
      return;
    }
    twBeatBy = twFoeDex, twBeatStep = twStep;
    pet.towerLost();
    twEndClimb();
    twView = TW_OVER;
    return;
  }
  pet.towerFightWon(petFought);
  if (twStep <= TOWER_FLOORS) {
    twGot = pet.towerFloorWon(twStep);
    for (int i = 0; i < twTeamN; i++) twTeam[i].f.hp = towerHeal(twTeam[i].f.hp, twTeam[i].f.maxHp);
    twView = TW_WON;
    if (twGot) sfxPlay(SFX_MEDAL);
    return;
  }
  if (twStep == TOWER_LEGEND) {  // the Legend's first fight: now Mewtwo (a little HP back first)
    for (int i = 0; i < twTeamN; i++) twTeam[i].f.hp = towerHeal(twTeam[i].f.hp, twTeam[i].f.maxHp);
    twStep++;
    twRollFoe();
    twView = TW_LEAD;
    return;
  }
  // the Legend won: Tower Master
  bool first = pet.towerBest < TOWER_LEGEND;
  twGot = pet.towerFloorWon(TOWER_LEGEND);
  if (first) {
    pet.hofShiny = 0;
    for (int i = 0; i < 3; i++) {
      pet.hofDex[i] = i < twTeamN ? twTeam[i].dex : 0;
      if (i < twTeamN && twTeam[i].shiny) pet.hofShiny |= 1 << i;
    }
    pet.hofLevel = twLevel();
    pet.hofEpoch = pet.lastSeenEpoch;
    pet.towerSave();
  }
  if (twWasMaster) {  // the challenges count only once the Legend was beaten before
    int16_t team[TOWER_TEAM];
    uint8_t stars[TOWER_TEAM];
    for (int i = 0; i < twTeamN; i++) team[i] = twTeam[i].dex, stars[i] = i ? pet.starsOf(twTeam[i].dex, twTeam[i].shiny) : pet.petStars();
    twChalNew = pet.towerChalDone(towerChallenges(twFaints, team, stars, twTeamN));
  }
  sfxPlay(SFX_MEDAL);
  twEndClimb();
  twView = TW_MASTER;
}

// ===================================================================== painters
// the dimmed variants of a scene: everything pulled toward the night tint (rooms2_px.py dim())
static void twDim(int a256) {
  uint16_t t = sc565(0x120E22);
  uint16_t *p = (uint16_t *)gfx->getFramebuffer();
  for (int i = 0; i < V2_W * V2_W; i++) p[i] = v2Blend(p[i], t, a256);
}

// the tower screen (tower3.py tower_screen()): a dusk sky over the sea, the main island far off, the islet's rock,
// and the tower, storey by storey; arg: the best floor won, then (bits 8..) how much to dim it
// a gold plus in front of a reward, so it reads as one to win (thick, outlined: not a sparkle)
static void twPlus(int x, int y) {
  static const char *const P[7] = { "..kkk..", "..kgk..", "kkkgkkk", "kgggggk", "kkkgkkk", "..kgk..", "..kkk.." };
  const ScKey k[2] = { { 'k', TW_INK }, { 'g', 0xF0C04A } };
  scSprite(P, 7, x, y, 3, k, 2);
}
static int twRoofY = 70;  // the tower screen's roof, where Mewtwo's shadow hovers
#define TW_BASE 300  // the tower's foot on its islet
#define TW_HOR 284   // the horizon
static void twPaintTowerScene(int arg) {
  uint8_t best = arg & 15;
  const Rgb SKY[5] = { 0x262A5E, 0x3A3C7C, 0x6A4E94, 0xB06A8E, 0xF0946E };
  const int16_t SE[6] = { 0, 76, 140, 196, 244, 290 };
  scBands(SKY, SE, 5, 0, V2_W);
  for (int k = 0; k < 34; k++) {  // stars
    int x = (k * 137 + 41) % V2_W / 4 * 4, y = (k * 71 + 13) % 130 / 4 * 4;
    scCell(x, y, k % 4 ? 0xFFF3DC : 0xB8C0F0);
  }
  static const int16_t STAR[2][2] = { { 64, 60 }, { 396, 96 } };
  for (auto &st : STAR) {
    if (best >= TOWER_LEGEND && st[0] < 100) continue;  // (the challenge medals are there: it read as a "+")
    scGridRect(st[0], st[1] - 8, st[0] + 4, st[1] + 12, 0xFFF3DC);
    scGridRect(st[0] - 8, st[1], st[0] + 12, st[1] + 4, 0xFFF3DC);
  }
  static const char *const CL[5] = { "....xxxxx.......", "..xxxxxxxxx.xx..", ".xxxxxxxxxxxxxxx", "xxxxxxxxxxxxxxxx", ".llllllllllllll." };
  const ScKey c1[2] = { { 'x', 0x8A5E9A }, { 'l', 0xE08A8A } }, c2[2] = { { 'x', 0x9A6496 }, { 'l', 0xF09A84 } };
  scSprite(CL, 5, 30, 180, 4, c1, 2);
  scSprite(CL, 5, 330, 200, 4, c2, 2);
  // the sea, the main island low on the horizon (a lit window, the Cave's peak), crests catching the sunset
  const Rgb SEA[2] = { 0x3A5A98, 0x2F4C88 };
  const int16_t EE[3] = { TW_HOR, 380, V2_W };
  scBands(SEA, EE, 2, 0, V2_W);
  for (int cy = TW_HOR - 20; cy < TW_HOR; cy += 4)
    for (int cx = 0; cx < 170; cx += 4) {
      float h = 14 + 8 * sinf(cx / 30.0f) + (cx > 110 && cx < 150 ? 6 : 0) + (cx > 120 && cx < 140 ? 10 : 0);
      if (TW_HOR - cy < h) scCell(cx, cy, 0x4A3A70);
    }
  scCell(52, TW_HOR - 12, 0xF8D870);
  for (int k = 0; k < 40; k++) {
    int x = (k * 211 + 7) % V2_W / 4 * 4, y = (TW_HOR + 8 + (k * 53) % (V2_W - TW_HOR - 8)) / 4 * 4;
    Rgb c = y < TW_HOR + 40 ? 0xF0A888 : 0x6F94D0;
    scCell(x, y, c);
    scCell(x + 4, y, c);
  }
  // the islet: foam, rock, grass on top, tufts
  scOval(scE(233 - 158, TW_BASE - 6, 233 + 158, TW_BASE + 54), 0xBFE0F0);
  scOval(scE(233 - 150, TW_BASE - 10, 233 + 150, TW_BASE + 50), 0x6A6478, 0x8A8098, 0x4A4458);
  scOval(scE(233 - 136, TW_BASE - 14, 233 + 136, TW_BASE + 12), 0x4F9A55, 0x79BD4F);
  // the tower: 8 storeys, each on a ledge, tapering upward (tower())
  static const uint8_t HS[8] = { 22, 22, 20, 20, 20, 18, 18, 18 };  // (short enough for Mewtwo's shadow on top)
  int y = TW_BASE, cx = 233;
  for (int n = 1; n <= TOWER_FLOORS; n++) {
    int H = HS[n - 1], half = (84 - (n - 1) * 5) / 4 * 4, x0 = cx - half, x1 = cx + half, top = y - H;
    bool won = false;  // (a climb's badges are its own: Fedor, "contained to each of the tower runs")
    bool lit = true;   // the floor guide: every window lit in its type's light, with its mark
    twStone(x0, top, x1, y);
    scRect(x0 - 4, top, x0 - 1, y - 1, TW_INK);
    scRect(x1, top, x1 + 3, y - 1, TW_INK);
    bool gate = towerIsGate(n);
    for (int x = x0 - 8; x < x1 + 8; x += 4) {  // the ledge it stands on (gold on a gate)
      bool end = x == x0 - 8 || x == x1 + 4;
      scCell(x, y, end ? TW_INK : gate ? 0xC08410 : 0x5A5068);
      scCell(x, y - 4, end ? TW_INK : gate ? (scMod(x / 4, 2) ? 0xF0C04A : 0xFFE27A) : 0xD8D0E4);
    }
    if (n == 1) {  // the door
      static const char *const DOOR[7] = { "..xxxx..", ".xwwwwx.", "xwwbwwwx", "xwwbwwwx", "xbbbbbbx", "xwwbwwwx", "xwwbwwwx" };
      const ScKey dk[3] = { { 'x', TW_INK }, { 'w', 0x6A4428 }, { 'b', 0x3A2414 } };
      scSprite(DOOR, 7, cx - 12, top - 4, 3, dk, 3);
    } else {  // an arched window: lit in its type's light with its mark, or dark
      int s = H >= 21 ? 3 : 2, ms = s == 3 ? 2 : 1, wy = top + (H - 7 * s) / 2;
      static const char *const WIN[7] = { "..xxxx..", ".xggggx.", "xggggggx", "xggggggx", "xggggggx", "xggggggx", "xxxxxxxx" };
      if (lit) twGlow(cx, wy + 3 * s, 8 * s + 2, TW_COL[n][1], 115);
      const ScKey wk[2] = { { 'x', TW_INK }, { 'g', lit ? scMix(TW_COL[n][1], 0xFFFFFF, .35) : (Rgb)0x2A2236 } };
      scSprite(WIN, 7, cx - 4 * s, wy, s, wk, 2);
      const ScKey mk[2] = { { 'x', lit ? TW_COL[n][0] : (Rgb)0x4A4058 }, { 'o', lit ? TW_COL[n][2] : (Rgb)0x3A3046 } };
      scSprite(TW_MARK[n], 7, cx - 7 * ms / 2, wy + 2 * s, ms, mk, 2);
    }
    char lab[6];
    snprintf(lab, sizeof(lab), "%dF", n);
    twPlaque(x0 + 6, top + (H - 18) / 2 + 1, lab, won);
    {  // the floor guide: its type's name out to the left, in its light (Fedor: plan the friends for the floors)
      const char *ty = TYPE_NAME[TOWER_TYPE[n]];
      int tx = x0 - 12 - textW(ty, 2), ty0 = top + (H - 16) / 2 + 1;
      setSize(2);
      gfx->setTextColor(sc565(TW_INK));
      for (int k = 0; k < 4; k++) {
        setCur(tx + (k == 0 ? -2 : k == 1 ? 2 : 0), ty0 + (k == 2 ? -2 : k == 3 ? 2 : 0));
        printT(ty);
      }
      gfx->setTextColor(sc565(TW_COL[n][1]));
      setCur(tx, ty0);
      printT(ty);
    }
    if (won) {  // its badge, hung beside the window
      static const char *const BD[9] = { "..xxxxx..", ".xLcccLx.", "xLcccccdx", "xccMMMccx", "xccMMMcdx", ".xcccccx.", "..xcccx..", "...xdx...", "....x...." };
      const ScKey bk[5] = { { 'x', TW_INK }, { 'L', TW_COL[n][1] }, { 'c', TW_COL[n][0] }, { 'd', TW_COL[n][2] }, { 'M', 0xFFFFFF } };
      scSprite(BD, 9, x1 - 30, top + (H - 18) / 2, 2, bk, 5);
    }
    y = top - 4;
  }
  // the Legend: a terrace with battlements at the top, Mewtwo's shadow hovering over it (drawn each frame by the view);
  // "LEGEND" on a plaque to its left, the Master Ball to its right until it's won (Fedor: "the shadow of mewtwo moving
  // around slightly and move the master ball off to the side like the ultra ball")
  int half = 48, x0 = cx - half, x1 = cx + half;
  twStone(x0, y - 12, x1, y);
  scRect(x0 - 4, y - 12, x0 - 1, y - 1, TW_INK);
  scRect(x1, y - 12, x1 + 3, y - 1, TW_INK);
  for (int bx = x0; bx < x1; bx += 16) {  // battlements
    scRect(bx - 4, y - 24, bx + 7, y - 13, TW_INK);
    scRect(bx, y - 24, bx + 3, y - 13, TW_ST_L);
    scRect(bx + 4, y - 24, bx + 7, y - 13, TW_ST);
  }
  twRoofY = y - 24;
  twPlus(x1 + 10, y - 50);  // the Master Ball it pays, every time it's beaten
  twBall(true, x1 + 32, y - 58, 2);
  if (best >= TOWER_LEGEND) {  // the Legend beaten: its challenges' medals on the other side, gold once done (a tap shows them)
    twChalX0 = x0 - 104, twChalY0 = y - 58;
    for (int k = 0; k < 3; k++) {
      int mx = twChalX0 + 14 + k * 30, my = twChalY0 + 16;
      bool done = (arg >> 4 & 7) >> k & 1;
      gfx->fillCircle(mx, my, 13, sc565(TW_INK));
      gfx->fillCircle(mx, my, 11, done ? sc565(0xF0C04A) : sc565(0x4A4058));
      gfx->fillCircle(mx, my, 6, done ? sc565(0xFFE27A) : sc565(0x3A3046));
    }
  }
  setSize(2);  // "LEGEND" on a plaque across the battlements, under Mewtwo
  const char *lg = T(S_LEGEND);
  int w = textW(lg, 2) + 16, lx = cx - w / 2, ly = y - 22;
  twRoofY = ly - 3;
  gfx->fillRect(lx - 3, ly - 3, w + 6, 27, sc565(TW_INK));
  gfx->fillRect(lx, ly, w, 21, sc565(0x5A3A98));
  gfx->fillRect(lx, ly, w, 3, sc565(0x9A6AD8));
  gfx->setTextColor(sc565(0xFFE27A));
  setCur(lx + 8, ly + 5);
  printT(lg);
  if (arg >> 8) twDim(arg >> 8);
}

// a floor's hall (views.py hall()): stone walls, pillars, its banners, torches, flags and a red runner; arg: the
// floor (9: the Legend's) and (bits 8..) how much to dim it
static void twPaintHall(int arg) {
  uint8_t fl = arg & 15;
  Rgb col = TW_COL[fl][0], lit = TW_COL[fl][1], dk = TW_COL[fl][2];
  twStone(0, 0, V2_W, 252, true);
  for (int px : { 64, 402 }) {  // two pillars, lit from the left
    static const Rgb PC[9] = { TW_INK, TW_ST_L, TW_ST_L, TW_ST, TW_ST, TW_ST, TW_ST_D, TW_ST_D, TW_INK };
    for (int cy = 20; cy < 252; cy += 4)
      for (int k = 0; k < 9; k++) scCell(px - 16 + k * 4, cy, PC[k]);
    scRect(px - 24, 16, px + 23, 27, TW_INK);
    scRect(px - 20, 16, px + 19, 23, TW_ST_L);
  }
  static const char *const TAIL[4] = { "x.....x", "xx...xx", "xxx.xxx", "xxxxxxx" };
  for (int bx : { 150, 316 }) {  // banners in its colours, its mark on each
    scRect(bx - 28, 36, bx + 27, 175, TW_INK);
    scRect(bx - 24, 36, bx + 23, 167, col);
    scRect(bx - 24, 36, bx - 17, 167, lit);
    scRect(bx + 16, 36, bx + 23, 167, dk);
    scRect(bx - 30, 30, bx + 29, 37, 0xC08410);
    const ScKey tk[1] = { { 'x', col } };
    scSprite(TAIL, 4, bx - 14, 168, 4, tk, 1);
    const ScKey mk[2] = { { 'x', 0xFFFFFF }, { 'o', lit } };
    scSprite(TW_MARK[fl], 7, bx - 14, 84, 4, mk, 2);
  }
  static const char *const TORCH[8] = { "..f..", ".fyf.", "fyyyf", ".fyf.", "xxxxx", ".xxx.", "..x..", "..x.." };
  const ScKey fk[3] = { { 'f', 0xF07030 }, { 'y', 0xFFE27A }, { 'x', TW_INK } };
  for (int tx : { 233 - 36, 233 + 28 }) {
    twGlow(tx + 4, 92, 52, 0xFFB050, 115);
    scSprite(TORCH, 8, tx - 6, 80, 3, fk, 3);
  }
  for (int ty = 252; ty < V2_W; ty += 24)  // flagstones in its dark tones
    for (int tx = 0; tx < V2_W; tx += 24) {
      Rgb c = scMix(col, 0x2A2236, (tx / 24 + ty / 24) % 2 ? .45 : .62);
      scRect(tx, ty, tx + 23, ty + 23, c);
      scRect(tx, ty, tx + 23, ty + 3, scMix(c, 0xFFFFFF, .12));
    }
  scRect(0, 248, V2_W - 1, 255, TW_INK);
  for (int cy = 256; cy < V2_W; cy += 4) {  // a runner toward you
    float half = 40 + (cy - 256) * .35f;
    for (int cx = scFl4((int)(233 - half)) * 4; cx < 233 + half; cx += 4) {
      bool edge = cx < 233 - half + 8 || cx > 233 + half - 8;
      scCell(cx, cy, edge ? 0xC08410 : scMod(cy / 4, 2) ? 0xA83A3A : 0x983232);
    }
  }
  if (arg >> 8) twDim(arg >> 8);
}

// the Legend's terrace at night (views.py legend_scene()): two great pillars, a gold lintel, the Master Ball glowing
static void twPaintLegend(int arg) {
  const Rgb SKY[3] = { 0x141432, 0x22224A, 0x3A2A68 };
  const int16_t SE[4] = { 0, 120, 230, 300 };
  scBands(SKY, SE, 3, 0, V2_W);
  for (int k = 0; k < 60; k++) scCell((k * 151 + 29) % V2_W / 4 * 4, (k * 83 + 7) % 260 / 4 * 4, k % 3 ? 0xFFF3DC : 0xB8C0F0);
  twGlow(233, 150, 120, 0xD0B0F8, 140);
  static const Rgb PC[9] = { TW_INK, TW_ST_L, TW_ST_L, TW_ST, TW_ST, TW_ST, TW_ST_D, TW_ST_D, TW_INK };
  for (int px : { 96, 346 })
    for (int cy = 64; cy < 300; cy += 4)
      for (int k = 0; k < 9; k++) scCell(px - 16 + k * 4, cy, PC[k]);
  scRect(60, 52, 405, 67, TW_INK);
  scRect(64, 52, 401, 63, TW_ST_L);
  for (int x = 64; x < 402; x += 4) scCell(x, 48, scMod(x / 4, 2) ? 0xF0C04A : 0xC08410);
  scRect(0, 296, V2_W - 1, 303, TW_INK);
  for (int ty = 304; ty < V2_W; ty += 24)
    for (int tx = 0; tx < V2_W; tx += 24) {
      Rgb c = (tx / 24 + ty / 24) % 2 ? 0x3A2A68 : 0x2E2258;
      scRect(tx, ty, tx + 23, ty + 23, c);
      scRect(tx, ty, tx + 23, ty + 3, scMix(c, 0xFFFFFF, .1));
    }
  scRect(205, 166, 260, 181, 0x5A3A98);
  scRect(205, 166, 260, 169, 0x9A6AD8);
  twBall(true, 215, 128, 2);
  if (arg >> 8) twDim(arg >> 8);
}

// a stone dais under a Pokemon (dais2())
static void twDais(int cx, int y, int w, Rgb col) {
  scOval(scE(cx - w / 2, y + 6, cx + w / 2, y + 30), TW_INK);
  scOval(scE(cx - w / 2 + 4, y, cx + w / 2 - 4, y + 22), scMix(col, 0x5A5068, .5), scMix(col, 0xFFFFFF, .3), scMix(col, TW_INK, .5));
}
// a small rounded chip with white text (chip())
static void twChip(const char *s, int cx, int y, uint16_t col);
// the friends' cheer, while there is one (after lost climbs): a gold chip, so waiting for the energy is worth it
static void twRimText(const char *t, int cx, int y, uint16_t col);
static void twCheerChip(int cy, int cx = CX) {
  if (!pet.towerCheer || pet.towerBest >= TOWER_LEGEND) return;
  char b[64];
  snprintf(b, sizeof(b), T(S_CHEER_FMT), TOWER_CHEER_PCT * pet.towerCheer);
  twChip(b, cx, cy, C565(0xc0, 0x84, 0x10));
}
static void twChip(const char *s, int cx, int y, uint16_t col = V2_RIM) {
  setSize(2);
  int w = textW(s, 2) + 16;
  gfx->fillRoundRect(cx - w / 2 - 3, y - 3, w + 6, 26, 8, sc565(TW_INK));
  gfx->fillRoundRect(cx - w / 2, y, w, 20, 6, col);
  gfx->setTextColor(UI_WHITE);
  setCur(cx - textW(s, 2) / 2, y + 3);
  printT(s);
}
// a floor's title for its board: "4F GRASS", or the Legend's
static void twFloorTitle(uint8_t step, char *b, int n) {
  if (step > TOWER_FLOORS) snprintf(b, n, "%s %d/2", T(S_LEGEND), step - TOWER_FLOORS);
  else snprintf(b, n, "%dF %s", step, TYPE_NAME[TOWER_TYPE[step]]);
}

// ===================================================================== the views
#define TW_DIM(a) ((a) << 8)

static void twBackTower(int dim) {
  int a = pet.towerBest | (pet.towerChal & TOWER_CH_ALL) << 4 | TW_DIM(dim);
  v2Cached(0x3E000000 + a, twPaintTowerScene, a);
}
static void twBackHall(uint8_t fl, int dim) { v2Cached(0x3E100000 + (fl | TW_DIM(dim)), twPaintHall, fl | TW_DIM(dim)); }
static void twBackLegend(int dim) { v2Cached(0x3E200000 + TW_DIM(dim), twPaintLegend, TW_DIM(dim)); }

static void twSignFloor(uint8_t step) {
  char b[32];
  twFloorTitle(step, b, sizeof(b));
  uint8_t fl = towerFloorOf(step);
  v2SignIn(b, 30, sc565(TW_COL[fl][2]), sc565(TW_COL[fl][0]));
}

// 02: the tower, CLIMB! under it: as wide as the zones' SEARCH and 52 px tall, well clear of the X (Fedor: "too close to
// the X and too small ... too hard to press on the Knomi"). A tap on the tower climbs too
#define TW_CLIMB_X0 101
#define TW_CLIMB_Y0 318
#define TW_CLIMB_X1 365
#define TW_CLIMB_Y1 370
static void twViewTower() {
  twBackTower(0);
  scScreen();
  uint32_t now = millis();  // Mewtwo's shadow hovers over the roof, bobbing and drifting a little
  int bob = SNAP2((int)lroundf(-3 + 3 * sinf(now * (6.2831853f / 2400)))), sway = SNAP2((int)lroundf(3 * sinf(now * (6.2831853f / 5200))));
  int feet = twRoofY - 4 + bob;
  twGlow(CX + sway, feet - 26, 50, 0xC8A0F8, 200);
  const uint8_t *mt = thumbs.getForm(DEX_MEWTWO, false);
  for (int k = 0; k < 4; k++)  // a soft purple rim round the shadow, so it reads against the night
    twThumb(mt, CX + sway + (k == 0 ? -2 : k == 1 ? 2 : 0), feet + (k == 2 ? -2 : k == 3 ? 2 : 0), 2, 0x9A6AD8);
  twThumb(mt, CX + sway, feet, 2, 0x2A1A4C);
  v2LabelButton(TW_CLIMB_X0, TW_CLIMB_Y0, TW_CLIMB_X1, TW_CLIMB_Y1, T(S_CLIMB), C565(0x58, 0xb8, 0x68));
  if (pet.towerCheer && pet.towerBest < TOWER_LEGEND) {  // the friends' cheer: a gold disc by the tower's foot, a heart, "+10%", "HP"
    const int dx = 384, dy = 246, r = 38;
    char b[12];
    gfx->fillCircle(dx, dy, r + 3, sc565(TW_INK));
    gfx->fillCircle(dx, dy, r, sc565(0xF0C04A));
    gfx->fillCircle(dx, dy, r - 5, sc565(0xC08410));
    static const char *const HEART[6] = { ".xx.xx.", "xxxxxxx", "xxxxxxx", ".xxxxx.", "..xxx..", "...x..." };
    for (int y = 0; y < 6; y++)
      for (int x = 0; x < 7; x++)
        if (HEART[y][x] == 'x') gfx->fillRect(dx - 7 + x * 2, dy - 30 + y * 2, 2, 2, sc565(0xFF6A7A));
    snprintf(b, sizeof(b), "+%d%%", TOWER_CHEER_PCT * pet.towerCheer);
    twRimText(b, dx, dy - 14, UI_WHITE);
    twRimText("HP", dx, dy + 6, sc565(0xFFE27A));
  }
  v2CloseX();
}
// 03: the first visit's card
static void twViewHow() {
  twBackTower(115);
  char b[32];
  v2SignIn(v2Caps(T(S_TOWER_TITLE), b, sizeof(b)), 60, C565(0x5a, 0x3a, 0x98), C565(0x8a, 0x6a, 0xd0));
  v2Panel(80, 100, 386, 338, 10);
  gfx->setTextColor(UI_INK);
  v2Wrapped(T(S_TWR_HOW), 122, 280, 22, 6);
  v2LabelButton(116, 262, 350, 314, T(S_LETS_CLIMB), C565(0x58, 0xb8, 0x68));
  v2CloseX();
}
// 04: not enough energy
static void twViewTired() {
  twBackTower(128);
  v2Panel(92, 132, 374, 318, 10);
  drawPmdAct(pmd.has(PMD_SLEEP) ? PMD_SLEEP : PMD_IDLE, CX, 214, millis(), true, false, 3);
  gfx->setTextColor(UI_INK);
  v2Wrapped(T(S_TWR_TIRED), 226, 250, 20, 2);
  gfx->fillRoundRect(132, 276, 202, 16, 5, V2_RIM);
  gfx->fillRect(135, 279, (int)(196L * pet.energy / 100), 10, C565(0xf0, 0xc0, 0x4a));
  gfx->fillRect(135 + 196 * TOWER_ENERGY_MIN / 100, 277, 2, 14, UI_WHITE);  // where a climb can start
  twCheerChip(306);  // what's waiting: the friends' cheer
  v2CloseX();
}
// 05: pick 2 friends: the Pokedex's own grid (Fedor: "no need to boxes behind the pokemon. Keep the pokedex looking
// like what we currently have built"): the dimmed room, the board with how many are picked, the sprites in its light
// panel, page arrows and "Page 1 of 3"; a pick gets a green numbered disc, a form your level doesn't allow yet is a
// shadow with the level it needs. The second pick goes on to the team page (05b), which has GO!
static int twPickList(int i, bool &sh) {  // the i-th pickable form: every one in the Pokedex but your own
  int n = twFormCount(), k = 0;
  for (int j = 0; j < n; j++) {
    int16_t d = twFormAt(j, sh);
    if (d == pet.speciesId && sh == pet.shiny) continue;
    if (k++ == i) return d;
  }
  return 0;
}
static int twPickCount() { bool sh; int n = 0; while (twPickList(n, sh)) n++; return n; }
static int twPickPages() { int n = twPickCount(); return n ? (n + DEXG_PAGE - 1) / DEXG_PAGE : 1; }
static bool twPickReady() { int n = twPickCount(); return twPickN == 2 || (twPickN == n && n < 2); }
static void twViewPick() {
  v2MenuBackdrop();
  char cnt[12], b[16];
  snprintf(cnt, sizeof(cnt), "%d/2", twPickN);
  v2GridHead(T(S_PICK_FRIENDS), cnt, DEXG_X, DEXG_Y, DEXG_CELL, 3);
  for (int i = 0; i < DEXG_PAGE; i++) {
    bool sh;
    int16_t d = twPickList(twPickPage * DEXG_PAGE + i, sh);
    if (!d) break;
    int x = DEXG_X + (i % DEXG_COLS) * DEXG_CELL, y = DEXG_Y + (i / DEXG_COLS) * DEXG_CELL, cx = x + DEXG_CELL / 2, cy = y + DEXG_CELL / 2;
    V2_HOVER(x, y, DEXG_CELL, DEXG_CELL);  // under the mouse: it lifts (hover_v2.h)
    bool ok = twAllowed(d, sh);
    if (const uint8_t *t = thumbs.getForm(d, sh)) {
      int box[4];
      drawThumbFit(t, cx, cy, DEXG_CELL - 6, DEXG_CELL - 4, DEXG_SPRITE_S, !ok, DEXG_STARS_H / 2, box);
      if (sh) drawSparkles(box[0], box[1], box[2], box[3], millis() + i * 337);
    }
    if (ok && pet.starsOf(d, sh) >= 3) drawStars(cx, y + DEXG_CELL - DEXG_STAR_R - 3, DEXG_STAR_R, 3);
    if (!ok) {
      snprintf(b, sizeof(b), "Lv%d", btlMinLevel(d));
      twChip(b, cx, y + DEXG_CELL - 26);
    }
    if (twPicked(d, sh)) {
      int k = twPickDex[0] == d && twPickShiny[0] == sh ? 1 : 2;
      gfx->fillCircle(x + DEXG_CELL - 16, y + 16, 13, sc565(TW_INK));
      gfx->fillCircle(x + DEXG_CELL - 16, y + 16, 11, UI_BAR_OK);
      snprintf(b, sizeof(b), "%d", k);
      gfx->setTextColor(UI_WHITE);
      setSize(2);
      setCur(x + DEXG_CELL - 16 - textW(b, 2) / 2, y + 9);
      printT(b);
    }
  }
  drawDexGridPage(twPickPage, twPickPages(), DEXG_PAGE_Y);
  drawPageArrows(twPickPage, twPickPages(), V2_PAGE_ARROW_Y);
  v2CloseX();
}

// 05b: your team (Fedor: "Maybe the go button is in a summary page"): your Pokemon and the two friends on their
// daises, a tap on a friend changes them (back to the grid), GO! where the tower's CLIMB! is: wide, 52 px, clear of the X
static int twTeamSlotX(int i, int n) { return n == 3 ? 100 + i * 133 : n == 2 ? 166 + i * 134 : 233; }
static void twViewTeam() {
  twBackHall(TOWER_LEGEND, 115);
  scScreen();
  char b[64];
  v2SignIn(v2Caps(T(S_YOUR_TEAM), b, sizeof(b)), 30, C565(0x5a, 0x3a, 0x98), C565(0x8a, 0x6a, 0xd0));
  int n = 1 + twPickN;
  for (int i = 0; i < n; i++) {
    int16_t d = i ? twPickDex[i - 1] : pet.speciesId;
    bool sh = i ? twPickShiny[i - 1] : pet.shiny;
    int cx = twTeamSlotX(i, n);
    V2_HOVER_DIFF(cx - 60, 100, i ? 120 : 0, 168);  // a friend under the mouse lifts, dais and all (a tap changes it)
    twDais(cx, 180, 112, i ? (Rgb)0x9A6AD8 : (Rgb)0x58B868);
    twThumb(thumbs.getForm(d, sh), cx, 194, 3);
    char nb[16];
    snprintf(nb, sizeof(nb), "%.9s", i ? dexName(d) : (pet.nick[0] ? pet.nick : dexName(d)));
    twChip(nb, cx, 216, i ? sc565(0x5A3A98) : sc565(0x3C8A4C));
    uint8_t st = i ? pet.starsOf(d, sh) : pet.petStars();
    if (st) drawStars(cx, 254, DEXG_STAR_R, st);
  }
  if (twPickN) {
    gfx->setTextColor(V2_ON_DARK);
    v2Wrapped(T(S_TAP_CHANGE), 274, 300, 20, 2);
  }
  if (pet.towerCheer) {  // friends cheer after lost climbs: more HP this time
    snprintf(b, sizeof(b), T(S_CHEER_FMT), TOWER_CHEER_PCT * pet.towerCheer);
    twChip(b, CX, 74, C565(0xc0, 0x84, 0x10));
  }
  v2LabelButton(TW_CLIMB_X0, TW_CLIMB_Y0, TW_CLIMB_X1, TW_CLIMB_Y1, T(S_GO), C565(0x58, 0xb8, 0x68));
  v2CloseX();
}
// 06: who goes first (and after a faint, who's next)
// the guardian on its dais, its level, and its HP bar (it carries over now): hp shown as given
static void twFoeStand(uint8_t fl, uint16_t hp) {
  twDais(233, 150, 120, TW_COL[fl][0]);
  twThumb(thumbs.getForm(twFoeDex, false), 233, 164, 3);
  char b[32];
  snprintf(b, sizeof(b), "Lv %d", towerLevel(twStep, twLevel()));
  twChip(b, 233, 176);
  int pct = twFoe.maxHp ? hp * 100 / twFoe.maxHp : 0;
  gfx->fillRoundRect(173, 201, 120, 10, 4, sc565(TW_INK));
  gfx->fillRoundRect(175, 203, 116, 6, 3, sc565(0xD8D2BD));
  gfx->fillRect(175, 203, 116 * pct / 100, 6, pct >= 50 ? UI_BAR_OK : pct >= 25 ? C565(0xf0, 0xc0, 0x4a) : UI_BAR_BAD);
}
void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS);
#define TW_LEAD_Y0 236  // the lead screen's team: where each one can be picked
#define TW_LEAD_Y1 372
// text with a dark rim, straight on the hall (no box behind it)
static void twRimText(const char *t, int cx, int y, uint16_t col) {
  setSize(2);
  int x = cx - textW(t, 2) / 2;
  gfx->setTextColor(sc565(TW_INK));
  for (int k = 0; k < 8; k++) {
    static const int8_t D[8][2] = { { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 }, { -2, -2 }, { 2, -2 }, { -2, 2 }, { 2, 2 } };
    setCur(x + D[k][0], y + D[k][1]);
    printT(t);
  }
  gfx->setTextColor(col);
  setCur(x, y);
  printT(t);
}
// your team, each on a dais in the hall (Fedor: "a version of this screen without white boxes"): its name, HP and
// the hint, Strong on a green dais with a glow; a fainted one asleep there, "Fainted" under it
static void twCards(int crossed) {
  int n = twTeamN, w = 116, gap = 8, x0 = CX - (n * w + (n - 1) * gap) / 2;
  for (int i = 0; i < n; i++) {
    TwMember &m = twTeam[i];
    int cx0 = x0 + i * (w + gap), cx1 = cx0 + w, mx = (cx0 + cx1) / 2;
    (void)cx1;
    V2_HOVER_DIFF(cx0, TW_LEAD_Y0, w, TW_LEAD_Y1 - TW_LEAD_Y0);  // under the mouse: its whole dais lifts, like a button
    bool out = !m.f.hp;
    int8_t hint = towerMatchup(m.f, twFoe);  // (the fight itself, as HP stands now)
    if (hint > 0 && !out) twGlow(mx, 292, 54, 0x8AD06A, 150);
    twDais(mx, 290, 104, out ? (Rgb)0x9A6AD8 : hint > 0 ? (Rgb)0x4FA04F : hint < 0 ? (Rgb)0xA84848 : (Rgb)0x9A6AD8);
    PmdMon &pm = m.isPet ? pmd : twPmd[i > 0 ? i - 1 : 0];
    if (out && pm.has(PMD_SLEEP)) {  // fainted: asleep on its dais (Fedor: "instead of the x and gray"), a Zz over it
      uint32_t now = millis();
      drawPmdActM(pm, PMD_SLEEP, mx, 304, now, true, false, 2);
      for (int z = 0; z < 2; z++) {
        int ph = (int)((now / 40 + z * 25) % 50);
        twRimText(z ? "z" : "Z", mx + 30 + z * 10 + ph / 10, 270 - z * 10 - ph / 5, sc565(0xD8D0F8));
      }
    } else {
      twThumb(thumbs.getForm(m.dex, m.shiny), mx, 302, 2);
    }
    char nb[16];
    snprintf(nb, sizeof(nb), "%.9s", dexName(m.dex));
    twRimText(nb, mx, 318, UI_WHITE);
    gfx->fillRoundRect(cx0 + 10, 339, w - 20, 10, 4, sc565(TW_INK));
    gfx->fillRoundRect(cx0 + 12, 341, w - 24, 6, 3, sc565(0x4A4458));
    if (!out) {
      int pct = m.f.hp * 100 / m.f.maxHp;
      gfx->fillRect(cx0 + 12, 341, (w - 24) * pct / 100, 6, pct >= 50 ? UI_BAR_OK : pct >= 25 ? C565(0xf0, 0xc0, 0x4a) : UI_BAR_BAD);
    }
    const char *h = out ? T(S_FAINTED) : hint > 0 ? T(S_STRONG) : hint < 0 ? T(S_WEAK) : "";
    uint16_t hc = out ? sc565(0x9A96A8) : hint > 0 ? sc565(0x9AE07A) : sc565(0xFF8A7A);
    if (*h) {
      int tw2 = textW(h, 2) + (hint && !out ? 16 : 0), tx = mx - tw2 / 2;
      if (hint && !out) {
        for (int r = 0; r < 3; r++) {  // a little triangle: up for Strong, down for Weak
          int half = hint > 0 ? r : 2 - r;
          gfx->fillRect(tx + 4 - half * 2 - 2, 359 + r * 2 - 2, 2 + half * 4 + 4, 6, sc565(TW_INK));
        }
        for (int r = 0; r < 3; r++) {
          int half = hint > 0 ? r : 2 - r;
          gfx->fillRect(tx + 4 - half * 2, 359 + r * 2, 2 + half * 4, 2, hc);
        }
      }
      twRimText(h, mx + (hint && !out ? 8 : 0), 355, hc);
    }
    (void)crossed;  // (the one just drained is asleep like the rest: no X)
  }
}
static void twViewLead() {
  uint8_t fl = towerFloorOf(twStep);
  twBackHall(fl, 77);
  scScreen();
  twSignFloor(twStep);
  twFoeStand(fl, twFoe.hp);
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(centerX(T(S_WHO_FIRST), 2), 214);
  printT(T(S_WHO_FIRST));
  twCards(-1);
  v2CloseX();
}
// 06b: a guardian drains the one it beat: its HP climbs, that one's card is crossed out (then who's next)
#define TW_DRAIN_MS 1600
static void twViewDrain() {
  uint8_t fl = towerFloorOf(twStep);
  twBackHall(fl, 77);
  scScreen();
  twSignFloor(twStep);
  uint32_t t = millis() - twDrainT0;
  float k = t >= TW_DRAIN_MS ? 1 : (float)t / TW_DRAIN_MS;
  twGlow(233, 120, 70, 0xC8A0F8, (int)(200 * (1 - k * .5f)));
  twFoeStand(fl, (uint16_t)(twDrainFrom + (twDrainTo - twDrainFrom) * k));
  char b[64], a1[12], a2[12];
  snprintf(a1, sizeof(a1), "%s", dexName(twFoeDex));
  snprintf(a2, sizeof(a2), "%s", dexName(twTeam[twDrained].dex));
  snprintf(b, sizeof(b), T(S_DRAIN_FMT), a1, a2);
  gfx->setTextColor(sc565(0xF0E0FF));
  v2Wrapped(b, 214, 330, 18, 1);
  twCards(twDrained);
  v2CloseX();
  if (t > 3600) twDrained = -1, twView = TW_LEAD;  // left alone, it moves on by itself
}
static int twLeadAt(int16_t x, int16_t y) {
  int n = twTeamN, w = 116, gap = 8, x0 = CX - (n * w + (n - 1) * gap) / 2;
  if (y < TW_LEAD_Y0 - 6 || y > TW_LEAD_Y1 + 4) return -1;
  for (int i = 0; i < n; i++)
    if (x >= x0 + i * (w + gap) - 4 && x <= x0 + i * (w + gap) + w + 4) return i;
  return -1;
}
// 08: a floor won: its badge, then on
static void twViewWon() {
  uint8_t fl = twStep;
  twBackHall(fl, 140);
  scScreen();
  twShield(CX, 84, 10, fl, true);
  for (int k = 1; k <= TOWER_FLOORS; k++) twShield(CX - 4 * 30 + (k - 1) * 30 + 15, 238, 2, k, k <= fl);  // this climb's
  char b[40];
  snprintf(b, sizeof(b), T(S_BADGE_FMT), TYPE_NAME[TOWER_TYPE[fl]]);
  v2SignIn(b, 30, sc565(TW_COL[fl][2]), sc565(TW_COL[fl][0]));
  v2Panel(96, 284, 370, 380, 10);
  gfx->setTextColor(UI_INK);
  setSize(2);
  snprintf(b, sizeof(b), T(S_FLOOR_WON_FMT), fl);
  setCur(centerX(b, 2), 296);
  printT(b);
  if (fl < TOWER_FLOORS) snprintf(b, sizeof(b), T(S_NEXT_FMT), fl + 1, TYPE_NAME[TOWER_TYPE[fl + 1]]);
  else snprintf(b, sizeof(b), "%s!", T(S_LEGEND));
  gfx->setTextColor(C565(0x3c, 0x8a, 0x4c));
  setCur(centerX(b, 2), 334);
  printT(b);
  v2CloseX();
}
// 10: the climb's over: who and where, its training against yours, one tip
static void twViewOver() {
  uint8_t fl = towerFloorOf(twBeatStep);
  twBackHall(fl, 158);
  scScreen();
  v2SignIn(T(S_CLIMB_OVER), 30, C565(0x6a, 0x30, 0x30), C565(0xc8, 0x40, 0x40));
  twDais(233, 130, 110, TW_COL[fl][0]);
  twThumb(thumbs.getForm(twBeatBy, false), 233, 144, 3);
  char b[96];
  if (twBeatStep > TOWER_FLOORS) snprintf(b, sizeof(b), "%s - %s", dexName(twBeatBy), T(S_LEGEND));
  else snprintf(b, sizeof(b), T(S_BEAT_YOU_FMT), dexName(twBeatBy), twBeatStep);
  twChip(b, 233, 156);
  v2Panel(70, 196, 396, 376, 10);
  // what to try next (Fedor: "a kid who loses at 7F should know exactly what to try next"): a type that's Strong
  // against it, or else the training the team is shortest of, with its training against yours
  char tip[200];
  uint8_t kind = TOWER_TIP_ATK;
  if (twRan) {
    snprintf(tip, sizeof(tip), "%s", T(S_TWR_TIP));
  } else {
    BtlFighter team[TOWER_TEAM], foe = towerGuardian(twBeatStep, twBeatBy, twLevel());
    const BtlFighter *last = nullptr;
    for (int i = 0; i < twTeamN; i++) {
      team[i] = twTeam[i].f;
      if (twTeam[i].dex == twLastDex) last = &twTeam[i].f;
    }
    uint8_t strong[2], ns = towerStrongTypes(twBeatBy, foe.level, strong);
    char types[24];
    snprintf(types, sizeof(types), ns > 1 ? "%s/%s" : "%s", ns ? TYPE_NAME[strong[0]] : "", ns > 1 ? TYPE_NAME[strong[1]] : "");
    kind = towerTip(last, foe, team, twTeamN, pet.trAtk, pet.trDef, pet.trSpe);
    switch (kind) {
      case TOWER_TIP_WEAK: snprintf(tip, sizeof(tip), T(S_TIP_WEAK_FMT), dexName(twLastDex), dexName(twBeatBy), types); break;
      case TOWER_TIP_NOSTRONG: snprintf(tip, sizeof(tip), T(S_TIP_NOSTRONG_FMT), dexName(twBeatBy), types); break;
      case TOWER_TIP_DEF: snprintf(tip, sizeof(tip), "%s", T(S_TIP_DEF)); break;
      case TOWER_TIP_SPD: snprintf(tip, sizeof(tip), "%s", T(S_TIP_SPD)); break;
      default: snprintf(tip, sizeof(tip), "%s", T(S_TIP_ATK)); break;
    }
  }
  bool cheer = !twRan && pet.towerCheer && pet.towerBest < TOWER_LEGEND;
  if (kind == TOWER_TIP_WEAK || kind == TOWER_TIP_NOSTRONG) {  // a type to bring: the tip has the panel
    gfx->setTextColor(UI_INK);
    int n = v2Wrapped(tip, 0, 300, 24, 5, -1, false);
    v2Wrapped(tip, (cheer ? 278 : 286) - n * 12, 300, 24, 5);
  } else {
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(T(S_TRAINING), 2), 208);
    printT(T(S_TRAINING));
    uint8_t theirs = TOWER_TRAIN[twBeatStep], yours = towerTrainAvg(pet.trAtk, pet.trDef, pet.trSpe);
    const char *lab[2] = { dexName(twBeatBy), T(S_YOURS) };
    uint8_t val[2] = { theirs, yours };
    for (int k = 0; k < 2; k++) {
      int yy = 234 + k * 28;
      char nb[12];
      snprintf(nb, sizeof(nb), "%.7s", lab[k]);
      gfx->setTextColor(UI_INK);
      setCur(88, yy);
      printT(nb);
      gfx->fillRoundRect(180, yy + 2, 192, 12, 4, sc565(0xD8D2BD));
      gfx->fillRoundRect(180, yy + 2, 192 * val[k] / 100 + 4, 12, 4, k ? UI_BAR_OK : sc565(TW_COL[fl][0]));
    }
    gfx->setTextColor(V2_INK_MUTED);
    v2Wrapped(tip, 292, 300, 20, 3);
  }
  if (cheer) {  // the friends will cheer the next climb: a gold ribbon on the panel's foot
    snprintf(b, sizeof(b), T(S_CHEER_NEXT_FMT), TOWER_CHEER_PCT * pet.towerCheer);
    twChip(b, CX, 362, C565(0xc0, 0x84, 0x10));
  }
  v2CloseX();
}
// 11: the Legend: its first guardian, and a shadow
static void twViewLegend() {
  twBackLegend(0);
  scScreen();
  char b[32];
  v2SignIn(T(S_LEGEND), 16, C565(0x5a, 0x3a, 0x98), C565(0x8a, 0x6a, 0xd0));
  twDais(140, 216, 130, 0x9A6AD8);
  twDais(326, 216, 130, 0x9A6AD8);
  twThumb(thumbs.getForm(twFoeDex, false), 140, 230, 3);
  twThumb(thumbs.getForm(DEX_MEWTWO, false), 326, 230, 3, 0x0E0A24);
  snprintf(b, sizeof(b), "Lv %d", towerLevel(TOWER_LEGEND, twLevel()));
  twChip(b, 140, 240);
  twChip("???", 326, 240);
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(centerX(T(S_BEAT_BOTH), 2), 280);
  printT(T(S_BEAT_BOTH));
  v2LabelButton(TW_CLIMB_X0, TW_CLIMB_Y0, TW_CLIMB_X1, TW_CLIMB_Y1, T(S_FIGHT), C565(0x9a, 0x6a, 0xd8));
  v2CloseX();
}
// 12: Tower Master: the Hall of Fame photo
static void twViewMaster() {
  twBackLegend(64);
  scScreen();
  v2SignIn(T(S_TOWER_MASTER), 16, C565(0x8a, 0x6a, 0x10), C565(0xf0, 0xc0, 0x4a));
  scRect(80, 196, 385, 325, TW_INK);
  scRect(84, 200, 381, 321, 0xC08410);
  scRect(92, 208, 373, 313, 0xFFF0B0);
  scRect(96, 212, 369, 277, 0x7CC4EA);
  scRect(96, 278, 369, 309, 0x79BD4F);
  for (int i = 0; i < 3; i++)
    if (pet.hofDex[i]) twThumb(thumbs.getForm(pet.hofDex[i], pet.hofShiny >> i & 1), 140 + i * 92, 300, 2);
  v2Panel(104, 334, 362, 384, 8);
  twBall(true, 112, 340, 2);  // the Master Ball it pays
  char b[32];
  gfx->setTextColor(C565(0x78, 0x48, 0xb8));
  setSize(2);
  setCur(160, 342);
  printT((twGot & 2) ? T(S_MASTER_PLUS) : "");  // (none if you hold the most already)
  snprintf(b, sizeof(b), "Lv %u", pet.hofLevel);
  gfx->setTextColor(V2_INK_MUTED);
  setCur(160, 362);
  printT(b);
  v2CloseX();
}

// 13: the challenges, once the Legend is beaten: three medals, gold once done, NEW! on those this climb did
static void twViewChal() {
  twBackLegend(96);
  scScreen();
  v2SignIn(T(S_CHALLENGES), 30, C565(0x8a, 0x6a, 0x10), C565(0xf0, 0xc0, 0x4a));
  v2Panel(64, 96, 402, 372, 10);
  gfx->setTextColor(V2_INK_MUTED);
  v2Wrapped(T(S_CH_HOW), 112, 300, 20, 2);
  static const StrId CH[3] = { S_CH_NOFAINT, S_CH_ONESTAR, S_CH_BASE };
  for (int k = 0; k < 3; k++) {
    int y = 166 + k * 64;
    bool done = pet.towerChal >> k & 1, fresh = twChalNew >> k & 1;
    gfx->fillCircle(104, y + 14, 19, sc565(TW_INK));
    gfx->fillCircle(104, y + 14, 16, done ? sc565(0xF0C04A) : sc565(0xC8C4D0));
    gfx->fillCircle(104, y + 14, 8, done ? sc565(0xFFE27A) : sc565(0xE4E2EA));
    gfx->setTextColor(done ? UI_INK : V2_INK_MUTED);
    v2Wrapped(T(CH[k]), y + (v2Wrapped(T(CH[k]), 0, 220, 18, 2, 136, false) > 1 ? 0 : 8), 220, 18, 2, 136);
    if (fresh) twChip(T(S_TWR_NEW), 340, y + 30, C565(0x3c, 0x8a, 0x4c));
  }
  v2CloseX();
}

void towerRender() {
  if (twView != TW_LEAD && twView != TW_PICK && twView != TW_WON && twView != TW_LEGEND) {}
  switch (twView) {
    case TW_TOWER: twViewTower(); break;
    case TW_HOW: twViewHow(); break;
    case TW_TIRED: twViewTired(); break;
    case TW_PICK: twViewPick(); break;
    case TW_TEAM: twViewTeam(); break;
    case TW_LEAD: twViewLead(); break;
    case TW_DRAIN: twViewDrain(); break;
    case TW_WON: twViewWon(); break;
    case TW_OVER: twViewOver(); break;
    case TW_LEGEND: twViewLegend(); break;
    case TW_MASTER: twViewMaster(); break;
    case TW_CHAL: twViewChal(); break;
  }
}

static void twNextAfterWin() {
  if (twStep >= TOWER_FLOORS) {  // 8F done: the Legend
    twStep = TOWER_LEGEND;
    twRollFoe();
    twView = TW_LEGEND;
    return;
  }
  twStep++;
  twRollFoe();
  twView = TW_LEAD;
}

// testing (TOWERSTEP): a climb under way jumps to step n (9: the Legend's intro)
void towerJump(uint8_t step) {
  if (!twTeamN || step < 1 || step > TOWER_STEPS) return;
  twStep = step;
  twRollFoe();
  twView = step == TOWER_LEGEND ? TW_LEGEND : TW_LEAD;
}

// the X, or a swipe down: back a step
void towerBack() {
  sfxPlay(SFX_TAP);
  switch (twView) {
    case TW_TOWER: zoneScreen = ZS_PICK; v2IslePage = 1; break;  // back to the islet on Travel
    case TW_LEAD: case TW_DRAIN: case TW_WON: case TW_LEGEND:  // leaving mid-climb ends it (no loss)
      twBeatBy = twFoeDex, twBeatStep = twStep, twRan = true;
      twEndClimb();
      twView = TW_OVER;
      break;
    case TW_PICK: twView = twPickReady() ? TW_TEAM : TW_TOWER; break;
    default: twView = TW_TOWER; twEndClimb(); break;
  }
}

void towerTap(int16_t x, int16_t y) {
  if (closeHit(x, y)) { towerBack(); return; }
  switch (twView) {
    case TW_TOWER:
      if (pet.towerBest >= TOWER_LEGEND && x >= twChalX0 - 4 && x <= twChalX0 + 104 && y >= twChalY0 - 8 && y <= twChalY0 + 40) {
        sfxPlay(SFX_TAP);  // the challenge medals: what they are
        twChalNew = 0;
        twView = TW_CHAL;
        return;
      }
      if ((y >= TW_CLIMB_Y0 - 6 && y <= TW_CLIMB_Y1 + 6 && x >= TW_CLIMB_X0 - 6 && x <= TW_CLIMB_X1 + 6) ||
          (y >= 40 && y < TW_BASE && abs(x - CX) <= 90))  // the button, or the tower itself
        twClimbTapped();
      break;
    case TW_HOW:
      if (y >= 256 && y <= 320 && x >= 110 && x <= 356) {
        pet.towerHowSeen = true;
        pet.towerSave();
        twClimbTapped();
      }
      break;
    case TW_TIRED: sfxPlay(SFX_TAP); twView = TW_TOWER; break;
    case TW_PICK: {
      if (int d = pageArrowAt(x, y, twPickPage, twPickPages(), V2_PAGE_ARROW_Y)) {
        sfxPlay(SFX_TAP);
        twPickPage += d;
        return;
      }
      if (x < DEXG_X || y < DEXG_Y || x >= DEXG_X + DEXG_COLS * DEXG_CELL || y >= DEXG_Y + 3 * DEXG_CELL) return;
      bool sh;
      int16_t d = twPickList(twPickPage * DEXG_PAGE + (y - DEXG_Y) / DEXG_CELL * DEXG_COLS + (x - DEXG_X) / DEXG_CELL, sh);
      if (!d) return;
      if (twPicked(d, sh)) {  // a second tap takes it out
        int k = twPickDex[0] == d && twPickShiny[0] == sh ? 0 : 1;
        if (k == 0 && twPickN == 2) twPickDex[0] = twPickDex[1], twPickShiny[0] = twPickShiny[1];
        twPickN--;
        sfxPlay(SFX_TAP);
      } else if (twAllowed(d, sh) && twPickN < 2) {
        twPickDex[twPickN] = d;
        twPickShiny[twPickN++] = sh;
        sfxPlay(SFX_TAP);
        if (twPickReady()) twView = TW_TEAM;  // both picked: the team, and GO!
      } else {
        sfxPlay(SFX_DENY);
      }
      break;
    }
    case TW_TEAM: {
      if (y >= TW_CLIMB_Y0 - 6 && y <= TW_CLIMB_Y1 + 6 && x >= TW_CLIMB_X0 - 6 && x <= TW_CLIMB_X1 + 6) { twStartClimb(); return; }
      int n = 1 + twPickN;
      for (int i = 1; i < n; i++)  // a friend: change it (out of the team, back to the grid)
        if (abs(x - twTeamSlotX(i, n)) < 60 && y > 100 && y < 268) {
          sfxPlay(SFX_TAP);
          if (i == 1 && twPickN == 2) twPickDex[0] = twPickDex[1], twPickShiny[0] = twPickShiny[1];
          twPickN--;
          twView = TW_PICK;
          return;
        }
      break;
    }
    case TW_DRAIN:  // on to who's next (once the bar has climbed)
      if (millis() - twDrainT0 > TW_DRAIN_MS / 2) { sfxPlay(SFX_TAP); twDrained = -1; twView = TW_LEAD; }
      break;
    case TW_LEAD: {
      int i = twLeadAt(x, y);
      if (i >= 0 && twTeam[i].f.hp) { sfxPlay(SFX_TAP); twFight(i); }
      else if (i >= 0) sfxPlay(SFX_DENY);
      break;
    }
    case TW_WON: sfxPlay(SFX_TAP); twNextAfterWin(); break;
    case TW_LEGEND:
      if (y >= TW_CLIMB_Y0 - 6 && y <= TW_CLIMB_Y1 + 6 && x >= TW_CLIMB_X0 - 6 && x <= TW_CLIMB_X1 + 6) { sfxPlay(SFX_TAP); twView = TW_LEAD; }
      break;
    case TW_MASTER: sfxPlay(SFX_TAP); twView = TW_CHAL; break;  // then the challenges (new, or what they are)
    case TW_OVER: case TW_CHAL: sfxPlay(SFX_TAP); twView = TW_TOWER; break;
  }
}

// what a press (or, with a mouse, the hover) would land on: the screen's button, a Pokemon that can be picked, a
// challenge medal, the page arrows or the X (pressTargetAt)
bool towerPressAt(int16_t x, int16_t y, PressTarget &t) {
  auto inBtn = [&](int x0, int y0, int x1, int y1) { return x >= x0 - 6 && x <= x1 + 6 && y >= y0 - 6 && y <= y1 + 6; };
  switch (twView) {
    case TW_TOWER:
      if (pet.towerBest >= TOWER_LEGEND && x >= twChalX0 - 4 && x <= twChalX0 + 104 && y >= twChalY0 - 8 && y <= twChalY0 + 40)
        return v2PressSet(t, twChalX0, twChalY0, 96, 32, T(S_CHALLENGES));
      if (inBtn(TW_CLIMB_X0, TW_CLIMB_Y0, TW_CLIMB_X1, TW_CLIMB_Y1))
        return v2PressSet(t, TW_CLIMB_X0, TW_CLIMB_Y0, TW_CLIMB_X1 - TW_CLIMB_X0, TW_CLIMB_Y1 - TW_CLIMB_Y0, T(S_CLIMB));
      break;
    case TW_HOW:
      if (inBtn(116, 262, 350, 314)) return v2PressSet(t, 116, 262, 234, 52, T(S_LETS_CLIMB));
      break;
    case TW_LEGEND:
      if (inBtn(TW_CLIMB_X0, TW_CLIMB_Y0, TW_CLIMB_X1, TW_CLIMB_Y1))
        return v2PressSet(t, TW_CLIMB_X0, TW_CLIMB_Y0, TW_CLIMB_X1 - TW_CLIMB_X0, TW_CLIMB_Y1 - TW_CLIMB_Y0, T(S_FIGHT));
      break;
    case TW_TEAM: {
      if (inBtn(TW_CLIMB_X0, TW_CLIMB_Y0, TW_CLIMB_X1, TW_CLIMB_Y1))
        return v2PressSet(t, TW_CLIMB_X0, TW_CLIMB_Y0, TW_CLIMB_X1 - TW_CLIMB_X0, TW_CLIMB_Y1 - TW_CLIMB_Y0, T(S_GO));
      int n = 1 + twPickN;
      for (int i = 1; i < n; i++)
        if (abs(x - twTeamSlotX(i, n)) < 60 && y > 100 && y < 268) return v2PressSet(t, twTeamSlotX(i, n) - 60, 100, 120, 168, dexName(twPickDex[i - 1]));
      break;
    }
    case TW_PICK: {
      if (x >= DEXG_X && y >= DEXG_Y && x < DEXG_X + DEXG_COLS * DEXG_CELL && y < DEXG_Y + 3 * DEXG_CELL) {
        int c = (x - DEXG_X) / DEXG_CELL, r = (y - DEXG_Y) / DEXG_CELL;
        bool sh;
        int16_t d = twPickList(twPickPage * DEXG_PAGE + r * DEXG_COLS + c, sh);
        if (d && (twAllowed(d, sh) || twPicked(d, sh))) return v2PressSet(t, DEXG_X + c * DEXG_CELL, DEXG_Y + r * DEXG_CELL, DEXG_CELL, DEXG_CELL, dexName(d));
      }
      return v2PressNav(x, y, t, twPickPage, twPickPages(), V2_PAGE_ARROW_Y, true);
    }
    case TW_LEAD: {
      int i = twLeadAt(x, y);
      if (i >= 0 && twTeam[i].f.hp) {
        int n = twTeamN, w = 116, gap = 8, x0 = CX - (n * w + (n - 1) * gap) / 2;
        return v2PressSet(t, x0 + i * (w + gap), TW_LEAD_Y0, w, TW_LEAD_Y1 - TW_LEAD_Y0, dexName(twTeam[i].dex));
      }
      break;
    }
    default: break;
  }
  return v2PressNav(x, y, t, 0, 0, 0, true);
}

// Travel: its page arrows, and the tower on page 2. True if it took the tap
bool v2TravelTap(int16_t x, int16_t y) {
  int dx = x - (v2IslePage ? 36 : 430), dy = y - V2_PAGE_ARROW_Y;
  if (dx * dx + dy * dy < 32 * 32) {
    sfxPlay(SFX_TAP);
    v2IslePage = !v2IslePage;
    return true;
  }
  if (!v2IslePage) return false;
  bool tower = twOnIslet(x, y) || (abs(x - TW_IX) < 70 && y > 40 && y < TW_IY + 110);
  if (tower) towerOpen();
  return true;  // page 2: nothing else to tap but the X
}
