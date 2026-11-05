// The host's "Arduino": clock, input, serial console and the tick loop.
#include "host.h"
#include "host_hal.h"
#include "host_internal.h"
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif
#include <time.h>
#include <Wire.h>
#include <chrono>
#include <deque>
#include <mutex>
#include <random>

// The most that passes from one tick to the next. The game's clock stops if the process stops running
// (Mac asleep, tab in the background) and the game sees the gap as away time, through
// clockPoll() and syncClock(): otherwise Pet::update() would run through hours at once at one minute per tick.
#define MAX_TICK_MS 250

// ---- clock ----
static uint32_t gMillis = 0;
static std::chrono::steady_clock::time_point gLastTick;
static bool gTicking = false;

uint32_t millis() { return gMillis; }
uint32_t micros() { return gMillis * 1000u; }

static uint32_t advanceClock() {
  auto now = std::chrono::steady_clock::now();
  uint32_t dt = 0;
  if (gTicking) {
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - gLastTick).count();
    dt = ms > MAX_TICK_MS ? MAX_TICK_MS : (uint32_t)ms;
    gLastTick += std::chrono::milliseconds(ms);
    if (ms > MAX_TICK_MS) gLastTick = now;
  } else {
    gLastTick = now;
    gTicking = true;
  }
  gMillis += dt;
  return dt;
}

// ---- random ----
static std::mt19937 gRng{std::random_device{}()};
long random(long howbig) {
  if (howbig <= 0) return 0;
  return (long)(gRng() % (uint32_t)howbig);
}
long random(long howsmall, long howbig) {
  if (howsmall >= howbig) return howsmall;
  return howsmall + random(howbig - howsmall);
}
void randomSeed(unsigned long seed) { gRng.seed((uint32_t)seed); }
uint32_t esp_random() { return gRng(); }

// ---- String ----
void String::trim() {
  size_t a = s_.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) { s_.clear(); return; }
  s_ = s_.substr(a, s_.find_last_not_of(" \t\r\n") - a + 1);
}
int String::indexOf(char c, unsigned from) const {
  size_t p = s_.find(c, from);
  return p == std::string::npos ? -1 : (int)p;
}
int String::indexOf(const char *t, unsigned from) const {
  size_t p = s_.find(t, from);
  return p == std::string::npos ? -1 : (int)p;
}
int String::lastIndexOf(char c) const {
  size_t p = s_.rfind(c);
  return p == std::string::npos ? -1 : (int)p;
}
String String::substring(unsigned left, unsigned right) const {
  if (right > s_.size()) right = (unsigned)s_.size();
  if (left >= right) return String("");
  return String(s_.substr(left, right - left));
}

// ---- serial console ----
HostSerial Serial;
static std::mutex gSerialMutex;
static std::deque<std::string> gSerialIn;

void hostSerialPush(const char *line) {
  std::lock_guard<std::mutex> lock(gSerialMutex);
  gSerialIn.push_back(line);
}
int HostSerial::available() {
  std::lock_guard<std::mutex> lock(gSerialMutex);
  return gSerialIn.empty() ? 0 : (int)gSerialIn.front().size() + 1;
}
String HostSerial::readStringUntil(char) {
  std::lock_guard<std::mutex> lock(gSerialMutex);
  if (gSerialIn.empty()) return String("");
  String s(gSerialIn.front());
  gSerialIn.pop_front();
  return s;
}
int HostSerial::printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vprintf(fmt, ap);
  va_end(ap);
  fflush(stdout);
  return n;
}
void HostSerial::print(const char *s) { fputs(s, stdout); fflush(stdout); }
void HostSerial::println(const char *s) { puts(s); fflush(stdout); }
size_t HostSerial::write(const uint8_t *data, size_t len) {
  size_t n = fwrite(data, 1, len, stdout);
  fflush(stdout);
  return n;
}

TwoWire Wire;
HostEsp ESP;
static bool gRestart = false;
void HostEsp::restart() { gRestart = true; }
bool hostRestartRequested(void) { return gRestart; }

// ---- input ----
static void (*gTouchIsr)() = nullptr;
static int16_t gPointerX = 0, gPointerY = 0;
static bool gPointerDown = false;
static uint32_t gPointerHoldUntil = 0;
static bool gButtonDown = false;
static bool gTiltLeft = false, gTiltRight = false;
static bool gArrowUp = false, gArrowDown = false;
static uint8_t gArrowLast = 0;  // the arrow pressed last: 1 left, 2 right, 3 up, 4 down
static float gTilt = 0;
static float gTiltSensor = 0;
static bool gTiltSensorOn = false;  // a real tilt sensor (a phone in the browser) instead of the keys

// the game polls the touch panel every 20 ms and resolves on release: a touch seen in two polls is not lost
#define POINTER_MIN_HOLD_MS 45
// seconds to reach the limit with the key held, and to return to straight on release
#define TILT_RAMP_S 0.12f
#define TILT_RELEASE_S 0.06f

void pinMode(uint8_t, uint8_t) {}
void digitalWrite(uint8_t, uint8_t) {}
int digitalRead(uint8_t pin) {
  if (pin == KNOMI_BTN_PIN) return gButtonDown ? LOW : HIGH;
  return HIGH;  // the touch panel's INT, at rest
}
void attachInterrupt(uint8_t pin, void (*fn)(), int) {
  if (pin == TP_INT) gTouchIsr = fn;
}

void hostPointer(int x, int y, bool down) {
  gPointerX = (int16_t)x;
  gPointerY = (int16_t)y;
  if (down && !gPointerDown) {
    gPointerHoldUntil = millis() + POINTER_MIN_HOLD_MS;
    if (gTouchIsr) gTouchIsr();
  }
  gPointerDown = down;
}
void hostPointerHover(void) { hostGameWake(); }
void hostTiltKeys(bool left, bool right) {
  if (left && !gTiltLeft) gArrowLast = 1;
  if (right && !gTiltRight) gArrowLast = 2;
  gTiltLeft = left;
  gTiltRight = right;
}
void hostArrowKeys(bool up, bool down) {
  if (up && !gArrowUp) gArrowLast = 3;
  if (down && !gArrowDown) gArrowLast = 4;
  gArrowUp = up;
  gArrowDown = down;
}
bool hostArrowDir(int8_t *dx, int8_t *dy) {
  const bool held[5] = { false, gTiltLeft, gTiltRight, gArrowUp, gArrowDown };
  uint8_t k = held[gArrowLast] ? gArrowLast : gArrowUp ? 3 : gArrowDown ? 4 : gTiltLeft ? 1 : gTiltRight ? 2 : 0;
  if (!k) return false;
  *dx = k == 1 ? -1 : k == 2 ? 1 : 0;
  *dy = k == 3 ? -1 : k == 4 ? 1 : 0;
  return true;
}
void hostTiltSensor(float x) {
  gTiltSensorOn = x == x;  // NaN: back to the keys
  if (gTiltSensorOn) gTiltSensor = x < -1 ? -1 : x > 1 ? 1 : x;
}
void hostButton(bool down) { gButtonDown = down; }

bool hostPointerRead(int16_t *x, int16_t *y) {
  *x = gPointerX;
  *y = gPointerY;
  return gPointerDown || (int32_t)(gPointerHoldUntil - millis()) > 0;
}

static void stepTilt(uint32_t dtMs) {
  if (gTiltSensorOn) {  // a sensor is already smooth: no ramp
    gTilt = gTiltSensor;
    return;
  }
  float target = (gTiltRight ? 1.0f : 0.0f) - (gTiltLeft ? 1.0f : 0.0f);
  float span = target == 0 ? TILT_RELEASE_S : TILT_RAMP_S;
  float step = dtMs / 1000.0f / span;
  if (gTilt < target) gTilt = gTilt + step > target ? target : gTilt + step;
  else if (gTilt > target) gTilt = gTilt - step < target ? target : gTilt - step;
}
float hostTiltValue() { return gTilt; }

// ---- screen ----
static bool gFrameDirty = false;
static uint8_t gBrightness = 180;

void hostPanelSetBrightness(uint8_t v) {
  if (v != gBrightness) gFrameDirty = true;
  gBrightness = v;
}
void hostCanvasFlushed() { gFrameDirty = true; }
bool hostFrameDirty(void) { return gFrameDirty; }

extern ScaledCanvas *gfx;

// The game's brightness protects the AMOLED from burn-in; here it only signals that it is idle, so
// it has a floor: a desktop pet must not go nearly black. 0 is a screen switched off by hand.
#define BRIGHTNESS_FULL 180
#define BRIGHTNESS_FLOOR 0.45f
void hostFrameRGBA(uint8_t *out) {
  gFrameDirty = false;
  const uint16_t *src = gfx->framebuffer();
  float f = gBrightness >= BRIGHTNESS_FULL ? 1.0f : (float)gBrightness / BRIGHTNESS_FULL;
  if (gBrightness == 0) f = 0;
  else if (f < BRIGHTNESS_FLOOR) f = BRIGHTNESS_FLOOR;
  uint32_t k = (uint32_t)(f * 256);
  for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
    uint16_t c = src[i];
    uint32_t r = (c >> 11) & 0x1F, g = (c >> 5) & 0x3F, b = c & 0x1F;
    *out++ = (uint8_t)((((r << 3) | (r >> 2)) * k) >> 8);
    *out++ = (uint8_t)((((g << 2) | (g >> 4)) * k) >> 8);
    *out++ = (uint8_t)((((b << 3) | (b >> 2)) * k) >> 8);
    *out++ = 255;
  }
}

uint16_t *ScaledCanvas::panelBuffer() {
  if (!panel_) panel_ = (uint16_t *)malloc(PANEL_SIZE * PANEL_SIZE * sizeof(uint16_t));
  // as knomi_hal.h's flush: each panel pixel the average of a 2x2 block starting on an even pixel
  for (int y = 0; y < PANEL_SIZE; y++)
    for (int x = 0; x < PANEL_SIZE; x++) {
      const uint16_t *a = framebuffer() + 2 * (y * (LCD_HEIGHT / 2) / PANEL_SIZE) * LCD_WIDTH + 2 * (x * (LCD_WIDTH / 2) / PANEL_SIZE);
      uint32_t s = 0;
      for (int k = 0; k < 4; k++) {
        uint16_t c = a[(k >> 1) * LCD_WIDTH + (k & 1)];
        s += (c | ((uint32_t)c << 16)) & 0x07E0F81F;
      }
      s = (s >> 2) & 0x07E0F81F;
      panel_[y * PANEL_SIZE + x] = (uint16_t)(s | (s >> 16));
    }
  return panel_;
}

// ---- resources ----
const uint8_t *u8g2_font_unifont_t_japanese3 = nullptr;
const uint8_t *u8g2_font_unifont_t_korean2 = nullptr;
static std::string gResources;
const char *hostResourcesDir() { return gResources.c_str(); }

static const uint8_t *loadFont(const char *name) {
  FILE *f = fopen((gResources + "/fonts/" + name).c_str(), "rb");
  if (!f) { fprintf(stderr, "missing font %s\n", name); return nullptr; }
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  rewind(f);
  uint8_t *data = (uint8_t *)malloc((size_t)n);
  if (data && fread(data, 1, (size_t)n, f) != (size_t)n) { free(data); data = nullptr; }
  fclose(f);
  return data;
}

static bool makeDirs(const std::string &path) {
  for (size_t i = 1; i <= path.size(); i++) {
    if (i < path.size() && path[i] != '/') continue;
    std::string sub = path.substr(0, i);
    if (sub.size() == 2 && sub[1] == ':') continue;  // a Windows drive ("C:")
    if (mkdir(sub.c_str(), 0755) != 0 && errno != EEXIST) return false;
  }
  return true;
}

// ---- lifecycle ----
void hostInit(const char *resources, const char *saveDir) {
  gResources = resources;
  if (saveDir && !makeDirs(saveDir)) fprintf(stderr, "cannot create %s\n", saveDir);
  hostPrefsSetDir(saveDir);
  u8g2_font_unifont_t_japanese3 = loadFont("unifont_t_japanese3.u8g2");
  u8g2_font_unifont_t_korean2 = loadFont("unifont_t_korean2.u8g2");
  advanceClock();
  hostGameSetup();
  hostPrefsFlush();
}

void hostTick(void) {
  stepTilt(advanceClock());
  hostGameLoop();
  hostPrefsFlush();
}

void hostFlushSave(void) {
  hostGameSave();
  hostPrefsFlush();
}
