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
// its top left at (x, y), cells of s px; the can itself is 1.5 cells left of the square's middle and 1 cell up
// (the mist is on its right), so one meant to sit in the middle goes at v2RepelIcon(cx - 7.5 s, cy - 8 s)
static void v2RepelIcon(int x, int y, int s) {
  static const ScKey K[8] = { { 'k', 0x1B1B25 }, { 'S', 0x787E8C }, { 's', 0xC8CCD4 }, { 'V', 0x5A3A8C },
                              { 'v', 0x8A5CC8 }, { 'h', 0xA888E0 }, { 'w', 0xFFFFFF }, { 'm', 0xE85AA8 } };
  scScreen();
  scGradeTod = 0;
  scSprite(SPR_V2_REPEL, 18, x, y, s, K, 8);
}

// ----- earning: every second of a minigame played -----
// Played means someone's playing: a touch, a tilt past the dead zone or a key held in the last V2_PLAY_IDLE_MS
// (Fedor: sitting in the dungeon without moving doesn't count)
#define V2_PLAY_IDLE_MS 5000
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
#ifdef BOARD_HOST
  int8_t kx, ky;
  if (hostArrowDir(&kx, &ky)) v2PlayInputAt = now;  // an arrow key held (walking in the dungeon)
#endif
  bool playing = v2PlayInputAt && now - v2PlayInputAt < V2_PLAY_IDLE_MS;
  if (dt && playing && v2PlayingGame() && pet.repelPlay(dt)) v2RepelNewT = now ? now : 1;
}
static bool v2RepelFresh() { return v2RepelNewT && millis() - v2RepelNewT < 60000; }  // earned in this game

// ----- a game's end: the spray, how many you hold, the time to the next; or "REPEL!" for the one just earned -----
// as wide as the panel above it (x0..x1): the bar takes the rest of the row, the time on it (Fedor: it was too short
// to see fill)
static void v2RepelRow(int y, int x0, int x1) {
  v2Panel(x0, y, x1, y + 32, 10);
  v2RepelIcon(x0 + 6, y - 2, 2);
  bool fresh = v2RepelFresh();
  uint8_t held = pet.repelHeld;
  if (held) drawCountBadge(x0 + 38, y + 2, held);
  int bx = x0 + 58, bw = x1 - 9 - bx;  // the bar: past the badge to the panel's inner line
  setSize(2);
  if (fresh) {  // it's yours: its name, gold, with sparkles beside
    char b[32];
    snprintf(b, sizeof(b), "%s!", T(S_REPEL));
    char c[32];
    v2Caps(b, c, sizeof(c));
    gfx->setTextColor(C565(0xc0, 0x84, 0x10));
    setCur(bx + (bw - textW(c, 2)) / 2, y + 9);
    printT(c);
    if ((millis() / 300) % 2) {
      scGridRect(x1 - 10, y - 8, x1 - 6, y + 4, 0xFFD95E);
      scGridRect(x1 - 14, y - 4, x1 - 2, y, 0xFFD95E);
    }
    return;
  }
  uint16_t sec = pet.repelPlaySec;
  gfx->fillRoundRect(bx, y + 6, bw, 20, 7, C565(0xd8, 0xd2, 0xbd));
  int fw = (bw - 4) * sec / REPEL_PLAY_S;
  if (fw > 2) gfx->fillRoundRect(bx + 2, y + 8, fw, 16, 6, C565(0xa8, 0x88, 0xe0));
  char b[16];
  snprintf(b, sizeof(b), "%u:%02u/%u:%02u", sec / 60, sec % 60, REPEL_PLAY_S / 60, REPEL_PLAY_S % 60);
  bool cjk = classicFontBegin();
  setSize(2);
  gfx->setTextColor(UI_INK);
  setCur(bx + (bw - textW(b, 2)) / 2 + 1, y + 9);
  printT(b);
  classicFontEnd(cjk);
}

// ----- the card's item pages (Fedor): the Ultra Ball, the Master Ball and the Repel, each with what it does, how
// it's earned and its bar (the card's spacing: V2_CARD_* in menus_v2.h) -----
// a bar across the panel, k of it filled in col, its words (a count, or a time) at its right
static void v2ItemBar(int y, float k, const char *txt, uint16_t col) {
  bool cjk = classicFontBegin();
  setSize(2);
  int tw = textW(txt, 2);
  v2CardBar(V2_CARD_L, V2_CARD_R - tw - 12, y, k, col);
  gfx->setTextColor(V2_INK_MUTED);
  setCur(V2_CARD_R - tw, y - 1);
  printT(txt);
  classicFontEnd(cjk);
}
// the item hovering on its page (Fedor: "a slight bob up and down like they are hovering"): 6 px up and down, in
// the Knomi's own pixels, every 1.8 s
static int v2ItemBob() {
  float s = sinf((millis() % 1800) * (6.2831853f / 1800));
  return 2 * (int)lroundf(s * 3);
}
// a soft glow behind it, rings on the 2 px grid in its own colour, so a dark ball shows up in front of the room's
// window (Fedor)
static void v2ItemGlow(int cx, int cy, uint16_t col) {
  // each ring's reach, and its strength /256 (Fedor: half the first glow's, which was 150, 105, 66, 34)
  static const uint8_t R[4] = { 50, 56, 62, 68 }, A[4] = { 75, 52, 33, 17 };
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  for (int y = (cy - 68) & ~1; y <= cy + 68; y += 2)
    for (int x = (cx - 68) & ~1; x <= cx + 68; x += 2) {
      int fx = x + 1 - cx, fy = y + 1 - cy, r2 = fx * fx + fy * fy;
      for (int i = 0; i < 4; i++)
        if (r2 <= R[i] * R[i]) {
          v2Cell(fb, x, y, col, A[i]);
          break;
        }
    }
}
// An item's own page of the card (Fedor: one each, so there's room to say what it does): 0 the Ultra Ball, 1 the
// Master Ball, 2 the Repel. The item hovering in its glow with how many you hold, then in a panel what it does, how
// to earn one, and how far along the next is (the Repel's, while one runs: its minutes left)
static void v2CardItem(int which) {
  static const StrId NAME[3] = { S_ULTRA, S_MASTER, S_REPEL };
  static const uint16_t GLOW[3] = { C565(0xff, 0xe0, 0x70), C565(0xff, 0xa8, 0xe8), C565(0xd0, 0xb8, 0xff) };
  char does[200], earn[64], t[16];
  if (which == 2) snprintf(does, sizeof(does), T(S_REPEL_DOES_FMT), (unsigned)REPEL_MIN);
  else strlcpy(does, T(which ? S_MASTER_DOES : S_ULTRA_DOES), sizeof(does));
  if (which == 2) snprintf(earn, sizeof(earn), T(S_REPEL_EARN_FMT), (unsigned)(REPEL_PLAY_S / 60));
  else snprintf(earn, sizeof(earn), T(S_MASTER_EARN_FMT), (unsigned)(which ? MASTER_FIGHTS : ULTRA_FIGHTS));
  const int w = V2_CARD_R - V2_CARD_L, pitch = 22;
  int lines = v2Wrapped(does, 0, w, pitch, 5, -1, false);
  int h = (lines - 1) * pitch + 16 + 18 + 16 + 12 + 14;  // what it does, how it's earned, the bar
  // the item halfway between the board and the panel (no lower than 150, where the shorter panels leave it)
  int cy = (V2_BOARD_Y + 32 + v2CardTop(h) - 3) / 2;
  if (cy > 150) cy = 150;
  cy += v2ItemBob();
  v2ItemGlow(CX, cy, GLOW[which]);
  if (which == 2) v2RepelIcon(CX - 37, cy - 40, 5);  // (the can in the middle)
  else if (which == 1) v2MasterBall(CX - 45, cy - 45, 5);
  else v2UltraBall(CX - 45, cy - 45, 5);
  int held = which == 0 ? pet.ultraBalls : which == 1 ? pet.masterBalls : pet.repelHeld;
  if (held) drawCountBadge(CX + 44, cy - 33, held);
  char nm[40];
  v2Sign(v2Caps(T(NAME[which]), nm, sizeof(nm)));  // (over the glow's top)
  int y = v2CardPanel(h);
  gfx->setTextColor(UI_INK);
  v2Wrapped(does, y, w, pitch, 5);
  y += (lines - 1) * pitch + 16 + 18;
  gfx->setTextColor(V2_INK_MUTED);
  v2Wrapped(earn, y, w, 20, 1);
  y += 16 + 12;
  if (which < 2) {
    uint8_t bar = which ? pet.masterBar : pet.ultraBar, n = which ? MASTER_FIGHTS : ULTRA_FIGHTS;
    snprintf(t, sizeof(t), "%u/%u", bar, n);
    v2ItemBar(y, bar / (float)n, t, which ? v2MasterInk('q') : C565(0xd0, 0xa0, 0x20));
  } else if (pet.repelOn()) {
    snprintf(t, sizeof(t), "%um", pet.repelMinutesLeft());
    v2ItemBar(y, pet.repelMinutesLeft() / (float)REPEL_MIN, t, C565(0x5a, 0x3a, 0x8c));
  } else {
    uint16_t sec = pet.repelPlaySec;
    snprintf(t, sizeof(t), "%u:%02u/%u:%02u", sec / 60, sec % 60, REPEL_PLAY_S / 60, REPEL_PLAY_S % 60);
    v2ItemBar(y, sec / (float)REPEL_PLAY_S, t, C565(0x8a, 0x5c, 0xc8));
  }
}

// ----- the backpack: the Repel's seat, after the berries and the candy, while you hold one or one runs -----
static bool v2RepelSeat() { return pet.repelHeld || pet.repelOn(); }
// its seat (the bubble's own, centre (cx, cy)): the spray, how many you hold, and the minutes left while one runs
static void v2RepelSeatDraw(int cx, int cy) {
  v2RepelIcon(cx - 30, cy - 32, V2_ICON_S);  // the can in the middle of its seat (Fedor), at 4x as the foods
  if (pet.repelHeld) v2CountBadge(cx + 30, cy - 37, pet.repelHeld);
  if (pet.repelOn()) {
    char b[8];
    snprintf(b, sizeof(b), "%um", pet.repelMinutesLeft());
    bool cjk = classicFontBegin();
    setSize(2);
    int w = textW(b, 2) + 10;
    gfx->fillRoundRect(cx - w / 2, cy + 24, w, 18, 8, C565(0x5a, 0x3a, 0x8c));
    gfx->setTextColor(UI_WHITE);
    setCur(cx - textW(b, 2) / 2 + 1, cy + 26);
    printT(b);
    classicFontEnd(cjk);
  }
}
// ----- a food from the backpack (Fedor): it drops from the top onto your Pokemon, which hops for a second; the bag
// stays open for another -----
#define V2_DROP_MS 560
#define V2_HOP_MS 1000
#define V2_DROP_TURN 170  // how far a food turns as it falls, in degrees (Fedor: a slight spin)
static uint32_t v2DropT = 0;  // when the last one was tapped (0: none falling or hopping)
static uint8_t v2DropItem = 0;
static int v2DropX = CX;
static int8_t v2DropSpin = 1;  // which way it turns: the other way each time
static void v2FoodDropStart(int item) {
  v2DropItem = item;
  v2DropT = millis() ? millis() : 1;
  v2DropX = partyOthers() ? PARTY_MINE_X : (int)beh.x;
  v2DropSpin = -v2DropSpin;
}
// where a food lands (Fedor): its middle on your Pokemon's, halfway from the top of what it draws to the ground
static int v2DropMidY() {
  const PmdAct &a = pmd.acts[PMD_IDLE];
  int s = v2HomeScale(pmd), top = a.h;
  for (int r = 0; r < a.h && top == a.h; r++)  // the first row its first frame draws in
    for (int c = 0; c < a.w; c++)
      if (a.data[r * a.w + c] != 0xFF) {
        top = r;
        break;
      }
  if (top == a.h) top = 0;
  int y0 = PET_GROUND - pmdFrameRise(a, 0, s);  // the frame's top, as drawPmdActM() places it
  return (y0 + top * s + PET_GROUND) / 2;
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
  if (e >= V2_DROP_MS || !pmd.loaded) return;  // landed: eaten (or sprayed)
  static const char *const *const FOOD[4] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_CANDY };
  float k = e / (float)V2_DROP_MS;
  int mid = v2DropMidY(), cy = 57 + (int)((mid - 57) * k * k);  // its middle, from the top: faster and faster
  if (v2DropItem < 4)  // turning a little as it falls
    drawMapTurned(FOOD[v2DropItem], HOME_ICON_N, 3, v2DropSpin * V2_DROP_TURN * k, v2DropX, cy, HOME_ICON_N / 2.0f, HOME_ICON_N / 2.0f);
  else
    v2RepelIcon(v2DropX - 22, cy - 24, 3);  // the Repel, dropping too (Fedor), the can centred
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
  v2FoodDropStart(4);  // it drops onto your Pokemon as the berries do
}
// the bag on the home screen, while one runs: a purple badge with the minutes left
static void v2RepelBagBadge(int cx, int cy) {
  if (!pet.repelOn()) return;
  char b[8];
  snprintf(b, sizeof(b), "%u", pet.repelMinutesLeft());
  gfx->fillCircle(cx, cy, 12, V2_RIM);
  gfx->fillCircle(cx, cy, 10, C565(0x8a, 0x5c, 0xc8));
  gfx->setTextColor(UI_WHITE);
  v2BadgeNum(cx, cy, b);  // centred on the disc (scene_v2.h)
}
