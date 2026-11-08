// V2: the Repel (Fedor). scene_v2.h includes this at its end, so it's only there with UI_V2 on.
//
// A spray in the backpack, beside the berries and the candy. Used, it runs for REPEL_MIN of the game's minutes
// (pet.useRepel()): wild encounters keep weaker Pokemon away (none below your level, nor any species whose base stats
// add up to less than yours), and in a Travel zone not yet complete every non-legendary there is as likely; in a
// complete zone it works as in the wild. Shiny odds don't change. Earned by playing minigames: every second of one
// played (not its how-to card nor its end) counts, REPEL_PLAY_S of them for a Repel. They stack, as Master Balls do.
// A game's end shows how far along the next one is; while one runs, the bag on the home screen wears a purple
// badge with the minutes left.

// ----- the spray: a purple can, a white label with a pink stripe, a grey cap and nozzle, a puff of mist -----
static const char *const SPR_V2_REPEL[18] = {
  "......kkk.........",
  ".....kSsSk...w.w..",
  ".....kSsSk....w...",
  "....kkkkkkk..w.w..",
  "...kVvvvvvVk......",
  "..kVhvvvvvvVk.....",
  "..khvvvvvvvVk.....",
  "..kwwwwwwwwwk.....",
  "..kwmmmmmmmwk.....",
  "..kwwwwwwwwwk.....",
  "..khvvvvvvvVk.....",
  "..khvvvvvvvVk.....",
  "..khvvvvvvvVk.....",
  "..kVvvvvvvvVk.....",
  "..kSSSSSSSSSk.....",
  "...kkkkkkkkk......",
  "..................",
  "..................",
};
static void v2RepelIcon(int x, int y, int s) {  // its top left at (x, y), cells of s px
  static const ScKey K[8] = { { 'k', 0x1B1B25 }, { 'S', 0x787E8C }, { 's', 0xC8CCD4 }, { 'V', 0x5A3A8C },
                              { 'v', 0x8A5CC8 }, { 'h', 0xA888E0 }, { 'w', 0xFFFFFF }, { 'm', 0xE85AA8 } };
  scScreen();
  scGradeTod = 0;
  scSprite(SPR_V2_REPEL, 18, x, y, s, K, 8);
}

// ----- earning: every second of a minigame played -----
static uint32_t v2RepelNewT = 0;  // when one was last earned (its moment on that game's end)
// a minigame being played right now: open, its card gone, not ended, not waiting for a buddy's READY
static bool v2PlayingGame() {
  if (introUp()) return false;
  if (gameOpen) return !gameOverUntil;
  if (quizOpen) return quizPhase != QP_END;
  if (runOpen) return !runOverT;
  if (dungeonOpen) return !dnOverT;
  if (surfOpen) return surfDuel ? buddyGameReady() && !surfDuelDoneT : !surfOverT;
  if (sdOpen) return sdPhase == SD_ASK || sdPhase == SD_REVEAL;
  if (vbOpen) return vbPhase != VB_READY && vbPhase != VB_DONE;
  return false;
}
static void v2RepelTick(uint32_t now) {  // once a loop
  static uint32_t last = 0;
  uint32_t dt = last ? now - last : 0;
  last = now;
  if (dt > 250) dt = 250;  // (a stall isn't play)
  if (dt && v2PlayingGame() && pet.repelPlay(dt)) v2RepelNewT = now ? now : 1;
}
static bool v2RepelFresh() { return v2RepelNewT && millis() - v2RepelNewT < 60000; }  // earned in this game

// ----- a game's end: the spray, how many you hold, the time to the next; or "REPEL!" for the one just earned -----
static void v2RepelRow(int y) {
  v2Panel(126, y, 340, y + 32, 10);
  v2RepelIcon(132, y - 2, 2);
  bool fresh = v2RepelFresh();
  uint8_t held = pet.repelHeld;
  if (held) drawCountBadge(164, y + 2, held);
  setSize(2);
  if (fresh) {  // it's yours: its name, gold, with sparkles beside
    char b[32];
    snprintf(b, sizeof(b), "%s!", T(S_REPEL));
    char c[32];
    v2Caps(b, c, sizeof(c));
    gfx->setTextColor(C565(0xc0, 0x84, 0x10));
    setCur(190 + (146 - textW(c, 2)) / 2, y + 9);
    printT(c);
    if ((millis() / 300) % 2) {
      scGridRect(330, y - 8, 334, y + 4, 0xFFD95E);
      scGridRect(326, y - 4, 338, y, 0xFFD95E);
    }
    return;
  }
  uint16_t sec = pet.repelPlaySec;
  char b[16];
  snprintf(b, sizeof(b), "%u:%02u/%u:%02u", sec / 60, sec % 60, REPEL_PLAY_S / 60, REPEL_PLAY_S % 60);
  bool cjk = classicFontBegin();
  setSize(2);
  int tw = textW(b, 2), bx = 182, bw = 334 - bx - tw - 8;
  gfx->fillRoundRect(bx, y + 9, bw, 15, 6, C565(0xd8, 0xd2, 0xbd));
  int fw = (bw - 4) * sec / REPEL_PLAY_S;
  if (fw > 2) gfx->fillRoundRect(bx + 2, y + 11, fw, 11, 5, C565(0x8a, 0x5c, 0xc8));
  gfx->setTextColor(V2_INK_MUTED);
  setCur(334 - tw, y + 9);
  printT(b);
  classicFontEnd(cjk);
}

// ----- the card's Items page (Fedor): the Master Ball and the Repel, each with how it's earned and its bar -----
// a bar across the panel, k of it filled in col, its words (a count, or a time) at its right
static void v2ItemBar(int y, float k, const char *txt, uint16_t col) {
  bool cjk = classicFontBegin();
  setSize(2);
  int tw = textW(txt, 2), bx = 116, bw = 350 - bx - tw - 10;
  gfx->fillRoundRect(bx, y, bw, 15, 6, C565(0xd8, 0xd2, 0xbd));
  int fw = (int)((bw - 4) * (k < 0 ? 0 : k > 1 ? 1 : k));
  if (fw > 2) gfx->fillRoundRect(bx + 2, y + 2, fw, 11, 5, col);
  gfx->setTextColor(V2_INK_MUTED);
  setCur(350 - tw, y);
  printT(txt);
  classicFontEnd(cjk);
}
static void v2ItemHead(const char *name, const char *how, int y) {  // its name, then how to earn it (muted)
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(name, 2), y);
  printT(name);
  gfx->setTextColor(V2_INK_MUTED);
  setCur(centerX(how, 2), y + 22);
  printT(how);
}
static void v2CardItems() {
  char b[40];
  v2Sign(v2Caps(T(S_ITEMS), b, sizeof(b)));
  v2Panel(92, 70, 374, 372, 10);
  char t[16];
  // the Master Ball: how many you hold, the fights to the next
  v2MasterBall(CX - 27, 82, 3);
  if (pet.masterBalls) drawCountBadge(CX + 26, 88, pet.masterBalls);
  snprintf(b, sizeof(b), T(S_MASTER_EARN_FMT), (unsigned)MASTER_FIGHTS);
  v2ItemHead(T(S_MASTER), b, 142);
  snprintf(t, sizeof(t), "%u/%u", pet.masterBar, MASTER_FIGHTS);
  v2ItemBar(188, pet.masterBar / (float)MASTER_FIGHTS, t, v2MasterInk('q'));
  gfx->fillRect(112, 218, 242, 2, C565(0xe4, 0xea, 0xf2));
  // the Repel: how many you hold, the play to the next; while one runs, its minutes left
  v2RepelIcon(CX - 27, 228, 3);
  if (pet.repelHeld) drawCountBadge(CX + 26, 234, pet.repelHeld);
  snprintf(b, sizeof(b), T(S_REPEL_EARN_FMT), (unsigned)(REPEL_PLAY_S / 60));
  v2ItemHead(T(S_REPEL), b, 288);
  uint16_t sec = pet.repelPlaySec;
  if (pet.repelOn()) {
    snprintf(t, sizeof(t), "%um", pet.repelMinutesLeft());
    v2ItemBar(334, pet.repelMinutesLeft() / (float)REPEL_MIN, t, C565(0x5a, 0x3a, 0x8c));
  } else {
    snprintf(t, sizeof(t), "%u:%02u/%u:%02u", sec / 60, sec % 60, REPEL_PLAY_S / 60, REPEL_PLAY_S % 60);
    v2ItemBar(334, sec / (float)REPEL_PLAY_S, t, C565(0x8a, 0x5c, 0xc8));
  }
}

// ----- the backpack: the Repel's seat, after the berries and the candy, while you hold one or one runs -----
static bool v2RepelSeat() { return pet.repelHeld || pet.repelOn(); }
// its seat (the bubble's own, centre (cx, cy)): the spray, how many you hold, and the minutes left while one runs
static void v2RepelSeatDraw(int cx, int cy) {
  v2RepelIcon(cx - 27, cy - 27, 3);
  if (pet.repelHeld) v2CountBadge(cx + 22, cy - 28, pet.repelHeld);
  if (pet.repelOn()) {
    char b[8];
    snprintf(b, sizeof(b), "%um", pet.repelMinutesLeft());
    bool cjk = classicFontBegin();
    setSize(2);
    int w = textW(b, 2) + 10;
    gfx->fillRoundRect(cx - w / 2, cy + 18, w, 18, 8, C565(0x5a, 0x3a, 0x8c));
    gfx->setTextColor(UI_WHITE);
    setCur(cx - textW(b, 2) / 2 + 1, cy + 20);
    printT(b);
    classicFontEnd(cjk);
  }
}
// ----- a food from the backpack (Fedor): it drops from the top onto your Pokemon, which hops for a second; the bag
// stays open for another -----
#define V2_DROP_MS 450
#define V2_HOP_MS 1000
static uint32_t v2DropT = 0;  // when the last one was tapped (0: none falling or hopping)
static uint8_t v2DropItem = 0;
static int v2DropX = CX;
static void v2FoodDropStart(int item) {
  v2DropItem = item;
  v2DropT = millis() ? millis() : 1;
  v2DropX = partyOthers() ? PARTY_MINE_X : (int)beh.x;
}
static int v2FeedHop() {  // how high your Pokemon is, hopping once a food has landed on it
  if (!v2DropT) return 0;
  uint32_t e = millis() - v2DropT;
  if (e < V2_DROP_MS || e >= V2_DROP_MS + V2_HOP_MS) return 0;
  return SNAP2((int)(fabsf(sinf((e - V2_DROP_MS) * 3.14159f / 500)) * 14));  // two hops
}
static void v2FoodDropDraw() {  // over everything, the bag too
  if (!v2DropT) return;
  uint32_t e = millis() - v2DropT;
  if (e >= V2_DROP_MS + V2_HOP_MS) {
    v2DropT = 0;
    return;
  }
  if (e >= V2_DROP_MS || !pmd.loaded) return;  // landed: eaten
  static const char *const *const FOOD[4] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_CANDY };
  float k = e / (float)V2_DROP_MS;
  int head = PET_GROUND - pmd.acts[PMD_IDLE].h * v2HomeScale(pmd) + 12;
  drawMap(FOOD[v2DropItem], HOME_ICON_N, v2DropX - 27, 30 + (int)((head - 84) * k * k), 3, false);  // falling faster and faster
}

// used from the backpack: it starts, and the home says for how long; one already running can't be doubled
static void v2RepelUse() {
  if (!pet.useRepel()) {
    sfxPlay(SFX_DENY);
    return;
  }
  sfxPlay(SFX_MEDAL);
  snprintf(buddyNote, sizeof(buddyNote), T(S_REPEL_ON_FMT), (unsigned)REPEL_MIN);
  buddyNoteUntil = millis() + 3000;
}
// the bag on the home screen, while one runs: a purple badge with the minutes left
static void v2RepelBagBadge(int cx, int cy) {
  if (!pet.repelOn()) return;
  char b[8];
  snprintf(b, sizeof(b), "%u", pet.repelMinutesLeft());
  gfx->fillCircle(cx, cy, 12, V2_RIM);
  gfx->fillCircle(cx, cy, 10, C565(0x8a, 0x5c, 0xc8));
  bool cjk = classicFontBegin();
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(cx - textW(b, 2) / 2 + 1, cy - 7);
  printT(b);
  classicFontEnd(cjk);
}
