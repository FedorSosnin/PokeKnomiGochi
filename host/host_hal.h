#pragma once
// El hardware del Knomi tal como lo ve el sketch (misma interfaz que knomi_hal.h), sin hardware.
// El host se compila con BOARD_HOST y BOARD_KNOMI_V2: es un Knomi con otra cosa por debajo.
#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"

// Internos del host (host_runtime.cpp): el estado que ponen los hostXxx() de host.h
void hostPanelSetBrightness(uint8_t v);
void hostCanvasFlushed();
bool hostPointerRead(int16_t *x, int16_t *y);
float hostTiltValue();

class KnomiPanel {
public:
  KnomiPanel(Arduino_DataBus *, int, int, bool, int, int) {}
  void setBrightness(uint8_t v) { hostPanelSetBrightness(v); }
};

class ScaledCanvas : public Arduino_Canvas {
public:
  explicit ScaledCanvas(KnomiPanel *) : Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT) {}
  void flush(bool = false) override { hostCanvasFlushed(); }
  // para el comando SHOT de la consola: lo que veria el panel del Knomi
  uint16_t *panelBuffer();
private:
  uint16_t *panel_ = nullptr;
};

class KnomiTouch {
public:
  void setPins(int, int) {}
  template <class W> bool begin(W &, uint8_t, int, int) { return true; }
  void reset() {}
  void setMaxCoordinates(int, int) {}
  void setMirrorXY(bool, bool) {}

  // toque simulado desde la consola serie, en coordenadas logicas
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
    return hostPointerRead(x, y) ? 1 : 0;
  }

private:
  int16_t ix = 0, iy = 0, jx = 0, jy = 0;
  uint32_t iUntil = 0, iMs = 1;
  bool iOn = false;
};

// Las flechas hacen de acelerometro: -1..1 son los 30 grados a los que el juego llega al tope
class KnomiAccel {
public:
  template <class W> bool begin(W &) { return true; }
  bool ready() const { return true; }
  bool read(float g[3]) {
    g[0] = g[1] = g[2] = 0;
    g[KNOMI_TILT_AXIS] = hostTiltValue() * 0.5f / KNOMI_TILT_SIGN;
    return true;
  }
};
