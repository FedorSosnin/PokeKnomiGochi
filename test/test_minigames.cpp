// Captura, quiz y visita diaria (minigames.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../minigames.h"
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
