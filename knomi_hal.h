#pragma once
// BTT Knomi V2 hardware, with the same interface the game uses on the Waveshare: the
// canvas is still 466x466 (coordinates and touches don't change) and is scaled down to the panel.
#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "autorot.h"
#include <esp_heap_caps.h>
#include <soc/usb_serial_jtag_reg.h>
#include <soc/soc.h>

// GC9A01 with AW9346 backlight: 16 levels stepped down by pulses (BTT driver). The chip only steps
// down, wrapping from the dimmest back to the brightest, so counting from where it was supposed to be
// went wrong whenever that was off (after a restart, the pin kept high, the chip kept its old level
// while the count started from the brightest): asleep, it then stepped past the dimmest and came
// back near full brightness. Every change now starts over from a known place: off (low over 2.5 ms),
// on at the brightest, then down to the level wanted.
class KnomiPanel : public Arduino_GC9A01 {
public:
  using Arduino_GC9A01::Arduino_GC9A01;
  void setBrightness(uint8_t v) {
    int8_t to = v == 0 ? 0 : 1 + (v * 15) / 255;
    if (to == level) return;
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);
    delay(3);  // >2.5 ms = AW9346 off
    level = 0;
    if (to == 0) return;
    noInterrupts();  // a pulse stretched by an interrupt could be counted wrong
    digitalWrite(LCD_BL, HIGH);
    delayMicroseconds(25);  // >20 us to turn on: starts at the brightest (16)
    for (int8_t i = 16; i > to; i--) {
      digitalWrite(LCD_BL, LOW);
      delayMicroseconds(1);
      digitalWrite(LCD_BL, HIGH);
      delayMicroseconds(1);
    }
    interrupts();
    level = to;
  }
private:
  int8_t level = -1;
};

void pressOverlay();  // the sketch's press preview, drawn over every frame

// Logical 466x466 canvas in PSRAM; flush() averages 2x2 onto the 240x240 panel, with the current rotation
class ScaledCanvas : public Arduino_Canvas {
public:
  ScaledCanvas(Arduino_G *out) : Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, out) {}

  bool begin(int32_t speed = GFX_NOT_DEFINED) override {
    // GPIO19/20 are also the S3's internal USB: they must be released to use them as SPI
    CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE);
    if (!Arduino_Canvas::begin(speed)) return false;
    size_t s = PANEL_SIZE * PANEL_SIZE * 2;
    panel = (uint16_t *)heap_caps_malloc(s, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!panel) panel = (uint16_t *)ps_malloc(s);
    return panel != nullptr;
  }

  // Each panel pixel averages a 2x2 block that always starts on an even canvas pixel: the 233 pairs
  // of columns (and rows) spread over the 240 panel pixels, 7 of them shown twice. With blocks that
  // drifted by one pixel every 16, half the screen blurred every edge; aligned, what is drawn on even
  // pixels (sprites, icons, text: SNAP2 in the sketch) lands whole on the panel and stays sharp.
  void flush(bool = false) override {
    pressOverlay();
    const RotBlit r = rotBlit(gPanelRot, PANEL_SIZE);
    for (int y = 0; y < PANEL_SIZE; y++) {
      const uint16_t *a = _framebuffer + (2 * (y * (LCD_HEIGHT / 2) / PANEL_SIZE)) * LCD_WIDTH;
      const uint16_t *b = a + LCD_WIDTH;
      for (int x = 0; x < PANEL_SIZE; x++) {
        int sx = 2 * (x * (LCD_WIDTH / 2) / PANEL_SIZE);
        uint32_t s = expand(a[sx]) + expand(a[sx + 1]) + expand(b[sx]) + expand(b[sx + 1]);
        s = (s >> 2) & 0x07E0F81F;
        panel[r.base + x * r.dx + y * r.dy] = (uint16_t)(s | (s >> 16));
      }
    }
    _output->draw16bitRGBBitmap(0, 0, panel, PANEL_SIZE, PANEL_SIZE);
  }

  uint16_t *panelBuffer() { return panel; }

private:
  static inline uint32_t expand(uint16_t c) { return (c | ((uint32_t)c << 16)) & 0x07E0F81F; }
  uint16_t *panel = nullptr;
};

// CST816S by registers, with the TouchDrvCST92xx interface that handleTouch() uses
class KnomiTouch {
public:
  void setPins(int rst, int irq) { (void)irq; rstPin = rst; }
  bool begin(TwoWire &w, uint8_t a, int, int) {
    wire = &w;
    addr = a;
    reset();
    wire->beginTransmission(addr);
    return wire->endTransmission() == 0;
  }
  void reset() {
    pinMode(rstPin, OUTPUT);
    digitalWrite(rstPin, LOW);
    delay(10);
    digitalWrite(rstPin, HIGH);
    delay(100);
    writeReg(0xEC, 0x00);  // no chip gestures
    writeReg(0xFA, 0x60);  // INT on touch and on change, also on release
    writeReg(0xFB, 0x00);  // no auto-reset
    writeReg(0xFC, 0x00);  // no reset on long press
    writeReg(0xFE, 0x01);  // no auto-sleep: the chip always answers over I2C
  }
  void setMaxCoordinates(int, int) {}  // fixed: the panel is 240
  void setMirrorXY(bool, bool) {}      // the Knomi's rotation is handled in getPoint() (the Waveshare uses this)

  // simulated touch from the serial console, in logical coordinates
  void inject(int16_t x, int16_t y, uint32_t ms) { swipe(x, y, x, y, ms); }
  void swipe(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t ms) {
    ix = x0; iy = y0; jx = x1; jy = y1; iMs = ms ? ms : 1; iUntil = millis() + iMs; iOn = true;
  }

  uint8_t getPoint(int16_t *x, int16_t *y, uint8_t) {
    if (iOn) {
      int32_t left = (int32_t)(iUntil - millis());
      if (left > 0) {
        *x = jx - (jx - ix) * left / (int32_t)iMs;
        *y = jy - (jy - iy) * left / (int32_t)iMs;
        return 1;
      }
      iOn = false;
      return 0;
    }
    wire->beginTransmission(addr);
    wire->write(0x02);
    if (wire->endTransmission(false) != 0 || wire->requestFrom(addr, (uint8_t)5) != 5) return 0;
    uint8_t n = wire->read(), xh = wire->read(), xl = wire->read(), yh = wire->read(), yl = wire->read();
    if ((n & 0x0F) == 0) return 0;
    int32_t px = (((xh & 0x0F) << 8 | xl) * LCD_WIDTH) / PANEL_SIZE;
    int32_t py = (((yh & 0x0F) << 8 | yl) * LCD_HEIGHT) / PANEL_SIZE;
    int32_t lx, ly;
    rotTouch(gPanelRot, px, py, LCD_WIDTH, &lx, &ly);
    *x = lx;
    *y = ly;
    return 1;
  }

private:
  void writeReg(uint8_t r, uint8_t v) {
    wire->beginTransmission(addr);
    wire->write(r);
    wire->write(v);
    wire->endTransmission();
  }
  TwoWire *wire = nullptr;
  uint8_t addr = 0x15;
  int rstPin = -1;
  int16_t ix = 0, iy = 0, jx = 0, jy = 0;
  uint32_t iUntil = 0, iMs = 1;
  bool iOn = false;
};

// LIS2DW12 (0x19, same I2C bus). Gravity is read for tilting; the game sets zero at start.
#define KNOMI_ACC_ADDR 0x19
class KnomiAccel {
public:
  bool begin(TwoWire &w) {
    wire = &w;
    if (readReg(0x0F) != 0x44) return false;  // WHO_AM_I
    writeReg(0x20, 0x54);  // 100 Hz, high performance
    writeReg(0x21, 0x0C);  // BDU and auto-increment
    writeReg(0x25, 0x04);  // +-2 g, low noise
    ok = true;
    return true;
  }
  bool ready() const { return ok; }
  // gravity in g per axis
  bool read(float g[3]) {
    if (!ok) return false;
    wire->beginTransmission(KNOMI_ACC_ADDR);
    wire->write(0x28);
    if (wire->endTransmission(false) != 0 || wire->requestFrom((uint8_t)KNOMI_ACC_ADDR, (uint8_t)6) != 6) return false;
    for (int i = 0; i < 3; i++) {
      int16_t v = wire->read();
      v |= (int16_t)wire->read() << 8;
      g[i] = v / 16384.0f;
    }
    return true;
  }
private:
  uint8_t readReg(uint8_t r) {
    wire->beginTransmission(KNOMI_ACC_ADDR);
    wire->write(r);
    if (wire->endTransmission(false) != 0 || wire->requestFrom((uint8_t)KNOMI_ACC_ADDR, (uint8_t)1) != 1) return 0;
    return wire->read();
  }
  void writeReg(uint8_t r, uint8_t v) {
    wire->beginTransmission(KNOMI_ACC_ADDR);
    wire->write(r);
    wire->write(v);
    wire->endTransmission();
  }
  TwoWire *wire = nullptr;
  bool ok = false;
};
