#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ui_v2.h"

// Wi-Fi updates: the pure parts (tested on the PC). The Knomi reads update.json from the web
// installer's site, written by tools/build_knomi_web.sh:
//   {"version": "1.19", "sprites_size": 1290000, "sprites_sha256": "ab12..."}
// Catch 151's own site (2.2.0 on); if it doesn't answer, the old one, which keeps a frozen copy of 2.2.0 for the Knomis
// that hadn't moved yet when the repo was renamed (Fedor: "Catch151 is the new game name")
#define OTA_BASE_URL "https://fedorsosnin.github.io/Catch151/"
#define OTA_BASE_URL_OLD "https://fedorsosnin.github.io/PokeKnomiGochi/"
#define OTA_CHECK_EVERY_MS (24UL * 60 * 60 * 1000)  // the automatic check: once a day
#define OTA_FIRST_CHECK_MS (2UL * 60 * 1000)        // and the first, 2 minutes after boot
#define OTA_CHECK_V2_MS (60UL * 60 * 1000)           // V2: every hour, so a release made overnight is in by morning

// "1.19" > "1.18", "1.9" < "1.10": compares number by number. -1, 0 or 1.
inline int otaVersionCmp(const char *a, const char *b) {
  while (*a || *b) {
    long x = strtol(a, (char **)&a, 10), y = strtol(b, (char **)&b, 10);
    if (x != y) return x < y ? -1 : 1;
    while (*a && *a != '.') a++;  // a suffix like "-knomi" doesn't count
    while (*b && *b != '.') b++;
    if (*a == '.') a++;
    if (*b == '.') b++;
  }
  return 0;
}

// the value of "key" in a flat JSON object, as text (strings without their quotes). false if missing.
inline bool otaJsonField(const char *json, const char *key, char *out, size_t n) {
  if (!n) return false;
  size_t kl = strlen(key);
  for (const char *p = strchr(json, '"'); p; p = strchr(p + 1, '"')) {
    if (strncmp(p + 1, key, kl) != 0 || p[kl + 1] != '"') continue;
    p += kl + 2;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p++ != ':') return false;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    bool quoted = *p == '"';
    if (quoted) p++;
    size_t i = 0;
    while (*p && (quoted ? *p != '"' : (*p != ',' && *p != '}' && *p != ' ' && *p != '\n' && *p != '\r'))) {
      if (i + 1 >= n) return false;  // doesn't fit: better nothing than a cut value
      out[i++] = *p++;
    }
    if (quoted && *p != '"') return false;
    out[i] = 0;
    return i > 0;
  }
  return false;
}

#if UI_V2
// V2's auto-update: may the version found go in by itself now? The sketch says what's going on:
//   idle: the screen dimmed or off (nobody's using it)     busy: something a restart would lose is under way
//   asleep: the pet's light is off                         hour: the local hour, 0..23
// Any screen will do, not only the home screen: menus stay open when left alone, so a Knomi left on Settings (where
// it shows the update) never got it. By day only, as the restart lights the screen; with the light left off (only
// the light wakes the pet, so it can sleep all day) only in the middle of the day, when no bedroom is dark.
inline bool otaAutoNow(bool idle, bool busy, bool asleep, int hour) {
  if (!idle || busy) return false;
  return asleep ? hour >= 10 && hour < 18 : hour >= 7 && hour < 20;
}
#endif
