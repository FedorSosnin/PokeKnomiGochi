#pragma once
// Every Pokemon's cry (cry.h), petted, as JSON for the Shiny Pokedex page (tools/make_sprite_gallery.py puts it in;
// tools/sprite_gallery.html plays it): {"25":[[ms,f0,f1,v0,v1,duty,vib,vibHz,harm,hv,rasp,nhz],...]} with the pitches
// and the noise's clock in Hz, the levels, the warble and the interval in hundredths. `make -C host voices` writes
// tools/voices.json; the host tests check that file is what this makes, so the page plays what the game plays.
#include <stdio.h>
#include <string>
#include "cry.h"

static inline std::string cryVoicesJson() {
  std::string out = "{";
  char b[160];
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    Cry c;
    cryBuild(c, d, VOICE_PET);
    snprintf(b, sizeof(b), "%s\n\"%d\":[", d > 1 ? "," : "", d);
    out += b;
    for (int i = 0; i < c.n; i++) {
      const CrySeg &s = c.seg[i];
      snprintf(b, sizeof(b), "%s[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", i ? "," : "", s.ms, (int)lrintf(s.f0),
               (int)lrintf(s.f1), (int)lrintf(s.v0 * 100), (int)lrintf(s.v1 * 100), s.duty, (int)lrintf(s.vib * 100),
               (int)lrintf(s.vibHz), (int)lrintf(s.harm * 100), (int)lrintf(s.hv * 100), (int)lrintf(s.rasp * 100),
               (int)lrintf(s.nhz));
      out += b;
    }
    out += "]";
  }
  return out + "\n}\n";
}
