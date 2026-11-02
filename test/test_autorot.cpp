// Giro automatico de la pantalla (autorot.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../autorot.h"

TEST(autorot, cada_lado_hacia_arriba) {
  CHECK_EQ(rotFromGravity(0, -1, 1), 0);
  CHECK_EQ(rotFromGravity(1, 0, 0), 1);
  CHECK_EQ(rotFromGravity(0, 1, 0), 2);
  CHECK_EQ(rotFromGravity(-1, 0, 0), 3);
}

TEST(autorot, plano_o_en_diagonal_no_cambia) {
  CHECK_EQ(rotFromGravity(-0.43f, 0.11f, 1), 1);  // en la mesa, medido en la placa
  CHECK_EQ(rotFromGravity(0.7f, 0.7f, 2), 2);
}

TEST(autorot, solo_cambia_si_se_mantiene) {
  RotFilter f;
  uint8_t rot = 1;
  CHECK(!f.update(0, 1, 1000, &rot));
  CHECK(!f.update(0, 1, 1000 + ROT_STABLE_MS - 1, &rot));
  CHECK_EQ(rot, 1);
  CHECK(!f.update(0, 0, 1000 + ROT_STABLE_MS, &rot));  // vuelve a plano: empieza de cero
  CHECK(!f.update(0, 1, 5000, &rot));
  CHECK(f.update(0, 1, 5000 + ROT_STABLE_MS, &rot));
  CHECK_EQ(rot, 2);
}

TEST(autorot, el_toque_cae_en_el_pixel_que_se_ve) {
  const int32_t S = 240;
  for (uint8_t r = 0; r < 4; r++) {
    RotBlit b = rotBlit(r, S);
    for (int32_t y = 0; y < S; y += 7)
      for (int32_t x = 0; x < S; x += 7) {
        int32_t i = b.base + x * b.dx + y * b.dy;
        CHECK(i >= 0 && i < S * S);
        int32_t ox, oy;
        rotTouch(r, i % S, i / S, S, &ox, &oy);
        CHECK_EQ(ox, x);
        CHECK_EQ(oy, y);
      }
  }
}

TEST(autorot, girar_a_la_derecha_lleva_arriba_a_la_derecha) {
  RotBlit b = rotBlit(1, 240);
  int32_t i = b.base + 120 * b.dx + 0 * b.dy;  // centro de arriba de la imagen
  CHECK_EQ(i % 240, 239);                       // queda en el borde derecho del panel
  CHECK_EQ(i / 240, 120);
}

TEST(autorot, inclinar_a_la_izquierda_siempre_es_negativo) {
  // el lado izquierdo de la imagen baja: lee -1 g el eje que apunta hacia el
  const float left[4][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 } };
  for (uint8_t r = 0; r < 4; r++) CHECK(rotTilt(left[r], r) < 0);
}
