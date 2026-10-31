// Captura, quiz y visita diaria (minigames.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../minigames.h"
#include "../pokerun.h"
#include "../pet.h"
#include "../dex.h"

TEST(catching, solo_el_tiro_directo_acierta) {
  CHECK(catchHit(233, 233));
  CHECK(catchHit(233 + CATCH_RADIUS - 1, 233));
  CHECK(!catchHit(233 + CATCH_RADIUS, 233));
  CHECK(!catchHit(100, 233));
}

TEST(quiz, tres_opciones_distintas_con_la_respuesta) {
  for (int answer = 1; answer <= 151; answer += 7)
    for (int a = 1; a <= 151; a += 11)
      for (int b = 1; b <= 151; b += 13)
        for (int pos = 0; pos < 3; pos++) {
          int16_t o[3];
          quizChoices(answer, a, b, pos, o);
          CHECK_EQ((int)o[pos], answer);
          CHECK(o[0] != o[1] && o[1] != o[2] && o[0] != o[2]);
          for (int i = 0; i < 3; i++) CHECK(o[i] >= 1 && o[i] <= 151);
        }
}

TEST(visitor, una_vez_al_dia_y_de_dia) {
  uint32_t day = 20000;
  uint8_t h = visitorHour(day);
  CHECK(h >= 9 && h <= 18);
  CHECK(!visitorDue(day, h - 1, 0));
  CHECK(visitorDue(day, h, 0));
  CHECK_MSG(!visitorDue(day, h, day), "no repite el mismo dia");
  CHECK(!visitorDue(day, 21, 0));
  CHECK(visitorDue(day + 1, 20, day));
  CHECK_MSG(!visitorDue(0, 12, 5), "sin hora valida no viene nadie");
}

TEST(visitor, la_hora_cambia_entre_dias) {
  bool differs = false;
  for (uint32_t d = 20000; d < 20030; d++) differs = differs || visitorHour(d) != visitorHour(20000);
  CHECK(differs);
}

TEST(quit, solo_de_abajo_a_arriba) {
  CHECK(swipeUpQuit(233, 400, 233, 200, 300));
  CHECK_MSG(!swipeUpQuit(233, 200, 233, 40, 300), "desde arriba no cuenta");
  CHECK_MSG(!swipeUpQuit(233, 200, 233, 400, 300), "hacia abajo no cuenta");
  CHECK_MSG(!swipeUpQuit(233, 400, 233, 340, 300), "un toque movido no cuenta");
  CHECK_MSG(!swipeUpQuit(100, 400, 300, 200, 300), "en diagonal no cuenta");
  CHECK_MSG(!swipeUpQuit(233, 400, 233, 200, 1500), "muy lento no cuenta");
}

static float runBotGame(uint32_t seed, float maxT, float frame) {
  RunState s;
  runStart(s, seed);
  while (!s.dead && s.t < maxT) runStep(s, frame, runBotWantsJump(s));
  return s.t;
}

TEST(pokerun, sin_saltar_se_choca_pronto) {
  RunState s;
  runStart(s, 7);
  while (!s.dead && s.t < 10) runStep(s, 0.045f, false);
  CHECK(s.dead);
  CHECK(s.t > 2.0f);  // da tiempo a situarse
}

TEST(pokerun, siempre_se_puede_pasar) {
  for (float frame : { 0.03f, 0.045f, 0.085f })
    for (uint32_t seed = 1; seed <= 100; seed++)
      CHECK(runBotGame(seed * 97, 300, frame) >= 300);
}

TEST(pokerun, cada_vez_mas_rapido_con_tope) {
  CHECK(runSpeed(30) > runSpeed(0));
  CHECK(runSpeed(60) > runSpeed(30));
  CHECK(runSpeed(1000) == RUN_SPEED_MAX);
  CHECK(runMinGap(0) > runMinGap(30));
  CHECK(runMinGap(1000) == RUN_GAP_MIN);
}

TEST(pokerun, el_reto_llega_entre_30_y_60_segundos) {
  // margen desde que se ve el obstaculo hasta tocarlo: holgado al principio, justo despues
  float seen0 = (RUN_SEEN_X - RUN_PET_X) / runSpeed(0), seen30 = (RUN_SEEN_X - RUN_PET_X) / runSpeed(30),
        seen60 = (RUN_SEEN_X - RUN_PET_X) / runSpeed(60);
  CHECK(seen0 > 1.2f);
  CHECK(seen30 > 0.75f);
  CHECK(seen60 < 0.65f);
}

TEST(pokerun, un_paron_no_mata) {
  RunState s;
  runStart(s, 7);
  float x;
  for (int i = 0; i < 40; i++) runStep(s, 0.045f, false);
  x = s.dist;
  runStep(s, 5.0f, false);
  CHECK(s.dist - x < runSpeed(s.t) * 0.13f);
}

TEST(pokerun, el_record_se_guarda_y_lo_viejo_no_cambia) {
  Pet p;
  p.begin();
  p.chooseStarter(1);
  p.eggTap(); p.eggTap(); p.eggTap();
  p.gameHi = 130;
  p.runResult(48);
  CHECK_EQ(p.runHi, (uint16_t)48);
  CHECK_MSG(p.gameHi == 130, "no toca el record de Tilt Catch");
  p.runResult(20);
  CHECK_EQ(p.runHi, (uint16_t)48);
}
