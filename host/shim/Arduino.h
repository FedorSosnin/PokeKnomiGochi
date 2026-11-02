// Arduino.h del host: lo que el juego usa de la plataforma ESP32, sin hardware.
// Lo implementa host_runtime.cpp; ver host/README.md.
#pragma once
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <type_traits>

#define IRAM_ATTR
#define PROGMEM
#define HIGH 1
#define LOW 0
#define INPUT 0x01
#define OUTPUT 0x03
#define INPUT_PULLUP 0x05
#define FALLING 0x02
#define PI 3.1415926535897932384626433832795
#define digitalPinToInterrupt(p) (p)

using std::abs;
// Con tipos mezclados: min(uint32_t, 800UL) es del mismo tipo en el ESP32 pero no aqui
template <class A, class B> inline auto min(A a, B b) -> std::common_type_t<A, B> { return a < b ? a : b; }
template <class A, class B> inline auto max(A a, B b) -> std::common_type_t<A, B> { return a > b ? a : b; }

// reloj: avanza una vez por tick del host (ver hostTick), asi que es constante en un pase de loop()
uint32_t millis();
uint32_t micros();
// el juego corre por ticks y no puede bloquear (en WebAssembly no se puede): delay() no espera
inline void delay(uint32_t) {}
inline void delayMicroseconds(uint32_t) {}

long random(long howbig);
long random(long howsmall, long howbig);
void randomSeed(unsigned long seed);
uint32_t esp_random();

inline void *ps_malloc(size_t n) { return malloc(n); }

void pinMode(uint8_t pin, uint8_t mode);
int digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t val);
void attachInterrupt(uint8_t pin, void (*fn)(), int mode);

// el subconjunto de String que usa la consola serie del sketch
class String {
public:
  String(const char *s = "") : s_(s ? s : "") {}
  String(const std::string &s) : s_(s) {}
  unsigned length() const { return (unsigned)s_.size(); }
  const char *c_str() const { return s_.c_str(); }
  void trim();
  bool startsWith(const char *p) const { return s_.rfind(p, 0) == 0; }
  int indexOf(char c, unsigned from = 0) const;
  int indexOf(const char *t, unsigned from = 0) const;
  int lastIndexOf(char c) const;
  String substring(unsigned left, unsigned right = 0xFFFFFFFFu) const;
  long toInt() const { return atol(s_.c_str()); }
  float toFloat() const { return (float)atof(s_.c_str()); }
  bool operator==(const char *o) const { return s_ == o; }
  bool operator!=(const char *o) const { return s_ != o; }
private:
  std::string s_;
};

// La consola serie del Knomi: entra lo que pase hostSerialPush() y sale por stdout
class HostSerial {
public:
  void begin(unsigned long) {}
  void setRxBufferSize(size_t) {}
  void setTimeout(unsigned long) {}
  int available();
  String readStringUntil(char);
  int printf(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
  void print(const char *s);
  void println(const char *s = "");
  size_t write(const uint8_t *data, size_t len);
};
extern HostSerial Serial;

class HostEsp {
public:
  uint32_t getFreeHeap() { return 0; }
  uint32_t getMinFreeHeap() { return 0; }
  void restart();  // lo atiende el front end al acabar el tick: ver hostRestartRequested()
};
extern HostEsp ESP;
