#include "sdmon.h"
#include "pin_config.h"
#if UI_V2
#include <new>  // (placement new: PmdSide)
#endif
#ifdef BOARD_HOST
#include "host/host_pak.h"
#elif defined(BOARD_KNOMI_V2)
#include "flashpak.h"
#else
#include <FS.h>
#include <SD_MMC.h>
#endif

bool sdReady = false;
bool sdDirty = false;
SdThumbs thumbs;
#if UI_V2
uint16_t gPmdNoPal[256];
PmdAct gPmdNoActs[PMD_NACTS];
#endif

bool PmdMon::load(uint8_t dexNum, bool shiny) {
  unload();
  if (!sdReady) return false;

  char path[28];
  snprintf(path, sizeof(path), "/mons/p%s%03u.bin", shiny ? "s" : "", dexNum);
  File f = SD_MMC.open(path, FILE_READ);
  bool shinyFile = shiny && f;
  if (!f && shiny) {  // no PMD shiny: use the normal one
    snprintf(path, sizeof(path), "/mons/p%03u.bin", dexNum);
    f = SD_MMC.open(path, FILE_READ);
  }
  if (!f) return false;

  uint32_t size = f.size();
  if (size < 7 || size > 3UL * 1024 * 1024) { f.close(); return false; }
  blob = (uint8_t *)ps_malloc(size);
  if (!blob || f.read(blob, size) != size || memcmp(blob, "TPK2", 4) != 0) {
    if (blob) { free(blob); blob = nullptr; }
    f.close();
    return false;
  }
  f.close();
#if UI_V2
  side = (PmdSide *)ps_malloc(sizeof(PmdSide));  // its palette and actions, in PSRAM (sdmon.h)
  if (!side) { unload(); return false; }
  new (side) PmdSide();
  pal = side->pal;
  acts = side->acts;
#endif

  uint8_t nActs = blob[4];
  memcpy(&palCount, blob + 5, 2);
  if (palCount > 256 || (uint32_t)7 + palCount * 2 > size) { unload(); return false; }
  memcpy(pal, blob + 7, palCount * 2);

  const uint8_t *p = blob + 7 + palCount * 2;
  const uint8_t *end = blob + size;
#if UI_V2
  // V2's walk rows come after the counted actions, in a tail that 1.51's loader never reads ("TPKX", u8 n,
  // then n more actions laid out the same), and after it the back views ("TPKY", the same again), which 2.0.5's
  // loader never reads (it stops at an action it doesn't know: a Knomi gone back to it still loads these packs).
  // The loop below runs over each in turn
  for (int part = 0; part < 3; part++) {
    if (part > 0) {
      if (p + 5 > end || memcmp(p, part == 1 ? "TPKX" : "TPKY", 4) != 0) break;
      nActs = p[4];
      p += 5;
    }
#endif
  for (uint8_t i = 0; i < nActs && p + 4 <= end; i++) {
    uint8_t id = p[0], w = p[1], h = p[2], nf = p[3];
    p += 4;
    if (id >= PMD_NACTS || nf > 24) { unload(); return false; }
    // validates that ms[] and the frame data fit in the blob (truncated file)
    uint32_t bytes = (uint32_t)nf * 2 + (uint32_t)w * h * nf;
    if (w == 0 || h == 0 || nf == 0 || p + bytes > end) { unload(); return false; }
    PmdAct &a = acts[id];
    a.w = w;
    a.h = h;
    a.frames = nf;
    for (uint8_t k = 0; k < nf; k++) {
      a.ms[k] = p[0] | (p[1] << 8);
      if (a.ms[k] == 0) a.ms[k] = 100;  // never 0: pmdFrameAt() would spin without advancing
      p += 2;
    }
    a.data = p;
    p += (uint32_t)w * h * nf;
    // lowest row with content in any frame: anchor by the feet
    uint8_t base = 1;
    for (uint8_t f = 0; f < nf; f++) {
      const uint8_t *fr = a.data + (uint32_t)f * w * h;
      for (int r = h - 1; r >= 0; r--) {
        bool any = false;
        for (int c = 0; c < w && !any; c++)
          if (fr[r * w + c] != 0xFF) any = true;
#if UI_V2
        if (any) a.low[f] = r + 1;  // this frame's own feet
#endif
        if (any) { if (r + 1 > base) base = r + 1; break; }
      }
    }
    a.base = base;
#if UI_V2
    a.top = h;  // the highest row with something in it, in any frame
    for (uint8_t f = 0; f < nf; f++) {
      const uint8_t *fr = a.data + (uint32_t)f * w * h;
      for (int r = 0; r < a.top; r++) {
        bool any = false;
        for (int c = 0; c < w && !any; c++)
          if (fr[r * w + c] != 0xFF) any = true;
        if (any) { a.top = r; break; }
      }
      if (!a.low[f]) a.low[f] = base;  // an empty frame stands where the others do
    }
    if (a.top >= base) a.top = 0;
    // Ivysaur and Venusaur face you head down, the face lower than their feet: stood by the face, their legs were
    // in the air (Fedor: "His nose touches but his legs are in the air"). Facing you, each frame stands by its
    // legs instead, the lowest of its three outermost columns each side, and the face hangs over the ground in
    // front: standing, walking toward you, asleep, hopping, hurt and attacking (asleep, it was still on its chin:
    // "Are you sure its fixed? It looks the same"). Turned any other way a leg is what's lowest, and Ivysaur
    // sleeps curled up on its side, lying on what's lowest
    bool facingYou = id == PMD_WALKD || (id < PMD_WALKD && id != PMD_WALKL && id != PMD_WALKR);
    if ((dexNum == 3 || (dexNum == 2 && id != PMD_SLEEP)) && facingYou) {
      uint8_t legsBase = 0;
      for (uint8_t f = 0; f < nf; f++) {
        const uint8_t *fr = a.data + (uint32_t)f * w * h;
        int x0 = w, x1 = -1;
        for (int r = 0; r < h; r++)
          for (int c = 0; c < w; c++)
            if (fr[r * w + c] != 0xFF) {
              if (c < x0) x0 = c;
              if (c > x1) x1 = c;
            }
        uint8_t legs = 0;
        for (int k = 0; k < 3 && x1 >= 0; k++) {
          const int cols[2] = { x0 + k, x1 - k };
          for (int c : cols)
            for (int r = h - 1; r >= 0; r--)
              if (fr[r * w + c] != 0xFF) {
                if (r + 1 > legs) legs = r + 1;
                break;
              }
        }
        if (legs) a.low[f] = legs;
        if (a.low[f] > legsBase) legsBase = a.low[f];
        if (x1 >= 0 && x1 - x0 + 1 > a.footW) a.footW = x1 - x0 + 1;  // (its shadow reaches past its legs: the home)
      }
      if (legsBase) a.base = legsBase;
    }
    // Diglett and Dugtrio come up out of a mound of earth, drawn with them: stood on the mound's rounded bottom they
    // were in the air (Fedor: "diglet ... is a bit too high in the air. Its another pokemon whos sprite needs to be a
    // bit lower"). They stand on its wide part instead, the rows under it in the ground: 4 of Diglett's, 5 of
    // Dugtrio's (up to where the mound is still 85% as wide as it gets, standing, asleep and hurt), in every row
    if (dexNum == 50 || dexNum == 51) {
      uint8_t sink = dexNum == 50 ? 4 : 5;
      for (uint8_t f = 0; f < nf; f++) a.low[f] = a.low[f] > sink ? a.low[f] - sink : 1;
      a.base = a.base > sink ? a.base - sink : 1;
    }
    // Blastoise sleeps sitting back with a leg stretched out toward you, its foot 4 rows under the rest of it: stood on
    // that foot, its body and shell were 20 px over the ground, over the sea on the Beach (Fedor: "When Blastoise
    // sleeps, he looks to be sitting in the water", "Still floating above the ground"). Asleep it sits on its body,
    // the foot on the ground in front of it
    // Magikarp sleeps on its side with a fin hanging 3 rows under its body: stood on the fin's tip, it hovered 12 px
    // over the ground (Fedor: "When sleeping, the magicarp appears to hover off the ground"). It lies on its body,
    // the fin on the ground in front of it
    uint8_t sink = id != PMD_SLEEP ? 0 : dexNum == 9 ? 4 : dexNum == 129 ? 3 : 0;
    if (sink) {
      for (uint8_t f = 0; f < nf; f++) a.low[f] = a.low[f] > sink ? a.low[f] - sink : 1;
      a.base = a.base > sink ? a.base - sink : 1;
    }
#endif
  }
#if UI_V2
  }
#endif
  loaded = true;
  this->shiny = shinyFile;
#if UI_V2
  dex = dexNum;
#endif
  Serial.printf("loaded %s (%u KB)\n", path, size / 1024);
  return true;
}

#if UI_V2
// how tall each one is, in tenths of a metre (the Pokedex's heights): its size on screen (pmdScale)
static const uint8_t DEX_DM[151] = {
  7, 10, 20, 6, 11, 17, 5, 10, 16, 3, 7, 11, 3, 6, 10, 3, 11, 15, 3, 7,                // 1-20
  3, 12, 20, 35, 4, 8, 6, 10, 4, 8, 13, 5, 9, 14, 6, 13, 6, 11, 5, 10,                // 21-40
  8, 16, 5, 8, 12, 3, 10, 10, 15, 2, 7, 4, 10, 8, 17, 5, 10, 7, 19, 6,                // 41-60
  10, 13, 9, 13, 15, 8, 15, 16, 7, 10, 17, 9, 16, 4, 10, 14, 10, 17, 12, 16,          // 61-80
  3, 10, 8, 14, 18, 11, 17, 9, 12, 3, 15, 13, 16, 15, 88, 10, 16, 4, 13, 5,           // 81-100
  12, 4, 20, 4, 10, 15, 14, 12, 6, 12, 10, 19, 11, 10, 22, 4, 12, 6, 13, 8,           // 101-120
  11, 13, 15, 14, 11, 13, 15, 14, 9, 65, 25, 3, 3, 10, 8, 9, 8, 4, 10, 5,             // 121-140
  13, 18, 21, 17, 16, 20, 18, 40, 22, 20, 4,                                          // 141-151
};
// A sprite's scale: as before, its frame at most 170 px tall (and maxS); a big Pokemon, 1.5 m and up (Snorlax,
// Lapras...), at least 5 and from 1 m at least 4, while it stands no taller than 150 px (under the name and
// mood, hops and all). Frames padded for a big idle made Snorlax smaller than Venusaur (Fedor: "should be the
// same size as Venasaur"). Nothing comes out smaller than before
uint8_t pmdScale(const PmdMon &m, uint8_t maxS) {
  const PmdAct &i = m.acts[PMD_IDLE];
  uint8_t s = i.h ? 170 / i.h : 5;
  if (s < 2) s = 2;
  if (s > maxS) s = maxS;
  uint8_t dm = m.dex >= 1 && m.dex <= 151 ? DEX_DM[m.dex - 1] : 0;
  int big = dm >= 15 ? 5 : dm >= 10 ? 4 : 0, tall = i.base > i.top ? i.base - i.top : i.h;
  if (big > maxS) big = maxS;
  while (big > s && tall * big > 150) big--;
  return big > s ? (uint8_t)big : s;
}
#endif

void PmdMon::unload() {
  loaded = shiny = false;
  if (blob) {
    free(blob);
    blob = nullptr;
  }
#if UI_V2
  if (side) {  // (PmdSide has nothing to destroy)
    free(side);
    side = nullptr;
  }
  pal = gPmdNoPal;
  acts = gPmdNoActs;
  palCount = 0;
#else
  for (auto &a : acts) {
    a.w = a.h = a.frames = a.base = 0;
    a.data = nullptr;
  }
#endif
  loaded = false;
}

void SdThumbs::unload() {
  if (data) { free(data); data = nullptr; }
  loaded = false;
  count = 0;
  size = 0;
}

bool SdThumbs::load() {
  unload();  // reload without leaking the previous blob
  if (!sdReady) return false;
  File f = SD_MMC.open("/mons/thumbs.bin", FILE_READ);
  if (!f) {
    Serial.println("no thumbs.bin (gallery without thumbnails)");
    return false;
  }
  uint32_t sz = f.size();
  // bounds the size: avoids an absurd ps_malloc with a corrupt file
  if (sz < 10 || sz > 2UL * 1024 * 1024) {
    Serial.println("thumbs.bin invalid (size)");
    f.close();
    return false;
  }
  data = (uint8_t *)ps_malloc(sz);
  if (!data || f.read(data, sz) != sz || memcmp(data, "TPTH", 4) != 0) {
    Serial.println("thumbs.bin invalid");
    if (data) { free(data); data = nullptr; }
    f.close();
    return false;
  }
  f.close();
  memcpy(&count, data + 4, 2);
  // the offset table (one per species) must fit entirely in what was read
  if ((uint32_t)6 + 4UL * count > sz) {
    Serial.println("thumbs.bin invalid (offset table)");
    unload();
    return false;
  }
  size = sz;
  loaded = true;
  Serial.printf("thumbnails loaded: %u (%u KB)\n", count, sz / 1024);
  return true;
}

const uint8_t *SdThumbs::get(int16_t dex) const {
  if (!loaded || dex < 1 || dex > count) return nullptr;
  uint32_t off;
  memcpy(&off, data + 6 + 4 * (dex - 1), 4);
  // the offset comes from the file unchecked: without this, a truncated
  // or corrupt thumbs.bin would read outside the PSRAM allocation
  if (off > size - 3) return nullptr;  // the w,h,palCount header must fit (size >= 10 always)
  uint32_t need = 3 + (uint32_t)data[off + 2] * 2 + (uint32_t)data[off] * data[off + 1];
  if (need > size || off > size - need) return nullptr;  // and the whole blob too
  return data + off;
}

#ifdef BOARD_KNOMI_V2
bool sdBegin() {
  sdReady = PAK.begin();
  Serial.println(sdReady ? "flash sprites ok" : "no flash sprites (ffat partition empty)");
  return sdReady;
}
#else
bool sdBegin() {
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
  sdReady = SD_MMC.begin("/sdcard", true /* 1-bit mode */, true /* format if mount fails */);
  if (sdReady) {
    Serial.printf("SD mounted: %llu MB\n", SD_MMC.cardSize() / (1024ULL * 1024ULL));
    SD_MMC.mkdir("/mons");
  } else {
    Serial.println("SD not detected (the game uses the flash sprites)");
  }
  return sdReady;
}

#endif  // BOARD_KNOMI_V2

bool SdMon::load(uint8_t dexNum, bool shiny) {
  unload();
  if (!sdReady) return false;

  char path[24];
  snprintf(path, sizeof(path), "/mons/%s%03u.bin", shiny ? "s" : "", dexNum);
  File f = SD_MMC.open(path, FILE_READ);
  if (!f && shiny) {  // no shiny variant: use the normal one
    snprintf(path, sizeof(path), "/mons/%03u.bin", dexNum);
    f = SD_MMC.open(path, FILE_READ);
  }
  if (!f) {
    Serial.printf("%s does not exist\n", path);
    return false;
  }

  char magic[4];
  uint16_t header[4];
  if (f.read((uint8_t *)magic, 4) != 4 || memcmp(magic, "TPK1", 4) != 0 ||
      f.read((uint8_t *)header, 8) != 8) {
    f.close();
    return false;
  }
  w = header[0];
  h = header[1];
  frames = header[2];
  frameMs = header[3];
  // bounds dimensions: avoids overflowed or absurd size with a corrupt file
  if (f.read((uint8_t *)&palCount, 2) != 2 || palCount > 256 ||
      w == 0 || w > 256 || h == 0 || h > 256 || frames == 0 || frames > 64) {
    f.close();
    return false;
  }
  if (f.read((uint8_t *)pal, palCount * 2) != palCount * 2) {
    f.close();
    return false;
  }

  uint32_t size = (uint32_t)w * h * frames;
  data = (uint8_t *)ps_malloc(size);
  if (!data) {
    Serial.println("no PSRAM for the sprite");
    f.close();
    return false;
  }
  uint32_t got = f.read(data, size);
  f.close();
  if (got != size) {
    Serial.printf("%s truncated (%u of %u)\n", path, got, size);
    unload();
    return false;
  }

  // integer zoom so the critter is ~200 px tall on screen
  scale = 200 / h;
  if (scale < 2) scale = 2;
  if (scale > 5) scale = 5;

  Serial.printf("loaded %s: %ux%u x%u frames @%ums, scale %u\n",
                path, w, h, frames, frameMs, scale);
  loaded = true;
  return true;
}

void SdMon::unload() {
  if (data) {
    free(data);
    data = nullptr;
  }
  loaded = false;
}

// ---------------------------------------------------------------------------
// Serial loading protocol over USB (to fill the SD without taking it out of the board):
// PUT <path> <bytes>\n  + raw data   -> "OK" ... "DONE"
// LS\n                                   -> listing of /mons
// Use with tools/send_sd.py
// ---------------------------------------------------------------------------

#ifdef BOARD_KNOMI_V2
bool sdSerialCommand(const String &) { return false; }  // sprites flashed with esptool
#else
bool sdSerialCommand(const String &line) {
  if (line.startsWith("PUT ")) {
    int sp = line.lastIndexOf(' ');
    String path = line.substring(4, sp);
    uint32_t size = line.substring(sp + 1).toInt();
    if (!sdReady || size == 0 || size > 4 * 1024 * 1024) {
      Serial.println("ERR");
      return true;
    }
    if (!path.startsWith("/")) path = "/" + path;
    // bounds the write to /mons/: the path arrives as is from the serial line, without
    // sanitizing, so a tampered PUT could write anywhere on the
    // card. The real client (tools/send_sd.py) already sends names with the mons/
    // prefix.
    if (!path.startsWith("/mons/") || path.indexOf("..") >= 0) {
      Serial.println("ERR");
      return true;
    }
    // FILE_WRITE APPENDS at the end if the file already exists, so retrying one
    // that was left half-done lengthened it instead of replacing it: a corrupt
    // sprite was left, larger than the original. It matters more since the installer
    // resumes interrupted transfers, because it retries exactly the ones that failed.
    if (SD_MMC.exists(path)) SD_MMC.remove(path);
    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) {
      Serial.println("ERR");
      return true;
    }
    Serial.println("OK");
    static uint8_t buf[2048];
    uint32_t remaining = size;
    Serial.setTimeout(5000);
    while (remaining > 0) {
      size_t want = remaining > sizeof(buf) ? sizeof(buf) : remaining;
      size_t n = Serial.readBytes(buf, want);
      if (n == 0) break;  // timeout
      // without checking the return, a full card or a write failure gave
      // DONE anyway and left the file truncated on the SD: exactly the corrupt entry
      // that must be guarded against later when loading the sprite
      if (f.write(buf, n) != n) break;
      remaining -= n;
      Serial.println("#");  // ack: ready for the next block
    }
    f.close();
    Serial.setTimeout(1000);
    sdDirty = (remaining == 0);
    Serial.println(remaining == 0 ? "DONE" : "ERR");
    return true;
  } else if (line == "SDINFO") {  // remote diagnosis of "it doesn't recognize my SD"
    Serial.printf("total=%llu used=%llu\n", SD_MMC.totalBytes(), SD_MMC.usedBytes());
    Serial.println("DONE");
    return true;
  } else if (line == "LS") {
    File dir = SD_MMC.open("/mons");
    if (dir) {
      File e;
      while ((e = dir.openNextFile())) {
        Serial.printf("%s %u\n", e.name(), (uint32_t)e.size());
        e.close();
      }
      dir.close();
    }
    Serial.println("DONE");
    return true;
  }
  return false;
}
#endif  // BOARD_KNOMI_V2
