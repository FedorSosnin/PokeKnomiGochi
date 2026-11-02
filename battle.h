#pragma once
#include <stdint.h>
#include "dex.h"
#include "moves.h"

// Battle rules, without hardware or game state (tested in test/test_battle.cpp).
// Simplified gen 1 version: damage formula and real type table, a single type per
// species (as they come in dex_data.py), and no status, accuracy or critical hits. The moves
// are each species' Red/Blue ones (moves.h).

#define BTL_LEVEL_CAP 100     // the pet's level keeps rising; it is capped here to balance
#define DEX_DITTO 132         // TRANSFORM: fights with the rival's moves

struct BtlFighter {
  int16_t dex = 0;
  uint8_t level = 1;
  uint16_t hp = 1, maxHp = 1, atk = 1, def = 1, spe = 1;
};

// the two moves of a battle, MOVE_TBL indices
struct BtlMoves { uint8_t count; uint8_t ids[2]; uint8_t newest; };

inline uint8_t btlCapLevel(uint16_t lvl) { return lvl > BTL_LEVEL_CAP ? BTL_LEVEL_CAP : (lvl < 1 ? 1 : (uint8_t)lvl); }

// What was learned up to its level in Red/Blue: the best of its type on the right and the best
// of the others on the left. The games keep 4, and with only the last ones learned a
// level 30 Charmander would be left with no fire move.
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
  if (best < 0 && other < 0) return { 1, { 0, 0 }, 0 };  // STRUGGLE: knows nothing that does damage
  if (best < 0 || other < 0) {
    uint8_t only = LEARN_TBL[best < 0 ? other : best].move;
    return { 1, { only, only }, only };
  }
  return { 2, { LEARN_TBL[other].move, LEARN_TBL[best].move }, LEARN_TBL[best > other ? best : other].move };
}

// The last announced move is saved as id + 3: old saves store 1 or 2
// (how many it knew). Returns the move to announce, or -1 if there is nothing new.
inline uint8_t btlMovesSeenValue(const BtlMoves &m) { return m.newest + 3; }
inline int16_t btlMoveToAnnounce(uint8_t seen, const BtlMoves &m) {
  if (seen == btlMovesSeenValue(m) || m.newest == 0) return -1;
  if (seen <= 2 && m.count <= seen) return -1;
  return m.newest;
}

// effectiveness x2 (integer): 0 = immune, 1 = half, 2 = normal, 4 = double
inline uint8_t btlEffect(uint8_t moveType, uint8_t defType) {
  // rows: attacker
  // columns in TY_* order: normal fire water grass electric ice fighting poison
  // ground psychic bug rock ghost dragon flying
  static const char CHART[TY_COUNT][TY_COUNT + 1] = {
    /* normal    */ "222222222221022",
    /* fire      */ "211424222241212",
    /* water     */ "241122224224212",
    /* grass     */ "214122214214211",
    /* electric  */ "224112220222214",
    /* ice       */ "221421224222244",
    /* fighting  */ "422224212114021",
    /* poison    */ "222422211241122",
    /* ground    */ "242142242214220",
    /* psychic   */ "222222442122222",
    /* bug       */ "212422142422221",
    /* rock      */ "242224121242224",
    /* ghost     */ "022222222022422",
    /* dragon    */ "222222222222242",
    /* flying    */ "222412422241222",
  };
  if (moveType >= TY_COUNT || defType >= TY_COUNT) return 2;
  return (uint8_t)(CHART[moveType][defType] - '0');
}

// same scaling formulas as Pet::atkStat(), with the level capped
inline uint16_t btlStat(uint8_t base, uint8_t gene, uint8_t lvl, uint8_t trained) {
  return (uint16_t)base * gene / 100 + lvl + trained;
}
inline uint16_t btlMaxHp(uint8_t base, uint8_t lvl) { return (uint16_t)base * 2 * lvl / 100 + lvl + 10; }

// level at which a species can be seen: an evolution doesn't appear before its real level
inline uint8_t btlMinLevel(int16_t d) {
  if (d >= 134 && d <= 136) return DEX_TBL[DEX_EEVEE].evolveLevel;  // Eevee branches
  for (int16_t s = 1; s <= DEX_COUNT; s++)
    if (DEX_TBL[s].evolvesTo == d) return DEX_TBL[s].evolveLevel;
  return 1;
}

// the rival doesn't train like a cared-for pet: training should be noticeable
inline uint8_t btlWildTraining(uint8_t lvl) { return lvl / 3 > 30 ? 30 : lvl / 3; }

// rival's genes, as a percentage: somewhat weaker than a pet's (90-110). roll in [0, 15]
inline uint8_t btlWildGene(uint8_t roll) { return 78 + (roll & 15); }

// rival's level: near yours, somewhat below. roll in [0, 3]
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

// gen 1 formula without crits. roll in [85, 100], like the original game.
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

// speed ties at random (coin = 0/1), like the original
inline bool btlPlayerFirst(uint16_t mySpe, uint16_t foeSpe, uint8_t coin) {
  return mySpe != foeSpe ? mySpe > foeSpe : coin != 0;
}

// the rival prefers its type move if it has one (roll in [0, 99])
inline uint8_t btlWildMove(const BtlMoves &m, uint8_t roll) { return m.count == 2 && roll < 70 ? 1 : 0; }
