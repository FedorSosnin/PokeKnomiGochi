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

// the food picker: a text-box bubble out of the bag (its tail points at it), the foods on round seats 66 px
// apart; it grows to the right, so with the candy gone it's three seats from the same left edge
#define V2_FEED_X0 101
#define V2_FEED_Y0 264
#define V2_FEED_H 72
#define V2_FEED_CELL 66
#define V2_FEED_SEAT_R 30
