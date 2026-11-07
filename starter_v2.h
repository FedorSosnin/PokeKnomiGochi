// V2 step 6: the starter screen (v2d-starter.gif, r29-starter-no-names.png, v2h-starter-tap.gif). scene_v2.h
// includes this at its end, so it's only there with UI_V2 on.
//
// The four starters walk side by side toward you through the Field while the world slides by (walk_v2.h's
// scene and clock). Tap one and all four stop, the world holds still, the one tapped hops with a ring at its
// feet and the question slides up from the bottom ("Choose PIKACHU?" YES / BACK). Tapping another switches to
// it; BACK, or a tap anywhere else, slides the question down and they walk on. YES makes it yours at once (no
// egg). A starter whose shiny form you own walks in shiny, sparkles and all, and is yours shiny.
// It comes up for the first game, and after every ending until a goodbye has been offered (pet.canPickFromDex()).

static const int16_t V2_STARTERS[4] = { 1, 4, 7, 25 };  // Bulbasaur, Charmander, Squirtle, Pikachu
static const int16_t V2_STARTER_X[4] = { 100, 186, 276, 364 };
#define V2_STARTER_FEET 312
#define V2_SQ_Y 330    // the question's panel when it's up: 92..374 x 330..414
#define V2_SQ_MS 220   // its slide, up or down
#define V2_SQ_BTN_Y (V2_SQ_Y + 36)
#define V2_SQ_BTN_H 40
static const int16_t V2_SQ_BTN_X[2][2] = { { 112, 226 }, { 240, 354 } };  // YES, BACK

static PmdMon *v2StarterPmd = nullptr;  // the four sprites, while the screen is up
static int8_t v2StarterSel = -1;        // the one tapped, or -1: they walk
static int8_t v2StarterAsked = -1;      // the one the question names (it keeps it as it slides down)
static uint32_t v2StarterSelT = 0;      // when it was tapped: its hop
static uint32_t v2StarterUpT = 0;       // when the question started up
static uint32_t v2StarterDownT = 0;     // when it started down

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
}

// how far up the question is: 0 down (off the screen), 1 up; and eased, for drawing
static float v2StarterQLin() {
  uint32_t now = millis();
  float k = v2StarterSel >= 0 ? (now - v2StarterUpT) / (float)V2_SQ_MS : 1.0f - (now - v2StarterDownT) / (float)V2_SQ_MS;
  return k < 0 ? 0 : k > 1 ? 1 : k;
}
static float v2StarterQK() {
  float k = v2StarterQLin();
  return 1.0f - (1.0f - k) * (1.0f - k);
}
// the starter under a tap (its sprite, generously), or -1
static int v2StarterAt(int16_t x, int16_t y) {
  for (int k = 0; k < 4; k++)
    if (abs(x - V2_STARTER_X[k]) <= 42 && y >= V2_STARTER_FEET - 100 && y <= V2_STARTER_FEET + 18) return k;
  return -1;
}
// the question's button under a tap once it's all the way up: 1 YES, 2 BACK, 0 none
static int v2StarterBtnAt(int16_t x, int16_t y) {
  if (v2StarterSel < 0 || millis() - v2StarterUpT < V2_SQ_MS) return 0;
  if (y < V2_SQ_BTN_Y - 6 || y > V2_SQ_BTN_Y + V2_SQ_BTN_H + 6) return 0;
  for (int b = 0; b < 2; b++)
    if (x >= V2_SQ_BTN_X[b][0] - 4 && x <= V2_SQ_BTN_X[b][1] + 4) return b + 1;
  return 0;
}

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
        int cx = x + (int)(cosf(a) * 38), cy = feet + 2 + (int)(sinf(a) * 10);
        scGridRect(cx - 2, cy - 2, cx + 2, cy + 2, 0x1B1B25);
      }
    }
    v2PetShadow(x, feet - 2, 26);
    uint8_t act = PMD_IDLE;
    uint32_t t = now + k * 230;  // standing, out of step
    int hop = 0;
    if (walking && m.has(PMD_WALKD)) {
      act = PMD_WALKD;
      t = v2WalkT() + k * 170;
    } else if (sel) {  // little hops on the spot (its own Hop rises far too high at 2x)
      t = now - v2StarterSelT;
      hop = SNAP2((int)(fabsf(sinf(t * 0.00698f)) * 12));
    }
    gPmdForceScale = 2;
    drawPmdActM(m, act, x, feet - hop, t, true, false, 2);
    gPmdForceScale = 0;
  }

  char b[48];
  v2Sign(v2Caps(T(S_CHOOSE_STARTER), b, sizeof(b)), 36);
  v2SmallPanel(T(S_TAP_ONE), 76);

  float k = v2StarterQK();  // the question, sliding up from the bottom (and down again)
  if (k > 0 && v2StarterAsked >= 0) {
    int dy = (int)((1.0f - k) * (V2_W - V2_SQ_Y + 8));
    v2Panel(92, V2_SQ_Y + dy, 374, V2_SQ_Y + 84 + dy, 12);
    char q[48];
    snprintf(q, sizeof(q), T(S_CHOOSE_Q_FMT), dexName(V2_STARTERS[v2StarterAsked]));
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
}

static void v2StarterTap(int16_t x, int16_t y) {
  uint32_t now = millis();
  int bt = v2StarterBtnAt(x, y);
  if (bt == 1) {  // YES: it's yours
    sfxPlay(SFX_TAP);
    int k = v2StarterSel;
    bool shiny = v2StarterPmd && v2StarterPmd[k].shiny;
    v2StarterFree();
    pet.chooseStarter(V2_STARTERS[k], shiny);
    return;
  }
  int k = v2StarterAt(x, y);
  bool onPanel = v2StarterSel >= 0 && x >= 92 && x <= 374 && y >= V2_SQ_Y && y <= V2_SQ_Y + 84;
  if (bt == 0 && k < 0 && onPanel) return;  // the question's panel itself: nothing
  if (k >= 0 && bt == 0) {  // a starter: it's the one asked about
    if (k == v2StarterSel) return;
    sfxPlay(SFX_TAP);
    if (v2StarterSel < 0) v2StarterUpT = now - (uint32_t)(v2StarterQLin() * V2_SQ_MS);  // (from where it is, if it was going down)
    v2StarterSel = v2StarterAsked = k;
    v2StarterSelT = now;
    return;
  }
  if (v2StarterSel >= 0) {  // BACK, or anywhere else: they walk on
    sfxPlay(SFX_TAP);
    v2StarterDownT = now - (uint32_t)((1.0f - v2StarterQLin()) * V2_SQ_MS);
    v2StarterSel = -1;
  }
}
