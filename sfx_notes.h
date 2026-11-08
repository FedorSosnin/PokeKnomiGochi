#pragma once
#include <stdint.h>
#include "audio.h"
#include "ui_v2.h"

// Sound effects as square-wave tone notes (Game Boy style); f == 0 is a rest.
// The SFX order matches the Sfx enum.
struct Note { uint16_t f, ms; };

#if UI_V2
// V2: the tap is Fedor's "confirm" blip, as measured: 2049 and 2731 Hz (a fourth apart), low, high, low in 16 ms
// steps, then the high one held 83 ms and fading; its notes run into each other (sfxLegato())
static const Note N_TAP[]    = {{2049, 17}, {2731, 16}, {2049, 16}, {2731, 83}};
#define N_TAP_LEN 4
#else
static const Note N_TAP[]    = {{880, 35}};
#define N_TAP_LEN 1
#endif
static const Note N_EAT[]    = {{660, 45}, {0, 12}, {660, 45}};
static const Note N_PLAY[]   = {{784, 45}, {988, 60}};
static const Note N_HEART[]  = {{1047, 55}, {1319, 90}};
static const Note N_HATCH[]  = {{523, 80}, {659, 80}, {784, 110}, {1047, 170}};
static const Note N_EVOLVE[] = {{523, 80}, {659, 80}, {784, 80}, {1047, 90}, {1319, 230}};
static const Note N_MEDAL[]  = {{784, 70}, {0, 25}, {784, 70}, {0, 25}, {1047, 200}};
static const Note N_DENY[]   = {{300, 110}, {200, 170}};
static const Note N_BYE[]    = {{784, 150}, {659, 150}, {523, 280}};
static const Note N_LEVEL[]  = {{784, 70}, {1047, 130}};
static const Note N_SHAKE1[] = {{523, 110}};  // the count to three, rising
static const Note N_SHAKE2[] = {{659, 110}};
static const Note N_SHAKE3[] = {{784, 110}};

struct SfxDef { const Note *n; uint8_t len; };
static const SfxDef SFX[SFX_COUNT] = {
  {N_TAP, N_TAP_LEN}, {N_EAT, 3}, {N_PLAY, 2}, {N_HEART, 2}, {N_HATCH, 4},
  {N_EVOLVE, 5}, {N_MEDAL, 5}, {N_DENY, 2}, {N_BYE, 3}, {N_LEVEL, 2},
  {N_SHAKE1, 1}, {N_SHAKE2, 1}, {N_SHAKE3, 1},
};
#if UI_V2
// an effect whose notes run into each other: no fade between them, the wave carried on, and its last note fading
// to about half by its end (the tap)
static inline bool sfxLegato(uint8_t id) { return id == SFX_TAP; }
#endif
