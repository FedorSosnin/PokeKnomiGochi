// TamaPoke - tamagotchi pixel art inspirado en la gen 1
// para Waveshare ESP32-S3-Touch-AMOLED-1.75
//
// Librerias (Library Manager o repo de Waveshare):
//   - "GFX Library for Arduino" (moononournation), con soporte CO5300 QSPI
//   - "SensorLib" (Lewis He), driver tactil CST9217
//
// Placa: ESP32S3 Dev Module | Flash 16MB | PSRAM: OPI PSRAM | USB CDC On Boot: Enabled
//
// Los sprites y la tabla de especies se generan con tools/sprites.py (emit).

#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#ifdef BOARD_KNOMI_V2
#include "knomi_hal.h"
#else
#include "TouchDrvCSTXXX.hpp"
#endif
#include <U8g2lib.h>  // fuentes CJK (japones); ver applyLangFont()
#include <esp_sleep.h>
#include <driver/gpio.h>
#include "pin_config.h"
#include "species.h"
#include "dex.h"
#include "pet.h"
#include "sdmon.h"
#include "rtcbat.h"
#include "i18n.h"
#include "audio.h"
#include "blit.h"
#include "power.h"
#include "battle.h"
#include "dexfacts.h"
#include "textwrap.h"
#include "wifi_setup.h"
#include "kblayout.h"
#include "minigames.h"
#include "pokerun.h"

// Version del firmware. Subir este numero en cada release (y manifest.json para
// el instalador web). Se muestra en la pantalla de ajustes y por serie al arrancar.
#define FW_VERSION "1.18"

#ifdef BOARD_KNOMI_V2
Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, GFX_NOT_DEFINED);
KnomiPanel *panel = new KnomiPanel(bus, LCD_RESET, 0 /*rotation*/, true /*IPS*/, PANEL_SIZE, PANEL_SIZE);
ScaledCanvas *gfx = new ScaledCanvas(panel);

KnomiTouch touch;
#else
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_CO5300 *panel = new Arduino_CO5300(
  bus, LCD_RESET, 0 /*rotation*/, LCD_WIDTH, LCD_HEIGHT, 6, 0, 0, 0);
// Framebuffer completo en PSRAM: dibujamos todo y hacemos flush() (sin parpadeo)
Arduino_Canvas *gfx = new Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, panel);

TouchDrvCST92xx touch;
#endif
// fuente CJK activa: de momento siempre false, la carga llegara con el
// soporte de japones/coreano/chino (ver PR #13)
bool gCjkFont = false;
#ifdef BOARD_KNOMI_V2
#define TOUCH_ADDR 0x15  // CST816S
#else
#define TOUCH_ADDR 0x5A  // CST9217
#endif
Pet pet;

// sprite animado de la SD para la especie actual (si existe el archivo)
SdMon mon;          // sprite B/N (respaldo y minijuego si no hay PMD)
PmdMon pmd;         // sprite PMD multi-accion (pantalla principal)
PmdMon evoPmd;      // forma anterior, solo durante el parpadeo de evolucion
int16_t monFor = -2;
bool monShinyFor = false;

// comportamiento del bicho en pantalla
struct {
  uint8_t mode = 0;     // 0 idle, 1 paseo, 2 gesto one-shot
  uint8_t act = PMD_IDLE;
  uint32_t t0 = 0;      // inicio de la animacion en curso
  uint32_t until = 0;   // fin del estado actual
  float x = 233, targetX = 233;
} beh;
uint32_t walkLast = 0;  // instante del ultimo paso del paseo (0 = ninguno aun)
#define PET_GROUND 304  // linea de suelo de la mascota
PmdMon galleryPmd;  // sprite grande de la vista detalle de la galeria (PMD/TPK2, legal)

// galeria pokedex
bool galleryOpen = false;
bool galleryDirty = false;
int galleryPage = 0;        // 10 paginas de 16
int eggPickPage = 0;        // eleccion del huevo: rejilla solo con lo registrado
int16_t eggPickDetail = 0;
bool eggPickDirty = true;
int16_t galleryDetail = 0;  // dex en vista detalle, 0 = rejilla

bool screenOff = false;       // pulsacion corta del boton PWR
bool cardOpen = false;        // ficha del bicho (deslizar vertical)
bool kbOpen = false;          // teclado para renombrar al bicho
char nameBuf[12] = "";
uint8_t nameLen = 0;
uint8_t cardPage = 0;         // 0 perfil, 1 stats+medallas
uint8_t clockPage = 0;        // 0 hora, 1 ajustes (solo Knomi)
uint8_t wifiListPage = 0, wifiKbLayer = 0;
int8_t wifiKbHover = -1;      // tecla bajo el dedo: se escribe al levantarlo
int16_t wifiKbX = 0, wifiKbY = 0;
#define KB_COLS 6
#define KB_X 40
#define KB_Y 150
#define KB_W 64
#define KB_H 52
bool clockOpen = false;       // pantalla de ajuste de hora (deslizar abajo)
int clockH = 12, clockM = 0;  // hora en edicion
bool clockEdited = false;      // tocada a mano: no pisarla con la hora de red

// escena de bano: espuma sobre el bicho y limpieza al reventar
uint32_t bathUntil = 0;
bool bathPending = false;
struct { int16_t x, y; uint8_t r, ph; } bubbles[14];
uint32_t feedMenuUntil = 0;   // selector de comida abierto hasta este millis

// minijuego "toques": mantener la pokeball en el aire
bool gameOpen = false;
bool gamesMenuOpen = false;
uint32_t gamesMenuUntil = 0;
// quien es ese pokemon
enum QuizPhase : uint8_t { QP_ASK, QP_REVEAL, QP_END };
bool quizOpen = false;
uint8_t gPmdForceScale = 0;  // si no es 0, escala fija para el siguiente sprite
QuizPhase quizPhase = QP_ASK;
uint32_t quizT = 0;
uint8_t quizRound = 0, quizScore = 0;
int16_t quizAnswer = 0, quizOpts[3] = {0, 0, 0}, quizTapped = 0;
PmdMon quizPmd;
// visita diaria
enum VisitPhase : uint8_t { VP_NONE, VP_ANNOUNCE, VP_ENTER, VP_HANG, VP_LEAVE };
VisitPhase visitPhase = VP_NONE;
uint32_t visitT = 0, visitCheckT = 0;
int16_t visitDex = 0;
PmdMon visitPmd;
// inclinacion
float tiltBase = 0, tiltSmooth = 0, tiltOverride = 0;
bool tiltForced = false, tiltAim = false;
#ifdef BOARD_KNOMI_V2
KnomiAccel accel;
#endif
uint32_t gameOverUntil = 0;
// pokerun
bool runOpen = false, runJump = false, runNewHi = false, runBot = false;
RunState run;
uint32_t runLast = 0, runOverT = 0, runFrames = 0;
uint8_t runScale = 2;
float ballX, ballY, ballVX, ballVY, gamePetX, gamePetVX = 0;
uint32_t gameStartT = 0;
uint32_t lastGameStep = 0;  // ultima llamada a stepGame(): fisica por tiempo real, no por frame
uint8_t gameScore, gameMisses;
float hitX, hitY;             // ultimo golpe (anillo de impacto)
uint32_t hitTime = 0;
bool gameNewHi = false;

// combate contra un rival salvaje (entrena la fuerza y llena la pokedex)
enum BattlePhase : uint8_t { BP_TIRED, BP_INTRO, BP_LEARNED, BP_CHOOSE, BP_ATTACK, BP_HIT, BP_EFFECT, BP_RAN, BP_END,
                            BP_AIM, BP_THROW, BP_MISS, BP_CAUGHT, BP_FLED };
bool battleOpen = false;
BattlePhase bPhase = BP_INTRO;
uint32_t bPhaseT = 0;
BtlFighter bMe, bFoe;
PmdMon wildPmd;
float bShownMe = 0, bShownFoe = 0;  // vida en pantalla: baja poco a poco hasta la real
struct { bool mine; uint8_t move; } bTurn[2];
uint8_t bStep = 0, bLastEff = 2;
uint8_t bGain = 0;
bool bWon = false;
BtlMoves bMeMoves, bFoeMoves;
uint8_t bLearned = 0;  // el ataque que anuncia BP_LEARNED
float bBallX = 233, bFoeX = 233;  // captura: posicion del tiro y del rival
uint8_t bBalls = 0;
uint32_t bBallT = 0;

// las 9 especies con sprite propio en flash (respaldo sin SD): dex -> indice
int flashIdxForDex(int16_t dex) {
  static const int8_t IDX[10] = { -1, 3, 4, 5, 0, 1, 2, 6, 7, 8 };
  return (dex >= 1 && dex <= 9) ? IDX[dex] : -1;
}

#define CX 233  // centro de la pantalla redonda
#define CY 233
#define PET_CY 202  // centro vertical del sprite

static const uint16_t INK_K = 0x18C4;  // spriteColor('k')

// botones de icono siguiendo el arco inferior de la pantalla redonda
// (los exteriores van mas altos para no salirse del circulo)
struct Btn {
  int16_t cx, cy;
  const char *const *icon;
};
Btn buttons[4] = {
  { 140, 390, SPR_ICON_FOOD },   // comer
  { 202, 404, SPR_ICON_PLAY },   // jugar
  { 264, 404, SPR_ICON_LIGHT },  // luz
  { 326, 390, SPR_ICON_CLEAN },  // bano
};
#define BTN_HALF 26  // boton de 52x52
#define BTN_HIT 36   // radio tactil (un poco mas generoso)

// grietas del huevo (pixeles 'k' sobre el sprite)
static const uint8_t CRACK1[][2] = { {15,8},{16,9},{15,10} };
static const uint8_t CRACK2[][2] = { {11,13},{12,14},{11,15},{20,12},{19,13},{20,14} };
// estrellas del modo noche
static const uint16_t STARS[][2] = { {120,140},{330,120},{370,210},{95,230},{280,90},{160,95} };

bool wasPressed = false;
// eleccion de inicial (primera partida): Bulbasaur / Charmander / Squirtle, 3 filas
static const int16_t STARTER_DEX[3] = { 1, 4, 7 };
#define STARTER_ROW_Y 110
#define STARTER_ROW_H 70
#define STARTER_ROW_GAP 8
// boton-CTA de evolucion (centrado, mitad de pantalla)
#define EVO_BTN_W 256
#define EVO_BTN_H 64
#define EVO_BTN_X (CX - EVO_BTN_W / 2)
#define EVO_BTN_Y 172
// boton-CTA de despedida (mas ancho: lleva el nombre + frase)
#define FAR_BTN_W 408
#define FAR_BTN_H 58
#define FAR_BTN_X (CX - FAR_BTN_W / 2)
#define FAR_BTN_Y 176
// el CST9217 avisa por el pin INT cuando hay datos tactiles; lo usamos para no
// leer el bus I2C mientras el chip esta dormido (esa lectura se colgaba ~1s)
volatile bool gTouchIrq = false;
void IRAM_ATTR touchIsr() { gTouchIrq = true; }
uint32_t lastRender = 0;
uint32_t sdEpoch = 0;  // +1 cada vez que se recarga la SD (las miniaturas cambian)
// proteccion del AMOLED: atenuado por inactividad
uint32_t lastInteract = 0;
uint8_t dimStage = 0;        // 0 despierto, 1 atenuado (90s), 2 casi apagado (5min)
bool swallowGesture = false; // el toque que despierta no acciona nada
uint32_t holdStart = 0;     // pulsacion larga sobre el bicho
uint32_t confirmUntil = 0;  // dialogo "soltar?" activo hasta este millis
#ifdef BOARD_KNOMI_V2
uint32_t gBtnSimUntil = 0;  // pulsacion simulada del boton fisico (consola serie)
#endif
uint8_t choiceKind = 0;     // dialogo de decision: 0 ninguno, 1 evolucion, 2 despedida, 3 borrar partida
uint32_t choiceUntil = 0;   // se cierra solo a este millis
int16_t tX0, tY0, tXl, tYl; // gesto en curso (inicio y ultima posicion)
uint32_t tStart = 0;
bool holdFired = false;

void setup() {
  Serial.setRxBufferSize(8192);  // la transferencia a SD llega en bloques de 2 KB
  Serial.begin(115200);
  // CRITICO: sin esto, Serial.print BLOQUEA el juego cuando no hay un
  // monitor serie abierto en el host (el bufer TX del USB CDC se llena
  // y nadie lo vacia) -> con timeout 0 los mensajes se descartan
#ifndef BOARD_KNOMI_V2  // el Knomi saca el serie por un CH340 (UART0): nunca bloquea
  Serial.setTxTimeoutMs(0);
#endif
  Serial.printf("TamaPoke fw v%s\n", FW_VERSION);
  loadLang();  // idioma guardado (ES por defecto)
  Wire.begin(IIC_SDA, IIC_SCL);
  // CST9217 (tactil), AXP2101 (PMU) y PCF85063 (RTC) comparten este bus I2C.
  // Red de seguridad para PMU/RTC (SensorLib NO respeta este timeout en el
  // tactil; el cuelgue del tactil dormido se resuelve gateando por INT, ver
  // handleTouch).
  Wire.setTimeOut(50);

  // CRITICO: encender la alimentacion del panel (BLDO1=OLED VDD 3.3V) ANTES de
  // inicializar el display. Si el PMU se reseteo (drenaje total), este rail
  // queda OFF y la pantalla se ve negra aunque el resto de la placa funcione.
  pmuEnablePanel();

  // QSPI a 80MHz (por defecto 40): el flush del framebuffer es el cuello de
  // botella del fps (~56ms a 40MHz). Si el panel mostrara basura, bajar a 40M.
  if (!gfx->begin(80000000)) Serial.println("gfx->begin() fallo");
  panel->setBrightness(180);
  applyLangFont();  // fuente del idioma guardado (clasica salvo CJK)

  touch.setPins(TP_RESET, TP_INT);
  bool touchOk = false;
  for (int i = 0; i < 3 && !touchOk; i++) {  // a veces falla al primer intento
    touchOk = touch.begin(Wire, TOUCH_ADDR, IIC_SDA, IIC_SCL);
    if (!touchOk) delay(150);
  }
  if (!touchOk) Serial.println("CST9217 no detectado");
  // begin() deja el chip en modo comando (lee la identidad y no sale);
  // hace falta un reset por hardware para que vuelva a reportar toques
  touch.reset();
  touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
  touch.setMirrorXY(true, true);  // el panel esta montado girado 180 grados
  // INT activo-bajo: salta cuando hay datos. Gatea las lecturas I2C (ver loop)
  pinMode(TP_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TP_INT), touchIsr, FALLING);

#ifdef BOARD_KNOMI_V2
  pinMode(KNOMI_BTN_PIN, INPUT_PULLUP);
  if (!accel.begin(Wire)) Serial.println("LIS2DW12 no detectado");
#endif
  pet.begin();
  sdBegin();
  thumbs.load();

  // reloj real: aplica el tiempo que estuvo apagado
  rtcBegin();
  batBegin();
  pwrSetup();
  uint32_t e = rtcEpoch();
  if (e == 0) {
    // Sin pila de respaldo, el PCF85063 pierde la hora al cortar la
    // alimentacion. Sembrar siempre la fecha fija haria RETROCEDER el tiempo de
    // juego respecto a lo ya guardado, y eso rompe la racha, la edad y la hora
    // de la escena. Se siembra con la ultima hora vista, si es posterior, para
    // que el tiempo no vaya nunca hacia atras. Los minutos apagado se pierden
    // igual (sin RTC no hay forma de saberlos), pero nada se descuadra.
    uint32_t seed = 1767225600UL;             // RTC virgen de verdad
    uint32_t seen = pet.savedEpoch();         // usa la NVS que pet ya tiene abierta
    if (seen > seed) seed = seen;
    rtcSetEpoch(seed);
    e = rtcEpoch();
    Serial.printf("RTC sin hora: sembrado en %u%s\n", seed,
                  seen > 1767225600UL ? " (desde la ultima hora guardada)" : "");
  }
  pet.syncClock(e);

  audioBegin();  // ES8311 + I2S + amplificador (suena un jingle de arranque)

  lastInteract = millis();
}

// carga/descarga el sprite de SD cuando cambia la especie
void ensureMon() {
  if (pet.speciesId == monFor && monShinyFor == pet.shiny && !sdDirty) return;
  // los ficheros recien recibidos pueden incluir un thumbs.bin nuevo, que se
  // cargaba solo en setup(): sin esto no se veia en la galeria hasta reiniciar
  if (sdDirty) { thumbs.unload(); thumbs.load(); sdEpoch++; }
  sdDirty = false;
  monFor = pet.speciesId;
  monShinyFor = pet.shiny;
  mon.unload();
  pmd.unload();
  beh.x = beh.targetX = 233;
  beh.mode = 0;
  beh.until = 0;
  if (pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT) {
    pmd.load(pet.speciesId, pet.shiny);          // principal: PMD
    if (!pmd.loaded) mon.load(pet.speciesId, pet.shiny);  // respaldo: B/N
  }
}

void loop() {
  uint32_t now = millis();
  pet.update(now);

  // el amplificador sigue al estado de sueno (la llamada sale sola si no cambia).
  // Aqui cubre todas las vias: el boton de luz, el sueno nocturno y el que llega
  // aplicado desde la progresion offline.
  audioSetSleeping(pet.sleeping);

  // avisa con un sonido cuando el bicho pasa a estar listo para evolucionar
  // (incluye el caso de cumplir al despertar). canEvolveNow es false durmiendo.
  static bool wasEvoReady = false;
  bool evoReady = pet.wantEvolveButton();
  if (evoReady && !wasEvoReady) sfxPlay(SFX_MEDAL);
  wasEvoReady = evoReady;
  // aviso sombrio cuando el bicho esta a punto de escaparse por abandono
  static bool wasRunReady = false;
  bool runReady = pet.canRunawayNow();
  if (runReady && !wasRunReady) sfxPlay(SFX_DENY);
  wasRunReady = runReady;

  handleTouch();
  visitTick(now);
  handleSerial();
  ensureMon();

  // pulsacion corta del PWR: pantalla on/off
  static uint32_t lastPwr = 0;
  if (now - lastPwr > 250) {
    lastPwr = now;
    if (pwrShortPressed()) {
      screenOff = !screenOff;
      if (!screenOff) wakeUI();
    }
  }
#ifdef BOARD_KNOMI_V2
  handleKnomiButton(now);
  wifiSetupPoll();
  if (wifiSetupState() != WS_OFF) lastInteract = now;  // que no se apague mientras se configura
#endif

  updateBrightness(now);

  // bateria al limite seguro (sin USB): se guarda la partida y se apaga el PMU antes de que la
  // celda baje mas; el RTC conserva la hora
  if (batShouldShutdown() && !usbPresent()) {
    pet.flushSave();
    panel->setBrightness(0);
    batPowerOff();
  }

  // vuelca el autoguardado periodico SOLO con la pantalla atenuada/apagada o
  // durmiendo: la escritura a NVS congela ~1s ambos cores (caché de flash off),
  // y aqui no hay animacion que se corte ni dedo esperando respuesta. Con 90s
  // de inactividad la pantalla ya atenua, asi que se vuelca enseguida; el uso
  // activo persiste igual por los guardados de cada accion (comer/jugar/...).
  if (pet.savePending() && (screenOff || dimStage >= 1 || pet.sleeping)) {
    pet.flushSave();
  }

#ifdef BOARD_KNOMI_V2
  if (clockPoll()) {  // aplica el tiempo apagado en cuanto hay hora de red
    if (clockWasSeed()) pet.setClock(rtcEpoch());
    else pet.syncClock(rtcEpoch());
    if (clockOpen && !clockEdited) clockLoadNow();  // la pantalla de hora abierta tambien se entera
  }
#endif

  // anota la hora real cada 30 s (se persiste en cada save del juego)
  static uint32_t lastClock = 0;
  if (now - lastClock > 30000) {
    lastClock = now;
    uint32_t e = rtcEpoch();
    if (e) pet.lastSeenEpoch = e;
  }

  // latido de salud cada 5 min (para el soak test; se descarta si no hay monitor)
  static uint32_t lastHealth = 0;
  if (now - lastHealth > 300000) {
    lastHealth = now;
    Serial.printf("HEALTH up=%lus heap=%u min=%u bat=%d%% mv=%d chg=%d usb=%d dim=%u off=%d\n",
                  (unsigned long)(now / 1000), ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                  batPercent(), batMillivolts(), batCharging() ? 1 : 0,
                  usbPresent() ? 1 : 0, dimStage, screenOff ? 1 : 0);
  }

  // Ritmo de dibujo (power.h): 10 fps con el bicho en uso, menos cuanto mas rato sin tocar,
  // 85 ms en juego/saco (margen seguro para que el redibujado no pise el envio DMA del frame
  // anterior: a 40-65 ms solapaba y causaba flashes negros).
  // Con la pantalla apagada no se dibuja: a brillo 0 no se ve nada, pero el redibujado y el
  // flush DMA del framebuffer del lienzo seguian corriendo a 10 fps, y medido son 70 ms de
  // trabajo por cada 100 (46 de ellos solo el volcado). Cada render repinta la escena
  // entera, asi que no queda nada a medias; y al quedarse lastRender congelado, el primer
  // frame tras despertar sale en el acto.
  PowerCtx pc = powerCtx(millis());
  audioSetIdle(!amplifierWanted(pc));  // el amplificador consume en reposo
  batSetPollMs(powerCacheMs(pc));
  uint32_t interval = frameIntervalMs(pc);
  bool drewFrame = false;
  if (interval && now - lastRender >= interval) {
    lastRender = now;
    drewFrame = render();
  }
  // cede la CPU en los pases que no dibujan: sin esto el loop gira a pleno rendimiento
  // entre frames; 1 ms no se nota en el tactil (se lee cada 20 ms) y el frame no se retrasa
  if (!drewFrame) idleWait(pc);
}

// estado de consumo actual (ver power.h)
PowerCtx powerCtx(uint32_t now) {
  PowerCtx c;
  c.idleMs = idleSince(now, lastInteract);
  c.screenOff = screenOff;
  c.sleeping = pet.sleeping;
  c.gameOpen = gameOpen || battleOpen || quizOpen || runOpen;
  c.arcadeOpen = runOpen;
  c.usb = usbPresent();
  c.charging = batCharging();
  c.batPct = batPercent();
  return c;
}

// el usuario ha interactuado: vuelve el ritmo normal de dibujo (sin esperar al proximo
// frame de un ritmo lento) y el amplificador se enciende antes de que suene nada
void wakeUI() {
  uint32_t now = millis();
  uint32_t iv = frameIntervalMs(powerCtx(now));
  bool slow = !iv || iv > 100;  // pantalla apagada o ritmo lento: se dibuja ya, sin esperar al frame
  lastInteract = now;
  // el frame forzado respeta los 85 ms entre redibujados (si no, solapa el DMA del anterior)
  if (slow) lastRender = (now - lastRender >= 85) ? 0 : lastRender - 15;
  audioSetIdle(false);
}

// brillo segun sueno + inactividad (proteccion del AMOLED) y bateria
void updateBrightness(uint32_t now) {
  // los eventos visibles despiertan la pantalla solos
  if (!screenOff && (pet.evolving() || pet.ceremony || pet.eating() || pet.showHeart())) {
    if (dimStageFor(idleSince(now, lastInteract)) != DIM_AWAKE) wakeUI();
    else lastInteract = now;
  }
  PowerCtx c = powerCtx(now);
  dimStage = dimStageFor(c.idleMs);
  uint8_t target = brightnessFor(c);
  static uint8_t current = 255;
  if (target != current) {
    current = target;
    panel->setBrightness(target);
  }
}

// Pausa de un pase del bucle que no ha dibujado. Con la pantalla apagada, o encendida pero casi
// apagada (5 min sin tocar), y sin USB se duerme la CPU (light sleep) hasta el proximo frame o
// 200 ms: despierta por el temporizador o por un toque (el INT del tactil baja). Con USB no, para
// no perder el puerto serie de la tarjeta SD: se mira sin cache, por si se acaba de enchufar.
void idleWait(PowerCtx c) {
  // no se duerme justo tras dibujar (el volcado DMA del frame sigue en marcha) ni con un aviso sonando
  if (lightSleepAllowed(c) && millis() - lastRender >= 100 && !audioBusy()) {
    c.usb = usbPresentNow();
    if (!lightSleepAllowed(c)) { delay(idleDelayMs(c)); return; }
    if (digitalRead(TP_INT) == LOW) { gTouchIrq = true; delay(1); return; }  // toque pendiente: no se duerme
    uint32_t ms = POWER_SLEEP_MS;
    uint32_t iv = frameIntervalMs(c);
    if (iv) {  // pantalla encendida: hasta el proximo frame
      uint32_t due = lastRender + iv - millis();
      if ((int32_t)due < (int32_t)ms) ms = (int32_t)due > 1 ? due : 1;
    }
    esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000);
    gpio_wakeup_enable((gpio_num_t)TP_INT, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
    // la interrupcion del tactil vuelve a ser por flanco (como la dejo attachInterrupt); un toque
    // que ya haya bajado la linea se registra a mano
    gpio_wakeup_disable((gpio_num_t)TP_INT);
    gpio_set_intr_type((gpio_num_t)TP_INT, GPIO_INTR_NEGEDGE);
    if (digitalRead(TP_INT) == LOW) gTouchIrq = true;
    return;
  }
  delay(idleDelayMs(c));
}

// ---------- consola serie (provision de SD + depuracion) ----------

void openClock();  // prototipo

void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  if (sdSerialCommand(line)) return;

#ifdef BOARD_KNOMI_V2
  if (line == "SHOT") {  // captura del panel: "SHOT 240 240\n" + RGB565 little endian
    Serial.printf("SHOT %d %d\n", PANEL_SIZE, PANEL_SIZE);
    Serial.write((uint8_t *)gfx->panelBuffer(), PANEL_SIZE * PANEL_SIZE * 2);
    Serial.println("DONE");
    return;
  } else if (line.startsWith("TAP ")) {  // toque simulado en coordenadas logicas (466): "TAP x y [ms]"
    int a = line.indexOf(' '), b = line.indexOf(' ', a + 1), c = line.indexOf(' ', b + 1);
    touch.inject(line.substring(a + 1, b).toInt(), line.substring(b + 1, c).toInt(), c > 0 ? line.substring(c + 1).toInt() : 120);
    gTouchIrq = true;
    Serial.println("DONE");
    return;
  } else if (line.startsWith("SWIPE ")) {  // gesto simulado: "SWIPE x0 y0 x1 y1"
    int x0, y0, x1, y1;
    if (sscanf(line.c_str(), "SWIPE %d %d %d %d", &x0, &y0, &x1, &y1) == 4) {
      touch.swipe(x0, y0, x1, y1, 240);
      gTouchIrq = true;
    }
    Serial.println("DONE");
    return;
  } else if (line == "WIFISETUP") {  // abre la configuracion sin pasar por gestos
    openClock();
    clockPage = 1;
    wifiSetupStart();
    Serial.println("DONE");
    return;
  } else if (line == "WIFIEND") {
    wifiSetupStop();
    clockOpen = false;
    Serial.println("DONE");
    return;
  } else if (line == "ACC") {  // gravedad por eje, para elegir eje y signo de la inclinacion
    float g[3] = {0, 0, 0};
    bool ok = accel.read(g);
    Serial.printf("acc ok=%d x=%.2f y=%.2f z=%.2f\n", ok, g[0], g[1], g[2]);
    Serial.println("DONE");
    return;
  } else if (line == "WIFIINFO") {
    Serial.printf("wifi: red '%s' estado=%u setup=%u\n", clockWifiSsid(), clockWifiStatus(), wifiSetupState());
    Serial.println("DONE");
    return;
  } else if (line.startsWith("WIFITRY ")) {  // prueba del camino de fallo, con clave inventada
    wifiSetupTry(line.substring(8).c_str(), "not-the-password");
    Serial.println("DONE");
    return;
  } else if (line.startsWith("BTN ")) {  // pulsacion simulada del boton fisico: "BTN ms"
    gBtnSimUntil = millis() + line.substring(4).toInt();
    Serial.println("DONE");
    return;
  } else if (line == "PERF") {  // coste del volcado al panel
    uint32_t t0 = micros();
    for (int i = 0; i < 10; i++) gfx->flush();
    Serial.printf("flush=%lu us\n", (unsigned long)((micros() - t0) / 10));
    Serial.println("DONE");
    return;
  } else if (line == "I2C") {
    for (uint8_t a = 1; a < 127; a++) {
      Wire.beginTransmission(a);
      if (Wire.endTransmission() == 0) Serial.printf("i2c 0x%02X\n", a);
    }
    Serial.println("DONE");
    return;
  }
#endif
  if (line == "NOSAVE") {  // pruebas en la placa: nada llega a la NVS hasta reiniciar
    pet.noSave = true;
    Serial.println("nosave=1");
    Serial.println("DONE");
  } else if (line == "AWAKE" && pet.noSave) {  // solo en modo de prueba
    pet.sleeping = false;
    pet.energy = 100;
    Serial.println("DONE");
  } else if (line == "BTLWIN" && pet.noSave && battleOpen) {
    bFoe.hp = 0;
    battleFinish(true);
    Serial.println("DONE");
  } else if (line.startsWith("TILT ")) {  // inclinacion simulada, -1..1; "aim" apunta solo; "off" vuelve al sensor
    String v = line.substring(5);
    tiltForced = v != "off";
    tiltAim = v == "aim";
    tiltOverride = v.toFloat();
    Serial.println("DONE");
  } else if (line == "RUN") {
    startRun();
    Serial.println("DONE");
  } else if (line == "RUNBOT" && pet.noSave) {  // juega solo, para ver velocidades altas
    runBot = !runBot;
    Serial.println("DONE");
  } else if (line == "GAMES") {
    openGamesMenu();
    Serial.println("DONE");
  } else if (line.startsWith("VISIT")) {
    startVisitWith(line.length() > 6 ? line.substring(6).toInt() : 0);
    Serial.println("DONE");
  } else if (line == "BATTLE") {
    startBattle();
    Serial.println("DONE");
  } else if (line.startsWith("WILD ")) {  // rival concreto (pruebas)
    startBattleWith(line.substring(5).toInt());
    Serial.println("DONE");
  } else if (line == "HATCH") {
    pet.eggTap(); pet.eggTap(); pet.eggTap();
    Serial.println("DONE");
  } else if (line.startsWith("SPEC ")) {
    int n = line.substring(5).toInt();
    if (n >= 1 && n <= DEX_COUNT) {
      pet.prevSpeciesId = pet.speciesId;
      pet.speciesId = n;
      Serial.printf("especie #%d %s\n", n, DEX_TBL[n].name);
    }
    Serial.println("DONE");
  } else if (line.startsWith("LVL ")) {
    pet.ageMinutes = (uint32_t)line.substring(4).toInt() * MINUTES_PER_LEVEL;
    Serial.println("DONE");
  } else if (line.startsWith("TIME ")) {
    uint32_t e = (uint32_t)line.substring(5).toInt();
    rtcSetEpoch(e);
    pet.setClock(e);
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line.startsWith("RTCSET ")) {  // solo RTC (simular apagados en pruebas)
    rtcSetEpoch((uint32_t)line.substring(7).toInt());
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line == "TIME") {
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line == "GAL") {
    galleryOpen = !galleryOpen;
    galleryDetail = 0;
    galleryDirty = true;
    if (!galleryOpen) galleryPmd.unload();
    Serial.println("DONE");
  } else if (line == "EGGS") {
    // simula 20 tiradas de huevo (no cambia el estado del juego)
    for (int i = 0; i < 20; i++) {
      int16_t d = pet.pickEggSpecies();
      Serial.printf("%d:%s(r%u) ", d, DEX_TBL[d].name, DEX_TBL[d].rarity);
    }
    Serial.println();
    Serial.println("DONE");
  } else if (line == "SHINY") {  // alterna shiny del actual (pruebas)
    pet.shiny = !pet.shiny;
    Serial.printf("shiny=%d\n", pet.shiny);
    Serial.println("DONE");
  } else if (line.startsWith("NICK ")) {
    pet.rename(line.substring(5).c_str());
    Serial.printf("nick=%s\n", pet.nick);
    Serial.println("DONE");
  } else if (line == "CAREDAY") {  // simula un dia nuevo cuidado (pruebas)
    pet.setClock(pet.lastSeenEpoch + 86400);
    pet.caress();
    Serial.printf("streak=%u bond=%u medals=0x%X\n", pet.streak, pet.bond, pet.medals);
    Serial.println("DONE");
  } else if (line == "BYE") {
    pet.startFarewell();
    Serial.println("DONE");
  } else if (line == "RUN") {
    pet.startRunaway();
    Serial.println("DONE");
  } else if (line == "BEEP") {
    sfxPlay(SFX_HATCH);  // prueba de audio
    Serial.println("DONE");
  } else if (line == "ABANDON") {
    pet.dbgRunawayReady();  // fuerza el estado "lista para escaparse" (test del boton)
    Serial.println("DONE");
  } else if (line == "WIPE") {
    pet.factoryReset();     // borra NVS y reinicia -> partida nueva (eleccion de inicial)
    Serial.println("DONE");
    delay(100);
    ESP.restart();
  } else if (line == "REG") {
    Serial.printf("pokedex %u/151:", pet.registeredCount());
    for (int i = 1; i <= 151; i++)
      if (pet.isRegistered(i)) Serial.printf(" %d", i);
    Serial.println();
    Serial.println("DONE");
  } else if (line == "HEALTH") {
    Serial.printf("up=%lus heap=%u min=%u sd=%d mon=%d\n",
                  (unsigned long)(millis() / 1000), ESP.getFreeHeap(),
                  ESP.getMinFreeHeap(), sdReady, pmd.loaded || mon.loaded);
    Serial.println("DONE");
  } else if (line == "STATS") {
    Serial.printf("spec=%d nv=%u com=%u fel=%u ene=%u lim=%u desc=%u sd=%d mon=%d bat=%d usb=%d rtc=%u\n",
                  pet.speciesId, pet.level(), pet.fullness, pet.joy, pet.energy,
                  pet.hygiene, pet.careMistakes, sdReady, mon.loaded,
                  batPercent(), usbPresent(), rtcEpoch());
    Serial.printf("peso=%u fue=%u def=%u vel=%u genes=%u/%u/%u tr=%u/%u/%u baya=%d\n",
                  pet.weight, pet.atkStat(), pet.defStat(), pet.speStat(),
                  pet.geneAtk, pet.geneDef, pet.geneSpe,
                  pet.trAtk, pet.trDef, pet.trSpe, pet.berryKnown);
    Serial.printf("shiny=%d streak=%u/%u bond=%u medals=0x%X(%u) nick=%s\n",
                  pet.shiny, pet.streak, pet.bestStreak, pet.bond, pet.medals,
                  pet.totalMedals, pet.nick);
    Serial.println("DONE");
  }
}

// ---------- entrada tactil ----------

bool inPetZone(int16_t x, int16_t y) {
  return x > 110 && x < 356 && y > 95 && y < 310;
}

// el toque se resuelve al LEVANTAR el dedo para distinguir tap de deslizar
void handleTouch() {
  static uint32_t lastPoll = 0;
  if (millis() - lastPoll < 20) return;  // 50 Hz le sobra a un dedo
  lastPoll = millis();
  // solo tocamos el bus si el chip aviso por INT o si el dedo sigue abajo (hay
  // que detectar el levantamiento). Leer el CST9217 dormido se colgaba ~1s y
  // congelaba el loop entero; SensorLib no respeta el timeout de Wire.
  if (!gTouchIrq && !wasPressed) return;
  gTouchIrq = false;
  // Un ciclo de solo direccion antes de leer. Sin esto, getPoint() se colgaba
  // exactamente 1000 ms (el timeout por defecto del driver I2C, que SensorLib no
  // acota) y congelaba el loop entero: es lo que se percibia como "el minijuego
  // se congela 2-3 segundos al tocar la bola" (issue #16), porque ahi los toques
  // son rapidos y seguidos.
  //
  // Medido en placa, jugando lo mismo (~470 lecturas, marcador ~50):
  //   sin esta linea: 5 parones de 1000 ms en 60 s
  //   con ella:       0
  // El contador de rechazos salio 0 en ambos casos, asi que NO funciona
  // saltandose lecturas cuando el chip no contesta: lo que hace es despertarlo,
  // para que la lectura siguiente no se encuentre el CST9217 dormido.
  Wire.beginTransmission(TOUCH_ADDR);
  if (Wire.endTransmission() != 0) return;  // no responde: se reintenta en 20 ms
  int16_t x, y;
  bool pressed = touch.getPoint(&x, &y, 1) > 0;

#ifdef BOARD_KNOMI_V2
  // clave de wifi: la tecla se ilumina bajo el dedo y se pulsa al levantarlo, para poder
  // deslizarse hasta la buena en un teclado tan pequeno
  if (clockOpen && clockPage == 1 && wifiSetupState() == WS_PASS) {
    if (pressed) {
      wakeUI();
      wifiKbX = x;
      wifiKbY = y;
      wifiKbHover = wifiKeyAt(x, y);
    } else if (wasPressed) {
      wifiKbHover = -1;
      settingsTap(wifiKbX, wifiKbY);
    }
    wasPressed = pressed;
    return;
  }
#endif

  // juegos: el toque se resuelve al levantar el dedo, para que deslizar arriba salga
  // sin pulsar nada de paso. Solo el salto de PokeRun va al apoyar: ahi cuenta cada ms.
  if (battleOpen || quizOpen || gamesMenuOpen || gameOpen || runOpen) {
    if (pressed) { tXl = x; tYl = y; }  // tXl: apuntar con el dedo donde no hay acelerometro
    if (pressed && !wasPressed) {
      tX0 = x;
      tY0 = y;
      tStart = millis();
      // solo despierta si no se veia nada: atenuada sigue siendo la partida en curso (en
      // Tilt Catch no se toca la pantalla, asi que se atenua jugando)
      swallowGesture = dimStage >= DIM_OFF || screenOff;
      screenOff = false;
      wakeUI();
      if (runOpen && !swallowGesture) runJump = true;
    } else if (!pressed && wasPressed) {
      wakeUI();
      if (!swallowGesture) {
        if (swipeUpQuit(tX0, tY0, tXl, tYl, millis() - tStart)) quitGame();
        else if (abs(tXl - tX0) < 40 && abs(tYl - tY0) < 40) {
          if (battleOpen) battleTap(tX0, tY0);
          else if (quizOpen) quizTap(tX0, tY0);
          else if (gamesMenuOpen) gamesMenuTap(tX0, tY0);
          else if (runOpen) runTap();
        }
      }
    }
    wasPressed = pressed;
    return;
  }

  if (pressed && !wasPressed) {  // empieza el gesto
    tX0 = tXl = x;
    tY0 = tYl = y;
    tStart = millis();
    holdFired = false;
    swallowGesture = (dimStage > 0) || screenOff;  // si estaba a oscuras, solo despierta
    screenOff = false;
    wakeUI();
  } else if (pressed) {  // sigue apoyado
    tXl = x;
    tYl = y;
    // pulsacion larga sin moverse sobre el bicho -> dialogo de soltar
    if (!holdFired && !swallowGesture && !galleryOpen && !cardOpen && !kbOpen && !clockOpen && millis() - tStart > 3000 &&
        abs(tXl - tX0) < 30 && abs(tYl - tY0) < 30 && inPetZone(tX0, tY0) &&
        !pet.isEgg() && !confirmUntil && !pet.ceremony) {
      confirmUntil = millis() + 10000;
      holdFired = true;
    }
  } else if (wasPressed) {  // levanta el dedo: resolver gesto
    wakeUI();
    int dx = tXl - tX0, dy = tYl - tY0;
    uint32_t dt = millis() - tStart;
    if (!holdFired && !swallowGesture) {
      if (abs(dx) > 80 && abs(dy) < 70 && dt < 800) onSwipe(dx > 0 ? 1 : -1);
      else if (abs(dy) > 80 && abs(dx) < 70 && dt < 800) onSwipeV(dy > 0 ? 1 : -1);
      else if (dt < 1500 && abs(dx) < 40 && abs(dy) < 40) onTap(tX0, tY0);
    }
  }
  wasPressed = pressed;
}

// deslizar vertical: abre/cierra la ficha del bicho
void openClock();  // prototipo

void onSwipeV(int dir) {
  if (pet.awaitingStarter() || pet.awaitingEggPick()) return;  // bloqueado mientras se elige
  if (gameOpen || galleryOpen || kbOpen || battleOpen || quizOpen || gamesMenuOpen || pet.ceremony) return;
  if (clockOpen) {
    if (wifiSetupState() == WS_PASS) return;  // un toque movido no debe tirar la clave escrita
    wifiSetupStop();
    clockOpen = false;
    return;
  }
  if (cardOpen) {
    if (dir < 0) cardOpen = false;  // arriba cierra la ficha
    return;
  }
  if (dir > 0) {                    // deslizar abajo: ajustar hora
    if (!confirmUntil && !feedMenuUntil) openClock();
  } else if (!pet.isEgg() && !confirmUntil && !feedMenuUntil) {
    cardOpen = true;                // deslizar arriba: ficha
    cardPage = 0;
  }
}

// deslizar: dir +1 = hacia la derecha
void onSwipe(int dir) {
  if (pet.awaitingEggPick()) {
    int pages = (pet.registeredCount() + 15) / 16;
    int np = eggPickPage - dir;  // deslizar a la izquierda avanza pagina
    if (!eggPickDetail && np >= 0 && np < pages) {
      eggPickPage = np;
      eggPickDirty = true;
    }
    return;
  }
  if (pet.awaitingStarter()) return;  // bloqueado durante la eleccion de inicial
#ifdef BOARD_KNOMI_V2
  if (clockOpen && wifiSetupState() == WS_OFF) {  // hora <-> ajustes; izquierda avanza
    uint8_t next = dir > 0 ? 0 : 1;
    if (next == 0 && clockPage != 0 && !clockEdited) clockLoadNow();  // ha podido sincronizarse en ajustes
    clockPage = next;
    return;
  }
#endif
  if (gameOpen || kbOpen || clockOpen) return;
  if (cardOpen) {  // dentro de la ficha: cambiar entre las 4 paginas
    int p = (int)cardPage + (dir > 0 ? -1 : 1);  // izquierda avanza
    cardPage = p < 0 ? 0 : (p > 3 ? 3 : p);
    return;
  }
  if (!galleryOpen) {
    if (!pet.ceremony && !confirmUntil) {
      galleryOpen = true;
      galleryPage = 0;
      galleryDetail = 0;
      galleryDirty = true;
    }
    return;
  }
  if (galleryDetail) {  // en detalle: volver a la rejilla
    galleryDetail = 0;
    galleryPmd.unload();
    galleryDirty = true;
    return;
  }
  int np = galleryPage - dir;  // deslizar a la izquierda avanza pagina
  if (np < 0) {                // retroceder desde la primera = salir
    galleryOpen = false;
    galleryPmd.unload();
    return;
  }
  if (np > 9) np = 9;
  if (np != galleryPage) {
    galleryPage = np;
    galleryDirty = true;
  }
}

void onTap(int16_t x, int16_t y) {
  // Serial.printf("TOUCH %d %d\n", x, y);  // diagnostico (silenciado: satura el log)
  if (pet.awaitingStarter()) {  // primera partida: elegir inicial
    for (int i = 0; i < 3; i++) {
      int ry = STARTER_ROW_Y + i * (STARTER_ROW_H + STARTER_ROW_GAP);
      if (x >= 70 && x <= 396 && y >= ry && y <= ry + STARTER_ROW_H) {
        pet.chooseStarter(STARTER_DEX[i]);
        sfxPlay(SFX_TAP);
        break;
      }
    }
    return;
  }
  if (pet.awaitingEggPick()) {
    eggPickTap(x, y);
    return;
  }
  if (galleryOpen) {
    galleryTap(x, y);
    return;
  }
  if (kbOpen) {
    keyboardTap(x, y);
    return;
  }
  if (clockOpen) {
    clockTap(x, y);
    return;
  }
  if (pet.ceremony) return;  // durante la despedida no hay botones
  if (cardOpen) {
    if (cardPage == 0 && y < 84) openKeyboard();  // tocar el nombre = renombrar
#ifndef BOARD_KNOMI_V2  // en el Knomi el combate se abre con el boton fisico
    else if (cardPage == 1 && y >= 300 && y <= 340 && x >= 96 && x <= 370) {
      cardOpen = false;            // boton de combate
      startBattle();
    }
#endif
    else {
      cardOpen = false;
    }
    return;
  }
  if (choiceKind) {          // dialogo de decision: boton accion (arriba) / mantener (abajo)
    bool b1 = (x >= 93 && x <= 373 && y >= 206 && y <= 258);  // accion
    bool b2 = (x >= 93 && x <= 373 && y >= 268 && y <= 320);  // mantener / quedaros
    if (choiceKind == 1) {                 // evolucion
      if (b1) { int16_t old = pet.speciesId; pet.evolve(); evoPmd.load(old, pet.shiny); }
      else if (b2) pet.declineEvolve();
    } else if (choiceKind == 2) {          // despedida
      if (b1) pet.startFarewell();
      else if (b2) pet.declineFarewell();
    } else if (choiceKind == 3 && b1) {    // borrar la partida (boton del Knomi)
      pet.factoryReset();
      delay(100);
      ESP.restart();
    }
    choiceKind = 0;
    return;
  }
  if (confirmUntil) {        // dialogo "soltar?": SI / NO
    if (timeLeft(confirmUntil) && x >= 118 && x <= 218 && y >= 252 && y <= 304) {
      pet.release();
    }
    confirmUntil = 0;
    return;
  }
  if (feedMenuUntil) {       // selector de comida
    if (timeLeft(feedMenuUntil) && y >= 288 && y <= 352 && x >= 101 && x <= 365) {
      int item = (x - 101) / 66;
      if (item == 3) pet.feedCandy();
      else pet.feedBerry(item);
      sfxPlay(SFX_EAT);
    }
    feedMenuUntil = 0;
    return;
  }
  if (pet.isEgg()) {
    pet.eggTap();
    sfxPlay(SFX_TAP);
    return;
  }
  // boton de evolucion: abre el dialogo evolucionar/mantener
  if (pet.wantEvolveButton() && x >= EVO_BTN_X && x <= EVO_BTN_X + EVO_BTN_W &&
      y >= EVO_BTN_Y && y <= EVO_BTN_Y + EVO_BTN_H) {
    choiceKind = 1; choiceUntil = millis() + 12000;
    return;
  }
  // botones de final (mismo recuadro): escapada directa; despedida abre dialogo
  if (x >= FAR_BTN_X && x <= FAR_BTN_X + FAR_BTN_W &&
      y >= FAR_BTN_Y && y <= FAR_BTN_Y + FAR_BTN_H) {
    if (pet.canRunawayNow()) { pet.startRunaway(); return; }
    if (pet.wantFarewellButton()) { choiceKind = 2; choiceUntil = millis() + 12000; return; }
  }
  for (int i = 0; i < 4; i++) {
    int dx = x - buttons[i].cx, dy = y - buttons[i].cy;
    if (dx * dx + dy * dy <= BTN_HIT * BTN_HIT) {
      Serial.printf("BTN %d\n", i);
      sfxPlay(SFX_TAP);
      if (i == 0) {
        if (!pet.sleeping) feedMenuUntil = millis() + 6000;
      } else if (i == 1) {
        openGamesMenu();
      } else if (i == 2) {
        pet.toggleLight();
      } else {
        startBath();
      }
      return;
    }
  }
  // tocar al bicho = caricia
  if (inPetZone(x, y)) {
    Serial.println("PET");
    pet.caress();
    if (!pet.sleeping) sfxPlay(SFX_HEART);
  }
}

// ---------- render ----------

bool gNight = false;  // noche real (por hora) o durmiendo: lo fija render()
uint16_t inkColor() { return gNight ? UI_INK_NIGHT : UI_INK; }

// ---------- escena de fondo: bioma del tipo + hora real del RTC ----------

#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define HORIZON 232  // linea donde el cielo se encuentra con el suelo

uint16_t lerp565(uint16_t a, uint16_t b, int i, int n) {
  if (n <= 0) return a;
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)((((ar + (br - ar) * i / n) << 11)) |
                    (((ag + (bg - ag) * i / n) << 5)) | (ab + (bb - ab) * i / n));
}

// hora del dia 0-23 (de la hora real cacheada cada 30s; 13 si no hay reloj)
int sceneHour() {
  uint32_t e = pet.lastSeenEpoch;
  return e ? (int)((e / 3600) % 24) : 13;
}

// suelo de cada bioma de dia (de noche se mezcla hacia el azul nocturno)
static const uint16_t BIOME_SOIL[6] = {
  C565(0x7e, 0xc0, 0x7f),  // 0 pradera
  C565(0xdc, 0xca, 0x94),  // 1 playa (arena)
  C565(0x4f, 0x8a, 0x55),  // 2 bosque
  C565(0x8a, 0x55, 0x44),  // 3 volcan
  C565(0xa8, 0x90, 0x6a),  // 4 montana
  C565(0xe6, 0xee, 0xf5),  // 5 nieve
};

void drawClouds(uint32_t now, uint16_t col) {
  for (int k = 0; k < 2; k++) {
    int cx = (int)((now / 50 + k * 250) % 560) - 40;
    int cy = 70 + k * 34;
    gfx->fillCircle(cx, cy, 16, col);
    gfx->fillCircle(cx + 18, cy + 3, 13, col);
    gfx->fillCircle(cx - 15, cy + 4, 12, col);
  }
}

void drawScene(uint8_t biome, uint32_t now, bool night) {
  int h = sceneHour();
  uint16_t top, bot;
  if (night)            { top = C565(0x0c, 0x12, 0x24); bot = C565(0x1e, 0x26, 0x46); }
  else if (h < 8)       { top = C565(0xd1, 0x6a, 0x86); bot = C565(0xf3, 0xb8, 0x7c); }  // amanecer
  else if (h < 18)      { top = C565(0x8f, 0xc8, 0xea); bot = C565(0xdc, 0xee, 0xe6); }  // dia
  else                  { top = C565(0xc7, 0x5a, 0x4a); bot = C565(0xf0, 0xae, 0x64); }  // atardecer

  // cielo en bandas
  for (int y = 0; y < HORIZON; y += 8)
    gfx->fillRect(0, y, 466, 8, lerp565(top, bot, y, HORIZON));

  // sol o luna
  if (night) {
    gfx->fillCircle(360, 78, 24, C565(0xe8, 0xee, 0xf5));
    gfx->fillCircle(370, 72, 22, lerp565(top, bot, 78, HORIZON));  // creciente
    for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
  } else if (h < 18) {
    gfx->fillCircle(360, 84, 26, h < 8 ? C565(0xff, 0xd9, 0x8a) : C565(0xff, 0xe7, 0x9f));
    drawClouds(now, C565(0xff, 0xff, 0xff));
  } else {
    gfx->fillCircle(233, HORIZON - 6, 34, C565(0xff, 0xf1, 0xc8));  // sol poniente
  }

  // mar de la playa: una franja de agua sobre la arena
  uint16_t soil = BIOME_SOIL[biome < 6 ? biome : 0];
  if (night) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  if (biome == 1) {
    uint16_t sea = night ? C565(0x1c, 0x34, 0x52) : C565(0x4f, 0x96, 0xc4);
    gfx->fillRect(0, HORIZON - 26, 466, 26, sea);
    for (int i = 0; i < 3; i++) {
      int wy = HORIZON - 22 + i * 7;
      uint16_t fc = night ? C565(0x3a, 0x58, 0x78) : C565(0xbf, 0xe6, 0xf5);
      gfx->fillRect(60 + ((now / 60 + i * 30) % 60), wy, 26, 2, fc);
      gfx->fillRect(300 - ((now / 60 + i * 20) % 60), wy, 26, 2, fc);
    }
  }

  // suelo
  gfx->fillRect(0, HORIZON, 466, 466 - HORIZON, soil);
  uint16_t hill = lerp565(soil, night ? C565(0x0c, 0x12, 0x24) : C565(0xff, 0xff, 0xff), 3, 16);
  gfx->fillRoundRect(-60, HORIZON - 14, 586, 60, 30, hill);

  // detalles del bioma
  uint16_t dk = lerp565(soil, C565(0x10, 0x18, 0x20), night ? 11 : 7, 16);
  if (biome == 2) {  // bosque: coniferas en silueta
    for (int tx : { 60, 150, 360, 416 }) {
      gfx->fillTriangle(tx, HORIZON - 46, tx - 16, HORIZON, tx + 16, HORIZON, dk);
      gfx->fillTriangle(tx, HORIZON - 60, tx - 12, HORIZON - 28, tx + 12, HORIZON - 28, dk);
    }
  } else if (biome == 3) {  // volcan: rocas y brasas
    gfx->fillTriangle(70, HORIZON, 40, HORIZON + 30, 100, HORIZON + 30, dk);
    gfx->fillTriangle(400, HORIZON + 4, 372, HORIZON + 30, 430, HORIZON + 30, dk);
    if (!night)
      for (int e = 0; e < 4; e++)
        gfx->fillRect(120 + e * 70, HORIZON + 8 + (e % 2) * 6, 4, 4, C565(0xff, 0x9b, 0x3a));
  } else if (biome == 4) {  // montana: cumbres al fondo
    gfx->fillTriangle(140, HORIZON - 50, 60, HORIZON, 220, HORIZON, dk);
    gfx->fillTriangle(330, HORIZON - 38, 250, HORIZON, 410, HORIZON, dk);
  } else if (biome == 5 && !night) {  // nieve: copos cayendo
    for (int f = 0; f < 10; f++) {
      int fx = (f * 53 + now / 40) % 466;
      int fy = (f * 90 + now / 18) % HORIZON;
      gfx->fillRect(fx, fy, 3, 3, UI_WHITE);
    }
  } else if (biome == 0) {  // pradera: matas de hierba
    for (int gx : { 80, 175, 300, 395 })
      for (int b = -1; b <= 1; b++)
        gfx->fillRect(gx + b * 5, HORIZON + 6, 2, 8 + (b == 0 ? 4 : 0), dk);
  }
}

// primera partida: elige inicial entre Bulbasaur / Charmander / Squirtle
void renderStarterSelect() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  const char *t = T(S_CHOOSE_STARTER);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(t, 2), 68);
  printT(t);
  for (int i = 0; i < 3; i++) {
    int16_t d = STARTER_DEX[i];
    const DexEntry &de = DEX_TBL[d];
    int ry = STARTER_ROW_Y + i * (STARTER_ROW_H + STARTER_ROW_GAP);
    gfx->fillRoundRect(70, ry, 326, STARTER_ROW_H, 14, lerp565(de.accent, UI_WHITE, 6, 8));
    gfx->drawRoundRect(70, ry, 326, STARTER_ROW_H, 14, de.accent);
    const uint8_t *th = thumbs.get(d);     // miniatura del inicial (si la SD esta lista)
    if (th) drawThumb(th, 76, ry - 5, 3, false);
    gfx->setTextColor(UI_INK);
    setSize(3);
    setCur(178, ry + 24);
    printT(dexName(d));
  }
  gfx->flush();
}

// ---------- texto: tamano y cursor (preparado para fuentes CJK) ----------
// La fuente clasica ancla el cursor en la ESQUINA SUPERIOR del texto, y las 74
// coordenadas Y del codigo estan escritas asi. Las fuentes U8g2 lo anclan en la
// LINEA BASE (drawChar hace curY = y - baseline*size), asi que al cambiar de
// fuente todo el texto subiria. setCur() compensa esa diferencia en un solo
// sitio, para que la Y siga significando "arriba" con cualquier fuente.
//
// setSize() existe solo porque la libreria tiene textsize_x protegido y setCur()
// necesita saber la escala activa para multiplicar el ascenso.
//
// Con la fuente clasica gFontAscent es 0, asi que setCur(x,y) es exactamente
// setCur(x,y): esto no mueve un pixel en los seis idiomas actuales.
uint8_t gTextSize = 1;
int gFontAscent = 0;  // px del borde superior a la linea base, 0 = fuente clasica

// --- ajuste de la fuente CJK (los dos valores van juntos) ---
// La fuente clasica mide 8 px de alto a escala 1. Si la CJK mide mas, hay que
// dividir la escala para que los tamanos cuadren... pero dividir ADELGAZA el
// trazo: a escala 1 la unifont pinta lineas de 1 px mientras el latino a escala
// 2 las pinta de 2, y el japones se ve tenue aunque mida igual.
//
// Por eso conviene una fuente base pequena Y EN NEGRITA (_b_), que permita usar
// la MISMA escala que el latino: asi el grosor coincide y no hay que dividir.
//
// Se probo una fuente realmente negrita (b10_b) y NO sirve a este tamano: los
// trazos se juntan y los kana pierden detalle, ilegibles incluso para un lector
// nativo. Se vuelve a la unifont, que es la mas legible, y el trazo fino se
// compensa en printT() repintando 1 px desplazado (pseudo-negrita).
// japanese3 y no japanese1: los subconjuntos 1 y 2 traen kana y kanji pero
// NINGUNA puntuacion CJK, y Arduino_GFX no dibuja nada cuando le falta el
// glifo (ni avanza el cursor), asi que los 25 signos ！ y ？ de las cadenas
// desaparecian. Es la misma familia unifont: cabecera de metricas identica y
// superconjunto estricto, comprobado glifo a glifo sobre los 226 codepoints
// que usa el firmware. Cuesta 102 KB mas de flash y no mueve un pixel.
// El coreano necesita una fuente con hangul, y CJK_FONT era un solo #define.
// En vez de dar por hecho que un subconjunto sirve para las dos escrituras, la
// fuente se elige por idioma. korean2 y no korean1: el coreano de aqui usa 296
// silabas distintas y todas estan en KS X 1001, el conjunto de 2350 que es el
// tamano medido para korean2; korean1 trae 478. La comprobacion mira la
// pertenencia a KS X 1001, no recorre la tabla de la fuente, asi que se apoya
// en que el conjunto de korean2 sea ese; el ASCII se da por presente.
// Sin comprobar en placa: si CJK_SIZE_DIV 2 le sienta a korean2 igual que a la
// japonesa. El ascenso si se mide en tiempo de ejecucion en applyLangFont().
#define CJK_FONT_JA u8g2_font_unifont_t_japanese3
#define CJK_FONT_KO u8g2_font_unifont_t_korean2
#define CJK_SIZE_DIV 2

void setSize(uint8_t n) {
  // gTextSize guarda la escala REALMENTE aplicada, no la pedida: setCur()
  // multiplica el ascenso por ella y tiene que cuadrar con lo que se pinta.
  gTextSize = gCjkFont ? (n >= CJK_SIZE_DIV ? n / CJK_SIZE_DIV : 1) : n;
  gfx->setTextSize(gTextSize);
}

void setCur(int x, int y) {
  gfx->setCursor(x, y + gFontAscent * gTextSize);
}

// Fija la fuente del idioma activo. Es el UNICO sitio que la toca: el resto del
// codigo dibuja igual para todos los idiomas gracias a textW()/setCur().
//
// El ascenso se mide en vez de codificarlo: getTextBounds() devuelve y1 como
// desplazamiento del borde superior respecto al cursor, negativo con las fuentes
// U8g2 (que anclan en la linea base). Asi setCur() puede seguir tratando la Y
// como "arriba" sea cual sea la fuente, sin constantes magicas por fuente.
void applyLangFont() {
  gCjkFont = LANG_IS_CJK(gLang);
  if (!gCjkFont) {
    gfx->setFont();            // 5x7 clasica, CP437: la Y ya es el borde superior
    gfx->setUTF8Print(false);
    gFontAscent = 0;
    return;
  }
  gfx->setFont((gLang == LANG_KO) ? CJK_FONT_KO : CJK_FONT_JA);
  gfx->setUTF8Print(true);     // las cadenas CJK son UTF-8 multibyte
  int16_t x1, y1;
  uint16_t w, h;
  uint8_t antes = gTextSize;
  gfx->setTextSize(1);
  gfx->getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
  gFontAscent = -y1;           // y1 negativo: subir desde la linea base
  gfx->setTextSize(antes);
}

// ---------- medida de texto (preparado para fuentes CJK) ----------
// La fuente clasica de Arduino_GFX avanza 6 px por caracter a tamano 1, y la UI
// centraba con strlen(s)*6*n a pelo. Eso solo vale para un byte por caracter y
// ancho fijo: con una fuente U8g2 para japones/coreano/chino, ni strlen() cuenta
// caracteres (UTF-8 es multibyte) ni todos miden lo mismo. Centralizarlo aqui es
// lo que permitira anadir esos idiomas sin tocar cada punto de dibujado.
//
// Para la fuente clasica devuelve EXACTAMENTE la misma cuenta que habia antes,
// asi que este cambio no mueve un pixel en los seis idiomas actuales.
uint16_t textW(const char *s, uint8_t size) {
  if (!gCjkFont) return (uint16_t)strlen(s) * 6 * size;
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return w;
}

// x del cursor para dejar el texto centrado en CX
int centerX(const char *s, uint8_t size) { return CX - textW(s, size) / 2; }

// Imprime respetando la fuente activa. Con la CJK repinta el texto desplazado
// 1 px: la unifont a escala reducida deja trazos de 1 px y se ve tenue, y una
// fuente de verdad negrita a este tamano junta los trazos y estropea los kana.
// Repintar engorda el trazo sin deformar el glifo. Probado en placa por
// usakomint, que comparo las dos opciones en japones.
// El teclado de apodos es solo alfabeto (se deja asi a proposito), de modo que
// aqui nunca hay CJK y no hace falta engordar nada.
void printT(char c) { gfx->print(c); }

void printT(const char *s) {
  if (gCjkFont) {
    int16_t x = gfx->getCursorX(), y = gfx->getCursorY();
    gfx->print(s);
    gfx->setCursor(x + 1, y);
  }
  gfx->print(s);
}

// Pantallas que solo cambian con un toque (o con un dato que cambia muy de vez en cuando):
// mientras su "clave" no cambie no se repintan ni se vuelcan por QSPI.
static bool staticScreen() {
  if (pet.awaitingStarter()) return true;
  if (pet.awaitingEggPick() || galleryOpen || gameOpen || battleOpen || quizOpen || gamesMenuOpen || runOpen) return false;  // la galeria gestiona su propio repintado
  if (kbOpen || clockOpen) return true;
  return cardOpen && cardPage != 0;  // la pagina 0 tiene el retrato animado
}
static uint32_t mixKey(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }
static uint32_t screenKey() {
  uint32_t h = 2166136261u;
  h = mixKey(h, pet.awaitingStarter() ? 1 : kbOpen ? 2 : clockOpen ? 3 : 4 + cardPage);
  h = mixKey(h, sdEpoch);
  if (kbOpen) {
    h = mixKey(h, nameLen);
    for (int i = 0; i < nameLen; i++) h = mixKey(h, (uint8_t)nameBuf[i]);
  } else if (clockOpen) {
    h = mixKey(h, clockH * 60 + clockM);
    h = mixKey(h, clockPage);
    h = mixKey(h, wifiSetupState());
    h = mixKey(h, wifiListPage);
    h = mixKey(h, wifiKbLayer);
    h = mixKey(h, (uint32_t)(wifiKbHover + 1));
    h = mixKey(h, wifiSetupNetCount());
    for (const char *p = wifiSetupPass(); *p; p++) h = mixKey(h, (uint8_t)*p);
#ifdef BOARD_KNOMI_V2
    h = mixKey(h, clockWifiStatus());
    h = mixKey(h, clockWifiSsid()[0]);
#endif
    h = mixKey(h, audioEnabled());
    h = mixKey(h, gLang);
  } else if (cardOpen) {
    for (uint32_t v : { (uint32_t)pet.speciesId, (uint32_t)pet.shiny, (uint32_t)pet.ageMinutes, (uint32_t)pet.streak,
                        (uint32_t)pet.bestStreak, (uint32_t)pet.bond, (uint32_t)pet.medals, (uint32_t)pet.weight,
                        (uint32_t)pet.trAtk, (uint32_t)pet.trDef, (uint32_t)pet.trSpe, (uint32_t)pet.geneAtk,
                        (uint32_t)pet.geneDef, (uint32_t)pet.geneSpe, (uint32_t)pet.careMistakes,
                        (uint32_t)pet.fullness, (uint32_t)pet.joy, (uint32_t)pet.energy, (uint32_t)pet.hygiene,
                        (uint32_t)pet.berryKnown, (uint32_t)pet.battleWins, (uint32_t)gLang })
      h = mixKey(h, v);
    for (const char *c = pet.nick; *c; c++) h = mixKey(h, (uint8_t)*c);
  }
  return h;
}
bool screenKeyValid = false;  // (el renderer del host lo invalida entre escenas)
uint32_t lastScreenKey = 0;

// Dibuja un frame y devuelve true si lo ha volcado al panel.
bool render() {
  if (staticScreen()) {
    uint32_t k = screenKey();
    if (screenKeyValid && k == lastScreenKey) return false;
    lastScreenKey = k;
    screenKeyValid = true;
  } else {
    screenKeyValid = false;
  }
  if (pet.awaitingStarter()) {  // primera partida: elegir inicial (prioridad total)
    renderStarterSelect();
    return true;
  }
  if (pet.awaitingEggPick()) {
    renderEggPick();
    return true;
  }
  if (galleryOpen) {
    renderGallery();
    return true;
  }
  if (gameOpen) {
    renderGame();
    return true;
  }
  if (runOpen) {
    renderRun();
    return true;
  }
  if (battleOpen) {
    renderBattle();
    return true;
  }
  if (quizOpen) {
    renderQuiz();
    return true;
  }
  if (gamesMenuOpen) {
    renderGamesMenu();
    return true;
  }
  if (kbOpen) {
    renderKeyboard();
    return true;
  }
  if (clockOpen) {
    renderClock();
    return true;
  }
  if (cardOpen) {
    renderCard();
    return true;
  }
  int h = sceneHour();
  gNight = pet.sleeping || h < 6 || h >= 20;
  // drawScene cubre los 466x466 completos: sin fillScreen(NEGRO) previo para
  // que un flush DMA solapado nunca capture negro a medias (anti-parpadeo)
  drawScene(pet.isEgg() ? 0 : DEX_TBL[pet.speciesId].biome, millis(), gNight);

  if (pet.ceremony) {
    const DexEntry &d = DEX_TBL[pet.speciesId];
    const char *msg = (pet.ceremony == CER_FAREWELL) ? T(S_FAREWELL)
                      : (pet.ceremony == CER_RUNAWAY) ? T(S_RUNAWAY)
                                                      : T(S_GOODBYE);
    drawHeader(dexName(pet.speciesId), d.accent, msg);
    drawCeremony();
    gfx->flush();
    return true;
  }

  if (pet.isEgg()) {
    drawHeader(T(S_EGG_HDR), inkColor(), eggMsg());
    int s = 5, x = CX - 16 * s, y = PET_CY - 16 * s;
    drawMap(SPR_EGG, SPRITE_H, x, y, s, false);
    if (pet.eggCracks() >= 1)
      for (auto &c : CRACK1) gfx->fillRect(x + c[0] * s, y + c[1] * s, s, s, INK_K);
    if (pet.eggCracks() >= 2)
      for (auto &c : CRACK2) gfx->fillRect(x + c[0] * s, y + c[1] * s, s, s, INK_K);
    if (pet.eggRarity() >= R_RARO) {
      const char *rar = (pet.eggRarity() == R_LEGENDARIO) ? T(S_EGG_LEGEND) : T(S_EGG_RARE);
      gfx->setTextColor(pet.eggRarity() == R_LEGENDARIO ? UI_BAR_WARN : 0x4C98);
      setSize(2);
      setCur(centerX(rar, 2), 316);
      printT(rar);
    }
    char reg[24];
    snprintf(reg, sizeof(reg), T(S_POKEDEX_FMT), pet.registeredCount());
    gfx->fillRect(0, 312, 466, 154, gNight ? UI_BG_NIGHT : UI_BG_DAY);
    gfx->setTextColor(inkColor());
    setSize(2);
    setCur(centerX(reg, 2), 348);
    printT(reg);
  } else {
    const DexEntry &d = DEX_TBL[pet.speciesId];
    char name[28];
    const char *base = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
    snprintf(name, sizeof(name), T(S_NAME_FMT), pet.shiny ? "*" : "", base, pet.level());
    drawHeader(name, gNight ? UI_INK_NIGHT : d.accent, statusMsg());
    drawStreakBadge();
    drawPet();
    drawBath();
    drawPoops();
    // panel inferior: base limpia para barras y botones sobre el paisaje
    gfx->fillRect(0, 312, 466, 154, gNight ? UI_BG_NIGHT : UI_BG_DAY);
    drawBars();
    drawButtons();
    drawCelebration();
    if (pet.wantEvolveButton()) drawEvolveButton();        // CTA rojo: evolucionar
    else if (pet.canRunawayNow()) drawRunawayButton();     // CTA sombrio: escapada (abandono)
    else if (pet.wantFarewellButton()) drawFarewellButton();  // CTA dorado: despedida
  }

  if (pet.sleeping) {
    gfx->setTextColor(UI_INK_NIGHT);
    setSize(3);
    setCur(320, 130);
    printT("Zz");
  }

  // selector de comida
  if (feedMenuUntil) {
    if (!timeLeft(feedMenuUntil)) {
      feedMenuUntil = 0;
    } else {
      gfx->fillRoundRect(101, 288, 264, 64, 14, UI_WHITE);
      gfx->drawRoundRect(101, 288, 264, 64, 14, inkColor());
      drawMap(SPR_ICON_FOOD, 16, 110, 296, 3, false);
      drawMap(SPR_ICON_BERRY_B, 16, 176, 296, 3, false);
      drawMap(SPR_ICON_BERRY_G, 16, 242, 296, 3, false);
      drawMap(SPR_ICON_CANDY, 16, 308, 296, 3, false);
    }
  }

  // dialogo "soltar?" (pulsacion larga sobre el bicho)
  if (confirmUntil) {
    if (!timeLeft(confirmUntil)) {
      confirmUntil = 0;
    } else {
      gfx->fillRoundRect(94, 168, 278, 152, 16, UI_WHITE);
      gfx->drawRoundRect(94, 168, 278, 152, 16, UI_INK);
      // 48 y no 28: el buffer se dimensiono cuando toda cadena era de un byte
      // por caracter. Con una fila UTF-8, "%s 놓아줄까요?" mas el nombre pasa
      // de 28 y snprintf corta a mitad de una secuencia de 3 bytes, que la
      // fuente ya no sabe dibujar.
      char q[48];
      snprintf(q, sizeof(q), T(S_RELEASE_FMT), dexName(pet.speciesId));
      gfx->setTextColor(UI_INK);
      setSize(2);
      setCur(centerX(q, 2), 196);
      printT(q);
      gfx->fillRoundRect(118, 252, 100, 52, 12, UI_BAR_OK);
      gfx->setTextColor(UI_WHITE);
      setCur(118 + (100 - textW(T(S_YES), 2)) / 2, 270);
      printT(T(S_YES));
      gfx->fillRoundRect(248, 252, 100, 52, 12, UI_BAR_BAD);
      setCur(248 + (100 - textW(T(S_NO), 2)) / 2, 270);
      printT(T(S_NO));
    }
  }

  // dialogo de decision (evolucionar/mantener, despedirse/quedaros)
  if (choiceKind) {
    if (!timeLeft(choiceUntil)) choiceKind = 0;
    else drawChoiceDialog();
  }

  gfx->flush();
  return true;
}

// ---------- minijuego: toques con la pokeball ----------

void startGame() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  gameOpen = true;
  gameOverUntil = 0;
  gameScore = 0;
  gameMisses = 0;
  gameNewHi = false;
  hitTime = 0;
  gamePetX = 233;
  lastGameStep = millis();
  gameStartT = lastGameStep;
  tiltZero();
  respawnBall();
}

// la baya que cae (ballX/ballY) sale de cualquier punto que quepa en la pantalla redonda
void respawnBall() {
  ballX = 96 + random(275);
  ballY = 96;
  ballVX = 0;
  float sp = 8.0f + gameScore * 0.25f;  // cada vez mas rapido
  ballVY = sp > 18.0f ? 18.0f : sp;
}

// deslizar arriba en cualquier juego: fuera al momento y sin premio (el premio se da al
// terminar la partida, asi que basta con cerrar)
void quitGame() {
  sfxPlay(SFX_TAP);
  if (battleOpen) closeBattle();
  if (quizOpen) closeQuiz();
  if (runOpen) closeRun();
  gameOpen = gamesMenuOpen = false;
}

// cero de la inclinacion: lo que marque ahora es "recto"
float tiltRaw() {
#ifdef BOARD_KNOMI_V2
  float g[3];
  if (accel.read(g)) return g[KNOMI_TILT_AXIS] * KNOMI_TILT_SIGN;
#endif
  return 0;
}

void tiltZero() {
  tiltBase = tiltRaw();
  tiltSmooth = 0;
}

// -1..1. Con unos 30 grados se llega al tope. Sin acelerometro, se apunta con el dedo.
float playerTilt() {
  if (tiltForced) return tiltOverride;
  float t;
#ifdef BOARD_KNOMI_V2
  if (accel.ready()) t = (tiltRaw() - tiltBase) / 0.5f;
  else
#endif
    t = wasPressed ? (tXl - CX) / 150.0f : 0;
  if (t > -0.06f && t < 0.06f) t = 0;  // zona muerta: el pulso no mueve nada
  t = t > 1 ? 1 : (t < -1 ? -1 : t);
  tiltSmooth += (t - tiltSmooth) * 0.5f;
  return tiltSmooth;
}

void stepGame() {
  uint32_t now = millis();
  float k = lastGameStep ? (now - lastGameStep) / 85.0f : 1.0f;
  if (k > 3) k = 3;
  lastGameStep = now;

  float tilt = playerTilt();
  gamePetX += tilt * 22.0f * k;
  if (gamePetX < 90) gamePetX = 90;
  if (gamePetX > 376) gamePetX = 376;
  gamePetVX = tilt;

  ballY += ballVY * k;
  if (ballY >= 330 && ballY <= 392 && fabsf(ballX - gamePetX) < 54) {  // a la boca
    if (gameScore < 255) gameScore++;
    hitX = ballX;
    hitY = ballY;
    hitTime = now;
    sfxPlay(SFX_PLAY);
    respawnBall();
  } else if (ballY > 392) {  // al suelo
    sfxPlay(SFX_DENY);
    if (++gameMisses >= 3) {
      gameNewHi = (gameScore > pet.gameHi);
      pet.playResult(gameScore);  // actualiza el record y da felicidad
      sfxPlay(gameNewHi && gameScore > 0 ? SFX_MEDAL : SFX_LEVEL);
      gameOverUntil = now + 3500;
      return;
    }
    respawnBall();
  }
}

// ---------- saco de entrenamiento (entrena la fuerza) ----------

// ---------- menu de juegos ----------
#define GAMES_ROW_X 83
#define GAMES_ROW_W 300
#define GAMES_ROW_H 56
#define GAMES_ROW_Y 120
#ifdef BOARD_KNOMI_V2
#define GAMES_COUNT 3
#else
#define GAMES_COUNT 4  // sin boton fisico, el combate vive aqui
#endif

void openGamesMenu() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) { sfxPlay(SFX_DENY); return; }
  gamesMenuOpen = true;
  gamesMenuUntil = millis() + 20000;
}

void gamesMenuTap(int16_t x, int16_t y) {
  gamesMenuOpen = false;
  int row = (y - GAMES_ROW_Y) / (GAMES_ROW_H + 10);
  if (y < GAMES_ROW_Y || row >= GAMES_COUNT || x < GAMES_ROW_X || x > GAMES_ROW_X + GAMES_ROW_W) return;  // fuera = volver
  sfxPlay(SFX_TAP);
  if (row == 0) startGame();
  else if (row == 1) startQuiz();
  else if (row == 2) startRun();
  else startBattle();
}

void renderGamesMenu() {
  if (!timeLeft(gamesMenuUntil) || pet.sleeping || pet.ceremony) { gamesMenuOpen = false; return; }
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_GAMES), 3), 74);
  printT(T(S_GAMES));
  const char *labels[4] = { T(S_GAME_TILT), T(S_GAME_WHO), T(S_GAME_RUN), T(S_BTL_START) };
  const uint16_t colors[4] = { TYPE_ACCENT[TY_PLANTA], TYPE_ACCENT[TY_PSIQUICO], TYPE_ACCENT[TY_FUEGO], UI_BAR_BAD };
  setSize(2);
  for (int i = 0; i < GAMES_COUNT; i++) {
    int y = GAMES_ROW_Y + i * (GAMES_ROW_H + 10);
    gfx->fillRoundRect(GAMES_ROW_X, y, GAMES_ROW_W, GAMES_ROW_H, 14, colors[i]);
    gfx->setTextColor(UI_WHITE);
    setCur(centerX(labels[i], 2), y + 20);
    printT(labels[i]);
  }
  gfx->setTextColor(UI_INK);
  setCur(centerX(T(S_SWIPE_QUIT), 2), GAMES_ROW_Y + GAMES_COUNT * (GAMES_ROW_H + 10) + 8);
  printT(T(S_SWIPE_QUIT));
  gfx->flush();
}

// ---------- quien es ese pokemon ----------
void quizNextRound() {
  int16_t prev = quizAnswer;
  do { quizAnswer = 1 + random(151); } while (quizAnswer == prev);
  quizChoices(quizAnswer, 1 + random(151), 1 + random(151), random(3), quizOpts);
  quizPmd.load(quizAnswer, false);
  quizTapped = 0;
  quizPhase = QP_ASK;
  quizT = millis();
}

void startQuiz() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) return;
  quizRound = 0;
  quizScore = 0;
  quizAnswer = 0;
  quizNextRound();
  if (!quizPmd.loaded) return;  // sin sprites no hay silueta que ensenar
  quizOpen = true;
}

void closeQuiz() {
  quizOpen = false;
  quizPmd.unload();
}

#define QUIZ_OPT_Y 268
#define QUIZ_OPT_H 44
#define QUIZ_OPT_GAP 6

void quizTap(int16_t x, int16_t y) {
  if (quizPhase == QP_END) {
    if (millis() - quizT > 800) closeQuiz();
    return;
  }
  if (quizPhase != QP_ASK) return;
  if (y < QUIZ_OPT_Y) return;
  int i = (y - QUIZ_OPT_Y) / (QUIZ_OPT_H + QUIZ_OPT_GAP);
  if (i > 2) return;
  quizTapped = quizOpts[i];
  bool right = quizTapped == quizAnswer;
  if (right) quizScore++;
  sfxPlay(right ? SFX_HEART : SFX_DENY);
  quizPhase = QP_REVEAL;
  quizT = millis();
}

void renderQuiz() {
  if (pet.ceremony) { closeQuiz(); return; }
  uint32_t t = millis() - quizT;
  if (quizPhase == QP_REVEAL && t > 2000) {
    if (++quizRound >= QUIZ_ROUNDS) {
      pet.quizResult(quizScore);
      sfxPlay(quizScore >= QUIZ_ROUNDS ? SFX_MEDAL : SFX_LEVEL);
      quizPhase = QP_END;
      quizT = millis();
    } else {
      quizNextRound();
    }
    t = 0;
  }
  if (quizPhase == QP_END && t > 5000) { closeQuiz(); return; }
  if (!quizOpen) return;

  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, TYPE_ACCENT[TY_AGUA]);
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  char b[32];
  if (quizPhase == QP_END) {
    snprintf(b, sizeof(b), "%u/%u", quizScore, QUIZ_ROUNDS);
    setSize(6);
    setCur(centerX(b, 6), 170);
    printT(b);
    setSize(2);
    const char *msg = quizScore >= 3 ? T(S_GREAT_JOY) : T(S_PLUS_JOY);
    setCur(centerX(msg, 2), 260);
    printT(msg);
    gfx->flush();
    return;
  }
  bool reveal = quizPhase == QP_REVEAL;
  if (reveal) snprintf(b, sizeof(b), T(S_ITS_FMT), dexName(quizAnswer));
  else strlcpy(b, T(S_WHO_Q), sizeof(b));
  setCur(centerX(b, 2), 48);
  printT(b);
  snprintf(b, sizeof(b), "%u/%u", quizRound + 1, QUIZ_ROUNDS);
  setCur(centerX(b, 2), 72);
  printT(b);
  gPmdForceScale = pmdFitScale(quizPmd, PMD_IDLE, 300, 166);
  drawPmdActM(quizPmd, PMD_IDLE, CX, 262, reveal ? millis() : 0, true, !reveal, 8);
  gPmdForceScale = 0;
  for (int i = 0; i < 3; i++) {
    int y = QUIZ_OPT_Y + i * (QUIZ_OPT_H + QUIZ_OPT_GAP);
    int inset = i == 2 ? 26 : 0;  // la de abajo, mas estrecha: el circulo se cierra
    uint16_t bg = UI_WHITE;
    if (reveal && quizOpts[i] == quizAnswer) bg = UI_BAR_OK;
    else if (reveal && quizOpts[i] == quizTapped) bg = UI_BAR_BAD;
    gfx->fillRoundRect(83 + inset, y, 300 - 2 * inset, QUIZ_OPT_H, 10, bg);
    gfx->setTextColor(bg == UI_WHITE ? UI_INK : UI_WHITE);
    setCur(centerX(dexName(quizOpts[i]), 2), y + 15);
    printT(dexName(quizOpts[i]));
  }
  gfx->flush();
}

// ---------- visita diaria ----------
#define VISIT_ANNOUNCE_MS 3500UL
#define VISIT_ENTER_MS 2500UL
#define VISIT_HANG_MS 40000UL
#define VISIT_LEAVE_MS 3500UL
#define VISIT_X 356

void startVisitWith(int16_t dex) {
  if (visitPhase != VP_NONE || pet.isEgg() || pet.sleeping || pet.ceremony) return;
  if (dex < 1 || dex > DEX_COUNT) dex = pet.pickWildSpecies(btlCapLevel(pet.level()));
  visitPmd.load(dex, false);
  if (!visitPmd.loaded) return;
  visitDex = dex;
  visitPhase = VP_ANNOUNCE;
  visitT = millis();
  pet.visitorArrived();
  sfxPlay(SFX_HEART);
  Serial.printf("visita: #%d\n", dex);
}

void endVisit() {
  visitPhase = VP_NONE;
  visitPmd.unload();
}

// comprueba la hora y hace avanzar la visita, aunque haya otra pantalla abierta encima
void visitTick(uint32_t now) {
  if (visitPhase == VP_NONE) {
    if (now - visitCheckT < 20000) return;
    visitCheckT = now;
    bool mainScreen = !gameOpen && !runOpen && !battleOpen && !quizOpen && !gamesMenuOpen && !galleryOpen && !cardOpen &&
                      !kbOpen && !clockOpen && !choiceKind && !confirmUntil;
    // con la pantalla atenuada no hay nadie mirando: mejor esperar a que alguien vuelva
    if (mainScreen && !screenOff && dimStage == 0 && pet.visitorDueNow()) startVisitWith(0);
    return;
  }
  if (pet.sleeping || pet.ceremony) { endVisit(); return; }
  uint32_t t = now - visitT;
  if (visitPhase == VP_ANNOUNCE && t > VISIT_ANNOUNCE_MS) { visitPhase = VP_ENTER; visitT = now; }
  else if (visitPhase == VP_ENTER && t > VISIT_ENTER_MS) { visitPhase = VP_HANG; visitT = now; }
  else if (visitPhase == VP_HANG && t > VISIT_HANG_MS) {
    visitPhase = VP_LEAVE;
    visitT = now;
    sfxPlay(SFX_BYE);
  } else if (visitPhase == VP_LEAVE && t > VISIT_LEAVE_MS) {
    pet.visitorLeft();
    endVisit();
  }
}

void drawVisitBanner(const char *msg) {
  setSize(2);
  int w = textW(msg, 2) + 32;
  gfx->fillRoundRect(CX - w / 2, 116, w, 36, 12, UI_WHITE);
  gfx->drawRoundRect(CX - w / 2, 116, w, 36, 12, UI_INK);
  gfx->setTextColor(UI_INK);
  setCur(centerX(msg, 2), 126);
  printT(msg);
}

void drawVisitor(uint32_t now) {
  if (visitPhase == VP_NONE) return;
  uint32_t t = now - visitT;
  int x = VISIT_X;
  uint8_t act = PMD_IDLE;
  if (visitPhase == VP_ANNOUNCE) {
    drawVisitBanner(T(S_VISITOR));
    return;
  }
  if (visitPhase == VP_ENTER) {
    x = 540 - (int)((540 - VISIT_X) * (float)t / VISIT_ENTER_MS);
    if (visitPmd.has(PMD_WALKL)) act = PMD_WALKL;
  } else if (visitPhase == VP_LEAVE) {
    x = VISIT_X + (int)((540 - VISIT_X) * (float)t / VISIT_LEAVE_MS);
    if (visitPmd.has(PMD_WALKR)) act = PMD_WALKR;
  } else if ((t / 6000) % 2 && visitPmd.has(PMD_HOP)) {
    act = PMD_HOP;
  }
  drawPmdActM(visitPmd, act, x, PET_GROUND, now - visitT, true, false, 4);
  if (visitPhase == VP_HANG && (t / 900) % 4 == 0) drawMap(SPR_HEART, 32, x - 90, PET_GROUND - 160, 1, false);
  if (visitPhase == VP_LEAVE) {
    char b[40];
    snprintf(b, sizeof(b), T(S_VISIT_BYE_FMT), dexName(visitDex));
    drawVisitBanner(b);
  }
}

#ifdef BOARD_KNOMI_V2
// Boton BOOT del Knomi (GPIO0, activo en bajo): toque = rival salvaje, 1-3 s = pantalla,
// 10 s = preguntar si borrar la partida.
// La linea DTR del CH340 tambien mueve este pin al abrir el puerto serie, de ahi la espera
// tras arrancar. Mantenerlo pulsado durante un reset sigue entrando en modo de grabacion.
#define KNOMI_BTN_HOLD_MS 800      // soltar a partir de aqui: pantalla
#define KNOMI_BTN_HOLD_MAX_MS 3000 // soltar mas tarde no hace nada: se ha arrepentido
#define KNOMI_BTN_RESET_MS 10000   // mantenido hasta aqui: pregunta si borrar la partida

void knomiButtonTap() {
  bool wasAsleep = screenOff || dimStage > 0;
  screenOff = false;
  wakeUI();
  if (wasAsleep) return;  // a oscuras solo despierta, como la pantalla tactil
  if (gameOpen || runOpen || quizOpen || gamesMenuOpen || galleryOpen || cardOpen || kbOpen || clockOpen || choiceKind || confirmUntil) return;
  if (visitPhase != VP_NONE) return;  // no se pelea con la visita delante
  startBattle();
}

// el dialogo se dibuja en la pantalla principal: cierra lo que este abierto por encima
void knomiAskReset() {
  if (pet.awaitingStarter() || pet.ceremony) return;  // nada que borrar, o despedida en curso
  if (battleOpen) closeBattle();
  if (quizOpen) closeQuiz();
  if (runOpen) closeRun();
  gamesMenuOpen = false;
  if (galleryOpen) { galleryOpen = false; galleryPmd.unload(); }
  wifiSetupStop();
  gameOpen = cardOpen = kbOpen = clockOpen = false;
  confirmUntil = 0;
  screenOff = false;
  wakeUI();
  sfxPlay(SFX_DENY);
  choiceKind = 3;
  choiceUntil = millis() + 12000;
}

void handleKnomiButton(uint32_t now) {
  static uint32_t downAt = 0;
  static bool down = false, resetAsked = false;
  if (now < 3000) return;
  bool pressed = digitalRead(KNOMI_BTN_PIN) == LOW || timeLeft(gBtnSimUntil) > 0;
  if (pressed && !down) {
    down = true;
    resetAsked = false;
    downAt = now;
  } else if (pressed && !resetAsked && now - downAt >= KNOMI_BTN_RESET_MS) {
    resetAsked = true;
    knomiAskReset();
  } else if (!pressed && down) {
    down = false;
    uint32_t held = now - downAt;
    if (held < 30 || resetAsked) return;  // 30 ms: antirrebote
    if (held < KNOMI_BTN_HOLD_MS) {
      knomiButtonTap();
    } else if (held < KNOMI_BTN_HOLD_MAX_MS) {
      screenOff = !screenOff;
      if (!screenOff) wakeUI();
      Serial.printf("boton: pantalla %s\n", screenOff ? "apagada" : "encendida");
    }
  }
}
#endif

// ---------- combate ----------
// Banda inferior de mensajes y botones; lo que hay por encima es la escena, como en gen 1.
#define BTL_BAND_Y 336
#define BTL_FOE_X 330
#define BTL_FOE_GROUND 186
#define BTL_ME_X 128
#define BTL_ME_GROUND 322

void battleSetPhase(BattlePhase p) {
  bPhase = p;
  bPhaseT = millis();
}

void startBattleWith(int16_t dex) {
  if (pet.isEgg() || pet.ceremony || battleOpen || !pmd.loaded) return;
  uint8_t lvl = btlCapLevel(pet.level());
  if (pet.sleeping || pet.energy < BTL_MIN_ENERGY) {
    battleOpen = true;  // un aviso breve en vez de un boton que no hace nada
    sfxPlay(SFX_DENY);
    battleSetPhase(BP_TIRED);
    return;
  }
  uint8_t foeLvl = btlWildLevel(lvl, random(4));
  if (dex < 1 || dex > DEX_COUNT) dex = pet.pickWildSpecies(foeLvl);
  wildPmd.load(dex, false);
  if (!wildPmd.loaded) return;  // sin sprite no hay rival que ensenar

  const DexEntry &d = DEX_TBL[pet.speciesId];
  bMe = BtlFighter();
  bMe.dex = pet.speciesId;
  bMe.level = lvl;
  bMe.hp = bMe.maxHp = btlMaxHp(d.bHp, lvl);
  bMe.atk = btlStat(d.bAtk, pet.geneAtk, lvl, pet.trAtk);
  bMe.def = btlStat(d.bDef, pet.geneDef, lvl, pet.trDef);
  bMe.spe = btlStat(d.bSpe, pet.geneSpe, lvl, pet.trSpe);
  bFoe = btlMakeWild(dex, foeLvl, btlWildGene(random(16)));
  bMeMoves = btlMovesFor(bMe.dex, lvl, dex);
  bFoeMoves = btlMovesFor(dex, foeLvl, bMe.dex);
  bShownMe = bMe.hp;
  bShownFoe = bFoe.hp;
  bWon = false;
  bGain = 0;
  battleOpen = true;
  battleSetPhase(BP_INTRO);
  sfxPlay(SFX_PLAY);
  Serial.printf("combate: rival #%d Nv.%u %s/%s contra %s/%s\n", dex, foeLvl,
                MOVE_TBL[bFoeMoves.ids[0]].name, MOVE_TBL[bFoeMoves.ids[1]].name,
                MOVE_TBL[bMeMoves.ids[0]].name, MOVE_TBL[bMeMoves.ids[1]].name);
}

void startBattle() { startBattleWith(0); }

void closeBattle() {
  battleOpen = false;
  wildPmd.unload();
}

void battleMoveRect(uint8_t i, int &x0, int &x1) {
  if (bMeMoves.count == 1) { x0 = 116; x1 = 350; return; }
  x0 = i == 0 ? 62 : 238;
  x1 = i == 0 ? 228 : 404;
}

void battleTap(int16_t x, int16_t y) {
  if (bPhase == BP_END) {
    if (millis() - bPhaseT > 800) battleAfterEnd();  // tiempo para leer el resultado
    return;
  }
  if (bPhase == BP_AIM) {
    if (y < BTL_BAND_Y) return;
    sfxPlay(SFX_TAP);
    tiltZero();
    bBallX = CX;
    bBallT = millis();
    battleSetPhase(BP_THROW);
    return;
  }
  if (bPhase != BP_CHOOSE) return;
  if (y >= 392 && x >= 160 && x <= 306) {
    sfxPlay(SFX_TAP);
    battleSetPhase(BP_RAN);
    return;
  }
  if (y < 344 || y > 386) return;
  for (uint8_t i = 0; i < bMeMoves.count; i++) {
    int x0, x1;
    battleMoveRect(i, x0, x1);
    if (x < x0 || x > x1) continue;
    sfxPlay(SFX_TAP);
    bool first = btlPlayerFirst(bMe.spe, bFoe.spe, random(2));
    uint8_t foeMove = btlWildMove(bFoeMoves, random(100));
    bTurn[0] = { first, first ? i : foeMove };
    bTurn[1] = { !first, first ? foeMove : i };
    bStep = 0;
    battleSetPhase(BP_ATTACK);
    return;
  }
}

void battleFinish(bool won) {
  bWon = won;
  bGain = pet.battleResult(won);
  sfxPlay(won ? SFX_LEVEL : SFX_DENY);
  Serial.printf("combate: gana=%d fuerza+%u\n", won, bGain);
  battleSetPhase(BP_END);
}

// tras el resultado: si es una especie nueva, se puede intentar capturar
void battleAfterEnd() {
  if (bWon && !pet.isRegistered(bFoe.dex)) {
    bBalls = CATCH_BALLS;
    battleSetPhase(BP_AIM);
  } else {
    closeBattle();
  }
}

// el rival se mueve de lado a lado para que haya que apuntar
float battleFoeSway(uint32_t now) { return CX + 110.0f * sinf(now * 6.28318f / 4500.0f); }

void battleThrowStep(uint32_t now) {
  static uint32_t last = 0;
  float dt = (now - last) / 1000.0f;
  last = now;
  if (dt > 0.2f) dt = 0.2f;
  bBallX += playerTilt() * 280.0f * dt;
  if (tiltAim) bBallX = battleFoeSway(now);
  if (bBallX < 70) bBallX = 70;
  if (bBallX > 396) bBallX = 396;
  if (now - bBallT < 1500) return;
  if (catchHit(bBallX, battleFoeSway(now))) {
    bool isNew = pet.catchWild(bFoe.dex);
    Serial.printf("captura: #%d nuevo=%d\n", bFoe.dex, isNew);
    sfxPlay(SFX_MEDAL);
    battleSetPhase(BP_CAUGHT);
  } else {
    bBalls--;
    Serial.printf("captura: fallo, quedan %u\n", bBalls);
    sfxPlay(SFX_DENY);
    battleSetPhase(BP_MISS);
  }
}

// aplica el golpe del turno en curso
void battleApplyHit() {
  bool mine = bTurn[bStep].mine;
  BtlFighter &a = mine ? bMe : bFoe, &t = mine ? bFoe : bMe;
  uint8_t aType = DEX_TBL[a.dex].type, tType = DEX_TBL[t.dex].type;
  BtlMove m = MOVE_TBL[(mine ? bMeMoves : bFoeMoves).ids[bTurn[bStep].move]];
  uint16_t dmg = btlDamage(a.level, m, a.atk, t.def, aType, tType, 85 + random(16));
  bLastEff = btlEffect(m.type, tType);
  t.hp = dmg >= t.hp ? 0 : t.hp - dmg;
  if (dmg) sfxPlay(SFX_PLAY);
}

void battleAfterHit() {
  if (bFoe.hp == 0) { battleFinish(true); return; }
  if (bMe.hp == 0) { battleFinish(false); return; }
  if (++bStep < 2) battleSetPhase(BP_ATTACK);
  else battleSetPhase(BP_CHOOSE);
}

void battleStep() {
  uint32_t t = millis() - bPhaseT;
  switch (bPhase) {
    case BP_TIRED: if (t > 1500) closeBattle(); break;
    case BP_INTRO:
      if (t > 1800) {
        // se avisa del ataque nuevo una sola vez; DITTO copia, no aprende
        int16_t learned = bMe.dex == DEX_DITTO ? -1 : btlMoveToAnnounce(pet.movesSeen, bMeMoves);
        if (bMe.dex != DEX_DITTO) pet.movesSeen = btlMovesSeenValue(bMeMoves);
        if (learned >= 0) {
          bLearned = learned;
          sfxPlay(SFX_MEDAL);
          battleSetPhase(BP_LEARNED);
        } else battleSetPhase(BP_CHOOSE);
      }
      break;
    case BP_LEARNED: if (t > 2000) battleSetPhase(BP_CHOOSE); break;
    case BP_CHOOSE:  // nadie responde: huye solo para que la pantalla pueda atenuarse
      if (idleSince(millis(), lastInteract) > 60000) battleSetPhase(BP_RAN);
      break;
    case BP_ATTACK:
      if (t > 800) { battleApplyHit(); battleSetPhase(BP_HIT); }
      break;
    case BP_HIT:
      if (t > 700) {
        if (bLastEff != 2) battleSetPhase(BP_EFFECT);
        else battleAfterHit();
      }
      break;
    case BP_EFFECT: if (t > 1000) battleAfterHit(); break;
    case BP_RAN: if (t > 1200) closeBattle(); break;
    case BP_END: if (t > 3500) battleAfterEnd(); break;
    case BP_AIM:  // nadie responde: se va
      if (idleSince(millis(), lastInteract) > 60000) battleSetPhase(BP_FLED);
      break;
    case BP_THROW: battleThrowStep(millis()); break;
    case BP_MISS:
      if (t > 2000) battleSetPhase(bBalls ? BP_AIM : BP_FLED);
      break;
    case BP_CAUGHT: if (t > 5000) closeBattle(); break;
    case BP_FLED: if (t > 3500) closeBattle(); break;
  }
}

void drawBattlePanel(int x, int y, const BtlFighter &f, float shown, bool numbers) {
  gfx->fillRoundRect(x, y, 178, 62, 10, UI_WHITE);
  gfx->drawRoundRect(x, y, 178, 62, 10, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(x + 10, y + 6);
  printT(dexName(f.dex));
  float pct = f.maxHp ? shown / f.maxHp : 0;
  gfx->fillRoundRect(x + 10, y + 28, 158, 10, 4, UI_TRACK);
  int w = (int)(158 * pct);
  if (w > 2) gfx->fillRoundRect(x + 10, y + 28, w, 10, 4, pct > 0.5f ? UI_BAR_OK : (pct > 0.2f ? UI_BAR_WARN : UI_BAR_BAD));
  char b[16];
  if (numbers) snprintf(b, sizeof(b), "L%u  %u/%u", f.level, (unsigned)(shown + 0.5f), f.maxHp);
  else snprintf(b, sizeof(b), "L%u", f.level);
  setCur(x + 10, y + 42);
  printT(b);
}

void drawBattleFighter(PmdMon &m, bool mine, int cx, int ground, uint8_t maxS) {
  uint32_t now = millis(), t = now - bPhaseT;
  uint8_t act = PMD_IDLE;
  bool attacking = bPhase == BP_ATTACK && bTurn[bStep].mine == mine;
  bool hurt = (bPhase == BP_HIT || bPhase == BP_EFFECT) && bTurn[bStep].mine != mine && bLastEff != 0;
  int x = cx, y = ground;
  if (attacking) {
    if (m.has(PMD_ATTACK)) act = PMD_ATTACK;
    float k = sinf(min(t, 800UL) * 3.14159f / 800.0f);  // embestida hacia el rival
    x += (int)((mine ? 34 : -34) * k);
    y += (int)((mine ? -14 : 14) * k);
  } else if (hurt && bPhase == BP_HIT) {
    if (m.has(PMD_HURT)) act = PMD_HURT;
    if ((t / 90) % 2) return;  // parpadeo del golpe
  }
  if (bPhase == BP_END && (mine ? bMe.hp : bFoe.hp) == 0) return;  // el derrotado ya no esta
  drawPmdActM(m, act, x, y, now - bPhaseT, true, false, maxS);
}

void battleCenterLine(const char *s, int y) {
  setCur(centerX(s, 2), y);
  printT(s);
}

// el resultado de un tiro: texto grande sobre color, para que nadie se quede con la duda
void catchBanner(const char *s, uint16_t bg) {
  uint8_t size = textW(s, 3) <= 306 ? 3 : 2;
  int w = textW(s, size) + 24, h = size == 3 ? 44 : 36;
  gfx->fillRoundRect(CX - w / 2, 352, w, h, 12, bg);
  gfx->setTextColor(UI_WHITE);
  setSize(size);
  setCur(centerX(s, size), 352 + (h - 8 * size) / 2);
  printT(s);
  setSize(2);
  gfx->setTextColor(UI_INK);
}

void renderCatch() {
  uint32_t now = millis();
  drawGameScene();
  float foeX = battleFoeSway(now);
  bFoeX = foeX;
  uint8_t bio = DEX_TBL[bFoe.dex].biome;
  uint16_t soil = BIOME_SOIL[bio < 6 ? bio : 0];
  if (sceneHour() < 6 || sceneHour() >= 20) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  gfx->fillRect(0, 216, 466, BTL_BAND_Y - 216, soil);
  if (bPhase != BP_CAUGHT && bPhase != BP_FLED) {
    drawPmdActM(wildPmd, PMD_IDLE, (int)foeX, 216, now - bPhaseT, true, false, 3);
  }
  if (bPhase == BP_THROW) {
    float k = (now - bBallT) / 1500.0f;
    if (k > 1) k = 1;
    int y = 360 - (int)(200 * k), s = k < 0.5f ? 3 : 2;  // se aleja
    drawMap(SPR_ICON_PLAY, 16, (int)bBallX - 8 * s, y - 8 * s, s, false);
  } else if (bPhase == BP_MISS) {  // rebota y cae: se ve que no ha entrado
    float k = (now - bPhaseT) / 700.0f;
    if (k > 1) k = 1;
    drawMap(SPR_ICON_PLAY, 16, (int)(bBallX + (bBallX < foeX ? -70 : 70) * k) - 16, 144 + (int)(200 * k * k), 2, false);
  } else if (bPhase == BP_CAUGHT) {
    int wob = (int)(10 * sinf((now - bPhaseT) * 0.012f));
    drawMap(SPR_ICON_PLAY, 16, (int)bFoeX - 24 + wob, 150, 3, false);
  }

  gfx->fillRect(0, BTL_BAND_Y, 466, 466 - BTL_BAND_Y, UI_BG_DAY);
  gfx->fillRect(0, BTL_BAND_Y, 466, 3, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(2);
  char b[48];
  switch (bPhase) {
    case BP_AIM:
      battleCenterLine(T(S_THROW_HINT), 350);
      drawMap(SPR_ICON_PLAY, 16, CX - 32, 376, 4, false);  // la pokeball es el boton
      for (uint8_t i = 0; i < bBalls; i++) gfx->fillCircle(CX + 56 + i * 16, 408, 5, UI_BAR_BAD);
      break;
    case BP_THROW:
      battleCenterLine(T(S_TILT_HINT), 380);
      break;
    case BP_MISS:
      catchBanner(T(S_MISSED), C565(0xb8, 0x2e, 0x2a));
      for (uint8_t i = 0; i < bBalls; i++) gfx->fillCircle(CX + (i * 2 - (bBalls - 1)) * 12, 416, 6, UI_BAR_BAD);
      break;
    case BP_CAUGHT:
      snprintf(b, sizeof(b), T(S_CAUGHT_FMT), dexName(bFoe.dex));
      catchBanner(b, C565(0x1e, 0x7d, 0x3c));
      battleCenterLine(T(S_BTL_NEWDEX), 408);
      break;
    case BP_FLED:
      snprintf(b, sizeof(b), T(S_FLED_FMT), dexName(bFoe.dex));
      catchBanner(b, C565(0x55, 0x55, 0x60));
      break;
    default: break;
  }
  gfx->flush();
}

void renderBattle() {
  if (pet.ceremony) { closeBattle(); return; }  // la despedida manda
  battleStep();
  if (!battleOpen) return;

  if (bPhase >= BP_AIM) {
    renderCatch();
    return;
  }
  drawGameScene();
  bool tired = bPhase == BP_TIRED;
  if (!tired) {
    uint8_t bio = DEX_TBL[bFoe.dex].biome;
    uint16_t soil = BIOME_SOIL[bio < 6 ? bio : 0];
    if (sceneHour() < 6 || sceneHour() >= 20) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
    gfx->fillRect(0, 232, 466, BTL_BAND_Y - 232, soil);
    gfx->fillRoundRect(BTL_FOE_X - 70, BTL_FOE_GROUND - 10, 140, 24, 12, soil);
    // la vida en pantalla persigue a la real
    bShownMe += (bMe.hp - bShownMe) * 0.28f;
    bShownFoe += (bFoe.hp - bShownFoe) * 0.28f;
    drawBattleFighter(wildPmd, false, BTL_FOE_X, BTL_FOE_GROUND, 3);
    drawBattleFighter(pmd, true, BTL_ME_X, BTL_ME_GROUND, 4);
    drawBattlePanel(72, 64, bFoe, bShownFoe, false);
    drawBattlePanel(226, 256, bMe, bShownMe, true);
  } else {
    drawPmdAct(pet.sleeping && pmd.has(PMD_SLEEP) ? PMD_SLEEP : PMD_IDLE, CX, 300, millis(), true, false, 4);
  }

  gfx->fillRect(0, BTL_BAND_Y, 466, 466 - BTL_BAND_Y, UI_BG_DAY);
  gfx->fillRect(0, BTL_BAND_Y, 466, 3, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(2);
  char b[48];
  switch (bPhase) {
    case BP_TIRED: battleCenterLine(T(S_BTL_TIRED), 380); break;
    case BP_INTRO:
      snprintf(b, sizeof(b), T(S_BTL_WILD), dexName(bFoe.dex));
      battleCenterLine(b, 380);
      break;
    case BP_LEARNED:
      snprintf(b, sizeof(b), T(S_BTL_LEARNED), MOVE_TBL[bLearned].name);
      battleCenterLine(b, 380);
      break;
    case BP_CHOOSE:
      for (uint8_t i = 0; i < bMeMoves.count; i++) {
        BtlMove m = MOVE_TBL[bMeMoves.ids[i]];
        int x0, x1;
        battleMoveRect(i, x0, x1);
        gfx->fillRoundRect(x0, 346, x1 - x0, 40, 10, TYPE_ACCENT[m.type]);
        gfx->setTextColor(UI_WHITE);
        setCur((x0 + x1) / 2 - (int)strlen(m.name) * 6, 359);
        printT(m.name);
      }
      gfx->fillRoundRect(160, 394, 146, 38, 10, UI_TRACK);
      gfx->setTextColor(UI_INK);
      battleCenterLine(T(S_BTL_RUN), 406);
      break;
    case BP_ATTACK:
    case BP_HIT: {
      const BtlFighter &a = bTurn[bStep].mine ? bMe : bFoe;
      snprintf(b, sizeof(b), "%s: %s!", dexName(a.dex), MOVE_TBL[(bTurn[bStep].mine ? bMeMoves : bFoeMoves).ids[bTurn[bStep].move]].name);
      battleCenterLine(b, 380);
      break;
    }
    case BP_EFFECT:
      battleCenterLine(T(bLastEff == 0 ? S_BTL_IMMUNE : (bLastEff > 2 ? S_BTL_SUPER : S_BTL_WEAK)), 380);
      break;
    case BP_RAN: battleCenterLine(T(S_BTL_RAN), 380); break;
    case BP_END: {
      if (bWon) {
        gfx->setTextColor(UI_BAR_OK);
        battleCenterLine(T(S_BTL_WON), 352);
      } else {
        snprintf(b, sizeof(b), T(S_BTL_LOST), dexName(bMe.dex));
        battleCenterLine(b, 352);
      }
      gfx->setTextColor(UI_BAR_BAD);
      snprintf(b, sizeof(b), T(S_STR_GAIN_FMT), bGain);
      battleCenterLine(b, 380);
      break;
    }
  }
  gfx->flush();
}

// fondo del minijuego: hatibat del bicho (cielo por hora + suelo del bioma)
void drawGameScene() {
  int hh = sceneHour();
  bool night = hh < 6 || hh >= 20;
  uint16_t top, bot;
  if (night)       { top = C565(0x0c, 0x12, 0x24); bot = C565(0x1e, 0x26, 0x46); }
  else if (hh < 8) { top = C565(0xd1, 0x6a, 0x86); bot = C565(0xf3, 0xb8, 0x7c); }
  else if (hh < 18){ top = C565(0x8f, 0xc8, 0xea); bot = C565(0xdc, 0xee, 0xe6); }
  else             { top = C565(0xc7, 0x5a, 0x4a); bot = C565(0xf0, 0xae, 0x64); }
  int hor = 376;
  for (int y = 0; y < hor; y += 8)
    gfx->fillRect(0, y, 466, 8, lerp565(top, bot, y, hor));
  if (night)
    for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
  uint8_t bio = pet.isEgg() ? 0 : DEX_TBL[pet.speciesId].biome;
  uint16_t soil = BIOME_SOIL[bio < 6 ? bio : 0];
  if (night) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  gfx->fillRect(0, hor, 466, 466 - hor, soil);
}

void renderGame() {
  // sin fillScreen(NEGRO): drawGameScene cubre los 466x466 completos. Si el
  // DMA del flush anterior aun lee el buffer, vera contenido valido (no negro
  // a medio pintar), que era el parpadeo a 25 fps.
  bool night = sceneHour() < 6 || sceneHour() >= 20;
  uint16_t ink = night ? UI_INK_NIGHT : UI_INK;

  if (gameOverUntil) {
    drawGameScene();
    if (!timeLeft(gameOverUntil)) {
      gameOpen = false;
      return;
    }
    char buf[22];
    snprintf(buf, sizeof(buf), T(S_SCORE_FMT), gameScore);
    gfx->setTextColor(ink);
    setSize(4);
    setCur(centerX(buf, 4), 160);
    printT(buf);
    setSize(2);
    if (gameNewHi && gameScore > 0) {
      gfx->setTextColor(UI_BAR_WARN);
      setCur(centerX(T(S_NEW_RECORD), 2), 214);
      printT(T(S_NEW_RECORD));
    } else {
      char rec[20];
      snprintf(rec, sizeof(rec), T(S_RECORD_FMT), pet.gameHi);
      gfx->setTextColor(ink);
      setCur(centerX(rec, 2), 214);
      printT(rec);
    }
    const char *msg = gameScore >= 10 ? T(S_GREAT_JOY) : T(S_PLUS_JOY);
    gfx->setTextColor(ink);
    setCur(centerX(msg, 2), 250);
    printT(msg);
    gfx->flush();
    return;
  }

  drawGameScene();
  stepGame();

  // marcador, record y vidas
  char buf[8];
  snprintf(buf, sizeof(buf), "%u", gameScore);
  gfx->setTextColor(ink);
  setSize(4);
  setCur(centerX(buf, 4), 30);
  printT(buf);
  // 24 y no 12: en japones "きろく %u" son 10 bytes antes de la cifra y el
  // record se quedaba en su primer digito ("きろく 2" con 219). Las tallas de
  // los buffers de texto las vigila TestBuffersDeTexto en test/test_tools.py.
  char rec[24];
  snprintf(rec, sizeof(rec), T(S_REC_FMT), pet.gameHi);
  setSize(2);
  setCur(centerX(rec, 2), 76);
  printT(rec);
  for (int i = 0; i < 3; i++) {
    if (i < 3 - gameMisses) gfx->fillCircle(180 + i * 28, 104, 6, UI_BAR_BAD);
    else gfx->drawCircle(180 + i * 28, 104, 6, UI_TRACK);
  }

  if (pmd.loaded) {
    uint8_t act = gamePetVX > 0.1f ? PMD_WALKR : gamePetVX < -0.1f ? PMD_WALKL : PMD_IDLE;
    if (!pmd.has(act)) act = PMD_IDLE;
    drawPmdAct(act, (int)gamePetX, 394, millis(), true, false, 3);
  } else if (mon.loaded) {
    int s = (mon.h * 2 > 130) ? 1 : 2;
    int w = mon.w * s, h = mon.h * s;
    uint16_t fm = mon.frameMs ? mon.frameMs : 100;
    uint16_t fi = (millis() / fm) % mon.frames;
    const uint8_t *fr = mon.data + (uint32_t)fi * mon.w * mon.h;
    int px = (int)gamePetX - w / 2, py = 394 - h;
    for (int r = 0; r < mon.h; r++) {
      const uint8_t *row = fr + r * mon.w;
      forRuns(mon.w, [&](int c) { return indexedKey(row[c], false); },
              [&](int c0, int len, int k) { gfx->fillRect(px + c0 * s, py + r * s, len * s, s, mon.pal[k]); });
    }
  }

  // anillo de impacto que se expande y desvanece (feedback suave del golpe)
  uint32_t ht = millis() - hitTime;
  if (hitTime && ht < 260) {
    int rad = 22 + (int)(ht / 6);
    gfx->drawCircle((int)hitX, (int)hitY, rad, C565(0xff, 0xe7, 0x9f));
    gfx->drawCircle((int)hitX, (int)hitY, rad - 2, C565(0xff, 0xd9, 0x8a));
  }

  // la baya
  gfx->fillCircle((int)ballX, (int)ballY, 16, C565(0xd8, 0x3a, 0x3a));
  gfx->fillCircle((int)ballX - 5, (int)ballY - 5, 4, C565(0xff, 0xa0, 0xa0));
  gfx->fillRoundRect((int)ballX - 3, (int)ballY - 24, 14, 8, 3, C565(0x3c, 0x8a, 0x4c));
  if (millis() - gameStartT < 3000) {
    gfx->setTextColor(ink);
    setSize(2);
    setCur(centerX(T(S_TILT_HINT), 2), 136);
    printT(T(S_TILT_HINT));
  }

  gfx->flush();
}

// ---------- pokerun ----------
void startRun() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony || !pmd.loaded) { sfxPlay(SFX_DENY); return; }
  runStart(run, esp_random());
  runOpen = true;
  runJump = false;
  runNewHi = false;
  runOverT = 0;
  runFrames = 0;
  runLast = millis();
  uint8_t fit = pmdFitScale(pmd, PMD_IDLE, 90, 90);  // los grandes no deben tapar lo que viene
  runScale = fit > 3 ? 3 : fit;
}

void closeRun() { runOpen = false; runBot = false; }

void runTap() {
  if (runOverT && millis() - runOverT > 800) closeRun();  // tiempo para leer el resultado
}

// bosque con dos capas de pinos que se mueven mas despacio que el suelo
void drawForest(float dist, bool night) {
  uint16_t dark = C565(0x0c, 0x12, 0x24);
  uint16_t top = C565(0xa8, 0xdc, 0xd8), bot = C565(0xe2, 0xf2, 0xc8);
  uint16_t farC = C565(0x6f, 0xa8, 0x7c), nearC = C565(0x35, 0x7a, 0x52), trunk = C565(0x5a, 0x40, 0x30);
  uint16_t grass = C565(0x5c, 0xa8, 0x4c), tuft = C565(0x3f, 0x88, 0x3c), dirt = C565(0x7a, 0x56, 0x3a);
  if (night) {
    top = lerp565(top, dark, 13, 16); bot = lerp565(bot, dark, 11, 16);
    farC = lerp565(farC, dark, 10, 16); nearC = lerp565(nearC, dark, 11, 16); trunk = lerp565(trunk, dark, 10, 16);
    grass = lerp565(grass, dark, 9, 16); tuft = lerp565(tuft, dark, 9, 16); dirt = lerp565(dirt, dark, 9, 16);
  }
  for (int y = 0; y < RUN_GROUND_Y; y += 10)
    gfx->fillRect(0, y, 466, 10, lerp565(top, bot, y, RUN_GROUND_Y));

  gfx->fillRect(0, 286, 466, RUN_GROUND_Y - 286, farC);
  for (int x = -((int)(dist * 0.2f) % 92) - 46; x < 520; x += 92) {
    gfx->fillTriangle(x, 176, x - 34, 250, x + 34, 250, farC);
    gfx->fillTriangle(x, 212, x - 44, 290, x + 44, 290, farC);
  }
  for (int x = -((int)(dist * 0.45f) % 190) - 95; x < 560; x += 190) {
    gfx->fillRect(x - 9, 270, 18, RUN_GROUND_Y - 270, trunk);
    gfx->fillTriangle(x, 150, x - 42, 226, x + 42, 226, nearC);
    gfx->fillTriangle(x, 190, x - 56, 280, x + 56, 280, nearC);
  }

  gfx->fillRect(0, RUN_GROUND_Y, 466, 14, grass);
  gfx->fillRect(0, RUN_GROUND_Y + 14, 466, 466 - RUN_GROUND_Y - 14, dirt);
  for (int x = -((int)dist % 64); x < 480; x += 64) {
    gfx->fillRect(x, RUN_GROUND_Y - 5, 5, 8, tuft);
    gfx->fillRect(x + 28, RUN_GROUND_Y + 6, 12, 4, tuft);
  }
}

void drawRunRock(int x, int w, int h, bool night) {
  uint16_t c = night ? C565(0x5c, 0x62, 0x70) : C565(0x8c, 0x8c, 0x90), hi = night ? C565(0x7c, 0x84, 0x92) : C565(0xbc, 0xbc, 0xc0);
  gfx->fillRoundRect(x, RUN_GROUND_Y - h, w, h + 4, 10, c);
  gfx->fillRoundRect(x + 6, RUN_GROUND_Y - h + 4, w - 16, 8, 4, hi);
}

void drawRunObstacle(const RunObstacle &o, bool night) {
  int x = (int)o.x, w = runObstacleW(o.kind), h = runObstacleH(o.kind);
  if (o.kind == RUN_BUSH) {
    uint16_t c = night ? C565(0x1c, 0x5a, 0x34) : C565(0x24, 0x7c, 0x3c), hi = night ? C565(0x2c, 0x74, 0x44) : C565(0x3c, 0xa0, 0x50);
    gfx->fillCircle(x + 12, RUN_GROUND_Y - 13, 14, c);
    gfx->fillCircle(x + w - 12, RUN_GROUND_Y - 13, 14, c);
    gfx->fillCircle(x + w / 2, RUN_GROUND_Y - h + 16, 17, c);
    gfx->fillRect(x + 6, RUN_GROUND_Y - 14, w - 12, 17, c);
    gfx->fillCircle(x + w / 2 - 5, RUN_GROUND_Y - h + 12, 6, hi);
    gfx->fillCircle(x + 11, RUN_GROUND_Y - 16, 3, C565(0xd8, 0x3a, 0x3a));
    gfx->fillCircle(x + w - 13, RUN_GROUND_Y - 20, 3, C565(0xd8, 0x3a, 0x3a));
  } else if (o.kind == RUN_DOUBLE) {
    drawRunRock(x, 36, h, night);
    drawRunRock(x + w - 36, 36, h, night);
  } else {
    drawRunRock(x, w, h, night);
  }
}

void renderRun() {
  if (pet.ceremony || !pmd.loaded) { closeRun(); return; }
  uint32_t now = millis();
  if (runOverT && now - runOverT > 4000) { closeRun(); return; }

  if (!run.dead) {
    bool jump = runJump || (runBot && runBotWantsJump(run));
    runJump = false;
    runStep(run, (now - runLast) / 1000.0f, jump);
    runLast = now;
    runFrames++;
    if (run.jumped) sfxPlay(SFX_TAP);
    if (run.dead) {
      uint16_t secs = (uint16_t)run.t;
      runNewHi = secs > pet.runHi;
      pet.runResult(secs);  // el premio y el record solo al terminar de verdad
      sfxPlay(runNewHi ? SFX_MEDAL : SFX_DENY);
      runOverT = millis();
      Serial.printf("pokerun: %u s, %lu frames\n", secs, (unsigned long)runFrames);
    }
  }

  int hh = sceneHour();
  bool night = hh < 6 || hh >= 20;
  drawForest(run.dist, night);
  for (auto &o : run.ob)
    if (o.on) drawRunObstacle(o, night);

  uint8_t act = PMD_WALKR;
  if (run.dead) act = PMD_HURT;
  if (!pmd.has(act)) act = PMD_IDLE;
  gPmdForceScale = runScale;
  drawPmdAct(act, RUN_PET_X, RUN_GROUND_Y + 2 - (int)run.h, now, true, false, 3);
  gPmdForceScale = 0;

  uint16_t ink = night ? UI_WHITE : UI_INK;
  char b[24];
  snprintf(b, sizeof(b), "%us", (unsigned)run.t);
  gfx->setTextColor(ink);
  if (runOverT) {
    setSize(5);
    setCur(centerX(b, 5), 62);
    printT(b);
    setSize(2);
    if (runNewHi) {
      gfx->setTextColor(UI_BAR_WARN);
      setCur(centerX(T(S_NEW_RECORD), 2), 112);
      printT(T(S_NEW_RECORD));
    } else {
      snprintf(b, sizeof(b), T(S_RECORD_FMT), pet.runHi);
      setCur(centerX(b, 2), 112);
      printT(b);
    }
    const char *msg = run.t >= 30 ? T(S_GREAT_JOY) : T(S_PLUS_JOY);
    gfx->setTextColor(ink);
    setCur(centerX(msg, 2), 136);
    printT(msg);
  } else {
    setSize(4);
    setCur(centerX(b, 4), 44);
    printT(b);
    setSize(2);
    snprintf(b, sizeof(b), T(S_REC_FMT), pet.runHi);
    setCur(centerX(b, 2), 88);
    printT(b);
    if (run.t < 3) {
      setCur(centerX(T(S_TAP_JUMP), 2), 118);
      printT(T(S_TAP_JUMP));
    }
  }
  gfx->flush();
}

// ---------- ficha del bicho (deslizar vertical) ----------

// x donde arrancan las barras de la ficha. Estaba fijo en 150, que daba de sobra
// para etiquetas latinas de 3 caracteres pero no para las japonesas, mas anchas:
// こうげき se metia dentro de la barra. Se calcula a partir de la etiqueta mas
// larga para que las cuatro barras sigan alineadas en cualquier idioma, con el
// valor original como suelo (asi en los idiomas latinos no cambia nada).
// Requiere tener ya puesto el tamano de texto 2, porque textW() lo necesita.
static int statBarX() {
  const StrId ids[] = { S_STAT_ATK, S_STAT_DEF, S_STAT_SPE, S_STAT_WGT, S_VIN };
  int ancho = 0;
  for (StrId id : ids) {
    int w = textW(T(id), 2);
    if (w > ancho) ancho = w;
  }
  int x = 96 + ancho + 12;   // 12 px de aire entre etiqueta y barra
  return x < 150 ? 150 : x;
}

void drawCardStat(int y, const char *label, uint16_t val, uint16_t maxBar, uint16_t color) {
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(96, y);
  printT(label);
  char num[8];
  snprintf(num, sizeof(num), "%u", val);
  setCur(330, y);
  printT(num);
  int bx = statBarX();
  int bw = 310 - bx;   // la barra siempre acaba en 310, dejando aire hasta el numero
  int fw = (int)val * bw / maxBar;
  if (fw > bw) fw = bw;
  gfx->fillRoundRect(bx, y + 2, bw, 11, 3, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(bx, y + 2, fw, 11, 3, color);
}

// ---------- ajuste de hora en pantalla (deslizar abajo) ----------
// El usuario pone su hora LOCAL a ojo; el firmware la usa tal cual, asi que
// no hay que gestionar zona horaria. Preserva el dia (no rompe racha/edad).

// copia la hora actual al editor. El reloj va primero: lastSeenEpoch solo se anota cada 30 s.
void clockLoadNow() {
  uint32_t e = rtcEpoch();
  if (!e) e = pet.lastSeenEpoch;
  clockH = (e / 3600) % 24;
  clockM = (e / 60) % 60;
  clockEdited = false;
}

void openClock() {
  clockLoadNow();
  clockPage = 0;
  clockOpen = true;
}

void applyClock() {
  uint32_t base = pet.lastSeenEpoch ? pet.lastSeenEpoch : rtcEpoch();
  uint32_t e = (base / 86400) * 86400 + (uint32_t)clockH * 3600 + (uint32_t)clockM * 60;
  rtcSetEpoch(e);
  pet.setClock(e);
  clockOpen = false;
}

void drawClockBtn(int x, int y, const char *l) {
  gfx->fillRoundRect(x, y, 58, 58, 12, UI_WHITE);
  gfx->drawRoundRect(x, y, 58, 58, 12, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(4);
  setCur(x + 17, y + 15);
  printT(l);
}

// pildoras de idioma centradas en y; rellena la activa
#define LANG_PILL_Y 296
#define LANG_PILL_H 30
#define LANG_PILL_X 336          // pildora de idioma (cicla LANG_COUNT al tocar)
#define LANG_PILL_W 96
static const char *const LANG_CODES[LANG_COUNT] = { "ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO" };

#ifdef BOARD_KNOMI_V2
// puntos de pagina de la pantalla de hora/ajustes
void drawClockDots() {
  gfx->fillCircle(CX - 9, 24, 4, clockPage == 0 ? UI_INK : UI_TRACK);
  gfx->fillCircle(CX + 9, 24, 4, clockPage == 1 ? UI_INK : UI_TRACK);
}

// tecla del teclado de la clave en esas coordenadas, -1 si no hay
int wifiKeyAt(int16_t x, int16_t y) {
  if (x < KB_X || y < KB_Y) return -1;
  int col = (x - KB_X) / KB_W, row = (y - KB_Y) / KB_H;
  if (col >= KB_COLS || row >= 5) return -1;
  return row * KB_COLS + col;
}

#define WIFI_ROWS 5
#define WIFI_ROW_Y 88
#define WIFI_ROW_H 48

void settingsTap(int16_t x, int16_t y) {
  switch (wifiSetupState()) {
    case WS_OFF:
      if (x >= 96 && x <= 370 && y >= 236 && y <= 284) {
        sfxPlay(SFX_TAP);
        wifiListPage = 0;
        wifiSetupStart();
      }
      break;
    case WS_LIST: {
      int pages = (wifiSetupNetCount() + WIFI_ROWS - 1) / WIFI_ROWS;
      if (y >= WIFI_ROW_Y && y < WIFI_ROW_Y + WIFI_ROWS * WIFI_ROW_H) {
        sfxPlay(SFX_TAP);
        wifiKbLayer = 0;
        wifiSetupPick(wifiListPage * WIFI_ROWS + (y - WIFI_ROW_Y) / WIFI_ROW_H);
      } else if (y >= 334 && y <= 376) {
        if (x < 170) { if (wifiListPage > 0) wifiListPage--; }
        else if (x > 296) { if (wifiListPage + 1 < pages) wifiListPage++; }
        else { wifiListPage = 0; wifiSetupRescan(); }
      } else if (y > 376) {
        wifiSetupStop();
      }
      break;
    }
    case WS_PASS: {
      if (y < 80) { wifiSetupBackToList(); break; }  // "< red": volver a la lista
      int i = wifiKeyAt(x, y);
      if (i < 0) break;
      if (i < KB_CHAR_KEYS) {
        char c = kbLayerChar(wifiKbLayer, i);
        if (c) wifiSetupType(c);
      } else if (i == KB_CHAR_KEYS) {
        wifiKbLayer = (wifiKbLayer + 1) % KB_LAYER_COUNT;
      } else if (i == KB_CHAR_KEYS + 1) {
        wifiSetupDelete();
      } else if (wifiSetupCanSubmit()) {
        sfxPlay(SFX_TAP);
        wifiSetupSubmit();
      } else {
        sfxPlay(SFX_DENY);  // menos de 8 caracteres
      }
      break;
    }
    default: break;
  }
}

void renderWifiList() {
  setSize(2);
  gfx->setTextColor(UI_INK);
  setCur(centerX("Wi-Fi", 2), 56);
  printT("Wi-Fi");
  int n = wifiSetupNetCount();
  int pages = (n + WIFI_ROWS - 1) / WIFI_ROWS;
  if (pages == 0) pages = 1;
  if (wifiListPage >= pages) wifiListPage = pages - 1;
  for (int r = 0; r < WIFI_ROWS; r++) {
    int idx = wifiListPage * WIFI_ROWS + r;
    if (idx >= n) break;
    int y = WIFI_ROW_Y + r * WIFI_ROW_H;
    gfx->fillRoundRect(62, y, 342, WIFI_ROW_H - 6, 8, UI_WHITE);
    gfx->drawRoundRect(62, y, 342, WIFI_ROW_H - 6, 8, UI_INK);
    char line[28];
    strlcpy(line, wifiSetupNet(idx), sizeof(line));
    gfx->setTextColor(UI_INK);
    setCur(centerX(line, 2), y + 13);
    printT(line);
  }
  // paginas y volver a buscar
  gfx->setTextColor(UI_INK);
  gfx->fillRoundRect(96, 334, 70, 40, 10, wifiListPage > 0 ? UI_WHITE : UI_BG_DAY);
  gfx->drawRoundRect(96, 334, 70, 40, 10, UI_INK);
  setCur(125, 347);
  printT("<");
  gfx->fillRoundRect(176, 334, 114, 40, 10, UI_WHITE);
  gfx->drawRoundRect(176, 334, 114, 40, 10, UI_INK);
  setCur(centerX("SCAN", 2), 347);
  printT("SCAN");
  gfx->fillRoundRect(300, 334, 70, 40, 10, wifiListPage + 1 < pages ? UI_WHITE : UI_BG_DAY);
  gfx->drawRoundRect(300, 334, 70, 40, 10, UI_INK);
  setCur(329, 347);
  printT(">");
  gfx->fillRoundRect(150, 384, 166, 40, 10, UI_TRACK);
  setCur(centerX(T(S_CANCEL), 2), 396);
  printT(T(S_CANCEL));
}

void renderWifiKeyboard() {
  gfx->setTextColor(UI_INK);
  setSize(2);
  char title[28];
  snprintf(title, sizeof(title), "< %.24s", wifiSetupSsid());
  setCur(centerX(title, 2), 54);
  printT(title);
  gfx->fillRoundRect(58, 84, 350, 40, 8, UI_WHITE);
  gfx->drawRoundRect(58, 84, 350, 40, 8, UI_INK);
  // se ve lo escrito: en una pantalla tan pequena, sin verlo es imposible acertar. Solo el final.
  const char *pass = wifiSetupPass();
  size_t len = strlen(pass);
  setCur(68, 98);
  printT(len ? (len > 27 ? pass + len - 27 : pass) : "_");

  bool ready = wifiSetupCanSubmit();
  for (int i = 0; i < 30; i++) {
    int x = KB_X + (i % KB_COLS) * KB_W, y = KB_Y + (i / KB_COLS) * KB_H;
    char c = i < KB_CHAR_KEYS ? kbLayerChar(wifiKbLayer, i) : 0;
    if (i < KB_CHAR_KEYS && !c) continue;
    uint16_t bg = i < KB_CHAR_KEYS ? UI_WHITE : (i == 29 ? (ready ? UI_BAR_OK : UI_TRACK) : UI_BAR_WARN);
    if (i == wifiKbHover) bg = TYPE_ACCENT[TY_AGUA];
    gfx->fillRoundRect(x, y, KB_W - 6, KB_H - 6, 6, bg);
    gfx->drawRoundRect(x, y, KB_W - 6, KB_H - 6, 6, UI_INK);
    gfx->setTextColor(UI_INK);
    if (i < KB_CHAR_KEYS && c != ' ') {
      setCur(x + KB_W / 2 - 9, y + KB_H / 2 - 10);
      printT(c);
    } else {
      const char *lab = i < KB_CHAR_KEYS ? "sp" : i == KB_CHAR_KEYS ? KB_LAYER_NEXT[wifiKbLayer] : i == 28 ? "<-" : "OK";
      setCur(x + (KB_W - 6 - (int)strlen(lab) * 12) / 2, y + KB_H / 2 - 10);
      printT(lab);
    }
  }

  // la tecla bajo el dedo, en grande y por encima del campo: el dedo tapa la de verdad
  if (wifiKbHover >= 0) {
    char big[4] = "";
    if (wifiKbHover < KB_CHAR_KEYS) {
      char c = kbLayerChar(wifiKbLayer, wifiKbHover);
      if (c == ' ') strlcpy(big, "sp", sizeof(big));
      else if (c) big[0] = c;
    } else {
      strlcpy(big, wifiKbHover == KB_CHAR_KEYS ? KB_LAYER_NEXT[wifiKbLayer] : wifiKbHover == 28 ? "<-" : "OK", sizeof(big));
    }
    if (big[0]) {
      gfx->fillRoundRect(CX - 62, 34, 124, 92, 14, TYPE_ACCENT[TY_AGUA]);
      gfx->setTextColor(UI_WHITE);
      setSize(6);
      setCur(CX - (int)strlen(big) * 18, 54);
      printT(big);
    }
  }
}

void renderSettings() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  WifiSetupState ws = wifiSetupState();
  if (ws == WS_LIST) { renderWifiList(); gfx->flush(); return; }
  if (ws == WS_PASS) { renderWifiKeyboard(); gfx->flush(); return; }
  if (ws != WS_OFF) {
    const char *msg = ws == WS_SCAN ? T(S_WIFI_SEARCH) : ws == WS_OK ? T(S_WIFI_SAVED)
                      : ws == WS_FAIL ? T(S_WIFI_KEPT) : T(S_WIFI_TRYING);
    gfx->setTextColor(ws == WS_OK ? UI_BAR_OK : ws == WS_FAIL ? UI_BAR_BAD : UI_INK);
    setSize(2);
    setCur(centerX(msg, 2), 224);
    printT(msg);
    gfx->flush();
    return;
  }

  drawClockDots();
  setSize(3);
  setCur(centerX(T(S_SETTINGS), 3), 56);
  printT(T(S_SETTINGS));
  setSize(2);
  setCur(centerX("Wi-Fi", 2), 126);
  printT("Wi-Fi");
  const char *ssid = clockWifiSsid();
  setCur(centerX(ssid[0] ? ssid : "---", 2), 156);
  printT(ssid[0] ? ssid : "---");
  uint8_t st = clockWifiStatus();
  gfx->setTextColor(st == 1 ? UI_BAR_OK : st == 2 ? UI_BAR_BAD : UI_TRACK);
  const char *stText = st == 1 ? T(S_WIFI_SYNCED) : st == 2 ? T(S_WIFI_NOCONN) : T(S_WIFI_TRYING);
  setCur(centerX(stText, 2), 186);
  printT(stText);
  gfx->fillRoundRect(96, 236, 274, 48, 12, TYPE_ACCENT[TY_AGUA]);
  gfx->setTextColor(UI_WHITE);
  setCur(centerX(T(S_WIFI_CHANGE), 2), 252);
  printT(T(S_WIFI_CHANGE));
  gfx->setTextColor(UI_TRACK);
  setCur(centerX(T(S_CLOCK_CANCEL), 2), 410);
  printT(T(S_CLOCK_CANCEL));
  gfx->flush();
}
#endif

void renderClock() {
#ifdef BOARD_KNOMI_V2
  if (clockPage == 1) {
    renderSettings();
    return;
  }
#endif
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_SET_TIME), 3), 44);
  printT(T(S_SET_TIME));
#ifdef BOARD_KNOMI_V2
  drawClockDots();
#endif

  char t[8];
  snprintf(t, sizeof(t), "%02d:%02d", clockH, clockM);
  setSize(7);
  setCur(CX - 105, 108);
  printT(t);

  drawClockBtn(104, 190, "-");  // hora -
  drawClockBtn(170, 190, "+");  // hora +
  drawClockBtn(252, 190, "-");  // min -
  drawClockBtn(318, 190, "+");  // min +
  setSize(2);
  gfx->setTextColor(UI_TRACK);
  setCur(120, 256);
  printT(T(S_HOUR));
  setCur(276, 256);
  printT(T(S_MIN));

  // interruptor de sonido (izquierda de la fila de idioma)
  bool snd = audioEnabled();
  const char *sl = snd ? T(S_SND_ON) : T(S_SND_OFF);
  gfx->fillRoundRect(34, LANG_PILL_Y, 96, LANG_PILL_H, 8, snd ? UI_BAR_OK : UI_WHITE);
  gfx->drawRoundRect(34, LANG_PILL_Y, 96, LANG_PILL_H, 8, UI_INK);
  gfx->setTextColor(snd ? UI_BG_DAY : UI_INK);
  setSize(2);
  setCur(34 + (96 - textW(sl, 2)) / 2, LANG_PILL_Y + 8);
  printT(sl);

  // selector de idioma: una pildora que cicla los 6 idiomas al tocar
  gfx->fillRoundRect(LANG_PILL_X, LANG_PILL_Y, LANG_PILL_W, LANG_PILL_H, 8, UI_WHITE);
  gfx->drawRoundRect(LANG_PILL_X, LANG_PILL_Y, LANG_PILL_W, LANG_PILL_H, 8, UI_INK);
  char lp[10];
  snprintf(lp, sizeof(lp), "%s >", LANG_CODES[gLang]);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(LANG_PILL_X + (LANG_PILL_W - textW(lp, 2)) / 2, LANG_PILL_Y + 8);
  printT(lp);

  gfx->fillRoundRect(133, 340, 200, 48, 14, UI_BAR_OK);
  gfx->setTextColor(UI_BG_DAY);
  setSize(3);
  setCur(CX - 18, 352);
  printT("OK");

  gfx->setTextColor(UI_TRACK);
  setSize(2);
  setCur(centerX(T(S_CLOCK_CANCEL), 2), 410);
  printT(T(S_CLOCK_CANCEL));

  // version del firmware (discreta, abajo del todo)
  char ver[20];
  snprintf(ver, sizeof(ver), "TamaPoke v%s", FW_VERSION);
  setSize(1);
  setCur(centerX(ver, 1), 436);
  printT(ver);
  gfx->flush();
}

void clockTap(int16_t x, int16_t y) {
#ifdef BOARD_KNOMI_V2
  if (clockPage == 1) {
    settingsTap(x, y);
    return;
  }
#endif
  if (y >= 190 && y <= 248) {  // fila de botones +/-
    clockEdited = true;
    if (x >= 104 && x < 162) clockH = (clockH + 23) % 24;
    else if (x >= 170 && x < 228) clockH = (clockH + 1) % 24;
    else if (x >= 252 && x < 310) clockM = (clockM + 59) % 60;
    else if (x >= 318 && x < 376) clockM = (clockM + 1) % 60;
    return;
  }
  if (y >= LANG_PILL_Y && y <= LANG_PILL_Y + LANG_PILL_H) {
    if (x >= 34 && x < 130) {                  // interruptor de sonido
      audioSetEnabled(!audioEnabled());
      if (audioEnabled()) sfxPlay(SFX_TAP);    // confirma al encender
      return;
    }
    if (x >= LANG_PILL_X && x < LANG_PILL_X + LANG_PILL_W) {  // cicla idioma
      setLang((Lang)((gLang + 1) % LANG_COUNT));
      applyLangFont();  // la fuente cambia con el idioma
      sfxPlay(SFX_TAP);
      return;
    }
  }
  if (y >= 340 && y <= 388 && x >= 133 && x <= 333) { applyClock(); return; }
}

// llama + numero de racha arriba a la izquierda
void drawStreakBadge() {
  if (pet.streak < 1) return;
  int x = 26, y = 16;
  gfx->fillTriangle(x + 8, y, x + 1, y + 17, x + 15, y + 17, UI_BAR_BAD);
  gfx->fillTriangle(x + 8, y + 7, x + 4, y + 17, x + 12, y + 17, UI_BAR_WARN);
  char s[6];
  snprintf(s, sizeof(s), "%u", pet.streak);
  gfx->setTextColor(inkColor());
  setSize(2);
  setCur(x + 22, y + 2);
  printT(s);
}

// banner temporal: medalla nueva o hito de racha
void drawCelebration() {
  const char *l1 = nullptr, *l2 = nullptr;
  // 32 y no 20: "%u にちれんぞく！" ya pasa de 20 con una sola cifra, y snprintf
  // cortaba a mitad de un caracter de 3 bytes.
  char buf[32];
  if (pet.showMedal()) {
    for (int i = 0; i < MED_COUNT; i++)
      if (pet.newMedal & (1 << i)) { l2 = medalName(i); break; }
    l1 = T(S_MEDAL_BANNER);
  } else if (pet.showMilestone()) {
    snprintf(buf, sizeof(buf), T(S_STREAK_DAYS_FMT), pet.streak);
    l1 = T(S_GREAT);
    l2 = buf;
  }
  if (!l1) return;
  gfx->fillRoundRect(73, 150, 320, 96, 16, UI_BAR_WARN);
  gfx->drawRoundRect(73, 150, 320, 96, 16, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(l1, 3), 176);
  printT(l1);
  setSize(2);
  setCur(centerX(l2, 2), 212);
  printT(l2);
}

// medallas en la ficha: badge con etiqueta, color si conseguida
void drawMedalBadge(int x, int y, int i) {
  bool got = pet.hasMedal(1 << i);
  gfx->fillRoundRect(x, y, 100, 24, 6, got ? UI_BAR_OK : UI_TRACK);
  if (!got) gfx->drawRoundRect(x, y, 100, 24, 6, UI_TRACK);
  gfx->setTextColor(got ? UI_BG_DAY : 0x9492);
  setSize(2);
  setCur(x + (100 - textW(medalLabel(i), 2)) / 2, y + 5);
  printT(medalLabel(i));
}

// pagina 0: perfil (retrato grande, identidad, racha, vinculo, baya)
void renderCardProfile() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  char head[32];
  snprintf(head, sizeof(head), T(S_NAME_FMT), pet.shiny ? "*" : "", nm, pet.level());
  gfx->setTextColor(d.accent);
  // auto-encoge: a tamano 3 los nombres largos no caben en la franja estrecha de
  // arriba de la pantalla redonda, asi que se cortaban por el borde. El ancho se
  // MIDE con textW(): strlen() cuenta bytes, y en una fila UTF-8 no hay un byte
  // por caracter ni todos miden lo mismo. Con la fuente clasica textW() devuelve
  // strlen()*6*size, o sea exactamente la cuenta anterior: no mueve un pixel en
  // los seis idiomas latinos (11 caracteres a tamano 3 son esos 198 px).
  setSize(3);
  int hts = (textW(head, 3) <= 198) ? 3 : 2;
  setSize(hts);
  setCur(centerX(head, hts), hts == 3 ? 34 : 40);
  printT(head);
  if (pet.nick[0]) {  // especie real bajo el apodo
    const char *sp = dexName(pet.speciesId);
    char par[32];
    snprintf(par, sizeof(par), "(%s)", sp);
    gfx->setTextColor(UI_TRACK);
    setSize(2);
    setCur(centerX(par, 2), 64);   // medido, no contado en bytes
    printT(par);
  }

  // retrato grande animado
  if (pmd.loaded) drawPmdAct(PMD_IDLE, CX, 206, millis(), true, false, 4);

  // racha con llama
  int sx = 138, sy = 224;
  gfx->fillTriangle(sx + 8, sy, sx + 1, sy + 18, sx + 15, sy + 18, UI_BAR_BAD);
  gfx->fillTriangle(sx + 8, sy + 7, sx + 4, sy + 18, sx + 12, sy + 18, UI_BAR_WARN);
  char rl[40];  // en japones pierde cifras con 30 a partir de 100 dias
  snprintf(rl, sizeof(rl), T(S_STREAK_FMT), pet.streak, pet.bestStreak);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(sx + 24, sy + 2);
  printT(rl);

  drawCardStat(258, T(S_VIN), pet.bond, 100, C565(0xd4, 0x52, 0x7e));

  const char *berry = !pet.berryKnown ? T(S_BERRY_UNK)
                      : pet.lovesBerry(0) ? T(S_BERRY_RED)
                      : pet.lovesBerry(1) ? T(S_BERRY_BLUE)
                                          : T(S_BERRY_GREEN);
  char info[48];
  snprintf(info, sizeof(info), T(S_INFO_FMT), berry,
           (unsigned long)(pet.ageMinutes / 1440));
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(info, 2), 296);
  printT(info);

  gfx->setTextColor(UI_TRACK);
  setCur(centerX(T(S_RENAME_HINT), 2), 332);
  printT(T(S_RENAME_HINT));
}

// pagina 1: combate (4 barras + boton de entrenar)
void renderCardStats() {
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_BATTLE), 3), 48);
  printT(T(S_BATTLE));

  drawCardStat(118, T(S_STAT_ATK), pet.atkStat(), 260, UI_BAR_BAD);
  drawCardStat(160, T(S_STAT_DEF), pet.defStat(), 260, 0x4C98);
  drawCardStat(202, T(S_STAT_SPE), pet.speStat(), 260, UI_BAR_WARN);
  drawCardStat(244, T(S_STAT_WGT), pet.weight, 100, 0xB3C8);

#ifndef BOARD_KNOMI_V2  // en el Knomi el combate se abre con el boton fisico
  gfx->fillRoundRect(96, 300, 274, 40, 12, UI_BAR_BAD);
  gfx->setTextColor(UI_BG_DAY);
  setSize(2);
  setCur(centerX(T(S_BTL_START), 2), 311);
  printT(T(S_BTL_START));
#endif
  setSize(2);
  char wins[24];
  snprintf(wins, sizeof(wins), T(S_WINS_FMT), pet.battleWins);
  gfx->setTextColor(UI_INK);
  setCur(centerX(wins, 2), 350);
  printT(wins);
}

// pagina 2: medallas con etiqueta descriptiva
void renderCardMedals() {
  int got = 0;
  for (int i = 0; i < MED_COUNT; i++)
    if (pet.hasMedal(1 << i)) got++;
  char head[24];
  snprintf(head, sizeof(head), T(S_MEDALS_FMT), got, MED_COUNT);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(head, 3), 48);
  printT(head);

  for (int i = 0; i < MED_COUNT; i++) {
    int x = 28 + (i % 2) * 206, y = 104 + (i / 2) * 54;
    bool g = pet.hasMedal(1 << i);
    gfx->fillRoundRect(x, y, 196, 44, 10, g ? UI_BAR_OK : UI_TRACK);
    if (g) {  // marca de conseguida
      gfx->fillCircle(x + 22, y + 22, 11, UI_BG_DAY);
      gfx->setTextColor(UI_BAR_OK);
      setSize(2);
      setCur(x + 16, y + 13);
      printT("v");
    }
    gfx->setTextColor(g ? UI_BG_DAY : 0x8410);
    setSize(2);
    setCur(x + 44, y + 14);
    printT(medalDesc(i));
  }
}

// pagina 3: progreso (nivel, evolucion, descuidos) — saca a la luz mecanicas
// que antes eran invisibles (cuanto falta para subir/evolucionar y por que)
void renderCardProgress() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_PROGRESS), 3), 44);
  printT(T(S_PROGRESS));

  // nivel grande
  char lv[10];
  snprintf(lv, sizeof(lv), T(S_LVL_FMT), pet.level());
  setSize(5);
  setCur(centerX(lv, 5), 86);
  printT(lv);

  // barra de progreso al siguiente nivel (1 nivel = 60 min de juego)
  uint8_t into = pet.ageMinutes % MINUTES_PER_LEVEL;
  int bx = 93, bw = 280, by = 158, bh = 22;
  gfx->fillRoundRect(bx, by, bw, bh, 6, UI_TRACK);
  int fw = (bw - 4) * into / MINUTES_PER_LEVEL;
  if (fw > 0) gfx->fillRoundRect(bx + 2, by + 2, fw, bh - 4, 5, UI_BAR_OK);
  char nx[32];
  snprintf(nx, sizeof(nx), T(S_NEXT_LVL_FMT), MINUTES_PER_LEVEL - into, pet.level() + 1);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(nx, 2), by + 32);
  printT(nx);

  // estado de evolucion
  gfx->setTextColor(UI_TRACK);
  setCur(centerX(T(S_EVO_LABEL), 2), 230);
  printT(T(S_EVO_LABEL));
  char evoBuf[32];
  const char *evo;
  uint16_t evoCol = UI_INK;
  if (d.evolvesTo == 0) {
    evo = T(S_FINAL_FORM);
  } else {
    int needed = d.evolveLevel + pet.careMistakes;
    if (pet.level() >= needed) {
      if (pet.lowestStat() >= 40) { evo = T(S_EVO_READY); evoCol = UI_BAR_OK; }
      else { evo = T(S_EVO_BLOCKED); evoCol = UI_BAR_BAD; }
    } else {
      snprintf(evoBuf, sizeof(evoBuf), T(S_EVO_IN_FMT), needed - pet.level());
      evo = evoBuf;
    }
  }
  gfx->setTextColor(evoCol);
  setCur(centerX(evo, 2), 256);
  printT(evo);

  // descuidos (retrasan la evolucion)
  char ms[24];
  snprintf(ms, sizeof(ms), T(S_MISTAKES_FMT), pet.careMistakes);
  gfx->setTextColor(pet.careMistakes > 0 ? UI_BAR_BAD : UI_INK);
  setCur(centerX(ms, 2), 312);
  printT(ms);
}

void renderCard() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  if (cardPage == 0) renderCardProfile();
  else if (cardPage == 1) renderCardStats();
  else if (cardPage == 2) renderCardMedals();
  else renderCardProgress();

  // indicador de 4 paginas + ayuda
  for (int i = 0; i < 4; i++) {
    if (i == cardPage) gfx->fillCircle(194 + i * 26, 374, 5, UI_INK);
    else gfx->drawCircle(194 + i * 26, 374, 4, UI_INK);
  }
  gfx->setTextColor(UI_TRACK);
  setSize(2);
  setCur(centerX(T(S_BACK), 2), 398);
  printT(T(S_BACK));
  gfx->flush();
}

// ---------- teclado para renombrar ----------

static const char KB_KEYS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-";  // 28 + DEL + OK = 30

void openKeyboard() {
  kbOpen = true;
  strncpy(nameBuf, pet.nick, sizeof(nameBuf) - 1);
  nameBuf[sizeof(nameBuf) - 1] = 0;
  nameLen = strlen(nameBuf);
}

void renderKeyboard() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(T(S_NAME), 2), 56);
  printT(T(S_NAME));
  // buffer actual
  gfx->fillRoundRect(83, 84, 300, 40, 8, UI_WHITE);
  gfx->drawRoundRect(83, 84, 300, 40, 8, UI_INK);
  setSize(3);
  setCur(95, 94);
  printT(nameLen ? nameBuf : "_");

  for (int i = 0; i < 30; i++) {
    int x = KB_X + (i % KB_COLS) * KB_W, y = KB_Y + (i / KB_COLS) * KB_H;
    bool special = (i >= 28);
    gfx->fillRoundRect(x, y, KB_W - 6, KB_H - 6, 6, special ? UI_BAR_WARN : UI_WHITE);
    gfx->drawRoundRect(x, y, KB_W - 6, KB_H - 6, 6, UI_INK);
    gfx->setTextColor(UI_INK);
    setSize(2);
    if (i < 28) {
      setCur(x + KB_W / 2 - 9, y + KB_H / 2 - 10);
      printT(KB_KEYS[i]);
    } else {
      const char *lab = (i == 28) ? "<-" : "OK";
      setCur(x + KB_W / 2 - 15, y + KB_H / 2 - 10);
      printT(lab);
    }
  }
  gfx->flush();
}

void keyboardTap(int16_t x, int16_t y) {
  // filtra ANTES de dividir: con x/y menores que KB_X/KB_Y la resta da negativo
  // y la division entera trunca hacia cero (no hacia -inf), asi que col/row
  // saldria 0 en vez de negativo y el filtro de abajo no lo detectaria
  if (x < KB_X || y < KB_Y) return;
  int col = (x - KB_X) / KB_W, row = (y - KB_Y) / KB_H;
  if (col >= KB_COLS || row >= 5) return;
  int i = row * KB_COLS + col;
  if (i >= 30) return;
  if (i == 28) {  // borrar
    if (nameLen) nameBuf[--nameLen] = 0;
  } else if (i == 29) {  // OK
    pet.rename(nameBuf);
    kbOpen = false;
  } else if (nameLen < sizeof(nameBuf) - 1) {
    nameBuf[nameLen++] = KB_KEYS[i];
    nameBuf[nameLen] = 0;
  }
}

// ---------- galeria pokedex ----------

#define GAL_X 73
#define GAL_Y 84
#define GAL_CELL 80

// dibuja una miniatura centrada en su celda; sil=true la pinta en tinta
void drawThumb(const uint8_t *b, int x, int y, int s, bool sil) {
  uint8_t w = b[0], h = b[1], n = b[2];
  const uint8_t *pal = b + 3;
  const uint8_t *d = pal + n * 2;
  int ox = x + (GAL_CELL - w * s) / 2;
  int oy = y + (GAL_CELL - h * s) / 2;
  for (int r = 0; r < h; r++) {
    const uint8_t *row = d + r * w;
    forRuns(w, [&](int c) { return indexedKey(row[c], sil); },
            [&](int c0, int len, int k) {
              uint16_t col = sil ? INK_K : (uint16_t)(pal[k * 2] | (pal[k * 2 + 1] << 8));
              gfx->fillRect(ox + c0 * s, oy + r * s, len * s, s, col);
            });
  }
}

// tipo, evolucion y dato curioso de una especie ya encontrada
void drawDexInfo(int16_t dex, bool withFact = true) {
  const DexEntry &d = DEX_TBL[dex];
  setSize(2);
  const char *ty = TYPE_NAME[d.type];
  int w = textW(ty, 2) + 24;
  gfx->fillRoundRect(CX - w / 2, 246, w, 24, 8, d.accent);
  gfx->setTextColor(UI_WHITE);
  setCur(centerX(ty, 2), 251);
  printT(ty);

  char line[40];
  gfx->setTextColor(UI_INK);
  if (d.evolvesTo) {  // la siguiente forma no se revela hasta encontrarla
    snprintf(line, sizeof(line), "Lv.%u > %s", d.evolveLevel, pet.isRegistered(d.evolvesTo) ? dexName(d.evolvesTo) : "???");
    setCur(centerX(line, 2), 278);
    printT(line);
  } else {
    setCur(centerX(T(S_FINAL_FORM), 2), 278);
    printT(T(S_FINAL_FORM));
  }

  if (!withFact) return;
  gfx->setTextColor(UI_INK);
  const char *p = DEX_FACT[dex];
  for (int y = 304; *p && y <= 376; y += 24) {
    int skip, n = wrapNext(p, 29, skip);
    snprintf(line, sizeof(line), "%.*s", n, p);
    setCur(centerX(line, 2), y);
    printT(line);
    p += skip;
  }
}

void renderGallery() {
  if (galleryDetail) {  // vista detalle: se redibuja siempre (animada)
    gfx->fillScreen(RGB565_BLACK);
    gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
    const DexEntry &d = DEX_TBL[galleryDetail];
    bool reg = pet.isRegistered(galleryDetail);
    char head[32];
    snprintf(head, sizeof(head), "N.%03d %s%s", galleryDetail,
             pet.isShinyRegistered(galleryDetail) ? "*" : "", reg ? dexName(galleryDetail) : "???");
    gfx->setTextColor(reg ? d.accent : UI_INK);
    // auto-encoge nombres largos (no caben a t3), midiendo en vez de contar
    // bytes; 13 caracteres a tamano 3 son los 234 px de antes.
    setSize(3);
    int gts = (textW(head, 3) <= 234) ? 3 : 2;
    setSize(gts);
    setCur(centerX(head, gts), gts == 3 ? 56 : 60);
    printT(head);
    if (reg) {
      if (galleryPmd.loaded) drawPmdActM(galleryPmd, PMD_IDLE, CX, 240, millis(), true, false, 4);
      else if (const uint8_t *t = thumbs.get(galleryDetail)) drawThumb(t, CX - GAL_CELL, 110, 3, false);
      drawDexInfo(galleryDetail);
    } else if (galleryPmd.loaded) {
      drawPmdActM(galleryPmd, PMD_IDLE, CX, 300, 0, true, true, 6);  // silueta estatica (estilo "?")
    } else if (const uint8_t *t = thumbs.get(galleryDetail)) {
      drawThumb(t, CX - GAL_CELL, 135, 4, true);
    }
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(T(S_DETAIL_BACK), 2), 408);
    printT(T(S_DETAIL_BACK));
    gfx->flush();
    return;
  }

  if (!galleryDirty) return;  // la rejilla es estatica
  galleryDirty = false;

  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  char head[24];
  snprintf(head, sizeof(head), T(S_POKEDEX_FMT), pet.registeredCount());
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(head, 3), 36);
  printT(head);

  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      int16_t dex = galleryPage * 16 + r * 4 + c + 1;
      if (dex > 151) break;
      int x = GAL_X + c * GAL_CELL, y = GAL_Y + r * GAL_CELL;
      const uint8_t *t = thumbs.get(dex);
      if (t) {
        drawThumb(t, x, y, 2, !pet.isRegistered(dex));
        if (pet.isShinyRegistered(dex)) {
          gfx->setTextColor(UI_BAR_WARN);
          setSize(2);
          setCur(x + 62, y + 4);
          printT("*");
        }
      } else {
        char num[6];
        snprintf(num, sizeof(num), "%d", dex);
        gfx->setTextColor(UI_TRACK);
        setSize(2);
        setCur(x + 24, y + 32);
        printT(num);
      }
    }
  }
  // puntos de pagina
  for (int i = 0; i < 10; i++) {
    if (i == galleryPage) gfx->fillCircle(170 + i * 14, 436, 4, UI_INK);
    else gfx->drawCircle(170 + i * 14, 436, 3, UI_INK);
  }
  gfx->flush();
}

void galleryTap(int16_t x, int16_t y) {
  if (galleryDetail) {  // volver a la rejilla
    galleryDetail = 0;
    galleryPmd.unload();
    galleryDirty = true;
    return;
  }
  if (y < 72) {  // tocar la cabecera = salir
    galleryOpen = false;
    galleryPmd.unload();
    return;
  }
  // mismo motivo que en el teclado: filtrar antes de dividir
  if (x < GAL_X || y < GAL_Y) return;
  int c = (x - GAL_X) / GAL_CELL, r = (y - GAL_Y) / GAL_CELL;
  if (c > 3 || r > 3) return;
  int16_t dex = galleryPage * 16 + r * 4 + c + 1;
  if (dex > 151) return;
  galleryDetail = dex;
  galleryPmd.load(dex, pet.isShinyRegistered(dex));
}

// ---------- eleccion del huevo ----------
// misma rejilla que la Pokedex, pero sin huecos: solo lo que ya se ha registrado

#define PICK_BTN_Y 316
#define PICK_BTN_H 56
#define PICK_BACK_Y 384
#define PICK_BACK_H 44

void renderEggPick() {
  if (eggPickDetail) {  // animada: se redibuja siempre
    const DexEntry &d = DEX_TBL[eggPickDetail];
    gfx->fillScreen(RGB565_BLACK);
    gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
    gfx->setTextColor(d.accent);
    const char *name = dexName(eggPickDetail);
    setSize(3);
    setCur(centerX(name, 3), 56);
    printT(name);
    if (galleryPmd.loaded) drawPmdActM(galleryPmd, PMD_IDLE, CX, 240, millis(), true, false, 4);
    else if (const uint8_t *t = thumbs.get(eggPickDetail)) drawThumb(t, CX - GAL_CELL, 110, 3, false);
    drawDexInfo(eggPickDetail, false);
    gfx->fillRoundRect(CX - 110, PICK_BTN_Y, 220, PICK_BTN_H, 14, C565(0x1e, 0x7d, 0x3c));
    gfx->setTextColor(UI_WHITE);
    setSize(3);
    setCur(centerX(T(S_CHOOSE_BTN), 3), PICK_BTN_Y + 16);
    printT(T(S_CHOOSE_BTN));
    gfx->fillRoundRect(CX - 80, PICK_BACK_Y, 160, PICK_BACK_H, 12, UI_TRACK);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(T(S_BACK_BTN), 2), PICK_BACK_Y + 14);
    printT(T(S_BACK_BTN));
    gfx->flush();
    return;
  }

  if (!eggPickDirty) return;  // la rejilla es estatica
  eggPickDirty = false;

  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(T(S_CHOOSE_POKEMON), 2), 50);
  printT(T(S_CHOOSE_POKEMON));
  for (int i = 0; i < 16; i++) {
    int16_t dex = pet.registeredAt(eggPickPage * 16 + i);
    if (!dex) break;
    if (const uint8_t *t = thumbs.get(dex)) drawThumb(t, GAL_X + (i % 4) * GAL_CELL, GAL_Y + (i / 4) * GAL_CELL, 2, false);
  }
  int pages = (pet.registeredCount() + 15) / 16;
  if (pages > 1) {
    for (int i = 0; i < pages; i++) {
      int dotX = CX + (i * 2 - (pages - 1)) * 7;
      if (i == eggPickPage) gfx->fillCircle(dotX, 436, 4, UI_INK);
      else gfx->drawCircle(dotX, 436, 3, UI_INK);
    }
  }
  gfx->flush();
}

void eggPickTap(int16_t x, int16_t y) {
  if (eggPickDetail) {
    if (y >= PICK_BTN_Y && y < PICK_BTN_Y + PICK_BTN_H && abs(x - CX) <= 110) {
      pet.chooseEgg(eggPickDetail);
      sfxPlay(SFX_TAP);
    } else if (y >= PICK_BACK_Y && y < PICK_BACK_Y + PICK_BACK_H && abs(x - CX) <= 80) {
      sfxPlay(SFX_TAP);
    } else {
      return;
    }
    eggPickDetail = 0;
    eggPickPage = 0;
    eggPickDirty = true;
    galleryPmd.unload();
    return;
  }
  if (x < GAL_X || y < GAL_Y) return;
  int c = (x - GAL_X) / GAL_CELL, r = (y - GAL_Y) / GAL_CELL;
  if (c > 3 || r > 3) return;
  int16_t dex = pet.registeredAt(eggPickPage * 16 + r * 4 + c);
  if (!dex) return;
  sfxPlay(SFX_TAP);
  eggPickDetail = dex;
  galleryPmd.load(dex, false);  // el shiny sigue siendo sorpresa
}

void drawBattery() {
  int pc = batPercent();
  if (pc < 0) return;  // sin bateria conectada
  int x = CX - 14, y = 12, w = 24, h = 11;
  bool charging = batCharging();
  uint16_t col = charging ? UI_BAR_OK
                 : (pc >= 40) ? inkColor()
                 : (pc >= 15) ? UI_BAR_WARN
                              : UI_BAR_BAD;
  gfx->drawRoundRect(x, y, w, h, 2, col);
  gfx->fillRect(x + w, y + 3, 3, 5, col);  // borne
  if (charging) {
    // rayo de carga (zigzag) en vez de la barra de nivel
    uint16_t bolt = C565(0xff, 0xd9, 0x4a);
    int bx = x + w / 2;
    gfx->fillTriangle(bx + 3, y + 1, bx - 4, y + 6, bx + 1, y + 6, bolt);
    gfx->fillTriangle(bx - 1, y + 5, bx + 4, y + 5, bx - 3, y + 10, bolt);
  } else {
    int fw = (w - 4) * pc / 100;
    if (fw > 0) gfx->fillRect(x + 2, y + 2, fw, h - 4, col);
  }
}

void drawHeader(const char *name, uint16_t nameColor, const char *msg) {
  drawBattery();
  gfx->setTextColor(nameColor);
  setSize(3);
  setCur(centerX(name, 3), 52);
  printT(name);
  gfx->setTextColor(inkColor());
  setSize(2);
  setCur(centerX(msg, 2), 90);
  printT(msg);
}

// animacion de la ceremonia (10s): despedida = reverencia con corazones y se
// aleja caminando; escapada = se asusta y sale corriendo. Sustituye al idle.
void drawCeremony() {
  if (!pmd.loaded) { drawPet(); return; }  // respaldo si no hay sprite PMD
  uint32_t now = millis();
  float t = pet.ceremonyT();               // 0..1 a lo largo de los 10s
  bool panic = (pet.ceremony == CER_RUNAWAY);
  int x = CX, y = PET_GROUND;
  uint8_t act = PMD_IDLE;

  if (panic) {
    // final triste: penumbra azulada + lluvia
    for (int i = 0; i < 46; i++) {
      int rx = (i * 47 + now / 3) % 466;
      int ry = (i * 91 + now / 2) % 470;
      gfx->drawLine(rx, ry, rx - 3, ry + 12, C565(0x6a, 0x84, 0xb0));
    }
    bool fade = false;
    if (t < 0.30f) {                       // cabizbajo, temblando
      act = pmd.has(PMD_HURT) ? PMD_HURT : PMD_IDLE;
      x = CX + (int)(4 * sinf(now * 0.04f));
    } else {                               // se aleja despacio y se desvanece
      act = pmd.has(PMD_WALKL) ? PMD_WALKL : PMD_IDLE;
      x = CX - (int)(((t - 0.30f) / 0.70f) * (CX + 120));
      fade = (t > 0.6f) && ((now / 160) % 2 == 0);  // parpadea hacia la silueta
    }
    drawPmdAct(act, x, y, now, true, fade, 5);  // fade=silueta: se difumina al irse
    // lagrima cayendo del bicho
    if (t < 0.55f) {
      int ty = y - 150 + (int)((now / 6) % 40);
      gfx->fillRect(x + 6, ty, 3, 6, C565(0x9a, 0xc4, 0xe8));
    }
    return;
  }

  // despedida epica: halo dorado pulsante + chispas y corazones que ascienden
  int gcy = PET_GROUND - 96;
  for (int k = 0; k < 4; k++) {
    int r = 60 + k * 34 + (int)(10 * sinf(now * 0.02f));
    gfx->drawCircle(CX, gcy, r, C565(0xff, 0xdf, 0x8a));
  }
  for (int i = 0; i < 16; i++) {
    int px = (i * 71 + 28) % 466;
    int py = 410 - (int)((now / 8 + i * 70) % 360);   // suben y reaparecen abajo
    if (py < 30) continue;
    if (i % 4 == 0) drawMap(SPR_HEART, 32, px - 8, py - 8, 1, false);  // corazoncito
    else gfx->fillRect(px, py, 4, 4, (i % 2) ? C565(0xff, 0xe7, 0x9f) : C565(0xff, 0x9a, 0xc0));
  }

  if (t < 0.45f) {                         // reverencia / pose de despedida
    act = pmd.has(PMD_POSE) ? PMD_POSE : (pmd.has(PMD_NOD) ? PMD_NOD : PMD_IDLE);
  } else {                                 // se aleja por la derecha
    act = pmd.has(PMD_WALKR) ? PMD_WALKR : PMD_IDLE;
    x = CX + (int)(((t - 0.45f) / 0.55f) * (CX + 140));
  }
  drawPmdAct(act, x, y, now, true, false, 5);
  if (pet.showHeart())                     // corazon grande siguiendo al bicho
    drawMap(SPR_HEART, 32, x + 50, y - 190, 2, false);
}

// dialogo de decision (2 botones apilados): evolucionar/mantener o despedirse/quedaros
void drawChoiceDialog() {
  const char *q, *o1, *o2;
  uint16_t c1, c2, t1, t2;
  if (choiceKind == 1) {  // evolucion
    q = T(S_EVO_Q); o1 = T(S_EVO_TAP); o2 = T(S_EVO_KEEP);
    c1 = UI_BAR_BAD; t1 = UI_WHITE; c2 = UI_TRACK; t2 = UI_INK;
  } else if (choiceKind == 3) {  // borrar la partida
    q = T(S_RESET_Q); o1 = T(S_RESET_GO); o2 = T(S_RESET_KEEP);
    c1 = UI_BAR_BAD; t1 = UI_WHITE; c2 = UI_BAR_OK; t2 = UI_WHITE;
  } else {                // despedida
    q = T(S_FAR_Q); o1 = T(S_FAR_GO); o2 = T(S_FAR_STAY);
    c1 = UI_BAR_WARN; t1 = UI_INK; c2 = UI_BAR_OK; t2 = UI_WHITE;
  }
  gfx->fillRoundRect(73, 156, 320, 188, 16, UI_WHITE);
  gfx->drawRoundRect(73, 156, 320, 188, 16, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(q, 2), 176);
  printT(q);
  gfx->fillRoundRect(93, 206, 280, 52, 12, c1);     // boton accion
  gfx->setTextColor(t1);
  setCur(centerX(o1, 2), 224);
  printT(o1);
  gfx->fillRoundRect(93, 268, 280, 52, 12, c2);     // boton mantener/quedaros
  gfx->setTextColor(t2);
  setCur(centerX(o2, 2), 286);
  printT(o2);
}

// boton-CTA rojo y grande para evolucionar (pulsa para llamar la atencion)
void drawEvolveButton() {
  uint32_t now = millis();
  int p = (int)(5 * sinf(now * 0.006f));  // late: -5..5
  int x = EVO_BTN_X - p, y = EVO_BTN_Y - p, w = EVO_BTN_W + 2 * p, h = EVO_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 18, UI_BAR_BAD);
  gfx->drawRoundRect(x, y, w, h, 18, UI_WHITE);
  gfx->drawRoundRect(x + 2, y + 2, w - 4, h - 4, 16, UI_WHITE);
  gfx->setTextColor(UI_WHITE);
  setSize(3);
  const char *t = T(S_EVO_TAP);
  setCur(centerX(t, 3), y + h / 2 - 11);
  printT(t);
}

// boton-CTA dorado de despedida: "<nombre> quiere decirte algo..."
void drawFarewellButton() {
  uint32_t now = millis();
  int p = (int)(4 * sinf(now * 0.005f));
  int x = FAR_BTN_X - p, y = FAR_BTN_Y - p, w = FAR_BTN_W + 2 * p, h = FAR_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 16, UI_BAR_WARN);
  gfx->drawRoundRect(x, y, w, h, 16, UI_INK);
  char buf[52];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  snprintf(buf, sizeof(buf), T(S_FAREWELL_BTN), nm);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(buf, 2), y + h / 2 - 8);
  printT(buf);
}

// boton-CTA sombrio de escapada por abandono: "<nombre> se siente abandonado..."
// (final triste: azul-gris oscuro, latido lento y apagado)
void drawRunawayButton() {
  uint32_t now = millis();
  int p = (int)(3 * sinf(now * 0.003f));
  int x = FAR_BTN_X - p, y = FAR_BTN_Y - p, w = FAR_BTN_W + 2 * p, h = FAR_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 16, C565(0x3a, 0x44, 0x5a));
  gfx->drawRoundRect(x, y, w, h, 16, C565(0x70, 0x80, 0x98));
  char buf[52];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  snprintf(buf, sizeof(buf), T(S_RUNAWAY_BTN), nm);
  gfx->setTextColor(C565(0xc8, 0xd2, 0xe0));
  setSize(2);
  setCur(centerX(buf, 2), y + h / 2 - 8);
  printT(buf);
}

// animacion epica de evolucion: halo radial + rayos giratorios + parpadeo del
// sprite acelerando + chispas que salen disparadas + fogonazo final
void drawEvolveFX(uint32_t now) {
  float t = pet.evolveT();          // 0..1
  int cx = CX, cy = PET_GROUND - 96;

  // halo radial que crece y pulsa
  int halo = 36 + (int)(t * 150) + (int)(8 * sinf(now * 0.02f));
  for (int k = 0; k < 4; k++) {
    int r = halo - k * 7;
    if (r > 0) gfx->drawCircle(cx, cy, r, UI_WHITE);
  }
  // rayos giratorios desde el centro del bicho
  float base = now * 0.004f;
  for (int i = 0; i < 12; i++) {
    float a = base + i * (float)(PI / 6);
    int len = 90 + (int)(70 * (0.5f + 0.5f * sinf(now * 0.012f + i)));
    gfx->drawLine(cx, cy, cx + (int)(cosf(a) * len), cy + (int)(sinf(a) * len), UI_WHITE);
  }
  // parpadeo entre la forma ANTERIOR y la NUEVA (siluetas), acelerando; al
  // final (t>0.9) se queda fija en la nueva para el fogonazo de revelado
  int period = 60 + (int)(220 * (1.0f - t));
  bool showOld = t < 0.9f && evoPmd.loaded && ((now / period) % 2) == 0;
  if (showOld) drawPmdActM(evoPmd, PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  else drawPmdAct(PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  // chispas que salen disparadas
  for (int i = 0; i < 10; i++) {
    float a = i * (float)(PI / 5) + t * 4.0f;
    int d = (int)((now / 14 + i * 33) % 200);
    int sx = cx + (int)(cosf(a) * d), sy = cy + (int)(sinf(a) * d);
    gfx->fillRect(sx - 2, sy - 2, 5, 5, (i & 1) ? C565(0xff, 0xe0, 0x70) : UI_WHITE);
  }
  // fogonazo final antes de revelar la forma nueva
  if (t > 0.9f) gfx->fillCircle(cx, cy, (int)(300 * (t - 0.9f) / 0.1f), UI_WHITE);
}

void drawPet() {
  if (pmd.loaded) {
    drawPetPMD();
    return;
  }
  if (mon.loaded) {
    drawPetSD();
    return;
  }
  int fi = flashIdxForDex(pet.speciesId);
  if (fi < 0) {
    // sin SD y sin sprite de flash: aviso claro de que faltan sprites
    gfx->setTextColor(inkColor());
    setSize(6);
    setCur(CX - 18, PET_CY - 80);
    printT("?");
    setSize(2);
    const char *l1 = T(S_NO_SPRITES);
    setCur(centerX(l1, 2), PET_CY - 4);
    printT(l1);
    const char *l2 = T(S_LOAD_SPRITES);
    setCur(centerX(l2, 2), PET_CY + 20);
    printT(l2);
    return;
  }
  const Species &sp = SPECIES[fi];
  int s = sp.scale;
  int x = CX - 16 * s;
  int y = PET_CY - 16 * s;

  // animacion de evolucion: alterna la silueta de la forma anterior y la nueva
  if (pet.evolving()) {
    bool flash = (millis() / 300) % 2;
    int16_t showDex = (flash && pet.prevSpeciesId >= 0) ? pet.prevSpeciesId : pet.speciesId;
    int sfi = flashIdxForDex(showDex);
    if (sfi >= 0) {
      const Species &show = SPECIES[sfi];
      drawMap(show.sprite, SPRITE_H, CX - 16 * show.scale, PET_CY - 16 * show.scale, show.scale, flash);
    }
    return;
  }

  PetMood m = pet.mood();
  if (m == MOOD_HAPPY && (millis() / 500) % 2) y -= 6;  // saltito

  drawMap(sp.sprite, SPRITE_H, x, y, s, false);

  // expresiones superpuestas usando las anclas de la especie
  bool blink = (millis() % 3500 < 300);
  if (m == MOOD_SLEEPING || blink) {
    overlayEye(sp, x, y, s, sp.eyeColL);
    overlayEye(sp, x, y, s, sp.eyeColR);
  }
  if (m == MOOD_EATING) overlayMouth(sp, x, y, s, true);
  else if (m == MOOD_SAD) overlayMouth(sp, x, y, s, false);

  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + 20 * s, y - 2 * s, 2, false);
}

// ---------- escena de bano ----------

void startBath() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony || bathUntil) return;
  bathUntil = millis() + 3000;
  bathPending = true;
  int cx = (int)beh.x;
  for (auto &b : bubbles) {
    b.x = cx - 70 + random(140);
    b.y = PET_GROUND - random(150);
    b.r = 8 + random(16);
    b.ph = random(64);
  }
}

void drawBath() {
  uint32_t now = millis();
  if (!timeLeft(bathUntil)) {
    bathUntil = 0;
    if (bathPending) {
      bathPending = false;
      pet.clean();
      // pose de alegria al quedar limpio
      if (pmd.has(PMD_POSE)) {
        beh.mode = 2;
        beh.act = PMD_POSE;
        beh.t0 = now;
        beh.until = now + pmdActTotalMs(pmd.acts[PMD_POSE]) * 2;
      }
    }
    return;
  }
  uint32_t left = bathUntil - now;
  if (left > 800) {
    // espuma: pompas meciendose y subiendo poco a poco
    float t = now / 220.0f;
    for (auto &b : bubbles) {
      int bx = b.x + (int)(sinf(t + b.ph) * 6);
      int by = b.y - (int)((3000 - left) / 90);
      gfx->fillCircle(bx, by, b.r, UI_WHITE);
      gfx->drawCircle(bx, by, b.r, 0x7E3D);
      gfx->fillCircle(bx - b.r / 3, by - b.r / 3, b.r / 4, UI_BG_DAY);
    }
  } else {
    // las pompas revientan: destellos
    for (int i = 0; i < 8; i++) {
      auto &b = bubbles[i];
      int sx = b.x + (i % 3) * 6 - 6, sy = b.y - 18;
      uint16_t col = (i % 2) ? UI_BAR_WARN : UI_WHITE;
      gfx->fillRect(sx - 6, sy - 1, 13, 3, col);
      gfx->fillRect(sx - 1, sy - 6, 3, 13, col);
    }
  }
}

// ---------- mascota PMD: comportamiento ----------

uint32_t pmdActTotalMs(const PmdAct &a) {
  uint32_t t = 0;
  for (uint8_t i = 0; i < a.frames; i++) t += a.ms[i];
  return t ? t : 100;
}

uint8_t pmdFrameAt(const PmdAct &a, uint32_t t, bool loop) {
  uint32_t total = pmdActTotalMs(a);
  if (!loop && t >= total) return a.frames - 1;
  t %= total;
  uint8_t i = 0;
  // guarda: nunca mas vueltas que frames, por si un .bin corrupto trae ms[]=0
  // pese al saneo de la carga (defensa en profundidad)
  for (uint8_t guard = 0; guard < a.frames && t >= a.ms[i]; guard++) {
    t -= a.ms[i];
    i = (i + 1) % a.frames;
  }
  return i;
}

// dibuja una accion anclada por la base (centro-x, suelo) y devuelve su escala
// escala entera para que lo visible del sprite (sin el relleno transparente del lienzo) quepa
// en maxW x maxH: el alto del lienzo incluye mucho aire y se queda muy por debajo
uint8_t pmdFitScale(const PmdMon &m, uint8_t actId, int maxW, int maxH) {
  const PmdAct &a = m.acts[actId];
  int top = a.h, bottom = -1, left = a.w, right = -1;
  for (int r = 0; r < a.h; r++)
    for (int c = 0; c < a.w; c++)
      if (a.data[r * a.w + c] != 0xFF) {
        if (r < top) top = r;
        if (r > bottom) bottom = r;
        if (c < left) left = c;
        if (c > right) right = c;
      }
  if (bottom < 0) return 2;
  int sw = maxW / (right - left + 1), sh = maxH / (bottom - top + 1);
  int fit = sw < sh ? sw : sh;
  return fit < 2 ? 2 : (fit > 8 ? 8 : fit);
}

// dibuja una accion de un PmdMon concreto (m); drawPmdAct usa el global pmd
void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS) {
  const PmdAct &a = m.acts[actId];
  if (!a.frames) return;
  uint8_t sBase = m.acts[PMD_IDLE].h ? 170 / m.acts[PMD_IDLE].h : 5;
  if (sBase < 2) sBase = 2;
  if (sBase > maxS) sBase = maxS;
  uint8_t s = sBase;
  while (s > 2 && a.h * s > 250) s--;  // acciones con frame grande (ataque)
  if (gPmdForceScale) s = gPmdForceScale;
  uint8_t fi = pmdFrameAt(a, t, loop);
  const uint8_t *fr = a.data + (uint32_t)fi * a.w * a.h;
  // anclar por los pies (a.base), no por el alto del lienzo: asi las acciones
  // con padding distinto (Hurt, Eat...) quedan todas a la misma altura de suelo
  int x0 = cx - a.w * s / 2, y0 = groundY - (a.base ? a.base : a.h) * s;
  for (int r = 0; r < a.h; r++) {
    const uint8_t *row = fr + r * a.w;
    forRuns(a.w, [&](int c) { return indexedKey(row[c], sil); },
            [&](int c0, int len, int k) { gfx->fillRect(x0 + c0 * s, y0 + r * s, len * s, s, sil ? INK_K : m.pal[k]); });
  }
}
void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS) {
  drawPmdActM(pmd, actId, cx, groundY, t, loop, sil, maxS);
}

// elige el siguiente capricho del bicho cuando esta contento
void behNext() {
  uint32_t now = millis();
  beh.t0 = now;
  int r = random(100);
  if (r < 35 && (pmd.has(PMD_WALKL) || pmd.has(PMD_WALKR))) {
    beh.mode = 1;  // paseo
    beh.targetX = 150 + random(176);
    beh.until = now + 15000;
  } else if (r < 60) {
    // gesto aleatorio entre los disponibles
    // (Hop fuera: salta demasiado alto; Sit fuera: mira hacia atras)
    static const uint8_t flair[] = { PMD_POSE, PMD_NOD, PMD_BREATH };
    uint8_t pick[3], n = 0;
    for (uint8_t f : flair)
      if (pmd.has(f)) pick[n++] = f;
    if (n) {
      beh.mode = 2;
      beh.act = pick[random(n)];
      beh.until = now + pmdActTotalMs(pmd.acts[beh.act]);
      return;
    }
    beh.mode = 0;
    beh.until = now + 2000 + random(3000);
  } else {
    beh.mode = 0;  // mirar al frente
    beh.until = now + 2000 + random(3000);
  }
}

void drawPetPMD() {
  uint32_t now = millis();

  if (pet.evolving()) {
    drawEvolveFX(now);
    return;
  }
  if (evoPmd.loaded) evoPmd.unload();  // termino la evolucion: libera la forma anterior

  PetMood m = pet.mood();
  uint8_t act;
  bool loop = true;
  if (m == MOOD_SLEEPING && pmd.has(PMD_SLEEP)) {
    act = PMD_SLEEP;
    beh.mode = 0;
  } else if (m == MOOD_EATING && pmd.has(PMD_EAT)) {
    act = PMD_EAT;
    beh.t0 = 0;
  } else if (m == MOOD_SAD && pmd.has(PMD_HURT)) {
    act = PMD_HURT;
  } else {
    // contento: el planificador decide (idle / paseo / gesto)
    if (now > beh.until) behNext();
    if (beh.mode != 1) walkLast = 0;  // al volver a caminar no se cuenta el tiempo parado
    if (beh.mode == 1) {
      float d = beh.targetX - beh.x;
      if (fabsf(d) < 4) {
        behNext();
        act = PMD_IDLE;
      } else {
        // 30 px/s de diseno (3 px por frame a 10 fps), midiendo el tiempo real: con menos frames
        // por segundo camina igual de rapido
        uint32_t dt = walkLast ? now - walkLast : 100;
        uint32_t cap = frameIntervalMs(powerCtx(now)) + 100;  // un hueco mayor (otra pantalla de por medio) no es un paso largo
        if (dt > cap) dt = cap;
        walkLast = now;
        float step = 30.0f * dt / 1000.0f;
        if (step > fabsf(d)) step = fabsf(d);  // un frame lento no sobrepasa el destino
        beh.x += d > 0 ? step : -step;
        act = (d > 0) ? PMD_WALKR : PMD_WALKL;
      }
    } else {
      act = (beh.mode == 2) ? beh.act : PMD_IDLE;
      loop = false;
    }
    if (!pmd.has(act)) act = PMD_IDLE;
  }

  drawPmdAct(act, (int)beh.x, PET_GROUND, now - beh.t0, loop || act == PMD_IDLE, false, 5);

  if (pet.showHeart()) drawMap(SPR_HEART, 32, (int)beh.x + 50, PET_GROUND - 190, 2, false);
  drawVisitor(now);
}

// sprite animado desde la SD: zoom entero por pixel, frames a su ritmo
void drawPetSD() {
  int s = mon.scale;
  int w = mon.w * s, h = mon.h * s;
  int x = CX - w / 2;
  int y = PET_CY - h / 2;

  bool sil = false;
  if (pet.evolving()) {
    sil = (millis() / 300) % 2;
  } else if (pet.mood() == MOOD_HAPPY && (millis() / 500) % 2) {
    y -= 6;  // saltito
  }

  uint16_t fm = mon.frameMs ? mon.frameMs : 100;
  uint16_t fi = pet.sleeping ? 0 : (millis() / fm) % mon.frames;
  const uint8_t *fr = mon.data + (uint32_t)fi * mon.w * mon.h;
  for (int r = 0; r < mon.h; r++) {
    const uint8_t *row = fr + r * mon.w;
    forRuns(mon.w, [&](int c) { return indexedKey(row[c], sil); },
            [&](int c0, int len, int k) { gfx->fillRect(x + c0 * s, y + r * s, len * s, s, sil ? INK_K : mon.pal[k]); });
  }

  // emotes en vez de expresiones (los sprites importados no tienen anclas)
  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + w - 30, y - 50, 2, false);
}

// ojo cerrado: borra el ojo 3x4 y dibuja el parpado
void overlayEye(const Species &sp, int x, int y, int s, int col) {
  gfx->fillRect(x + col * s, y + sp.eyeRow * s, 3 * s, 4 * s, sp.bodyColor);
  gfx->fillRect(x + col * s, y + (sp.eyeRow + 2) * s, 3 * s, s, INK_K);
}

// borra la sonrisa base y pinta boca abierta (comer) o ceno (triste)
void overlayMouth(const Species &sp, int x, int y, int s, bool open) {
  int mc = sp.mouthCol, mr = sp.mouthRow;
  gfx->fillRect(x + (mc - 3) * s, y + mr * s, 7 * s, 2 * s, sp.bodyColor);
  if (open) {
    gfx->fillRect(x + (mc - 2) * s, y + mr * s, 5 * s, 2 * s, INK_K);
  } else {
    gfx->fillRect(x + (mc - 2) * s, y + mr * s, 5 * s, s, INK_K);
    gfx->fillRect(x + (mc - 3) * s, y + (mr + 1) * s, s, s, INK_K);
    gfx->fillRect(x + (mc + 3) * s, y + (mr + 1) * s, s, s, INK_K);
  }
}

void drawPoops() {
  for (int i = 0; i < pet.poops; i++) {
    drawMap(SPR_POOP, 32, 36 + i * 46, 244, 2, false);
  }
}

void drawBars() {
  drawBar(78, 318, T(S_BAR_FOOD), pet.fullness);
  drawBar(244, 318, T(S_BAR_JOY), pet.joy);
  drawBar(78, 346, T(S_BAR_ENE), pet.energy);
  drawBar(244, 346, T(S_BAR_HYG), pet.hygiene);
}

// Separacion entre la etiqueta y su barra en la fila de necesidades. Estaba fija
// en 48 px, justo lo que ocupan 4 letras latinas a escala 2, sin margen. Las
// etiquetas japonesas son mas anchas (ごきげん son 4 kana, ~64 px) y se metian
// dentro de la barra. Se deriva de la etiqueta mas larga, con el 48 de suelo
// para que en los idiomas latinos no cambie nada.
// Requiere el tamano de texto 2 ya puesto, porque textW() lo necesita.
static int barLabelGap() {
  const StrId ids[] = { S_BAR_FOOD, S_BAR_JOY, S_BAR_ENE, S_BAR_HYG };
  int ancho = 0;
  for (StrId id : ids) {
    int w = textW(T(id), 2);
    if (w > ancho) ancho = w;
  }
  // por debajo del umbral original se deja EXACTAMENTE 48, para no mover nada en
  // los idiomas latinos; por encima se anade aire para que la etiqueta respire
  return ancho <= 48 ? 48 : ancho + 8;
}

void drawBar(int x, int y, const char *label, uint8_t val) {
  gfx->setTextColor(inkColor());
  setSize(2);
  setCur(x, y);
  printT(label);
  int gap = barLabelGap();
  // las dos columnas empiezan en 78 y 244: la barra de la primera tiene que
  // acabar antes de la etiqueta de la segunda, y ambas se pintan igual de anchas
  int bw = 232 - (78 + gap);
  if (bw > 100) bw = 100;   // con etiquetas latinas sale 100, como estaba
  int bx = x + gap, bh = 15;
  uint16_t fill = (val >= 50) ? UI_BAR_OK : (val >= 25) ? UI_BAR_WARN : UI_BAR_BAD;
  gfx->fillRoundRect(bx, y, bw, bh, 4, UI_TRACK);
  int fw = (bw - 4) * val / 100;
  if (fw > 0) gfx->fillRoundRect(bx + 2, y + 2, fw, bh - 4, 3, fill);
}

void drawButtons() {
  for (int i = 0; i < 4; i++) {
    bool off = pet.sleeping && i != 2;  // durmiendo solo funciona LUZ
    int bx = buttons[i].cx - BTN_HALF, by = buttons[i].cy - BTN_HALF;
    if (!pet.sleeping) gfx->fillRoundRect(bx, by, 2 * BTN_HALF, 2 * BTN_HALF, 14, UI_WHITE);
    gfx->drawRoundRect(bx, by, 2 * BTN_HALF, 2 * BTN_HALF, 14, inkColor());
    if (!off) drawMap(buttons[i].icon, 16, buttons[i].cx - 16, buttons[i].cy - 16, 2, false);
  }
}

const char *eggMsg() {
  switch (pet.eggCracks()) {
    case 0: return T(S_EGG_TOUCH);
    case 1: return T(S_EGG_MOVES);
    default: return T(S_EGG_ALMOST);
  }
}

const char *statusMsg() {
  if (pet.evolving()) return T(S_EVOLVING);
  if (bathUntil) return "Splish splash!";  // onomatopeya universal
  if (pet.sleeping) return "Zzz...";
  if (pet.eating()) return T(S_EATING);
  if (pet.showHeart()) return T(S_LIKES);
  if (pet.fullness < 25) return T(S_HUNGRY);
  if (pet.hygiene < 25) return T(S_NEEDS_BATH);
  if (pet.energy < 25) return T(S_EXHAUSTED);
  if (pet.joy < 25) return T(S_SAD);
  if (pet.weight > 60) return T(S_CHUBBY);
  if (pet.shiny && pet.ageMinutes < 15) return T(S_IS_SHINY);
  return T(S_HAPPY);
}

// dibuja un mapa de n x n pixeles escalado; silhouette=true lo pinta en tinta
void drawMap(const char *const *map, int n, int x, int y, int s, bool silhouette) {
  for (int r = 0; r < n; r++) {
    const char *row = map[r];
    forRuns(n, [&](int c) { return charKey(row[c], silhouette); },
            [&](int c0, int len, int k) { gfx->fillRect(x + c0 * s, y + r * s, len * s, s, silhouette ? INK_K : spriteColor((char)k)); });
  }
}
