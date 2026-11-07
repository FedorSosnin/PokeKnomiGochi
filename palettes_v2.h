#pragma once
#include <stdint.h>
// PokeKnomiGochi V2 scene palettes (RGB565, with the #rrggbb they came from).
// Sky: three flat bands top -> mix(top,bot,0.5) -> bot, split at y 124 and 196, two checker rows at each seam.
// Hills: far / mid / near fills; the top row of each hill is mixed toward white by `lit`, the next row checkered at lit/2.
// Field, Forest, Beach use the meadow dusk/night shifts: dusk mixes hills 20-32% toward #f0a060, night uses meadow_night.
// Cave and Mansion look the same at every hour (the Mansion is always night).

struct V2Pal { uint16_t top, bot, far, mid, near, dark, trunk, cloud, cloudSh, sun, halo; uint8_t lit16; };

static const V2Pal V2PAL_MEADOW_DAY = { 0x7DFC, 0xDF7D, 0xA6B6, 0x8631, 0x6DAE, 0x3BEA, 0x7AA6, 0xFFFF, 0xCF1D, 0xFF31, 0xFF99, 4 };  // #7fbde6 #d8eee8 #a7d7b0 #86c58e #6db673 #3f7e52
static const V2Pal V2PAL_MEADOW_DUSK = { 0xC2C9, 0xF5AD, 0xBE32, 0xA5D0, 0x8D8D, 0x642A, 0x6A05, 0xFED7, 0xECCF, 0xFF99, 0xFE93, 4 };  // #c75a4a #f6b46a #bec596 #a3ba81 #8cb06e #628454
static const V2Pal V2PAL_MEADOW_NIGHT = { 0x0884, 0x218B, 0x21A9, 0x1968, 0x1946, 0x10E4, 0x18C4, 0x29CB, 0x2169, 0xEF7E, 0x29AA, 2 };  // #0c1224 #233058 #25364c #1e2e40 #1a2a36 #101c26
static const V2Pal V2PAL_FIELD_DAY = { 0xAEFE, 0xE79D, 0xA6F2, 0x8E6F, 0x7DE9, 0x4C46, 0x7AA6, 0xFFFF, 0xCF1D, 0xFF31, 0xFF99, 4 };  // #a8dcf0 #e2f2ea #a6dc90 #8fce7a #79bd4f #4f8a32
static const V2Pal V2PAL_FOREST_DAY = { 0x9EBE, 0xDF7C, 0x5D0D, 0x3C49, 0x4CCA, 0x1A45, 0x7AA6, 0xFFFF, 0xCF1D, 0xFF31, 0xFF99, 3 };  // #9fd6f0 #dcefe4 #5fa36c #3f8a4c #4f9a55 #1f4a2a
static const V2Pal V2PAL_BEACH_DAY = { 0x7DFC, 0xDF7D, 0xA6B6, 0xEEF5, 0xE673, 0xBCCB, 0x8AC6, 0xFFFF, 0xCF1D, 0xFF31, 0xFF99, 4 };  // #7fbde6 #d8eee8 #a7d7b0 #ecdcac #e2cf98 #b89a5e
static const V2Pal V2PAL_CAVE_ANY = { 0x20E5, 0x2947, 0x39AA, 0x4A0B, 0x524C, 0x18A4, 0x7AA6, 0xFFFF, 0xCF1D, 0xFF31, 0xFF99, 2 };  // #201c2c #2f2a3f #3a3450 #494258 #544b63 #1a1624
static const V2Pal V2PAL_MANSION_ANY = { 0x1065, 0x290A, 0x20E8, 0x1906, 0x2147, 0x0844, 0x18C4, 0x398C, 0x292A, 0xEF7E, 0x29AA, 2 };  // #140f2c #2a2350 #241e40 #1e2034 #25283a #0d0b20

// need-bar fills, as V1: >= 50 ok, >= 25 warn, else bad
#define V2_BAR_OK 0x5DCD    // #58b868
#define V2_BAR_WARN 0xED07  // #e8a23c
#define V2_BAR_BAD 0xEA87   // #e8503a
#define V2_TRACK 0x18C4     // #1b1b25: the empty track and the icon band, blended (see layout_v2.h)
