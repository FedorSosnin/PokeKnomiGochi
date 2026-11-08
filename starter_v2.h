// V2 step 6: choosing a new Pokemon, with no egg. scene_v2.h includes this at its end, so it's only there with
// UI_V2 on. Fedor: the four starters for the first game and after every ending until a goodbye has been offered
// (a final form on its 4th day); from then on any base form in the Pokedex (pet.canPickFromDex()).

// ----- the question that slides up from the bottom: "Choose PIKACHU?" YES / BACK (Fedor's "are you sure?") -----
#define V2_SQ_Y 330    // its panel when it's up: 92..374 x 330..414
#define V2_SQ_MS 220   // its slide, up or down
#define V2_SQ_BTN_Y (V2_SQ_Y + 36)
#define V2_SQ_BTN_H 40
static const int16_t V2_SQ_BTN_X[2][2] = { { 112, 226 }, { 240, 354 } };  // YES, BACK
struct V2Ask {
  bool up = false;
  uint32_t upT = 0, downT = 0;  // when it started up, and down
};
// how far up it is: 0 down (off the screen), 1 up; eased, for drawing
static float v2AskLin(const V2Ask &a) {
  uint32_t now = millis();
  float k = a.up ? (now - a.upT) / (float)V2_SQ_MS : 1.0f - (now - a.downT) / (float)V2_SQ_MS;
  return k < 0 ? 0 : k > 1 ? 1 : k;
}
static float v2AskK(const V2Ask &a) {
  float k = v2AskLin(a);
  return 1.0f - (1.0f - k) * (1.0f - k);
}
static void v2AskOpen(V2Ask &a) {  // (from where it is, if it was on its way down)
  if (a.up) return;
  a.upT = millis() - (uint32_t)(v2AskLin(a) * V2_SQ_MS);
  a.up = true;
}
static void v2AskClose(V2Ask &a) {
  if (!a.up) return;
  a.downT = millis() - (uint32_t)((1.0f - v2AskLin(a)) * V2_SQ_MS);
  a.up = false;
}
// its button under a tap once it's all the way up: 1 YES, 2 BACK, 0 none
static int v2AskBtnAt(const V2Ask &a, int16_t x, int16_t y) {
  if (!a.up || millis() - a.upT < V2_SQ_MS) return 0;
  if (y < V2_SQ_BTN_Y - 6 || y > V2_SQ_BTN_Y + V2_SQ_BTN_H + 6) return 0;
  for (int b = 0; b < 2; b++)
    if (x >= V2_SQ_BTN_X[b][0] - 4 && x <= V2_SQ_BTN_X[b][1] + 4) return b + 1;
  return 0;
}
static bool v2AskOnPanel(const V2Ask &a, int16_t x, int16_t y) {
  return a.up && x >= 92 && x <= 374 && y >= V2_SQ_Y && y <= V2_SQ_Y + 84;
}
static void v2AskDraw(const V2Ask &a, const char *name) {
  float k = v2AskK(a);
  if (k <= 0) return;
  int dy = (int)((1.0f - k) * (V2_W - V2_SQ_Y + 8));
  v2Panel(92, V2_SQ_Y + dy, 374, V2_SQ_Y + 84 + dy, 12);
  char q[48];
  snprintf(q, sizeof(q), T(S_CHOOSE_Q_FMT), name);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(q, 2), V2_SQ_Y + 12 + dy);
  printT(q);
  char yes[16];
  char back[24];
  v2LabelButton(V2_SQ_BTN_X[0][0], V2_SQ_BTN_Y + dy, V2_SQ_BTN_X[0][1], V2_SQ_BTN_Y + V2_SQ_BTN_H + dy,
                v2Caps(T(S_YES), yes, sizeof(yes)), C565(0x58, 0xb8, 0x68));
  v2LabelButton(V2_SQ_BTN_X[1][0], V2_SQ_BTN_Y + dy, V2_SQ_BTN_X[1][1], V2_SQ_BTN_Y + V2_SQ_BTN_H + dy,
                v2Caps(T(S_BACK_BTN), back, sizeof(back)), C565(0x8e, 0x8c, 0x96));
}

// ----- the starter screen (v2d-starter.gif, r29-starter-no-names.png, v2h-starter-tap.gif) -----
// The four starters walk side by side toward you through the Field while the world slides by (walk_v2.h's
// scene and clock). Tap one and all four stop, the world holds still, the one tapped hops with a ring at its
// feet and the question slides up. Tapping another switches to it; BACK, or a tap anywhere else, slides the
// question down and they walk on. YES makes it yours at once. A starter whose shiny form you own walks in
// shiny, sparkles and all, and is yours shiny.
static const int16_t V2_STARTERS[4] = { 1, 4, 7, 25 };  // Bulbasaur, Charmander, Squirtle, Pikachu
static const int16_t V2_STARTER_X[4] = { 100, 186, 276, 364 };
#define V2_STARTER_FEET 312
#define V2_STARTER_S 3  // their scale: 3x (Fedor: 2x looked small on the Knomi)

static PmdMon *v2StarterPmd = nullptr;  // the four sprites, while the screen is up
static int8_t v2StarterSel = -1;        // the one tapped, or -1: they walk
static int8_t v2StarterAsked = -1;      // the one the question names (it keeps it as it slides down)
static uint32_t v2StarterSelT = 0;      // when it was tapped: its hop
static V2Ask v2StarterQ;
static uint32_t v2ArriveT = 0;  // when a new one started walking in (0: none; v2Arrival() below)

static void v2StarterLoad() {
  if (v2StarterPmd) return;
  v2StarterPmd = new PmdMon[4];
  for (int k = 0; k < 4; k++) v2StarterPmd[k].load(V2_STARTERS[k], pet.hasForm(V2_STARTERS[k], true));
}
static void v2StarterFree() {
  if (!v2StarterPmd) return;
  for (int k = 0; k < 4; k++) v2StarterPmd[k].unload();
  delete[] v2StarterPmd;
  v2StarterPmd = nullptr;
  v2StarterSel = v2StarterAsked = -1;
  v2StarterQ = V2Ask();
}
// the starter under a tap (its sprite, generously), or -1
static int v2StarterAt(int16_t x, int16_t y) {
  for (int k = 0; k < 4; k++)
    if (abs(x - V2_STARTER_X[k]) <= 44 && y >= V2_STARTER_FEET - 120 && y <= V2_STARTER_FEET + 18) return k;
  return -1;
}
static int v2StarterBtnAt(int16_t x, int16_t y) { return v2AskBtnAt(v2StarterQ, x, y); }

static void v2StarterScreen() {
  v2StarterLoad();
  uint32_t now = millis();
  bool walking = v2StarterSel < 0;
  // the world's clock, as the home's walk keeps it: it runs only while they walk
  if (walking) {
    uint32_t dt = v2WalkLast ? now - v2WalkLast : 0;
    if (dt > 300) dt = 300;
    v2WalkMs += dt;
    v2WalkLast = now;
  } else {
    v2WalkLast = 0;
  }
  v2Walking = walking;
  PmdMon &lead = v2StarterPmd[0];
  v2Bob = walking && lead.has(PMD_WALKD) && (pmdFrameAt(lead.acts[PMD_WALKD], v2WalkT(), true) & 1) ? 2 : 0;
  uint8_t tod = v2HourTod();
  gNight = tod == 2;
  gV2Dark = false;
  if (!v2DrawWalkScene(Z_FIELD, tod)) v2DrawScene(Z_FIELD, tod);

  for (int k = 0; k < 4; k++) {
    PmdMon &m = v2StarterPmd[k];
    if (!m.loaded) continue;
    int x = V2_STARTER_X[k], feet = V2_STARTER_FEET + v2Bob;
    bool sel = k == v2StarterSel;
    if (sel) {  // a ring of dark cells round its feet, turning slowly
      scScreen();
      scGradeTod = 0;
      for (int i = 0; i < 12; i++) {
        float a = i * 0.5235988f + now * 0.0012f;
        int cx = x + (int)(cosf(a) * 44), cy = feet + 2 + (int)(sinf(a) * 12);
        scGridRect(cx - 2, cy - 2, cx + 2, cy + 2, 0x1B1B25);
      }
    }
    v2PetShadow(x, feet - 2, 34);
    uint8_t act = PMD_IDLE;
    uint32_t t = now + k * 230;  // standing, out of step
    int hop = 0;
    if (walking && m.has(PMD_WALKD)) {
      act = PMD_WALKD;
      t = v2WalkT() + k * 170;
    } else if (sel) {  // little hops on the spot (its own Hop rises far too high at 3x)
      t = now - v2StarterSelT;
      hop = SNAP2((int)(fabsf(sinf(t * 0.00698f)) * 16));
    }
    gPmdForceScale = V2_STARTER_S;
    drawPmdActM(m, act, x, feet - hop, t, true, false, V2_STARTER_S);
    gPmdForceScale = 0;
  }

  char b[48];
  v2Sign(v2Caps(T(S_CHOOSE_STARTER), b, sizeof(b)), 36);
  v2SmallPanel(T(S_TAP_ONE), 76);
  if (v2StarterAsked >= 0) v2AskDraw(v2StarterQ, dexName(V2_STARTERS[v2StarterAsked]));
}

static void v2StarterTap(int16_t x, int16_t y) {
  int bt = v2StarterBtnAt(x, y);
  if (bt == 1) {  // YES: it's yours
    sfxPlay(SFX_TAP);
    int k = v2StarterSel;
    bool shiny = v2StarterPmd && v2StarterPmd[k].shiny;
    v2StarterFree();
    pet.chooseStarter(V2_STARTERS[k], shiny);
    v2ArriveT = millis();  // it walks in (v2Arrival())
    return;
  }
  int k = v2StarterAt(x, y);
  if (bt == 0 && k < 0 && v2AskOnPanel(v2StarterQ, x, y)) return;  // the question's panel itself: nothing
  if (k >= 0 && bt == 0) {  // a starter: it's the one asked about
    if (k == v2StarterSel) return;
    sfxPlay(SFX_TAP);
    v2StarterSel = v2StarterAsked = k;
    v2StarterSelT = millis();
    v2AskOpen(v2StarterQ);
    return;
  }
  if (v2StarterSel >= 0) {  // BACK, or anywhere else: they walk on
    sfxPlay(SFX_TAP);
    v2StarterSel = -1;
    v2AskClose(v2StarterQ);
  }
}

// ----- the Pokedex pick -----
// The Pokedex's own look (the dimmed room, the board, the squares in a white panel, the page buttons), with only
// the forms you can raise: base forms you have, each form on its own (a shiny one sparkles), each with its stars
// (they set its genes). No X on the grid (one has to be chosen), so the panel and its page line sit in the
// middle of the room between the board and the screen's bottom (Fedor). Tap one for its entry: it turns round as
// in the Pokedex, with its type under it (not the level it evolves at: Fedor), CHOOSE and the X back to the grid;
// CHOOSE asks the question, and YES makes it yours at once.
#define V2_PICK_BTN_Y 316  // CHOOSE: 136..330 x 316..360, between the entry's panel and the X
#define V2_PICK_BTN_H 44
static V2Ask v2PickQ;

static int v2PickPages() { return (pet.eggChoiceCount() + DEXG_PAGE - 1) / DEXG_PAGE; }
// the grid's panel: its top, centred with the page line (if there's more than one page) between the board and
// the bottom; its squares start 6 px in, and the page buttons are level with its middle
#define V2_PICK_PANEL_H (3 * DEXG_CELL + 12)
static int v2PickPanelY() {
  int top = V2_BOARD_Y + 28, h = V2_PICK_PANEL_H + (v2PickPages() > 1 ? 28 : 0);
  return top + (V2_W - top - h) / 2;
}
static int v2PickGridY() { return v2PickPanelY() + 6; }
static int v2PickArrowY() { return v2PickPanelY() + V2_PICK_PANEL_H / 2; }
static void v2PickOpen(int16_t dex, bool shiny) {
  eggPickDetail = dex;
  eggPickShiny = shiny;
  galleryPmd.load(dex, shiny);
  galleryShinyPmd.unload();
  dexArtCount = 0;
  dexPatSlot = -1;
  v2PickQ = V2Ask();
}
static void v2PickBack() {
  eggPickDetail = 0;
  galleryUnload();
  v2PickQ = V2Ask();
}
// on the entry: 1 YES, 2 BACK (the question's), 3 CHOOSE, 4 the X; 0 none
static int v2PickBtnAt(int16_t x, int16_t y) {
  if (!eggPickDetail) return 0;
  if (v2PickQ.up) return v2AskBtnAt(v2PickQ, x, y);
  if (y >= V2_PICK_BTN_Y - 4 && y <= V2_PICK_BTN_Y + V2_PICK_BTN_H + 4 && abs(x - CX) <= 101) return 3;
  return closeHit(x, y) ? 4 : 0;
}

static void v2PickScreen() {
  v2MenuBackdrop();
  if (eggPickDetail) {  // the entry: the board with its name and number, it turning round, CHOOSE
    v2DexBoard(eggPickDetail, true);
    uint8_t stars = pet.starsOf(eggPickDetail, eggPickShiny);
    if (galleryPmd.loaded) {
      drawDexArt(galleryPmd, stars, nullptr, 0);
    } else if (const uint8_t *t = thumbs.getForm(eggPickDetail, eggPickShiny)) {
      int box[4];
      drawThumbFit(t, CX, 160, 200, 120, 4, false, 0, box);
      if (eggPickShiny) drawSparkles(box[0], box[1], box[2], box[3], millis());
      drawStars(CX, 237, 9, stars);  // (DEX_STAR_R's size)
    }
    v2TypeChip(eggPickDetail);
    if (!v2PickQ.up && v2AskK(v2PickQ) <= 0) {  // CHOOSE and the X, unless the question is up (or on its way)
      char b[24];
      v2LabelButton(CX - 97, V2_PICK_BTN_Y, CX + 97, V2_PICK_BTN_Y + V2_PICK_BTN_H, v2Caps(T(S_CHOOSE_BTN), b, sizeof(b)),
                    C565(0x58, 0xb8, 0x68), UI_WHITE, 3);
      v2CloseX();
    }
    v2AskDraw(v2PickQ, dexName(eggPickDetail));
    return;
  }
  char b[40];
  v2Sign(v2Caps(T(S_CHOOSE_POKEMON), b, sizeof(b)));
  int pages = v2PickPages(), gy = v2PickGridY();
  if (eggPickPage >= pages) eggPickPage = pages ? pages - 1 : 0;
  v2Panel(DEXG_X - 6, gy - 6, DEXG_X + 3 * DEXG_CELL + 6, gy + 3 * DEXG_CELL + 6, 10);
  for (int i = 0; i < DEXG_PAGE; i++) {
    bool shiny;
    int16_t dex = pet.eggChoiceAt(eggPickPage * DEXG_PAGE + i, shiny);
    if (!dex) break;
    int x = DEXG_X + (i % DEXG_COLS) * DEXG_CELL, y = gy + (i / DEXG_COLS) * DEXG_CELL;
    int cx = x + DEXG_CELL / 2, cy = y + DEXG_CELL / 2;
    if (const uint8_t *t = thumbs.getForm(dex, shiny)) {
      int box[4];
      drawThumbFit(t, cx, cy, DEXG_CELL - 6, DEXG_CELL - 4, DEXG_SPRITE_S, false, DEXG_STARS_H / 2, box);
      if (shiny) drawSparkles(box[0], box[1], box[2], box[3], millis() + i * 337);
    }
    drawStars(cx, y + DEXG_CELL - DEXG_STAR_R - 3, DEXG_STAR_R, pet.starsOf(dex, shiny));
  }
  if (pages > 1) {
    drawDexGridPage(eggPickPage, pages, gy + 3 * DEXG_CELL + 18);  // (12 px under the panel: clear of its rim)
    drawPageArrows(eggPickPage, pages, v2PickArrowY());
  }
}

static void v2PickTap(int16_t x, int16_t y) {
  if (eggPickDetail) {
    int b = v2PickBtnAt(x, y);
    if (b == 1) {  // YES: it's yours
      sfxPlay(SFX_TAP);
      int16_t dex = eggPickDetail;
      bool shiny = eggPickShiny;
      v2PickBack();
      eggPickPage = 0;
      if (pet.chooseEgg(dex, shiny)) v2ArriveT = millis();  // it walks in (v2Arrival())
      return;
    }
    if (b == 3) {  // CHOOSE: are you sure?
      sfxPlay(SFX_TAP);
      v2AskOpen(v2PickQ);
      return;
    }
    if (b == 4) {  // the X: back to the grid
      sfxPlay(SFX_TAP);
      v2PickBack();
      return;
    }
    if (v2PickQ.up) {  // BACK, or anywhere off the question: it goes
      if (b == 2 || !v2AskOnPanel(v2PickQ, x, y)) {
        sfxPlay(SFX_TAP);
        v2AskClose(v2PickQ);
      }
      return;
    }
    dexArtPet(x, y);  // its sprite: a pat, as in the Pokedex
    return;
  }
  if (int d = pageArrowAt(x, y, eggPickPage, v2PickPages(), v2PickArrowY())) {
    sfxPlay(SFX_TAP);
    eggPickPage += d;
    return;
  }
  int i = dexGridAt(x, y, DEXG_X, v2PickGridY(), DEXG_CELL);
  if (i < 0) return;
  bool shiny;
  int16_t dex = pet.eggChoiceAt(eggPickPage * DEXG_PAGE + i, shiny);
  if (!dex) return;
  sfxPlay(SFX_TAP);
  v2PickOpen(dex, shiny);
}

// ----- a goodbye: it walks away up the hill and over it (r11-farewell.png); a new one walks in -----
// Fedor: "the old pokemon walks backwards into the scene behind the hills", and the new one "walks in from a
// distance". A goodbye (or a release) is the home's scene at sunset, with the name and level over it and the
// panel and icons gone: over the ceremony's 10 s it thanks you (a heart pops up over it), turns its back, and
// walks away up the near hill, smaller and smaller, then over the crest, where the hill hides it as it goes down
// the far side. Hearts float up and a panel says goodbye ("Thank you for 4 days"). A runaway goes the same way
// at the hour's light, in the rain, head down first, and the panel says it ran away. A new Pokemon comes the
// other way: up from behind the crest, toward you, bigger and bigger, to its place, and the home goes on.
#define V2_CREST_Y 276     // the near hill's top at the middle (V2_HILL_NEAR, on the 4 px grid)
#define V2_ARRIVE_MS 3600
// v2ArriveT (declared with the starters): when a new one started walking in (0: none)

// its scale at home, as drawPetPMD() gets it from drawPmdActM()
static uint8_t v2HomeScale(PmdMon &m) {
  uint8_t mx = partyScale(partyOthers(), true), s = m.acts[PMD_IDLE].h ? 170 / m.acts[PMD_IDLE].h : 5;
  if (s < 2) s = 2;
  return s > mx ? mx : s;
}
// a Pokemon at (x, feet), scale s; behind: past the crest, so the hill hides what's under its top line (what
// was there before it was drawn is put back, grass and pines and all)
static uint16_t *v2CrestKeep = nullptr;
#define V2_CREST_KEEP_W 160
#define V2_CREST_KEEP_H 96
static void v2CrestPet(PmdMon &m, uint8_t act, int x, int feet, int s, uint32_t t, bool behind) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  int x0 = x - V2_CREST_KEEP_W / 2, h = feet + 4 - V2_CREST_Y;
  if (h > V2_CREST_KEEP_H) h = V2_CREST_KEEP_H;
  if (behind && h > 0 && !v2CrestKeep) v2CrestKeep = (uint16_t *)ps_malloc(V2_CREST_KEEP_W * V2_CREST_KEEP_H * sizeof(uint16_t));
  bool keep = behind && h > 0 && v2CrestKeep && x0 >= 0 && x0 + V2_CREST_KEEP_W <= V2_W;
  if (keep)
    for (int r = 0; r < h; r++) memcpy(v2CrestKeep + r * V2_CREST_KEEP_W, fb + (V2_CREST_Y + r) * V2_W + x0, V2_CREST_KEEP_W * 2);
  if (!behind) v2PetShadow(x, feet - 4, 6 + 9 * s);
  gPmdForceScale = s;
  drawPmdActM(m, m.has(act) ? act : PMD_IDLE, x, feet, t, true, false, s);
  gPmdForceScale = 0;
  if (keep)
    for (int r = 0; r < h; r++) memcpy(fb + (V2_CREST_Y + r) * V2_W + x0, v2CrestKeep + r * V2_CREST_KEEP_W, V2_CREST_KEEP_W * 2);
}
// a pixel heart, its top left at (x, y)
static void v2PixHeart(int x, int y, int cell, Rgb col) {
  static const char *const H[6] = { ".pp.pp.", "ppppppp", "ppppppp", ".ppppp.", "..ppp..", "...p..." };
  const ScKey k[1] = { { 'p', col } };
  scSprite(H, 6, x, y, cell, k, 1);
}

static void v2Departure() {
  if (!pmd.loaded) return;
  uint32_t now = millis();
  float t = pet.ceremonyT();
  bool ranAway = pet.ceremony == CER_RUNAWAY;
  int s0 = v2HomeScale(pmd), h1 = pmd.acts[PMD_WALKU].h ? pmd.acts[PMD_WALKU].h : pmd.acts[PMD_IDLE].h;
  scScreen();
  scGradeTod = 0;
  if (ranAway) {  // rain, slanting down the screen
    for (int i = 0; i < 44; i++) {
      int rx = (i * 47 + now / 3) % 470, ry = (i * 91 + now / 2) % 480 - 12;
      scGridRect(rx, ry, rx + 4, ry + 12, 0x8AB4E0);
    }
  } else {  // hearts floating up, swaying, over the sky
    static const int16_t HX[6] = { 120, 300, 190, 350, 90, 250 };
    for (int i = 0; i < 6; i++) {
      int y = 250 - (int)((now / 12 + i * 61) % 170), x = HX[i] + (int)(8 * sinf(now * 0.003f + i));
      v2PixHeart(x, y, 3, i % 2 ? 0xF080A0 : 0xE85878);
    }
  }
  // where it is: thanking you, turning its back, walking up the hill, then over the crest
  const float T1 = ranAway ? 0.20f : 0.14f, T2 = T1 + 0.08f, T3 = 0.70f, T4 = 0.88f;
  uint8_t act = PMD_IDLE;
  int feet = PET_GROUND, s = s0, x = CX;
  bool behind = false;
  uint32_t at = now;
  if (t < T1) {
    if (ranAway) {  // head down, trembling
      act = PMD_HURT;
      x = CX + (int)(4 * sinf(now * 0.04f));
    }
  } else if (t < T2) {  // it turns its back: the views from facing you round to facing away
    static const uint8_t TURN[5] = { PMD_WALKD, PMD_WALKDL, PMD_WALKL, PMD_WALKUL, PMD_WALKU };
    act = TURN[(int)((t - T1) / (T2 - T1) * 5) % 5];
    at = 0;
  } else if (t < T3) {  // up the hill, smaller and smaller
    float u = (t - T2) / (T3 - T2);
    act = PMD_WALKU;
    s = s0 - (int)(u * (s0 - 1) + 0.5f);
    feet = PET_GROUND - (int)(u * (PET_GROUND - V2_CREST_Y + 4));
  } else if (t < T4) {  // over the crest and down the far side, out of sight
    float u = (t - T3) / (T4 - T3);
    act = PMD_WALKU;
    s = 1;
    feet = V2_CREST_Y - 4 + (int)(u * (h1 + 8));
    behind = true;
  } else {
    act = 0xFF;  // gone
  }
  if (act != 0xFF) v2CrestPet(pmd, act, x, feet, s, at, behind);
  if (!ranAway && t < T1) {  // its thank-you heart, popping up over its head
    int hy = feet - pmd.acts[PMD_IDLE].h * s0 - 30 - (int)(t / T1 * 24);
    v2PixHeart(CX - 14, hy, 4, 0xE85878);
  }
  // the panel: goodbye, and how long you had it (counting the day it's on); or that it ran away
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  v2Panel(96, 312, 370, 380, 12);
  setSize(2);
  if (ranAway) {
    gfx->setTextColor(UI_INK);
    setCur(centerX(T(S_RUNAWAY), 2), 338);
    printT(T(S_RUNAWAY));
    return;
  }
  char a[40];
  char b[40];
  snprintf(a, sizeof(a), T(S_VISIT_BYE_FMT), nm);
  snprintf(b, sizeof(b), T(S_THANKS_DAYS_FMT), (unsigned)(1 + pet.ageMinutes / 1440));
  gfx->setTextColor(UI_INK);
  setCur(centerX(a, 2), 326);
  printT(a);
  gfx->setTextColor(V2_INK_MUTED);
  setCur(centerX(b, 2), 350);
  printT(b);
}

// a new one walking in; false once it's in its place (or with nothing to walk in)
static bool v2Arrival() {
  if (!v2ArriveT) return false;
  uint32_t e = millis() - v2ArriveT;
  if (e >= V2_ARRIVE_MS || pet.isEgg()) {
    v2ArriveT = 0;
    return false;
  }
  if (!pmd.loaded) return false;  // (its sprite is still on its way: the walk starts when it's there)
  float u = e / (float)V2_ARRIVE_MS;
  int s0 = v2HomeScale(pmd), h1 = pmd.acts[PMD_WALKD].h ? pmd.acts[PMD_WALKD].h : pmd.acts[PMD_IDLE].h;
  if (u < 0.25f) {  // up from behind the crest
    float k = u / 0.25f;
    v2CrestPet(pmd, PMD_WALKD, CX, V2_CREST_Y - 4 + (int)((1 - k) * (h1 + 8)), 1, v2WalkT() + e, true);
  } else {  // toward you, bigger and bigger
    float k = (u - 0.25f) / 0.75f;
    int s = 1 + (int)(k * (s0 - 1) + 0.5f), feet = V2_CREST_Y - 4 + (int)(k * (PET_GROUND - V2_CREST_Y + 4));
    v2CrestPet(pmd, PMD_WALKD, CX, feet, s, e, false);
  }
  return true;
}
