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
  { 155, 198, true },   // ENE   (lower left)
  { 342, 385, false },  // HYG   (lower right; 385 = 25)
};

// icons: a circle round the screen's own centre, so each is the same distance from the edge. At 4x, 72 px (Fedor:
// "make the icons on the main screen and their surround, including the backpack popup, a bit larger. They some times
// take extra clicks and effort to press on the tiny knomi"; they were 3x on a circle of 180, 60 px apart), on a
// circle of about 176. Their outline is whole, 4 px, as the backpack's foods have it (v2IconTrim; Fedor: "match the
// outline to the current berry icons"), so they sit further round than they did (Fedor, choosing it: "A: spread them
// out"): at 142.4, 114.4, 91, 65 and 36.7 degrees, 6 px between what each draws (Fedor: the icons "touch each other
// and overlap. Just a few pixels will fix this"). For the band's round ends to stay 6 px clear of the ENE and HYG
// bars (Fedor: "a few pixels of gap ... I dont want those touching"), those two end 5 degrees higher than FOOD and
// JOY start (V2_BARS). Each centre on even pixels, so the cells land whole on the Knomi
#define V2_ICON_S 4
#define V2_ICON_HALF 36
#define V2_ICON_R 176
// Left to right: the Game Boy, the map, the backpack in the middle at the bottom, the moon, the drop (Fedor: "Lets swap
// the icons around so that the back pack in the middle bottom ... Its weird to click the edge buttons all the time, but
// totally fine when its occasionally"; it was on the left). The bag at 90 degrees; the map and the moon as far from it
// either side (Fedor: "So the spacing between the backpack and the moon is the same as the backpack and map"), 6 px
// between what each draws, as before; the Game Boy, narrower than the bag that was there, 4 px further round so the
// map has room, the drop where it was. Gaps: 5.7, 6, 6, 8 px
static const int16_t V2_BTN[5][2] = { {90, 336}, {158, 392}, {232, 408}, {306, 392}, {374, 338} };  // games, map, bag, moon, drop

// the icon band (the "holder"): one curved band under all five, ends centred on the Game Boy and the drop
#define V2_BAND_RIN 138
#define V2_BAND_ROUT 214
#define V2_BAND_A0 36.7
#define V2_BAND_A1 142.4

// the home's scene sits this much higher than it's painted, the Pokemon with it (PET_GROUND 256 where it was 288;
// Fedor: "move up the entire scene. So the pokemon and its background are always above the center line"), so the
// food over the icons doesn't cover it
#define V2_SCENE_UP 32
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
// With the foods at 4x as the icons (they were 3x, 62 px apart), centred (Fedor: "keep the berry pop up centered"),
// on a circle round the icons' own centre, so it follows their band's arch at an even distance, and 10 px over it
// (Fedor: "It can also be moved down slightly to cover the menu under it a bit. Since both cant be used at the same
// time"). Spread out left and right (Fedor: "we need to spread them out some left and right"; they were 72 px apart,
// 4 px between seats): 92 px apart along the arc, closer only so the end seats stay within V2_FEED_SPAN degrees of
// straight down, as five do (the Repel's with the foods), up beside your Pokemon's feet. A seat's tap reaches half
// way to the next. Flatter than the icons' arch since (Fedor: "reduce its curvature to bring the edges of it down a
// bit. Plenty of space side to side"): a circle of 150 round a point 45 px over the screen's centre, its bottom
// where it was (the round ends 15 px lower, 20 px further out); three seats spread to 50 degrees each side
#define V2_FEED_CX CX         // the seats' circle: its centre, and its radius
#define V2_FEED_CY 188
#define V2_FEED_R 150
#define V2_FEED_RIN 110       // the band, from that centre
#define V2_FEED_ROUT 194
#define V2_FEED_STEP 95       // from one seat to the next, along it, at most
#define V2_FEED_SPAN 66       // the end seats at most this far round from straight down (degrees)...
#define V2_FEED_SPAN_MIN 50   // ...and at least this far
#define V2_FEED_SEAT_R 34

// Settings' pages: the time with the sound and the language, the Knomi's Wi-Fi and updates, and About (about_v2.h)
#if defined(BOARD_KNOMI_V2) && !defined(BOARD_HOST)
#define V2_CLOCK_PAGES 3
#else
#define V2_CLOCK_PAGES 2  // (a computer's Wi-Fi and updates are its own business)
#endif
#define V2_ABOUT_PAGE (V2_CLOCK_PAGES - 1)
#define V2_SET_PANEL_Y1 318  // the first page's panel's bottom: START OVER is under it

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
  if (y >= 344) return -1;  // the carpet, the count's board, the X
  if (y < 108) return 0;
  if (y < 180) return 1;
  if (y < 256) return 2;
  return x < 240 ? 3 : 4;
}
// ...and where each group stands in the room, which its press and hover outline
static inline void v2TrophyGroupBox(int g, int &x, int &y, int &w, int &h) {
  static const int16_t B[V2_TR_GROUPS][4] = { { 122, 52, 222, 56 }, { 84, 108, 298, 74 }, { 84, 182, 298, 76 },
                                              { 40, 258, 182, 56 }, { 246, 252, 176, 86 } };
  x = B[g][0], y = B[g][1], w = B[g][2], h = B[g][3];
}

// the radial menu (radial_v2.h): how long a still finger on the Pokemon takes to open it (the taps need it early)
#define V2_RADIAL_HOLD_MS 450
