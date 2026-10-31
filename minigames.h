#pragma once
#include <stdint.h>
// Reglas de la captura, el quiz y la visita diaria, sin hardware (test/test_minigames.cpp).

#define CATCH_BALLS 3         // tiros por encuentro
#define CATCH_RADIUS 48       // px logicos: el acierto tiene que ser directo, pero sin ser cruel
#define QUIZ_ROUNDS 5

inline bool catchHit(float ballX, float foeX) {
  float d = ballX - foeX;
  return d > -CATCH_RADIUS && d < CATCH_RADIUS;
}

// tres opciones distintas con la respuesta en la posicion pos. a y b son especies al azar.
inline void quizChoices(int16_t answer, int16_t a, int16_t b, uint8_t pos, int16_t out[3]) {
  while (a == answer) a = a % 151 + 1;
  while (b == answer || b == a) b = b % 151 + 1;
  pos %= 3;
  out[pos] = answer;
  out[(pos + 1) % 3] = a;
  out[(pos + 2) % 3] = b;
}

// la visita llega a una hora distinta cada dia, entre las 9 y las 18
inline uint8_t visitorHour(uint32_t day) { return 9 + ((day * 2654435761u) >> 28) % 10; }

inline bool visitorDue(uint32_t day, uint8_t hour, uint32_t lastDay) {
  return day != 0 && day != lastDay && hour >= visitorHour(day) && hour < 21;
}
