#pragma once
// Every Pokemon's voice (chirp.h), petted, by its English name, as JSON for the Shiny Pokedex page
// (tools/make_sprite_gallery.py puts it in; tools/sprite_gallery.html plays it): {"25":[[ms,f0,f1,vol,duty,noise],...]}
// with the pitches in Hz and the level in hundredths. `make -C host voices` writes tools/voices.json; the host tests
// check that file is what this makes, so the page always says what the game says.
#include <stdio.h>
#include <string>
#include "chirp.h"

static inline std::string chirpVoicesJson() {
  std::string out = "{";
  char b[64];
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    Chirp c;
    chirpBuild(c, DEX_TBL[d].name, d, VOICE_PET);
    snprintf(b, sizeof(b), "%s\n\"%d\":[", d > 1 ? "," : "", d);
    out += b;
    for (int i = 0; i < c.n; i++) {
      const ChirpSeg &s = c.seg[i];
      snprintf(b, sizeof(b), "%s[%d,%d,%d,%d,%d,%d]", i ? "," : "", s.ms, (int)lrintf(s.f0), (int)lrintf(s.f1),
               (int)lrintf(s.vol * 100), s.duty, s.noise);
      out += b;
    }
    out += "]";
  }
  return out + "\n}\n";
}
