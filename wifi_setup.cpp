#include "wifi_setup.h"

#ifdef BOARD_KNOMI_V2
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "rtcbat.h"
#include "wifi_form.h"

#define SETUP_TIMEOUT_MS (5UL * 60UL * 1000UL)  // nadie configura nada: se cierra sola
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

// la mas fuerte primero, sin repetir nombres (una red puede tener varios puntos de acceso)
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
    if (name.length() == 0 || name.length() > 32) continue;  // oculta o invalida
    bool dup = false;
    for (int k = 0; k < netCount && !dup; k++) dup = name == nets[k];
    if (dup) continue;
    netOpen[netCount] = WiFi.encryptionType(best) == WIFI_AUTH_OPEN;
    strlcpy(nets[netCount++], name.c_str(), sizeof(nets[0]));
  }
  WiFi.scanDelete();
  Serial.printf("wifi setup: %d redes a la vista\n", netCount);
  setState(WS_LIST);
}

void wifiSetupStart() {
  if (state != WS_OFF) return;
  clockPollPause(true);
  picked = false;
  setState(WS_SCAN);  // busca en el siguiente sondeo: antes se pinta el aviso
}

void wifiSetupRescan() {
  if (state == WS_LIST) setState(WS_SCAN);
}

void wifiSetupStop() {
  if (state == WS_OFF) return;
  esp_wifi_set_storage(WIFI_STORAGE_FLASH);  // por si se cancela con un intento en curso
  WiFi.mode(WIFI_OFF);
  memset(tryPass, 0, sizeof(tryPass));
  memset(&oldCfg, 0, sizeof(oldCfg));
  passLen = 0;
  setState(WS_OFF);
  clockPollPause(false);  // vuelve a poner la hora con la red que quede guardada
}

static void beginTry() {
  esp_wifi_get_config(WIFI_IF_STA, &oldCfg);
  Serial.printf("wifi setup: probando '%s'\n", trySsid);
  // en RAM hasta saber que funciona: ni un fallo ni un corte de luz tocan la red guardada
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

// Vuelve a guardar en flash y escribe cfg (la configuracion activa si es nullptr). El driver
// rechaza cambios mientras intenta conectar, asi que antes corta y espera a que pare.
static bool applyStaConfig(wifi_config_t *cfg) {
  wifi_config_t current;
  if (!cfg) {
    esp_wifi_get_config(WIFI_IF_STA, &current);
    cfg = &current;
  }
  esp_wifi_disconnect();
  esp_wifi_set_storage(WIFI_STORAGE_FLASH);
  esp_err_t err = ESP_FAIL;
  for (int i = 0; i < 20 && err != ESP_OK; i++) {
    delay(50);
    err = esp_wifi_set_config(WIFI_IF_STA, cfg);
  }
  memset(&current, 0, sizeof(current));
  return err == ESP_OK;
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
        Serial.println("wifi setup: sin uso, se cierra");
        wifiSetupStop();
      }
      break;
    case WS_TRY:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("wifi setup: conectado, ip=%s\n", WiFi.localIP().toString().c_str());
        bool saved = applyStaConfig(nullptr);
        Serial.println(saved ? "wifi setup: guardada" : "wifi setup: no se pudo guardar");
        if (saved) clockSetWifiSsid(trySsid);
        setState(saved ? WS_OK : WS_FAIL);
      } else if (now - stateT > TRY_TIMEOUT_MS) {
        Serial.println("wifi setup: fallo, se recupera la red anterior");
        applyStaConfig(&oldCfg);
        memset(&oldCfg, 0, sizeof(oldCfg));
        setState(WS_FAIL);
      }
      break;
    case WS_OK:
      if (now - stateT > OK_SHOW_MS) wifiSetupStop();
      break;
    case WS_FAIL:
      // vuelve al teclado con lo escrito: lo normal es corregir una errata
      if (now - stateT > FAIL_SHOW_MS) setState(picked && passLen ? WS_PASS : WS_LIST);
      break;
    default: break;
  }
}

#else
// la Waveshare no usa wifi
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
