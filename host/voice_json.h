#pragma once
// Every Pokemon's cry (cry.h), petted, as JSON for the KnomiDex page (tools/make_sprite_gallery.py puts it in;
// tools/sprite_gallery.html plays it), a list of notes each:
//   [steps, f0 Hz, sweep (thousandths of a semitone a step), trill (hundredths), v0, vstep, vevery, duty, harm
//    (hundredths), hv, n0, nevery, noise Hz, short loop]
// `make -C host voices` writes tools/voices.json; the host tests check that file is what this makes, so the page plays
// what the game plays.
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
      const CryNote &n = c.note[i];
      snprintf(b, sizeof(b), "%s[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]", i ? "," : "", n.frames, (int)lrintf(n.f0),
               (int)lrintf(n.sweep * 1000), (int)lrintf(n.trill * 100), n.v0, n.vstep, n.vevery, n.duty,
               (int)lrintf(n.harm * 100), n.hv, n.n0, n.nevery, n.nhz, n.nshort);
      out += b;
    }
    out += "]";
  }
  return out + "\n}\n";
}
