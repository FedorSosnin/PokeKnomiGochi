// V2 step 4: the walk. scene_v2.h includes this at its end, so it's only there with UI_V2 on.
//
// At rest on the home screen the Pokemon sometimes walks toward you (PMD Walk row 0: PMD_WALKD) while the
// world slides away behind it, as walk.py draws it (walk-field-band.gif, walk-beach.gif): grass and flowers
// come up from the bottom and shrink toward the horizon, the land bobs 2 px on every other step, the far and
// middle hills sway a few px out of step, the clouds drift and the birds flap. One loop is 8.4 s (84 of the
// mockup's 100 ms frames). The world's clock runs only while it walks, so when it stops everything holds
// still where it was, the grass too.
//
// The scene is put together every frame: the sky from the scene buffer (v2Cached), then the far, middle and
// near hills, each from a strip of its own (drawn once per zone and time of day, with what stands on it) and
// shifted by its sway and the bob, then what stands in front of them, then the stream. The Forest, the Cave
// and the Mansion weren't mocked walking: they move the same way, with their own things in the stream.

// ----- the world's clock -----
static uint32_t v2WalkMs = 0;     // how far it has walked
static uint32_t v2WalkLast = 0;   // the last frame it walked in (0: it wasn't walking)
static uint32_t v2RestUntil = 0;  // after a walk it rests a while (behNext)
static bool v2Walking = false;    // walking this frame
static int v2Bob = 0;             // the land and everyone on it are 2 px lower on every other step

// the walk's time at the home's slow pace: the packs time PMD at 1.4x its own pace, this walk is 2.0x
// (a step cycle of 1.2 s for most)
static uint32_t v2WalkT() { return v2WalkMs * 7 / 10; }

// It walks while the planner has chosen a walk (behNext), it's happy, and nothing stops it: a bath, sleep,
// food, evolving, a ceremony, a dialog or the food picker.
static bool v2CanWalk() {
  return beh.mode == 1 && pmd.has(PMD_WALKD) && !pet.isEgg() && !pet.evolving() && !pet.ceremony &&
         pet.mood() == MOOD_HAPPY && !bathUntil && !feedMenuUntil && !confirmUntil && !choiceKind &&
         !homeModal() && !timeLeft(farHintUntil);
}
// once a frame, before the scene: moves the clock on while it walks, and sets the bob by its step
static void v2WalkUpdate() {
  uint32_t now = millis();
  v2Walking = v2CanWalk();
  if (v2Walking) {
    uint32_t dt = v2WalkLast ? now - v2WalkLast : 0;
    if (dt > 300) dt = 300;  // a long gap (another screen in between) isn't a long stride
    v2WalkMs += dt;
    v2WalkLast = now;
  } else {
    v2WalkLast = 0;
  }
  v2Bob = v2Walking && (pmdFrameAt(pmd.acts[PMD_WALKD], v2WalkT(), true) & 1) ? 2 : 0;
}

// ----- the layers -----
// A strip holds one layer of hills and what stands on it, in screen rows y0..y0+h-1 and reaching m px past
// each side (as far as it sways), with the magenta key where the layer has nothing. What it covers is kept
// as runs, so putting it on the screen is a copy per run.
struct V2Strip {
  int y0, h, m;
  uint16_t *px;    // (V2_W + 2m) x h
  int32_t *run0;   // where each row's runs start in runs[] (h + 1 of them)
  int16_t *runs;   // x0, x1 pairs (strip columns, x1 just past the run)
  int room;        // runs[] has room for this many
};
static V2Strip v2Far = { 88, 232, 4, nullptr, nullptr, nullptr, 0 };   // down to below the hills' lowest dip
static V2Strip v2Mid = { 240, 80, 8, nullptr, nullptr, nullptr, 0 };
static V2Strip v2Near = { 272, 194, 0, nullptr, nullptr, nullptr, 0 };  // only bobs
static int v2StripKey = -1;  // zone * 4 + tod of what they hold
#define V2_WALK_SKY_ROWS 320  // from here down the near hill covers everything, bobbing or not

static bool v2StripPaint(V2Strip &s, void (*paint)(uint8_t, uint8_t), uint8_t zone, uint8_t tod) {
  int w = V2_W + 2 * s.m;
  if (!s.px) s.px = (uint16_t *)ps_malloc(w * s.h * sizeof(uint16_t));
  if (!s.run0) s.run0 = (int32_t *)ps_malloc((s.h + 1) * sizeof(int32_t));
  if (!s.px || !s.run0) return false;
  for (int i = 0; i < w * s.h; i++) s.px[i] = V2_STRIP_KEY;
  scFb = s.px, scW = w, scH = s.h, scDX = s.m, scDY = -s.y0, scKeyed = true;
  paint(zone, tod);
  scScreen();
  scGradeTod = 0;
  for (int pass = 0; pass < 2; pass++) {  // count the runs, then note them
    int n = 0;
    for (int r = 0; r < s.h; r++) {
      if (pass) s.run0[r] = n;
      const uint16_t *row = s.px + r * w;
      for (int x = 0; x < w;) {
        if (row[x] == V2_STRIP_KEY) {
          x++;
          continue;
        }
        int x0 = x;
        while (x < w && row[x] != V2_STRIP_KEY) x++;
        if (pass) s.runs[2 * n] = x0, s.runs[2 * n + 1] = x;
        n++;
      }
    }
    if (pass) {
      s.run0[s.h] = n;
    } else if (n > s.room) {
      free(s.runs);
      s.runs = (int16_t *)ps_malloc((2 * n + 2) * sizeof(int16_t));
      s.room = s.runs ? n : 0;
      if (!s.runs) return false;
    }
  }
  return true;
}
static void v2StripBlit(const V2Strip &s, int dx, int dy) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  int w = V2_W + 2 * s.m;
  for (int r = 0; r < s.h; r++) {
    int y = s.y0 + r + dy;
    if (y < 0 || y >= V2_W) continue;
    uint16_t *dst = fb + y * V2_W;
    const uint16_t *src = s.px + r * w;
    for (int k = s.run0[r]; k < s.run0[r + 1]; k++) {
      int a = s.runs[2 * k], x0 = a - s.m + dx, x1 = s.runs[2 * k + 1] - s.m + dx;
      if (x0 < 0) a -= x0, x0 = 0;
      if (x1 > V2_W) x1 = V2_W;
      if (x1 > x0) memcpy(dst + x0, src + a, (x1 - x0) * sizeof(uint16_t));
    }
  }
}

// what each layer holds, drawn in screen coordinates (as the still scene has it, without the grass and
// flowers on the near hill: the stream replaces them)
static uint8_t v2LandTod(uint8_t zone, uint8_t tod) { return zone == Z_CAVE || zone == Z_MANSION ? 0 : tod; }
static void v2PaintFar(uint8_t zone, uint8_t tod) {
  if (zone == Z_BEACH) return;  // the sea is behind the dunes, drawn every frame
  ScPal p = scZonePal(zone);
  scGradeTod = v2LandTod(zone, tod);
  ScLayer L[1] = { { p.far, SC_FAR, (uint8_t)(zone == Z_MANSION ? 3 : 2) } };
  scHills(L, 1, p.lit, 1);
  if (zone == Z_FIELD) {
    scHouse(50, 186, tod == 2);
    for (auto &t : SC_FIELD_TREES) scRole(SPR_V2_TREE, 10, t[0], t[1], 3, p, p.mid);
  } else if (zone == Z_FOREST) {
    scPineWall(p);
  } else if (zone == Z_CAVE) {
    scCaveBack();
  } else if (zone == Z_MANSION) {
    scMansionFar(p);
  }
}
static void v2PaintMid(uint8_t zone, uint8_t tod) {
  ScPal p = scZonePal(zone);
  scGradeTod = v2LandTod(zone, tod);
  ScLayer L[1] = { { p.mid, zone == Z_BEACH ? SC_DUNES : SC_MID, 2 } };
  scHills(L, 1, p.lit, 1);
  if (zone == Z_FIELD) {  // the wooden fence, its rails reaching into the margins so a sway shows no gap
    const Rgb post = 0x9A6F43, rail = 0xB38A58;
    scFence(262, 0, 160, post, rail, 24);
    scFence(266, 312, 466, post, rail, 24);
    scRect(-8, 262 - 12, -1, 262 - 9, rail);
    scRect(-8, 262 - 4, -1, 262 - 1, rail);
    scRect(467, 266 - 12, 474, 266 - 9, rail);
    scRect(467, 266 - 4, 474, 266 - 1, rail);
  }
}
static void v2PaintNear(uint8_t zone, uint8_t tod) {
  ScPal p = scZonePal(zone);
  scGradeTod = v2LandTod(zone, tod);
  ScLayer L[1] = { { p.near, zone == Z_BEACH ? SC_SHORE : SC_NEAR, 1 } };
  scHills(L, 1, p.lit, 1);
}
// the sky as the still scene has it, without its clouds and birds (they move)
static void v2PaintSky(int key) {
  uint8_t zone = key / 4, tod = key % 4;
  ScPal p = scZonePal(zone);
  if (zone == Z_CAVE) {
    scCaveWall(p);
    return;
  }
  const ScPal &s = zone == Z_MANSION || tod == 0 ? p : tod == 2 ? SC_NIGHT : SC_DUSK_SKY;
  scSky(s.top, s.bot);
  scRect(0, 300, V2_W, V2_WALK_SKY_ROWS, s.bot);  // under the sky's bands, where a dip in the hills shows it
  if (zone == Z_MANSION) scSun(p, true, 372, 128);
  else if (tod == 0) scSun(p, false, zone == Z_FIELD ? 110 : 352, zone == Z_FIELD ? 134 : 128);
  else scSun(s, tod == 2, 352, 128);
}

// ----- what moves on its own -----
// walk.py's random tables, worked out with Python's random as it ran them. The stream (its Random(5)): a
// lane (-1..1, left or right of the pet), where in the loop it starts, a flower or not, and which of the
// three flower colours (int(lane * 10) % 3, as Python works it)
static const double V2_STREAM[14][4] = {
  { 0.8192508924825106, 0.039759678278284832, 1, 2 },   { -0.88824368436586743, 0.11022649980035727, 0, 1 },
  { -0.93063034422543578, 0.14851744108980006, 0, 0 },  { -0.56653054193133556, 0.21938443439441005, 0, 1 },
  { -0.8118286017455969, 0.30612183846301139, 0, 1 },   { 0.41172294865062675, 0.39700020671441738, 0, 1 },
  { 0.38868946278518879, 0.42866017168155529, 0, 0 },   { -0.84151559211937199, 0.54800635501087502, 0, 1 },
  { -0.5025136174228485, 0.61950247087607557, 1, 1 },   { -0.42716806798192258, 0.69128498157403806, 0, 2 },
  { 0.50915222849770747, 0.73234521102183348, 0, 2 },   { -0.48466835726497914, 0.80230701007531757, 1, 2 },
  { -0.71718526538129124, 0.89252666476396991, 0, 2 },  { 0.5487459501099784, 0.94387464152730027, 1, 2 },
};
// the Beach's glints (its Random(3)): five on each of five rows, where each starts and which way it slides
static const int16_t V2_GLINT[25][2] = {
  { 121, -1 }, { 189, 1 },  { 320, -1 }, { 310, -1 }, { 465, 1 },  { 132, -1 }, { 98, 1 },   { 276, 1 },  { 203, -1 },
  { 118, -1 }, { 444, 1 },  { 379, -1 }, { 343, -1 }, { 81, -1 },  { 154, -1 }, { 421, 1 },  { 242, 1 },  { 365, 1 },
  { 202, 1 },  { 68, 1 },   { 49, -1 },  { 69, 1 },   { 111, 1 },  { 344, 1 },  { 398, 1 },
};
// a palm with its fronds leaning (walk.py: the top five rows a cell to the left)
static const char *const V2_PALM_LEAN[12] = {
  ".11....11...", "1221..1221..", "22221112221.", "..1222221.1.", "..1122211...", "..121.5.121.",
  "..1...5...1.", "......5.....", "......5.....", ".....5......", ".....5......", "....555.....",
};
static int v2Round(double v) { return (int)lrint(v); }  // as Python rounds: halves to the even side

// The stream: on lanes either side of the pet, things come up big from the bottom and shrink as they slide
// toward the horizon, the far ones drawn first. Grass and flowers (sandy grass on the Beach, pink flowers in
// the Forest), rocks and crystals in the Cave, grass alone in the Mansion.
static void v2Stream(uint8_t zone, const ScPal &p, double f) {
  struct It { double y, x; int s, i; } it[14];
  int n = 0;
  for (int i = 0; i < 14; i++) {
    double t = V2_STREAM[i][1] - f / 84;
    t -= floor(t);
    double y = 290 + pow(t, 1.5) * 130, x = 233 + V2_STREAM[i][0] * (36 + t * 230);
    if (y > 420 || x < 0 || x > V2_W) continue;
    It e = { y, x, t < .35 ? 2 : t < .7 ? 3 : 4, i };
    int k = n++;
    for (; k > 0 && (it[k - 1].y > e.y || (it[k - 1].y == e.y && it[k - 1].x > e.x)); k--) it[k] = it[k - 1];
    it[k] = e;
  }
  const ScKey grass[2] = { { 'G', p.dark }, { 'g', scMix(p.near, p.dark, .35) } };
  for (int k = 0; k < n; k++) {
    int x = (int)it[k].x / 2 * 2, y = (int)it[k].y / 2 * 2, s = it[k].s;
    bool flower = V2_STREAM[it[k].i][2] > 0;
    if (zone == Z_CAVE) {
      if (flower) scSprite(SPR_V2_CRYSTAL, 6, x - 5 * s / 2, y, s, SC_CRYSTAL_KEYS, 2);
      else scRole(SPR_V2_ROCK, 5, x - 4 * s, y, s, p, p.near);
    } else if (!flower) {
      scSprite(SPR_V2_TALLGRASS, 5, x - 3 * s, y, s, grass, 2);
    } else if (zone != Z_MANSION) {
      Rgb c = zone == Z_FOREST ? SC_FOREST_PINK : p.fl[(int)V2_STREAM[it[k].i][3]];
      scRect(x, y, x + s + 1, y + s + 1, c);
      scRect(x + s / 2, y + s + 2, x + s / 2 + 1, y + 2 * s + 3, p.dark);
    }
  }
}

// The home's scene for the five zones, put together for this frame. False without the memory for the strips
// (the caller then draws the still scene).
static bool v2DrawWalkScene(uint8_t zone, uint8_t tod) {
  if (zone == Z_CAVE || zone == Z_MANSION) tod = 0;
  int key = zone * 4 + tod;
  if (key != v2StripKey) {
    v2StripKey = -1;
    if (!v2StripPaint(v2Far, v2PaintFar, zone, tod) || !v2StripPaint(v2Mid, v2PaintMid, zone, tod) ||
        !v2StripPaint(v2Near, v2PaintNear, zone, tod))
      return false;
    v2StripKey = key;
  }
  v2Cached(700 + key, v2PaintSky, key, V2_WALK_SKY_ROWS);  // the sky, from the buffer
  scScreen();
  ScPal p = scZonePal(zone);
  double f = v2WalkMs / 100.0, th = 6.283185307179586 * f / 84;  // in the mockup's frames
  int fi = (int)(v2WalkMs / 100), bob = v2Bob;
  int fdx = v2Round(sin(th)) * 4;                                          // far: 4 px either way
  int mdx = v2Round((zone == Z_BEACH ? 2.0 : 2.5) * sin(th + .8)) * 4;     // middle: 8, out of step
  int cdx = v2Round(3 * sin(th)) * 4;                                      // the clouds drift 12
  // the sky's own: clouds, and the Field's birds flapping every 5 frames
  scGradeTod = 0;
  if (zone == Z_MANSION) {
    scCloud(p, 60 + cdx, 150, 3);
  } else if (zone != Z_CAVE) {
    const ScPal &s = tod == 0 ? p : tod == 2 ? SC_NIGHT : SC_DUSK_SKY;
    if (tod == 0 && zone == Z_FIELD) {
      scCloud(p, 260 + cdx, 110, 4);
      scCloud(p, 330 + cdx / 2, 182, 3);
    } else if (tod == 0 && zone == Z_FOREST) {
      scCloud(p, 118 + cdx, 104, 4);
    } else {
      scCloud(s, 116 + cdx, 104, 4);
      scCloud(s, 292 + cdx / 2, 184, 4);
    }
  }
  scGradeTod = v2LandTod(zone, tod);
  if (zone == Z_FIELD) {
    const ScKey bird[1] = { { 'k', 0x18181F } };
    bool up = (fi / 5) % 2;
    const char *const *b = up ? SPR_V2_BIRD_UP : SPR_V2_BIRD_DOWN;
    scSprite(b, 3, 196 + cdx * 2, 150, 3, bird, 1);
    scSprite(b, 3, 226 + cdx * 2, 140 + (up ? 2 : 0), 3, bird, 1);
  }
  if (zone == Z_BEACH) {  // the sea, its glints sliding sideways and blinking
    scRect(0, 224 + bob, V2_W, 300, SC_SEA);
    scRect(0, 224 + bob, V2_W, 227 + bob, scMix(SC_SEA, 0xFFFFFF, .35));
    for (int i = 0; i < 25; i++) {
      int y = 236 + i / 5 * 12;
      if ((fi + i % 5 * 5) % 24 >= 19) continue;
      double xf = fmod(V2_GLINT[i][0] + V2_GLINT[i][1] * (1 + (y - 236) / 30.0) * f * 2, V2_W + 40);
      if (xf < 0) xf += V2_W + 40;
      int x = scFl4((int)xf - 20) * 4;
      scRect(x, y + bob, x + 11, y + bob + 3, y < 260 ? SC_SEA_HI : scMix(SC_SEA_HI, SC_SEA, .4));
    }
  }
  v2StripBlit(v2Far, fdx, bob);
  v2StripBlit(v2Mid, mdx, bob);
  if (zone == Z_BEACH) {  // palms on the dunes, under the shore's edge, their fronds leaning on alternate beats
    ScKey palm[5];
    scPalmKeys(palm, p);
    const char *const *spr = (fi / 12) % 2 ? V2_PALM_LEAN : SPR_V2_PALM;
    scSprite(spr, 12, 34 + mdx, 224 + bob, 4, palm, 5);
    scSprite(spr, 12, 368 + mdx, 236 + bob, 4, palm, 5);
  }
  v2StripBlit(v2Near, 0, bob);
  // what stands in front of the hills
  if (zone == Z_FOREST) {
    for (auto &t : SC_FOREST_BIG) scRole(SPR_V2_PINE, 12, t[0] + mdx, t[1] + bob, 4, p, p.mid);
  } else if (zone == Z_MANSION) {
    scGraveyard(mdx, bob);
  } else if (zone == Z_CAVE) {
    scCaveGlints();
  }
  v2Stream(zone, p, f);
  scGradeTod = 0;
  return true;
}

#ifdef BOARD_HOST
// for the host tests and the screenshot tool: start a walk now; how far it has walked; is it walking
void v2HostStartWalk() {
  beh.mode = 1;
  beh.t0 = millis();
  beh.until = millis() + 60000;
}
uint32_t v2HostWalkMs() { return v2WalkMs; }
bool v2HostWalking() { return v2Walking; }
#endif
#ifdef BOARD_HOST
// for the screenshot tool: a zone's still scene (the one an egg has, and what the walk's layers come from)
void v2HostStillScene(int zone, int tod) {
  v2SceneKey = -1;
  scPaint(zone, tod);
}
#endif
