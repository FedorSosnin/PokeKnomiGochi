#pragma once
#include <math.h>
#include <stdint.h>
#include "zones.h"

// Scenery for each zone: a 100x100 RGB565 picture, independent of the screen. The menu
// draws it as is in a round window and the battle scales it up as a background.
#define ZONE_ART 100

struct ZoneCanvas {
  uint16_t *px;
  static uint16_t rgb(uint32_t c) { return ((c >> 19 & 31) << 11) | ((c >> 10 & 63) << 5) | (c >> 3 & 31); }
  void rect(float x, float y, float w, float h, uint32_t c) {
    uint16_t v = rgb(c);
    int x0 = (int)floorf(x), y0 = (int)floorf(y), x1 = (int)floorf(x + w), y1 = (int)floorf(y + h);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > ZONE_ART) x1 = ZONE_ART;
    if (y1 > ZONE_ART) y1 = ZONE_ART;
    for (int r = y0; r < y1; r++)
      for (int q = x0; q < x1; q++) px[r * ZONE_ART + q] = v;
  }
  void oval(int cx, int cy, int rx, int ry, uint32_t c) {
    for (int dy = -ry; dy <= ry; dy++) {
      float k = 1.0f - (float)(dy * dy) / (float)(ry * ry);
      float half = rx * sqrtf(k < 0 ? 0 : k);
      rect(cx - half, cy + dy, half * 2 + 1, 1, c);
    }
  }
  void pine(float x, int base, int h, uint32_t c) {
    for (int i = 0; i < h; i++) {
      int w = 1 + (i * 42 / 100) * 2 - (i % 5 == 4 ? 2 : 0);
      rect(x - w / 2.0f, base - h + i, w, 1, c);
    }
  }
  void grass(int x, int y, uint32_t c) {
    rect(x, y, 1, 4, c);
    rect(x + 2, y + 1, 1, 3, c);
    rect(x - 2, y + 1, 1, 3, c);
  }
  void crystal(int x, int y, int h) {
    rect(x, y, 4, h, 0x6fe0d4);
    rect(x + 1, y - 2, 2, 2, 0xa7f3ea);
    rect(x + 3, y + 1, 1, h - 2, 0x3faaa2);
  }
};

inline void zoneArtDraw(uint16_t *px, uint8_t z) {
  ZoneCanvas g{px};
  if (z == Z_FOREST) {
    g.rect(0, 0, 100, 100, 0xa9d8ea);
    g.rect(0, 26, 100, 22, 0x5d9e78);
    g.pine(8, 62, 34, 0x2d6b4a); g.pine(28, 62, 28, 0x2d6b4a); g.pine(70, 62, 30, 0x2d6b4a); g.pine(92, 62, 36, 0x2d6b4a);
    g.rect(0, 60, 100, 40, 0x4f9a4a);
    g.rect(0, 60, 100, 3, 0x3f8440);
    const int t[3][2] = { { 18, 44 }, { 50, 38 }, { 84, 46 } };
    for (auto &p : t) {
      g.rect(p[0] - 2, 66, 4, 12, 0x6a4a2e);
      g.pine(p[0], 70, p[1], 0x1f5238);
    }
    g.grass(30, 82, 0x2f7a38); g.grass(62, 88, 0x2f7a38); g.grass(10, 90, 0x2f7a38); g.grass(76, 78, 0x2f7a38);
    g.rect(40, 84, 3, 2, 0xe9587a);
    g.rect(41, 83, 1, 4, 0xe9587a);
  } else if (z == Z_BEACH) {
    g.rect(0, 0, 100, 100, 0x9fd8f2);
    g.oval(74, 18, 9, 9, 0xffe79f);
    g.rect(14, 14, 18, 3, 0xffffff);
    g.rect(18, 11, 10, 3, 0xffffff);
    g.rect(0, 34, 100, 28, 0x3f8fc4);
    g.rect(0, 34, 100, 3, 0x2f76a8);
    const int w[5][3] = { { 8, 42, 16 }, { 46, 40, 14 }, { 70, 48, 18 }, { 22, 52, 18 }, { 56, 56, 12 } };
    for (auto &p : w) g.rect(p[0], p[1], p[2], 2, 0xc5eaf7);
    g.rect(0, 62, 100, 38, 0xecd593);
    g.rect(0, 61, 100, 2, 0xffffff);
    const int s[5][2] = { { 20, 72 }, { 66, 80 }, { 40, 88 }, { 84, 70 }, { 12, 90 } };
    for (auto &p : s) g.rect(p[0], p[1], 2, 1, 0xcdb26f);
    g.rect(60, 68, 2, 3, 0xe86f5f);
    g.rect(58, 70, 6, 2, 0xe86f5f);
  } else if (z == Z_CAVE) {
    g.rect(0, 0, 100, 100, 0x201c2c);
    g.rect(28, 22, 44, 44, 0x2f2a3f);
    g.rect(38, 16, 24, 50, 0x3a3450);
    const int up[6][3] = { { 4, 12, 26 }, { 20, 8, 16 }, { 36, 10, 12 }, { 54, 8, 18 }, { 70, 12, 24 }, { 88, 12, 30 } };
    for (auto &p : up)
      for (int i = 0; i < p[2]; i++) {
        float q = p[1] * (1.0f - (float)i / p[2]);
        g.rect(p[0] + (p[1] - q) / 2, i, q + 1, 1, 0x494258);
      }
    g.rect(0, 68, 100, 32, 0x544b63);
    g.rect(0, 68, 100, 2, 0x6a6078);
    const int low[3][3] = { { 2, 16, 22 }, { 80, 18, 26 }, { 62, 8, 10 } };
    for (auto &p : low)
      for (int i = 0; i < p[2]; i++) {
        float q = p[1] * ((float)i / p[2]);
        g.rect(p[0] + (p[1] - q) / 2, 70 - p[2] + i, q + 1, 1, 0x3d3650);
      }
    g.crystal(22, 66, 12); g.crystal(28, 70, 8); g.crystal(70, 74, 12); g.crystal(76, 78, 8);
  } else if (z == Z_MANSION) {
    const uint32_t m = 0x0d0b20;
    g.rect(0, 0, 100, 100, 0x1b1838);
    g.rect(0, 42, 100, 26, 0x2a2350);
    g.oval(78, 18, 9, 9, 0xe8eef5);
    g.oval(82, 15, 8, 8, 0x1b1838);
    const int st[5][2] = { { 12, 12 }, { 34, 8 }, { 58, 20 }, { 92, 34 }, { 6, 32 } };
    for (auto &p : st) g.rect(p[0], p[1], 1, 1, 0xffffff);
    g.rect(24, 40, 52, 38, m);
    g.rect(40, 30, 20, 12, m);
    for (int i = 0; i < 16; i++) {
      g.rect(50 - i, 16 + i, i * 2, 1, m);
      g.rect(30 - i * 0.6f, 26 + i, i * 1.2f + 1, 1, m);
      g.rect(70 - i * 0.6f, 26 + i, i * 1.2f + 1, 1, m);
    }
    const int win[6][2] = { { 29, 48 }, { 41, 48 }, { 55, 48 }, { 67, 48 }, { 29, 62 }, { 67, 62 } };
    for (auto &p : win) {
      g.rect(p[0], p[1], 5, 7, 0xf0c04a);
      g.rect(p[0] + 2, p[1], 1, 7, m);
    }
    g.rect(46, 35, 8, 8, 0xa45de0);
    g.rect(45, 62, 10, 16, 0x3a2a18);
    g.rect(0, 78, 100, 22, 0x25283a);
    g.rect(0, 78, 100, 2, 0x363a50);
    g.rect(10, 54, 3, 26, m); g.rect(7, 60, 7, 2, m); g.rect(11, 56, 7, 2, m); g.rect(6, 57, 2, 4, m);
    g.rect(82, 66, 2, 13, m); g.rect(92, 72, 2, 7, m); g.rect(78, 70, 20, 2, m);
  } else {
    g.rect(0, 0, 100, 100, 0xa8dcf0);
    g.oval(20, 16, 8, 8, 0xffe79f);
    const int cl[4][3] = { { 54, 12, 22 }, { 60, 9, 12 }, { 30, 28, 16 }, { 76, 30, 18 } };
    for (auto &p : cl) g.rect(p[0], p[1], p[2], 3, 0xffffff);
    const int bird[2][2] = { { 44, 20 }, { 52, 24 } };
    for (auto &p : bird) {
      g.rect(p[0], p[1], 2, 1, 0x18181f);
      g.rect(p[0] + 3, p[1], 2, 1, 0x18181f);
      g.rect(p[0] + 2, p[1] + 1, 1, 1, 0x18181f);
    }
    g.oval(26, 62, 42, 18, 0x8fce7a);
    g.oval(82, 64, 36, 15, 0x8fce7a);
    g.rect(0, 62, 100, 38, 0x79bd4f);
    g.rect(0, 62, 100, 2, 0x65a842);
    for (int x : { 6, 22, 38, 54, 70, 86 }) g.rect(x, 58, 3, 12, 0x9a6f43);
    g.rect(0, 61, 100, 2, 0xb38a58);
    g.grass(14, 76, 0x4f932f); g.grass(34, 86, 0x4f932f); g.grass(60, 78, 0x4f932f); g.grass(80, 88, 0x4f932f); g.grass(48, 92, 0x4f932f);
    const uint32_t fl[4][3] = { { 24, 72, 0xf6d33c }, { 70, 72, 0xf07aa0 }, { 90, 80, 0xffffff }, { 8, 88, 0xf6d33c } };
    for (auto &p : fl) {
      g.rect((int)p[0] - 1, p[1], 3, 1, p[2]);
      g.rect(p[0], (int)p[1] - 1, 1, 3, p[2]);
    }
  }
}
