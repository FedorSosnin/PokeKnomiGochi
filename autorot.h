#pragma once
#include <stdint.h>

// Giro automatico de la pantalla del Knomi. rot = cuartos de vuelta a la derecha de la imagen sobre
// el panel. El juego siempre ve un lienzo recto: aqui solo se traducen pixeles, toques e inclinacion.
// Ejes del acelerometro respecto al panel sin girar: +X a la derecha, +Y hacia abajo; el eje que
// apunta hacia arriba lee +1 g.
#define ROT_MIN_G 0.6f       // por debajo esta casi plano: se queda como estaba
#define ROT_MARGIN_G 0.2f    // en diagonal tampoco cambia, para que no parpadee entre dos lados
#define ROT_STABLE_MS 600UL  // tiene que mantenerse asi este rato

#ifdef BOARD_HOST
inline uint8_t gPanelRot = 0;  // una ventana no gira
#else
inline uint8_t gPanelRot = 1;  // al arrancar: en la mano derecha, con el cable a la izquierda
#endif

inline uint8_t rotFromGravity(float x, float y, uint8_t cur) {
  float ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
  float top = ax > ay ? ax : ay, gap = ax > ay ? ax - ay : ay - ax;
  if (top < ROT_MIN_G || gap < ROT_MARGIN_G) return cur;
  if (ax > ay) return x > 0 ? 1 : 3;
  return y > 0 ? 2 : 0;
}

struct RotFilter {
  int8_t candidate = -1;
  uint32_t since = 0;
  void reset() { candidate = -1; }
  // true si hay que cambiar a *rot: el lado nuevo lleva ya ROT_STABLE_MS sin moverse
  bool update(float x, float y, uint32_t now, uint8_t *rot) {
    uint8_t want = rotFromGravity(x, y, *rot);
    if (want == *rot) {
      candidate = -1;
      return false;
    }
    if (candidate != want) {
      candidate = want;
      since = now;
      return false;
    }
    if (now - since < ROT_STABLE_MS) return false;
    *rot = want;
    candidate = -1;
    return true;
  }
};

// donde cae en el panel el pixel (x, y) de la imagen: indice = base + x * dx + y * dy
struct RotBlit { int32_t base, dx, dy; };
inline RotBlit rotBlit(uint8_t rot, int32_t size) {
  switch (rot & 3) {
    case 1: return { size - 1, size, -1 };
    case 2: return { size * size - 1, -1, -size };
    case 3: return { (size - 1) * size, -size, 1 };
    default: return { 0, 1, size };
  }
}

// de un punto del panel (px, py) al punto de la imagen que se ve ahi
inline void rotTouch(uint8_t rot, int32_t px, int32_t py, int32_t size, int32_t *x, int32_t *y) {
  switch (rot & 3) {
    case 1: *x = py; *y = size - 1 - px; break;
    case 2: *x = size - 1 - px; *y = size - 1 - py; break;
    case 3: *x = size - 1 - py; *y = px; break;
    default: *x = px; *y = py; break;
  }
}

// inclinar a la izquierda de la imagen es negativo, sea cual sea el lado que hace de arriba
inline float rotTilt(const float g[3], uint8_t rot) {
  switch (rot & 3) {
    case 1: return -g[1];
    case 2: return g[0];
    case 3: return g[1];
    default: return -g[0];
  }
}
