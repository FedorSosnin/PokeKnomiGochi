// The host's I2C bus: nothing is attached, but everything answers (the sketch checks the touch panel before reading it).
#pragma once
#include <stdint.h>

class TwoWire {
public:
  bool begin(int, int) { return true; }
  void setTimeOut(uint16_t) {}
  void beginTransmission(uint8_t) {}
  uint8_t endTransmission(bool = true) { return 0; }
};
extern TwoWire Wire;
