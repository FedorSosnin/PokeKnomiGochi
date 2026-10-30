#pragma once
// Hardware del BTT Knomi V2, con la misma interfaz que usa el juego en la Waveshare: el
// lienzo sigue siendo de 466x466 (coordenadas y toques no cambian) y se reduce al panel.
#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include <esp_heap_caps.h>
#include <soc/usb_serial_jtag_reg.h>
#include <soc/soc.h>

// GC9A01 con retroiluminacion AW9346: 16 niveles que se bajan a pulsos (driver de BTT)
class KnomiPanel : public Arduino_GC9A01 {
public:
  using Arduino_GC9A01::Arduino_GC9A01;
  void setBrightness(uint8_t v) {
    int8_t to = v == 0 ? 0 : 1 + (v * 15) / 255;
    if (to == level) return;
    pinMode(LCD_BL, OUTPUT);
    if (to == 0) {
      digitalWrite(LCD_BL, LOW);
      delay(3);  // >2.5 ms = apagado del AW9346
      level = 0;
      return;
    }
    if (level <= 0) {
      digitalWrite(LCD_BL, HIGH);
      delayMicroseconds(25);  // >20 us para encender: arranca al maximo
      level = 16;
    }
    int8_t pulses = level >= to ? level - to : level + 16 - to;  // el contador da la vuelta
    for (int8_t i = 0; i < pulses; i++) {
      digitalWrite(LCD_BL, LOW);
      delayMicroseconds(1);
      digitalWrite(LCD_BL, HIGH);
      delayMicroseconds(1);
    }
    level = to;
  }
private:
  int8_t level = -1;
};

// Lienzo logico 466x466 en PSRAM; flush() promedia 2x2 sobre el panel de 240x240
class ScaledCanvas : public Arduino_Canvas {
public:
  ScaledCanvas(Arduino_G *out) : Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, out) {}

  bool begin(int32_t speed = GFX_NOT_DEFINED) override {
    // GPIO19/20 son tambien el USB interno del S3: hay que soltarlos para usarlos como SPI
    CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE);
    if (!Arduino_Canvas::begin(speed)) return false;
    size_t s = PANEL_SIZE * PANEL_SIZE * 2;
    panel = (uint16_t *)heap_caps_malloc(s, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!panel) panel = (uint16_t *)ps_malloc(s);
    return panel != nullptr;
  }

  void flush(bool = false) override {
    uint16_t *d = panel;
    for (int y = 0; y < PANEL_SIZE; y++) {
      const uint16_t *a = _framebuffer + (y * (LCD_HEIGHT - 1) / PANEL_SIZE) * LCD_WIDTH;
      const uint16_t *b = a + LCD_WIDTH;
      for (int x = 0; x < PANEL_SIZE; x++) {
        int sx = x * (LCD_WIDTH - 1) / PANEL_SIZE;
        uint32_t s = expand(a[sx]) + expand(a[sx + 1]) + expand(b[sx]) + expand(b[sx + 1]);
        s = (s >> 2) & 0x07E0F81F;
        *d++ = (uint16_t)(s | (s >> 16));
      }
    }
    _output->draw16bitRGBBitmap(0, 0, panel, PANEL_SIZE, PANEL_SIZE);
  }

  uint16_t *panelBuffer() { return panel; }

private:
  static inline uint32_t expand(uint16_t c) { return (c | ((uint32_t)c << 16)) & 0x07E0F81F; }
  uint16_t *panel = nullptr;
};

// CST816S por registros, con la interfaz de TouchDrvCST92xx que usa handleTouch()
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
    writeReg(0xEC, 0x00);  // sin gestos del chip
    writeReg(0xFA, 0x60);  // INT al tocar y al cambiar, tambien al levantar
    writeReg(0xFB, 0x00);  // sin auto-reset
    writeReg(0xFC, 0x00);  // sin reset por pulsacion larga
    writeReg(0xFE, 0x01);  // sin auto-sleep: el chip contesta siempre por I2C
  }
  void setMaxCoordinates(int, int) {}  // fijas: el panel es de 240
  void setMirrorXY(bool, bool) {}      // el Knomi no va girado (la Waveshare si)

  // toque simulado desde la consola serie, en coordenadas logicas
  void inject(int16_t x, int16_t y, uint32_t ms) { ix = x; iy = y; iUntil = millis() + ms; iOn = true; }

  uint8_t getPoint(int16_t *x, int16_t *y, uint8_t) {
    if (iOn) {
      if ((int32_t)(iUntil - millis()) > 0) { *x = ix; *y = iy; return 1; }
      iOn = false;
      return 0;
    }
    wire->beginTransmission(addr);
    wire->write(0x02);
    if (wire->endTransmission(false) != 0 || wire->requestFrom(addr, (uint8_t)5) != 5) return 0;
    uint8_t n = wire->read(), xh = wire->read(), xl = wire->read(), yh = wire->read(), yl = wire->read();
    if ((n & 0x0F) == 0) return 0;
    *x = (((xh & 0x0F) << 8 | xl) * LCD_WIDTH) / PANEL_SIZE;
    *y = (((yh & 0x0F) << 8 | yl) * LCD_HEIGHT) / PANEL_SIZE;
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
  int16_t ix = 0, iy = 0;
  uint32_t iUntil = 0;
  bool iOn = false;
};
