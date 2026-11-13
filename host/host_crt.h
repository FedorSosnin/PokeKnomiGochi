// The CRT screen for the computer builds (Mac, Windows; the web has the same look as a WebGL shader in
// webgame/shell.html). Fedor: "That looks incredible" - on by default, at Large.
//
// It reads like a tube, not a filter: each scanline is a soft beam that swells where the picture is bright
// (233 of them: two of the game's rows each, as an old console's 240), the beam soft along the line and sharp
// across it, red and blue a hair apart (convergence), a glow on bright pixels and a wide warm halo in the glass,
// richer colour with deeper blacks, the picture bulging as through curved glass out to the rim and darkening softly
// into it (since 2.1.4; before, it stopped short at a black edge), a reflection on the glass, and grain. Tuned on mockups at 2x (Large on a Retina Mac); its sizes are in
// the game's pixels, so it looks the same at any size. Since 2.1.3 the glow, the halo and the glass's bulge are half
// 2.1.2's (Fedor, of a Lite look tried beside it: "I just want this lite effect as on").
#pragma once
#include <stdint.h>

// Once per frame, from the game's 466 x 466 canvas (R, G, B, x bytes), for a side x side square: the colour, the
// beam and the glows, and (when the side changes) the curved glass's tables.
void hostCrtPrepare(const uint8_t *rgba, int side);
// Then its rows y0..y1-1 into out (4 bytes a pixel, stride bytes a row): BGRA for Windows' bitmaps, RGBA for the
// Mac's images. Rows can be drawn on several threads at once. frame: a number that changes every frame (the grain).
void hostCrtRows(uint8_t *out, int stride, bool bgra, int y0, int y1, uint32_t frame);
// A point on the effect's square (0..side) -> the canvas point under it through the curved glass (0..466); false
// off the picture. Taps and the mouse go through this, so they land on what's shown.
bool hostCrtPoint(float x, float y, int side, float &cx, float &cy);
