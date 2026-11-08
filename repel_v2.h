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

// ----- the card's Progress page: under the Master Ball's bar, the spray, how many, the play to the next -----
static void v2RepelCardRow(int x, int y) {
  v2RepelIcon(x + 2, y, 2);
  if (pet.repelHeld) drawCountBadge(x + 34, y + 4, pet.repelHeld);
  uint16_t sec = pet.repelPlaySec;
  char b[16];
  if (pet.repelOn()) snprintf(b, sizeof(b), "%um", pet.repelMinutesLeft());  // one running: its minutes left
  else snprintf(b, sizeof(b), "%u:%02u", sec / 60, sec % 60);
  bool cjk = classicFontBegin();
  setSize(2);
  int tw = textW(b, 2), bx = x + 50, bw = 244 - 50 - tw - 10;
  gfx->fillRoundRect(bx, y + 11, bw, 15, 6, C565(0xd8, 0xd2, 0xbd));
  int fw = pet.repelOn() ? (bw - 4) * pet.repelMinutesLeft() / REPEL_MIN : (bw - 4) * sec / REPEL_PLAY_S;  // running: its time left
  if (fw > 2) gfx->fillRoundRect(bx + 2, y + 13, fw, 11, 5, C565(0x8a, 0x5c, 0xc8));
  gfx->setTextColor(pet.repelOn() ? C565(0x5a, 0x3a, 0x8c) : V2_INK_MUTED);
  setCur(x + 244 - tw, y + 11);
  printT(b);
  classicFontEnd(cjk);
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
