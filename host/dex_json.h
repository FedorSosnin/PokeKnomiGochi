#pragma once
// The game's Pokedex, moves and type chart as JSON for the Shiny Pokedex page (tools/make_sprite_gallery.py puts it
// in; tools/sprite_gallery.html shows it), straight from the tables and rules the game itself uses, so the page can't
// say something the game doesn't:
//   types   TYPE_NAME (dexfacts.h), in TY_* order
//   chart   btlEffect() (battle.h), a row per attacking type, a digit per defending type: x2, so 0 = no effect,
//           1 = half, 2 = normal, 4 = double
//   moves   MOVE_TBL (moves.h): [name, type, power], power as the game counts it (Red/Blue's, with accuracy, hits in
//           a row and charging turns folded in)
//   mons    one line per Pokemon (dex.h, dexfacts.h, moves.h, battle.h):
//             type, [hp, atk, def, spe] (its base stats), evo ([[into, level], ...]; Eevee's three branches),
//             rarity (0 only by evolving, 1 common, 2 rare, 3 legendary egg), min (the lowest level it can be met
//             at: btlMinLevel()), fact (DEX_FACT: what its Pokedex page says), learn ([[level, move], ...]:
//             LEARN_TBL), battle ([[from level, move, move], ...]: btlMovesFor(), the two moves on its battle
//             buttons from that level on, one when it has a single move; empty for Ditto, which copies its foe's)
//   training how your Pokemon is trained (pet.h, pet.cpp): each kind up to max; win, ATK a battle won; careHours,
//           hours of good care for 1 DEF, and graceMin, the minutes awake a need may dip under 40; levelHours of
//           age a level, battles counting up to levelCap; genes, [lowest, top of 1 star, top of 2, highest] (%);
//           games, what each mini game gives SPD: [name, its unit, per that many, this much]
// `make -C host dex` writes tools/dex.json; the host tests check that file is what this makes, and the pet's tests
// (test/test_pet.cpp) that DEX_TRAIN_GAMES is what the games really give.
#include <stdio.h>
#include <string>
#include "../src/pet.h"
#include "../src/dex.h"
#include "../src/moves.h"
#include "../src/battle.h"
#include "../src/dexfacts.h"

// what each mini game trains (SPD), as the page says it: Pet::playResult(), runResult(), surfResult(), dungeonResult()
enum DexTrainKind : uint8_t { DEX_TRAIN_BERRY, DEX_TRAIN_RUN, DEX_TRAIN_SURF, DEX_TRAIN_DUNGEON };
struct DexTrainGame { DexTrainKind kind; const char *name, *unit; uint8_t per, gain; };
static const DexTrainGame DEX_TRAIN_GAMES[] = {
  { DEX_TRAIN_BERRY, "Berry Drop", "points", 5, 1 },
  { DEX_TRAIN_RUN, "PokeRun", "seconds", 20, 1 },
  { DEX_TRAIN_SURF, "PikaSurf", "seconds", 20, 1 },
  { DEX_TRAIN_DUNGEON, "PokeDungeon", "floors", 5, 2 },
};
#define DEX_TRAIN_MAX 100   // each kind of training stops here (Pet::clamp100)
#define DEX_GENE_MIN 90     // a Pokemon's genes, % of its base stats (Pet::hatch())
#define DEX_GENE_MAX 110

static inline std::string dexJsonStr(const char *s) {
  std::string o = "\"";
  for (; *s; s++) {
    if (*s == '"' || *s == '\\') o += '\\';
    o += *s;
  }
  return o + "\"";
}

static inline std::string dexPageJson() {
  std::string out = "{\n\"types\":[";
  char b[200];
  for (int t = 0; t < TY_COUNT; t++) out += (t ? "," : "") + dexJsonStr(TYPE_NAME[t]);
  out += "],\n\"chart\":[";
  for (int a = 0; a < TY_COUNT; a++) {
    out += a ? ",\"" : "\"";
    for (int d = 0; d < TY_COUNT; d++) out += (char)('0' + btlEffect(a, d));
    out += "\"";
  }
  out += "],\n\"moves\":[";
  for (size_t i = 0; i < sizeof(MOVE_TBL) / sizeof(MOVE_TBL[0]); i++) {
    snprintf(b, sizeof(b), "%s\n[%s,%d,%d]", i ? "," : "", dexJsonStr(MOVE_TBL[i].name).c_str(), MOVE_TBL[i].type,
             MOVE_TBL[i].power);
    out += b;
  }
  snprintf(b, sizeof(b), "\n],\n\"training\":{\"max\":%d,\"win\":%d,\"careHours\":%d,\"graceMin\":%d,\"levelHours\":%d,"
           "\"levelCap\":%d,\"genes\":[%d,%d,%d,%d],\"games\":[", DEX_TRAIN_MAX, BTL_WIN_TRAIN, GOOD_CARE_MIN / 60,
           CARE_GRACE_MIN, MINUTES_PER_LEVEL / 60, BTL_LEVEL_CAP, DEX_GENE_MIN, GENE_BAND_1, GENE_BAND_2, DEX_GENE_MAX);
  out += b;
  for (size_t i = 0; i < sizeof(DEX_TRAIN_GAMES) / sizeof(DEX_TRAIN_GAMES[0]); i++) {
    const DexTrainGame &g = DEX_TRAIN_GAMES[i];
    snprintf(b, sizeof(b), "%s[%s,%s,%d,%d]", i ? "," : "", dexJsonStr(g.name).c_str(), dexJsonStr(g.unit).c_str(), g.per,
             g.gain);
    out += b;
  }
  out += "]},\n\"mons\":{";
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    const DexEntry &e = DEX_TBL[d];
    snprintf(b, sizeof(b), "%s\n\"%d\":{\"type\":%d,\"stats\":[%d,%d,%d,%d],\"evo\":[", d > 1 ? "," : "", d, e.type,
             e.bHp, e.bAtk, e.bDef, e.bSpe);
    out += b;
    if (d == DEX_EEVEE) {
      snprintf(b, sizeof(b), "[134,%d],[135,%d],[136,%d]", e.evolveLevel, e.evolveLevel, e.evolveLevel);
      out += b;
    } else if (e.evolvesTo) {
      snprintf(b, sizeof(b), "[%d,%d]", e.evolvesTo, e.evolveLevel);
      out += b;
    }
    snprintf(b, sizeof(b), "],\"rarity\":%d,\"min\":%d,\"fact\":", e.rarity, btlMinLevel(d));
    out += b;
    out += dexJsonStr(DEX_FACT[d]);
    out += ",\"learn\":[";
    for (uint16_t i = LEARN_START[d]; i < LEARN_START[d + 1]; i++) {
      snprintf(b, sizeof(b), "%s[%d,%d]", i > LEARN_START[d] ? "," : "", LEARN_TBL[i].level, LEARN_TBL[i].move);
      out += b;
    }
    out += "],\"battle\":[";
    if (d != DEX_DITTO) {
      int had = -1;
      for (int lvl = 1; lvl <= BTL_LEVEL_CAP; lvl++) {
        BtlMoves m = btlMovesFor(d, (uint8_t)lvl, d);
        int key = m.ids[0] * 256 + m.ids[1];
        if (key == had) continue;
        if (m.ids[0] == m.ids[1]) snprintf(b, sizeof(b), "%s[%d,%d]", had >= 0 ? "," : "", lvl, m.ids[0]);
        else snprintf(b, sizeof(b), "%s[%d,%d,%d]", had >= 0 ? "," : "", lvl, m.ids[0], m.ids[1]);
        out += b;
        had = key;
      }
    }
    out += "]}";
  }
  return out + "\n}\n}\n";
}
