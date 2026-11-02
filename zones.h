#pragma once
#include <stdint.h>
#include "dex.h"

// Zonas de Combate. Reparto a mano por habitat, no por tipo: cada especie solo tiene un tipo y
// ninguna es Volador, asi que el tipo no sirve para meter a los pajaros en el campo ni a Zubat
// en la cueva. Cada zona guarda un legendario, que sale cuando el resto de la zona esta completo.
enum : uint8_t { Z_FOREST, Z_BEACH, Z_CAVE, Z_MANSION, Z_FIELD, ZONE_COUNT };

static const uint8_t ZONE_OF[DEX_COUNT] = {
  0, 0, 0, 4, 4, 4, 1, 1, 1, 0,  // 1
  0, 0, 0, 0, 0, 4, 4, 4, 4, 4,  // 11
  4, 4, 0, 0, 0, 0, 2, 2, 0, 0,  // 21
  0, 0, 0, 0, 2, 2, 0, 0, 4, 4,  // 31
  2, 2, 0, 0, 0, 0, 0, 0, 0, 2,  // 41
  2, 0, 0, 1, 1, 0, 0, 4, 4, 1,  // 51
  1, 1, 3, 3, 3, 2, 2, 2, 0, 0,  // 61
  0, 1, 1, 2, 2, 2, 4, 4, 1, 1,  // 71
  3, 3, 4, 4, 4, 1, 1, 3, 3, 1,  // 81
  1, 3, 3, 3, 2, 3, 3, 1, 1, 3,  // 91
  3, 0, 0, 2, 2, 2, 2, 4, 3, 3,  // 101
  2, 2, 4, 0, 4, 1, 1, 1, 1, 1,  // 111
  1, 3, 0, 1, 3, 2, 0, 4, 1, 1,  // 121
  1, 3, 4, 1, 4, 4, 3, 2, 2, 2,  // 131
  2, 2, 4, 1, 4, 2, 2, 2, 2, 3,  // 141
  0,  // 151
};

static const uint8_t ZONE_LEGEND[ZONE_COUNT] = { 151, 144, 146, 150, 145 };  // Mew, Articuno, Moltres, Mewtwo, Zapdos

#define ZONE_NEW_PCT 70          // cuanto se prefiere lo que falta (como el combate al azar)
#define ZONE_LEGEND_NEW_PCT 25   // zona completa y legendario sin atrapar
#define ZONE_LEGEND_SEEN_PCT 3   // ya atrapado: puede volver, pero poco

inline uint8_t zoneOf(int16_t dex) { return ZONE_OF[dex - 1]; }

inline int zoneSize(uint8_t z) {
  int n = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) n += zoneOf(d) == z;
  return n;
}

// el i-esimo de la zona en orden de Pokedex, con el legendario siempre al final. 0 = no hay
inline int16_t zoneMember(uint8_t z, int i) {
  for (int16_t d = 1; d <= DEX_COUNT; d++)
    if (zoneOf(d) == z && d != ZONE_LEGEND[z] && i-- == 0) return d;
  return i == 0 ? ZONE_LEGEND[z] : 0;
}
