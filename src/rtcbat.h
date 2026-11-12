#pragma once
#include <Arduino.h>

// The clock. rtcEpoch() is LOCAL time counted as if it were UTC (the game's days and hours run on it).
bool rtcBegin();
uint32_t rtcEpoch();             // local seconds since 1970; 0 if there is no time yet
void rtcSetEpoch(uint32_t e);    // sets it by hand (the time screen, the seed): no longer network time
bool clockFromNetwork();         // network time (NTP, a time server): the time screen only shows it

#ifdef BOARD_KNOMI_V2
// clockPoll()'s news for the loop
enum ClockEvent : uint8_t {
  CLOCK_NONE = 0,
  CLOCK_TRUSTED,  // network time just arrived: clockUtc() is it (Pet::trustedTime)
  CLOCK_GAP,      // the game wasn't running for clockGapMin() minutes, by a monotonic clock (hosts)
};
uint8_t clockPoll();             // poll in the loop
uint32_t clockUtc();             // network time now (UTC seconds), 0 without it
uint32_t clockGapMin();          // CLOCK_GAP's minutes
bool clockWasSeed();             // the last sync started from the seed: there is no absence to apply
const char *clockWifiSsid();      // saved network, for the settings screen
uint8_t clockWifiStatus();       // 0 not tried, 1 network time correct, 2 no connection
void clockSetWifiSsid(const char *s);
void clockPollPause(bool p);
// time zones (timezones.h). Mac, Windows and the web follow the computer's: clockTzIndex() is -1 there
int clockTzIndex();
void clockSetTz(int i);          // saved; the local time moves to it at once
uint32_t clockLocalIn(int tz, uint32_t utc);  // local time in TIME_ZONES[tz] at utc (the picker's preview)
#endif

// AXP2101 PMU: battery status
bool batBegin();
void pmuEnablePanel();           // turns on BLDO1 (OLED VDD 3.3V); call before gfx->begin()
void batSetPollMs(uint32_t ms);   // how often the PMU is read over I2C (lengthened with the screen off)
int batPercent();                // 0-100, -1 if no battery is connected
bool batCharging();
int batMillivolts();               // battery voltage in mV, 0 if no reading
bool batShouldShutdown();          // discharged to the safe limit (batcurve.h): save and shut down
void batPowerOff();                // physical PMU power-off
bool usbPresent();
bool usbPresentNow();             // uncached (one I2C read): to decide whether it can sleep

// AXP2101 PWR button: long press 4s = physical power-off (RTC stays alive);
// the short press is captured by the firmware (screen on/off)
void pwrSetup();
bool pwrShortPressed();  // poll in the loop
