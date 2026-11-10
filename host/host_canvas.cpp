// Software canvas: Arduino_Canvas (see shim/Arduino_GFX_Library.h).
#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "glcdfont.h"

Arduino_Canvas::Arduino_Canvas(int16_t w, int16_t h) : w_(w), h_(h) {}
Arduino_Canvas::~Arduino_Canvas() { free(fb_); }

bool Arduino_Canvas::begin(int32_t) {
  if (!fb_) fb_ = (uint16_t *)calloc((size_t)w_ * h_, sizeof(uint16_t));
  return fb_ != nullptr;
}

void Arduino_Canvas::flush(bool) {}

// V2's hover (hover_v2.h): while gHoverRec is on, each pixel drawn on the canvas whose framebuffer is gHoverFb is
// marked in gHoverMask, and the first time, what was there before is kept in gHoverUnder. So the control under the
// mouse is known pixel by pixel, and what's behind it, and it can be lifted off the screen with its shadow under it
bool gHoverRec = false;
uint16_t *gHoverFb = nullptr;
uint8_t *gHoverMask = nullptr;     // (w x h, set up by hover_v2.h)
uint16_t *gHoverUnder = nullptr;
int16_t gHoverBox[4] = { 32767, 32767, -1, -1 };  // what's marked: x0, y0, x1, y1 (inclusive)
static void hoverMark(int x0, int y0, int x1, int y1, int w, const uint16_t *fb) {  // x1, y1 exclusive
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++) {
      int i = y * w + x;
      if (gHoverMask[i]) continue;
      gHoverMask[i] = 1;
      gHoverUnder[i] = fb[i];
    }
  if (x0 < gHoverBox[0]) gHoverBox[0] = (int16_t)x0;
  if (y0 < gHoverBox[1]) gHoverBox[1] = (int16_t)y0;
  if (x1 - 1 > gHoverBox[2]) gHoverBox[2] = (int16_t)(x1 - 1);
  if (y1 - 1 > gHoverBox[3]) gHoverBox[3] = (int16_t)(y1 - 1);
}

void Arduino_Canvas::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
  int x0 = x, y0 = y, x1, y1;
  if (w < 0) { x0 += w + 1; w = -w; }
  if (h < 0) { y0 += h + 1; h = -h; }
  x1 = x0 + w;
  y1 = y0 + h;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > w_) x1 = w_;
  if (y1 > h_) y1 = h_;
  if (x0 >= x1 || y0 >= y1) return;
  if (gHoverRec && fb_ == gHoverFb) hoverMark(x0, y0, x1, y1, w_, fb_);
  for (int yy = y0; yy < y1; yy++) {
    uint16_t *p = fb_ + yy * w_ + x0;
    for (int n = x1 - x0; n > 0; n--) *p++ = c;
  }
}

void Arduino_Canvas::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c) {
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) { std::swap(x0, y0); std::swap(x1, y1); }
  if (x0 > x1) { std::swap(x0, x1); std::swap(y0, y1); }
  int16_t dx = x1 - x0, dy = abs(y1 - y0), err = dx >> 1, step = y0 < y1 ? 1 : -1;
  for (; x0 <= x1; x0++) {
    if (steep) fillRect(y0, x0, 1, 1, c);
    else fillRect(x0, y0, 1, 1, c);
    err -= dy;
    if (err < 0) { err += dx; y0 += step; }
  }
}

// the library's writeEllipseHelper() with rx == ry
void Arduino_Canvas::ellipseCorners(int32_t x, int32_t y, int32_t r, uint8_t corners, uint16_t c) {
  if (r <= 0) return;
  int32_t xt = 0, yt = r, i = -1, r2 = r * r;
  int32_t s = (r2 << 1) + r2 * (1 - (r << 1));
  do {
    while (s < 0) s += r2 * ((++xt << 2) + 2);
    if (corners & 1) hline(x - xt, y - yt, xt - i, c);
    if (corners & 2) hline(x + i + 1, y - yt, xt - i, c);
    if (corners & 4) hline(x + i + 1, y + yt, xt - i, c);
    if (corners & 8) hline(x - xt, y + yt, xt - i, c);
    i = xt;
    s -= (--yt) * r2 << 2;
  } while (r2 * xt <= r2 * yt);

  i = -1;
  yt = 0;
  xt = r;
  s = (r2 << 1) + r2 * (1 - (r << 1));
  do {
    while (s < 0) s += r2 * ((++yt << 2) + 2);
    if (corners & 1) vline(x - xt, y - yt, yt - i, c);
    if (corners & 2) vline(x + xt, y - yt, yt - i, c);
    if (corners & 4) vline(x + xt, y + i + 1, yt - i, c);
    if (corners & 8) vline(x - xt, y + i + 1, yt - i, c);
    i = yt;
    s -= (--xt) * r2 << 2;
  } while (r2 * yt <= r2 * xt);
}

// the library's writeFillEllipseHelper() with rx == ry
void Arduino_Canvas::fillCorners(int32_t x, int32_t y, int32_t r, uint8_t corners, int16_t delta, uint16_t c) {
  if (r <= 0) return;
  int32_t r2 = r * r, xt = r, yt = 0, i = 0;
  int32_t s = (r2 << 1) + r2 * (1 - (r << 1));
  hline(x - r, y, (r << 1) + 1, c);
  do {
    while (s < 0) s += r2 * ((++yt << 2) + 2);
    if (corners & 1) fillRect(x - xt, y - yt, (xt << 1) + 1 + delta, yt - i, c);
    if (corners & 2) fillRect(x - xt, y + i + 1, (xt << 1) + 1 + delta, yt - i, c);
    i = yt;
    s -= (--xt) * r2 << 2;
  } while (r2 * yt <= r2 * xt);

  xt = 0;
  yt = r;
  s = (r2 << 1) + r2 * (1 - (r << 1));
  do {
    while (s < 0) s += r2 * ((++xt << 2) + 2);
    if (corners & 1) hline(x - xt, y - yt, (xt << 1) + 1 + delta, c);
    if (corners & 2) hline(x - xt, y + yt, (xt << 1) + 1 + delta, c);
    s -= (--yt) * r2 << 2;
  } while (r2 * xt <= r2 * yt);
}

void Arduino_Canvas::drawCircle(int16_t x, int16_t y, int16_t r, uint16_t c) { ellipseCorners(x, y, r, 0xF, c); }
void Arduino_Canvas::fillCircle(int16_t x, int16_t y, int16_t r, uint16_t c) { fillCorners(x, y, r, 3, 0, c); }

void Arduino_Canvas::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c) {
  int16_t maxR = (w < h ? w : h) / 2;
  if (r > maxR) r = maxR;
  hline(x + r, y, w - 2 * r, c);
  hline(x + r, y + h - 1, w - 2 * r, c);
  vline(x, y + r, h - 2 * r, c);
  vline(x + w - 1, y + r, h - 2 * r, c);
  ellipseCorners(x + r, y + r, r, 1, c);
  ellipseCorners(x + w - r - 1, y + r, r, 2, c);
  ellipseCorners(x + w - r - 1, y + h - r - 1, r, 4, c);
  ellipseCorners(x + r, y + h - r - 1, r, 8, c);
}

void Arduino_Canvas::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c) {
  int16_t maxR = (w < h ? w : h) / 2;
  if (r > maxR) r = maxR;
  fillRect(x, y + r, w, h - (r << 1), c);
  fillCorners(x + r, y + r, r, 1, w - 2 * r - 1, c);
  fillCorners(x + r, y + h - r - 1, r, 2, w - 2 * r - 1, c);
}

void Arduino_Canvas::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c) {
  if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
  if (y1 > y2) { std::swap(y2, y1); std::swap(x2, x1); }
  if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
  int16_t a, b, y, last;
  if (y0 == y2) {
    a = b = x0;
    if (x1 < a) a = x1; else if (x1 > b) b = x1;
    if (x2 < a) a = x2; else if (x2 > b) b = x2;
    hline(a, y0, b - a + 1, c);
    return;
  }
  int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;
  last = y1 == y2 ? y1 : y1 - 1;
  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    if (a > b) std::swap(a, b);
    hline(a, y, b - a + 1, c);
  }
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    if (a > b) std::swap(a, b);
    hline(a, y, b - a + 1, c);
  }
}

void Arduino_Canvas::draw16bitRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h) {
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++) {
      int px = x + i, py = y + j;
      if (px >= 0 && px < w_ && py >= 0 && py < h_) {
        if (gHoverRec && fb_ == gHoverFb) hoverMark(px, py, px + 1, py + 1, w_, fb_);
        fb_[py * w_ + px] = bitmap[j * w + i];
      }
    }
}

// ---- text ----

void Arduino_Canvas::setFont(const uint8_t *f) {
  font_ = f;
  utf8State_ = 0;
}

void Arduino_Canvas::print(const char *s) {
  while (*s) write((uint8_t)*s++);
}

void Arduino_Canvas::drawGlcdChar(int16_t x, int16_t y, uint8_t ch) {
  int16_t cx = x;
  for (int i = 0; i < 5; i++, cx += textSize_) {
    uint8_t line = GLCD_FONT[ch * 5 + i];
    int16_t cy = y;
    for (int j = 0; j < 8; j++, line >>= 1, cy += textSize_)
      if (line & 1) fillRect(cx, cy, textSize_, textSize_, textColor_);
  }
}

// header of a U8g2 font (u8g2_read_font_info)
enum { U8_BITS0 = 2, U8_BITS1 = 3, U8_BITS_W = 4, U8_BITS_H = 5, U8_BITS_X = 6, U8_BITS_Y = 7, U8_BITS_DX = 8,
       U8_MAX_H = 10, U8_POS_A = 17, U8_POS_a = 19, U8_POS_UNI = 21, U8_HEADER = 23 };

static uint16_t fontWord(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

uint8_t Arduino_Canvas::bitsU(uint8_t n) {
  uint8_t val = (uint8_t)(*bitPtr_ >> bitPos_);
  uint8_t next = bitPos_ + n;
  if (next >= 8) {
    bitPtr_++;
    val |= (uint8_t)(*bitPtr_ << (8 - bitPos_));
    next -= 8;
  }
  bitPos_ = next;
  return val & (uint8_t)((1U << n) - 1);
}

int8_t Arduino_Canvas::bitsS(uint8_t n) {
  return (int8_t)((int8_t)bitsU(n) - (int8_t)(1 << (n - 1)));
}

bool Arduino_Canvas::findGlyph(uint32_t enc, Glyph &g) {
  const uint8_t *p = font_ + U8_HEADER, *data = nullptr;
  if (enc <= 255) {
    if (enc >= 'a') p += fontWord(font_ + U8_POS_a);
    else if (enc >= 'A') p += fontWord(font_ + U8_POS_A);
    while (p[1] != 0) {
      if (p[0] == enc) data = p + 2;
      p += p[1];
    }
  } else {
    p += fontWord(font_ + U8_POS_UNI);
    const uint8_t *table = p;
    uint16_t e;
    do {
      p += fontWord(table);
      e = fontWord(table + 2);
      table += 4;
    } while (e < enc);
    while ((e = fontWord(p)) != 0) {
      if (e == enc) { data = p + 3; break; }
      p += p[2];
    }
  }
  if (!data) return false;
  bitPtr_ = data;
  bitPos_ = 0;
  g.w = bitsU(font_[U8_BITS_W]);
  g.h = bitsU(font_[U8_BITS_H]);
  g.x = bitsS(font_[U8_BITS_X]);
  g.y = bitsS(font_[U8_BITS_Y]);
  g.dx = bitsS(font_[U8_BITS_DX]);
  g.data = bitPtr_;
  return true;
}

bool Arduino_Canvas::nextGlyph(uint8_t c, Glyph &g, bool &newline) {
  newline = false;
  if (utf8_) {
    if (utf8State_ == 0) {
      if (c >= 0xfc) { utf8State_ = 5; c &= 1; }
      else if (c >= 0xf8) { utf8State_ = 4; c &= 3; }
      else if (c >= 0xf0) { utf8State_ = 3; c &= 7; }
      else if (c >= 0xe0) { utf8State_ = 2; c &= 15; }
      else if (c >= 0xc0) { utf8State_ = 1; c &= 0x1f; }
      encoding_ = c;
    } else {
      utf8State_--;
      encoding_ = (encoding_ << 6) | (c & 0x3f);
    }
  } else {
    encoding_ = c;
  }
  if (utf8State_ != 0) return false;
  if (encoding_ == '\n') { newline = true; return false; }
  if (encoding_ == '\r') return false;
  return findGlyph(encoding_, g);
}

void Arduino_Canvas::drawU8g2Glyph(const Glyph &g, int16_t x, int16_t y) {
  if (g.w == 0) return;
  int tx = x + g.x * textSize_;
  int ty = y - (g.h + g.y) * textSize_;
  uint8_t bits0 = font_[U8_BITS0], bits1 = font_[U8_BITS1];
  int lx = 0, ly = 0;
  auto run = [&](uint8_t len, bool fg) {
    int cnt = len;
    for (;;) {
      int rem = g.w - lx, cur = cnt < rem ? cnt : rem;
      if (fg) fillRect(tx + lx * textSize_, ty + ly * textSize_, cur * textSize_, textSize_, textColor_);
      if (cnt < rem) { lx += cnt; return; }
      cnt -= rem;
      lx = 0;
      ly++;
    }
  };
  bitPtr_ = g.data;  // findGlyph left it right after the glyph header, but charBounds may move it
  for (;;) {
    uint8_t a = bitsU(bits0), b = bitsU(bits1);
    do {
      run(a, false);
      run(b, true);
    } while (bitsU(1) != 0);
    if (ly >= g.h) break;
  }
}

void Arduino_Canvas::write(uint8_t c) {
  if (font_) {
    Glyph g;
    bool newline;
    uint8_t startBit;
    if (nextGlyph(c, g, newline)) {
      startBit = bitPos_;
      if (g.w > 0 && cursorX_ + textSize_ * g.w - 1 > w_ - 1) {
        cursorX_ = 0;
        cursorY_ += textSize_ * font_[U8_MAX_H];
      }
      bitPos_ = startBit;
      drawU8g2Glyph(g, cursorX_, cursorY_);
      cursorX_ += textSize_ * g.dx;
    } else if (newline) {
      cursorX_ = 0;
      cursorY_ += textSize_ * font_[U8_MAX_H];
    }
    return;
  }
  if (c == '\n') {
    cursorX_ = 0;
    cursorY_ += textSize_ * 8;
  } else if (c != '\r') {
    if (cursorX_ + textSize_ * 6 - 1 > w_ - 1) {  // the library wraps the line when it runs off the right edge
      cursorX_ = 0;
      cursorY_ += textSize_ * 8;
    }
    drawGlcdChar(cursorX_, cursorY_, c);
    cursorX_ += textSize_ * 6;
  }
}

void Arduino_Canvas::charBounds(uint8_t c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy) {
  if (font_) {
    Glyph g;
    bool newline;
    if (nextGlyph(c, g, newline)) {
      if (g.w > 0 && *x + textSize_ * g.w - 1 > w_ - 1) {
        *x = 0;
        *y += textSize_ * font_[U8_MAX_H];
      }
      int16_t x1 = *x + g.x * textSize_, y1 = *y - (g.h + g.y) * textSize_;
      int16_t x2 = x1 + g.w * textSize_ - 1, y2 = y1 + g.h * textSize_ - 1;
      if (x1 < *minx) *minx = x1;
      if (y1 < *miny) *miny = y1;
      if (x2 > *maxx) *maxx = x2;
      if (y2 > *maxy) *maxy = y2;
      *x += textSize_ * g.dx;
    } else if (newline) {
      *x = 0;
      *y += textSize_ * font_[U8_MAX_H];
    }
    return;
  }
  if (c == '\n') {
    *x = 0;
    *y += textSize_ * 8;
  } else if (c != '\r') {
    if (*x + textSize_ * 6 - 1 > w_ - 1) {
      *x = 0;
      *y += textSize_ * 8;
    }
    int16_t x2 = *x + textSize_ * 6 - 1, y2 = *y + textSize_ * 8 - 1;
    if (x2 > *maxx) *maxx = x2;
    if (y2 > *maxy) *maxy = y2;
    if (*x < *minx) *minx = *x;
    if (*y < *miny) *miny = *y;
    *x += textSize_ * 6;
  }
}

void Arduino_Canvas::getTextBounds(const char *s, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
  *x1 = x;
  *y1 = y;
  *w = *h = 0;
  int16_t minx = w_ - 1, miny = h_ - 1, maxx = 0, maxy = 0;
  uint8_t c;
  while ((c = (uint8_t)*s++)) charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
  if (maxx >= minx) { *x1 = minx; *w = maxx - minx + 1; }
  if (maxy >= miny) { *y1 = miny; *h = maxy - miny + 1; }
}
