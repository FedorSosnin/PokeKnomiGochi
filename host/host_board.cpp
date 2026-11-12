// What rtcbat.cpp and wifi_setup.cpp are on the Knomi (buddy_net.cpp is host_buddy.cpp).
#include <Arduino.h>
#include <time.h>
#include <string>
#include "rtcbat.h"
#include "wifi_setup.h"

// ---- time ----
// The game's clock is UTC from an anchor, moved along by a monotonic clock that keeps counting while
// the computer sleeps, and shown in the computer's time zone. So changing the computer's clock does
// nothing: the anchor is the computer's clock only until a time server answers (HOST_TIME_URL's Date
// header, fetched by the front end). Time away comes from that server or the monotonic clock, never
// from the computer's clock. The game uses LOCAL time as its epoch (see openClock).
#ifdef _WIN32  // just the one call: windows.h clashes with the Arduino shim
extern "C" __declspec(dllimport) unsigned long long __stdcall GetTickCount64(void);
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#include <math.h>
#include "host.h"
#include "../src/timezones.h"

static double monoS() {  // seconds; keeps counting while asleep where the system allows
#ifdef _WIN32
  return GetTickCount64() / 1000.0;  // includes sleep and hibernation
#elif defined(__EMSCRIPTEN__)
  return emscripten_get_now() / 1000.0;  // performance.now(): runs on in a hidden tab, maybe not asleep
#elif defined(__APPLE__)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);  // on macOS, counts while asleep
  return ts.tv_sec + ts.tv_nsec / 1e9;
#else
  struct timespec ts;
  clock_gettime(CLOCK_BOOTTIME, &ts);  // Linux: counts while suspended
  return ts.tv_sec + ts.tv_nsec / 1e9;
#endif
}

static int32_t osOffset(uint32_t utc) {  // the computer's local time minus UTC, at utc
  time_t t = utc;
  struct tm lt;
#ifdef _WIN32  // no tm_gmtoff: the local time read back as if it were UTC gives the offset
  localtime_s(&lt, &t);
  return (int32_t)(_mkgmtime(&lt) - t);
#else
  localtime_r(&t, &lt);
  return (int32_t)lt.tm_gmtoff;
#endif
}

#define CLOCK_GAP_S 120              // a game pause this long is time away (shorter ones are just lost)
#define NET_RETRY_MS 120000UL        // no answer: ask again in 2 minutes
#define NET_RECHECK_MS 900000UL      // the computer's clock disagrees: ask at most every 15 minutes
static double gAnchorUtc = 0, gAnchorMono = 0;
static bool gAnchored = false, gNet = false;
static int64_t gManual = 0;          // the console's TIME / RTCSET (tests, screenshots): seconds on top
static bool gNetWanted = true, gNetAsked = false, gNetGot = false;
static uint32_t gNetUtc = 0, gNetAskMs = 0, gGapMin = 0;
static double gNetMono = 0;

static double utcNow() {
  if (!gAnchored) {
    gAnchorUtc = (double)time(nullptr);
    gAnchorMono = monoS();
    gAnchored = true;
  }
  return gAnchorUtc + (monoS() - gAnchorMono);
}

bool rtcBegin() { return true; }
uint32_t rtcEpoch() {
  uint32_t u = (uint32_t)utcNow();
  return (uint32_t)((int64_t)u + osOffset(u) + gManual);
}
void rtcSetEpoch(uint32_t e) { gManual += (int64_t)e - (int64_t)rtcEpoch(); }
bool clockFromNetwork() { return gNet && !gManual; }
uint32_t clockUtc() { return gNet ? (uint32_t)utcNow() : 0; }
uint32_t clockGapMin() { return gGapMin; }

extern "C" bool hostNetTimeWanted(void) {
  if (!gNetWanted || gNetAsked) return false;
  gNetWanted = false;
  gNetAsked = true;
  gNetAskMs = millis();
  return true;
}

extern "C" void hostNetTime(uint32_t utc) {
  gNetAsked = false;
  if (utc < 1767225600UL || utc > 4102444800UL) return;  // failed (or not 2026-2100): clockPoll() asks again
  gNetUtc = utc;
  gNetMono = monoS();
  gNetGot = true;
}

uint8_t clockPoll() {
  static double lastMono = -1;
  static uint32_t lastMs = 0, lastCheckMs = 0;
  double m = monoS();
  uint32_t ms = millis();
  if (gNetGot) {  // the time server's answer: the clock follows it from now on
    gNetGot = false;
    gAnchorUtc = gNetUtc;
    gAnchorMono = gNetMono;
    gAnchored = gNet = true;
    lastCheckMs = ms;
    return CLOCK_TRUSTED;
  }
  // the monotonic clock ran on while the game didn't (asleep, App Nap, a hidden tab): time away
  bool gap = lastMono >= 0 && (m - lastMono) - (double)(ms - lastMs) / 1000.0 > CLOCK_GAP_S;
  if (gap) gGapMin = (uint32_t)(((m - lastMono) - (double)(ms - lastMs) / 1000.0) / 60.0);
  if (lastMono < 0 || gap || ms - lastMs >= 1000) {
    lastMono = m;
    lastMs = ms;
  }
  if (gap) {
    gNetWanted = true;  // a browser's clock may have stopped asleep: the server tells what it missed
    return CLOCK_GAP;
  }
  if (gNetAsked && ms - gNetAskMs > 30000UL) gNetAsked = false;  // never answered: as if it failed
  if (!gNetAsked && !gNetWanted && ms - lastCheckMs >= (gNet ? NET_RECHECK_MS : NET_RETRY_MS)) {
    lastCheckMs = ms;
    // no server yet; or the computer's clock and ours disagree: set by hand (ignored), or asleep
    // without the monotonic clock seeing it (a browser): the server settles it
    if (!gNet || fabs((double)time(nullptr) - utcNow()) > CLOCK_GAP_S) gNetWanted = true;
  }
  return CLOCK_NONE;
}

int clockTzIndex() { return -1; }  // the computer's time zone
void clockSetTz(int) {}
// the picker's preview, for screenshots on the host (the picker itself is the Knomi's)
uint32_t clockLocalIn(int tz, uint32_t utc) {
  if (tz < 0 || tz >= TIME_ZONE_COUNT) return utc;
#ifdef _WIN32
  return utc;
#else
  const char *old = getenv("TZ");
  std::string keep = old ? old : "";
  setenv("TZ", TIME_ZONES[tz].posix, 1);
  tzset();
  int32_t off = osOffset(utc);
  if (old) setenv("TZ", keep.c_str(), 1);
  else unsetenv("TZ");
  tzset();
  return utc + off;
#endif
}

const char *clockWifiSsid() { return ""; }
uint8_t clockWifiStatus() { return 1; }
void clockSetWifiSsid(const char *) {}
void clockPollPause(bool) {}
bool clockWasSeed() { return false; }

// ---- power: always plugged in ----
bool batBegin() { return false; }
void pmuEnablePanel() {}
void batSetPollMs(uint32_t) {}
int batPercent() { return -1; }
bool batCharging() { return false; }
int batMillivolts() { return 0; }
bool batShouldShutdown() { return false; }
void batPowerOff() {}
bool usbPresent() { return true; }
bool usbPresentNow() { return true; }
void pwrSetup() {}
bool pwrShortPressed() { return false; }

// ---- wifi: handled by the system; the game does not configure it (nor show that page on the host) ----
void wifiSetupStart() {}
void wifiSetupStop() {}
void wifiSetupPoll() {}
WifiSetupState wifiSetupState() { return WS_OFF; }
int wifiSetupNetCount() { return 0; }
const char *wifiSetupNet(int) { return ""; }
void wifiSetupRescan() {}
void wifiSetupPick(int) {}
void wifiSetupBackToList() {}
const char *wifiSetupSsid() { return ""; }
const char *wifiSetupPass() { return ""; }
void wifiSetupType(char) {}
void wifiSetupDelete() {}
bool wifiSetupCanSubmit() { return false; }
void wifiSetupSubmit() {}
void wifiSetupTry(const char *, const char *) {}
