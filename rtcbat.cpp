#include "rtcbat.h"
#include "pin_config.h"  // define XPOWERS_CHIP_AXP2101

#ifdef BOARD_KNOMI_V2
// Sin RTC ni PMU. La hora es un reloj por software que se pone en hora por NTP con la wifi
// que el Knomi ya tenia guardada en la NVS (WiFi.begin() sin credenciales), asi que el
// firmware no maneja la contrasena. Sin red, setup() siembra con la ultima hora guardada.
#include <WiFi.h>
#include <esp_sntp.h>
#include <esp_wifi.h>
#include <time.h>

// Nueva York; el juego usa la hora LOCAL como epoch (ver openClock), no hay zonas horarias
#ifndef KNOMI_TZ
#define KNOMI_TZ "EST5EDT,M3.2.0,M11.1.0"
#endif
#define WIFI_TIMEOUT_MS 30000UL
#define NTP_RESYNC_MS (24UL * 3600UL * 1000UL)
#define NTP_RETRY_MS (10UL * 60UL * 1000UL)
#define WIFI_RECONNECT_MS 60000UL

static uint32_t baseEpoch = 0, baseMs = 0;

bool rtcBegin() { return true; }
uint32_t rtcEpoch() { return baseEpoch ? baseEpoch + (millis() - baseMs) / 1000 : 0; }
void rtcSetEpoch(uint32_t e) { baseEpoch = e; baseMs = millis(); }

// Si el reloj aun venia de la semilla fija de setup(), el salto a la hora de red no mide ninguna
// ausencia: aplicarlo envejeceria a la mascota hasta el tope de 2 semanas.
#define CLOCK_REAL_MIN 1790000000UL  // septiembre de 2026, muy por encima de la semilla
static bool wasSeed = false;
bool clockWasSeed() { return wasSeed; }

// segundos desde 1970 de una fecha civil, sin pasar por la zona horaria (dias de Hinnant)
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
static bool stayedOn = false;   // la wifi sigue encendida tras la ultima hora de red
static uint8_t wifiStatus = 0;  // 0 sin intentar, 1 ultima hora de red correcta, 2 sin conexion
static char savedSsid[33] = "";
static volatile uint8_t lastReason = 0;  // ultimo motivo de desconexion: dice si es la clave o la cobertura

const char *clockWifiSsid() { return savedSsid; }
uint8_t clockWifiStatus() { return wifiStatus; }
void clockSetWifiSsid(const char *s) { strlcpy(savedSsid, s, sizeof(savedSsid)); wifiStatus = 0; }

// la pantalla de cambio de wifi manda sobre la radio; al terminar se reintenta enseguida
void clockPollPause(bool p) {
  paused = p;
  st = CLK_IDLE;
  if (!p) first = true;
}

// Maquina de estados sin bloqueos: conecta, espera NTP y repite cada 24 h. La wifi se queda
// encendida si hubo red (el buddy la necesita, y el Knomi siempre va enchufado por USB).
bool clockPoll() {
  if (paused) return false;
  enum { IDLE = CLK_IDLE, CONNECTING = CLK_CONNECTING, WAIT_NTP = CLK_WAIT_NTP };
  uint32_t now = millis();
  switch (st) {
    case IDLE:
      if (stayedOn && WiFi.status() == WL_CONNECTED) {
        if (now - lastTry < wait) return false;
        lastTry = t0 = now;  // sigue conectada: basta con volver a preguntar la hora
        configTzTime(KNOMI_TZ, "pool.ntp.org", "time.google.com");
        st = WAIT_NTP;
        return false;
      }
      // se cayo la red (router reiniciado...): se reconecta pronto, sin esperar al NTP
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
        // lo guardado por el firmware anterior, pasado de nuevo para limpiar canal/BSSID viejos
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
        configTzTime(KNOMI_TZ, "pool.ntp.org", "time.google.com");
        t0 = now;
        st = WAIT_NTP;
      } else if (now - t0 > WIFI_TIMEOUT_MS) {
        wifi_config_t cfg = {};
        esp_wifi_get_config(WIFI_IF_STA, &cfg);
        Serial.printf("wifi: sin conexion (motivo %u, estado %d, ssid guardado '%s', clave de %u caracteres), sigue con el reloj interno\n",
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
        rtcSetEpoch(civilEpoch(tmv));
        Serial.printf("NTP ok, hora local %02d:%02d\n", tmv.tm_hour, tmv.tm_min);
      } else Serial.println("NTP: sin respuesta");
      stayedOn = WiFi.status() == WL_CONNECTED;
      if (stayedOn) WiFi.setSleep(false);  // con ahorro se pierden mensajes a toda la red
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
  if (!rtcOk) Serial.println("PCF85063 no detectado");
  return rtcOk;
}

uint32_t rtcEpoch() {
  if (!rtcOk) return 0;
  RTC_DateTime t = rtc.getDateTime();
  if (t.getYear() < 2025 || t.getYear() > 2120) return 0;  // sin hora valida
  struct tm tmv = {};
  tmv.tm_year = t.getYear() - 1900;
  tmv.tm_mon = t.getMonth() - 1;
  tmv.tm_mday = t.getDay();
  tmv.tm_hour = t.getHour();
  tmv.tm_min = t.getMinute();
  tmv.tm_sec = t.getSecond();
  time_t e = mktime(&tmv);  // TZ por defecto = UTC, consistente con gmtime_r
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

bool batBegin() {
  pmuOk = pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
  if (!pmuOk) { Serial.println("AXP2101 no detectado"); return false; }
  pmu.enableBattVoltageMeasure();
  pmu.setSysPowerDownVoltage(BAT_HW_CUTOFF_MV);  // respaldo: el PMU corta solo si el firmware no llega a tiempo
  return true;
}

// Enciende la alimentacion de la AMOLED. En la Waveshare 1.75 el panel (OLED VDD)
// cuelga del rail BLDO1 a 3.3V del AXP2101. El firmware daba por hecho que estaba
// encendido; si el PMU se resetea (drenaje total), BLDO1 queda OFF y la pantalla
// se ve negra aunque el resto funcione. Hay que llamarla ANTES de gfx->begin().
void pmuEnablePanel() {
  if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    Serial.println("AXP2101 no detectado (pmuEnablePanel)");
    return;
  }
  pmu.setBLDO1Voltage(3300);   // OLED VDD
  pmu.enableBLDO1();
}

// el estado de energia (I2C) se cachea ~2 s: leerlo en cada frame del loop
// metia trafico I2C inutil y podia oscilar (parpadeo de brillo)
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
  // En descarga el porcentaje sale de la curva de una celda LiPo (batcurve.h); con USB o cargando
  // la tension sube por la carga y no vale, y se usa la del PMU.
  bool discharging = conn && !cachedUsb && !cachedCharging;
  if (discharging != monDischarging) {
    mon.reset();
    mon.seed(cachedPct);  // al desenchufar el porcentaje no sube (la tension sigue inflada por la carga)
    monDischarging = discharging;
  }
  if (discharging) cachedPct = mon.sample((int)pmu.getBattVoltage(), now);
  else cachedPct = conn ? pmu.getBatteryPercent() : -1;
  if (cachedPct < 0 && conn) cachedPct = pmu.getBatteryPercent();  // sin lectura de tension
}

int batPercent() { refreshPower(); return cachedPct; }
bool batCharging() { refreshPower(); return cachedCharging; }
bool batShouldShutdown() { refreshPower(); return monDischarging && mon.shutdownNeeded(millis()); }
void batPowerOff() {
  if (pmuOk) pmu.shutdown();
  mon.reset();  // si sigue vivo (p. ej. se enchufo USB justo ahora) tiene que volver a cumplirse todo el tiempo
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
