#pragma once
#include <stdint.h>
// PokeKnomiGochi V2 home layout, on the 466x466 canvas (centre 233,233, visible radius 231).
// Angles are in degrees, 0 = right, clockwise (screen y goes down).

#define V2_GRID 4                 // one 8-bit pixel of scenery = 4 canvas px = 2 Knomi px

// header (no battery, no streak flame on the home screen)
#define V2_CLOCK_Y 10             // size 2, e.g. "4:15 PM" (clock12Hour)
#define V2_LEVEL_Y 34             // size 2
#define V2_NAME_Y 54              // size 3, bold (drawn twice 1 px apart), accent colour; light ink on dark zones
#define V2_STATUS_Y 90            // size 2

// need bars: four arcs on the rim, round ends, label written along the middle of each bar
#define V2_BAR_RIN 210
#define V2_BAR_ROUT 228
#define V2_BAR_RIM 2              // a soft rim 2 px outside the track
// track: V2_TRACK blended 22%; rim: V2_TRACK blended 18%. Fill: the status colour, opaque.
// Each bar fills from its LOWER end. Labels: size 2, white, centred on the bar, glyphs rotated to the tangent.
struct V2Bar { uint16_t a0, a1; bool leftSide; };
static const V2Bar V2_BARS[4] = {
  { 203, 251, true },   // FOOD  (upper left)
  { 289, 337, false },  // JOY   (upper right)
  { 150, 198, true },   // ENE   (lower left)
  { 342, 390, false },  // HYG   (lower right; 390 = 30)
};

// icons: a circle round the screen's own centre, so each is the same distance from the edge
#define V2_ICON_R 180
#define V2_ICON_STEP_DEG 19.32   // keeps 60 px between centres
static const int16_t V2_BTN[5][2] = { {121, 374}, {173, 403}, {233, 413}, {293, 403}, {345, 374} };  // bag, games, map, moon, drop

// the icon band (the "holder"): one curved band under all five, ends centred on the bag and the drop
#define V2_BAND_RIN 153
#define V2_BAND_ROUT 207
#define V2_BAND_A0 51.4
#define V2_BAND_A1 128.6
// band: V2_TRACK 22%, plus a rim 150..210 at 18%. Round ends. On dark zones (cave, mansion, night) use white at ~12% instead.

// the close X: one place on every screen. Nothing else is drawn below y 394.
#define V2_CLOSE_Y 422
#define V2_CLOSE_R 24
#define V2_CONTENT_BOTTOM 394
// the X is red: #e8503a with a #3a4a5a rim and a lighter top
// page buttons: every paged screen, left and right, blue #4f93c4 discs r 22
#define V2_PAGE_ARROW_Y 228
#define V2_PAGE_ARROW_L 36
#define V2_PAGE_ARROW_R 430
#define V2_PAGE_ARROW_R_PX 22
#define V2_MIN_TAP 52             // the smallest tap target, canvas px

// the release question (rooms2_px.py release_dialog()): its RELEASE, red, and KEEP, green (the taps read them
// early in the sketch); a tap anywhere but RELEASE keeps it, as in V1
#define V2_REL_Y0 266
#define V2_REL_Y1 312
#define V2_REL_X0 104   // RELEASE: 104..226
#define V2_REL_X1 226
#define V2_KEEP_X0 240  // KEEP: 240..362
#define V2_KEEP_X1 362

// pet
#define V2_PET_FEET 296           // ground line of the pet's feet
#define V2_PET_SHADOW_Y 292       // soft shadow ellipse 40x7, 28% black (35% at night)

// hills (ellipses x0,y0,x1,y1), back to front, shared by every zone
static const int16_t V2_HILL_FAR[2][4]  = { {-120, 214, 230, 500}, {200, 200, 600, 520} };
static const int16_t V2_HILL_MID[2][4]  = { {-200, 246, 300, 600}, {240, 256, 700, 620} };
static const int16_t V2_HILL_NEAR[1][4] = { {-400, 276, 866, 1100} };
static const int16_t V2_MANSION_RISE[4] = { 20, 226, 446, 330 };   // added to the far hills in the Mansion zone

// the walk (when it walks): PMD Walk facing down, slowed 2.0x (one step cycle = 1.2 s)
#define V2_WALK_PACE_X10 20
#define V2_STREAM_LOOP_MS 8400    // a tuft takes this long from the bottom to the horizon

// the food picker (Fedor): a green band just above the icons' band and curved as it is (the icons' circle raised
// 72 px), its round ends on the first and last seats; the foods on round seats V2_FEED_STEP px apart along it, so
// your Pokemon stays in view
#define V2_FEED_CY (CY - 72)  // the seats' circle: its centre, and its radius, the icons' V2_ICON_R
#define V2_FEED_R 180
#define V2_FEED_RIN 147       // the band, from that centre (its rim reaches 377 under the middle: 6 px over the
#define V2_FEED_ROUT 213      // icons' band's)
#define V2_FEED_STEP 62       // from one seat to the next, along it
#define V2_FEED_SEAT_R 28

// the time page's START OVER (menus_v2.h): its box, which is also its tap
#define V2_RESET_X0 113
#define V2_RESET_Y0 337
#define V2_RESET_X1 353
#define V2_RESET_Y1 373

// the trophy room (menus_v2.h): its medals in the room's order, badges, the top shelf, the cups, the frames,
// the rosettes (medal bits, pet.h), which the medal card's page buttons and swipes step through ("Page 7 of 25")
static const uint8_t V2_TROPHY_ORDER[MED_COUNT] = { 17, 18, 19, 20, 21, 9, 10, 11, 12, 8, 13, 22, 23, 24, 14, 15, 16, 0, 1, 2, 3, 4, 5, 6, 7 };
static inline int v2TrophyPos(int medal) {
  for (int p = 0; p < MED_COUNT; p++)
    if (V2_TROPHY_ORDER[p] == medal) return p;
  return 0;
}
// ...in five groups, as the room's order runs (Fedor: the room, then each group a step closer with the names, then a
// medal's card): the badges, the top shelf (the Pokedex statues and the golden Pikachu), the cups, the photos, the
// rosettes. A group's shelf has two rows: how many are on the first (the rest go on the second), and how wide a place
// each one takes on each row. A row is V2_TR_ROW_H tall: V2_TR_ART_H for the thing (standing on the bottom of it, or
// in its middle when it hangs), then a shelf, then its name on up to two lines
#define V2_TR_GROUPS 5
static const uint8_t V2_TR_GROUP_FIRST[V2_TR_GROUPS + 1] = { 0, 5, 10, 14, 17, MED_COUNT };
static const uint8_t V2_TR_ROW1[V2_TR_GROUPS] = { 3, 3, 2, 2, 4 };
static const uint8_t V2_TR_CELL_W[V2_TR_GROUPS][2] = { { 108, 108 }, { 108, 150 }, { 150, 150 }, { 150, 150 }, { 92, 92 } };
#define V2_TR_ROW_Y0 70  // the first row's top
#define V2_TR_ROW_H 152
#define V2_TR_ART_H 100
static inline int v2TrophyGroup(int medal) {
  int p = v2TrophyPos(medal), g = 0;
  while (g < V2_TR_GROUPS - 1 && p >= V2_TR_GROUP_FIRST[g + 1]) g++;
  return g;
}
static inline int v2TrophyGroupSize(int g) { return V2_TR_GROUP_FIRST[g + 1] - V2_TR_GROUP_FIRST[g]; }
// the k-th of group g on its shelf: its place, which is also its tap
static inline void v2ShelfCell(int g, int k, int &x0, int &y0, int &w, int &h) {
  int r = k < V2_TR_ROW1[g] ? 0 : 1, inRow = r ? v2TrophyGroupSize(g) - V2_TR_ROW1[g] : V2_TR_ROW1[g];
  w = V2_TR_CELL_W[g][r];
  h = V2_TR_ROW_H;
  x0 = 233 - inRow * w / 2 + (r ? k - V2_TR_ROW1[g] : k) * w;
  y0 = V2_TR_ROW_Y0 + r * V2_TR_ROW_H;
}
// the medal under a tap on group g's shelf, or -1
static inline int v2ShelfAt(int g, int x, int y) {
  for (int k = 0; k < v2TrophyGroupSize(g); k++) {
    int x0, y0, w, h;
    v2ShelfCell(g, k, x0, y0, w, h);
    if (x >= x0 && x < x0 + w && y >= y0 && y < y0 + h) return V2_TROPHY_ORDER[V2_TR_GROUP_FIRST[g] + k];
  }
  return -1;
}
// the group under a tap in the room (menus_v2.h trRoom()): the badges' case, the top shelf, the cups' shelf, then on
// the lower wall the photos to the left and the rosettes to the right
static inline int v2TrophyGroupAt(int x, int y) {
  if (y < 108) return 0;
  if (y < 180) return 1;
  if (y < 256) return 2;
  return x < 240 ? 3 : 4;
}

// the radial menu (radial_v2.h): how long a still finger on the Pokemon takes to open it (the taps need it early)
#define V2_RADIAL_HOLD_MS 450
