#pragma once
#include <stdint.h>
#include "dex.h"
#include "moves.h"

// Reglas del combate, sin hardware ni estado de juego (se prueban en test/test_battle.cpp).
// Version simplificada de gen 1: formula de dano y tabla de tipos reales, un solo tipo por
// especie (asi vienen en dex_data.py), y sin estados, precision ni golpes criticos. Los ataques
// son los de cada especie en Red/Blue (moves.h).

#define BTL_LEVEL_CAP 100     // el nivel de la mascota sigue subiendo; aqui se topa para equilibrar
#define DEX_DITTO 132         // TRANSFORM: pelea con los ataques del rival

struct BtlFighter {
  int16_t dex = 0;
  uint8_t level = 1;
  uint16_t hp = 1, maxHp = 1, atk = 1, def = 1, spe = 1;
};

// los dos ataques de un combate, indices de MOVE_TBL
struct BtlMoves { uint8_t count; uint8_t ids[2]; uint8_t newest; };

inline uint8_t btlCapLevel(uint16_t lvl) { return lvl > BTL_LEVEL_CAP ? BTL_LEVEL_CAP : (lvl < 1 ? 1 : (uint8_t)lvl); }

// Lo aprendido hasta su nivel en Red/Blue: el mejor de su tipo a la derecha y el mejor
// de los demas a la izquierda. Los juegos guardan 4, y solo con los ultimos aprendidos un
// Charmander de nivel 30 se quedaria sin ningun ataque de fuego.
inline BtlMoves btlMovesFor(int16_t dex, uint8_t lvl, int16_t foeDex) {
  if (dex == DEX_DITTO) dex = foeDex;
  uint8_t type = DEX_TBL[dex].type;
  int best = -1, other = -1;
  uint16_t from = LEARN_START[dex], to = LEARN_START[dex + 1];
  for (uint16_t i = from; i < to && LEARN_TBL[i].level <= lvl; i++) {
    uint8_t p = MOVE_TBL[LEARN_TBL[i].move].power;
    if (MOVE_TBL[LEARN_TBL[i].move].type == type && (best < 0 || p >= MOVE_TBL[LEARN_TBL[best].move].power)) best = i;
  }
  for (uint16_t i = from; i < to && LEARN_TBL[i].level <= lvl; i++) {
    if ((int)i == best) continue;
    uint8_t p = MOVE_TBL[LEARN_TBL[i].move].power;
    if (other < 0 || p >= MOVE_TBL[LEARN_TBL[other].move].power) other = i;
  }
  if (best < 0 && other < 0) return { 1, { 0, 0 }, 0 };  // STRUGGLE: no conoce nada que haga dano
  if (best < 0 || other < 0) {
    uint8_t only = LEARN_TBL[best < 0 ? other : best].move;
    return { 1, { only, only }, only };
  }
  return { 2, { LEARN_TBL[other].move, LEARN_TBL[best].move }, LEARN_TBL[best > other ? best : other].move };
}

// El ultimo ataque anunciado se guarda como id + 3: las partidas antiguas guardan 1 o 2
// (cuantos conocia). Devuelve el ataque que avisar, o -1 si no hay nada nuevo.
inline uint8_t btlMovesSeenValue(const BtlMoves &m) { return m.newest + 3; }
inline int16_t btlMoveToAnnounce(uint8_t seen, const BtlMoves &m) {
  if (seen == btlMovesSeenValue(m) || m.newest == 0) return -1;
  if (seen <= 2 && m.count <= seen) return -1;
  return m.newest;
}

// efectividad x2 (entero): 0 = inmune, 1 = la mitad, 2 = normal, 4 = el doble
inline uint8_t btlEffect(uint8_t moveType, uint8_t defType) {
  // filas: atacante
  // columnas en el orden de TY_*: normal fuego agua planta electrico hielo lucha veneno
  // tierra psiquico bicho roca fantasma dragon volador
  static const char CHART[TY_COUNT][TY_COUNT + 1] = {
    /* normal    */ "222222222221022",
    /* fuego     */ "211424222241212",
    /* agua      */ "241122224224212",
    /* planta    */ "214122214214211",
    /* electrico */ "224112220222214",
    /* hielo     */ "221421224222244",
    /* lucha     */ "422224212114021",
    /* veneno    */ "222422211241122",
    /* tierra    */ "242142242214220",
    /* psiquico  */ "222222442122222",
    /* bicho     */ "212422142422221",
    /* roca      */ "242224121242224",
    /* fantasma  */ "022222222022422",
    /* dragon    */ "222222222222242",
    /* volador   */ "222412422241222",
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
inline uint8_t btlWildMove(const BtlMoves &m, uint8_t roll) { return m.count == 2 && roll < 70 ? 1 : 0; }
