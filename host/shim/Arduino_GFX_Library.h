// El subconjunto de Arduino_GFX que usa el juego, sobre un framebuffer RGB565 en memoria.
// Los algoritmos son los de la libreria (v1.6.5) para que cada pixel caiga en el mismo sitio.
#pragma once
#include <stdint.h>

#define RGB565_BLACK 0x0000
#define GFX_NOT_DEFINED -1

class Arduino_DataBus {};
class Arduino_ESP32SPI : public Arduino_DataBus {
public:
  Arduino_ESP32SPI(int, int, int, int, int) {}
};

class Arduino_Canvas {
public:
  Arduino_Canvas(int16_t w, int16_t h);
  virtual ~Arduino_Canvas();
  virtual bool begin(int32_t speed = GFX_NOT_DEFINED);
  virtual void flush(bool force = false);

  uint16_t *framebuffer() { return fb_; }
  int16_t width() const { return w_; }
  int16_t height() const { return h_; }

  void fillScreen(uint16_t c) { fillRect(0, 0, w_, h_, c); }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c);
  void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t c);
  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t c);
  void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c);
  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c);
  void draw16bitRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);

  void setTextColor(uint16_t c) { textColor_ = c; }
  void setTextSize(uint8_t s) { textSize_ = s > 0 ? s : 1; }
  void setCursor(int16_t x, int16_t y) { cursorX_ = x; cursorY_ = y; }
  int16_t getCursorX() const { return cursorX_; }
  int16_t getCursorY() const { return cursorY_; }
  void setFont(const uint8_t *u8g2Font = nullptr);
  void setUTF8Print(bool on) { utf8_ = on; }
  void getTextBounds(const char *s, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
  void print(const char *s);
  void print(char c) { write((uint8_t)c); }
  void write(uint8_t c);

private:
  struct Glyph {
    const uint8_t *data = nullptr;
    uint8_t w = 0, h = 0;
    int8_t x = 0, y = 0, dx = 0;
  };
  void hline(int16_t x, int16_t y, int16_t w, uint16_t c) { fillRect(x, y, w, 1, c); }
  void vline(int16_t x, int16_t y, int16_t h, uint16_t c) { fillRect(x, y, 1, h, c); }
  void ellipseCorners(int32_t x, int32_t y, int32_t r, uint8_t corners, uint16_t c);
  void fillCorners(int32_t x, int32_t y, int32_t r, uint8_t corners, int16_t delta, uint16_t c);
  void drawGlcdChar(int16_t x, int16_t y, uint8_t ch);
  // U8g2: decodifica un byte de UTF-8 y busca el glifo; false si aun no hay caracter completo
  bool nextGlyph(uint8_t c, Glyph &g, bool &newline);
  bool findGlyph(uint32_t enc, Glyph &g);
  void drawU8g2Glyph(const Glyph &g, int16_t x, int16_t y);
  uint8_t bitsU(uint8_t n);
  int8_t bitsS(uint8_t n);
  void charBounds(uint8_t c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy);

  int16_t w_, h_;
  uint16_t *fb_ = nullptr;
  uint16_t textColor_ = 0xFFFF;
  uint8_t textSize_ = 1;
  int16_t cursorX_ = 0, cursorY_ = 0;
  const uint8_t *font_ = nullptr;
  bool utf8_ = false;
  uint8_t utf8State_ = 0;
  uint32_t encoding_ = 0;
  const uint8_t *bitPtr_ = nullptr;
  uint8_t bitPos_ = 0;
};
