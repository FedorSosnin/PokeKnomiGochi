#pragma once
// The Knomi hardware as the sketch sees it (same interface as knomi_hal.h), without hardware.
// The host is built with BOARD_HOST and BOARD_KNOMI_V2: it is a Knomi with something else underneath.
#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"

// Host internals (host_runtime.cpp): the state set by the hostXxx() functions of host.h
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
  // for the console's SHOT command: what the Knomi's panel would show
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
    return hostPointerRead(x, y) ? 1 : 0;
  }

private:
  int16_t ix = 0, iy = 0, jx = 0, jy = 0;
  uint32_t iUntil = 0, iMs = 1;
  bool iOn = false;
};

// The arrow keys act as the accelerometer: -1..1 is the 30 degrees at which the game reaches its limit
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
