// Bus I2C del host: no hay nada colgado, pero todo contesta (el sketch comprueba el tactil antes de leerlo).
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
