#include "wifi_setup.h"

#ifdef BOARD_KNOMI_V2
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <nvs.h>
#include "rtcbat.h"
#include "wifi_form.h"

#define SETUP_TIMEOUT_MS (5UL * 60UL * 1000UL)  // nobody configures anything: it closes itself
#define TRY_TIMEOUT_MS 20000UL
#define OK_SHOW_MS 4000UL
#define FAIL_SHOW_MS 4000UL
#define MAX_NETS 15

static WifiSetupState state = WS_OFF;
static uint32_t stateT = 0, lastUse = 0;
static char nets[MAX_NETS][33];
static bool netOpen[MAX_NETS];
static int netCount = 0;
static char trySsid[33], tryPass[64];
static uint8_t passLen = 0;
static bool picked = false;
static wifi_config_t oldCfg;

WifiSetupState wifiSetupState() { return state; }
int wifiSetupNetCount() { return netCount; }
const char *wifiSetupNet(int i) { return i >= 0 && i < netCount ? nets[i] : ""; }
const char *wifiSetupSsid() { return trySsid; }
const char *wifiSetupPass() { return tryPass; }

static void setState(WifiSetupState s) {
  state = s;
  stateT = millis();
  lastUse = stateT;
}

// strongest first, without repeating names (a network can have several access points)
static void scan() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  netCount = 0;
  int found = WiFi.scanNetworks();
  bool used[64] = {};
  if (found > 64) found = 64;
  while (netCount < MAX_NETS) {
    int best = -1;
    for (int i = 0; i < found; i++)
      if (!used[i] && (best < 0 || WiFi.RSSI(i) > WiFi.RSSI(best))) best = i;
    if (best < 0) break;
    used[best] = true;
    String name = WiFi.SSID(best);
    if (name.length() == 0 || name.length() > 32) continue;  // hidden or invalid
    bool dup = false;
    for (int k = 0; k < netCount && !dup; k++) dup = name == nets[k];
    if (dup) continue;
    netOpen[netCount] = WiFi.encryptionType(best) == WIFI_AUTH_OPEN;
    strlcpy(nets[netCount++], name.c_str(), sizeof(nets[0]));
  }
  WiFi.scanDelete();
  Serial.printf("wifi setup: %d networks in view\n", netCount);
  setState(WS_LIST);
}

void wifiSetupStart() {
  if (state != WS_OFF) return;
  clockPollPause(true);
  picked = false;
  setState(WS_SCAN);  // scans on the next poll: the notice is drawn first
}

void wifiSetupRescan() {
  if (state == WS_LIST) setState(WS_SCAN);
}

void wifiSetupStop() {
  if (state == WS_OFF) return;
  esp_wifi_set_storage(WIFI_STORAGE_FLASH);  // in case it is cancelled with an attempt in progress
  WiFi.mode(WIFI_OFF);
  memset(tryPass, 0, sizeof(tryPass));
  memset(&oldCfg, 0, sizeof(oldCfg));
  passLen = 0;
  setState(WS_OFF);
  clockPollPause(false);  // sets the time again with whichever network remains saved
}

static void beginTry() {
  esp_wifi_get_config(WIFI_IF_STA, &oldCfg);
  Serial.printf("wifi setup: trying '%s'\n", trySsid);
  // in RAM until it is known to work: neither a failure nor a power cut touches the saved network
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  WiFi.begin(trySsid, tryPass);
  setState(WS_TRY);
}

void wifiSetupPick(int i) {
  if (state != WS_LIST || i < 0 || i >= netCount) return;
  strlcpy(trySsid, nets[i], sizeof(trySsid));
  memset(tryPass, 0, sizeof(tryPass));
  passLen = 0;
  picked = true;
  if (netOpen[i]) beginTry();
  else setState(WS_PASS);
}

void wifiSetupBackToList() {
  if (state != WS_PASS) return;
  memset(tryPass, 0, sizeof(tryPass));
  passLen = 0;
  picked = false;
  setState(WS_LIST);
}

void wifiSetupType(char c) {
  lastUse = millis();
  if (state == WS_PASS && passLen < sizeof(tryPass) - 1) {
    tryPass[passLen++] = c;
    tryPass[passLen] = 0;
  }
}

void wifiSetupDelete() {
  lastUse = millis();
  if (state == WS_PASS && passLen) tryPass[--passLen] = 0;
}

bool wifiSetupCanSubmit() { return state == WS_PASS && passLen >= 8 && wifiFormValid(trySsid, tryPass); }

void wifiSetupSubmit() {
  if (wifiSetupCanSubmit()) beginTry();
}

void wifiSetupTry(const char *ssid, const char *pass) {
  if (state != WS_LIST && state != WS_PASS) return;
  if (!wifiFormValid(ssid, pass)) return;
  strlcpy(trySsid, ssid, sizeof(trySsid));
  strlcpy(tryPass, pass, sizeof(tryPass));
  passLen = strlen(tryPass);
  picked = false;
  beginTry();
}

// Saves to flash again and writes cfg (the active configuration if nullptr). The driver
// rejects changes while it is trying to connect, so it first disconnects and waits for it to stop.
static bool writeStaConfig(wifi_config_t *cfg) {
  esp_err_t err = ESP_FAIL;
  for (int i = 0; i < 20 && err != ESP_OK; i++) {
    delay(50);
    err = esp_wifi_set_config(WIFI_IF_STA, cfg);
  }
  return err == ESP_OK;
}

static bool applyStaConfig(wifi_config_t *cfg) {
  wifi_config_t current, blank;
  memset(&blank, 0, sizeof(blank));
  if (!cfg) {
    esp_wifi_get_config(WIFI_IF_STA, &current);
    cfg = &current;
  }
  esp_wifi_disconnect();
  esp_wifi_set_storage(WIFI_STORAGE_FLASH);
  // the driver may skip the write if the configuration doesn't change relative to RAM's:
  // going through an empty one first, the good one always reaches the NVS
  bool ok = writeStaConfig(&blank) && writeStaConfig(cfg);
  memset(&current, 0, sizeof(current));
  return ok;
}

// checks in the NVS that the network was really saved
static bool savedSsidIs(const char *ssid) {
  nvs_handle_t h;
  if (nvs_open("nvs.net80211", NVS_READONLY, &h) != ESP_OK) return false;
  uint8_t blob[64];
  size_t len = sizeof(blob);
  bool ok = nvs_get_blob(h, "sta.ssid", blob, &len) == ESP_OK &&
            memmem(blob, len, ssid, strlen(ssid)) != nullptr;
  nvs_close(h);
  return ok;
}

void wifiSetupPoll() {
  if (state == WS_OFF) return;
  uint32_t now = millis();
  switch (state) {
    case WS_SCAN:
      if (now - stateT > 150) scan();
      break;
    case WS_LIST:
    case WS_PASS:
      if (now - lastUse > SETUP_TIMEOUT_MS) {
        Serial.println("wifi setup: unused, closing");
        wifiSetupStop();
      }
      break;
    case WS_TRY:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("wifi setup: connected, ip=%s\n", WiFi.localIP().toString().c_str());
        bool saved = applyStaConfig(nullptr) && savedSsidIs(trySsid);
        Serial.println(saved ? "wifi setup: saved and verified" : "wifi setup: could not save");
        if (saved) clockSetWifiSsid(trySsid);
        setState(saved ? WS_OK : WS_FAIL);
      } else if (now - stateT > TRY_TIMEOUT_MS) {
        Serial.println("wifi setup: failed, restoring the previous network");
        applyStaConfig(&oldCfg);
        memset(&oldCfg, 0, sizeof(oldCfg));
        setState(WS_FAIL);
      }
      break;
    case WS_OK:
      if (now - stateT > OK_SHOW_MS) wifiSetupStop();
      break;
    case WS_FAIL:
      // back to the keyboard with what was typed: the usual case is fixing a typo
      if (now - stateT > FAIL_SHOW_MS) setState(picked && passLen ? WS_PASS : WS_LIST);
      break;
    default: break;
  }
}

#else
// the Waveshare does not use wifi
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
#endif
