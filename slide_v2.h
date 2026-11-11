// V2: a page turning slides (Fedor: "Can we make the left and right arrows through the menus 'slide' the menu screens
// over in that direction instead of instantly appear?"). Every menu with pages: the card, Settings, the Pokedex's grid
// and its entries, a zone's grid and its entries, the trophy shelves and a medal's cards, a buddy's two pages, the
// trade grid, the Wi-Fi list, the time zones and the egg grid; Travel's two islands and Knomi Tower's friends. A side
// button or a swipe turns the page as before; then what was on screen slides out and the new page slides in after it, from the side the side button points to,
// V2_SLIDE_MS, easing in and out. Where the menu sits on a room that is the screen's cached backdrop (v2SceneBuf),
// the room stays put and only what's on it moves (the board, the panel, the grid, a shelf's things); on a backdrop
// drawn some other way the whole screen slides. The side buttons, the page line and the X stay where they are, the
// new page's from the start.
// The page isn't drawn again while it slides: the last frame of the page you were on is kept (a copy of each frame a
// paged menu flushes), the new page is drawn once, and each frame of the slide is made from the two by copying, which
// the Knomi does far quicker than drawing a page, so its frames come V2_SLIDE_FRAME_MS (layout_v2.h) apart, not the menus'
// 100 ms. Without the memory for the two copies (two frames, 434 KB each), pages turn at once, as before.
#define V2_SLIDE_MS 280
// From a row down (the page line, the X) the new page's, still: under every panel's rim, so no edge of a panel stays
// behind while the panel moves (Fedor: "The page with Raichu on it has a strange white line on the bottom during the
// animation": an entry's panel reaches 389, under the card's line)
#define V2_SLIDE_CHROME_Y 380  // the page line's top (v2PageOf()); the card's panels end 375, Settings', a buddy's, the
                               // Wi-Fi list's and a medal's 377 at most
#define V2_SLIDE_DEX_Y 360     // the Pokedex grid: its page line (DEXG_PAGE_Y 362), its panel's rim ends 359
#define V2_SLIDE_ZONE_Y 334    // a zone's grid: from SEARCH's rim (ZONE_BTN_Y 336), its panel's ends 333 (both checked
                               // there)
#define V2_SLIDE_ENTRY_Y 392   // an entry: its panel's rim ends 389, the X's starts 396
#define V2_SLIDE_TRADE_Y 397   // the trade grid: its page line sits on its panel's rim, so it moves with the grid
#define V2_SLIDE_TRAVEL_Y 398  // Travel's two islands: the whole map moves, all but the X (its rim starts 398)
#define V2_SLIDE_ARROW_R 30    // round a side button (its rim and shadow too): the new page's, still
#define V2_SLIDE_SENT 0x0821   // in a page kept for a slide, a pixel of the room (a dark grey nothing is drawn in)
#define V2_SLIDE_WIFI_ROWS 5   // the Wi-Fi list's rows a page (WIFI_ROWS, further down the sketch, checked there)

// which paged menu is on screen and its page (menu 0: none), in render()'s order of screens
struct V2PageKey {
  uint8_t menu;
  int16_t page, pages, arrowY, still = V2_SLIDE_CHROME_Y;  // still: the rows from here down don't move
};
static V2PageKey v2PageKeyNow() {
  V2PageKey k = { 0, 0, 0, V2_PAGE_ARROW_Y };
  int pos, n;
  if (pet.awaitingStarter()) return k;
  if (pet.awaitingEggPick()) {
    if (!eggPickDetail)  // (its page line 12 px under the panel's rim)
      k = { 1, (int16_t)eggPickPage, (int16_t)v2PickPages(), (int16_t)v2PickArrowY(), (int16_t)(v2PickGridY() + 3 * DEXG_CELL + 10) };
    return k;
  }
  if (buddyMenuOpen) return { 2, buddyPage, 2, V2_PAGE_ARROW_Y };
  if (partyOpen || playMenuOpen || sdOpen || vbOpen) return k;
  if (tradeOpen) {
    if (trPhase == TR_PICK) k = { 3, (int16_t)trPage, (int16_t)((pet.formCount() + 15) / 16), V2_PAGE_ARROW_Y, V2_SLIDE_TRADE_Y };
    return k;
  }
  if (galleryOpen) {
    if (galleryDetail) {
      v2DexEntryPos(galleryDetail, pos, n);
      return { 4, (int16_t)pos, (int16_t)n, V2_PAGE_ARROW_Y, V2_SLIDE_ENTRY_Y };
    }
    return { 5, (int16_t)galleryPage, DEX_PAGES, PAGE_ARROW_Y, V2_SLIDE_DEX_Y };
  }
  if (gameOpen || dungeonOpen || runOpen || surfOpen || battleOpen || quizOpen || gamesMenuOpen) return k;
  if (zoneScreen == ZS_PICK) return { 50, (int16_t)v2IslePage, 2, V2_PAGE_ARROW_Y, V2_SLIDE_TRAVEL_Y };  // Knomi Island, then Knomi Tower's islet
  if (zoneScreen == ZS_TOWER) {  // Knomi Tower (tower_v2.h): picking two friends, the Pokedex's grid
    if (twView == TW_PICK) k = { 51, (int16_t)twPickPage, (int16_t)twPickPages(), V2_PAGE_ARROW_Y, V2_SLIDE_DEX_Y };
    return k;
  }
  if (zoneScreen) {
    if (zoneScreen != ZS_ZONE) return k;
    if (zoneDetail) {
      v2DexEntryPos(zoneDetail, pos, n);
      return { (uint8_t)(10 + zoneCur), (int16_t)pos, (int16_t)n, V2_PAGE_ARROW_Y, V2_SLIDE_ENTRY_Y };
    }
    return { (uint8_t)(20 + zoneCur), zonePage, (int16_t)((zoneSize(zoneCur) + DEXG_PAGE - 1) / DEXG_PAGE), PAGE_ARROW_Y,
             V2_SLIDE_ZONE_Y };
  }
  if (kbOpen) return k;
  if (clockOpen) {
    WifiSetupState ws = wifiSetupState();
    if (ws == WS_LIST) return { 6, wifiListPage, (int16_t)((wifiSetupNetCount() + V2_SLIDE_WIFI_ROWS - 1) / V2_SLIDE_WIFI_ROWS), V2_PAGE_ARROW_Y };
    if (ws != WS_OFF) return k;
    if (tzPickOpen) return { 7, (int16_t)tzPickIdx, TIME_ZONE_COUNT, TZ_ARROW_Y };
    return { 8, clockPage, V2_CLOCK_PAGES, V2_PAGE_ARROW_Y };
  }
  if (cardOpen) {
    if (!cardAch) return { 9, cardPage, (int16_t)cardPageCount(), V2_PAGE_ARROW_Y };
    if (achDetail >= 0) {  // a medal's card: its place in its group
      int g = v2TrophyGroup(achDetail);
      return { (uint8_t)(30 + g), (int16_t)(v2TrophyPos(achDetail) - V2_TR_GROUP_FIRST[g]), (int16_t)v2TrophyGroupSize(g), V2_PAGE_ARROW_Y };
    }
    if (achGroup >= 0) return { 40, achGroup, V2_TR_GROUPS, V2_PAGE_ARROW_Y };
  }
  return k;
}

static uint16_t *v2SlideOld = nullptr, *v2SlideNew = nullptr;  // the page you were on, the one coming
static V2PageKey v2SlideLast = { 0, 0, 0, 0 };  // the paged menu of the last frame (v2SlideOld holds it if v2SlideHave)
static bool v2SlideHave = false;
static V2PageKey v2SlideTo;     // the page sliding in
static int8_t v2SlideDir = 0;   // 1: on (in from the right), -1: back (in from the left); 0: no slide
static uint32_t v2SlideT0 = 0;
static bool v2SlideRoom = false;  // only what's on the room moves (the pages' room pixels are V2_SLIDE_SENT)
static int16_t v2SlideSpan[2][V2_W][2];  // each row's first and past-last pixel that moves: old, new
static bool v2Sliding() { return v2SlideDir != 0; }

static bool v2SlideBufs() {
  if (!v2SlideOld) v2SlideOld = (uint16_t *)ps_malloc(V2_W * V2_W * sizeof(uint16_t));
  if (!v2SlideNew) v2SlideNew = (uint16_t *)ps_malloc(V2_W * V2_W * sizeof(uint16_t));
  return v2SlideOld && v2SlideNew;
}
// in the circle of a side button that page k shows
static bool v2SlideOnArrow(const V2PageKey &k, int x, int y) {
  int dy = y - k.arrowY;
  if (dy * dy > V2_SLIDE_ARROW_R * V2_SLIDE_ARROW_R) return false;
  int dx = pageArrowDx(k.arrowY);
  for (int side = -1; side <= 1; side += 2) {
    if (side < 0 ? k.page <= 0 : k.page >= k.pages - 1) continue;
    int ex = x - (CX + side * dx);
    if (ex * ex + dy * dy <= V2_SLIDE_ARROW_R * V2_SLIDE_ARROW_R) return true;
  }
  return false;
}
// the two pages ready to move: on a room, each pixel of it (and the side buttons of both pages) marked as room, and
// each row's span of what moves
static void v2SlidePrepare(const V2PageKey &from, const V2PageKey &to) {
  const uint16_t *room = v2SceneKey >= 0 ? v2SceneBuf : nullptr;
  v2SlideRoom = false;
  if (room) {  // the room: only if both pages show it (at least a third of each, outside their side buttons)
    long same[2] = { 0, 0 };
    for (int y = 0; y < to.still; y++)
      for (int x = 0; x < V2_W; x++) {
        int i = y * V2_W + x;
        same[0] += v2SlideOld[i] == room[i];
        same[1] += v2SlideNew[i] == room[i];
      }
    long third = (long)to.still * V2_W / 3;
    v2SlideRoom = same[0] > third && same[1] > third;
  }
  for (int b = 0; b < 2; b++) {
    uint16_t *p = b ? v2SlideNew : v2SlideOld;
    if (!v2SlideRoom) {  // the whole screen moves: each page's own side buttons out of it (the new page's are drawn still
                         // on top; Travel's two islands had one gliding in under the one standing), filled from above them
      const V2PageKey &k = b ? to : from;
      int dx = pageArrowDx(k.arrowY), r = V2_SLIDE_ARROW_R;
      for (int side = -1; side <= 1; side += 2) {
        if (side < 0 ? k.page <= 0 : k.page >= k.pages - 1) continue;
        int cx = CX + side * dx, above = k.arrowY - r - 1 > 0 ? k.arrowY - r - 1 : 0;  // (from the row above it: a coast
                                                                                     // that runs down stays whole)
        for (int y = k.arrowY - r; y <= k.arrowY + r; y++) {
          if (y < 0 || y >= to.still) continue;
          for (int x = cx - r; x <= cx + r; x++)
            if (x >= 0 && x < V2_W && (x - cx) * (x - cx) + (y - k.arrowY) * (y - k.arrowY) <= r * r) p[y * V2_W + x] = p[above * V2_W + x];
        }
      }
    }
    for (int y = 0; y < to.still; y++) {
      int16_t x0 = V2_W, x1 = 0;
      if (!v2SlideRoom) x0 = 0, x1 = V2_W;
      else
        for (int x = 0; x < V2_W; x++) {
          int i = y * V2_W + x;
          if (p[i] == room[i] || v2SlideOnArrow(from, x, y) || v2SlideOnArrow(to, x, y)) {
            p[i] = V2_SLIDE_SENT;
            continue;
          }
          if (p[i] == V2_SLIDE_SENT) p[i] ^= 1;  // (a drawn pixel that happens to be the mark: a hair bluer)
          if (x < x0) x0 = x;
          x1 = x + 1;
        }
      v2SlideSpan[b][y][0] = x0;
      v2SlideSpan[b][y][1] = x1;
    }
  }
}
// one page's row onto the screen's, shifted by off (the screen's x + off is the page's); on a room only what moves
static inline void v2SlideRow(uint16_t *row, const uint16_t *src, int off, const int16_t *span) {
  int a = span[0] - off, e = span[1] - off;
  if (a < 0) a = 0;
  if (e > V2_W) e = V2_W;
  if (a >= e) return;
  if (!v2SlideRoom) {
    memcpy(row + a, src + a + off, (size_t)(e - a) * sizeof(uint16_t));
    return;
  }
  for (int x = a; x < e; x++) {
    uint16_t v = src[x + off];
    if (v != V2_SLIDE_SENT) row[x] = v;
  }
}
// the slide p of the way (0 to 1) into the frame
static void v2SlideCompose(float p) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  int d = p >= 1 ? V2_W : SNAP2((int)(p * p * (3 - 2 * p) * V2_W));  // easing in and out; even: sharp on the Knomi
  int oOff = v2SlideDir * d, nOff = v2SlideDir * (d - V2_W);
  int still = v2SlideTo.still;
  for (int y = 0; y < still; y++) {
    uint16_t *row = fb + y * V2_W;
    if (v2SlideRoom) memcpy(row, v2SceneBuf + y * V2_W, V2_W * sizeof(uint16_t));
    v2SlideRow(row, v2SlideOld + y * V2_W, oOff, v2SlideSpan[0][y]);
    v2SlideRow(row, v2SlideNew + y * V2_W, nOff, v2SlideSpan[1][y]);
  }
  memcpy(fb + still * V2_W, v2SlideNew + still * V2_W, (size_t)(V2_W - still) * V2_W * sizeof(uint16_t));
  drawPageArrows(v2SlideTo.page, v2SlideTo.pages, v2SlideTo.arrowY);  // the new page's side buttons, still
}
// each frame a screen flushes (pressOverlay(), before the press preview): a slide's next frame in place of what was
// drawn; or, a paged menu's page just turned, the slide starts from the page kept; or, a paged menu's frame, kept
static void v2SlideFrame() {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  if (v2SlideDir) {
    uint32_t t = millis() - v2SlideT0;
    v2SlideCompose(t >= V2_SLIDE_MS ? 1 : t / (float)V2_SLIDE_MS);
    if (t < V2_SLIDE_MS) return;
    v2SlideDir = 0;  // landed: the new page as it was drawn, kept for the next turn
    memcpy(v2SlideOld, fb, V2_W * V2_W * sizeof(uint16_t));
    v2SlideLast = v2SlideTo;
    v2SlideHave = true;
    return;
  }
  V2PageKey k = v2PageKeyNow();
  if (k.menu && v2SlideHave && k.menu == v2SlideLast.menu && k.page != v2SlideLast.page) {
    memcpy(v2SlideNew, fb, V2_W * V2_W * sizeof(uint16_t));
    v2SlideDir = k.page > v2SlideLast.page ? 1 : -1;
    v2SlideTo = k;
    v2SlidePrepare(v2SlideLast, k);
    v2SlideT0 = millis();
    v2SlideCompose(0);
    return;
  }
  v2SlideLast = k;
  v2SlideHave = k.menu && v2SlideBufs();
  if (v2SlideHave) memcpy(v2SlideOld, fb, V2_W * V2_W * sizeof(uint16_t));
}
