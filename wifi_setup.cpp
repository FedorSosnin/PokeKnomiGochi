#include "wifi_setup.h"

#ifdef BOARD_KNOMI_V2
#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_random.h>
#include <esp_wifi.h>
#include <qrcode.h>
#include "rtcbat.h"
#include "wifi_form.h"

#define SETUP_TIMEOUT_MS (5UL * 60UL * 1000UL)  // nadie configura nada: se cierra sola
#define TRY_TIMEOUT_MS 20000UL
#define OK_SHOW_MS 5000UL
#define FAIL_SHOW_MS 5000UL
#define MAX_NETS 16
#define QR_MAX 41  // version 6

static WifiSetupState state = WS_OFF;
static uint32_t stateT = 0, lastUse = 0;
static char apSsid[20], apPass[12];
static char nets[MAX_NETS][33];
static int netCount = 0;
static uint8_t qr[QR_MAX][QR_MAX];
static int qrSize = 0;
static DNSServer dns;
static WebServer server(80);
static char trySsid[33], tryPass[64];
static bool tryPending = false;
static wifi_config_t oldCfg;

WifiSetupState wifiSetupState() { return state; }
const char *wifiSetupApSsid() { return apSsid; }
const char *wifiSetupApPass() { return apPass; }
int wifiSetupQrSize() { return qrSize; }
bool wifiSetupQrDark(int x, int y) { return x >= 0 && y >= 0 && x < qrSize && y < qrSize && qr[y][x]; }

static void setState(WifiSetupState s) {
  state = s;
  stateT = millis();
}

static void qrCopy(esp_qrcode_handle_t h) {
  int n = esp_qrcode_get_size(h);
  if (n > QR_MAX) return;
  qrSize = n;
  for (int y = 0; y < n; y++)
    for (int x = 0; x < n; x++) qr[y][x] = esp_qrcode_get_module(h, x, y);
}

// sin caracteres que se confundan al leerlos en una pantalla pequena
static void randomText(char *out, int len) {
  static const char ALPHA[] = "abcdefghjkmnpqrstuvwxyz23456789";
  for (int i = 0; i < len; i++) out[i] = ALPHA[esp_random() % (sizeof(ALPHA) - 1)];
  out[len] = 0;
}

static void sendPage(const char *notice) {
  String h;
  h.reserve(1800);
  h += F("<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
         "<title>TamaPoke Wi-Fi</title><style>body{font:17px system-ui;margin:24px;max-width:420px}"
         "input{font-size:17px;width:100%;box-sizing:border-box;padding:10px;margin:6px 0 16px}"
         "button{font-size:17px;padding:12px 20px}small{color:#666}</style><h2>TamaPoke Wi-Fi</h2>");
  if (notice) { h += "<p><b>"; h += notice; h += "</b></p>"; }
  h += F("<form method=post action=/save autocomplete=off><label>Network<input name=ssid list=nets maxlength=32 "
         "autocapitalize=none autocorrect=off required></label><datalist id=nets>");
  char esc[200];
  for (int i = 0; i < netCount; i++) {
    htmlEscape(nets[i], esc, sizeof(esc));
    h += "<option value=\""; h += esc; h += "\">";
  }
  h += F("</datalist><label>Password<input name=pass type=password maxlength=63 autocapitalize=none "
         "autocorrect=off></label><button>Connect</button></form>"
         "<p><small>Only 2.4 GHz networks work. Watch the TamaPoke screen for the result. "
         "If it fails, your previous network stays saved.</small></p>");
  server.send(200, "text/html", h);
}

static void onSave() {
  lastUse = millis();
  String s = server.arg("ssid"), p = server.arg("pass");
  if (!wifiFormValid(s.c_str(), p.c_str())) {
    sendPage("Check the name, and the password must be 8 to 63 characters.");
    return;
  }
  server.send(200, "text/html",
              "<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
              "<body style='font:17px system-ui;margin:24px'><h2>Connecting...</h2>"
              "<p>Look at the TamaPoke screen for the result.</p>");
  wifiSetupTry(s.c_str(), p.c_str());
}

static void redirectHome() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

static void boot() {
  char id[5];
  randomText(id, 4);
  snprintf(apSsid, sizeof(apSsid), "TamaPoke-%s", id);
  randomText(apPass, 8);

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect(false, false);  // no sigue intentando la red guardada: moveria el canal de la AP
  netCount = 0;
  int found = WiFi.scanNetworks();
  for (int i = 0; i < found; i++) {
    String name = WiFi.SSID(i);
    if (name.length() == 0 || name.length() > 32) continue;
    bool dup = false;
    for (int k = 0; k < netCount && !dup; k++) dup = name == nets[k];
    if (dup || netCount >= MAX_NETS) continue;
    strlcpy(nets[netCount++], name.c_str(), sizeof(nets[0]));
  }
  WiFi.scanDelete();
  Serial.printf("wifi setup: %d redes a la vista\n", netCount);

  WiFi.softAP(apSsid, apPass);
  dns.start(53, "*", WiFi.softAPIP());  // portal cautivo: cualquier dominio lleva a la pagina
  server.on("/", HTTP_GET, []() { lastUse = millis(); sendPage(nullptr); });
  server.on("/save", HTTP_POST, onSave);
  server.onNotFound(redirectHome);
  server.begin();

  char text[64];
  snprintf(text, sizeof(text), "WIFI:T:WPA;S:%s;P:%s;;", apSsid, apPass);
  qrSize = 0;
  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  cfg.display_func = qrCopy;
  cfg.max_qrcode_version = 6;
  cfg.qrcode_ecc_level = ESP_QRCODE_ECC_LOW;
  esp_qrcode_generate(&cfg, text);

  lastUse = millis();
  setState(WS_WAIT);
}

void wifiSetupStart() {
  if (state != WS_OFF) return;
  clockPollPause(true);
  setState(WS_BOOT);  // arranca en el siguiente sondeo: antes se pinta la pantalla
}

void wifiSetupStop() {
  if (state == WS_OFF) return;
  if (state != WS_BOOT) {
    server.stop();
    dns.stop();
    WiFi.softAPdisconnect(true);
  }
  esp_wifi_set_storage(WIFI_STORAGE_FLASH);  // por si se cancela con un intento en curso
  WiFi.mode(WIFI_OFF);
  memset(tryPass, 0, sizeof(tryPass));
  memset(&oldCfg, 0, sizeof(oldCfg));
  setState(WS_OFF);
  clockPollPause(false);  // vuelve a poner la hora con la red que quede guardada
}

void wifiSetupTry(const char *ssid, const char *pass) {
  if (state != WS_WAIT) return;
  strlcpy(trySsid, ssid, sizeof(trySsid));
  strlcpy(tryPass, pass, sizeof(tryPass));
  tryPending = true;  // desde el sondeo: la respuesta HTTP tiene que salir antes
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
  if (state == WS_BOOT) {
    if (now - stateT > 150) boot();
    return;
  }
  dns.processNextRequest();
  server.handleClient();
  switch (state) {
    case WS_WAIT:
      if (tryPending) {
        tryPending = false;
        esp_wifi_get_config(WIFI_IF_STA, &oldCfg);
        Serial.printf("wifi setup: probando '%s'\n", trySsid);
        // en RAM hasta saber que funciona: ni un fallo ni un corte de luz tocan la red guardada
        esp_wifi_set_storage(WIFI_STORAGE_RAM);
        WiFi.begin(trySsid, tryPass);
        setState(WS_TRY);
      } else if (now - lastUse > SETUP_TIMEOUT_MS) {
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
      if (now - stateT > FAIL_SHOW_MS) {
        lastUse = now;
        setState(WS_WAIT);  // la red de configuracion sigue abierta: se puede reintentar
      }
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
const char *wifiSetupApSsid() { return ""; }
const char *wifiSetupApPass() { return ""; }
int wifiSetupQrSize() { return 0; }
bool wifiSetupQrDark(int, int) { return false; }
void wifiSetupTry(const char *, const char *) {}
#endif
