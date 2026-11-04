// The host's Arduino.h: what the game uses from the ESP32 platform, without hardware.
// Implemented by host_runtime.cpp; see host/README.md.
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
#ifdef _WIN32  // the ESP32's (and macOS's) strlcpy, which Windows doesn't have
inline size_t strlcpy(char *dst, const char *src, size_t size) {
  size_t n = strlen(src);
  if (size) {
    size_t k = n < size - 1 ? n : size - 1;
    memcpy(dst, src, k);
    dst[k] = 0;
  }
  return n;
}
#endif
// With mixed types: min(uint32_t, 800UL) is the same type on the ESP32 but not here
template <class A, class B> inline auto min(A a, B b) -> std::common_type_t<A, B> { return a < b ? a : b; }
template <class A, class B> inline auto max(A a, B b) -> std::common_type_t<A, B> { return a > b ? a : b; }

// clock: advances once per host tick (see hostTick), so it is constant within one pass of loop()
uint32_t millis();
uint32_t micros();
// the game runs in ticks and cannot block (it can't in WebAssembly): delay() does not wait
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

// the subset of String that the sketch's serial console uses
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

// The Knomi's serial console: input is whatever hostSerialPush() passes in, output goes to stdout
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
  void restart();  // handled by the front end at the end of the tick: see hostRestartRequested()
};
extern HostEsp ESP;
