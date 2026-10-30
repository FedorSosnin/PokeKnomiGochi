#pragma once
#include <stdint.h>
#include "dex.h"

// Reglas del combate, sin hardware ni estado de juego (se prueban en test/test_battle.cpp).
// Version simplificada de gen 1: formula de dano y tabla de tipos reales, un solo tipo por
// especie (asi vienen en dex_data.py), y sin estados, precision ni golpes criticos.

#define BTL_LEVEL_CAP 100     // el nivel de la mascota sigue subiendo; aqui se topa para equilibrar
#define BTL_MOVE2_LEVEL 10    // nivel al que aprende el segundo ataque
#define BTL_TACKLE_POWER 40
#define BTL_TYPE_POWER 60

struct BtlFighter {
  int16_t dex = 0;
  uint8_t level = 1;
  uint16_t hp = 1, maxHp = 1, atk = 1, def = 1, spe = 1;
};

struct BtlMove { const char *name; uint8_t type; uint8_t power; };

inline uint8_t btlCapLevel(uint16_t lvl) { return lvl > BTL_LEVEL_CAP ? BTL_LEVEL_CAP : (lvl < 1 ? 1 : (uint8_t)lvl); }
inline uint8_t btlMoveCount(uint8_t lvl) { return lvl >= BTL_MOVE2_LEVEL ? 2 : 1; }

// ataque 0: PLACAJE para todos. Ataque 1: el del tipo de la especie.
inline BtlMove btlMove(uint8_t type, uint8_t idx) {
  if (idx == 0) return { "TACKLE", TY_NORMAL, BTL_TACKLE_POWER };
  static const char *const NAMES[TY_COUNT] = {
    "BODY SLAM", "EMBER", "WATER GUN", "VINE WHIP", "THUNDERSHOCK", "ICE BEAM", "KARATE CHOP",
    "POISON STING", "DIG", "CONFUSION", "LEECH LIFE", "ROCK THROW", "LICK", "DRAGON RAGE",
  };
  return { NAMES[type < TY_COUNT ? type : TY_NORMAL], type < TY_COUNT ? type : (uint8_t)TY_NORMAL, BTL_TYPE_POWER };
}

// efectividad x2 (entero): 0 = inmune, 1 = la mitad, 2 = normal, 4 = el doble
inline uint8_t btlEffect(uint8_t moveType, uint8_t defType) {
  // filas: atacante
  // columnas en el orden de TY_*: normal fuego agua planta electrico hielo lucha veneno
  // tierra psiquico bicho roca fantasma dragon
  static const char CHART[TY_COUNT][TY_COUNT + 1] = {
    /* normal    */ "22222222222102",
    /* fuego     */ "21142422224121",
    /* agua      */ "24112222422421",
    /* planta    */ "21412221421421",
    /* electrico */ "22411222022221",
    /* hielo     */ "22142122422224",
    /* lucha     */ "42222421211402",
    /* veneno    */ "22242221124112",
    /* tierra    */ "24214224221422",
    /* psiquico  */ "22222244212222",
    /* bicho     */ "21242214242222",
    /* roca      */ "24222412124222",
    /* fantasma  */ "02222222202242",
    /* dragon    */ "22222222222224",
  };
  if (moveType >= TY_COUNT || defType >= TY_COUNT) return 2;
  return (uint8_t)(CHART[moveType][defType] - '0');
}

// mismas formulas de escala que Pet::atkStat(), con el nivel topado
inline uint16_t btlStat(uint8_t base, uint8_t gene, uint8_t lvl, uint8_t trained) {
  return (uint16_t)base * gene / 100 + lvl + trained;
}
inline uint16_t btlMaxHp(uint8_t base, uint8_t lvl) { return (uint16_t)base * 2 * lvl / 100 + lvl + 10; }

// nivel al que se puede ver una especie: una evolucion no sale antes de su nivel real
inline uint8_t btlMinLevel(int16_t d) {
  if (d >= 134 && d <= 136) return DEX_TBL[DEX_EEVEE].evolveLevel;  // ramas de Eevee
  for (int16_t s = 1; s <= DEX_COUNT; s++)
    if (DEX_TBL[s].evolvesTo == d) return DEX_TBL[s].evolveLevel;
  return 1;
}

// el rival no entrena como una mascota cuidada: entrenar debe notarse
inline uint8_t btlWildTraining(uint8_t lvl) { return lvl / 3 > 30 ? 30 : lvl / 3; }

// genes del rival, en porcentaje: algo mas flojos que los de una mascota (90-110). roll en [0, 15]
inline uint8_t btlWildGene(uint8_t roll) { return 78 + (roll & 15); }

// nivel del rival: cerca del tuyo, algo por debajo. roll en [0, 3]
inline uint8_t btlWildLevel(uint8_t lvl, uint8_t roll) {
  int l = (int)lvl - 3 + (roll & 3);
  return l < 2 ? 2 : (l > BTL_LEVEL_CAP ? BTL_LEVEL_CAP : (uint8_t)l);
}

inline BtlFighter btlMakeWild(int16_t dex, uint8_t lvl, uint8_t gene) {
  const DexEntry &d = DEX_TBL[dex];
  BtlFighter f;
  uint8_t tr = btlWildTraining(lvl);
  f.dex = dex;
  f.level = lvl;
  f.hp = f.maxHp = btlMaxHp(d.bHp, lvl);
  f.atk = btlStat(d.bAtk, gene, lvl, tr);
  f.def = btlStat(d.bDef, gene, lvl, tr);
  f.spe = btlStat(d.bSpe, gene, lvl, tr);
  return f;
}

// formula de gen 1 sin criticos. roll en [85, 100], como el juego original.
inline uint16_t btlDamage(uint8_t lvl, const BtlMove &m, uint16_t atk, uint16_t def, uint8_t attackerType,
                          uint8_t defType, uint8_t roll) {
  uint8_t eff = btlEffect(m.type, defType);
  if (eff == 0) return 0;
  if (def == 0) def = 1;
  uint32_t d = ((2UL * lvl / 5 + 2) * m.power * atk / def) / 50 + 2;
  if (m.type == attackerType) d = d * 3 / 2;  // STAB
  d = d * eff / 2 * roll / 100;
  return d < 1 ? 1 : (d > 65535 ? 65535 : (uint16_t)d);
}

// empates de velocidad al azar (coin = 0/1), como en el original
inline bool btlPlayerFirst(uint16_t mySpe, uint16_t foeSpe, uint8_t coin) {
  return mySpe != foeSpe ? mySpe > foeSpe : coin != 0;
}

// el rival prefiere su ataque de tipo si lo tiene (roll en [0, 99])
inline uint8_t btlWildMove(uint8_t lvl, uint8_t roll) { return btlMoveCount(lvl) == 2 && roll < 70 ? 1 : 0; }
