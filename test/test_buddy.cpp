// Protocolo y reglas del buddy (buddy.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../buddy.h"
#include "../battle.h"

static BuddyMsg hello(uint32_t from, int16_t dex) {
  BuddyMsg m;
  buddyInit(m, BM_HELLO, from, 0, 0);
  m.dex = dex;
  m.level = 20;
  strcpy(m.nick, "SPARKY");
  return m;
}

static bool parseCopy(const BuddyMsg &in, BuddyMsg &out) {
  return buddyParse((const uint8_t *)&in, sizeof(in), out);
}

TEST(buddy, el_formato_no_cambia_sin_querer) { CHECK_EQ((int)sizeof(BuddyMsg), 65); }

TEST(buddy, acepta_un_saludo_valido) {
  BuddyMsg out;
  CHECK(parseCopy(hello(5, 25), out));
  CHECK_EQ(out.dex, (int16_t)25);
  CHECK(strcmp(out.nick, "SPARKY") == 0);
}

TEST(buddy, rechaza_lo_que_no_tiene_sentido) {
  BuddyMsg out, m = hello(5, 25);
  CHECK(!buddyParse((const uint8_t *)&m, sizeof(m) - 1, out));
  m = hello(5, 25); m.magic[0] = 'X';
  CHECK(!parseCopy(m, out));
  m = hello(5, 25); m.ver = BUDDY_VER + 1;
  CHECK(!parseCopy(m, out));
  m = hello(5, 25); m.type = BM_END;
  CHECK(!parseCopy(m, out));
  m = hello(5, 0);
  CHECK(!parseCopy(m, out));
  m = hello(5, 152);
  CHECK(!parseCopy(m, out));
  m = hello(5, 25); m.level = 0;
  CHECK(!parseCopy(m, out));
  m = hello(0, 25);
  CHECK(!parseCopy(m, out));

  buddyInit(m, BM_FIGHTER, 5, 6, 1);
  m.dex = 25; m.level = 20; m.hp = 50; m.atk = 40; m.def = 0; m.spe = 40;
  CHECK_MSG(!parseCopy(m, out), "defensa 0 dividiria entre 0");
  m.def = 40; m.atk = 60000;
  CHECK(!parseCopy(m, out));
  buddyInit(m, BM_MOVE, 5, 6, 1);
  m.move = 2;
  CHECK(!parseCopy(m, out));
  buddyInit(m, BM_INVITE, 5, 6, 1);
  m.kind = 9;
  CHECK(!parseCopy(m, out));
}

TEST(buddy, el_apodo_raro_no_llega_a_la_pantalla) {
  BuddyMsg out, m = hello(5, 25);
  memset(m.nick, 'A', sizeof(m.nick));  // sin terminador
  CHECK(parseCopy(m, out));
  CHECK_EQ((int)strlen(out.nick), (int)sizeof(out.nick) - 1);
  m = hello(5, 25);
  m.nick[1] = (char)0xC3;
  CHECK(parseCopy(m, out));
  CHECK_EQ((int)strlen(out.nick), 0);
}

TEST(buddy, llega_se_queda_y_se_va) {
  BuddyPeer p;
  BuddyMsg m = hello(5, 25);
  CHECK(buddyPeerHello(p, m, 1000));
  CHECK(!buddyPeerHello(p, m, 2000));
  CHECK(!buddyPeerGone(p, 2000 + BUDDY_TIMEOUT_MS));
  CHECK(buddyPeerGone(p, 2001 + BUDDY_TIMEOUT_MS));
  BuddyMsg other = hello(9, 4);
  CHECK(!buddyPeerHello(p, other, 3000));
  CHECK_EQ(p.id, (uint32_t)5);
}

TEST(buddy, la_pokedex_del_buddy) {
  uint8_t reg[19] = {};
  reg[0] = 1;
  reg[18] = 1 << 6;  // #151
  CHECK(buddyRegHas(reg, 1));
  CHECK(buddyRegHas(reg, 151));
  CHECK(!buddyRegHas(reg, 2));
  CHECK(!buddyRegHas(reg, 0));
  CHECK(!buddyRegHas(reg, 152));
}

// las dos placas tienen que ver exactamente el mismo combate
TEST(buddy, las_dos_placas_ven_el_mismo_combate) {
  for (uint32_t seed = 1; seed <= 300; seed++) {
    BtlFighter a = btlMakeWild(1 + seed % 151, 30, 90), b = btlMakeWild(1 + (seed * 7) % 151, 30, 90);
    if (seed % 3 == 0) b.spe = a.spe;  // empates de velocidad
    BtlMoves am = btlMovesFor(a.dex, a.level, b.dex), bm = btlMovesFor(b.dex, b.level, a.dex);
    BtlFighter a1 = a, b1 = b, a2 = a, b2 = b;  // lo que ve cada placa
    uint32_t r1 = seed, r2 = seed, idA = 100, idB = 200;
    for (int turn = 0; turn < 300 && a1.hp && b1.hp; turn++) {
      uint8_t aPick = turn % am.count, bPick = turn % bm.count;
      bool f1 = pvpMineFirst(a1.spe, b1.spe, idA, idB, r1);  // placa A: "mine" es a
      bool f2 = pvpMineFirst(b2.spe, a2.spe, idB, idA, r2);  // placa B: "mine" es b
      CHECK_MSG(f1 != f2, "una ataca primero en las dos");
      for (int step = 0; step < 2 && a1.hp && b1.hp; step++) {
        bool aHits = (step == 0) == f1;
        uint8_t ra = pvpRoll(r1), rb = pvpRoll(r2);
        CHECK_EQ(ra, rb);
        const BtlMove &m = MOVE_TBL[aHits ? am.ids[aPick] : bm.ids[bPick]];
        BtlFighter &at = aHits ? a1 : b1, &df = aHits ? b1 : a1;
        uint16_t d = btlDamage(at.level, m, at.atk, df.def, DEX_TBL[at.dex].type, DEX_TBL[df.dex].type, ra);
        df.hp = d >= df.hp ? 0 : df.hp - d;
        BtlFighter &at2 = aHits ? a2 : b2, &df2 = aHits ? b2 : a2;
        uint16_t d2 = btlDamage(at2.level, m, at2.atk, df2.def, DEX_TBL[at2.dex].type, DEX_TBL[df2.dex].type, rb);
        df2.hp = d2 >= df2.hp ? 0 : df2.hp - d2;
      }
      CHECK_EQ(a1.hp, a2.hp);
      CHECK_EQ(b1.hp, b2.hp);
    }
  }
}
