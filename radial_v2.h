// V2: the radial menu (Fedor). scene_v2.h includes this at its end, so it's only there with UI_V2 on.
//
// A hold on your Pokemon (V2_RADIAL_HOLD_MS, the finger still) opens a ring round the screen in four: Settings
// above, Medals to the right, Stats below and the Pokedex to the left, where the home's swipes put them. Its middle
// is open, so the finger that opened it covers none of it. Slide onto one and lift to open it; lift in the middle
// and the ring stays for a tap (in the middle or outside, it closes; left alone 8 s, too). Held on still in the
// middle, the release question comes at 3 s, as before. The ring is see-through, as the icons' band is (Fedor),
// so while it's open the home leaves out what it would show through: the header, the band, the icons, the pill.

#define V2_RAD_IN 86    // the ring, from the screen's centre
#define V2_RAD_OUT 204
#define V2_RAD_GAP 7    // between two sections, along the diagonals: |dx| and |dy| this close

// its icons, as the home's: 18 x 18, drawn at 3x
static const char *const SPR_V2_RAD_GEAR[18] = {
  "........kk........",
  ".......kssk.......",
  "....k..kssk..k....",
  "...kskksssskksk...",
  "..kssssssssssssk..",
  "...kssssssssssk...",
  "...ksssskkssssk...",
  ".kkssssk..ksssSkk.",
  "ksssssk....ksSSSSk",
  "ksssssk....kSSSSSk",
  ".kkssssk..kSSSSkk.",
  "...ksssskkSSSSk...",
  "...ksssssSSSSSk...",
  "..ksssssSSSSSSSk..",
  "...kskkSSSSkkSk...",
  "....k..kSSk..k....",
  ".......kSSk.......",
  "........kk........",
};
static const char *const SPR_V2_RAD_MEDAL[18] = {
  "..................",
  "..kkkk......kkkk..",
  "..kbbBk....krrRk..",
  "...kbbBk..krrRk...",
  "....kbbBkkrrRk....",
  ".....kbBkkrRk.....",
  "......kkkkkk......",
  ".....kwwffffk.....",
  "....kwffffffYk....",
  "...kwfffOOfffYk...",
  "...kffffOOfffYk...",
  "...kfOOOOOOOOYk...",
  "...kffOOOOOOfYk...",
  "...kfffOOOOffYk...",
  "....kfOOffOOYk....",
  ".....kYffffYk.....",
  "......kkkkkk......",
  "..................",
};
static const char *const SPR_V2_RAD_STATS[18] = {
  "..................",
  "..kkkkkkkkkkkkkk..",
  ".kwwwwwwwwwwwwwwk.",
  ".kwwwwwwwwwkkkwwk.",
  ".kwwwwwwwwwkggkwk.",
  ".kwwwwwkkkwkggkwk.",
  ".kwwwwwkfkwkgGkwk.",
  ".kwwwwwkfkwkgGkwk.",
  ".kwkkkwkfkwkgGkwk.",
  ".kwkrkwkfYwkgGkwk.",
  ".kwkrkwkfYwkgGkwk.",
  ".kwkrRwkfYwkgGkwk.",
  ".kwkrRwkfYwkgGkwk.",
  ".kwkkkkkkkkkkkkwk.",
  ".kwwwwwwwwwwwwwwk.",
  ".kSSSSSSSSSSSSSSk.",
  "..kkkkkkkkkkkkkk..",
  "..................",
};
static const char *const SPR_V2_RAD_DEX[18] = {
  "..................",
  "..kkkkkkkkkkkkkk..",
  ".kRrrrrrrrrrrrrRk.",
  ".krkkkkrrrrrrrrrk.",
  ".krkbbkrrkrkrrrrk.",
  ".krkbBkrrkykkgkrk.",
  ".krkkkkrrrrrrrrrk.",
  ".krrrrrrrrrrrrrrk.",
  ".krkkkkkkkkkkkkrk.",
  ".krkwwwwwwwwwwkrk.",
  ".krkwggwwwwwwwkrk.",
  ".krkwwwwwwwwwwkrk.",
  ".krkwwwwwgggwwkrk.",
  ".krkkkkkkkkkkkkrk.",
  ".krrrrrrrrrrrrrrk.",
  ".kRRRRRRRRRRRRRRk.",
  "..kkkkkkkkkkkkkk..",
  "..................",
};

static bool v2RadialSticky = false;  // let go in the middle: open for a tap
static uint32_t v2RadialT = 0;       // when it opened, or was let go in the middle

static void v2RadialStart() {
  v2RadialOpen = true;
  v2RadialSticky = false;
  v2RadialHover = -1;
  v2RadialT = millis();
  sfxPlay(SFX_TAP);
}
static void v2RadialClose() {
  v2RadialOpen = v2RadialSticky = false;
  v2RadialHover = -1;
}
// the section at (x, y): 0 up, 1 right, 2 down, 3 left; -1 in the middle or outside (a little past its edge counts)
static int v2RadialAt(int16_t x, int16_t y) {
  int fx = x - CX, fy = y - CY, r2 = fx * fx + fy * fy;
  if (r2 < V2_RAD_IN * V2_RAD_IN || r2 > (V2_RAD_OUT + 16) * (V2_RAD_OUT + 16)) return -1;
  return abs(fy) > abs(fx) ? (fy < 0 ? 0 : 2) : (fx > 0 ? 1 : 3);
}
// what each opens: the page its swipe opens
static void v2RadialPick(int s) {
  if (s == 0) {  // Settings: the swipe down's
    openClock();
  } else if (s == 1) {  // Medals: the swipe left's
    cardOpen = cardAch = true;
    cardPage = 0;
    achDetail = -1;
  } else if (s == 2) {  // Stats: the swipe up's card
    cardOpen = true;
    cardAch = false;
    cardPage = 0;
  } else {  // the Pokedex: the swipe right's
    galleryOpen = true;
    galleryPage = 0;
    galleryDetail = 0;
    galleryDirty = true;
  }
}
// a finger lifting while it's open: on a section, its page; in the middle after the hold that opened it, it stays
// for a tap; anywhere else, it closes
static void v2RadialLift(int16_t x, int16_t y) {
  int s = v2RadialAt(x, y), fx = x - CX, fy = y - CY;
  if (s >= 0) {
    sfxPlay(SFX_TAP);
    v2RadialClose();
    v2RadialPick(s);
  } else if (!v2RadialSticky && fx * fx + fy * fy < V2_RAD_IN * V2_RAD_IN) {
    v2RadialSticky = true;
    v2RadialHover = -1;
    v2RadialT = millis();
  } else {
    v2RadialClose();
  }
}
// an icon at 3x centred at (cx, cy), and its label (in capitals) centred under it or over it (labelY): white on a
// dark 2 px shadow, as the pages' numbers, so it reads on any scene through the ring
static void v2RadialItem(const char *const *icon, int cx, int cy, const char *label, int labelY) {
  drawMap(icon, HOME_ICON_N, cx - 27, cy - 27, 3, false);
  char b[40];
  v2Caps(label, b, sizeof(b));
  setSize(2);
  int x = centerX(b, 2) + (cx - CX);
  gfx->setTextColor(C565(0x1b, 0x1b, 0x25));
  setCur(x + 2, labelY + 2);
  printT(b);
  gfx->setTextColor(UI_WHITE);
  setCur(x, labelY);
  printT(b);
}
// The ring over the dimmed home, see-through as the icons' band is (Fedor: "the same color as the area behind the
// main screen icons"), with its fainter edge; the section under the finger green in the panels' dark rim. Its icons
// and words, the Pokemon seen through the middle.
static void v2RadialDraw() {
  if (v2RadialSticky && millis() - v2RadialT > 8000) {
    v2RadialClose();
    return;
  }
  v2DimFrame(140);
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const int ri = V2_RAD_IN, ro = V2_RAD_OUT, rr = ro + 3;
  const uint16_t green = C565(0x58, 0xb8, 0x68);
  uint16_t col;
  int ra, ta;
  v2BandInk(col, ra, ta);
  for (int y = (CY - rr) & ~1; y <= CY + rr; y += 2)
    for (int x = (CX - rr) & ~1; x <= CX + rr; x += 2) {
      int fx = x + 1 - CX, fy = y + 1 - CY, r2 = fx * fx + fy * fy;
      if (r2 < (ri - 3) * (ri - 3) || r2 > rr * rr) continue;
      int d = abs(abs(fx) - abs(fy));
      if (d < V2_RAD_GAP) continue;  // the gaps between them
      int s = abs(fy) > abs(fx) ? (fy < 0 ? 0 : 2) : (fx > 0 ? 1 : 3);
      bool rim = r2 < ri * ri || r2 > ro * ro || d < V2_RAD_GAP + 4;
      if (s == v2RadialHover) {
        v2Cell(fb, x, y, rim ? V2_RIM : green, 256);
      } else {
        v2Cell(fb, x, y, col, ra);
        if (!rim) v2Cell(fb, x, y, col, ta);
      }
    }
  v2RadialItem(SPR_V2_RAD_GEAR, CX, 74, T(S_SETTINGS), 106);
  v2RadialItem(SPR_V2_RAD_MEDAL, 378, 210, T(S_RAD_MEDALS), 244);
  v2RadialItem(SPR_V2_RAD_STATS, CX, 388, T(S_RAD_STATS), 340);
  v2RadialItem(SPR_V2_RAD_DEX, 88, 210, T(S_DEX_TITLE), 244);
}
