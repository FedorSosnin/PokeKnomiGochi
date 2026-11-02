// Lo que en el Knomi es rtcbat.cpp, wifi_setup.cpp y buddy_net.cpp.
#include <Arduino.h>
#include <time.h>
#include "buddy_net.h"
#include "rtcbat.h"
#include "wifi_setup.h"

// ---- hora: la del sistema. El juego usa la hora LOCAL como epoch (ver openClock) ----
static int64_t gClockOffset = 0;  // lo que se haya ajustado a mano en la pantalla de hora

static uint32_t localEpoch() {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  return (uint32_t)(now + t.tm_gmtoff);
}

bool rtcBegin() { return true; }
uint32_t rtcEpoch() { return (uint32_t)((int64_t)localEpoch() + gClockOffset); }
void rtcSetEpoch(uint32_t e) { gClockOffset = (int64_t)e - (int64_t)localEpoch(); }

// Cada vez que el reloj real se adelanta al del juego (Mac en reposo), hay ausencia que aplicar.
// Se compara el tiempo UTC: un cambio de hora no es una ausencia.
#define CLOCK_GAP_S 120
bool clockPoll() {
  static time_t lastReal = 0;
  static uint32_t lastMs = 0;
  time_t real = time(nullptr);
  uint32_t ms = millis();
  bool gap = lastReal && (int64_t)(real - lastReal) - (int64_t)((ms - lastMs) / 1000) > CLOCK_GAP_S;
  if (!lastReal || gap || ms - lastMs >= 1000) {
    lastReal = real;
    lastMs = ms;
  }
  return gap;
}
const char *clockWifiSsid() { return ""; }
uint8_t clockWifiStatus() { return 1; }
void clockSetWifiSsid(const char *) {}
void clockPollPause(bool) {}
bool clockWasSeed() { return false; }

// ---- alimentacion: siempre enchufado ----
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

// ---- wifi: es cosa del sistema, el juego no la configura (ni muestra esa pagina en el host) ----
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

// ---- buddy: sin red todavia, el juego no ve a nadie ----
uint32_t buddyNetId() { return 1; }
bool buddyNetUp() { return false; }
bool buddyNetPoll(BuddyMsg &) { return false; }
void buddyNetSend(const BuddyMsg &) {}
void buddyNetSetPeer(uint32_t) {}
