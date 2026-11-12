#include "rtcbat.h"
#include "savename.h"
#include "pin_config.h"  // define XPOWERS_CHIP_AXP2101

#ifdef BOARD_KNOMI_V2
// No RTC or PMU. The time is a software clock set via NTP over the wifi
// the Knomi already had saved in the NVS (WiFi.begin() without credentials), so the
// firmware doesn't handle the password. Without a network, setup() seeds with the last saved time.
#include <Preferences.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <esp_wifi.h>
#include <time.h>
#include "timezones.h"
#define WIFI_TIMEOUT_MS 30000UL
#define NTP_RESYNC_MS (24UL * 3600UL * 1000UL)
#define NTP_RETRY_MS (10UL * 60UL * 1000UL)
#define WIFI_RECONNECT_MS 60000UL

// The clock keeps UTC (millis() on top of a base) and shows it in the chosen time zone: changing the zone
// moves the hours, never the time that passed. fromNet: the base came from NTP and nobody set it since.
static uint32_t baseUtc = 0, baseMs = 0;
static bool fromNet = false;
static int tzIdx = TIME_ZONE_DEFAULT;

static uint32_t civilEpoch(const struct tm &t);

static void tzUse(int i) {
  setenv("TZ", TIME_ZONES[i].posix, 1);
  tzset();
}

// the saved city (by name, so the list can grow); New York until one is picked
static void tzLoad() {
  Preferences p;
  p.begin(SAVE_NS, true);  // read-only
  char city[24] = "";
  if (p.isKey("tzc")) p.getString("tzc", city, sizeof(city));
  p.end();
  for (int i = 0; i < TIME_ZONE_COUNT; i++)
    if (!strcmp(city, TIME_ZONES[i].city)) tzIdx = i;
  tzUse(tzIdx);
}

static int32_t offsetAt(uint32_t utc) {  // local minus UTC in the zone in use
  time_t t = utc;
  struct tm lt;
  localtime_r(&t, &lt);
  return (int32_t)(civilEpoch(lt) - utc);
}

static uint32_t rtcUtc() {
  if (!baseUtc) return 0;
  uint32_t d = millis() - baseMs;
  if (d >= 86400000UL) {  // moves the base along once a day, so millis() wrapping (49 days) can't bite
    baseUtc += d / 1000;
    baseMs += d / 1000 * 1000;
    d = millis() - baseMs;
  }
  return baseUtc + d / 1000;
}

bool rtcBegin() {
  tzLoad();
  return true;
}
uint32_t rtcEpoch() {
  uint32_t u = rtcUtc();
  return u ? u + offsetAt(u) : 0;
}
void rtcSetEpoch(uint32_t e) {  // local: back to UTC (twice, for a day that changes to or from summer time)
  uint32_t u = e - offsetAt(e);
  baseUtc = e - offsetAt(u);
  baseMs = millis();
  fromNet = false;
}
bool clockFromNetwork() { return fromNet; }
uint32_t clockUtc() { return fromNet ? rtcUtc() : 0; }
uint32_t clockGapMin() { return 0; }  // millis() runs through light sleep: no gaps

int clockTzIndex() { return tzIdx; }
void clockSetTz(int i) {
  if (i < 0 || i >= TIME_ZONE_COUNT) return;
  tzIdx = i;
  tzUse(i);
  Preferences p;
  p.begin(SAVE_NS, false);
  p.putString("tzc", TIME_ZONES[i].city);
  p.end();
}
uint32_t clockLocalIn(int tz, uint32_t utc) {
  if (tz < 0 || tz >= TIME_ZONE_COUNT) return utc;
  tzUse(tz);
  uint32_t l = utc + offsetAt(utc);
  tzUse(tzIdx);
  return l;
}

// If the clock was still on setup()'s fixed seed, the jump to network time measures no
// absence: applying it would age the pet up to the 2-week cap.
#define CLOCK_REAL_MIN 1790000000UL  // September 2026, well above the seed
static bool wasSeed = false;
bool clockWasSeed() { return wasSeed; }

// seconds since 1970 of a civil date, without going through the time zone (Hinnant's days)
static uint32_t civilEpoch(const struct tm &t) {
  int y = t.tm_year + 1900, m = t.tm_mon + 1;
  y -= m <= 2;
  int era = y / 400;
  unsigned yoe = y - era * 400, doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + t.tm_mday - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  uint32_t days = era * 146097 + (int)doe - 719468;
  return days * 86400UL + t.tm_hour * 3600UL + t.tm_min * 60UL + t.tm_sec;
}

enum { CLK_IDLE, CLK_CONNECTING, CLK_WAIT_NTP };
static uint8_t st = CLK_IDLE;
static uint32_t t0 = 0, lastTry = 0, wait = 0;
static bool first = true, paused = false;
static bool stayedOn = false;   // wifi stays on after the last network time
static uint8_t wifiStatus = 0;  // 0 not tried, 1 last network time correct, 2 no connection
static char savedSsid[33] = "";
static volatile uint8_t lastReason = 0;  // last disconnect reason: tells whether it's the password or the coverage

const char *clockWifiSsid() { return savedSsid; }
uint8_t clockWifiStatus() { return wifiStatus; }
void clockSetWifiSsid(const char *s) { strlcpy(savedSsid, s, sizeof(savedSsid)); wifiStatus = 0; }

// the wifi change screen takes priority over the radio; when it ends it retries right away
void clockPollPause(bool p) {
  paused = p;
  st = CLK_IDLE;
  if (!p) first = true;
}

// Non-blocking state machine: connects, waits for NTP and repeats every 24 h. The wifi stays
// on if there was a network (the buddy needs it, and the Knomi is always plugged in via USB).
static bool clockStep();
uint8_t clockPoll() { return clockStep() ? CLOCK_TRUSTED : CLOCK_NONE; }
static bool clockStep() {
  if (paused) return false;
  enum { IDLE = CLK_IDLE, CONNECTING = CLK_CONNECTING, WAIT_NTP = CLK_WAIT_NTP };
  uint32_t now = millis();
  switch (st) {
    case IDLE:
      if (stayedOn && WiFi.status() == WL_CONNECTED) {
        if (now - lastTry < wait) return false;
        lastTry = t0 = now;  // still connected: just ask for the time again
        configTzTime(TIME_ZONES[tzIdx].posix, "pool.ntp.org", "time.google.com");
        st = WAIT_NTP;
        return false;
      }
      // the network dropped (router restarted...): reconnects soon, without waiting for NTP
      if (!first && now - lastTry < (stayedOn ? WIFI_RECONNECT_MS : wait)) return false;
      first = false;
      stayedOn = false;
      lastTry = t0 = now;
      WiFi.mode(WIFI_STA);
      {
        static bool hooked = false;
        if (!hooked) {
          hooked = true;
          WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t i) {
            lastReason = i.wifi_sta_disconnected.reason;
          }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
        }
        // what the previous firmware saved, passed through again to clear old channel/BSSID
        wifi_config_t cfg = {};
        esp_wifi_get_config(WIFI_IF_STA, &cfg);
        strlcpy(savedSsid, (const char *)cfg.sta.ssid, sizeof(savedSsid));
        if (cfg.sta.ssid[0]) WiFi.begin((const char *)cfg.sta.ssid, (const char *)cfg.sta.password);
        else WiFi.begin();
      }
      st = CONNECTING;
      return false;
    case CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("wifi ok, ip=%s\n", WiFi.localIP().toString().c_str());
        configTzTime(TIME_ZONES[tzIdx].posix, "pool.ntp.org", "time.google.com");
        t0 = now;
        st = WAIT_NTP;
      } else if (now - t0 > WIFI_TIMEOUT_MS) {
        wifi_config_t cfg = {};
        esp_wifi_get_config(WIFI_IF_STA, &cfg);
        Serial.printf("wifi: no connection (reason %u, state %d, saved ssid '%s', password of %u characters), continuing with the internal clock\n",
                      lastReason, (int)WiFi.status(), (const char *)cfg.sta.ssid, (unsigned)strlen((const char *)cfg.sta.password));
        WiFi.mode(WIFI_OFF);
        wait = NTP_RETRY_MS;
        wifiStatus = 2;
        st = IDLE;
      }
      return false;
    case WAIT_NTP: {
      bool synced = sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED || time(nullptr) > 1700000000;
      if (!synced && now - t0 < WIFI_TIMEOUT_MS) return false;
      struct tm tmv;
      time_t tt = time(nullptr);
      bool ok = synced && localtime_r(&tt, &tmv);
      if (ok) {
        wasSeed = rtcEpoch() < CLOCK_REAL_MIN;
        baseUtc = (uint32_t)tt;
        baseMs = millis();
        fromNet = true;
        Serial.printf("NTP ok, local time %02d:%02d (%s)\n", tmv.tm_hour, tmv.tm_min, TIME_ZONES[tzIdx].city);
      } else Serial.println("NTP: no response");
      stayedOn = WiFi.status() == WL_CONNECTED;
      if (stayedOn) WiFi.setSleep(false);  // with power saving, network-wide messages are lost
      else WiFi.mode(WIFI_OFF);
      wait = ok ? NTP_RESYNC_MS : NTP_RETRY_MS;
      wifiStatus = ok ? 1 : 2;
      st = IDLE;
      return ok;
    }
  }
  return false;
}

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

#else

#include "batcurve.h"
#include <Wire.h>
#include <time.h>
#include <SensorPCF85063.hpp>
#include <XPowersLib.h>

static SensorPCF85063 rtc;
static XPowersPMU pmu;
static bool rtcOk = false;
static bool pmuOk = false;

bool rtcBegin() {
  rtcOk = rtc.begin(Wire, IIC_SDA, IIC_SCL);
  if (!rtcOk) Serial.println("PCF85063 not detected");
  return rtcOk;
}

uint32_t rtcEpoch() {
  if (!rtcOk) return 0;
  RTC_DateTime t = rtc.getDateTime();
  if (t.getYear() < 2025 || t.getYear() > 2120) return 0;  // no valid time
  struct tm tmv = {};
  tmv.tm_year = t.getYear() - 1900;
  tmv.tm_mon = t.getMonth() - 1;
  tmv.tm_mday = t.getDay();
  tmv.tm_hour = t.getHour();
  tmv.tm_min = t.getMinute();
  tmv.tm_sec = t.getSecond();
  time_t e = mktime(&tmv);  // default TZ = UTC, consistent with gmtime_r
  return e > 0 ? (uint32_t)e : 0;
}

void rtcSetEpoch(uint32_t e) {
  if (!rtcOk) return;
  time_t tt = e;
  struct tm tmv;
  gmtime_r(&tt, &tmv);
  rtc.setDateTime(RTC_DateTime(tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                               tmv.tm_hour, tmv.tm_min, tmv.tm_sec));
}

bool clockFromNetwork() { return false; }  // no Wi-Fi time on this board: the battery-backed RTC, set by hand

bool batBegin() {
  pmuOk = pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
  if (!pmuOk) { Serial.println("AXP2101 not detected"); return false; }
  pmu.enableBattVoltageMeasure();
  pmu.setSysPowerDownVoltage(BAT_HW_CUTOFF_MV);  // backup: the PMU cuts off on its own if the firmware doesn't make it in time
  return true;
}

// Powers the AMOLED supply. On the Waveshare 1.75 the panel (OLED VDD)
// hangs off the AXP2101's 3.3V BLDO1 rail. The firmware assumed it was
// on; if the PMU resets (total drain), BLDO1 stays OFF and the screen
// looks black even though everything else works. Must be called BEFORE gfx->begin().
void pmuEnablePanel() {
  if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    Serial.println("AXP2101 not detected (pmuEnablePanel)");
    return;
  }
  pmu.setBLDO1Voltage(3300);   // OLED VDD
  pmu.enableBLDO1();
}

// power status (I2C) is cached ~2 s: reading it every loop frame
// added useless I2C traffic and could oscillate (brightness flicker)
static uint32_t powerCacheT = 0;
static int cachedPct = -1;
static bool cachedCharging = false, cachedUsb = true;

static BatMonitor mon;
static bool monDischarging = false;
static uint32_t pollMs = 2000;
void batSetPollMs(uint32_t ms) { pollMs = ms; }

static void refreshPower() {
  uint32_t now = millis();
  if (powerCacheT && now - powerCacheT < pollMs) return;
  powerCacheT = now ? now : 1;
  if (!pmuOk) { cachedPct = -1; cachedCharging = false; cachedUsb = true; return; }
  bool conn = pmu.isBatteryConnect();
  cachedCharging = pmu.isCharging();
  cachedUsb = pmu.isVbusIn();
  // In discharge the percentage comes from a single-cell LiPo curve (batcurve.h); with USB or charging
  // the voltage rises from the charge and is no good, so the PMU's is used.
  bool discharging = conn && !cachedUsb && !cachedCharging;
  if (discharging != monDischarging) {
    mon.reset();
    mon.seed(cachedPct);  // on unplugging the percentage doesn't rise (the voltage is still inflated by the charge)
    monDischarging = discharging;
  }
  if (discharging) cachedPct = mon.sample((int)pmu.getBattVoltage(), now);
  else cachedPct = conn ? pmu.getBatteryPercent() : -1;
  if (cachedPct < 0 && conn) cachedPct = pmu.getBatteryPercent();  // no voltage reading
}

int batPercent() { refreshPower(); return cachedPct; }
bool batCharging() { refreshPower(); return cachedCharging; }
bool batShouldShutdown() { refreshPower(); return monDischarging && mon.shutdownNeeded(millis()); }
void batPowerOff() {
  if (pmuOk) pmu.shutdown();
  mon.reset();  // if it is still alive (e.g. USB was just plugged in) everything must hold again for the whole time
}
int batMillivolts() { return pmuOk ? (int)pmu.getBattVoltage() : 0; }
bool usbPresent() { refreshPower(); return cachedUsb; }
bool usbPresentNow() { return pmuOk ? pmu.isVbusIn() : true; }

void pwrSetup() {
  if (!pmuOk) return;
  pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
  pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ);
  pmu.clearIrqStatus();
}

bool pwrShortPressed() {
  if (!pmuOk) return false;
  pmu.getIrqStatus();
  bool hit = pmu.isPekeyShortPressIrq();
  if (hit) pmu.clearIrqStatus();
  return hit;
}

#endif  // BOARD_KNOMI_V2
