#pragma once
#include <Arduino.h>

// PCF85063 RTC: persistent time as long as the board has power
bool rtcBegin();
uint32_t rtcEpoch();             // unix seconds; 0 if the RTC is not valid
void rtcSetEpoch(uint32_t e);
#ifdef BOARD_KNOMI_V2
bool clockPoll();                // poll in the loop; true right when the time is set via NTP
const char *clockWifiSsid();      // saved network, for the settings screen
uint8_t clockWifiStatus();       // 0 not tried, 1 network time correct, 2 no connection
void clockSetWifiSsid(const char *s);
void clockPollPause(bool p);
bool clockWasSeed();             // the last sync started from the seed: there is no absence to apply
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
