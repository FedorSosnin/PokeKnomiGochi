// Automatic screen rotation (autorot.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../autorot.h"

TEST(autorot, each_side_up) {
  CHECK_EQ(rotFromGravity(0, -1, 1), 0);
  CHECK_EQ(rotFromGravity(1, 0, 0), 1);
  CHECK_EQ(rotFromGravity(0, 1, 0), 2);
  CHECK_EQ(rotFromGravity(-1, 0, 0), 3);
}

TEST(autorot, flat_or_diagonal_does_not_change) {
  CHECK_EQ(rotFromGravity(-0.43f, 0.11f, 1), 1);  // on the desk, measured on the board
  CHECK_EQ(rotFromGravity(0.7f, 0.7f, 2), 2);
}

TEST(autorot, only_changes_if_held) {
  RotFilter f;
  uint8_t rot = 1;
  CHECK(!f.update(0, 1, 1000, &rot));
  CHECK(!f.update(0, 1, 1000 + ROT_STABLE_MS - 1, &rot));
  CHECK_EQ(rot, 1);
  CHECK(!f.update(0, 0, 1000 + ROT_STABLE_MS, &rot));  // back to flat: starts from zero
  CHECK(!f.update(0, 1, 5000, &rot));
  CHECK(f.update(0, 1, 5000 + ROT_STABLE_MS, &rot));
  CHECK_EQ(rot, 2);
}

TEST(autorot, touch_lands_on_the_visible_pixel) {
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

TEST(autorot, rotating_right_puts_top_at_the_right) {
  RotBlit b = rotBlit(1, 240);
  int32_t i = b.base + 120 * b.dx + 0 * b.dy;  // top center of the image
  CHECK_EQ(i % 240, 239);                       // ends up on the panel's right edge
  CHECK_EQ(i / 240, 120);
}

TEST(autorot, tilting_left_is_always_negative) {
  // the left side of the image goes down: the axis pointing toward it reads -1 g
  const float left[4][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 } };
  for (uint8_t r = 0; r < 4; r++) CHECK(rotTilt(left[r], r) < 0);
}
