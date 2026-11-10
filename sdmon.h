#pragma once
#include <Arduino.h>
#include "ui_v2.h"

// Animated TPK1 sprite (legacy format, fallback path). The project uses
// PMD/TPK2 (PmdMon) for everything; this path stays inactive if there is no NNN.bin on the SD.
// Indexed data lives in PSRAM; the palette is RGB565.
struct SdMon {
  bool loaded = false;
  uint16_t w = 0, h = 0, frames = 0, frameMs = 100;
  uint8_t scale = 2;       // integer zoom factor when drawing
  uint16_t palCount = 0;
  uint16_t pal[256];
  uint8_t *data = nullptr;  // frames * w * h indices (0xFF = transparent)

  bool load(uint8_t dexNum, bool shiny = false);
  void unload();
};

// PMD sprite actions (TPK2 format)
enum : uint8_t {
  PMD_IDLE = 0, PMD_WALKL, PMD_WALKR, PMD_SLEEP, PMD_EAT, PMD_HURT,
  PMD_ATTACK, PMD_POSE, PMD_HOP, PMD_NOD, PMD_BREATH, PMD_SIT,
#if UI_V2
  PMD_WALKD, PMD_WALKU,  // V2: walking toward you and away (Walk rows 0 and 4), from the pack's tail
  PMD_WALKDR, PMD_WALKUR, PMD_WALKUL, PMD_WALKDL,  // ...and the diagonals (rows 1, 3, 5, 7), for the Pokedex's turn
#endif
  PMD_NACTS
};

struct PmdAct {
  uint8_t w = 0, h = 0, frames = 0;
  uint8_t base = 0;  // row+1 of the lowest pixel (anchor by the feet, not the canvas)
#if UI_V2
  uint8_t top = 0;       // V2: the highest row with something in it, in any frame (its height is base - top)
  uint8_t low[24] = {};  // V2: each frame's own lowest row + 1: every frame stands on its own feet (pmdFrameY)
  uint8_t footW = 0;     // V2: how far apart its legs stand, in its own pixels, where it stands by its legs (sdmon.cpp)
#endif
  uint16_t ms[24];
  const uint8_t *data = nullptr;  // frames * w * h in the blob
};

// multi-action PMD sprite loaded from SD into PSRAM
#if UI_V2
// V2: a sprite's palette and actions, kept in PSRAM beside its frames while it's loaded (PmdMon::side)
struct PmdSide {
  uint16_t pal[256];
  PmdAct acts[PMD_NACTS];
};
extern uint16_t gPmdNoPal[256];          // an unloaded sprite's: all zero, never written
extern PmdAct gPmdNoActs[PMD_NACTS];
#endif
struct PmdMon {
  bool loaded = false;
  bool shiny = false;  // the shiny sprite was loaded: it gets sparkles
#if UI_V2
  uint8_t dex = 0;  // V2: the species, for its size (pmdScale)
#endif
  uint16_t palCount = 0;
#if UI_V2
  // V2: the palette and the actions (2 KB, with the walks and each frame's feet) live in PSRAM with the frames while
  // it's loaded, not in the internal RAM that Wi-Fi and the buddy relay's TLS need: twenty of these took 40 KB of it,
  // and the relay's handshake left 6 KB free. Unloaded, they point at an empty set, read as before
  uint16_t *pal = gPmdNoPal;
  uint8_t *blob = nullptr;
  PmdAct *acts = gPmdNoActs;
  PmdSide *side = nullptr;
  PmdMon() = default;
  PmdMon(const PmdMon &) = delete;  // (side is its own)
  PmdMon &operator=(const PmdMon &) = delete;
  ~PmdMon() { unload(); }
#else
  uint16_t pal[256];
  uint8_t *blob = nullptr;
  PmdAct acts[PMD_NACTS];
#endif

  bool load(uint8_t dexNum, bool shiny = false);
  void unload();
  bool has(uint8_t a) const { return loaded && a < PMD_NACTS && acts[a].frames > 0; }
};

#if UI_V2
// V2: how far above the ground a frame's top goes. Each frame stands on its own feet: anchored by the lowest
// pixel of all its frames, the ones whose feet sat higher in the box (Rattata's walk rises and falls 12 rows,
// Eevee bounds, Venusaur breathes) floated a row's worth at 3 to 5 times the size, up to 60 px over their
// shadows (Fedor: "make sure they look like they walk"). What's left of a hop is half a pixel a row
inline int pmdFrameRise(const PmdAct &a, uint8_t f, uint8_t s) {
  int base = a.base ? a.base : a.h, low = f < 24 && a.low[f] ? a.low[f] : base;
  int hop = (base - low) / 2;
  return low * s + (hop > 12 ? 12 : hop);
}
uint8_t pmdScale(const PmdMon &m, uint8_t maxS);  // V2: a sprite's size, the same wherever it's drawn (sdmon.cpp)
#endif

// gallery thumbnails (whole thumbs.bin in PSRAM)
struct SdThumbs {
  bool loaded = false;
  uint8_t *data = nullptr;
  uint16_t count = 0;
  uint32_t size = 0;  // bytes read: bounds the file offsets
  bool load();
  void unload();  // frees the blob: reload after receiving a new thumbs.bin
  const uint8_t *get(int16_t dex) const;  // blob: w,h,palCount,pal[],idx[]
  // the shiny one (stored after the 151 regular ones); an older thumbs.bin has none: the regular
  const uint8_t *getForm(int16_t dex, bool shiny) const {
    const uint8_t *t = shiny && dex >= 1 && dex <= 151 ? get(dex + 151) : nullptr;
    return t ? t : get(dex);
  }
};
extern SdThumbs thumbs;

bool sdBegin();                 // mounts the SD (SDMMC 1-bit), true if there is a card
bool sdSerialCommand(const String &line);  // PUT/LS over USB; true if it handles it
extern bool sdReady;
extern bool sdDirty;  // true after receiving files: reload sprite
