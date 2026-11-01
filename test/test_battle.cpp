// Reglas del combate (battle.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../battle.h"
#include <stdlib.h>

TEST(battle, tabla_de_tipos_de_gen_1) {
  CHECK_EQ((int)btlEffect(TY_FUEGO, TY_PLANTA), 4);
  CHECK_EQ((int)btlEffect(TY_AGUA, TY_FUEGO), 4);
  CHECK_EQ((int)btlEffect(TY_PLANTA, TY_AGUA), 4);
  CHECK_EQ((int)btlEffect(TY_FUEGO, TY_AGUA), 1);
  CHECK_EQ((int)btlEffect(TY_NORMAL, TY_ROCA), 1);
  CHECK_EQ((int)btlEffect(TY_NORMAL, TY_FANTASMA), 0);
  CHECK_EQ((int)btlEffect(TY_ELECTRICO, TY_TIERRA), 0);
  CHECK_EQ((int)btlEffect(TY_FANTASMA, TY_PSIQUICO), 0);  // el fallo famoso de gen 1
  CHECK_EQ((int)btlEffect(TY_HIELO, TY_DRAGON), 4);
  CHECK_EQ((int)btlEffect(TY_NORMAL, TY_NORMAL), 2);
}

TEST(battle, tabla_completa_sin_valores_raros) {
  for (int a = 0; a < TY_COUNT; a++)
    for (int d = 0; d < TY_COUNT; d++) {
      int e = btlEffect(a, d);
      CHECK(e == 0 || e == 1 || e == 2 || e == 4);
    }
}

static const char *moveName(int16_t dex, uint8_t lvl, uint8_t slot) {
  return MOVE_TBL[btlMovesFor(dex, lvl, 1).ids[slot]].name;
}

static bool sameName(const char *a, const char *b) { return strcmp(a, b) == 0; }

TEST(battle, cada_especie_usa_sus_ataques_de_red_blue) {
  CHECK_EQ((int)btlMovesFor(102, 20, 1).count, 1);  // Exeggcute solo tiene BARRAGE a ese nivel
  CHECK(sameName(moveName(102, 20, 0), "BARRAGE"));
  CHECK_EQ((int)btlMovesFor(1, 5, 4).count, 1);
  CHECK(sameName(moveName(1, 5, 0), "TACKLE"));
  CHECK(sameName(moveName(1, 21, 0), "TACKLE"));
  CHECK(sameName(moveName(1, 21, 1), "VINE WHIP"));
  CHECK(sameName(moveName(4, 30, 0), "SLASH"));
  CHECK(sameName(moveName(4, 30, 1), "EMBER"));  // no se queda sin ataque de su tipo
  CHECK(sameName(moveName(25, 1, 0), "THUNDERSHOCK"));
}

TEST(battle, sin_ataques_que_dañen_usa_struggle) {
  CHECK(sameName(moveName(63, 50, 0), "STRUGGLE"));  // Abra solo tiene TELEPORT
  CHECK(sameName(moveName(129, 14, 0), "STRUGGLE"));  // Magikarp: SPLASH no hace nada
  CHECK(sameName(moveName(129, 15, 0), "TACKLE"));
}

TEST(battle, conserva_lo_aprendido_antes_de_evolucionar) {
  CHECK(sameName(moveName(11, 8, 0), "TACKLE"));  // Metapod, de cuando era Caterpie
  CHECK(sameName(moveName(14, 8, 0), "POISON STING"));
}

TEST(battle, ditto_copia_los_ataques_del_rival) {
  BtlMoves m = btlMovesFor(DEX_DITTO, 30, 25);
  BtlMoves p = btlMovesFor(25, 30, 1);
  CHECK_EQ((int)m.ids[0], (int)p.ids[0]);
  CHECK_EQ((int)m.ids[1], (int)p.ids[1]);
  CHECK(sameName(MOVE_TBL[btlMovesFor(DEX_DITTO, 30, DEX_DITTO).ids[0]].name, "STRUGGLE"));
}

TEST(battle, aviso_de_ataque_nuevo_una_sola_vez) {
  BtlMoves one = btlMovesFor(1, 5, 4), two = btlMovesFor(1, 13, 4);
  CHECK_EQ((int)btlMoveToAnnounce(1, one), -1);  // mascota nueva: nada que anunciar
  CHECK_EQ((int)btlMoveToAnnounce(1, two), (int)two.newest);
  CHECK_EQ((int)btlMoveToAnnounce(2, two), -1);  // partida antigua que ya tenia dos
  CHECK_EQ((int)btlMoveToAnnounce(btlMovesSeenValue(two), two), -1);
  CHECK_EQ((int)btlMoveToAnnounce(btlMovesSeenValue(one), two), (int)two.newest);
  CHECK(sameName(MOVE_TBL[two.newest].name, "VINE WHIP"));
}

TEST(battle, los_ataques_caben_en_el_boton) {
  for (size_t i = 0; i < sizeof(MOVE_TBL) / sizeof(MOVE_TBL[0]); i++) CHECK(strlen(MOVE_TBL[i].name) <= 13);
}

TEST(battle, dano_efectividad_y_stab) {
  BtlMove ember = { "EMBER", TY_FUEGO, 60 };
  uint16_t base = btlDamage(20, ember, 50, 50, TY_NORMAL, TY_NORMAL, 100);
  uint16_t stab = btlDamage(20, ember, 50, 50, TY_FUEGO, TY_NORMAL, 100);
  uint16_t sup = btlDamage(20, ember, 50, 50, TY_NORMAL, TY_PLANTA, 100);
  uint16_t weak = btlDamage(20, ember, 50, 50, TY_NORMAL, TY_AGUA, 100);
  CHECK(stab > base);
  CHECK_EQ((int)sup, (int)base * 2);
  CHECK(weak < base);
  CHECK(weak >= 1);
  CHECK_EQ((int)btlDamage(20, BtlMove{ "TACKLE", TY_NORMAL, 40 }, 50, 50, TY_NORMAL, TY_FANTASMA, 100), 0);
}

TEST(battle, nivel_tope_y_rival_cercano) {
  CHECK_EQ((int)btlCapLevel(999), BTL_LEVEL_CAP);
  CHECK_EQ((int)btlWildLevel(1, 0), 2);
  CHECK_EQ((int)btlWildLevel(100, 3), 100);
  for (int r = 0; r < 4; r++) {
    int l = btlWildLevel(50, r);
    CHECK(l >= 47 && l <= 50);
  }
}

// combate entero con la IA del rival, el jugador usa su mejor ataque
static bool simulate(int16_t mine, int16_t foeDex, uint8_t lvl, uint8_t trained) {
  const DexEntry &d = DEX_TBL[mine];
  BtlFighter me;
  me.dex = mine;
  me.level = lvl;
  me.hp = me.maxHp = btlMaxHp(d.bHp, lvl);
  me.atk = btlStat(d.bAtk, 100, lvl, trained);
  me.def = btlStat(d.bDef, 100, lvl, 0);
  me.spe = btlStat(d.bSpe, 100, lvl, 0);
  BtlFighter foe = btlMakeWild(foeDex, btlWildLevel(lvl, rand() % 4), btlWildGene(rand() % 16));
  uint8_t myT = d.type, foeT = DEX_TBL[foeDex].type;
  BtlMoves myMoves = btlMovesFor(mine, lvl, foeDex), foeMoves = btlMovesFor(foeDex, foe.level, mine);
  for (int turn = 0; turn < 200; turn++) {
    uint16_t best = 0;
    for (int m = 0; m < myMoves.count; m++) {
      uint16_t x = btlDamage(lvl, MOVE_TBL[myMoves.ids[m]], me.atk, foe.def, myT, foeT, 85 + rand() % 16);
      if (x > best) best = x;
    }
    BtlMove fm = MOVE_TBL[foeMoves.ids[btlWildMove(foeMoves, rand() % 100)]];
    uint16_t back = btlDamage(foe.level, fm, foe.atk, me.def, foeT, myT, 85 + rand() % 16);
    bool first = btlPlayerFirst(me.spe, foe.spe, rand() % 2);
    if (first) {
      foe.hp = best >= foe.hp ? 0 : foe.hp - best;
      if (!foe.hp) return true;
    }
    me.hp = back >= me.hp ? 0 : me.hp - back;
    if (!me.hp) return false;
    if (!first) {
      foe.hp = best >= foe.hp ? 0 : foe.hp - best;
      if (!foe.hp) return true;
    }
  }
  return false;
}

static int winRate(int16_t mine, uint8_t lvl, uint8_t trained) {
  srand(1234);
  int wins = 0, n = 0;
  for (int16_t foe = 1; foe <= 151; foe++)
    for (int i = 0; i < 10; i++) {
      if (DEX_TBL[foe].rarity == R_LEGENDARIO || btlMinLevel(foe) > lvl) continue;
      n++;
      if (simulate(mine, foe, lvl, trained)) wins++;
    }
  return wins * 100 / n;
}

TEST(battle, equilibrio_ni_regalado_ni_imposible) {
  for (int16_t mine : {1, 4, 7}) {
    int fresh = winRate(mine, 5, 0);
    printf("  #%d: nivel 5 sin entrenar %d%%, nivel 30 +40 FUE %d%%\n", mine, fresh, winRate(mine, 30, 40));
    CHECK_MSG(fresh >= 40 && fresh <= 85, "una mascota nueva debe poder ganar y perder");
  }
}

// una mascota evolucionada a su nivel normal, con entrenamiento moderado
TEST(battle, evolucionar_se_nota) {
  for (int16_t mine : {3, 6, 9}) {
    int r = winRate(mine, 40, 30);
    printf("  #%d: nivel 40 +30 FUE %d%%\n", mine, r);
    CHECK(r >= 60);
  }
}

TEST(battle, entrenar_se_nota) {
  CHECK(winRate(4, 30, 60) > winRate(4, 30, 0));
}
