// PokeKnomiGochi - pixel-art tamagotchi inspired by gen 1, adapted from TamaPoke by Quique Tortosa
// for Waveshare ESP32-S3-Touch-AMOLED-1.75
//
// Libraries (Library Manager or Waveshare repo):
//   - "GFX Library for Arduino" (moononournation), with CO5300 QSPI support
//   - "SensorLib" (Lewis He), CST9217 touch driver
//
// Placa: ESP32S3 Dev Module | Flash 16MB | PSRAM: OPI PSRAM | USB CDC On Boot: Enabled
//
// Sprites and the species table are generated with tools/sprites.py (emit).

#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#ifdef BOARD_HOST  // Mac and others: built as a Knomi with different hardware underneath (see host/README.md)
#include "host/host_hal.h"
#elif defined(BOARD_KNOMI_V2)
#include "knomi_hal.h"
#else
#include "TouchDrvCSTXXX.hpp"
#endif
#include <U8g2lib.h>  // CJK fonts (Japanese); see applyLangFont()
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
#include "dungeon.h"
#include "zones.h"
#include "zones_art.h"
#include "autorot.h"
#include "press.h"
#include "volley.h"
#include "ota.h"
#include "ota_update.h"
#include "buddy_net.h"

// Firmware version. Bump this number on every release (and manifest.json for
// the web installer). Shown on the settings screen and over serial at boot.
#define FW_VERSION "1.27"

#define UPD_BTN_Y 296      // settings page: the Wi-Fi update button
bool otaChecking = false;  // it says "Searching..." while the check runs

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
// Full framebuffer in PSRAM: we draw everything and then flush() (no flicker)
Arduino_Canvas *gfx = new Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, panel);

TouchDrvCST92xx touch;
#endif
// active CJK font: always false for now, loading will arrive with
// Japanese/Korean/Chinese support (see PR #13)
bool gCjkFont = false;
#ifdef BOARD_KNOMI_V2
#define TOUCH_ADDR 0x15  // CST816S
#else
#define TOUCH_ADDR 0x5A  // CST9217
#endif
Pet pet;

// animated sprite from the SD for the current species (if the file exists)
SdMon mon;          // B/W sprite (fallback and minigame if there is no PMD)
PmdMon pmd;         // multi-action PMD sprite (main screen)
PmdMon evoPmd;      // previous form, only during the evolution blink
int16_t monFor = -2;
bool monShinyFor = false;

// on-screen pet behavior
struct {
  uint8_t mode = 0;     // 0 idle, 1 walk, 2 one-shot gesture
  uint8_t act = PMD_IDLE;
  uint32_t t0 = 0;      // start of the current animation
  uint32_t until = 0;   // end of the current state
  float x = 233, targetX = 233;
} beh;
uint32_t walkLast = 0;  // time of the last walk step (0 = none yet)
#define PET_GROUND 304  // pet ground line
PmdMon galleryPmd;  // large sprite for the gallery detail view (PMD/TPK2, legal)
PmdMon galleryShinyPmd;  // the entry's shiny, next to the regular one when both are caught

// pokedex gallery
bool galleryOpen = false;
bool galleryDirty = false;
int galleryPage = 0;        // 10 pages of 16
int eggPickPage = 0;        // egg choice: grid with only what is registered
int16_t eggPickDetail = 0;
bool eggPickDirty = true;
int16_t galleryDetail = 0;  // dex in detail view, 0 = grid

bool screenOff = false;       // short press of the PWR button
bool cardOpen = false;        // pet card (vertical swipe)
bool kbOpen = false;          // keyboard for renaming the pet
char nameBuf[12] = "";
uint8_t nameLen = 0;
int cardNameBottom = 84;      // where the card's name (and the species under a nickname) ends: tapping above renames
uint8_t cardPage = 0;         // the card: 0 profile, 1 stats, 2 training, 3 progress; the achievements: their pages
#define CARD_PAGES 4
bool cardAch = false;         // the same screen showing the achievements (swipe right to left from home)
#define ACH_PAGES 5           // the pet's medals (2 pages), the trainer's (2), the zones' (1): 6 at most a page
#define cardPageCount() (cardAch ? ACH_PAGES : CARD_PAGES)  // a macro: a function here would come before the sketch's prototypes
uint8_t clockPage = 0;        // 0 time, 1 settings (Knomi only)
uint8_t wifiListPage = 0, wifiKbLayer = 0;
int8_t wifiKbHover = -1;      // key under the finger: typed on release
int16_t wifiKbX = 0, wifiKbY = 0;
#define KB_COLS 6
#define KB_X 40
#define KB_Y 150
#define KB_W 64
#define KB_H 52
bool clockOpen = false;       // time-setting screen (swipe down)
int clockH = 12, clockM = 0;  // time being edited
bool clockEdited = false;      // set by hand: don't overwrite it with network time

// bath scene: foam over the pet and cleanup when it pops
uint32_t bathUntil = 0;
bool bathPending = false;
struct { int16_t x, y; uint8_t r, ph; } bubbles[14];
uint32_t feedMenuUntil = 0;   // food picker open until this millis

// "taps" minigame: keep the pokeball in the air
bool gameOpen = false;
bool gamesMenuOpen = false;
enum ZoneScreen : uint8_t { ZS_NONE, ZS_PICK, ZS_ZONE };
ZoneScreen zoneScreen = ZS_NONE;
uint8_t zoneCur = 0, zonePage = 0;
int8_t bZone = -1;                  // this battle is in a zone; -1 = random
uint32_t zoneUntil = 0;
int16_t zoneDetail = 0;             // the entry opened from the zone's grid (0 = the grid)
uint16_t *zoneArt = nullptr;        // the 5 frames, painted the first time the menu opens
uint16_t *zoneBg = nullptr;         // the current battle's one, scaled up to the screen
uint32_t gamesMenuUntil = 0;
// who's that pokemon
enum QuizPhase : uint8_t { QP_ASK, QP_REVEAL, QP_END };
bool quizOpen = false;
uint8_t gPmdForceScale = 0;  // if not 0, fixed scale for the next sprite
QuizPhase quizPhase = QP_ASK;
uint32_t quizT = 0;
uint8_t quizRound = 0, quizScore = 0;
int16_t quizAnswer = 0, quizOpts[3] = {0, 0, 0}, quizTapped = 0;
PmdMon quizPmd;
// daily visit
enum VisitPhase : uint8_t { VP_NONE, VP_ANNOUNCE, VP_ENTER, VP_HANG, VP_LEAVE };
VisitPhase visitPhase = VP_NONE;
uint32_t visitT = 0, visitCheckT = 0;
int16_t visitDex = 0;
PmdMon visitPmd;
// tilt
float tiltBase = 0, tiltSmooth = 0, tiltOverride = 0;
bool tiltForced = false, tiltAim = false;
#ifdef BOARD_KNOMI_V2
KnomiAccel accel;
#endif
uint32_t gameOverUntil = 0;
// pokerun
bool runOpen = false, runJump = false, runNewHi = false, runBot = false;
bool dungeonOpen = false, dnBot = false, dnNewHi = false, dnHeld = false;
DnState dn;

// buddy: another PokeKnomiGochi on the same wifi
enum BuddySession : uint8_t { BS_IDLE, BS_INVITING, BS_INVITED, BS_SYNC, BS_BATTLE, BS_TRADE, BS_PLAY };
enum TradePhase : uint8_t { TR_PICK, TR_WAIT, TR_REVIEW, TR_WAITC, TR_DONE };
BuddyPeer bPeers[BUDDY_MAX];  // everyone around (buddy.h)
uint8_t bCur = 0;            // the one in the buddy menu, and in any battle or trade
bool buddyFake = false;  // FAKEBUDDY (test mode): a buddy without a network, for screenshots
PmdMon buddyPmd[BUDDY_MAX];  // each buddy's sprite, by slot
int16_t buddyPmdDex[BUDDY_MAX] = {};
BuddySession bsState = BS_IDLE;
uint8_t bsKind = 0;
uint32_t bsId = 0, bsT = 0, bsResendT = 0, buddyHelloT = 0;
bool buddyMenuOpen = false;
bool playMenuOpen = false;  // "Play together": the games to play with the buddy of the menu
// Who's That? Showdown: the quiz against a buddy, best of SD_ROUNDS (buddy.h)
enum SdPhase : uint8_t { SD_READY, SD_ASK, SD_REVEAL, SD_DONE };
bool sdOpen = false, sdHost = false, sdRewarded = false;
SdPhase sdPhase = SD_READY;
uint8_t sdRound = 0, sdWho = 0, sdHostScore = 0, sdGuestScore = 0;  // sdWho: SD_NOBODY/SD_HOST/SD_GUEST
int16_t sdAnswer = 0, sdOpts[3] = {};
int8_t sdMyPick = -1, sdTheirPick = -1;
uint16_t sdMyMs = 0, sdTheirMs = 0;
bool sdTheirIn = false;
uint32_t sdT = 0, sdAskT = 0, sdFirstRightT = 0, sdSendT = 0;
// Poke Ball Volley (volley.h): the ball is on one board at a time; over the net it goes to the other
enum VbPhase : uint8_t { VB_READY, VB_PLAY, VB_AWAY, VB_SERVE, VB_POINTED, VB_DONE };
bool vbOpen = false, vbHost = false, vbRewarded = false, vbMineLast = false;
VbPhase vbPhase = VB_READY;
VbBall vbBall;
float vbPadX = 233, vbSpeed0 = VB_SPEED0;
uint8_t vbMine = 0, vbTheirs = 0, vbSeqOut = 0, vbSeqIn = 0xFF;
bool vbOutPending = false, vbMissPending = false;  // a crossing / a point not acknowledged yet
VbBall vbOut;                                      // the crossing being sent
uint32_t vbT = 0, vbStepT = 0, vbSendT = 0;
bool vbBot = false;  // test mode: the paddle chases the ball, at a limited speed (VBBOT)
char buddyNote[44] = "";
uint32_t buddyNoteUntil = 0;
bool bPvp = false, bPvpGone = false;   // this battle is against the buddy
uint32_t bRng = 0;
uint8_t bPvpTurn = 0, bPvpMine = 0, bPvpMinePrev = 0;
int16_t bPvpFoeTurn = -1;              // buddy's turn and move, when they arrive
uint8_t bPvpFoeMove = 0;
bool tradeOpen = false;
TradePhase trPhase = TR_PICK;
int16_t trMine = 0, trTheirs = 0;
bool trMineShiny = false, trTheirsShiny = false;  // which form of each is traded
int trPage = 0;
bool trTheirConfirm = false;
uint32_t trT = 0;
uint32_t dnOverT = 0, dnStepT = 0, dnHoldT = 0, dnHurtT = 0, dnHitT = 0;
float dnCamX = 0, dnCamY = 0;
uint8_t dnScale = 2;
RunState run;
uint32_t runLast = 0, runOverT = 0, runFrames = 0;
uint8_t runScale = 2;
float ballX, ballY, ballVX, ballVY, gamePetX, gamePetVX = 0;
uint32_t gameStartT = 0;
uint32_t lastGameStep = 0;  // last call to stepGame(): physics by real time, not per frame
uint8_t gameScore, gameMisses;
float hitX, hitY;             // last hit (impact ring)
uint32_t hitTime = 0;
bool gameNewHi = false;

// battle against a wild foe (trains strength and fills the pokedex)
enum BattlePhase : uint8_t { BP_TIRED, BP_INTRO, BP_LEARNED, BP_CHOOSE, BP_WAIT, BP_ATTACK, BP_HIT, BP_EFFECT, BP_RAN, BP_END,
                            BP_AIM, BP_THROW, BP_MISS, BP_SHAKE, BP_BROKE, BP_CAUGHT, BP_FLED };
bool battleOpen = false;
BattlePhase bPhase = BP_INTRO;
uint32_t bPhaseT = 0;
BtlFighter bMe, bFoe;
PmdMon wildPmd;
float bShownMe = 0, bShownFoe = 0;  // on-screen HP: drops gradually down to the real value
struct { bool mine; uint8_t move; } bTurn[2];
uint8_t bStep = 0, bLastEff = 2;
uint8_t bGain = 0;
bool bWon = false;
BtlMoves bMeMoves, bFoeMoves;
uint8_t bLearned = 0;  // the move announced by BP_LEARNED
float bBallX = 233, bFoeX = 233;  // capture: position of the throw and of the foe
uint8_t bBalls = 0;
bool bFoeShiny = false;     // a shiny wild one: catching it registers the shiny form
bool bNextShiny = false;    // WILD <dex> S (testing): the next wild one is shiny
uint32_t bBallT = 0;

// the 9 species with their own sprite in flash (fallback without SD): dex -> index
int flashIdxForDex(int16_t dex) {
  static const int8_t IDX[10] = { -1, 3, 4, 5, 0, 1, 2, 6, 7, 8 };
  return (dex >= 1 && dex <= 9) ? IDX[dex] : -1;
}

#define CX 233  // center of the round screen
#define CY 233
#define PET_CY 202  // vertical center of the sprite

static const uint16_t INK_K = 0x18C4;  // spriteColor('k')

// icon buttons following the bottom arc of the round screen
// (the outer ones sit higher so they don't leave the circle)
struct Btn {
  int16_t cx, cy;
  const char *const *icon;
};
Btn buttons[4] = {
  { 140, 400, SPR_HOME_FOOD },    // eat
  { 202, 414, SPR_HOME_BALL },    // play
  { 264, 414, SPR_HOME_MOON },    // light
  { 326, 400, SPR_HOME_BUBBLE },  // bath
};
#define BTN_HALF 27  // the 54x54 icon (HOME_ICON_N at 3x)
#define BTN_HIT 36   // touch radius (a bit more generous)

// egg cracks ('k' pixels over the sprite)
static const uint8_t CRACK1[][2] = { {15,8},{16,9},{15,10} };
static const uint8_t CRACK2[][2] = { {11,13},{12,14},{11,15},{20,12},{19,13},{20,14} };
// night mode stars
static const uint16_t STARS[][2] = { {120,140},{330,120},{370,210},{95,230},{280,90},{160,95} };

bool wasPressed = false;
// starter choice (first game): Bulbasaur / Charmander / Squirtle, 3 rows
static const int16_t STARTER_DEX[3] = { 1, 4, 7 };
#define STARTER_ROW_Y 110
#define STARTER_ROW_H 70
#define STARTER_ROW_GAP 8
// evolution CTA button (centered, mid-screen)
#define EVO_BTN_W 256
#define EVO_BTN_H 64
#define EVO_BTN_X (CX - EVO_BTN_W / 2)
#define EVO_BTN_Y 172
// farewell CTA button (wider: carries the name + phrase)
#define FAR_BTN_W 408
#define FAR_BTN_H 58
#define FAR_BTN_X (CX - FAR_BTN_W / 2)
#define FAR_BTN_Y 176
// the CST9217 signals on the INT pin when there is touch data; we use it to avoid
// reading the I2C bus while the chip is asleep (that read hung for ~1s)
volatile bool gTouchIrq = false;
void IRAM_ATTR touchIsr() { gTouchIrq = true; }
uint32_t lastRender = 0;
uint32_t sdEpoch = 0;  // +1 every time the SD is reloaded (thumbnails change)
// AMOLED protection: dimmed on inactivity
uint32_t lastInteract = 0;
uint8_t dimStage = 0;        // 0 awake, 1 dimmed (90s), 2 nearly off (5min)
bool swallowGesture = false; // the touch that wakes it does nothing
uint32_t holdStart = 0;     // long press on the pet
uint32_t confirmUntil = 0;  // "release?" dialog active until this millis
#ifdef BOARD_KNOMI_V2
uint32_t gBtnSimUntil = 0;  // simulated press of the physical button (serial console)
#endif
uint8_t choiceKind = 0;     // decision dialog: 0 none, 1 evolution, 2 farewell, 3 delete save, 4 runaway
uint32_t choiceUntil = 0;   // closes by itself at this millis
int16_t tX0, tY0, tXl, tYl; // gesture in progress (start and last position)
uint32_t tStart = 0;
bool holdFired = false;

void setup() {
  Serial.setRxBufferSize(8192);  // the SD transfer arrives in 2 KB blocks
  Serial.begin(115200);
  // CRITICAL: without this, Serial.print BLOCKS the game when no
  // serial monitor is open on the host (the USB CDC TX buffer fills up
  // and nobody drains it) -> with timeout 0 messages are dropped
#ifndef BOARD_KNOMI_V2  // the Knomi sends serial through a CH340 (UART0): it never blocks
  Serial.setTxTimeoutMs(0);
#endif
  Serial.printf("PokeKnomiGochi fw v%s\n", FW_VERSION);
  loadLang();  // saved language (ES by default)
  Wire.begin(IIC_SDA, IIC_SCL);
  // CST9217 (touch), AXP2101 (PMU) y PCF85063 (RTC) share this I2C bus.
  // Safety net for PMU/RTC (SensorLib does NOT honor this timeout on the
  // touch; the sleeping-touch hang is solved by gating on INT, see
  // handleTouch).
  Wire.setTimeOut(50);

  // CRITICAL: power on the panel supply (BLDO1=OLED VDD 3.3V) BEFORE
  // initializing the display. If the PMU was reset (total drain), this rail
  // stays OFF and the screen looks black even though the rest of the board works.
  pmuEnablePanel();

  // QSPI at 80MHz (default 40): the framebuffer flush is the fps
  // bottleneck (~56ms at 40MHz). If the panel shows garbage, drop back to 40M.
  if (!gfx->begin(80000000)) Serial.println("gfx->begin() failed");
  panel->setBrightness(180);
  applyLangFont();  // font for the saved language (classic except CJK)

  touch.setPins(TP_RESET, TP_INT);
  bool touchOk = false;
  for (int i = 0; i < 3 && !touchOk; i++) {  // sometimes fails on the first attempt
    touchOk = touch.begin(Wire, TOUCH_ADDR, IIC_SDA, IIC_SCL);
    if (!touchOk) delay(150);
  }
  if (!touchOk) Serial.println("CST9217 not detected");
  // begin() leaves the chip in command mode (it reads the identity and stays there);
  // a hardware reset is needed for it to report touches again
  touch.reset();
  touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
  touch.setMirrorXY(true, true);  // the panel is mounted rotated 180 degrees
  // INT active-low: fires when there is data. Gates I2C reads (see loop)
  pinMode(TP_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TP_INT), touchIsr, FALLING);

#ifdef BOARD_KNOMI_V2
  pinMode(KNOMI_BTN_PIN, INPUT_PULLUP);
  if (!accel.begin(Wire)) Serial.println("LIS2DW12 not detected");
#endif
  pet.begin();
  sdBegin();
  thumbs.load();

  // real clock: applies the time it was off
  rtcBegin();
  batBegin();
  pwrSetup();
  uint32_t e = rtcEpoch();
  if (e == 0) {
    // Without a backup battery, the PCF85063 loses the time when power is
    // cut. Always seeding the fixed date would make game time GO BACKWARDS
    // relative to what was already saved, and that breaks the streak, the age and the
    // scene time. It is seeded with the last time seen, if later, so
    // that time never goes backwards. The minutes spent off are lost
    // anyway (without an RTC there is no way to know them), but nothing gets out of sync.
    uint32_t seed = 1767225600UL;             // truly blank RTC
    uint32_t seen = pet.savedEpoch();         // uses the NVS that pet already has open
    if (seen > seed) seed = seen;
    rtcSetEpoch(seed);
    e = rtcEpoch();
    Serial.printf("RTC has no time: seeded to %u%s\n", seed,
                  seen > 1767225600UL ? " (from the last saved time)" : "");
  }
  pet.syncClock(e);

  audioBegin();  // ES8311 + I2S + amplificador (plays a startup jingle)

  lastInteract = millis();
}

// load/unload the SD sprite when the species changes
void ensureMon() {
  if (pet.speciesId == monFor && monShinyFor == pet.shiny && !sdDirty) return;
  // freshly received files may include a new thumbs.bin, which was
  // only loaded in setup(): without this it didn't show in the gallery until reboot
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
    pmd.load(pet.speciesId, pet.shiny);          // main: PMD
    if (!pmd.loaded) mon.load(pet.speciesId, pet.shiny);  // fallback: B/W
  }
}

void loop() {
  uint32_t now = millis();
  pet.update(now);

  // the amplifier follows the sleep state (the call returns early if unchanged).
  // This covers every path: the light button, night sleep and the one that arrives
  // applied from offline progression.
  audioSetSleeping(pet.sleeping);

  // plays a sound when the pet becomes ready to evolve
  // (includes the case of qualifying on waking). canEvolveNow is false while asleep.
  static bool wasEvoReady = false;
  bool evoReady = pet.wantEvolveButton();
  if (evoReady && !wasEvoReady) sfxPlay(SFX_MEDAL);
  wasEvoReady = evoReady;
  // grim warning when the pet is about to run away from neglect
  static bool wasRunReady = false;
  bool runReady = pet.canRunawayNow();
  if (runReady && !wasRunReady) sfxPlay(SFX_DENY);
  wasRunReady = runReady;

  handleTouch();
#if defined(BOARD_KNOMI_V2) && !defined(BOARD_HOST)
  autoRotateTick(now);
#endif
  visitTick(now);
  buddyTick(now);
  handleSerial();
  ensureMon();

  // short PWR press: screen on/off
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
  if (wifiSetupState() != WS_OFF) lastInteract = now;  // don't let it switch off while being configured
#endif

  updateBrightness(now);

  // battery at the safe limit (no USB): the game is saved and the PMU is powered off before the
  // cell drops further; the RTC keeps the time
  if (batShouldShutdown() && !usbPresent()) {
    pet.flushSave();
    panel->setBrightness(0);
    batPowerOff();
  }

  // flushes the periodic autosave ONLY with the screen dimmed/off or
  // asleep: the NVS write freezes both cores for ~1s (flash cache off),
  // and here there is no animation to cut off nor finger waiting for a response. After 90s
  // of inactivity the screen already dims, so it flushes right away; active use
  // persists anyway through each action's saves (eat/play/...).
  if (pet.savePending() && (screenOff || dimStage >= 1 || pet.sleeping)) {
    pet.flushSave();
  }

#ifdef BOARD_KNOMI_V2
  if (clockPoll()) {  // applies the time spent off as soon as network time is available
    if (clockWasSeed()) pet.setClock(rtcEpoch());
    else pet.syncClock(rtcEpoch());
    if (clockOpen && !clockEdited) clockLoadNow();  // the open time screen also picks it up
  }

  // Wi-Fi updates: this firmware is confirmed once it has run 30 s (a broken update rolls
  // back), and a new one is looked for once a day, only with the screen dimmed: the
  // check holds the loop for a second or two
  static bool bootConfirmed = false;
  if (!bootConfirmed && now > 30000) {
    bootConfirmed = true;
    otaConfirmBoot();
  }
  static uint32_t lastOtaCheck = 0;
  uint32_t otaDue = !lastOtaCheck ? OTA_FIRST_CHECK_MS : otaState() == OTA_FAILED ? 3600000UL : OTA_CHECK_EVERY_MS;
  if ((screenOff || dimStage >= 1) && buddyNetUp() && !pet.ceremony && now - lastOtaCheck >= otaDue) {
    lastOtaCheck = now ? now : 1;
    otaCheck(FW_VERSION);
  }
#endif

  // records the real time every 30 s (persisted on each game save)
  static uint32_t lastClock = 0;
  if (now - lastClock > 30000) {
    lastClock = now;
    uint32_t e = rtcEpoch();
    if (e) pet.lastSeenEpoch = e;
  }

  // health heartbeat every 5 min (for the soak test; dropped if there is no monitor)
  static uint32_t lastHealth = 0;
  if (now - lastHealth > 300000) {
    lastHealth = now;
    Serial.printf("HEALTH up=%lus heap=%u min=%u bat=%d%% mv=%d chg=%d usb=%d dim=%u off=%d\n",
                  (unsigned long)(now / 1000), ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                  batPercent(), batMillivolts(), batCharging() ? 1 : 0,
                  usbPresent() ? 1 : 0, dimStage, screenOff ? 1 : 0);
  }

  // Draw rate (power.h): 10 fps while the pet is in use, lower the longer it goes untouched,
  // 85 ms in game/arcade (safe margin so the redraw doesn't overlap the DMA send of the previous
  // frame: at 40-65 ms it overlapped and caused black flashes).
  // With the screen off nothing is drawn: at brightness 0 nothing is visible, but the redraw and the
  // DMA flush of the canvas framebuffer kept running at 10 fps, and measured that is 70 ms of
  // work per 100 (46 of them just the flush). Each render repaints the whole scene,
  // so nothing is left half-done; and since lastRender stays frozen, the first
  // frame after waking goes out immediately.
  PowerCtx pc = powerCtx(millis());
  audioSetIdle(!amplifierWanted(pc));  // the amplifier draws power at idle
  batSetPollMs(powerCacheMs(pc));
  uint32_t interval = frameIntervalMs(pc);
#ifdef BOARD_HOST
  if (interval > 150) interval = 150;  // no battery to protect, the pet stays alive at idle
#endif
  bool drewFrame = false;
  if (interval && now - lastRender >= interval) {
    lastRender = now;
    drewFrame = render();
  }
  // yields the CPU on passes that don't draw: without this the loop spins at full speed
  // between frames; 1 ms is unnoticeable on touch (read every 20 ms) and the frame isn't delayed
  if (!drewFrame) idleWait(pc);
}

// current power state (see power.h)
PowerCtx powerCtx(uint32_t now) {
  PowerCtx c;
  c.idleMs = idleSince(now, lastInteract);
  c.screenOff = screenOff;
  c.sleeping = pet.sleeping;
#ifdef BOARD_HOST
  if (!c.sleeping) c.idleMs = 0;  // a desktop window has no panel to protect: only the pet's sleep dims it
#endif
  c.gameOpen = gameOpen || battleOpen || quizOpen || runOpen || dungeonOpen || sdOpen || vbOpen;
  c.arcadeOpen = runOpen || dungeonOpen || vbOpen;
  c.usb = usbPresent();
  c.charging = batCharging();
  c.batPct = batPercent();
  return c;
}

// the user has interacted: normal draw rate returns (without waiting for the next
// frame of a slow rate) and the amplifier turns on before anything plays
void wakeUI() {
  uint32_t now = millis();
  uint32_t iv = frameIntervalMs(powerCtx(now));
  bool slow = !iv || iv > 100;  // screen off or slow rate: draw now, without waiting for the frame
  lastInteract = now;
  // the forced frame respects the 85 ms between redraws (otherwise it overlaps the previous DMA)
  if (slow) lastRender = (now - lastRender >= 85) ? 0 : lastRender - 15;
  audioSetIdle(false);
}

// brightness by sleep + inactivity (AMOLED protection) and battery
void updateBrightness(uint32_t now) {
  // visible events wake the screen by themselves
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

// Pause for a loop pass that did not draw. With the screen off, or on but nearly
// off (5 min untouched), and without USB the CPU sleeps (light sleep) until the next frame or
// 200 ms: it wakes on the timer or on a touch (the touch INT goes low). Not with USB, so as
// not to lose the serial port of the SD card: checked uncached, in case it was just plugged in.
void idleWait(PowerCtx c) {
  // don't sleep right after drawing (the frame's DMA flush is still running) nor while an alert is playing
  if (lightSleepAllowed(c) && millis() - lastRender >= 100 && !audioBusy()) {
    c.usb = usbPresentNow();
    if (!lightSleepAllowed(c)) { delay(idleDelayMs(c)); return; }
    if (digitalRead(TP_INT) == LOW) { gTouchIrq = true; delay(1); return; }  // touch pending: don't sleep
    uint32_t ms = POWER_SLEEP_MS;
    uint32_t iv = frameIntervalMs(c);
    if (iv) {  // screen on: until the next frame
      uint32_t due = lastRender + iv - millis();
      if ((int32_t)due < (int32_t)ms) ms = (int32_t)due > 1 ? due : 1;
    }
    esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000);
    gpio_wakeup_enable((gpio_num_t)TP_INT, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
    // the touch interrupt goes back to edge-triggered (as attachInterrupt left it); a touch
    // that has already pulled the line low is registered by hand
    gpio_wakeup_disable((gpio_num_t)TP_INT);
    gpio_set_intr_type((gpio_num_t)TP_INT, GPIO_INTR_NEGEDGE);
    if (digitalRead(TP_INT) == LOW) gTouchIrq = true;
    return;
  }
  delay(idleDelayMs(c));
}

// ---------- serial console (SD provisioning + debugging) ----------

void openClock();  // prototype

void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  if (sdSerialCommand(line)) return;

#ifdef BOARD_KNOMI_V2
  if (line == "SHOT") {  // panel capture: "SHOT 240 240 rotation\n" + RGB565 little endian
    Serial.printf("SHOT %d %d %u\n", PANEL_SIZE, PANEL_SIZE, gPanelRot);
    Serial.write((uint8_t *)gfx->panelBuffer(), PANEL_SIZE * PANEL_SIZE * 2);
    Serial.println("DONE");
    return;
  } else if (line.startsWith("TAP ")) {  // simulated touch in logical coordinates (466): "TAP x y [ms]"
    int a = line.indexOf(' '), b = line.indexOf(' ', a + 1), c = line.indexOf(' ', b + 1);
    touch.inject(line.substring(a + 1, b).toInt(), line.substring(b + 1, c).toInt(), c > 0 ? line.substring(c + 1).toInt() : 120);
    gTouchIrq = true;
    Serial.println("DONE");
    return;
  } else if (line.startsWith("SWIPE ")) {  // simulated gesture: "SWIPE x0 y0 x1 y1"
    int x0, y0, x1, y1;
    if (sscanf(line.c_str(), "SWIPE %d %d %d %d", &x0, &y0, &x1, &y1) == 4) {
      touch.swipe(x0, y0, x1, y1, 240);
      gTouchIrq = true;
    }
    Serial.println("DONE");
    return;
  } else if (line == "WIFISETUP") {  // opens the setup without going through gestures
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
  } else if (line.startsWith("ROT ")) {  // manual rotation (testing): until the sensor says otherwise
    gPanelRot = line.substring(4).toInt() & 3;
    gfx->flush();
    Serial.println("DONE");
    return;
  } else if (line == "ACC") {  // gravity per axis, to choose the tilt axis and sign
    float g[3] = {0, 0, 0};
    bool ok = accel.read(g);
    Serial.printf("acc ok=%d x=%.2f y=%.2f z=%.2f\n", ok, g[0], g[1], g[2]);
    Serial.println("DONE");
    return;
  } else if (line == "OTACHECK") {
    otaCheck(FW_VERSION);
    Serial.printf("ota: state=%u new=%s\n", otaState(), otaNewVersion());
    Serial.println("DONE");
  } else if (line == "OTAGO") {
    startUpdate();
    Serial.println("DONE");
  } else if (line == "WIFIINFO") {
    Serial.printf("wifi: network '%s' state=%u setup=%u\n", clockWifiSsid(), clockWifiStatus(), wifiSetupState());
    Serial.println("DONE");
    return;
  } else if (line.startsWith("WIFITRY ")) {  // test of the failure path, with a made-up password
    wifiSetupTry(line.substring(8).c_str(), "not-the-password");
    Serial.println("DONE");
    return;
  } else if (line.startsWith("BTN ")) {  // simulated press of the physical button: "BTN ms"
    gBtnSimUntil = millis() + line.substring(4).toInt();
    Serial.println("DONE");
    return;
  } else if (line == "PERF") {  // cost of the flush to the panel
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
  if (line == "NOSAVE") {  // on-board testing: nothing reaches the NVS until reboot
    pet.noSave = true;
    Serial.println("nosave=1");
    Serial.println("DONE");
  } else if (line == "AWAKE" && pet.noSave) {  // test mode only
    pet.sleeping = false;
    pet.energy = 100;
    Serial.println("DONE");
  } else if (line == "BTLWIN" && pet.noSave && battleOpen) {
    bFoe.hp = 0;
    battleFinish(true);
    Serial.println("DONE");
  } else if (line.startsWith("TILT ")) {  // simulated tilt, -1..1; "aim" aims by itself; "off" returns to the sensor
    String v = line.substring(5);
    tiltForced = v != "off";
    tiltAim = v == "aim";
    tiltOverride = v.toFloat();
    Serial.println("DONE");
  } else if (line == "RUN") {
    startRun();
    Serial.println("DONE");
  } else if (line == "BUDDY") {
    Serial.printf("buddy: up=%d id=%08lx here=%d current=%u state=%u\n", buddyNetUp(), (unsigned long)buddyNetId(),
                  buddyCount(bPeers), bCur, bsState);
    for (int i = 0; i < BUDDY_MAX; i++)
      if (bPeers[i].on)
        Serial.printf("  %d: %08lx #%d Lv.%u\n", i, (unsigned long)bPeers[i].id, bPeers[i].dex, bPeers[i].level);
    Serial.println("DONE");
  } else if (line.startsWith("FAKEBUDDY ") && pet.noSave) {  // "FAKEBUDDY dex level [nick]": screenshots without a second board
    char nick[12] = "";
    int dex = 0, lv = 0;
    sscanf(line.c_str() + 10, "%d %d %11s", &dex, &lv, nick);
    int slot = -1;
    for (int i = 0; i < BUDDY_MAX && slot < 0; i++)
      if (!bPeers[i].on) slot = i;
    if (dex >= 1 && dex <= DEX_COUNT && slot >= 0) {
      BuddyPeer &p = bPeers[slot];
      p.on = true;
      p.id = 0xFA41 + 2 * slot;
      p.dex = dex;
      p.level = lv > 0 ? lv : 5;
      strlcpy(p.nick, nick, sizeof(p.nick));
      p.seen = millis();
      buddyFake = true;
    }
    Serial.println("DONE");
  } else if (line == "VBBOT" && pet.noSave) {  // Poke Ball Volley plays by itself (testing)
    vbBot = !vbBot;
    Serial.println("DONE");
  } else if (line.startsWith("CATCH ") && pet.noSave) {  // "CATCH dex [S]": into the Pokedex (testing)
    pet.catchWild(line.substring(6).toInt(), line.indexOf(" S") > 0);
    Serial.println("DONE");
  } else if (line.startsWith("SDTEST") && pet.noSave && buddyFake) {  // "SDTEST [session]": a Showdown against the stand-in buddy, for screenshots
    bsId = line.length() > 7 ? (uint32_t)line.substring(7).toInt() | 1 : esp_random() | 1;
    bsKind = BK_SHOWDOWN;
    bsState = BS_PLAY;
    sdStart(true);
    Serial.println("DONE");
  } else if (line == "DUNGEON") {
    startDungeon();
    Serial.println("DONE");
  } else if (line == "DNBOT" && pet.noSave) {  // descends by itself, to see deep floors
    startDungeon();
    dnBot = true;
    Serial.println("DONE");
  } else if (line == "RUNBOT" && pet.noSave) {  // plays by itself, to see high speeds
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
  } else if (line == "ZONES") {
    openZones();
    Serial.println("DONE");
  } else if (line.startsWith("WILD ")) {  // specific foe (testing); "WILD 25 S": a shiny one
    bNextShiny = line.indexOf(" S") > 0 && pet.noSave;
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
      Serial.printf("species #%d %s\n", n, DEX_TBL[n].name);
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
  } else if (line.startsWith("RTCSET ")) {  // RTC only (simulate power-offs in testing)
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
    if (!galleryOpen) galleryUnload();
    Serial.println("DONE");
  } else if (line == "EGGS") {
    // simulates 20 egg rolls (doesn't change the game state)
    for (int i = 0; i < 20; i++) {
      int16_t d = pet.pickEggSpecies();
      Serial.printf("%d:%s(r%u) ", d, DEX_TBL[d].name, DEX_TBL[d].rarity);
    }
    Serial.println();
    Serial.println("DONE");
  } else if (line == "SHINY") {  // toggles shiny on the current one (testing)
    pet.shiny = !pet.shiny;
    Serial.printf("shiny=%d\n", pet.shiny);
    Serial.println("DONE");
  } else if (line.startsWith("NICK ")) {
    pet.rename(line.substring(5).c_str());
    Serial.printf("nick=%s\n", pet.nick);
    Serial.println("DONE");
  } else if (line == "CAREDAY") {  // simulates a new cared-for day (testing)
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
    sfxPlay(SFX_HATCH);  // audio test
    Serial.println("DONE");
  } else if (line == "ABANDON") {
    pet.dbgRunawayReady();  // forces the "ready to run away" state (button test)
    Serial.println("DONE");
  } else if (line == "WIPE") {
    pet.factoryReset();     // wipes NVS and reboots -> new game (starter choice)
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
    Serial.printf("spec=%d lv=%u food=%u joy=%u energy=%u hyg=%u mistakes=%u sd=%d mon=%d bat=%d usb=%d rtc=%u\n",
                  pet.speciesId, pet.level(), pet.fullness, pet.joy, pet.energy,
                  pet.hygiene, pet.careMistakes, sdReady, mon.loaded,
                  batPercent(), usbPresent(), rtcEpoch());
    Serial.printf("weight=%u atk=%u def=%u spe=%u genes=%u/%u/%u tr=%u/%u/%u berry=%d\n",
                  pet.weight, pet.atkStat(), pet.defStat(), pet.speStat(),
                  pet.geneAtk, pet.geneDef, pet.geneSpe,
                  pet.trAtk, pet.trDef, pet.trSpe, pet.berryKnown);
    Serial.printf("shiny=%d streak=%u/%u bond=%u medals=0x%X(%u) nick=%s\n",
                  pet.shiny, pet.streak, pet.bestStreak, pet.bond, pet.medals,
                  pet.totalMedals, pet.nick);
    Serial.println("DONE");
  }
}

// ---------- touch input ----------

bool inPetZone(int16_t x, int16_t y) {
  int n = partyOthers();
  if (n)  // with others around, yours stands in the middle
    return abs(x - PARTY_MINE_X) < (n <= 2 ? 72 : 56) && y > PET_GROUND - (n <= 2 ? 150 : 115) && y < 310;
  return x > 110 && x < 356 && y > 95 && y < 310;
}

// press preview (press.h): the control under the finger, drawn by pressOverlay() on each flush
PressTarget pressT;
bool pressOn = false;
bool pressSoftGone = false;  // this touch moved off a soft target (a buddy): a swipe, no preview till lift
bool pressTargetAt(int16_t x, int16_t y, PressTarget &t);  // prototypes (defined at the end)
void gameTap(int16_t x, int16_t y);

// screens that only repaint on a change must repaint when the highlight moves
extern bool screenKeyValid;
void pressRepaint() {
  screenKeyValid = false;
  galleryDirty = true;
  eggPickDirty = true;
}

void pressUpdate(int16_t x, int16_t y) {
  PressTarget t;
  bool on = !swallowGesture && !pressSoftGone && pressTargetAt(x, y, t);
  if (on && t.soft) {  // a buddy: only for a finger resting on it, so swipes over buddies stay swipes
    if (pressSoftMoved(x - tX0, y - tY0)) pressSoftGone = true;
    on = pressSoftShows(x - tX0, y - tY0, millis() - tStart);
  }
  // where left and right turn pages (buddies, time/settings, the card), a sideways drag is a swipe
  bool pagesSideways = buddyMenuOpen || cardOpen || (clockOpen && wifiSetupState() == WS_OFF);
  if (pagesSideways && abs(x - tX0) > 40 && abs(x - tX0) > abs(y - tY0)) on = false;
  if (on == pressOn && (!on || (t.x == pressT.x && t.y == pressT.y && t.w == pressT.w && t.h == pressT.h))) return;
  pressOn = on;
  if (on) pressT = t;
  pressRepaint();
}

// finger lifted: true if it was on a control and the gesture wasn't a quick swipe
bool pressRelease() {
  bool picked = pressOn && !pressIsFlick(tXl - tX0, tYl - tY0, millis() - tStart);
  pressSoftGone = false;
  if (pressOn) {
    pressOn = false;
    pressRepaint();
  }
  return picked;
}

// the touch is resolved on finger LIFT to tell a tap from a swipe
void handleTouch() {
  static uint32_t lastPoll = 0;
  if (millis() - lastPoll < 20) return;  // 50 Hz is plenty for a finger
  lastPoll = millis();
  // we only touch the bus if the chip signaled via INT or the finger is still down (we
  // need to detect the lift). Reading the sleeping CST9217 hung for ~1s and
  // froze the whole loop; SensorLib does not honor the Wire timeout.
  if (!gTouchIrq && !wasPressed) return;
  gTouchIrq = false;
  // An address-only cycle before reading. Without it, getPoint() hung for
  // exactly 1000 ms (the I2C driver's default timeout, which SensorLib does not
  // bound) and froze the whole loop: that is what was perceived as "the minigame
  // freezes for 2-3 seconds when tapping the ball" (issue #16), because there the taps
  // are fast and back to back.
  //
  // Measured on the board, playing the same thing (~470 reads, score ~50):
  //   without this line: 5 stalls of 1000 ms in 60 s
  //   with it:        0
  // The reject counter came out 0 in both cases, so it does NOT work by
  // skipping reads when the chip doesn't answer: what it does is wake it up,
  // so the next read doesn't find the CST9217 asleep.
  Wire.beginTransmission(TOUCH_ADDR);
  if (Wire.endTransmission() != 0) return;  // no response: retry in 20 ms
  int16_t x, y;
  bool pressed = touch.getPoint(&x, &y, 1) > 0;

#ifdef BOARD_KNOMI_V2
  // wifi password: the key lights up under the finger and is pressed on release, so you can
  // slide to the right one on such a small keyboard
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

  // games: the touch is resolved on finger lift, so that swiping up exits
  // without pressing anything on the way. Only the PokeRun jump fires on touch-down: there every ms counts.
  if (battleOpen || quizOpen || gamesMenuOpen || gameOpen || runOpen || dungeonOpen || zoneScreen || sdOpen || vbOpen) {
    if (pressed) { tXl = x; tYl = y; }  // tXl: aim with the finger where there is no accelerometer
    // in the dungeon, keeping the finger down keeps walking
    if (dungeonOpen && pressed && wasPressed && !swallowGesture && !dnOverT && millis() - tStart > 350 &&
        millis() - dnHoldT > 230 && abs(tXl - tX0) < 40 && abs(tYl - tY0) < 40) {
      dnHoldT = millis();
      dnHeld = true;
      wakeUI();
      dungeonTap(tX0, tY0);
    }
    if (pressed && !wasPressed) {
      tX0 = x;
      tY0 = y;
      tStart = millis();
      dnHeld = false;
      // only wakes if nothing was visible: dimmed, the game in progress continues (in
      // Tilt Catch the screen isn't touched, so it dims while playing)
      swallowGesture = dimStage >= DIM_OFF || screenOff;
      screenOff = false;
      wakeUI();
      if (runOpen && !swallowGesture) runJump = true;
    }
    if (pressed) {
      pressUpdate(x, y);
    } else if (wasPressed) {
      wakeUI();
      bool picked = pressRelease();
      if (!swallowGesture) {
        if (picked) {
          gameTap(tXl, tYl);  // the control under the finger when it lifted
        } else if (swipeUpQuit(tX0, tY0, tXl, tYl, millis() - tStart)) {
          if (zoneScreen) zoneBack();
          else quitGame();
        } else if (zoneScreen && abs(tXl - tX0) >= 40 && abs(tYl - tY0) < abs(tXl - tX0)) {
          zoneTurnPage(tXl > tX0 ? -1 : 1);  // left advances, as in the Pokedex
        } else if (abs(tXl - tX0) < 40 && abs(tYl - tY0) < 40) {
          gameTap(tX0, tY0);
        }
      }
    }
    wasPressed = pressed;
    return;
  }

  if (pressed && !wasPressed) {  // gesture starts
    tX0 = tXl = x;
    tY0 = tYl = y;
    tStart = millis();
    holdFired = false;
    swallowGesture = (dimStage > 0) || screenOff;  // if it was dark, it only wakes
    screenOff = false;
    wakeUI();
  } else if (pressed) {  // still held down
    tXl = x;
    tYl = y;
    // long press without moving on the pet -> release dialog
    if (!holdFired && !swallowGesture && !galleryOpen && !cardOpen && !kbOpen && !clockOpen && !buddyMenuOpen && !tradeOpen && millis() - tStart > 3000 &&
        abs(tXl - tX0) < 30 && abs(tYl - tY0) < 30 && inPetZone(tX0, tY0) &&
        !pet.isEgg() && !confirmUntil && !pet.ceremony) {
      confirmUntil = millis() + 10000;
      holdFired = true;
    }
  }
  if (pressed) {
    if (!holdFired) pressUpdate(x, y);
  } else if (wasPressed) {  // finger lifts: resolve gesture
    wakeUI();
    int dx = tXl - tX0, dy = tYl - tY0;
    uint32_t dt = millis() - tStart;
    bool picked = pressRelease();
    if (!holdFired && !swallowGesture) {
      if (picked) onTap(tXl, tYl);  // the control under the finger when it lifted
      else if (abs(dx) > 80 && abs(dy) < 70 && dt < 800) onSwipe(dx > 0 ? 1 : -1);
      else if (abs(dy) > 80 && abs(dx) < 70 && dt < 800) onSwipeV(dy > 0 ? 1 : -1);
      else if (dt < 1500 && abs(dx) < 40 && abs(dy) < 40) onTap(tX0, tY0);
    }
  }
  wasPressed = pressed;
}

// vertical swipe: opens/closes the pet card
void openClock();  // prototype

void onSwipeV(int dir) {
  if (playMenuOpen) {  // up: back to the buddy menu
    if (dir < 0) { playMenuOpen = false; buddyMenuOpen = true; }
    return;
  }
  if (pet.awaitingStarter() || pet.awaitingEggPick() || buddyMenuOpen || tradeOpen) return;  // blocked while choosing
  if (gameOpen || galleryOpen || kbOpen || battleOpen || quizOpen || gamesMenuOpen || pet.ceremony) return;
  if (clockOpen) {
    if (wifiSetupState() == WS_PASS) return;  // a shifted touch must not discard the typed password
    wifiSetupStop();
    clockOpen = false;
    return;
  }
  if (cardOpen) {
    if (dir < 0 && !cardAch) cardOpen = false;  // up closes the card
    return;
  }
  if (dir > 0) {                    // swipe down: set the time
    if (!confirmUntil && !feedMenuUntil) openClock();
  } else if (!pet.isEgg() && !confirmUntil && !feedMenuUntil) {
    cardOpen = true;                // swipe up: card
    cardAch = false;
    cardPage = 0;
  }
}

// swipe: dir +1 = to the right
void onSwipe(int dir) {
  if (buddyMenuOpen) {  // several buddies: left and right go through them
    for (int k = 1; k < BUDDY_MAX && bsState == BS_IDLE; k++) {
      int i = (bCur + (dir > 0 ? BUDDY_MAX - k : k)) % BUDDY_MAX;
      if (bPeers[i].on) {
        buddySelect(i);
        break;
      }
    }
    return;
  }
  if (tradeOpen) {
    int pages = (pet.formCount() + 15) / 16;
    int np = trPage - dir;
    if (trPhase == TR_PICK && np >= 0 && np < pages) trPage = np;
    return;
  }
  if (pet.awaitingEggPick()) {
    int pages = (pet.registeredCount() + 15) / 16;
    int np = eggPickPage - dir;  // swiping left advances a page
    if (!eggPickDetail && np >= 0 && np < pages) {
      eggPickPage = np;
      eggPickDirty = true;
    }
    return;
  }
  if (pet.awaitingStarter()) return;  // blocked during the starter choice
#if defined(BOARD_KNOMI_V2) && !defined(BOARD_HOST)  // on the host, wifi is the system's business: there is no page
  if (clockOpen && wifiSetupState() == WS_OFF) {  // time <-> settings; left advances
    uint8_t next = dir > 0 ? 0 : 1;
    if (next == 0 && clockPage != 0 && !clockEdited) clockLoadNow();  // may have synced in settings
    clockPage = next;
    return;
  }
#endif
  if (gameOpen || kbOpen || clockOpen) return;
  if (cardOpen) {  // inside the card or the achievements: switch pages
    int p = (int)cardPage + (dir > 0 ? -1 : 1);  // left advances
    if (p < 0 && cardAch) {  // back past the first: home, as the Pokedex does
      cardOpen = cardAch = false;
      return;
    }
    cardPage = p < 0 ? 0 : (p > cardPageCount() - 1 ? cardPageCount() - 1 : p);
    return;
  }
  if (!galleryOpen && dir < 0) {  // right to left on home: the achievements, off to the right
    if (!pet.ceremony && !confirmUntil && !pet.isEgg()) {
      cardOpen = cardAch = true;
      cardPage = 0;
    }
    return;
  }
  if (!galleryOpen) {  // left to right: the Pokedex, off to the left
    if (!pet.ceremony && !confirmUntil) {
      galleryOpen = true;
      galleryPage = 0;
      galleryDetail = 0;
      galleryDirty = true;
    }
    return;
  }
  if (galleryDetail) {  // in detail: back to the grid
    galleryDetail = 0;
    galleryUnload();
    galleryDirty = true;
    return;
  }
  int np = galleryPage - dir;  // swiping left advances a page
  if (np < 0) {                // going back from the first = exit
    galleryOpen = false;
    galleryUnload();
    return;
  }
  if (np > 9) np = 9;
  if (np != galleryPage) {
    galleryPage = np;
    galleryDirty = true;
  }
}

void onTap(int16_t x, int16_t y) {
  // Serial.printf("TOUCH %d %d\n", x, y);  // diagnostic (silenced: floods the log)
  if (pet.awaitingStarter()) {  // first game: choose starter
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
  if (buddyMenuOpen) {
    buddyMenuTap(x, y);
    return;
  }
  if (playMenuOpen) {
    playMenuTap(x, y);
    return;
  }
  if (tradeOpen) {
    tradeTap(x, y);
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
  if (pet.ceremony) return;  // no buttons during the farewell
  if (cardOpen) {
    if (!cardAch && cardPage == 0 && y < cardNameBottom + 12) openKeyboard();  // tap the name (or its pencil) = rename
#ifndef BOARD_KNOMI_V2  // on the Knomi, battle opens with the physical button
    else if (!cardAch && cardPage == 1 && y >= 300 && y <= 340 && x >= 96 && x <= 370) {
      cardOpen = false;            // battle button
      startBattle();
    }
#endif
    else if (closeHit(x, y)) {
      sfxPlay(SFX_TAP);
      cardOpen = cardAch = false;
    }
    return;
  }
  if (choiceKind) {          // decision dialog: action button (top) / keep (bottom)
    bool b1 = (x >= 93 && x <= 373 && y >= 206 && y <= 258);  // action
    bool b2 = (x >= 93 && x <= 373 && y >= 268 && y <= 320);  // keep / stay together
    if (choiceKind == 1) {                 // evolution
      if (b1) { int16_t old = pet.speciesId; pet.evolve(); evoPmd.load(old, pet.shiny); }
      else if (b2) pet.declineEvolve();
    } else if (choiceKind == 2) {          // farewell
      if (b1) pet.startFarewell();
      else if (b2) pet.declineFarewell();
    } else if (choiceKind == 4) {          // runaway: caring for it is the top option
      if (b1) pet.rescue();
      else if (b2) pet.startRunaway();
    } else if (choiceKind == 5) {          // the buddy invites
      if (b1) buddyAcceptInvite();
      else buddyDeclineInvite();
    } else if (choiceKind == 3 && b1) {    // delete the game (Knomi button)
      pet.factoryReset();
      delay(100);
      ESP.restart();
    }
    choiceKind = 0;
    return;
  }
  if (confirmUntil) {        // "release?" dialog: YES / NO
    if (timeLeft(confirmUntil) && x >= 118 && x <= 218 && y >= 252 && y <= 304) {
      pet.release();
    }
    confirmUntil = 0;
    return;
  }
  if (feedMenuUntil) {       // food picker
    int count = pet.canHaveCandy() ? 4 : 3;
    int x0 = CX - count * 33;
    if (timeLeft(feedMenuUntil) && y >= 288 && y <= 352 && x >= x0 && x <= x0 + count * 66) {
      int item = (x - x0) / 66;
      if (item >= count) item = count - 1;
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
  int bt = buddyAt(x, y);
  if (bt >= 0) {
    sfxPlay(SFX_TAP);
    if (bsState == BS_IDLE) buddySelect(bt);
    buddyMenuOpen = true;
    return;
  }
  // evolution button: opens the evolve/keep dialog
  if (pet.wantEvolveButton() && x >= EVO_BTN_X && x <= EVO_BTN_X + EVO_BTN_W &&
      y >= EVO_BTN_Y && y <= EVO_BTN_Y + EVO_BTN_H) {
    choiceKind = 1; choiceUntil = millis() + 12000;
    return;
  }
  // ending buttons (same box): both open a dialog
  if (x >= FAR_BTN_X && x <= FAR_BTN_X + FAR_BTN_W &&
      y >= FAR_BTN_Y && y <= FAR_BTN_Y + FAR_BTN_H) {
    if (pet.canRunawayNow()) { choiceKind = 4; choiceUntil = millis() + 12000; return; }
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
  // tap the pet = caress
  if (inPetZone(x, y)) {
    Serial.println("PET");
    pet.caress();
    if (!pet.sleeping) sfxPlay(SFX_HEART);
  }
}

// ---------- render ----------

bool gNight = false;  // real night (by time) or sleeping: set by render()
uint16_t inkColor() { return gNight ? UI_INK_NIGHT : UI_INK; }

// ---------- background scene: type biome + real RTC time ----------

#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define HORIZON 232  // line where the sky meets the ground

uint16_t lerp565(uint16_t a, uint16_t b, int i, int n) {
  if (n <= 0) return a;
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)((((ar + (br - ar) * i / n) << 11)) |
                    (((ag + (bg - ag) * i / n) << 5)) | (ab + (bb - ab) * i / n));
}

// hour of day 0-23 (from the real time cached every 30s; 13 if there is no clock)
int sceneHour() {
  uint32_t e = pet.lastSeenEpoch;
  return e ? (int)((e / 3600) % 24) : 13;
}

// ground of each biome by day (at night it blends toward the night blue)
static const uint16_t BIOME_SOIL[6] = {
  C565(0x7e, 0xc0, 0x7f),  // 0 meadow
  C565(0xdc, 0xca, 0x94),  // 1 beach (sand)
  C565(0x4f, 0x8a, 0x55),  // 2 forest
  C565(0x8a, 0x55, 0x44),  // 3 volcano
  C565(0xa8, 0x90, 0x6a),  // 4 mountain
  C565(0xe6, 0xee, 0xf5),  // 5 snow
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
  else if (h < 8)       { top = C565(0xd1, 0x6a, 0x86); bot = C565(0xf3, 0xb8, 0x7c); }  // dawn
  else if (h < 18)      { top = C565(0x8f, 0xc8, 0xea); bot = C565(0xdc, 0xee, 0xe6); }  // day
  else                  { top = C565(0xc7, 0x5a, 0x4a); bot = C565(0xf0, 0xae, 0x64); }  // dusk

  // sky in bands
  for (int y = 0; y < HORIZON; y += 8)
    gfx->fillRect(0, y, 466, 8, lerp565(top, bot, y, HORIZON));

  // sun or moon
  if (night) {
    // below the name and level, so it doesn't cover them
    gfx->fillCircle(360, 134, 24, C565(0xe8, 0xee, 0xf5));
    gfx->fillCircle(370, 128, 22, lerp565(top, bot, 134, HORIZON));  // crescent
    for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
  } else if (h < 18) {
    gfx->fillCircle(360, 138, 26, h < 8 ? C565(0xff, 0xd9, 0x8a) : C565(0xff, 0xe7, 0x9f));
    drawClouds(now, C565(0xff, 0xff, 0xff));
  } else {
    gfx->fillCircle(233, HORIZON - 6, 34, C565(0xff, 0xf1, 0xc8));  // setting sun
  }

  // beach sea: a strip of water over the sand
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

  // ground
  gfx->fillRect(0, HORIZON, 466, 466 - HORIZON, soil);
  uint16_t hill = lerp565(soil, night ? C565(0x0c, 0x12, 0x24) : C565(0xff, 0xff, 0xff), 3, 16);
  gfx->fillRoundRect(-60, HORIZON - 14, 586, 60, 30, hill);

  // biome details
  uint16_t dk = lerp565(soil, C565(0x10, 0x18, 0x20), night ? 11 : 7, 16);
  if (biome == 2) {  // forest: conifers in silhouette
    for (int tx : { 60, 150, 360, 416 }) {
      gfx->fillTriangle(tx, HORIZON - 46, tx - 16, HORIZON, tx + 16, HORIZON, dk);
      gfx->fillTriangle(tx, HORIZON - 60, tx - 12, HORIZON - 28, tx + 12, HORIZON - 28, dk);
    }
  } else if (biome == 3) {  // volcano: rocks and embers
    gfx->fillTriangle(70, HORIZON, 40, HORIZON + 30, 100, HORIZON + 30, dk);
    gfx->fillTriangle(400, HORIZON + 4, 372, HORIZON + 30, 430, HORIZON + 30, dk);
    if (!night)
      for (int e = 0; e < 4; e++)
        gfx->fillRect(120 + e * 70, HORIZON + 8 + (e % 2) * 6, 4, 4, C565(0xff, 0x9b, 0x3a));
  } else if (biome == 4) {  // mountain: peaks in the background
    gfx->fillTriangle(140, HORIZON - 50, 60, HORIZON, 220, HORIZON, dk);
    gfx->fillTriangle(330, HORIZON - 38, 250, HORIZON, 410, HORIZON, dk);
  } else if (biome == 5 && !night) {  // snow: flakes falling
    for (int f = 0; f < 10; f++) {
      int fx = (f * 53 + now / 40) % 466;
      int fy = (f * 90 + now / 18) % HORIZON;
      gfx->fillRect(fx, fy, 3, 3, UI_WHITE);
    }
  } else if (biome == 0) {  // meadow: tufts of grass
    for (int gx : { 80, 175, 300, 395 })
      for (int b = -1; b <= 1; b++)
        gfx->fillRect(gx + b * 5, HORIZON + 6, 2, 8 + (b == 0 ? 4 : 0), dk);
  }
}

// first game: choose a starter among Bulbasaur / Charmander / Squirtle
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
    const uint8_t *th = thumbs.get(d);     // starter thumbnail (if the SD is ready)
    if (th) drawThumb(th, 76, ry - 5, 3, false);
    gfx->setTextColor(UI_INK);
    setSize(3);
    setCur(178, ry + 24);
    printT(dexName(d));
  }
  gfx->flush();
}

// ---------- text: size and cursor (CJK-font ready) ----------
// The classic font anchors the cursor at the TOP CORNER of the text, and the 74
// Y coordinates in the code are written that way. U8g2 fonts anchor it at the
// BASELINE (drawChar does curY = y - baseline*size), so when switching
// font all the text would shift up. setCur() compensates for that difference in one
// place, so that Y keeps meaning "top" with any font.
//
// setSize() exists only because the library has textsize_x protected and setCur()
// needs to know the active scale to multiply the ascent.
//
// With the classic font gFontAscent is 0, so setCur(x,y) is exactly
// setCur(x,y): this doesn't move a pixel in the six current languages.
uint8_t gTextSize = 1;
int gFontAscent = 0;  // px from the top edge to the baseline, 0 = classic font

// --- CJK font tuning (the two values go together) ---
// The classic font is 8 px tall at scale 1. If the CJK one is taller, the scale has to
// be divided so the sizes match... but dividing THINS the
// stroke: at scale 1 unifont draws 1 px lines while Latin at scale
// 2 draws them at 2, and Japanese looks faint even though it is the same size.
//
// So a small base font that is also BOLD (_b_) is desirable, allowing the
// SAME scale as Latin: that way the thickness matches and no division is needed.
//
// A truly bold font (b10_b) was tried and does NOT work at this size: the
// strokes merge and the kana lose detail, unreadable even for a native
// reader. We go back to unifont, which is the most legible, and the thin stroke is
// compensated in printT() by repainting shifted 1 px (pseudo-bold).
// japanese3 and not japanese1: subsets 1 and 2 have kana and kanji but
// NO CJK punctuation, and Arduino_GFX draws nothing when the
// glyph is missing (nor does it advance the cursor), so the 25 ！ and ？ marks in the
// strings vanished. It is the same unifont family: identical metrics header and
// a strict superset, checked glyph by glyph over the 226 codepoints
// the firmware uses. It costs 102 KB more flash and doesn't move a pixel.
// Korean needs a font with hangul, and CJK_FONT was a single #define.
// Instead of assuming one subset serves both scripts, the
// font is chosen per language. korean2 and not korean1: the Korean here uses 296
// distinct syllables and all of them are in KS X 1001, the 2350-set that is the
// size measured for korean2; korean1 has 478. The check looks at
// membership in KS X 1001, it doesn't walk the font table, so it relies
// on korean2's set being that one; ASCII is assumed present.
// Not checked on the board: whether CJK_SIZE_DIV 2 suits korean2 the same as the
// Japanese one. The ascent is measured at runtime in applyLangFont().
#define CJK_FONT_JA u8g2_font_unifont_t_japanese3
#define CJK_FONT_KO u8g2_font_unifont_t_korean2
#define CJK_SIZE_DIV 2

void setSize(uint8_t n) {
  // gTextSize stores the scale ACTUALLY applied, not the requested one: setCur()
  // multiplies the ascent by it and it has to match what is drawn.
  gTextSize = gCjkFont ? (n >= CJK_SIZE_DIV ? n / CJK_SIZE_DIV : 1) : n;
  gfx->setTextSize(gTextSize);
}

void setCur(int x, int y) {
  gfx->setCursor(x, y + gFontAscent * gTextSize);
}

// Sets the active language's font. It is the ONLY place that touches it: the rest of the
// code draws the same for all languages thanks to textW()/setCur().
//
// The ascent is measured instead of hard-coded: getTextBounds() returns y1 as the
// offset of the top edge relative to the cursor, negative with U8g2
// fonts (which anchor at the baseline). That way setCur() can keep treating Y
// as "top" whatever the font, with no per-font magic constants.
void applyLangFont() {
  gCjkFont = LANG_IS_CJK(gLang);
  if (!gCjkFont) {
    gfx->setFont();            // classic 5x7, CP437: Y is already the top edge
    gfx->setUTF8Print(false);
    gFontAscent = 0;
    return;
  }
  gfx->setFont((gLang == LANG_KO) ? CJK_FONT_KO : CJK_FONT_JA);
  gfx->setUTF8Print(true);     // CJK strings are multibyte UTF-8
  int16_t x1, y1;
  uint16_t w, h;
  uint8_t before = gTextSize;
  gfx->setTextSize(1);
  gfx->getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
  gFontAscent = -y1;           // y1 negative: go up from the baseline
  gfx->setTextSize(before);
}

// ---------- text measurement (CJK-font ready) ----------
// The classic Arduino_GFX font advances 6 px per character at size 1, and the UI
// centered with a bare strlen(s)*6*n. That only works for one byte per character and
// fixed width: with a U8g2 font for Japanese/Korean/Chinese, strlen() neither counts
// characters (UTF-8 is multibyte) nor do they all have the same width. Centralizing it here is
// what will allow adding those languages without touching every draw point.
//
// For the classic font it returns EXACTLY the same count as before,
// so this change doesn't move a pixel in the six current languages.
uint16_t textW(const char *s, uint8_t size) {
  if (!gCjkFont) return (uint16_t)strlen(s) * 6 * size;
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return w;
}

// cursor x to leave the text centered on CX
int centerX(const char *s, uint8_t size) { return CX - textW(s, size) / 2; }

// Prints respecting the active font. With CJK it repaints the text shifted by
// 1 px: unifont at reduced scale leaves 1 px strokes and looks faint, and a
// truly bold font at this size merges the strokes and ruins the kana.
// Repainting thickens the stroke without distorting the glyph. Tested on the board by
// usakomint, who compared both options in Japanese.
// The nickname keyboard is alphabet only (left that way on purpose), so
// there is never any CJK here and nothing needs thickening.
void printT(char c) { gfx->print(c); }

void printT(const char *s) {
  if (gCjkFont) {
    int16_t x = gfx->getCursorX(), y = gfx->getCursorY();
    gfx->print(s);
    gfx->setCursor(x + 1, y);
  }
  gfx->print(s);
}

// Screens that only change on a touch (or with data that changes very rarely):
// while their "key" doesn't change they are neither repainted nor flushed over QSPI.
static bool staticScreen() {
  if (pet.awaitingStarter()) return true;
  if (pet.awaitingEggPick() || buddyMenuOpen || playMenuOpen || tradeOpen || sdOpen || vbOpen || galleryOpen || gameOpen || battleOpen || quizOpen || gamesMenuOpen || zoneScreen || runOpen || dungeonOpen) return false;  // the gallery manages its own repainting
  if (kbOpen || clockOpen) return true;
  return cardOpen && (cardAch || cardPage != 0);  // the card's page 0 has the animated portrait
}
static uint32_t mixKey(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }
static uint32_t screenKey() {
  uint32_t h = 2166136261u;
  h = mixKey(h, pet.awaitingStarter() ? 1 : kbOpen ? 2 : clockOpen ? 3 : 4 + cardPage + (cardAch ? 16 : 0));
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
    h = mixKey(h, otaState() * 2 + otaChecking);
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
                        (uint32_t)pet.berryKnown, (uint32_t)pet.battleWins, (uint32_t)pet.trainerMedals, (uint32_t)gLang })
      h = mixKey(h, v);
    for (const char *c = pet.nick; *c; c++) h = mixKey(h, (uint8_t)*c);
  }
  return h;
}
bool screenKeyValid = false;  // (the host renderer invalidates it between scenes)
uint32_t lastScreenKey = 0;

// Draws a frame and returns true if it flushed it to the panel.
bool render() {
  if (staticScreen()) {
    uint32_t k = screenKey();
    if (screenKeyValid && k == lastScreenKey) return false;
    lastScreenKey = k;
    screenKeyValid = true;
  } else {
    screenKeyValid = false;
  }
  if (pet.awaitingStarter()) {  // first game: choose starter (top priority)
    renderStarterSelect();
    return true;
  }
  if (pet.awaitingEggPick()) {
    renderEggPick();
    return true;
  }
  if (buddyMenuOpen) {
    renderBuddyMenu();
    return true;
  }
  if (playMenuOpen) {
    renderPlayMenu();
    return true;
  }
  if (sdOpen) {
    renderShowdown();
    return true;
  }
  if (vbOpen) {
    renderVolley();
    return true;
  }
  if (tradeOpen) {
    renderTrade();
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
  if (dungeonOpen) {
    renderDungeon();
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
  if (zoneScreen) {
    renderZones();
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
  // drawScene covers the full 466x466: no prior fillScreen(BLACK) so
  // an overlapping DMA flush never captures half-black (anti-flicker)
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
    if (pet.eggRarity() >= R_RARE) {
      const char *rar = (pet.eggRarity() == R_LEGENDARY) ? T(S_EGG_LEGEND) : T(S_EGG_RARE);
      gfx->setTextColor(pet.eggRarity() == R_LEGENDARY ? UI_BAR_WARN : 0x4C98);
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
    // bottom panel: clean base for bars and buttons over the landscape
    gfx->fillRect(0, 312, 466, 154, gNight ? UI_BG_NIGHT : UI_BG_DAY);
    drawBars();
    drawButtons();
    drawCelebration();
    if (pet.wantEvolveButton()) drawEvolveButton();        // red CTA: evolve
    else if (pet.canRunawayNow()) drawRunawayButton();     // grim CTA: runaway (neglect)
    else if (pet.wantFarewellButton()) drawFarewellButton();  // gold CTA: farewell
  }

  if (pet.sleeping) {
    gfx->setTextColor(UI_INK_NIGHT);
    setSize(3);
    setCur(320, 130);
    printT("Zz");
  }

  // food picker
  if (feedMenuUntil) {
    if (!timeLeft(feedMenuUntil)) {
      feedMenuUntil = 0;
    } else {
      // once the day's candies are used up, the menu stays at the three berries
      int count = pet.canHaveCandy() ? 4 : 3;
      int x0 = CX - count * 33;
      gfx->fillRoundRect(x0, 288, count * 66, 64, 14, UI_WHITE);
      gfx->drawRoundRect(x0, 288, count * 66, 64, 14, inkColor());
      static const char *const *const FOOD[4] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_CANDY };
      for (int i = 0; i < count; i++) drawMap(FOOD[i], HOME_ICON_N, x0 + 6 + 66 * i, 293, 3, false);
    }
  }

  // "release?" dialog (long press on the pet)
  if (confirmUntil) {
    if (!timeLeft(confirmUntil)) {
      confirmUntil = 0;
    } else {
      gfx->fillRoundRect(94, 168, 278, 152, 16, UI_WHITE);
      gfx->drawRoundRect(94, 168, 278, 152, 16, UI_INK);
      // 48 and not 28: the buffer was sized when every string was one byte
      // per character. With a UTF-8 row, "%s 놓아줄까요?" plus the name goes
      // over 28 and snprintf cuts in the middle of a 3-byte sequence, which the
      // font can no longer draw.
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

  drawBuddyTop();

  // decision dialog (evolve/keep, say goodbye/stay together)
  if (choiceKind) {
    if (!timeLeft(choiceUntil)) choiceKind = 0;
    else drawChoiceDialog();
  }

  gfx->flush();
  return true;
}

// ---------- minigame: taps with the pokeball ----------

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

// the falling berry (ballX/ballY) spawns from any point that fits on the round screen
void respawnBall() {
  ballX = 96 + random(275);
  ballY = 96;
  ballVX = 0;
  float sp = 8.0f + gameScore * 0.25f;  // faster and faster
  ballVY = sp > 18.0f ? 18.0f : sp;
}

// swipe up in any game: out immediately and with no reward (the reward is given when
// the game ends, so closing is enough)
void quitGame() {
  sfxPlay(SFX_TAP);
  if (sdOpen) sdQuit();
  if (vbOpen) vbQuit();
  if (battleOpen) closeBattle();
  if (quizOpen) closeQuiz();
  if (runOpen) closeRun();
  if (dungeonOpen) closeDungeon();
  gameOpen = gamesMenuOpen = false;
}

// tilt zero: whatever it reads now is "straight"
#if defined(BOARD_KNOMI_V2) && !defined(BOARD_HOST)
// the screen follows gravity, except with a finger on it or when tilting is part of the game
void autoRotateTick(uint32_t now) {
  static uint32_t last = 0;
  static RotFilter filter;
  if (now - last < 100) return;
  last = now;
  if (screenOff || wasPressed || gameOpen || vbOpen || (battleOpen && bPhase >= BP_AIM)) {
    filter.reset();
    return;
  }
  float g[3];
  if (!accel.read(g)) return;
  uint8_t rot = gPanelRot;
  if (filter.update(g[0], g[1], now, &rot)) {
    gPanelRot = rot;
    gfx->flush();  // some screens only redraw on change
    Serial.printf("screen: rotation %u\n", rot);
  }
}
#endif

float tiltRaw() {
#ifdef BOARD_KNOMI_V2
  float g[3];
  if (accel.read(g)) return rotTilt(g, gPanelRot);
#endif
  return 0;
}

void tiltZero() {
#ifndef BOARD_HOST  // with arrow keys, straight is straight even if the key is already pressed at the start
  tiltBase = tiltRaw();
#endif
  tiltSmooth = 0;
}

// -1..1. About 30 degrees reaches the limit. Without an accelerometer, aim with the finger.
float playerTilt() {
  if (tiltForced) return tiltOverride;
  float t;
#ifdef BOARD_KNOMI_V2
  if (accel.ready()) t = (tiltRaw() - tiltBase) / 0.5f;
  else
#endif
    t = wasPressed ? (tXl - CX) / 150.0f : 0;
  if (t > -0.06f && t < 0.06f) t = 0;  // dead zone: a shaky hand moves nothing
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
  if (ballY >= 330 && ballY <= 392 && fabsf(ballX - gamePetX) < 54) {  // into the mouth
    if (gameScore < 255) gameScore++;
    hitX = ballX;
    hitY = ballY;
    hitTime = now;
    sfxPlay(SFX_PLAY);
    respawnBall();
  } else if (ballY > 392) {  // to the ground
    sfxPlay(SFX_DENY);
    if (++gameMisses >= 3) {
      gameNewHi = (gameScore > pet.gameHi);
      pet.playResult(gameScore);  // updates the record and gives happiness
      sfxPlay(gameNewHi && gameScore > 0 ? SFX_MEDAL : SFX_LEVEL);
      gameOverUntil = now + 3500;
      return;
    }
    respawnBall();
  }
}

// ---------- punching bag (trains strength) ----------

// ---------- games menu ----------
#define GAMES_ROW_X 83
#define GAMES_ROW_W 300
#define GAMES_ROW_H 48
#define GAMES_ROW_GAP 8
#define GAMES_ROW_Y 112
#ifdef BOARD_KNOMI_V2
#define GAMES_COUNT 5
#else
#define GAMES_COUNT 5  // without a physical button, the battle lives here
#endif

void openGamesMenu() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) { sfxPlay(SFX_DENY); return; }
  gamesMenuOpen = true;
  gamesMenuUntil = millis() + 20000;
}

const char *gameLabel(int row) {
#ifdef BOARD_KNOMI_V2
  static const StrId L[5] = { S_GAME_BATTLE, S_GAME_DUNGEON, S_GAME_RUN, S_GAME_WHO, S_GAME_TILT };
#else
  static const StrId L[5] = { S_GAME_TILT, S_GAME_WHO, S_GAME_RUN, S_GAME_DUNGEON, S_BTL_START };
#endif
  return T(L[row]);
}

void gamesMenuTap(int16_t x, int16_t y) {
  gamesMenuOpen = false;
  int row = (y - GAMES_ROW_Y) / (GAMES_ROW_H + GAMES_ROW_GAP);
  if (y < GAMES_ROW_Y || row >= GAMES_COUNT || x < GAMES_ROW_X || x > GAMES_ROW_X + GAMES_ROW_W) return;  // outside = go back
  sfxPlay(SFX_TAP);
#ifdef BOARD_KNOMI_V2
  if (row == 0) openZones();
  else if (row == 1) startDungeon();
  else if (row == 2) startRun();
  else if (row == 3) startQuiz();
  else startGame();
#else
  if (row == 0) startGame();
  else if (row == 1) startQuiz();
  else if (row == 2) startRun();
  else if (row == 3) startDungeon();
  else startBattle();
#endif
}

void renderGamesMenu() {
  if (!timeLeft(gamesMenuUntil) || pet.sleeping || pet.ceremony) { gamesMenuOpen = false; return; }
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_GAMES), 3), 74);
  printT(T(S_GAMES));
#ifdef BOARD_KNOMI_V2
  const uint16_t colors[5] = { UI_BAR_BAD, TYPE_ACCENT[TY_GHOST], TYPE_ACCENT[TY_FIRE], TYPE_ACCENT[TY_PSYCHIC], TYPE_ACCENT[TY_GRASS] };
#else
  const uint16_t colors[5] = { TYPE_ACCENT[TY_GRASS], TYPE_ACCENT[TY_PSYCHIC], TYPE_ACCENT[TY_FIRE], TYPE_ACCENT[TY_GHOST], UI_BAR_BAD };
#endif
  setSize(2);
  for (int i = 0; i < GAMES_COUNT; i++) {
    int y = GAMES_ROW_Y + i * (GAMES_ROW_H + GAMES_ROW_GAP);
    gfx->fillRoundRect(GAMES_ROW_X, y, GAMES_ROW_W, GAMES_ROW_H, 14, colors[i]);
    gfx->setTextColor(UI_WHITE);
    setCur(centerX(gameLabel(i), 2), y + 16);
    printT(gameLabel(i));
  }
  drawCloseButton();  // any tap outside the rows closes it too
  gfx->flush();
}

// ---------- who's that pokemon ----------
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
  if (!quizPmd.loaded) return;  // without sprites there is no silhouette to show
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
  gfx->fillCircle(CX, CY, 231, TYPE_ACCENT[TY_WATER]);
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
    int inset = i == 2 ? 26 : 0;  // the bottom one, narrower: the circle closes in
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

// ---------- daily visit ----------
#define VISIT_ANNOUNCE_MS 3500UL
#define VISIT_ENTER_MS 2500UL
#define VISIT_HANG_MS 40000UL
#define VISIT_LEAVE_MS 3500UL
#define VISIT_X 368  // where the visitor stands: a buddy's place (partyBuddyX(1, 1))

void startVisitWith(int16_t dex) {
  if (visitPhase != VP_NONE || buddyCount(bPeers) || pet.isEgg() || pet.sleeping || pet.ceremony) return;
  if (dex < 1 || dex > DEX_COUNT) dex = pet.pickWildSpecies(btlCapLevel(pet.level()));
  visitPmd.load(dex, false);
  if (!visitPmd.loaded) return;
  visitDex = dex;
  visitPhase = VP_ANNOUNCE;
  visitT = millis();
  pet.visitorArrived();
  sfxPlay(SFX_HEART);
  Serial.printf("visit: #%d\n", dex);
}

void endVisit() {
  visitPhase = VP_NONE;
  visitPmd.unload();
}

// checks the time and advances the visit, even with another screen open on top
bool onMainScreen() {
  return !gameOpen && !runOpen && !dungeonOpen && !battleOpen && !quizOpen && !gamesMenuOpen && !zoneScreen && !galleryOpen && !cardOpen &&
                      !kbOpen && !clockOpen && !choiceKind && !confirmUntil && !buddyMenuOpen && !tradeOpen &&
                      !playMenuOpen && !sdOpen && !vbOpen;
}

void visitTick(uint32_t now) {
  if (visitPhase == VP_NONE) {
    if (now - visitCheckT < 20000) return;
    visitCheckT = now;
    bool mainScreen = onMainScreen();
    // with the screen dimmed nobody is looking: better to wait until someone returns
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
  if (visitPhase == VP_NONE) {
    drawBuddyPet(now);
    return;
  }
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
  drawPmdActM(visitPmd, act, x, PET_GROUND, now - visitT, true, false, partyOthersScale(1));
  if (visitPhase == VP_HANG && (t / 900) % 4 == 0) drawMap(SPR_HEART, 32, 286, PET_GROUND - 150, 1, false);
  if (visitPhase == VP_LEAVE) {
    char b[40];
    snprintf(b, sizeof(b), T(S_VISIT_BYE_FMT), dexName(visitDex));
    drawVisitBanner(b);
  }
}

#ifdef BOARD_KNOMI_V2
// Knomi BOOT button (GPIO0, active low): tap = wild foe, 1-3 s = screen,
// 10 s = ask whether to delete the game.
// The CH340 DTR line also moves this pin when the serial port opens, hence the wait
// after boot. Holding it during a reset still enters flashing mode.
#define KNOMI_BTN_HOLD_MS 800      // release from here on: screen
#define KNOMI_BTN_HOLD_MAX_MS 3000 // releasing later does nothing: they changed their mind
#define KNOMI_BTN_RESET_MS 10000   // held this long: asks whether to delete the game

void knomiButtonTap() {
  bool wasAsleep = screenOff || dimStage > 0;
  screenOff = false;
  wakeUI();
  if (wasAsleep) return;  // in the dark it only wakes, like the touchscreen
  if (buddyMenuOpen || playMenuOpen || tradeOpen || sdOpen || vbOpen || gameOpen || runOpen || dungeonOpen || quizOpen || gamesMenuOpen || zoneScreen || galleryOpen || cardOpen || kbOpen || clockOpen || choiceKind || confirmUntil) return;
  if (visitPhase != VP_NONE) return;  // no fighting with a visitor present
  startBattle();
}

// the dialog is drawn on the main screen: close whatever is open on top
void knomiAskReset() {
  if (pet.awaitingStarter() || pet.ceremony) return;  // nothing to delete, or farewell in progress
  if (battleOpen) closeBattle();
  if (quizOpen) closeQuiz();
  if (sdOpen) sdQuit();
  if (vbOpen) vbQuit();
  if (runOpen) closeRun();
  if (dungeonOpen) closeDungeon();
  gamesMenuOpen = playMenuOpen = false;
  zoneDetailClose();
  zoneScreen = ZS_NONE;
  if (galleryOpen) { galleryOpen = false; galleryUnload(); }
  wifiSetupStop();
  gameOpen = cardOpen = cardAch = kbOpen = clockOpen = false;
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
    if (held < 30 || resetAsked) return;  // 30 ms: debounce
    if (held < KNOMI_BTN_HOLD_MS) {
      knomiButtonTap();
    } else if (held < KNOMI_BTN_HOLD_MAX_MS) {
      screenOff = !screenOff;
      if (!screenOff) wakeUI();
      Serial.printf("button: screen %s\n", screenOff ? "off" : "on");
    }
  }
}
#endif

// ---------- battle ----------
// Bottom band for messages and buttons; what is above is the scene, as in gen 1.
#define BTL_BAND_Y 336
#define BTL_FOE_X 330
#define BTL_FOE_GROUND 186
#define BTL_ME_X 128
#define BTL_ME_GROUND 322

void battleSetPhase(BattlePhase p) {
  bPhase = p;
  bPhaseT = millis();
}

BtlFighter petFighter(uint8_t lvl) {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  BtlFighter f;
  f.dex = pet.speciesId;
  f.level = lvl;
  f.hp = f.maxHp = btlMaxHp(d.bHp, lvl);
  f.atk = btlStat(d.bAtk, pet.geneAtk, lvl, pet.trAtk);
  f.def = btlStat(d.bDef, pet.geneDef, lvl, pet.trDef);
  f.spe = btlStat(d.bSpe, pet.geneSpe, lvl, pet.trSpe);
  return f;
}

void startBattleWith(int16_t dex) {
  if (pet.isEgg() || pet.ceremony || battleOpen || !pmd.loaded) return;
  uint8_t lvl = btlCapLevel(pet.level());
  if (pet.sleeping || pet.energy < BTL_MIN_ENERGY) {
    battleOpen = true;  // a brief notice instead of a button that does nothing
    sfxPlay(SFX_DENY);
    battleSetPhase(BP_TIRED);
    return;
  }
  uint8_t foeLvl = btlWildLevel(lvl, random(4));
  if (dex < 1 || dex > DEX_COUNT) dex = bZone >= 0 ? pet.pickZoneSpecies(bZone, foeLvl) : pet.pickWildSpecies(foeLvl);
  bFoeShiny = bNextShiny || random(100) < WILD_SHINY_PCT;
  bNextShiny = false;
  wildPmd.load(dex, bFoeShiny);
  if (!wildPmd.loaded) return;  // without a sprite there is no foe to show

  bMe = petFighter(lvl);
  bFoe = btlMakeWild(dex, foeLvl, btlWildGene(random(16)));
  bMeMoves = btlMovesFor(bMe.dex, lvl, dex);
  bFoeMoves = btlMovesFor(dex, foeLvl, bMe.dex);
  bShownMe = bMe.hp;
  bShownFoe = bFoe.hp;
  bWon = false;
  bGain = 0;
  battleOpen = true;
  battleSetPhase(BP_INTRO);
  sfxPlay(bFoeShiny ? SFX_MEDAL : SFX_PLAY);
  Serial.printf("battle: foe #%d%s Lv.%u %s/%s vs %s/%s\n", dex, bFoeShiny ? " shiny" : "", foeLvl,
                MOVE_TBL[bFoeMoves.ids[0]].name, MOVE_TBL[bFoeMoves.ids[1]].name,
                MOVE_TBL[bMeMoves.ids[0]].name, MOVE_TBL[bMeMoves.ids[1]].name);
}

void startBattle() { startBattleWith(0); }

void closeBattle() {
  if (bPvp) buddyBattleClosed();
  battleOpen = false;
  wildPmd.unload();
  if (bZone >= 0) zoneBattleEnded();
}

void battleMoveRect(uint8_t i, int &x0, int &x1) {
  if (bMeMoves.count == 1) { x0 = 116; x1 = 350; return; }
  x0 = i == 0 ? 62 : 238;
  x1 = i == 0 ? 228 : 404;
}

void battleTap(int16_t x, int16_t y) {
  if (bPhase == BP_END) {
    if (millis() - bPhaseT > 800) battleAfterEnd();  // time to read the result
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
    if (bPvp) {  // wait for the buddy's move; the turn is resolved on both boards at once
      bPvpMine = i;
      buddySendMove(bPvpTurn, i, false);
      bsResendT = millis();
      battleSetPhase(BP_WAIT);
      return;
    }
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
  if (bPvp) {
    pet.friendlyBattle(won);
    bGain = 0;
  } else {
    bGain = pet.battleResult(won);
  }
  sfxPlay(won ? SFX_LEVEL : SFX_DENY);
  Serial.printf("battle: won=%d atk+%u\n", won, bGain);
  battleSetPhase(BP_END);
}

// after the result: if it is a new species, a capture can be attempted
void battleAfterEnd() {
  if (!bPvp && bWon && !pet.hasForm(bFoe.dex, bFoeShiny)) {
    bBalls = CATCH_BALLS;
    battleSetPhase(BP_AIM);
  } else {
    closeBattle();
  }
}

uint8_t bBreakAt = 0, bShakesHeard = 0;  // the throw that hit: when it breaks out (0 = holds), tones played
float bCatchX = CX;                      // where the ball landed: the foe stops swaying in it

// the foe sways side to side so you have to aim
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
  if (catchHit(bBallX, battleFoeSway(now))) {  // in the ball: now the three shakes
    bCatchX = battleFoeSway(now);
    uint8_t chance = catchChance(bBallX, bCatchX);  // which ring it landed in
    bBreakAt = catchBreakAt(random(100), random(CATCH_SHAKES), chance);
    bShakesHeard = 0;
    Serial.printf("catch: in the ball, %u%% ring (%s)\n", chance, bBreakAt ? "will break out" : "holds");
    battleSetPhase(BP_SHAKE);
  } else {
    bBalls--;
    Serial.printf("catch: miss, %u left\n", bBalls);
    sfxPlay(SFX_DENY);
    battleSetPhase(BP_MISS);
  }
}

// applies the current turn's hit
void battleApplyHit() {
  bool mine = bTurn[bStep].mine;
  BtlFighter &a = mine ? bMe : bFoe, &t = mine ? bFoe : bMe;
  uint8_t aType = DEX_TBL[a.dex].type, tType = DEX_TBL[t.dex].type;
  BtlMove m = MOVE_TBL[(mine ? bMeMoves : bFoeMoves).ids[bTurn[bStep].move]];
  uint16_t dmg = btlDamage(a.level, m, a.atk, t.def, aType, tType, bPvp ? pvpRoll(bRng) : 85 + random(16));
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
        // the new move is announced only once; DITTO copies, doesn't learn
        bool announces = bMe.dex != DEX_DITTO && !bPvp;
        int16_t learned = announces ? btlMoveToAnnounce(pet.movesSeen, bMeMoves) : -1;
        if (announces) pet.movesSeen = btlMovesSeenValue(bMeMoves);
        if (learned >= 0) {
          bLearned = learned;
          sfxPlay(SFX_MEDAL);
          battleSetPhase(BP_LEARNED);
        } else battleSetPhase(BP_CHOOSE);
      }
      break;
    case BP_LEARNED: if (t > 2000) battleSetPhase(BP_CHOOSE); break;
    case BP_CHOOSE:  // nobody responds: it flees on its own so the screen can dim
      if (idleSince(millis(), lastInteract) > 60000) battleSetPhase(BP_RAN);
      break;
    case BP_WAIT:
      if (bPvpFoeTurn == bPvpTurn) {
        bool first = pvpMineFirst(bMe.spe, bFoe.spe, buddyNetId(), bPeers[bCur].id, bRng);
        bTurn[0] = { first, first ? bPvpMine : bPvpFoeMove };
        bTurn[1] = { !first, first ? bPvpFoeMove : bPvpMine };
        bPvpMinePrev = bPvpMine;
        bPvpTurn++;
        bStep = 0;
        battleSetPhase(BP_ATTACK);
      } else if (t > BUDDY_REPLY_MS) {
        buddyGoneFromBattle();
      }
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
    case BP_RAN: if (t > (bPvpGone ? 2500UL : 1200UL)) closeBattle(); break;
    case BP_END: if (t > 3500) battleAfterEnd(); break;
    case BP_AIM:  // nobody responds: it leaves
      if (idleSince(millis(), lastInteract) > 60000) battleSetPhase(BP_FLED);
      break;
    case BP_THROW: battleThrowStep(millis()); break;
    case BP_MISS:
      if (t > 2000) battleSetPhase(bBalls ? BP_AIM : BP_FLED);
      break;
    case BP_BROKE: if (t > 3000) closeBattle(); break;  // it broke out and ran off: no more throws
    case BP_SHAKE: {  // a rock and a tone per count; it may pop out halfway through one
      uint8_t shake = t / CATCH_SHAKE_MS;  // 0..2 while counting
      if (shake < CATCH_SHAKES && bShakesHeard <= shake) {
        bShakesHeard = shake + 1;
        sfxPlay((Sfx)(SFX_SHAKE1 + shake));
      }
      if (bBreakAt && t > (bBreakAt - 1) * CATCH_SHAKE_MS + CATCH_SHAKE_MS * 3 / 5) {
        Serial.printf("catch: broke out on %u and got away\n", bBreakAt);
        sfxPlay(SFX_DENY);
        battleSetPhase(BP_BROKE);
      } else if (!bBreakAt && t > CATCH_SHAKES * CATCH_SHAKE_MS + 250) {
        bool isNew = pet.catchWild(bFoe.dex, bFoeShiny);
        Serial.printf("catch: #%d%s new=%d\n", bFoe.dex, bFoeShiny ? " shiny" : "", isNew);
        sfxPlay(SFX_MEDAL);
        battleSetPhase(BP_CAUGHT);
      }
      break;
    }
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
    float k = sinf(min(t, 800UL) * 3.14159f / 800.0f);  // lunge toward the foe
    x += (int)((mine ? 34 : -34) * k);
    y += (int)((mine ? -14 : 14) * k);
  } else if (hurt && bPhase == BP_HIT) {
    if (m.has(PMD_HURT)) act = PMD_HURT;
    if ((t / 90) % 2) return;  // hit blink
  }
  if (bPhase == BP_END && (mine ? bMe.hp : bFoe.hp) == 0) return;  // the defeated one is gone
  drawPmdActM(m, act, x, y, now - bPhaseT, true, false, maxS);
}

void battleCenterLine(const char *s, int y) {
  setCur(centerX(s, 2), y);
  printT(s);
}

// the result of a throw: large text on color, so nobody is left in doubt
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

// the catch rings around the Pokemon, as in Pokemon GO: a faint ring for where a ball still hits,
// a bold white one around the body (three in four hold), and the small green circle that always holds
// centred on the Pokemon's body: halfway up what its sprite fills above the ground (drawn at 3x on 216)
int catchRingY() {
  const PmdAct &a = wildPmd.acts[PMD_IDLE];
  if (!a.frames) return 176;
  int top = a.h;
  for (int r = 0; r < a.h && top == a.h; r++)
    for (int c = 0; c < a.w; c++)
      if (a.data[r * a.w + c] != 0xFF) { top = r; break; }
  uint8_t s = a.h ? 170 / a.h : 3;
  s = s < 2 ? 2 : s > 3 ? 3 : s;
  return 216 - ((a.base ? a.base : a.h) - top) * s / 2;
}
void drawCatchRings(int cx) {
  int cy = catchRingY();
  gfx->drawCircle(cx, cy, CATCH_RADIUS, C565(0xdd, 0xe4, 0xea));
  for (int i = 0; i < 2; i++) gfx->drawCircle(cx, cy, CATCH_RING_MID - i, UI_WHITE);
  for (int i = 0; i < 3; i++) gfx->drawCircle(cx, cy, CATCH_RING_IN - i, C565(0x4c, 0xd9, 0x3c));
}

static const uint16_t ZONE_GROUND[ZONE_COUNT] = { C565(0x4f, 0x9a, 0x4a), C565(0xec, 0xd5, 0x93), C565(0x54, 0x4b, 0x63), C565(0x25, 0x28, 0x3a), C565(0x79, 0xbd, 0x4f) };

// caught: the ball bursts with light, a flash ring, rays that shoot out and fade, then a few
// sparkles twinkling around it
void drawCatchShine(int cx, int cy, uint32_t t) {
  const uint16_t gold = C565(0xff, 0xd9, 0x5e), pale = C565(0xff, 0xf6, 0xc8);
  if (t < 450) {  // the flash: rings spreading out
    int rr = 28 + (int)(t * 0.16f);
    for (int i = 0; i < 3; i++) gfx->drawCircle(cx, cy, rr + i, i == 1 ? UI_WHITE : pale);
    if (t > 120) for (int i = 0; i < 2; i++) gfx->drawCircle(cx, cy, rr - 18 + i, gold);
  }
  if (t < 1100) {  // the rays: out, then shorter as they fade
    float k = t / 1100.0f, len = 46 * sinf(k * 3.14159f), start = 32 + 20 * k;
    for (int i = 0; i < 12; i++) {
      float a = i * 6.28318f / 12 + 0.13f;
      int x0 = cx + (int)(cosf(a) * start), y0 = cy + (int)(sinf(a) * start);
      int x1 = cx + (int)(cosf(a) * (start + len)), y1 = cy + (int)(sinf(a) * (start + len));
      uint16_t c = i % 2 ? UI_WHITE : gold;
      gfx->drawLine(x0, y0, x1, y1, c);
      gfx->drawLine(x0 + 1, y0, x1 + 1, y1, c);
      gfx->drawLine(x0, y0 + 1, x1, y1 + 1, c);
    }
  }
  if (t > 250 && t < 3000) {  // sparkles, each on its own beat
    for (int i = 0; i < 6; i++) {
      float a = i * 6.28318f / 6 + 0.5f;
      int d = 52 + (i % 2) * 18;
      uint32_t beat = (t + i * 230) % 900;
      int sz = beat < 300 ? 13 * beat / 300 : beat < 600 ? 13 * (600 - beat) / 300 : 0;
      drawSparkle(cx + (int)(cosf(a) * d), cy + (int)(sinf(a) * d * 0.8f), sz);
    }
  }
}

void renderCatch() {
  uint32_t now = millis();
  drawGameScene();
  bool held = bPhase == BP_SHAKE || bPhase == BP_CAUGHT;  // in the ball, where it landed
  float foeX = held ? bCatchX : battleFoeSway(now);
  bFoeX = foeX;
  uint8_t bio = DEX_TBL[bFoe.dex].biome;
  uint16_t soil = BIOME_SOIL[bio < 6 ? bio : 0];
  if (sceneHour() < 6 || sceneHour() >= 20) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  if (!zoneBg) gfx->fillRect(0, 216, 466, BTL_BAND_Y - 216, soil);
  if (bPhase == BP_BROKE) {  // out of the ball and off it runs
    uint32_t t = now - bPhaseT;
    int dir = bCatchX < CX ? -1 : 1;
    int x = (int)bCatchX + dir * (int)(t > 400 ? (t - 400) * 0.25f : 0);
    uint8_t act = dir > 0 ? PMD_WALKR : PMD_WALKL;
    drawPmdActM(wildPmd, wildPmd.has(act) && t > 400 ? act : PMD_IDLE, x, 216, t, true, false, 3);
  } else if (bPhase != BP_CAUGHT && bPhase != BP_FLED && bPhase != BP_SHAKE) {
    drawPmdActM(wildPmd, PMD_IDLE, (int)foeX, 216, now - bPhaseT, true, false, 3);
    if (bPhase == BP_AIM || bPhase == BP_THROW) drawCatchRings((int)foeX);
  }
  if (bPhase == BP_THROW) {
    float k = (now - bBallT) / 1500.0f;
    if (k > 1) k = 1;
    int y = 360 - (int)(200 * k), s = k < 0.5f ? 3 : 2;  // moves away
    drawMap(SPR_HOME_BALL, HOME_ICON_N, (int)bBallX - 9 * s, y - 9 * s, s, false);
  } else if (bPhase == BP_MISS) {  // bounces and falls: clearly it didn't go in
    float k = (now - bPhaseT) / 700.0f;
    if (k > 1) k = 1;
    drawMap(SPR_HOME_BALL, HOME_ICON_N, (int)(bBallX + (bBallX < foeX ? -70 : 70) * k) - 18, 142 + (int)(200 * k * k), 2, false);
  } else if (bPhase == BP_SHAKE) {  // on the ground, rocking to one side on each count
    uint32_t t = now - bPhaseT;
    uint32_t in = t % CATCH_SHAKE_MS;
    float k = in < 420 ? sinf(in * 3.14159f / 420) : 0;  // a rock, then still until the next count
    int side = (t / CATCH_SHAKE_MS) % 2 ? 1 : -1;
    drawMap(SPR_HOME_BALL, HOME_ICON_N, (int)bCatchX - 27 + (int)(side * 14 * k), 165 + (int)(4 * k), 3, false);
  } else if (bPhase == BP_CAUGHT) {
    drawCatchShine((int)bFoeX, 165 + 27, now - bPhaseT);
    drawMap(SPR_HOME_BALL, HOME_ICON_N, (int)bFoeX - 27, 165, 3, false);
  }

  gfx->fillRect(0, BTL_BAND_Y, 466, 466 - BTL_BAND_Y, UI_BG_DAY);
  gfx->fillRect(0, BTL_BAND_Y, 466, 3, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(2);
  char b[48];
  switch (bPhase) {
    case BP_AIM:
      battleCenterLine(T(S_THROW_HINT), 350);
      drawMap(SPR_HOME_BALL, HOME_ICON_N, CX - 36, 372, 4, false);  // the pokeball is the button
      for (uint8_t i = 0; i < bBalls; i++) gfx->fillCircle(CX + 56 + i * 16, 408, 5, UI_BAR_BAD);
      break;
    case BP_THROW:
      battleCenterLine(T(S_TILT_HINT), 380);
      break;
    case BP_MISS:
      catchBanner(T(S_MISSED), C565(0xb8, 0x2e, 0x2a));
      for (uint8_t i = 0; i < bBalls; i++) gfx->fillCircle(CX + (i * 2 - (bBalls - 1)) * 12, 416, 6, UI_BAR_BAD);
      break;
    case BP_SHAKE: {  // the count to three, big
      uint32_t t = millis() - bPhaseT;
      uint8_t n = t / CATCH_SHAKE_MS + 1;
      if (n > CATCH_SHAKES) n = CATCH_SHAKES;
      char c[8];
      snprintf(c, sizeof(c), n == 1 ? "1" : n == 2 ? "1  2" : "1  2  3");
      setSize(5);
      setCur(centerX(c, 5), 362);
      printT(c);
      setSize(2);
      break;
    }
    case BP_BROKE:
      snprintf(b, sizeof(b), T(S_GOT_AWAY_FMT), dexName(bFoe.dex));
      catchBanner(b, C565(0x55, 0x55, 0x60));
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

// ---------- attack effects ----------
// A little of the move's element: a few on their way during the lunge for the moves that fly
// (fire, water, leaves, ice, sparks, the mind), then a small burst on the target as it lands.
static const uint16_t FX_WHITE = 0xFFFF, FX_YELLOW = 0xFECB, FX_ORANGE = 0xF427, FX_BLUE = 0x4C98,
                      FX_LIGHT_BLUE = 0x7E3D, FX_GREEN = 0x5DCD, FX_DARK_GREEN = 0x3C49, FX_ICE = 0xBF7F,
                      FX_PURPLE = 0x8A73, FX_PINK = 0xF454, FX_BROWN = 0x7AA6, FX_TAN = 0xB3C8;

static bool fxFlies(uint8_t ty) {
  return ty == TY_FIRE || ty == TY_WATER || ty == TY_GRASS || ty == TY_ICE || ty == TY_ELECTRIC ||
         ty == TY_PSYCHIC || ty == TY_POISON || ty == TY_GHOST || ty == TY_DRAGON;
}

#define FX_SIZE 3  // how big the pieces are, in halves: 3 = one and a half times the first size

// one piece of the element at (x, y); i tells the pieces apart, k is its age 0..1
static void fxPiece(uint8_t ty, int x, int y, int i, float k) {
  int r = (int)((5 - 2 * k) * FX_SIZE / 2);
  const int u = FX_SIZE;  // the other sizes, in halves too
  switch (ty) {
    case TY_FIRE:
    case TY_DRAGON: {  // a flame: round below, pointed above
      uint16_t out = ty == TY_FIRE ? FX_ORANGE : FX_PURPLE, in = ty == TY_FIRE ? FX_YELLOW : FX_PINK;
      gfx->fillCircle(x, y, r + 1, out);
      gfx->fillTriangle(x - r - 1, y, x + r + 1, y, x, y - 3 * r - 2, out);
      gfx->fillCircle(x, y + 1, r - 2 > 0 ? r - 2 : 1, in);
      break;
    }
    case TY_WATER:  // a drop with a glint
      gfx->fillCircle(x, y, r, i % 2 ? FX_BLUE : FX_LIGHT_BLUE);
      gfx->fillRect(x - 1, y - r + 1, 2, 2, FX_WHITE);
      break;
    case TY_GRASS: {  // a leaf, turning as it goes
      int dx = ((i + (int)(k * 6)) % 2 ? 3 : -3) * u / 2;
      gfx->fillCircle(x - dx / 2, y, 3 * u / 2, FX_GREEN);
      gfx->fillCircle(x + dx / 2, y - 1, 3 * u / 2, FX_GREEN);
      gfx->drawLine(x - dx, y + 1, x + dx, y - 2, FX_DARK_GREEN);
      break;
    }
    case TY_BUG:
      gfx->fillCircle(x, y, 2 * u / 2, i % 2 ? FX_GREEN : FX_DARK_GREEN);
      break;
    case TY_ELECTRIC: {  // a short zigzag spark
      if ((millis() / 70 + i) % 3 == 0) break;  // they flicker
      int z = (i % 2 ? 5 : -5) * u / 2, h = 10 * u / 2, m = 3 * u / 2;
      for (int o = -1; o <= 2; o++) {  // thick, with a dark edge, so it shows against a light sky
        uint16_t c = o == 2 ? FX_ORANGE : FX_YELLOW;
        gfx->drawLine(x + o, y - h, x + z + o, y - m, c);
        gfx->drawLine(x + z + o, y - m, x - z + o, y + m, c);
        gfx->drawLine(x - z + o, y + m, x + o, y + h, c);
      }
      gfx->drawLine(x, y - h, x + z, y - m, FX_WHITE);
      break;
    }
    case TY_ICE:  // a little crystal
      gfx->fillTriangle(x - r, y, x + r, y, x, y - r - 2, FX_ICE);
      gfx->fillTriangle(x - r, y, x + r, y, x, y + r + 2, FX_ICE);
      gfx->fillRect(x, y - 1, 1, 1, FX_WHITE);
      break;
    case TY_POISON:
    case TY_GHOST:  // bubbles
      gfx->drawCircle(x, y, r, FX_PURPLE);
      gfx->fillCircle(x, y, r - 2 > 0 ? r - 2 : 1, ty == TY_POISON ? 0xC2F0 : 0x6AD3);
      break;
    case TY_ROCK:
    case TY_GROUND:  // pebbles
      gfx->fillRoundRect(x - 3 * u / 2, y - 3 * u / 2, 7 * u / 2, 6 * u / 2, 3, i % 2 ? FX_BROWN : FX_TAN);
      break;
    case TY_FLYING:  // a gust
      gfx->fillRect(x - 8 * u / 2, y, 16 * u / 2, 2, FX_WHITE);
      gfx->fillRect(x - 4 * u / 2, y + 3 * u / 2, 10 * u / 2, 2, FX_WHITE);
      break;
    default:  // NORMAL, FIGHTING, PSYCHIC on its way: an impact star
      gfx->drawLine(x - r - 2, y, x + r + 2, y, i % 2 ? FX_WHITE : FX_YELLOW);
      gfx->drawLine(x, y - r - 2, x, y + r + 2, i % 2 ? FX_WHITE : FX_YELLOW);
      gfx->drawLine(x - r, y - r, x + r, y + r, FX_WHITE);
      break;
  }
}

// the burst on the target: k 0..1 over the hit
static void fxBurst(uint8_t ty, int cx, int cy, float k) {
  if (ty == TY_PSYCHIC) {  // the mind: rings spreading out
    for (int j = 0; j < 3; j++) {
      float kk = k * 1.4f - j * 0.2f;
      if (kk <= 0 || kk >= 1) continue;
      int rr = (8 + (int)(44 * kk)) * FX_SIZE / 2;
      gfx->drawCircle(cx, cy, rr, FX_PINK);
      gfx->drawCircle(cx, cy, rr + 1, FX_PINK);
    }
    return;
  }
  const int n = 8;
  for (int i = 0; i < n; i++) {
    float a = i * 6.28318f / n + 0.4f;
    float dist = (10 + 40 * k) * FX_SIZE / 2;
    int x = cx + (int)(cosf(a) * dist), y = cy + (int)(sinf(a) * dist * 0.75f);
    switch (ty) {
      case TY_FIRE: case TY_DRAGON: case TY_POISON: case TY_GHOST:
        y -= (int)(26 * k) * FX_SIZE / 2;  // they rise
        break;
      case TY_WATER: case TY_GRASS:
        y += ((int)(38 * k * k) - 8) * FX_SIZE / 2;  // thrown out, then they fall
        if (ty == TY_GRASS) x += (int)(6 * sinf(k * 9 + i));
        break;
      case TY_ROCK: case TY_GROUND:  // dropped on it from above
        x = cx + (i - n / 2) * 11 * FX_SIZE / 2;
        y = cy - 70 + (int)(80 * k * k) + (i % 3) * 6;
        break;
      case TY_ELECTRIC:
        dist = (14 + 20 * k) * FX_SIZE / 2;
        x = cx + (int)(cosf(a) * dist);
        y = cy + (int)(sinf(a) * dist);
        break;
      default: break;
    }
    fxPiece(ty, x, y, i, k);
  }
}

// the move being made right now, if any: its effect for this frame
void drawMoveFx(int meX, int meY, int foeX, int foeY) {
  if (bPhase != BP_ATTACK && bPhase != BP_HIT) return;
  bool mine = bTurn[bStep].mine;
  uint8_t ty = MOVE_TBL[(mine ? bMeMoves : bFoeMoves).ids[bTurn[bStep].move]].type;
  int ax = mine ? meX : foeX, ay = mine ? meY : foeY, tx = mine ? foeX : meX, ty_ = mine ? foeY : meY;
  uint32_t t = millis() - bPhaseT;
  if (bPhase == BP_ATTACK) {
    if (t < 420 || !fxFlies(ty)) return;
    float k = (t - 420) / 380.0f;
    for (int j = 0; j < 3; j++) {  // a few on their way
      float kk = k - j * 0.14f;
      if (kk < 0 || kk > 1) continue;
      fxPiece(ty == TY_PSYCHIC ? TY_NORMAL : ty, ax + (int)((tx - ax) * kk), ay + (int)((ty_ - ay) * kk) - (int)(18 * sinf(kk * 3.14159f)), j, 0);
    }
    return;
  }
  if (bLastEff == 0 || t > 700) return;  // no effect on an immune target
  fxBurst(ty, tx, ty_, t / 700.0f);
}

void renderBattle() {
  if (pet.ceremony) { closeBattle(); return; }  // the farewell takes priority
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
    if (zoneBg) {
      soil = ZONE_GROUND[bZone];  // the foe stands on its zone's ground
    } else {
      gfx->fillRect(0, 232, 466, BTL_BAND_Y - 232, soil);
    }
    gfx->fillRoundRect(BTL_FOE_X - 70, BTL_FOE_GROUND - 10, 140, 24, 12, soil);
    // the on-screen HP chases the real one
    bShownMe += (bMe.hp - bShownMe) * 0.28f;
    bShownFoe += (bFoe.hp - bShownFoe) * 0.28f;
    drawBattleFighter(wildPmd, false, BTL_FOE_X, BTL_FOE_GROUND, 3);
    drawBattleFighter(pmd, true, BTL_ME_X, BTL_ME_GROUND, 4);
    drawMoveFx(BTL_ME_X, (BTL_ME_GROUND + pmdTopY(pmd, BTL_ME_GROUND, 4)) / 2,
               BTL_FOE_X, (BTL_FOE_GROUND + pmdTopY(wildPmd, BTL_FOE_GROUND, 3)) / 2);
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
      if (bPvp) snprintf(b, sizeof(b), "VS %s!", buddyName());
      else snprintf(b, sizeof(b), T(bFoeShiny ? S_SHINY_WILD_FMT : S_BTL_WILD), dexName(bFoe.dex));
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
    case BP_WAIT: battleCenterLine(T(S_BUDDY_WAITING), 380); break;
    case BP_RAN:
      if (bPvpGone) {
        snprintf(b, sizeof(b), T(S_BUDDY_LEFT_FMT), buddyName());
        battleCenterLine(b, 380);
      } else {
        battleCenterLine(T(S_BTL_RAN), 380);
      }
      break;
    case BP_END: {
      if (bWon) {
        gfx->setTextColor(UI_BAR_OK);
        battleCenterLine(T(S_BTL_WON), 352);
      } else {
        snprintf(b, sizeof(b), T(S_BTL_LOST), dexName(bMe.dex));
        battleCenterLine(b, 352);
      }
      if (!bPvp && bGain) {  // no training between friends, nor from a loss
        gfx->setTextColor(UI_BAR_BAD);
        snprintf(b, sizeof(b), T(S_STR_GAIN_FMT), bGain);
        battleCenterLine(b, 380);
      } else if (!bPvp && bWon && pet.trAtk >= 100) {  // a win, but nothing more to train: say so
        gfx->setTextColor(UI_BAR_BAD);
        battleCenterLine(T(S_TRAIN_MAXED), 380);
      }
      break;
    }
  }
  gfx->flush();
}

// minigame background: the pet's habitat (sky by hour + biome ground)
void drawGameScene() {
  if (zoneBg) {
    gfx->draw16bitRGBBitmap(0, 0, zoneBg, 466, BTL_BAND_Y);
    return;
  }
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
  // no fillScreen(BLACK): drawGameScene covers the full 466x466. If the
  // previous flush's DMA is still reading the buffer, it will see valid content (not
  // half-painted black), which was the flicker at 25 fps.
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

  // score, record and lives
  char buf[8];
  snprintf(buf, sizeof(buf), "%u", gameScore);
  gfx->setTextColor(ink);
  setSize(4);
  setCur(centerX(buf, 4), 30);
  printT(buf);
  // 24 and not 12: in Japanese "きろく %u" is 10 bytes before the number and the
  // record was cut to its first digit ("きろく 2" with 219). The sizes of
  // the text buffers are watched by TestBuffersDeTexto in test/test_tools.py.
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

  // impact ring that expands and fades (soft hit feedback)
  uint32_t ht = millis() - hitTime;
  if (hitTime && ht < 260) {
    int rad = 22 + (int)(ht / 6);
    gfx->drawCircle((int)hitX, (int)hitY, rad, C565(0xff, 0xe7, 0x9f));
    gfx->drawCircle((int)hitX, (int)hitY, rad - 2, C565(0xff, 0xd9, 0x8a));
  }

  // the berry
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

// ---------- dungeon dive ----------
#define DN_TILE 64

void startDungeon() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony || !pmd.loaded) { sfxPlay(SFX_DENY); return; }
  dnStart(dn, petFighter(btlCapLevel(pet.level())), esp_random());
  dungeonOpen = true;
  dnBot = dnNewHi = dnHeld = false;
  dnOverT = dnStepT = dnHurtT = dnHitT = 0;
  dnCamX = dn.x;
  dnCamY = dn.y;
  uint8_t fit = pmdFitScale(pmd, PMD_IDLE, 76, 76);
  dnScale = fit > 3 ? 3 : fit;
}

void closeDungeon() { dungeonOpen = false; dnBot = false; }

void dungeonAfterStep() {
  uint32_t now = millis();
  dnStepT = now;
  for (uint8_t i = 0; i < dn.hitCount; i++) {
    if (dn.hits[i].mine) dnHitT = now;
    else dnHurtT = now;
    if (dn.hits[i].dmg) sfxPlay(SFX_PLAY);
  }
  if (dn.ate) sfxPlay(SFX_TAP);
  if (dn.descended) {
    sfxPlay(SFX_LEVEL);
    dnCamX = dn.x;  // new floor: no camera trip across the whole map
    dnCamY = dn.y;
  }
  if (dn.dead) {
    dnNewHi = dn.floor > pet.dungeonHi;
    pet.dungeonResult(dn.floor);  // the reward and record only on a real fall
    sfxPlay(dnNewHi ? SFX_MEDAL : SFX_DENY);
    dnOverT = now;
    Serial.printf("dungeon: B%uF, %u KO\n", dn.floor, dn.kos);
  }
}

// walk toward where it is touched, relative to the screen center
void dungeonTap(int16_t x, int16_t y) {
  if (dnOverT) {
    if (millis() - dnOverT > 800) closeDungeon();
    return;
  }
  int dx = x - CX, dy = y - CY;
  if (dx * dx + dy * dy < 36 * 36) return;
  bool horiz = abs(dx) >= abs(dy);
  if (dnStep(dn, horiz ? (dx > 0 ? 1 : -1) : 0, horiz ? 0 : (dy > 0 ? 1 : -1))) dungeonAfterStep();
}

void dnDrawTile(int tx, int ty, int sx, int sy) {
  const uint16_t floorC = C565(0x62, 0x58, 0x4c), floorD = C565(0x56, 0x4d, 0x43);
  const uint16_t wallTop = C565(0x24, 0x20, 0x2c), wallFace = C565(0x3e, 0x37, 0x42);
  uint8_t t = dn.map[ty][tx];
  if (t == DN_WALL) {
    gfx->fillRect(sx, sy, DN_TILE, DN_TILE, wallTop);
    if (ty + 1 < DN_H && dn.map[ty + 1][tx] != DN_WALL) gfx->fillRect(sx, sy + DN_TILE - 16, DN_TILE, 16, wallFace);
    return;
  }
  gfx->fillRect(sx, sy, DN_TILE, DN_TILE, floorC);
  uint8_t h = (tx * 7 + ty * 13) % 5;  // fixed little pebbles so you can tell you are moving
  gfx->fillRect(sx + 8 + h * 9, sy + 12 + h * 5, 7, 5, floorD);
  gfx->fillRect(sx + 44 - h * 6, sy + 42 - h * 2, 9, 5, floorD);
  if (t == DN_STAIRS) {
    for (int i = 0; i < 4; i++)
      gfx->fillRect(sx + 6 + i * 6, sy + 8 + i * 12, DN_TILE - 12 - i * 12, 10, lerp565(C565(0xd8, 0xd0, 0xb8), C565(0x20, 0x1c, 0x24), i, 4));
  }
}

void renderDungeon() {
  if (pet.ceremony || !pmd.loaded) { closeDungeon(); return; }
  uint32_t now = millis();
  if (dnOverT && now - dnOverT > 6000) { closeDungeon(); return; }

  if (dnBot && !dn.dead && now - dnStepT > 200) {
    int8_t dx, dy;
    if (dnBotDir(dn, dx, dy) && dnStep(dn, dx, dy)) dungeonAfterStep();
  }
#ifdef BOARD_HOST
  {  // the arrow keys walk: a step on the press, then on as the finger does while it's held
    static bool keyWas = false;
    static uint32_t keyT = 0;
    int8_t kx, ky;
    bool key = hostArrowDir(&kx, &ky);
    if (key && !dn.dead && !dnOverT && (!keyWas || now - keyT > 230)) {
      keyT = now;
      wakeUI();
      if (dnStep(dn, kx, ky)) dungeonAfterStep();
    }
    keyWas = key;
  }
#endif

  dnCamX += (dn.x - dnCamX) * 0.45f;
  dnCamY += (dn.y - dnCamY) * 0.45f;
  if (fabsf(dn.x - dnCamX) < 0.02f) dnCamX = dn.x;
  if (fabsf(dn.y - dnCamY) < 0.02f) dnCamY = dn.y;
  int ox = CX - DN_TILE / 2 - (int)(dnCamX * DN_TILE), oy = CY - DN_TILE / 2 - (int)(dnCamY * DN_TILE);

  gfx->fillScreen(C565(0x24, 0x20, 0x2c));
  int tx0 = (0 - ox) / DN_TILE - 1, ty0 = (0 - oy) / DN_TILE - 1;
  for (int ty = ty0 < 0 ? 0 : ty0; ty < DN_H && oy + ty * DN_TILE < 466; ty++)
    for (int tx = tx0 < 0 ? 0 : tx0; tx < DN_W && ox + tx * DN_TILE < 466; tx++)
      dnDrawTile(tx, ty, ox + tx * DN_TILE, oy + ty * DN_TILE);

  for (auto &b : dn.berry) {
    if (!b.on) continue;
    int bx = ox + b.x * DN_TILE + DN_TILE / 2, by = oy + b.y * DN_TILE + DN_TILE / 2;
    gfx->fillCircle(bx, by + 4, 12, C565(0xd8, 0x3a, 0x4a));
    gfx->fillCircle(bx - 4, by, 3, C565(0xf4, 0xa0, 0xa8));
    gfx->fillRect(bx - 1, by - 14, 3, 8, C565(0x3c, 0xa0, 0x50));
  }

  for (auto &m : dn.mon) {
    if (!m.on) continue;
    int mx = ox + m.x * DN_TILE + DN_TILE / 2, my = oy + m.y * DN_TILE + DN_TILE / 2;
    if (mx < -40 || mx > 506 || my < -40 || my > 506) continue;
    if (const uint8_t *t = thumbs.get(m.f.dex)) drawThumb(t, mx - 40, my - 44, 3, false);  // its cell is 80 wide
    else gfx->fillCircle(mx, my, 20, DEX_TBL[m.f.dex].accent);
    gfx->fillRect(mx - 24, my + 26, 48, 6, UI_INK);
    int w = 46 * m.f.hp / (m.f.maxHp ? m.f.maxHp : 1);
    gfx->fillRect(mx - 23, my + 27, w < 1 ? 1 : w, 4, UI_BAR_BAD);
  }

  uint8_t act = PMD_IDLE;
  if (dnOverT) act = PMD_HURT;
  else if (now - dnHurtT < 300) act = PMD_HURT;
  else if (now - dnHitT < 350) act = PMD_ATTACK;
  else if (now - dnStepT < 260) act = dn.faceX > 0 ? PMD_WALKR : PMD_WALKL;
  if (!pmd.has(act)) act = PMD_IDLE;
  gPmdForceScale = dnScale;
  drawPmdAct(act, ox + dn.x * DN_TILE + DN_TILE / 2, oy + dn.y * DN_TILE + DN_TILE - 4, now, true, false, 3);
  gPmdForceScale = 0;

  char b[48];
  snprintf(b, sizeof(b), "B%uF", dn.floor);
  setSize(3);
  int fw = textW(b, 3) + 24;
  gfx->fillRoundRect(CX - fw / 2, 14, fw, 36, 10, UI_INK);
  gfx->setTextColor(UI_WHITE);
  setCur(centerX(b, 3), 21);
  printT(b);

  float pct = dn.me.maxHp ? (float)dn.me.hp / dn.me.maxHp : 0;
  gfx->fillRoundRect(CX - 104, 432, 208, 16, 6, UI_INK);
  int hw = (int)(200 * pct);
  if (hw > 2) gfx->fillRoundRect(CX - 100, 436, hw, 8, 4, pct > 0.5f ? UI_BAR_OK : (pct > 0.2f ? UI_BAR_WARN : UI_BAR_BAD));

  // the latest thing that happened, while it can be read
  b[0] = 0;
  if (now - dnStepT < 1400) {
    if (dn.ate) strlcpy(b, T(S_DN_BERRY), sizeof(b));
    else if (dn.hitCount) {
      const DnHit &h = dn.hits[dn.hitCount - 1];
      snprintf(b, sizeof(b), "%s: %s!", dexName(h.mine || h.mon < 0 ? dn.me.dex : dn.mon[h.mon].f.dex), MOVE_TBL[h.move].name);
    }
  }
  setSize(2);
  if (b[0]) {
    int w = textW(b, 2) + 20;
    gfx->fillRoundRect(CX - w / 2, 390, w, 30, 8, UI_INK);
    gfx->setTextColor(UI_WHITE);
    setCur(centerX(b, 2), 397);
    printT(b);
  }

  if (dnOverT) {
    gfx->fillRoundRect(83, 130, 300, 190, 16, UI_WHITE);
    gfx->drawRoundRect(83, 130, 300, 190, 16, UI_INK);
    gfx->setTextColor(UI_INK);
    snprintf(b, sizeof(b), T(S_BTL_LOST), dexName(dn.me.dex));
    setCur(centerX(b, 2), 148);
    printT(b);
    snprintf(b, sizeof(b), "B%uF", dn.floor);
    setSize(5);
    setCur(centerX(b, 5), 180);
    printT(b);
    setSize(2);
    if (dnNewHi) {
      gfx->setTextColor(UI_BAR_WARN);
      setCur(centerX(T(S_NEW_RECORD), 2), 240);
      printT(T(S_NEW_RECORD));
    } else {
      snprintf(b, sizeof(b), T(S_RECORD_FMT), pet.dungeonHi);
      setCur(centerX(b, 2), 240);
      printT(b);
    }
    const char *msg = dn.floor >= 5 ? T(S_GREAT_JOY) : T(S_PLUS_JOY);
    gfx->setTextColor(UI_INK);
    setCur(centerX(msg, 2), 272);
    printT(msg);
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
  uint8_t fit = pmdFitScale(pmd, PMD_IDLE, 90, 90);  // big ones must not hide what is coming
  runScale = fit > 3 ? 3 : fit;
}

void closeRun() { runOpen = false; runBot = false; }

void runTap() {
  if (runOverT && millis() - runOverT > 800) closeRun();  // time to read the result
}

// forest with two layers of pines that move slower than the ground
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
      pet.runResult(secs);  // the reward and record only when it really ends
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

// ---------- pet card (vertical swipe) ----------

// x where the card's bars start. It was fixed at 150, which was plenty
// for 3-character Latin labels but not for the wider Japanese ones:
// こうげき ran into the bar. It is computed from the longest label
// so the four bars stay aligned in any language, with the
// original value as a floor (so nothing changes in Latin languages).
// Requires text size 2 to be already set, because textW() needs it.
static int statBarX() {
  const StrId ids[] = { S_STAT_ATK, S_STAT_DEF, S_STAT_SPE, S_STAT_WGT, S_VIN };
  int width = 0;
  for (StrId id : ids) {
    int w = textW(T(id), 2);
    if (w > width) width = w;
  }
  int x = 96 + width + 12;   // 12 px of space between label and bar
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
  int bw = 310 - bx;   // the bar always ends at 310, leaving space before the number
  int fw = (int)val * bw / maxBar;
  if (fw > bw) fw = bw;
  gfx->fillRoundRect(bx, y + 2, bw, 11, 3, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(bx, y + 2, fw, 11, 3, color);
}

// ---------- on-screen time setting (swipe down) ----------
// The user sets their LOCAL time by eye; the firmware uses it as is, so
// no time zone handling is needed. It preserves the day (doesn't break streak/age).

// copies the current time into the editor. The clock comes first: lastSeenEpoch is only recorded every 30 s.
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

// language pills centered on y; fills the active one
#define LANG_PILL_Y 296
#define LANG_PILL_H 30
#define LANG_PILL_X 336          // language pill (cycles through LANG_COUNT on tap)
#define LANG_PILL_W 96
static const char *const LANG_CODES[LANG_COUNT] = { "ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO" };

#ifdef BOARD_KNOMI_V2
// page dots of the time/settings screen
void drawClockDots() {
  gfx->fillCircle(CX - 9, 24, 4, clockPage == 0 ? UI_INK : UI_TRACK);
  gfx->fillCircle(CX + 9, 24, 4, clockPage == 1 ? UI_INK : UI_TRACK);
}

// password keyboard key at those coordinates, -1 if none
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
      } else if (x >= 96 && x <= 370 && y >= UPD_BTN_Y && y <= UPD_BTN_Y + 48) {
        sfxPlay(SFX_TAP);
        if (otaState() == OTA_AVAILABLE) startUpdate();
        else checkUpdate();
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
      if (y < 80) { wifiSetupBackToList(); break; }  // "< network": back to the list
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
        sfxPlay(SFX_DENY);  // fewer than 8 characters
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
  // pages and rescan
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
  // the typed text is visible: on such a small screen, getting it right without seeing it is impossible. Only the end.
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
    if (i == wifiKbHover) bg = TYPE_ACCENT[TY_WATER];
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

  // the key under the finger, large and above the field: the finger covers the real one
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
      gfx->fillRoundRect(CX - 62, 34, 124, 92, 14, TYPE_ACCENT[TY_WATER]);
      gfx->setTextColor(UI_WHITE);
      setSize(6);
      setCur(CX - (int)strlen(big) * 18, 54);
      printT(big);
    }
  }
}

// ---------- Wi-Fi updates (ota_update.h) ----------
void checkUpdate() {
  otaChecking = true;
  renderSettings();  // the check blocks: show it's working first
  otaCheck(FW_VERSION);
  otaChecking = false;
  screenKeyValid = false;
}

// the screen while installing: the loop is held until it restarts
void drawUpdateProgress(uint8_t pct) {
  static int8_t shown = -1;
  if (pct == shown) return;
  shown = pct;
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  char b[32];
  snprintf(b, sizeof(b), T(S_UPD_BUSY_FMT), pct);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(b, 2), 200);
  printT(b);
  gfx->fillRoundRect(93, 236, 280, 24, 12, UI_TRACK);
  if (pct) gfx->fillRoundRect(93, 236, 280 * pct / 100 < 24 ? 24 : 280 * pct / 100, 24, 12, UI_BAR_OK);
  gfx->flush();
}

void startUpdate() {
  if (otaState() != OTA_AVAILABLE) return;
  pet.flushSave();  // the save is never touched by an update, but nothing is left pending either
  wakeUI();
  if (!otaInstall(drawUpdateProgress)) {  // on success it restarts and doesn't come back
    sfxPlay(SFX_DENY);
    screenKeyValid = false;
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
  gfx->fillRoundRect(96, 236, 274, 48, 12, TYPE_ACCENT[TY_WATER]);
  gfx->setTextColor(UI_WHITE);
  setCur(centerX(T(S_WIFI_CHANGE), 2), 252);
  printT(T(S_WIFI_CHANGE));

  // Wi-Fi updates: check, or install the new version once one is found
  OtaState os = otaState();
  char ub[48];
  if (otaChecking) strlcpy(ub, T(S_WIFI_SEARCH), sizeof(ub));
  else if (os == OTA_AVAILABLE) snprintf(ub, sizeof(ub), T(S_UPD_TO_FMT), otaNewVersion());
  else strlcpy(ub, T(S_UPD_CHECK), sizeof(ub));
  gfx->fillRoundRect(96, UPD_BTN_Y, 274, 48, 12, os == OTA_AVAILABLE ? UI_BAR_OK : TYPE_ACCENT[TY_WATER]);
  gfx->setTextColor(UI_WHITE);
  setCur(centerX(ub, 2), UPD_BTN_Y + 16);
  printT(ub);
  if (!otaChecking && (os == OTA_LATEST || os == OTA_FAILED)) {
    const char *m = os == OTA_LATEST ? T(S_UPD_LATEST) : T(S_UPD_FAILED);
    gfx->setTextColor(os == OTA_LATEST ? UI_BAR_OK : UI_BAR_BAD);
    setCur(centerX(m, 2), UPD_BTN_Y + 60);
    printT(m);
  }
  drawCloseButton();
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
#if defined(BOARD_KNOMI_V2) && !defined(BOARD_HOST)
  drawClockDots();
#endif

  // the time is shown in 12-hour format; internally it is still 24
  char t[8];
  int h12 = clock12Hour(clockH);
  snprintf(t, sizeof(t), "%d:%02d", h12, clockM);
  setSize(7);
  setCur(CX - 105 + (h12 < 10 ? 42 : 0), 108);  // the minutes don't jump around: they stay above their buttons
  printT(t);
  setSize(3);
  setCur(CX + 109, 136);
  printT(clockH < 12 ? "AM" : "PM");

  drawClockBtn(104, 190, "-");  // hour -
  drawClockBtn(170, 190, "+");  // hour +
  drawClockBtn(252, 190, "-");  // min -
  drawClockBtn(318, 190, "+");  // min +
  setSize(2);
  gfx->setTextColor(UI_TRACK);
  setCur(120, 256);
  printT(T(S_HOUR));
  setCur(276, 256);
  printT(T(S_MIN));

  // sound switch (left of the language row)
  bool snd = audioEnabled();
  const char *sl = snd ? T(S_SND_ON) : T(S_SND_OFF);
  gfx->fillRoundRect(34, LANG_PILL_Y, 96, LANG_PILL_H, 8, snd ? UI_BAR_OK : UI_WHITE);
  gfx->drawRoundRect(34, LANG_PILL_Y, 96, LANG_PILL_H, 8, UI_INK);
  gfx->setTextColor(snd ? UI_BG_DAY : UI_INK);
  setSize(2);
  setCur(34 + (96 - textW(sl, 2)) / 2, LANG_PILL_Y + 8);
  printT(sl);

  // language selector: a pill that cycles through the 6 languages on tap
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

  drawCloseButton();

  // firmware version (discreet, at the very bottom)
  char ver[28];
  snprintf(ver, sizeof(ver), "PokeKnomiGochi v%s", FW_VERSION);
  gfx->setTextColor(UI_TRACK);
  setSize(1);
  setCur(centerX(ver, 1), 449);
  printT(ver);
  gfx->flush();
}

void clockTap(int16_t x, int16_t y) {
  if (closeHit(x, y) && wifiSetupState() == WS_OFF) {  // as swiping up does: nothing set
    sfxPlay(SFX_TAP);
    wifiSetupStop();
    clockOpen = false;
    return;
  }
#ifdef BOARD_KNOMI_V2
  if (clockPage == 1) {
    settingsTap(x, y);
    return;
  }
#endif
  if (y >= 190 && y <= 248) {  // row of +/- buttons
    clockEdited = true;
    if (x >= 104 && x < 162) clockH = (clockH + 23) % 24;
    else if (x >= 170 && x < 228) clockH = (clockH + 1) % 24;
    else if (x >= 252 && x < 310) clockM = (clockM + 59) % 60;
    else if (x >= 318 && x < 376) clockM = (clockM + 1) % 60;
    return;
  }
  if (y >= LANG_PILL_Y && y <= LANG_PILL_Y + LANG_PILL_H) {
    if (x >= 34 && x < 130) {                  // sound switch
      audioSetEnabled(!audioEnabled());
      if (audioEnabled()) sfxPlay(SFX_TAP);    // confirms when turned on
      return;
    }
    if (x >= LANG_PILL_X && x < LANG_PILL_X + LANG_PILL_W) {  // cycles language
      setLang((Lang)((gLang + 1) % LANG_COUNT));
      applyLangFont();  // the font changes with the language
      sfxPlay(SFX_TAP);
      return;
    }
  }
  if (y >= 340 && y <= 388 && x >= 133 && x <= 333) { applyClock(); return; }
}

// flame + streak number at the top left
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

// temporary banner: new medal or streak milestone
void drawCelebration() {
  const char *l1 = nullptr, *l2 = nullptr;
  // 32 and not 20: "%u にちれんぞく！" already exceeds 20 with a single digit, and snprintf
  // cut in the middle of a 3-byte character.
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

// medals on the card: badge with label, colored if earned
void drawMedalBadge(int x, int y, int i) {
  bool got = pet.hasMedal(1 << i);
  gfx->fillRoundRect(x, y, 100, 24, 6, got ? UI_BAR_OK : UI_TRACK);
  if (!got) gfx->drawRoundRect(x, y, 100, 24, 6, UI_TRACK);
  gfx->setTextColor(got ? UI_BG_DAY : 0x9492);
  setSize(2);
  setCur(x + (100 - textW(medalLabel(i), 2)) / 2, y + 5);
  printT(medalLabel(i));
}

// page 0: profile (large portrait, identity, streak, bond, berry)
// how high a sprite reaches when drawn on groundY: the top of what it fills, at drawPmdActM's scale
int pmdTopY(const PmdMon &m, int groundY, uint8_t maxS) {
  const PmdAct &a = m.acts[PMD_IDLE];
  if (!a.frames) return groundY - 120;
  uint8_t s = a.h ? 170 / a.h : 5;
  s = s < 2 ? 2 : s > maxS ? maxS : s;
  while (s > 2 && a.h * s > 250) s--;
  int top = a.h;
  for (int r = 0; r < a.h && top == a.h; r++)
    for (int c = 0; c < a.w; c++)
      if (a.data[r * a.w + c] != 0xFF) { top = r; break; }
  return groundY - ((a.base ? a.base : a.h) - top) * s;
}

void renderCardProfile() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  char head[32];
  snprintf(head, sizeof(head), T(S_NAME_FMT), pet.shiny ? "*" : "", nm, pet.level());
  gfx->setTextColor(d.accent);
  // auto-shrink: at size 3 long names don't fit in the narrow strip at
  // the top of the round screen, so they got cut off at the edge. The width is
  // MEASURED with textW(): strlen() counts bytes, and in a UTF-8 row there is not one byte
  // per character nor do they all have the same width. With the classic font textW() returns
  // strlen()*6*size, i.e. exactly the previous count: it doesn't move a pixel in
  // the six Latin languages (11 characters at size 3 are those 198 px).
  setSize(3);
  int hts = (textW(head, 3) <= 198) ? 3 : 2;
  // the name sits halfway between the top of the circle and the top of the Pokemon
  int blockH = 8 * hts + (pet.nick[0] ? 26 : 0);
  int spriteTop = pmd.loaded ? pmdTopY(pmd, 206, 4) : 120;
  int ny = (24 + spriteTop - 10 - blockH) / 2;
  if (ny < 34) ny = 34;
  setSize(hts);
  int hw = textW(head, hts);
  setCur(CX - (hw + 24) / 2, ny);  // room for the pencil after it
  printT(head);
  drawMap(SPR_PENCIL, 10, CX + (hw + 24) / 2 - 20, ny + 8 * hts / 2 - 10, 2, false);  // the name can be changed
  cardNameBottom = ny + blockH;
  if (pet.nick[0]) {  // real species under the nickname
    const char *sp = dexName(pet.speciesId);
    char par[32];
    snprintf(par, sizeof(par), "(%s)", sp);
    gfx->setTextColor(UI_TRACK);
    setSize(2);
    setCur(centerX(par, 2), ny + 8 * hts + 6);   // measured, not counted in bytes
    printT(par);
  }

  // large animated portrait
  if (pmd.loaded) drawPmdAct(PMD_IDLE, CX, 206, millis(), true, false, 4);

  // streak with flame
  int sx = 138, sy = 224;
  gfx->fillTriangle(sx + 8, sy, sx + 1, sy + 18, sx + 15, sy + 18, UI_BAR_BAD);
  gfx->fillTriangle(sx + 8, sy + 7, sx + 4, sy + 18, sx + 12, sy + 18, UI_BAR_WARN);
  char rl[40];  // in Japanese it loses digits at 30 from 100 days on
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
  if (pet.berryKnown) {  // the favorite berry's own icon before its name
    static const char *const *const BERRY_ICON[3] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP };
    int iw = textW(info, 2), x0 = CX - (iw + 42) / 2;
    drawMap(BERRY_ICON[pet.lovesBerry(0) ? 0 : pet.lovesBerry(1) ? 1 : 2], HOME_ICON_N, x0, 286, 2, false);
    setCur(x0 + 42, 296);
  } else {
    setCur(centerX(info, 2), 296);
  }
  printT(info);
}

// page 1: stats (4 bars + train button)
void renderCardStats() {
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_BATTLE), 3), 48);
  printT(T(S_BATTLE));  // the page's title: "BATTLE STATS"

  drawCardStat(118, T(S_STAT_ATK), pet.atkStat(), 260, UI_BAR_BAD);
  drawCardStat(160, T(S_STAT_DEF), pet.defStat(), 260, 0x4C98);
  drawCardStat(202, T(S_STAT_SPE), pet.speStat(), 260, UI_BAR_WARN);
  drawCardStat(244, T(S_STAT_WGT), pet.weight, 100, 0xB3C8);

#ifndef BOARD_KNOMI_V2  // on the Knomi, battle opens with the physical button
  gfx->fillRoundRect(96, 300, 274, 40, 12, UI_BAR_BAD);
  gfx->setTextColor(UI_BG_DAY);
  setSize(2);
  setCur(centerX(T(S_BTL_START), 2), 311);
  printT(T(S_BTL_START));
#endif
  setSize(2);
  char wins[40];  // "VITTORIE IN BATTAGLIA: 65535" and a Japanese line run past 24
  snprintf(wins, sizeof(wins), T(S_WINS_FMT), pet.battleWins);
  gfx->setTextColor(UI_INK);
  int ww = textW(wins, 2), x0 = CX - (ww + 32) / 2;  // a trophy, then the battles won
#ifdef BOARD_KNOMI_V2
  const int wy = 300;  // under the bars, clear of the page dots
#else
  const int wy = 270;  // above the battle button
#endif
  drawMap(SPR_TROPHY, 12, x0, wy - 4, 2, false);
  setCur(x0 + 32, wy);
  printT(wins);
}

// page 2: training, which adds to ATK, DEF and SPD on top of what levels give, 0..100 each,
// with what trains each one written under its bar; and weight, which food adds and play burns
// (not a goal, so no MAX)
void drawCardTrainRow(int y, const char *label, uint8_t tr, uint16_t color, const char *how, bool maxes) {
  drawCardStat(y, label, tr, 100, color);
  if (maxes && tr >= 100) {  // full: MAX in place of the number
    gfx->fillRect(330, y, 60, 16, UI_BG_DAY);
    gfx->setTextColor(color);
    setCur(330, y);
    printT("MAX");
  }
  gfx->setTextColor(lerp565(UI_TRACK, UI_INK, 11, 16));
  setCur(96, y + 22);  // under its own bar, clear of the next row
  printT(how);
}

void renderCardTraining() {
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_TRAINING), 3), 48);
  printT(T(S_TRAINING));
  setSize(2);
  drawCardTrainRow(110, T(S_STAT_ATK), pet.trAtk, UI_BAR_BAD, T(S_TRAIN_HOW_ATK), true);
  drawCardTrainRow(166, T(S_STAT_DEF), pet.trDef, 0x4C98, T(S_TRAIN_HOW_DEF), true);
  drawCardTrainRow(222, T(S_STAT_SPE), pet.trSpe, UI_BAR_WARN, T(S_TRAIN_HOW_SPE), true);
  drawCardTrainRow(278, T(S_STAT_WGT), pet.weight, 0xB3C8, T(S_TRAIN_HOW_WGT), false);
}

// pages 2 and 3: the pet's medals, then the trainer's, with descriptive label
// The achievements, off to the right of home: the pet's medals, the trainer's, the zones'. At most
// 6 a page in a 2x3 grid centred under the title, so none is cut off by the round screen.
#define ACH_PER_PAGE 6
#define ACH_BOX_W 196
#define ACH_BOX_H 52
#define ACH_GAP 10
// page p: its group (first medal, how many in all, title) and which of them it shows
static void achPage(uint8_t p, int &first, int &count, StrId &title, int &from, int &n) {
  static const struct { uint8_t first, count; StrId title; } G[3] = {
    { 0, MED_PET_COUNT, S_MEDALS_FMT },
    { MED_PET_COUNT, MED_FIRST_ZONE - MED_PET_COUNT, S_TRAINER_FMT },
    { MED_FIRST_ZONE, MED_COUNT - MED_FIRST_ZONE, S_ZONE_MEDALS_FMT } };
  for (int g = 0, page = 0; g < 3; g++) {
    int pages = (G[g].count + ACH_PER_PAGE - 1) / ACH_PER_PAGE;
    if (p < page + pages) {
      first = G[g].first;
      count = G[g].count;
      title = G[g].title;
      from = first + (p - page) * ACH_PER_PAGE;
      n = count - (from - first) < ACH_PER_PAGE ? count - (from - first) : ACH_PER_PAGE;
      return;
    }
    page += pages;
  }
  first = count = from = n = 0;
  title = S_MEDALS_FMT;
}

static void drawCheck(int x, int y, uint16_t c) {  // a check mark, 3 px thick
  for (int o = 0; o < 3; o++) {
    gfx->drawLine(x, y + 8 + o, x + 6, y + 14 + o, c);
    gfx->drawLine(x + 6, y + 14 + o, x + 17, y + 2 + o, c);
  }
}

void renderAchievements(uint8_t p) {
  int first, count, from, n;
  StrId title;
  achPage(p, first, count, title, from, n);
  int got = 0;
  for (int i = first; i < first + count; i++) got += pet.hasMedal(1UL << i);
  char head[40];
  snprintf(head, sizeof(head), T(title), got, count);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(head, 3), 48);
  printT(head);

  int rows = (n + 1) / 2, top = (92 + 360 - (rows * ACH_BOX_H + (rows - 1) * ACH_GAP)) / 2;
  setSize(2);
  for (int k = 0; k < n; k++) {
    int i = from + k, row = k / 2;
    bool alone = k == n - 1 && n % 2;  // an odd one out sits in the middle
    int x = alone ? CX - ACH_BOX_W / 2 : CX - ACH_BOX_W - ACH_GAP / 2 + (k % 2) * (ACH_BOX_W + ACH_GAP);
    int y = top + row * (ACH_BOX_H + ACH_GAP);
    bool g = pet.hasMedal(1UL << i);
    gfx->fillRoundRect(x, y, ACH_BOX_W, ACH_BOX_H, 10, g ? UI_BAR_OK : UI_TRACK);
    if (g) drawCheck(x + 12, y + ACH_BOX_H / 2 - 9, UI_WHITE);
    gfx->setTextColor(g ? UI_BG_DAY : 0x8410);
    // the name, on two lines when it doesn't fit on one
    const char *d = medalDesc(i);
    int tx = x + 38, avail = ACH_BOX_W - 44;
    if (textW(d, 2) <= avail) {
      setCur(tx, y + ACH_BOX_H / 2 - 8);
      printT(d);
    } else {
      char a[40], b[40];
      strlcpy(a, d, sizeof(a));
      char *cut = nullptr;
      for (char *c = a; *c; c++)
        if (*c == ' ') {
          *c = 0;
          if (textW(a, 2) <= avail) cut = c;
          *c = ' ';
        }
      if (cut) {
        *cut = 0;
        strlcpy(b, cut + 1, sizeof(b));
      } else {
        b[0] = 0;
      }
      setCur(tx, y + 8);
      printT(a);
      setCur(tx, y + 28);
      printT(b);
    }
  }
}

// the card's last page: progress (level, evolution, mistakes) — surfaces mechanics
// that used to be invisible (how long until level-up/evolution and why)
void renderCardProgress() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_PROGRESS), 3), 44);
  printT(T(S_PROGRESS));

  // large level
  char lv[10];
  snprintf(lv, sizeof(lv), T(S_LVL_FMT), pet.level());
  setSize(5);
  setCur(centerX(lv, 5), 86);
  printT(lv);

  // progress bar to the next level (1 level = 60 min of play)
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

  // evolution status
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

  // care mistakes (delay evolution)
  char ms[24];
  snprintf(ms, sizeof(ms), T(S_MISTAKES_FMT), pet.careMistakes);
  gfx->setTextColor(pet.careMistakes > 0 ? UI_BAR_BAD : UI_INK);
  setCur(centerX(ms, 2), 312);
  printT(ms);
}

void renderCard() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  if (cardAch) renderAchievements(cardPage);
  else if (cardPage == 0) renderCardProfile();
  else if (cardPage == 1) renderCardStats();
  else if (cardPage == 2) renderCardTraining();
  else renderCardProgress();

  // page indicator + help
  int pages = cardPageCount();
  for (int i = 0; i < pages; i++) {
    int dx = CX + (2 * i - (pages - 1)) * 13;
    if (i == cardPage) gfx->fillCircle(dx, 374, 5, UI_INK);
    else gfx->drawCircle(dx, 374, 4, UI_INK);
  }
  drawCloseButton();
  gfx->flush();
}

// ---------- keyboard for renaming ----------

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
  // current buffer
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
  // filter BEFORE dividing: with x/y below KB_X/KB_Y the subtraction is negative
  // and integer division truncates toward zero (not toward -inf), so col/row
  // would come out 0 instead of negative and the filter below wouldn't catch it
  if (x < KB_X || y < KB_Y) return;
  int col = (x - KB_X) / KB_W, row = (y - KB_Y) / KB_H;
  if (col >= KB_COLS || row >= 5) return;
  int i = row * KB_COLS + col;
  if (i >= 30) return;
  if (i == 28) {  // delete
    if (nameLen) nameBuf[--nameLen] = 0;
  } else if (i == 29) {  // OK
    pet.rename(nameBuf);
    kbOpen = false;
  } else if (nameLen < sizeof(nameBuf) - 1) {
    nameBuf[nameLen++] = KB_KEYS[i];
    nameBuf[nameLen] = 0;
  }
}

// ---------- pokedex gallery ----------

#define GAL_X 73
#define GAL_Y 72
#define GAL_DOTS_Y 400   // a grid's page dots, just under it, well clear of the close button
#define GAL_CLOSE_Y 432  // and the close button under them
#define GAL_CELL 80

// draws a thumbnail centered in its cell; sil=true paints it in ink
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

// type, evolution and fun fact of an already-found species
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
  if (d.evolvesTo) {  // the next form is not revealed until found
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

// a Pokedex entry's sprites: the regular one, and the shiny beside it when both are caught (a
// shiny alone takes galleryPmd's place)
void dexDetailLoad(int16_t dex) {
  bool regular = pet.hasForm(dex, false), shiny = pet.hasForm(dex, true);
  galleryPmd.load(dex, shiny && !regular);
  if (regular && shiny) galleryShinyPmd.load(dex, true);
  else galleryShinyPmd.unload();
}

void galleryUnload() {
  galleryPmd.unload();
  galleryShinyPmd.unload();
}

// a Pokedex entry, full screen (the Pokedex and the zones' grids); galleryPmd holds its sprite
void renderDexDetail(int16_t dex) {
    gfx->fillScreen(RGB565_BLACK);
    gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
    const DexEntry &d = DEX_TBL[dex];
    bool reg = pet.isRegistered(dex);
    char head[32];
    snprintf(head, sizeof(head), "N.%03d %s", dex, reg ? dexName(dex) : "???");
    gfx->setTextColor(reg ? d.accent : UI_INK);
    // auto-shrink long names (they don't fit at t3), measuring instead of counting
    // bytes; 13 characters at size 3 are the 234 px from before.
    setSize(3);
    int gts = (textW(head, 3) <= 234) ? 3 : 2;
    setSize(gts);
    setCur(centerX(head, gts), gts == 3 ? 56 : 60);
    printT(head);
    if (reg && galleryPmd.loaded && galleryShinyPmd.loaded) {  // both forms, side by side
      uint8_t s = min(pmdFitScale(galleryPmd, PMD_IDLE, 150, 150), pmdFitScale(galleryShinyPmd, PMD_IDLE, 150, 150));
      if (s > 3) s = 3;
      drawPmdActM(galleryPmd, PMD_IDLE, CX - 84, 240, millis(), true, false, s);
      drawPmdActM(galleryShinyPmd, PMD_IDLE, CX + 84, 240, millis(), true, false, s);
      drawDexInfo(dex);
    } else if (reg) {
      if (galleryPmd.loaded) drawPmdActM(galleryPmd, PMD_IDLE, CX, 240, millis(), true, false, 4);
      else if (const uint8_t *t = thumbs.get(dex)) drawThumb(t, CX - GAL_CELL, 110, 3, false);
      drawDexInfo(dex);
    } else if (galleryPmd.loaded) {
      drawPmdActM(galleryPmd, PMD_IDLE, CX, 300, 0, true, true, 6);  // static silhouette ("?" style)
    } else if (const uint8_t *t = thumbs.get(dex)) {
      drawThumb(t, CX - GAL_CELL, 135, 4, true);
    }
    drawCloseButton();  // a tap anywhere goes back too
    gfx->flush();
}

// a square of the Pokedex with the shiny form caught: a gold sparkle in its corner
void drawShinyMark(int x, int y, int cell) { drawSparkle(x + cell - 12, y + 12, 7); }

void renderGallery() {
  if (galleryDetail) {  // detail view: always redrawn (animated)
    renderDexDetail(galleryDetail);
    return;
  }

  if (!galleryDirty) return;  // the grid is static
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
        drawThumb(thumbs.getForm(dex, !pet.hasForm(dex, false) && pet.hasForm(dex, true)), x, y, 2, !pet.isRegistered(dex));
        if (pet.isShinyRegistered(dex)) drawShinyMark(x, y, GAL_CELL);
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
  // page dots
  for (int i = 0; i < 10; i++) {
    if (i == galleryPage) gfx->fillCircle(170 + i * 14, GAL_DOTS_Y, 4, UI_INK);
    else gfx->drawCircle(170 + i * 14, GAL_DOTS_Y, 3, UI_INK);
  }
  drawCloseButtonAt(GAL_CLOSE_Y);
  gfx->flush();
}

void galleryTap(int16_t x, int16_t y) {
  if (galleryDetail) {  // back to the grid
    galleryDetail = 0;
    galleryUnload();
    galleryDirty = true;
    return;
  }
  if (y < 66 || closeHitAt(x, y, GAL_CLOSE_Y)) {  // the close button (or the header) = exit
    sfxPlay(SFX_TAP);
    galleryOpen = false;
    galleryUnload();
    return;
  }
  // same reason as in the keyboard: filter before dividing
  if (x < GAL_X || y < GAL_Y) return;
  int c = (x - GAL_X) / GAL_CELL, r = (y - GAL_Y) / GAL_CELL;
  if (c > 3 || r > 3) return;
  int16_t dex = galleryPage * 16 + r * 4 + c + 1;
  if (dex > 151) return;
  galleryDetail = dex;
  dexDetailLoad(dex);
}

// ---------- egg choice ----------
// same grid as the Pokedex, but without gaps: only what has already been registered

#define PICK_BTN_Y 316
#define PICK_BTN_H 56

void renderEggPick() {
  if (eggPickDetail) {  // animated: always redrawn
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
    drawCloseButton();
    gfx->flush();
    return;
  }

  if (!eggPickDirty) return;  // the grid is static
  eggPickDirty = false;

  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  drawOwnedGrid(T(S_CHOOSE_POKEMON), eggPickPage, 0);
  gfx->flush();
}

// grid of what is registered, without gaps
// locked: shown grey and can't be picked (the one you are raising, when trading); 0 = none
void drawOwnedGrid(const char *title, int page, int16_t locked) {
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(title, 2), 50);
  printT(title);
  for (int i = 0; i < 16; i++) {
    int16_t dex = pet.registeredAt(page * 16 + i);
    if (!dex) break;
    if (const uint8_t *t = thumbs.get(dex)) drawThumb(t, GAL_X + (i % 4) * GAL_CELL, GAL_Y + (i / 4) * GAL_CELL, 2, dex == locked);
  }
  int pages = (pet.registeredCount() + 15) / 16;
  if (pages > 1) {
    for (int i = 0; i < pages; i++) {
      int dotX = CX + (i * 2 - (pages - 1)) * 7;
      if (i == page) gfx->fillCircle(dotX, GAL_DOTS_Y, 4, UI_INK);
      else gfx->drawCircle(dotX, GAL_DOTS_Y, 3, UI_INK);
    }
  }
}

int16_t ownedGridAt(int16_t x, int16_t y, int page) {
  if (x < GAL_X || y < GAL_Y) return 0;
  int c = (x - GAL_X) / GAL_CELL, r = (y - GAL_Y) / GAL_CELL;
  if (c > 3 || r > 3) return 0;
  return pet.registeredAt(page * 16 + r * 4 + c);
}

void eggPickTap(int16_t x, int16_t y) {
  if (eggPickDetail) {
    if (y >= PICK_BTN_Y && y < PICK_BTN_Y + PICK_BTN_H && abs(x - CX) <= 110) {
      pet.chooseEgg(eggPickDetail);
      sfxPlay(SFX_TAP);
    } else if (closeHit(x, y)) {
      sfxPlay(SFX_TAP);
    } else {
      return;
    }
    eggPickDetail = 0;
    eggPickPage = 0;
    eggPickDirty = true;
    galleryUnload();
    return;
  }
  int16_t dex = ownedGridAt(x, y, eggPickPage);
  if (!dex) return;
  sfxPlay(SFX_TAP);
  eggPickDetail = dex;
  galleryPmd.load(dex, false);  // the shiny remains a surprise
}

// ---------- buddy: another PokeKnomiGochi on the same wifi ----------
#define BUDDY_PILL_Y 116
#define BUDDY_PILL_H 32

const char *buddyNameOf(int i) {
  const BuddyPeer &p = bPeers[i];
  return p.nick[0] ? p.nick : (buddyDexOk(p.dex) ? dexName(p.dex) : "?");
}
const char *buddyName() { return buddyNameOf(bCur); }

BuddyMsg buddyMsg(BuddyType type) {
  BuddyMsg m;
  buddyInit(m, type, buddyNetId(), type == BM_HELLO ? 0 : bPeers[bCur].id, bsId);
  return m;
}

void buddySetNote(uint8_t fmt) {
  snprintf(buddyNote, sizeof(buddyNote), T((StrId)fmt), buddyName());
  buddyNoteUntil = millis() + 3000;
}

void buddySendHello() {
  BuddyMsg m = buddyMsg(BM_HELLO);
  m.dex = pet.speciesId;
  m.level = btlCapLevel(pet.level());
  m.shiny = pet.shiny;
  strlcpy(m.nick, pet.nick, sizeof(m.nick));
  for (int16_t d = 1; d <= 151; d++)
    if (pet.isRegistered(d)) m.reg[(d - 1) >> 3] |= 1 << ((d - 1) & 7);
  buddyNetSend(m);
}

void buddySendFighter(bool ack) {
  BuddyMsg m = buddyMsg(BM_FIGHTER);
  BtlFighter f = petFighter(btlCapLevel(pet.level()));
  m.ack = ack;
  m.dex = f.dex;
  m.level = f.level;
  m.shiny = pet.shiny;
  m.hp = f.maxHp;
  m.atk = f.atk;
  m.def = f.def;
  m.spe = f.spe;
  buddyNetSend(m);
}

void buddySendMove(uint8_t turn, uint8_t move, bool ack) {
  BuddyMsg m = buddyMsg(BM_MOVE);
  m.turn = turn;
  m.move = move;
  m.ack = ack;
  buddyNetSend(m);
}

void buddySendTrade(BuddyType type, int16_t dex, bool ack) {
  BuddyMsg m = buddyMsg(type);
  m.dex = dex;
  m.shiny = dex == trMine && trMineShiny;  // the form offered (older versions send 0: regular)
  m.ack = ack;
  buddyNetSend(m);
}

void buddyEndSession() {
  bsState = BS_IDLE;
  bsId = 0;
  if (choiceKind == 5) choiceKind = 0;
}

void buddyInvite(uint8_t kind) {
  bsKind = kind;
  bsId = esp_random() | 1;
  bsState = BS_INVITING;
  bsT = millis();
  bsResendT = 0;  // goes out on the next tick
  strlcpy(buddyNote, T(S_BUDDY_WAITING), sizeof(buddyNote));
  buddyNoteUntil = millis() + BUDDY_REPLY_MS;
}

void buddyStartSession() {
  bool inviter = bsState == BS_INVITING;  // the one who invited runs the showdown
  buddyNoteUntil = 0;
  if (!onMainScreen()) {  // something else opened while waiting
    buddyNetSend(buddyMsg(BM_LEAVE));
    buddyEndSession();
    return;
  }
  bsT = millis();
  bsResendT = 0;
  if (bsKind == BK_BATTLE) {
    bsState = BS_SYNC;
    return;
  }
  if (bsKind == BK_SHOWDOWN) {
    bsState = BS_PLAY;
    sdStart(inviter);
    return;
  }
  if (bsKind == BK_VOLLEY) {
    bsState = BS_PLAY;
    vbStart(inviter);
    return;
  }
  bsState = BS_TRADE;
  tradeOpen = true;
  trPhase = TR_PICK;
  trMine = trTheirs = 0;
  trMineShiny = trTheirsShiny = false;
  trPage = 0;
  trTheirConfirm = false;
  trT = millis();
}

void buddyAcceptInvite() {
  choiceKind = 0;
  bsState = BS_INVITED;  // the one accepting is the guest, even after inviting each other at once
  buddyNetSend(buddyMsg(BM_ACCEPT));
  buddyStartSession();
}

void buddyDeclineInvite() {
  buddyNetSend(buddyMsg(BM_DECLINE));
  buddyEndSession();
}

// the buddy's data arrives: the battle begins. HP is that of a full battle on both boards.
void buddyStartBattle(const BuddyMsg &m) {
  bFoeShiny = m.shiny;
  wildPmd.load(m.dex, m.shiny);
  if (!wildPmd.loaded) {
    buddyNetSend(buddyMsg(BM_LEAVE));
    buddyEndSession();
    return;
  }
  gamesMenuOpen = false;
  bMe = petFighter(btlCapLevel(pet.level()));
  bFoe = BtlFighter();
  bFoe.dex = m.dex;
  bFoe.level = m.level;
  bFoe.hp = bFoe.maxHp = m.hp;
  bFoe.atk = m.atk;
  bFoe.def = m.def;
  bFoe.spe = m.spe;
  bMeMoves = btlMovesFor(bMe.dex, bMe.level, bFoe.dex);
  bFoeMoves = btlMovesFor(bFoe.dex, bFoe.level, bMe.dex);
  bShownMe = bMe.hp;
  bShownFoe = bFoe.hp;
  bWon = false;
  bGain = 0;
  bPvp = true;
  bPvpGone = false;
  bRng = bsId;
  bPvpTurn = 0;
  bPvpFoeTurn = -1;
  battleOpen = true;
  bsState = BS_BATTLE;
  wakeUI();
  battleSetPhase(BP_INTRO);
  sfxPlay(SFX_PLAY);
  Serial.printf("buddy: battle vs #%d Lv.%u\n", m.dex, m.level);
}

void buddyGoneFromBattle() {
  bPvpGone = true;
  battleSetPhase(BP_RAN);
}

void buddyBattleClosed() {
  if (bPhase != BP_END && !bPvpGone) buddyNetSend(buddyMsg(BM_LEAVE));
  bPvp = bPvpGone = false;
  buddyEndSession();
}

void tradeClose(bool notify) {
  if (notify) buddyNetSend(buddyMsg(BM_LEAVE));
  tradeOpen = false;
  buddyEndSession();
}

void buddyHandle(const BuddyMsg &m, uint32_t now) {
  if (m.type == BM_HELLO) {
    bool arrived = false;
    int i = buddyHello(bPeers, m, now, &arrived);
    if (i >= 0 && arrived) {
      if (!bPeers[bCur].on || bCur == i) buddySelect(i);
      sfxPlay(SFX_MEDAL);
      wakeUI();
      Serial.printf("buddy: arrives #%d Lv.%u (%d here)\n", m.dex, m.level, buddyCount(bPeers));
    }
    return;
  }
  int from = buddyFind(bPeers, m.from);
  if (from < 0) return;  // must say hello first
  if (from != bCur) {
    // another buddy: a free board takes its invitation; a busy one turns it down
    if (m.type == BM_INVITE && bsState == BS_IDLE) {
      buddySelect(from);
    } else {
      if (m.type == BM_INVITE) {
        BuddyMsg d;
        buddyInit(d, BM_DECLINE, buddyNetId(), m.from, m.session);
        buddyNetSend(d);
      }
      return;
    }
  }

  if (m.type == BM_INVITE) {
    if (bsState == BS_IDLE) {
      bool free = onMainScreen() && !pet.isEgg() && !pet.sleeping && !pet.ceremony && !choiceKind &&
                  (m.kind != BK_TRADE || pet.tradeableCount() > 0);  // nothing to trade: no
      if (!free) {
        bsId = m.session;
        buddyNetSend(buddyMsg(BM_DECLINE));
        bsId = 0;
        return;
      }
      bsId = m.session;
      bsKind = m.kind;
      bsState = BS_INVITED;
      choiceKind = 5;
      choiceUntil = now + 12000;
      screenOff = false;
      wakeUI();
      sfxPlay(SFX_MEDAL);
    } else if (bsState == BS_INVITING && m.from < buddyNetId()) {
      bsId = m.session;  // they invited each other at once: the invitation from the lower id wins
      bsKind = m.kind;
      buddyAcceptInvite();
    } else if (m.session == bsId && bsState != BS_INVITED) {
      buddyNetSend(buddyMsg(BM_ACCEPT));  // the ACCEPT was lost
    }
    return;
  }
  if (m.session != bsId || bsState == BS_IDLE) return;

  switch (m.type) {
    case BM_ACCEPT:
      if (bsState == BS_INVITING) buddyStartSession();
      break;
    case BM_DECLINE:
      if (bsState == BS_INVITING) {
        buddySetNote(S_BUDDY_BUSY_FMT);
        buddyEndSession();
      }
      break;
    case BM_ROUND:
    case BM_ANSWER:
    case BM_RESULT:
      if (bsState == BS_INVITING && bsKind == BK_SHOWDOWN) buddyStartSession();  // the ACCEPT was lost
      sdHandle(m);
      break;
    case BM_VBALL:
    case BM_VMISS:
      if (bsState == BS_INVITING && bsKind == BK_VOLLEY) buddyStartSession();
      vbHandle(m);
      break;
    case BM_LEAVE:
      if (sdOpen || vbOpen) {
        buddySetNote(S_BUDDY_LEFT_FMT);
        if (sdOpen) sdClose();
        if (vbOpen) vbClose();
        break;
      }
      if (bPvp) {
        if (bPhase < BP_END) buddyGoneFromBattle();
      } else if (tradeOpen && trPhase != TR_DONE) {
        buddySetNote(S_BUDDY_LEFT_FMT);
        tradeClose(false);
      } else if (bsState != BS_TRADE) {
        buddySetNote(S_BUDDY_LEFT_FMT);
        buddyEndSession();
      }
      break;
    case BM_FIGHTER:
      if (bsState == BS_INVITING) buddyStartSession();
      if (!m.ack) buddySendFighter(true);
      if (bsState == BS_SYNC && bsKind == BK_BATTLE) buddyStartBattle(m);
      break;
    case BM_MOVE:
      if (!bPvp) break;
      if (m.turn == bPvpTurn) {
        bPvpFoeTurn = m.turn;
        bPvpFoeMove = m.move < bFoeMoves.count ? m.move : 0;
        if (!m.ack && bPhase == BP_WAIT) buddySendMove(bPvpTurn, bPvpMine, true);
      } else if (!m.ack && (uint8_t)(m.turn + 1) == bPvpTurn) {
        buddySendMove(m.turn, bPvpMinePrev, true);  // the buddy didn't receive the previous move
      }
      break;
    case BM_OFFER:
      if (bsState == BS_INVITING) buddyStartSession();
      if (!tradeOpen || !buddyRegHas(bPeers[bCur].reg, m.dex)) break;
      trTheirs = m.dex;
      trTheirsShiny = m.shiny != 0;
      if (!m.ack && trMine) buddySendTrade(BM_OFFER, trMine, true);
      if (trPhase == TR_WAIT) trPhase = TR_REVIEW;
      break;
    case BM_CONFIRM:
      if (!tradeOpen) break;
      trTheirConfirm = true;
      if (!m.ack && (trPhase == TR_WAITC || trPhase == TR_DONE)) buddySendTrade(BM_CONFIRM, trMine, true);
      break;
    default: break;
  }
}

void tradeFinish() {
  bool isNew = pet.tradeAway(trMine, trTheirs, trMineShiny, trTheirsShiny);  // yours leaves, theirs comes in
  Serial.printf("buddy: trade, gives #%d%s receives #%d%s new=%d\n", trMine, trMineShiny ? " shiny" : "", trTheirs,
                trTheirsShiny ? " shiny" : "", isNew);
  sfxPlay(SFX_MEDAL);
  trPhase = TR_DONE;
  trT = millis();
}

void buddyTick(uint32_t now) {
  if (buddyFake)  // never time out (until reboot: they only exist in test mode)
    for (BuddyPeer &p : bPeers) p.seen = now;
  if (!buddyNetUp()) {
    if (!buddyFake)
      for (BuddyPeer &p : bPeers) p.on = false;
  } else {
    if (now - buddyHelloT > BUDDY_HELLO_MS && !pet.isEgg() && !pet.ceremony && !pet.awaitingEggPick() && !pet.awaitingStarter()) {
      buddyHelloT = now;
      buddySendHello();
    }
    BuddyMsg m;
    for (int i = 0; i < 8 && buddyNetPoll(m); i++) buddyHandle(m, now);
  }
  // touches and messages have already stamped bsT and trT with millis(): with the old now the subtraction would wrap around
  now = millis();

  for (int i = 0; i < BUDDY_MAX; i++) {
    if (buddyPeerGone(bPeers[i], now)) bPeers[i].on = false;
    if (!bPeers[i].on && buddyPmdDex[i]) {
      buddyPmd[i].unload();
      buddyPmdDex[i] = 0;
    }
    // the sprite is loaded outside the network code: if it doesn't fit, that buddy is simply not drawn
    if (bPeers[i].on && buddyPmdDex[i] != bPeers[i].dex && visitPhase == VP_NONE) {
      buddyPmd[i].load(bPeers[i].dex, bPeers[i].shiny);
      buddyPmdDex[i] = bPeers[i].dex;
    }
  }
  pet.buddiesOnScreen(partyBuddies());
  if (!bPeers[bCur].on) {
    buddyNetSetPeer(0);
    buddyMenuOpen = playMenuOpen = false;
    if (sdOpen || vbOpen) {
      buddySetNote(S_BUDDY_LEFT_FMT);
      if (sdOpen) sdClose();
      if (vbOpen) vbClose();
    }
    if (bPvp && bPhase < BP_END) {
      if (bPhase != BP_RAN) buddyGoneFromBattle();
    } else if (tradeOpen && trPhase != TR_DONE) {
      tradeClose(false);
    } else if (bsState != BS_IDLE && !bPvp && !tradeOpen) {
      buddyEndSession();
    }
    // with nothing left in progress, the menu moves on to another buddy that is still here
    if (bsState == BS_IDLE && !bPvp && !tradeOpen)
      for (int i = 0; i < BUDDY_MAX; i++)
        if (bPeers[i].on) {
          buddySelect(i);
          break;
        }
    return;
  }

  if (bsState == BS_INVITED && choiceKind != 5) buddyDeclineInvite();  // time ran out
  sdTick(now);
  vbTick(now);

  if (bsState == BS_INVITING && now - bsT > BUDDY_REPLY_MS) {
    buddySetNote(S_BUDDY_BUSY_FMT);
    buddyEndSession();
  }
  if (bsState == BS_SYNC && now - bsT > BUDDY_REPLY_MS) buddyEndSession();
  if (tradeOpen && trPhase != TR_DONE && now - trT > 90000UL) tradeClose(true);  // nobody responds
  if (tradeOpen && trPhase == TR_DONE && now - trT > 3500) tradeClose(false);
  if (tradeOpen && trPhase == TR_WAITC && trTheirConfirm) tradeFinish();

  if (now - bsResendT < BUDDY_RESEND_MS) return;
  bsResendT = now;
  if (bsState == BS_INVITING) {
    BuddyMsg m = buddyMsg(BM_INVITE);
    m.kind = bsKind;
    buddyNetSend(m);
  } else if (bsState == BS_SYNC) {
    buddySendFighter(false);
  } else if (bPvp && bPhase == BP_WAIT) {
    buddySendMove(bPvpTurn, bPvpMine, false);
  } else if (tradeOpen && trPhase == TR_WAIT) {
    buddySendTrade(BM_OFFER, trMine, false);
  } else if (tradeOpen && trPhase == TR_WAITC) {
    buddySendTrade(BM_CONFIRM, trMine, false);
  }
}

bool buddyVisible() {
  return buddyCount(bPeers) && visitPhase == VP_NONE && !pet.isEgg() && !pet.sleeping && !pet.ceremony;
}

// buddies on the main screen right now (0 = none): with 2 or more everyone stands in a row
// buddies standing on the main screen, for where everyone stands: they stay while yours sleeps
// (only tapping them and their notice wait until it wakes up: buddyVisible)
int partyBuddies() {
  bool shown = buddyCount(bPeers) && visitPhase == VP_NONE && !pet.isEgg() && !pet.ceremony;
  return shown ? buddyCount(bPeers) : 0;
}

// others on the main screen (buddies, or the daily visitor while it is here): then yours stands
// in the middle, bigger (partyBuddyX / partyScale in buddy.h)
int partyOthers() {
  int n = partyBuddies();
  if (!n && (visitPhase == VP_ENTER || visitPhase == VP_HANG || visitPhase == VP_LEAVE)) n = 1;
  return n;
}

// the buddy the menu, battles and trades are about; direct messages go to it
void buddySelect(int i) {
  bCur = i;
  buddyNetSetPeer(bPeers[i].id);
}

void drawBuddyPet(uint32_t now) {
  int n = partyBuddies(), k = 0;
  if (!n) return;  // not standing on the main screen right now (an egg, a farewell, a visitor)
  for (int i = 0; i < BUDDY_MAX; i++) {
    if (!bPeers[i].on) continue;
    k++;  // party slot: in arrival-slot order, after yours
    if (!buddyPmd[i].loaded) continue;
    uint8_t act = ((now + i * 1300) / 5000) % 2 && buddyPmd[i].has(PMD_HOP) ? PMD_HOP : PMD_IDLE;
    drawPmdActM(buddyPmd[i], act, partyBuddyX(n, k), PET_GROUND, now, true, false, partyOthersScale(n));
  }
  if (n == 1 && (now / 900) % 4 == 0) drawMap(SPR_HEART, 32, 286, PET_GROUND - 150, 1, false);  // they get along
}

// the buddy notice and, on top of everything, the notes about what happened
void drawBuddyTop() {
  char b[44];
  setSize(2);
  if (timeLeft(buddyNoteUntil)) {
    int w = textW(buddyNote, 2) + 24;
    gfx->fillRoundRect(CX - w / 2, BUDDY_PILL_Y, w, BUDDY_PILL_H, 12, UI_INK);
    gfx->setTextColor(UI_WHITE);
    setCur(centerX(buddyNote, 2), BUDDY_PILL_Y + 8);
    printT(buddyNote);
  } else if (buddyVisible()) {
    int n = buddyCount(bPeers);
    if (n > 1) snprintf(b, sizeof(b), T(S_BUDDIES_FMT), n);
    else snprintf(b, sizeof(b), T(S_BUDDY_HERE_FMT), buddyName());
    int w = textW(b, 2) + 24;
    gfx->fillRoundRect(CX - w / 2, BUDDY_PILL_Y, w, BUDDY_PILL_H, 12, C565(0x1e, 0x7d, 0x3c));
    gfx->setTextColor(UI_WHITE);
    setCur(centerX(b, 2), BUDDY_PILL_Y + 8);
    printT(b);
  }
}

// the buddy under a touch on the main screen (its notice counts too), -1 if none
int buddyAt(int16_t x, int16_t y) {
  int n = partyBuddies(), k = 0;
  if (!n || !buddyVisible()) return -1;  // asleep: they are there, but not for playing
  if (y >= BUDDY_PILL_Y - 6 && y <= BUDDY_PILL_Y + BUDDY_PILL_H + 6 && x >= 60 && x <= 406) return bCur;
  for (int i = 0; i < BUDDY_MAX; i++) {
    if (!bPeers[i].on) continue;
    k++;
    if (abs(x - partyBuddyX(n, k)) < partyHalf(n) && y > PET_GROUND - partyTall(n) && y < PET_GROUND) return i;
  }
  return -1;
}

// the buddy menu's rows: Battle, Trade, Play (the close button under them)
#define BUDDY_ROWS 3
#define BUDDY_ROW_Y 172
#define BUDDY_ROW_STEP 58
#define BUDDY_ROW_H 48
static const StrId BUDDY_ROW_LABEL[BUDDY_ROWS] = { S_BUDDY_BATTLE, S_BUDDY_TRADE, S_PLAY_BTN };
static const uint16_t BUDDY_ROW_COLOR[BUDDY_ROWS] = { UI_BAR_BAD, C565(0x1e, 0x7d, 0x3c), TYPE_ACCENT[TY_PSYCHIC] };

void renderBuddyMenu() {
  if (!bPeers[bCur].on) { buddyMenuOpen = false; return; }
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  char b[44];
  snprintf(b, sizeof(b), T(S_BUDDY_HERE_FMT), buddyName());
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(b, 2), 66);
  printT(b);
  if (buddyPmd[bCur].loaded) drawPmdActM(buddyPmd[bCur], PMD_IDLE, CX, 160, millis(), true, false, 2);
  int n = buddyCount(bPeers);
  if (n > 1) {  // several: swipe to the others
    int pos = 1;
    for (int i = 0; i < bCur; i++) pos += bPeers[i].on;
    snprintf(b, sizeof(b), "< %d/%d >", pos, n);
    gfx->setTextColor(UI_TRACK);
    setCur(centerX(b, 2), 92);
    printT(b);
  }
  for (int i = 0; i < BUDDY_ROWS; i++) {
    int y = BUDDY_ROW_Y + i * BUDDY_ROW_STEP;
    bool off = i == 1 && !buddyTradeOk();  // Trade: nothing to trade on one side
    gfx->fillRoundRect(CX - 130, y, 260, BUDDY_ROW_H, 14, off ? UI_TRACK : BUDDY_ROW_COLOR[i]);
    gfx->setTextColor(off ? 0x8410 : UI_WHITE);
    setCur(centerX(T(BUDDY_ROW_LABEL[i]), 2), y + 17);
    printT(T(BUDDY_ROW_LABEL[i]));
  }
  drawCloseButton();
  gfx->flush();
}

// both have something to trade: a Pokemon other than the one each is raising
bool buddyTradeOk() { return pet.tradeableCount() > 0 && buddyTradeable(bPeers[bCur]) > 0; }

void buddyMenuTap(int16_t x, int16_t y) {
  int row = (y - BUDDY_ROW_Y) / BUDDY_ROW_STEP;
  if (abs(x - CX) > 130 || y < BUDDY_ROW_Y || row >= BUDDY_ROWS) {  // the close button, or outside the rows
    if (closeHit(x, y)) sfxPlay(SFX_TAP);
    buddyMenuOpen = false;
    return;
  }
  if (row == 1 && !buddyTradeOk()) {  // greyed out: the menu stays
    sfxPlay(SFX_DENY);
    Serial.println("buddy: nothing to trade");
    return;
  }
  buddyMenuOpen = false;
  sfxPlay(SFX_TAP);
  if (row == 0) buddyInvite(BK_BATTLE);
  else if (row == 1) buddyInvite(BK_TRADE);
  else if (row == 2) playMenuOpen = true;
}

// ---------- "Play together": games only buddies can play ----------
#define PLAY_ROW_Y 186
#define PLAY_ROW_H 56
static const StrId PLAY_GAMES[] = { S_SHOWDOWN, S_VOLLEY };
static const uint8_t PLAY_KINDS[] = { BK_SHOWDOWN, BK_VOLLEY };
#define PLAY_GAME_COUNT (int)(sizeof(PLAY_GAMES) / sizeof(PLAY_GAMES[0]))

void renderPlayMenu() {
  if (!bPeers[bCur].on) { playMenuOpen = false; return; }
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_PLAY_TOGETHER), 3), 84);
  printT(T(S_PLAY_TOGETHER));
  setSize(2);
  gfx->setTextColor(UI_INK);
  setCur(centerX(buddyName(), 2), 126);
  printT(buddyName());
  for (int i = 0; i < PLAY_GAME_COUNT; i++) {
    int y = PLAY_ROW_Y + i * (PLAY_ROW_H + 10);
    gfx->fillRoundRect(CX - 150, y, 300, PLAY_ROW_H, 14, TYPE_ACCENT[TY_PSYCHIC]);
    gfx->setTextColor(UI_WHITE);
    setCur(centerX(T(PLAY_GAMES[i]), 2), y + 20);
    printT(T(PLAY_GAMES[i]));
  }
  drawCloseButton();
  gfx->flush();
}

void playMenuTap(int16_t x, int16_t y) {
  for (int i = 0; i < PLAY_GAME_COUNT; i++) {
    int gy = PLAY_ROW_Y + i * (PLAY_ROW_H + 10);
    if (abs(x - CX) <= 150 && y >= gy && y < gy + PLAY_ROW_H) {
      sfxPlay(SFX_TAP);
      playMenuOpen = false;
      buddyInvite(PLAY_KINDS[i]);
      return;
    }
  }
  if (closeHit(x, y)) {  // back to the buddy menu
    sfxPlay(SFX_TAP);
    playMenuOpen = false;
    buddyMenuOpen = true;
  }
}

// ---------- Who's That? Showdown: the quiz against a buddy ----------
// Both boards draw the same rounds from the session (sdRoundOf). The host (the inviter) starts each
// round and referees it: each answer carries how long it took on its own screen, so the network
// delay doesn't decide who was first. Messages are resent until the next step shows they arrived.
#define SD_READY_MS 1800UL   // "get ready" before the first round
#define SD_ROUND_MS 10000UL  // nobody answers in time: nobody gets it
#define SD_GRACE_MS 700UL    // after a right answer, how long to wait for a faster one still on its way
#define SD_REVEAL_MS 2600UL
#define SD_DONE_MS 6000UL

uint8_t sdMine() { return sdHost ? sdHostScore : sdGuestScore; }
uint8_t sdTheirs() { return sdHost ? sdGuestScore : sdHostScore; }

void sdSend(BuddyType type, uint8_t move, uint16_t ms) {
  BuddyMsg m = buddyMsg(type);
  m.kind = BK_SHOWDOWN;
  m.turn = sdRound;
  m.move = move;
  m.hp = type == BM_ANSWER ? ms : sdHostScore;  // the host's score travels with every round and result
  m.atk = sdGuestScore;
  buddyNetSend(m);
}

void sdStart(bool host) {
  sdOpen = true;
  sdHost = host;
  sdRewarded = false;
  sdRound = 0;
  sdHostScore = sdGuestScore = 0;
  sdPhase = SD_READY;
  sdT = millis();
  sdSendT = 0;
  Serial.printf("buddy: showdown vs #%d (%s)\n", bPeers[bCur].dex, host ? "host" : "guest");
}

void sdBeginRound(uint8_t r) {
  sdRound = r;
  sdRoundOf(bsId, r, &sdAnswer, sdOpts);
  quizPmd.load(sdAnswer, false);
  sdPhase = SD_ASK;
  sdMyPick = sdTheirPick = -1;
  sdTheirIn = false;
  sdFirstRightT = sdSendT = 0;
  sdAskT = millis();  // after loading the sprite: the clock starts when the silhouette shows
  sdT = sdAskT;
}

void sdFinishRound(uint8_t who) {
  sdWho = who;
  if (who == SD_HOST) sdHostScore++;
  else if (who == SD_GUEST) sdGuestScore++;
  sdPhase = SD_REVEAL;
  sdT = millis();
  sdSendT = 0;
  bool mine = (who == SD_HOST) == sdHost && who != SD_NOBODY;
  sfxPlay(mine ? SFX_HEART : who == SD_NOBODY ? SFX_TAP : SFX_DENY);
  int at = sdOpts[0] == sdAnswer ? 0 : sdOpts[1] == sdAnswer ? 1 : 2;
  Serial.printf("buddy: showdown round %u: %s (%u-%u) answer@%d mine=%d/%ums theirs=%d/%ums\n", sdRound + 1,
                mine ? "mine" : who == SD_NOBODY ? "nobody" : "theirs", sdMine(), sdTheirs(), at, sdMyPick, sdMyMs,
                sdTheirIn ? sdTheirPick : -9, sdTheirMs);
}

void sdReward() {
  if (sdRewarded) return;
  sdRewarded = true;
  pet.showdownResult(sdMine(), sdTheirs());
  sfxPlay(sdMine() > sdTheirs() ? SFX_MEDAL : SFX_LEVEL);
  Serial.printf("buddy: showdown over %u-%u\n", sdMine(), sdTheirs());
}

void sdClose() {
  sdOpen = false;
  quizPmd.unload();
  buddyEndSession();
}

void sdQuit() {  // swiped away: the buddy is told
  buddyNetSend(buddyMsg(BM_LEAVE));
  sdClose();
}

void sdHandle(const BuddyMsg &m) {
  if (!sdOpen) return;
  if (!sdHost && m.type == BM_ROUND) {
    if ((sdPhase == SD_READY && m.turn == 0) || m.turn > sdRound || (sdPhase == SD_READY && m.turn > 0)) {
      sdHostScore = m.hp;  // the host's count wins, even if a result went missing
      sdGuestScore = m.atk;
      sdBeginRound(m.turn);
    }
  } else if (!sdHost && m.type == BM_RESULT) {
    if (m.turn == sdRound && sdPhase == SD_ASK) {
      sdFinishRound(m.move);
      sdHostScore = m.hp;
      sdGuestScore = m.atk;
    }
  } else if (sdHost && m.type == BM_ANSWER) {
    if (m.turn == sdRound && sdPhase == SD_ASK && !sdTheirIn) {
      sdTheirIn = true;
      sdTheirPick = m.move < 3 ? m.move : -1;
      sdTheirMs = m.hp;
    }
  }
}

void sdTick(uint32_t now) {
  if (!sdOpen) return;
  uint32_t t = now - sdT;
  bool resend = now - sdSendT >= BUDDY_RESEND_MS;
  switch (sdPhase) {
    case SD_READY:
      if (sdHost && t > SD_READY_MS) sdBeginRound(0);
      break;
    case SD_ASK:
      if (!sdHost) {  // the guest's answer goes out until the result comes back
        if (sdMyPick >= 0 && resend) {
          sdSendT = now;
          sdSend(BM_ANSWER, sdMyPick, sdMyMs);
        }
        break;
      }
      if (resend) {
        sdSendT = now;
        sdSend(BM_ROUND, 0, 0);
      }
      {
        bool myRight = sdMyPick >= 0 && sdOpts[sdMyPick] == sdAnswer;
        bool theirRight = sdTheirIn && sdTheirPick >= 0 && sdOpts[sdTheirPick] == sdAnswer;
        bool decide = (sdMyPick >= 0 && sdTheirIn) || now - sdAskT > SD_ROUND_MS;
        if (!decide && (myRight || theirRight)) {
          if (!sdFirstRightT) sdFirstRightT = now;
          decide = now - sdFirstRightT > SD_GRACE_MS;
        }
        if (decide) sdFinishRound(sdWinner(myRight, sdMyMs, theirRight, sdTheirMs));
      }
      break;
    case SD_REVEAL:
      if (sdHost && resend) {
        sdSendT = now;
        sdSend(BM_RESULT, sdWho, 0);
      }
      if (t > SD_REVEAL_MS) {
        if (sdRound + 1 >= SD_ROUNDS) {
          sdPhase = SD_DONE;
          sdT = now;
          sdReward();
        } else if (sdHost) {
          sdBeginRound(sdRound + 1);
        }
      }
      break;
    case SD_DONE:
      if (sdHost && t < 2500 && resend) {  // the last result, in case it went missing
        sdSendT = now;
        sdSend(BM_RESULT, sdWho, 0);
      }
      if (t > SD_DONE_MS) sdClose();
      break;
  }
}

#define SD_OPT_Y 268
#define SD_OPT_H 44
#define SD_OPT_GAP 6

void sdTap(int16_t x, int16_t y) {
  if (sdPhase == SD_DONE) {
    if (millis() - sdT > 1200) sdClose();
    return;
  }
  if (sdPhase != SD_ASK || sdMyPick >= 0 || y < SD_OPT_Y) return;
  int i = (y - SD_OPT_Y) / (SD_OPT_H + SD_OPT_GAP);
  if (i > 2) return;
  uint32_t ms = millis() - sdAskT;
  sdMyPick = i;
  sdMyMs = ms > 65535 ? 65535 : ms;
  sdSendT = 0;  // the guest's answer goes out right away
  if (sdOpts[i] != sdAnswer) sfxPlay(SFX_DENY);  // a wrong tap: out for this round
}

void renderShowdown() {
  if (!sdOpen) return;
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, TYPE_ACCENT[TY_PSYCHIC]);
  gfx->setTextColor(UI_WHITE);
  char b[48];
  // the score: you on the left, the buddy on the right
  setSize(2);
  snprintf(b, sizeof(b), "%s %u : %u %s", T(S_SD_YOU_LBL), sdMine(), sdTheirs(), buddyName());
  setCur(centerX(b, 2), 44);
  printT(b);
  if (sdPhase == SD_DONE) {
    snprintf(b, sizeof(b), "%u : %u", sdMine(), sdTheirs());
    setSize(6);
    setCur(centerX(b, 6), 150);
    printT(b);
    setSize(2);
    if (sdMine() > sdTheirs()) strlcpy(b, T(S_SD_WIN), sizeof(b));
    else if (sdMine() < sdTheirs()) snprintf(b, sizeof(b), T(S_SD_LOSE_FMT), buddyName());
    else strlcpy(b, T(S_SD_DRAW), sizeof(b));
    setCur(centerX(b, 2), 250);
    printT(b);
    gfx->flush();
    return;
  }
  if (sdPhase == SD_READY) {
    setCur(centerX(T(S_SD_READY), 2), 220);
    printT(T(S_SD_READY));
    gfx->flush();
    return;
  }
  bool reveal = sdPhase == SD_REVEAL;
  if (reveal) {
    bool mine = sdWho != SD_NOBODY && (sdWho == SD_HOST) == sdHost;
    if (mine) strlcpy(b, T(S_SD_YOU_GOT), sizeof(b));
    else if (sdWho == SD_NOBODY) strlcpy(b, T(S_SD_NOBODY), sizeof(b));
    else snprintf(b, sizeof(b), T(S_SD_THEY_GOT_FMT), buddyName());
  } else {
    snprintf(b, sizeof(b), "%u/%u", sdRound + 1, SD_ROUNDS);
  }
  setCur(centerX(b, 2), 72);
  printT(b);
  gPmdForceScale = pmdFitScale(quizPmd, PMD_IDLE, 300, 166);
  drawPmdActM(quizPmd, PMD_IDLE, CX, 262, reveal ? millis() : 0, true, !reveal, 8);
  gPmdForceScale = 0;
  bool locked = sdMyPick >= 0 && !reveal;
  for (int i = 0; i < 3; i++) {
    int y = SD_OPT_Y + i * (SD_OPT_H + SD_OPT_GAP);
    int inset = i == 2 ? 26 : 0;  // the bottom one, narrower: the circle closes in
    uint16_t bg = UI_WHITE;
    if (reveal && sdOpts[i] == sdAnswer) bg = UI_BAR_OK;
    else if (i == sdMyPick && sdOpts[i] != sdAnswer) bg = UI_BAR_BAD;
    else if (locked) bg = UI_TRACK;  // answered: out until the round ends
    gfx->fillRoundRect(83 + inset, y, 300 - 2 * inset, SD_OPT_H, 10, bg);
    gfx->setTextColor(bg == UI_WHITE || bg == UI_TRACK ? UI_INK : UI_WHITE);
    setCur(centerX(dexName(sdOpts[i]), 2), y + 15);
    printT(dexName(sdOpts[i]));
  }
  gfx->flush();
}

// ---------- Poke Ball Volley: the ball goes from one screen to the other ----------
// The ball lives on one board at a time and is simulated there (volley.h). Over the net it is
// handed to the buddy (BM_VBALL); a miss is a point for them (BM_VMISS), and the one who missed
// serves next. Both are resent until acknowledged, and any newer message counts as one too.
#define VB_READY_MS 1800UL
#define VB_SERVE_MS 1300UL  // the ball sits on your Pokemon for a moment, then off it goes
#define VB_POINT_MS 1800UL
#define VB_DONE_MS 6000UL

void vbSend(BuddyType type, bool ack, uint8_t seq) {
  BuddyMsg m = buddyMsg(type);
  m.kind = BK_VOLLEY;
  m.ack = ack;
  m.turn = seq;
  if (type == BM_VBALL) {
    m.hp = (uint16_t)vbOut.x;
    m.dex = (int16_t)vbOut.vx;
    m.spe = (uint16_t)fabsf(vbOut.vy);
    float c = vbOut.ax < -2000 ? -2000 : vbOut.ax > 2000 ? 2000 : vbOut.ax;
    m.atk = (uint16_t)(c + 2000);  // the curve, offset to fit unsigned
  } else {
    m.atk = vbMine;  // the score as the sender sees it
    m.def = vbTheirs;
  }
  buddyNetSend(m);
}

void vbStart(bool host) {
  vbOpen = true;
  vbHost = host;
  vbRewarded = false;
  vbMine = vbTheirs = 0;
  vbSpeed0 = VB_SPEED0;
  vbSeqOut = 0;
  vbSeqIn = 0xFF;
  vbOutPending = vbMissPending = false;
  vbPadX = CX;
  vbPhase = VB_READY;
  vbT = millis();
  tiltZero();
  Serial.printf("buddy: volley vs #%d (%s)\n", bPeers[bCur].dex, host ? "host" : "guest");
}

void vbClose() {
  vbOpen = false;
  buddyEndSession();
}

void vbQuit() {
  buddyNetSend(buddyMsg(BM_LEAVE));
  vbClose();
}

void vbTap() {
  if (vbPhase == VB_DONE && millis() - vbT > 1200) vbClose();
}

bool vbOver() { return vbMine >= VB_WIN || vbTheirs >= VB_WIN; }

void vbReward() {
  if (vbRewarded) return;
  vbRewarded = true;
  pet.showdownResult(vbMine, vbTheirs);  // the same reward as any game with a buddy
  sfxPlay(vbMine > vbTheirs ? SFX_MEDAL : SFX_LEVEL);
  Serial.printf("buddy: volley over %u-%u\n", vbMine, vbTheirs);
}

// a point: who got it, then the next serve (the one who missed) or the end
void vbPoint(bool mine) {
  vbMineLast = mine;
  vbPhase = VB_POINTED;
  vbT = millis();
  sfxPlay(mine ? SFX_HEART : SFX_DENY);
  Serial.printf("buddy: volley point %s (%u-%u)\n", mine ? "mine" : "theirs", vbMine, vbTheirs);
}

void vbHandle(const BuddyMsg &m) {
  if (!vbOpen) return;
  if (m.ack) {  // the buddy got it
    if (m.turn == vbSeqOut) vbOutPending = vbMissPending = false;
    return;
  }
  bool fresh = m.turn != vbSeqIn;
  vbSend((BuddyType)m.type, true, m.turn);  // acknowledged, even a repeat (our ack may have been lost)
  if (!fresh) return;
  vbSeqIn = m.turn;
  vbOutPending = vbMissPending = false;  // anything new from them means ours arrived
  if (m.type == BM_VBALL) {
    vbBall = vbArrive(m.hp, m.dex, m.spe, (float)m.atk - 2000);
    vbPhase = VB_PLAY;
    vbStepT = millis();
  } else {  // they missed: a point for us, and they serve
    vbMine = m.def;
    vbTheirs = m.atk;
    vbPoint(true);
  }
}

void vbTick(uint32_t now) {
  if (!vbOpen) return;
  // the paddle: your finger where it touches, or the tilt (arrow keys on a computer)
  float dt = (now - vbStepT) / 1000.0f;
  if (dt > 0.1f) dt = 0.1f;
  if (vbBot) {  // a player who moves at most this fast: it misses once the ball is quick
    static float to = CX;  // aims like a person: a fresh look every 0.3 s, a tilting hand's speed
    static uint32_t lookT = 0;
    if (now - lookT > 300) {
      lookT = now;
      to = vbPhase == VB_PLAY && vbBall.vy > 0 ? vbBall.x : CX;
    }
    float step = 250.0f * dt;
    vbPadX += to > vbPadX + step ? step : to < vbPadX - step ? -step : to - vbPadX;
  } else if (wasPressed) {
    float k = dt * 14 > 1 ? 1 : dt * 14;
    vbPadX += (tXl - vbPadX) * k;
  } else {
    vbPadX += playerTilt() * 430.0f * dt;
  }
  if (vbPadX < VB_LEFT) vbPadX = VB_LEFT;
  if (vbPadX > VB_RIGHT) vbPadX = VB_RIGHT;

  if ((vbOutPending || vbMissPending) && now - vbSendT >= 300) {
    vbSendT = now;
    vbSend(vbOutPending ? BM_VBALL : BM_VMISS, false, vbSeqOut);
  }
  uint32_t t = now - vbT;
  switch (vbPhase) {
    case VB_READY:
      if (t > VB_READY_MS) {
        vbPhase = vbHost ? VB_SERVE : VB_AWAY;  // the inviter serves first
        vbT = now;
      }
      break;
    case VB_SERVE:
      if (t > VB_SERVE_MS) {
        vbBall = vbServe(vbPadX, vbSpeed0, playerTilt());
        vbPhase = VB_PLAY;
        sfxPlay(SFX_PLAY);
      }
      break;
    case VB_PLAY: {
      // small steps, so a fast ball can't jump past the paddle
      for (float left = dt; left > 0 && vbPhase == VB_PLAY; left -= 0.008f) {
        VbEvent ev = vbStep(vbBall, left < 0.008f ? left : 0.008f, vbPadX);
        if (ev == VB_HIT) {
          sfxPlay(SFX_PLAY);
        } else if (ev == VB_OVER_NET) {
          vbOut = vbBall;
          vbSeqOut++;
          vbOutPending = true;
          vbMissPending = false;
          vbSendT = 0;
          vbPhase = VB_AWAY;
        } else if (ev == VB_MISSED) {
          vbTheirs++;
          vbSpeed0 = vbNextServeSpeed(vbSpeed(vbBall));
          vbSeqOut++;
          vbMissPending = true;
          vbOutPending = false;
          vbSendT = 0;
          vbPoint(false);
        }
      }
      break;
    }
    case VB_POINTED:
      if (t > VB_POINT_MS) {
        if (vbOver()) {
          vbPhase = VB_DONE;
          vbT = now;
          vbReward();
        } else {
          vbPhase = vbMineLast ? VB_AWAY : VB_SERVE;  // the one who missed serves
          vbT = now;
        }
      }
      break;
    case VB_AWAY:
      break;
    case VB_DONE:
      if (t > VB_DONE_MS) vbClose();
      break;
  }
  vbStepT = now;
}

void renderVolley() {
  if (!vbOpen) return;
  drawGameScene();
  char b[48];
  // the net: past it, the buddy's side
  for (int x = VB_LEFT - 20; x < VB_RIGHT + 20; x += 18) gfx->fillRect(x, VB_TOP - 2, 10, 4, UI_WHITE);
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  snprintf(b, sizeof(b), "%s %u : %u %s", T(S_SD_YOU_LBL), vbMine, vbTheirs, buddyName());
  setCur(centerX(b, 2), 36);
  printT(b);
  // your Pokemon is the paddle
  gPmdForceScale = 2;
  drawPmdAct(PMD_IDLE, (int)vbPadX, VB_PADDLE_Y + 40, millis(), true, false, 2);
  gPmdForceScale = 0;
  if (vbPhase == VB_PLAY) drawMap(SPR_ICON_PLAY, 16, (int)vbBall.x - VB_BALL_R, (int)vbBall.y - VB_BALL_R, 4, false);
  if (vbPhase == VB_SERVE) drawMap(SPR_ICON_PLAY, 16, (int)vbPadX - 16, VB_PADDLE_Y - 34, 2, false);
  const char *line = nullptr;
  if (vbPhase == VB_READY) line = T(S_SD_READY);
  else if (vbPhase == VB_SERVE) line = T(S_VB_SERVE);
  else if (vbPhase == VB_POINTED) {
    if (vbMineLast) line = T(S_VB_POINT);
    else {
      snprintf(b, sizeof(b), T(S_VB_THEY_SCORE_FMT), buddyName());
      line = b;
    }
  }
  if (line) {
    gfx->setTextColor(UI_WHITE);
    setCur(centerX(line, 2), 190);
    printT(line);
  }
  if (vbPhase == VB_READY || vbPhase == VB_SERVE) {
    gfx->setTextColor(UI_WHITE);
    setCur(centerX(T(S_VB_HINT), 2), 222);
    printT(T(S_VB_HINT));
  }
  if (vbPhase == VB_DONE) {
    gfx->fillRoundRect(CX - 150, 130, 300, 150, 18, UI_BG_DAY);
    gfx->setTextColor(UI_INK);
    snprintf(b, sizeof(b), "%u : %u", vbMine, vbTheirs);
    setSize(5);
    setCur(centerX(b, 5), 150);
    printT(b);
    setSize(2);
    if (vbMine > vbTheirs) strlcpy(b, T(S_SD_WIN), sizeof(b));
    else snprintf(b, sizeof(b), T(S_SD_LOSE_FMT), buddyName());
    setCur(centerX(b, 2), 228);
    printT(b);
  }
  gfx->flush();
}

void renderTrade() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  if (trPhase == TR_PICK) {
    // every form you have, a shiny one glowing; yours can't be traded. Green dot: the buddy
    // is missing that species
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(T(S_TRADE_PICK), 2), 50);
    printT(T(S_TRADE_PICK));
    for (int i = 0; i < 16; i++) {
      bool shiny;
      int16_t dex = pet.formAt(trPage * 16 + i, shiny);
      if (!dex) break;
      int x = GAL_X + (i % 4) * GAL_CELL, y = GAL_Y + (i / 4) * GAL_CELL;
      if (const uint8_t *t = thumbs.getForm(dex, shiny)) drawThumb(t, x, y, 2, !pet.canTrade(dex, shiny));
      if (shiny) drawShinyMark(x, y, GAL_CELL);
      if (!buddyRegHas(bPeers[bCur].reg, dex)) gfx->fillCircle(x + 66, y + 66, 7, UI_BAR_OK);
    }
    int pages = (pet.formCount() + 15) / 16;
    for (int i = 0; pages > 1 && i < pages; i++) {
      int dotX = CX + (i * 2 - (pages - 1)) * 7;
      if (i == trPage) gfx->fillCircle(dotX, GAL_DOTS_Y, 4, UI_INK);
      else gfx->drawCircle(dotX, GAL_DOTS_Y, 3, UI_INK);
    }
    gfx->flush();
    return;
  }
  gfx->setTextColor(UI_INK);
  setSize(2);
  if (trPhase == TR_DONE) {
    char b[44];
    snprintf(b, sizeof(b), T(S_TRADE_GOT_FMT), dexName(trTheirs));
    if (const uint8_t *t = thumbs.getForm(trTheirs, trTheirsShiny)) drawThumb(t, CX - GAL_CELL / 2 - 40, 120, 4, false);
    if (trTheirsShiny) drawShinyMark(CX - 84, 116, 168);
    catchBanner(b, C565(0x1e, 0x7d, 0x3c));
    gfx->flush();
    return;
  }
  // what you give on the left, what you receive on the right (not yet known: a question mark)
  setCur(136 - textW(T(S_TRADE_GIVE), 2) / 2, 92);
  printT(T(S_TRADE_GIVE));
  setCur(330 - textW(T(S_TRADE_GET), 2) / 2, 92);
  printT(T(S_TRADE_GET));
  if (const uint8_t *t = thumbs.getForm(trMine, trMineShiny)) drawThumb(t, 136 - GAL_CELL / 2, 124, 3, false);
  if (trMineShiny) drawShinyMark(136 - 62, 120, 124);
  setCur(136 - textW(dexName(trMine), 2) / 2, 216);
  printT(dexName(trMine));
  if (trTheirs) {
    if (const uint8_t *t = thumbs.getForm(trTheirs, trTheirsShiny)) drawThumb(t, 330 - GAL_CELL / 2, 124, 3, false);
    if (trTheirsShiny) drawShinyMark(330 - 62, 120, 124);
    setCur(330 - textW(dexName(trTheirs), 2) / 2, 216);
    printT(dexName(trTheirs));
  } else {
    setSize(5);
    setCur(330 - textW("?", 5) / 2, 146);
    printT("?");
    setSize(2);
  }
  if (trPhase == TR_REVIEW) {
    gfx->fillRoundRect(CX - 110, PICK_BTN_Y - 40, 220, PICK_BTN_H, 14, C565(0x1e, 0x7d, 0x3c));
    gfx->setTextColor(UI_WHITE);
    setCur(centerX(T(S_BUDDY_TRADE), 2), PICK_BTN_Y - 21);
    printT(T(S_BUDDY_TRADE));
    drawCloseButton();
  } else {
    setCur(centerX(T(S_BUDDY_WAITING), 2), PICK_BTN_Y);
    printT(T(S_BUDDY_WAITING));
  }
  gfx->flush();
}

void tradeTap(int16_t x, int16_t y) {
  if (trPhase == TR_PICK) {
    if (y < 66 || closeHitAt(x, y, GAL_CLOSE_Y)) {  // the close button (or the header) = exit, as in the Pokedex
      sfxPlay(SFX_TAP);
      tradeClose(true);
      return;
    }
    if (x < GAL_X || y < GAL_Y) return;
    int c = (x - GAL_X) / GAL_CELL, r = (y - GAL_Y) / GAL_CELL;
    if (c > 3 || r > 3) return;
    bool shiny;
    int16_t dex = pet.formAt(trPage * 16 + r * 4 + c, shiny);
    if (!dex) return;
    if (!pet.canTrade(dex, shiny)) {  // the one you are raising stays
      sfxPlay(SFX_DENY);
      return;
    }
    sfxPlay(SFX_TAP);
    trMine = dex;
    trMineShiny = shiny;
    trT = millis();
    bsResendT = 0;
    trPhase = trTheirs ? TR_REVIEW : TR_WAIT;
    buddySendTrade(BM_OFFER, trMine, false);
  } else if (trPhase == TR_REVIEW) {
    if (y >= PICK_BTN_Y - 40 && y < PICK_BTN_Y - 40 + PICK_BTN_H && abs(x - CX) <= 110) {
      sfxPlay(SFX_TAP);
      trPhase = TR_WAITC;
      trT = millis();
      bsResendT = 0;
    } else if (closeHit(x, y)) {
      sfxPlay(SFX_TAP);
      tradeClose(true);
    }
  }
}

// ---------- Battle: the zones ----------
#define ZONE_ICON_R 50
#define ZONE_GRID_X 73
#define ZONE_GRID_Y 76
#define ZONE_CELL 80
#define ZONE_PAGE 12
#define ZONE_BTN_X 123
#define ZONE_BTN_Y 322
#define ZONE_BTN_W 220
#define ZONE_BTN_H 56
#define ZONE_IDLE_MS 30000UL

static const int16_t ZONE_XY[ZONE_COUNT][2] = { { 155, 112 }, { 311, 112 }, { 93, 258 }, { 233, 258 }, { 373, 258 } };
static const StrId ZONE_NAME[ZONE_COUNT] = { S_ZONE_FOREST, S_ZONE_BEACH, S_ZONE_CAVE, S_ZONE_MANSION, S_ZONE_FIELD };
static const uint16_t ZONE_COLOR[ZONE_COUNT] = { C565(0x3a, 0x8a, 0x4f), C565(0x2f, 0x7f, 0xb5), C565(0x6b, 0x5a, 0x8a), C565(0x7a, 0x3f, 0x8f), C565(0xc9, 0x8a, 0x1e) };

void openZones() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony) { sfxPlay(SFX_DENY); return; }
  if (!zoneArt) {
    zoneArt = (uint16_t *)ps_malloc(ZONE_COUNT * ZONE_ART * ZONE_ART * sizeof(uint16_t));
    if (zoneArt)
      for (uint8_t z = 0; z < ZONE_COUNT; z++) zoneArtDraw(zoneArt + z * ZONE_ART * ZONE_ART, z);
  }
  zoneScreen = ZS_PICK;
  zoneUntil = millis() + ZONE_IDLE_MS;
}

// the entry opened from a zone's grid, like the Pokedex's (0 = the grid)
void zoneDetailClose() {
  if (!zoneDetail) return;
  zoneDetail = 0;
  galleryUnload();
}

void zoneBack() {
  sfxPlay(SFX_TAP);
  if (zoneDetail) {
    zoneDetailClose();
    zoneUntil = millis() + ZONE_IDLE_MS;
  } else if (zoneScreen == ZS_ZONE) {
    zoneScreen = ZS_PICK;
    zoneUntil = millis() + ZONE_IDLE_MS;
  } else {
    zoneScreen = ZS_NONE;
    openGamesMenu();
  }
}

void zoneTurnPage(int dir) {
  zoneUntil = millis() + ZONE_IDLE_MS;
  if (zoneScreen != ZS_ZONE || zoneDetail) return;
  int pages = (zoneSize(zoneCur) + ZONE_PAGE - 1) / ZONE_PAGE;
  int np = (int)zonePage + dir;
  if (np >= 0 && np < pages) zonePage = np;
}

// the battle background is scaled up only once: copying it every frame is cheap
void zoneBgLoad(uint8_t z) {
  if (!zoneArt) return;
  zoneBg = (uint16_t *)ps_malloc(466 * BTL_BAND_Y * sizeof(uint16_t));
  if (!zoneBg) return;
  const uint16_t *art = zoneArt + z * ZONE_ART * ZONE_ART;
  for (int y = 0; y < BTL_BAND_Y; y++) {
    const uint16_t *row = art + (y * ZONE_ART / BTL_BAND_Y) * ZONE_ART;
    uint16_t *out = zoneBg + y * 466;
    for (int x = 0; x < 466; x++) out[x] = row[x * ZONE_ART / 466];
  }
}

void zoneSearch() {
  sfxPlay(SFX_TAP);
  zoneScreen = ZS_NONE;
  bZone = zoneCur;
  zoneBgLoad(zoneCur);
  startBattleWith(0);
  if (!battleOpen) zoneBattleEnded();  // without a sprite there is no battle: back to the zone
}

void zoneBattleEnded() {
  if (zoneBg) { free(zoneBg); zoneBg = nullptr; }
  zoneCur = bZone;
  bZone = -1;
  zoneScreen = ZS_ZONE;  // keep searching, now with the catch on the list
  zoneUntil = millis() + ZONE_IDLE_MS;
}

void zoneTap(int16_t x, int16_t y) {
  zoneUntil = millis() + ZONE_IDLE_MS;
  if (zoneDetail) {  // tap anywhere: back to the zone
    zoneDetailClose();
    return;
  }
  if (closeHit(x, y)) {  // the picker closes; a zone goes back to the picker
    zoneBack();
    return;
  }
  if (zoneScreen == ZS_PICK) {
    for (uint8_t z = 0; z < ZONE_COUNT; z++) {
      int dx = x - ZONE_XY[z][0], dy = y - ZONE_XY[z][1];
      if (dx * dx + dy * dy < (ZONE_ICON_R + 6) * (ZONE_ICON_R + 6) || (abs(dx) < 66 && dy > 0 && dy < ZONE_ICON_R + 44)) {
        sfxPlay(SFX_TAP);
        zoneCur = z;
        zonePage = 0;
        zoneScreen = ZS_ZONE;
        return;
      }
    }
    return;
  }
  if (y >= ZONE_BTN_Y && y < ZONE_BTN_Y + ZONE_BTN_H && x >= ZONE_BTN_X && x < ZONE_BTN_X + ZONE_BTN_W) {
    zoneSearch();
    return;
  }
  if (x < ZONE_GRID_X || y < ZONE_GRID_Y) return;
  int c = (x - ZONE_GRID_X) / ZONE_CELL, r = (y - ZONE_GRID_Y) / ZONE_CELL;
  if (c > 3 || r > 2) return;
  int16_t dex = zoneMember(zoneCur, zonePage * ZONE_PAGE + r * 4 + c);
  if (!dex) return;
  sfxPlay(SFX_TAP);
  zoneDetail = dex;
  dexDetailLoad(dex);
}

void zoneDrawIcon(uint8_t z) {
  int cx = ZONE_XY[z][0], cy = ZONE_XY[z][1];
  if (zoneArt) {
    const uint16_t *art = zoneArt + z * ZONE_ART * ZONE_ART;
    for (int dy = -ZONE_ICON_R; dy < ZONE_ICON_R; dy++) {
      int half = (int)sqrtf(ZONE_ICON_R * ZONE_ICON_R - (dy + 0.5f) * (dy + 0.5f));
      gfx->draw16bitRGBBitmap(cx - half, cy + dy, (uint16_t *)art + (dy + ZONE_ICON_R) * ZONE_ART + ZONE_ICON_R - half, half * 2, 1);
    }
  } else {
    gfx->fillCircle(cx, cy, ZONE_ICON_R, ZONE_COLOR[z]);
  }
  for (int r = 0; r < 3; r++) gfx->drawCircle(cx, cy, ZONE_ICON_R + r - 1, UI_INK);
}

void renderZones() {
  if (!timeLeft(zoneUntil) || pet.sleeping || pet.ceremony || pet.isEgg()) { zoneDetailClose(); zoneScreen = ZS_NONE; return; }
  if (zoneDetail) {
    renderDexDetail(zoneDetail);
    return;
  }
  char b[32];
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  if (zoneScreen == ZS_PICK) {
    setCur(centerX(T(S_GAME_BATTLE), 3), 26);
    printT(T(S_GAME_BATTLE));
    setSize(2);
    for (uint8_t z = 0; z < ZONE_COUNT; z++) {
      zoneDrawIcon(z);
      int cx = ZONE_XY[z][0], y = ZONE_XY[z][1] + ZONE_ICON_R + 6;
      int got = pet.zoneCaught(z), all = zoneSize(z);
      gfx->setTextColor(UI_INK);
      setCur(cx - textW(T(ZONE_NAME[z]), 2) / 2, y);
      printT(T(ZONE_NAME[z]));
      snprintf(b, sizeof(b), "%d/%d", got, all);
      gfx->setTextColor(got == all ? UI_BAR_WARN : got ? ZONE_COLOR[z] : C565(0x6b, 0x6a, 0x60));
      setCur(cx - textW(b, 2) / 2, y + 18);
      printT(b);
    }
    drawCloseButton();
    gfx->flush();
    return;
  }

  setCur(centerX(T(ZONE_NAME[zoneCur]), 3), 22);
  printT(T(ZONE_NAME[zoneCur]));
  setSize(2);
  const char *line = b;
  snprintf(b, sizeof(b), T(S_ZONE_CAUGHT_FMT), pet.zoneCaught(zoneCur), zoneSize(zoneCur));
  setCur(centerX(line, 2), 54);
  printT(line);
  for (int i = 0; i < ZONE_PAGE; i++) {
    int16_t dex = zoneMember(zoneCur, zonePage * ZONE_PAGE + i);
    if (!dex) break;
    int x = ZONE_GRID_X + (i % 4) * ZONE_CELL, y = ZONE_GRID_Y + (i / 4) * ZONE_CELL;
    if (const uint8_t *t = thumbs.getForm(dex, !pet.hasForm(dex, false) && pet.hasForm(dex, true)))
      drawThumb(t, x, y, 2, !pet.isRegistered(dex));
    if (pet.isShinyRegistered(dex)) drawShinyMark(x, y, ZONE_CELL);
    if (dex == ZONE_LEGEND[zoneCur]) {  // star: the legendary waits here
      gfx->setTextColor(UI_BAR_WARN);
      setCur(x + 62, y + 4);
      printT("*");
    }
  }
  gfx->fillRoundRect(ZONE_BTN_X, ZONE_BTN_Y, ZONE_BTN_W, ZONE_BTN_H, 14, ZONE_COLOR[zoneCur]);
  gfx->setTextColor(UI_WHITE);
  setCur(centerX(T(S_ZONE_SEARCH), 2), ZONE_BTN_Y + 21);
  printT(T(S_ZONE_SEARCH));
  int pages = (zoneSize(zoneCur) + ZONE_PAGE - 1) / ZONE_PAGE;
  for (int i = 0; i < pages; i++) {
    int dotX = CX + (i * 2 - (pages - 1)) * 7;
    if (i == zonePage) gfx->fillCircle(dotX, 390, 4, UI_INK);
    else gfx->drawCircle(dotX, 390, 3, UI_INK);
  }
  drawCloseButton();  // how to leave, as on the zone picker
  gfx->flush();
}

void drawBattery() {
  int pc = batPercent();
  if (pc < 0) return;  // no battery connected
  int x = CX - 14, y = 12, w = 24, h = 11;
  bool charging = batCharging();
  uint16_t col = charging ? UI_BAR_OK
                 : (pc >= 40) ? inkColor()
                 : (pc >= 15) ? UI_BAR_WARN
                              : UI_BAR_BAD;
  gfx->drawRoundRect(x, y, w, h, 2, col);
  gfx->fillRect(x + w, y + 3, 3, 5, col);  // terminal
  if (charging) {
    // charging bolt (zigzag) instead of the level bar
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

// ceremony animation (10s): farewell = bow with hearts and it
// walks away; runaway = it gets scared and runs off. Replaces the idle.
void drawCeremony() {
  if (!pmd.loaded) { drawPet(); return; }  // fallback if there is no PMD sprite
  uint32_t now = millis();
  float t = pet.ceremonyT();               // 0..1 over the 10s
  bool panic = (pet.ceremony == CER_RUNAWAY);
  int x = CX, y = PET_GROUND;
  uint8_t act = PMD_IDLE;

  if (panic) {
    // sad ending: bluish dimness + rain
    for (int i = 0; i < 46; i++) {
      int rx = (i * 47 + now / 3) % 466;
      int ry = (i * 91 + now / 2) % 470;
      gfx->drawLine(rx, ry, rx - 3, ry + 12, C565(0x6a, 0x84, 0xb0));
    }
    bool fade = false;
    if (t < 0.30f) {                       // head down, trembling
      act = pmd.has(PMD_HURT) ? PMD_HURT : PMD_IDLE;
      x = CX + (int)(4 * sinf(now * 0.04f));
    } else {                               // walks away slowly and fades
      act = pmd.has(PMD_WALKL) ? PMD_WALKL : PMD_IDLE;
      x = CX - (int)(((t - 0.30f) / 0.70f) * (CX + 120));
      fade = (t > 0.6f) && ((now / 160) % 2 == 0);  // blinks toward the silhouette
    }
    drawPmdAct(act, x, y, now, true, fade, 5);  // fade=silhouette: it blurs as it leaves
    // tear falling from the pet
    if (t < 0.55f) {
      int ty = y - 150 + (int)((now / 6) % 40);
      gfx->fillRect(x + 6, ty, 3, 6, C565(0x9a, 0xc4, 0xe8));
    }
    return;
  }

  // epic farewell: pulsing golden halo + sparks and hearts that rise
  int gcy = PET_GROUND - 96;
  for (int k = 0; k < 4; k++) {
    int r = 60 + k * 34 + (int)(10 * sinf(now * 0.02f));
    gfx->drawCircle(CX, gcy, r, C565(0xff, 0xdf, 0x8a));
  }
  for (int i = 0; i < 16; i++) {
    int px = (i * 71 + 28) % 466;
    int py = 410 - (int)((now / 8 + i * 70) % 360);   // they rise and reappear at the bottom
    if (py < 30) continue;
    if (i % 4 == 0) drawMap(SPR_HEART, 32, px - 8, py - 8, 1, false);  // little heart
    else gfx->fillRect(px, py, 4, 4, (i % 2) ? C565(0xff, 0xe7, 0x9f) : C565(0xff, 0x9a, 0xc0));
  }

  if (t < 0.45f) {                         // bow / farewell pose
    act = pmd.has(PMD_POSE) ? PMD_POSE : (pmd.has(PMD_NOD) ? PMD_NOD : PMD_IDLE);
  } else {                                 // walks away to the right
    act = pmd.has(PMD_WALKR) ? PMD_WALKR : PMD_IDLE;
    x = CX + (int)(((t - 0.45f) / 0.55f) * (CX + 140));
  }
  drawPmdAct(act, x, y, now, true, false, 5);
  if (pet.showHeart())                     // large heart following the pet
    drawMap(SPR_HEART, 32, x + 50, y - 190, 2, false);
}

// decision dialog (2 stacked buttons): evolve/keep or say goodbye/stay together
void drawChoiceDialog() {
  const char *q, *o1, *o2;
  uint16_t c1, c2, t1, t2;
  if (choiceKind == 1) {  // evolution
    q = T(S_EVO_Q); o1 = T(S_EVO_TAP); o2 = T(S_EVO_KEEP);
    c1 = UI_BAR_BAD; t1 = UI_WHITE; c2 = UI_TRACK; t2 = UI_INK;
  } else if (choiceKind == 3) {  // delete the game
    q = T(S_RESET_Q); o1 = T(S_RESET_GO); o2 = T(S_RESET_KEEP);
    c1 = UI_BAR_BAD; t1 = UI_WHITE; c2 = UI_BAR_OK; t2 = UI_WHITE;
  } else if (choiceKind == 5) {  // the buddy invites
    static char invite[44];
    snprintf(invite, sizeof(invite), T(bsKind == BK_BATTLE ? S_BUDDY_INV_BTL_FMT : bsKind == BK_TRADE ? S_BUDDY_INV_TRD_FMT : S_BUDDY_INV_PLAY_FMT), buddyName());
    q = invite; o1 = T(S_YES); o2 = T(S_NO);
    c1 = UI_BAR_OK; t1 = UI_WHITE; c2 = UI_TRACK; t2 = UI_INK;
  } else if (choiceKind == 4) {  // runaway
    q = T(S_RUNAWAY_Q); o1 = T(S_RUNAWAY_CARE); o2 = T(S_RUNAWAY_GO);
    c1 = UI_BAR_OK; t1 = UI_WHITE; c2 = UI_TRACK; t2 = UI_INK;
  } else {                // farewell
    q = T(S_FAR_Q); o1 = T(S_FAR_GO); o2 = T(S_FAR_STAY);
    c1 = UI_BAR_WARN; t1 = UI_INK; c2 = UI_BAR_OK; t2 = UI_WHITE;
  }
  gfx->fillRoundRect(73, 156, 320, 188, 16, UI_WHITE);
  gfx->drawRoundRect(73, 156, 320, 188, 16, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(q, 2), 176);
  printT(q);
  gfx->fillRoundRect(93, 206, 280, 52, 12, c1);     // action button
  gfx->setTextColor(t1);
  setCur(centerX(o1, 2), 224);
  printT(o1);
  gfx->fillRoundRect(93, 268, 280, 52, 12, c2);     // keep/stay button
  gfx->setTextColor(t2);
  setCur(centerX(o2, 2), 286);
  printT(o2);
}

// big red CTA button for evolving (pulses to draw attention)
void drawEvolveButton() {
  uint32_t now = millis();
  int p = (int)(5 * sinf(now * 0.006f));  // pulses: -5..5
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

// gold farewell CTA button: "<name> wants to tell you something..."
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

// grim runaway CTA button from neglect: "<name> feels abandoned..."
// (sad ending: dark blue-gray, slow muted pulse)
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

// epic evolution animation: radial halo + spinning rays + accelerating blink of the
// sprite + sparks shooting out + final flash
void drawEvolveFX(uint32_t now) {
  float t = pet.evolveT();          // 0..1
  int cx = CX, cy = PET_GROUND - 96;

  // radial halo that grows and pulses
  int halo = 36 + (int)(t * 150) + (int)(8 * sinf(now * 0.02f));
  for (int k = 0; k < 4; k++) {
    int r = halo - k * 7;
    if (r > 0) gfx->drawCircle(cx, cy, r, UI_WHITE);
  }
  // spinning rays from the pet's center
  float base = now * 0.004f;
  for (int i = 0; i < 12; i++) {
    float a = base + i * (float)(PI / 6);
    int len = 90 + (int)(70 * (0.5f + 0.5f * sinf(now * 0.012f + i)));
    gfx->drawLine(cx, cy, cx + (int)(cosf(a) * len), cy + (int)(sinf(a) * len), UI_WHITE);
  }
  // blink between the PREVIOUS and the NEW form (silhouettes), accelerating; at the
  // end (t>0.9) it holds on the new one for the reveal flash
  int period = 60 + (int)(220 * (1.0f - t));
  bool showOld = t < 0.9f && evoPmd.loaded && ((now / period) % 2) == 0;
  if (showOld) drawPmdActM(evoPmd, PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  else drawPmdAct(PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  // sparks shooting out
  for (int i = 0; i < 10; i++) {
    float a = i * (float)(PI / 5) + t * 4.0f;
    int d = (int)((now / 14 + i * 33) % 200);
    int sx = cx + (int)(cosf(a) * d), sy = cy + (int)(sinf(a) * d);
    gfx->fillRect(sx - 2, sy - 2, 5, 5, (i & 1) ? C565(0xff, 0xe0, 0x70) : UI_WHITE);
  }
  // final flash before revealing the new form
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
    // no SD and no flash sprite: clear notice that sprites are missing
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

  // evolution animation: alternates the silhouette of the previous and the new form
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
  if (m == MOOD_HAPPY && (millis() / 500) % 2) y -= 6;  // little hop

  drawMap(sp.sprite, SPRITE_H, x, y, s, false);

  // overlaid expressions using the species' anchors
  bool blink = (millis() % 3500 < 300);
  if (m == MOOD_SLEEPING || blink) {
    overlayEye(sp, x, y, s, sp.eyeColL);
    overlayEye(sp, x, y, s, sp.eyeColR);
  }
  if (m == MOOD_EATING) overlayMouth(sp, x, y, s, true);
  else if (m == MOOD_SAD) overlayMouth(sp, x, y, s, false);

  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + 20 * s, y - 2 * s, 2, false);
}

// ---------- bath scene ----------

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
      // joy pose once clean
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
    // foam: bubbles swaying and rising little by little
    float t = now / 220.0f;
    for (auto &b : bubbles) {
      int bx = b.x + (int)(sinf(t + b.ph) * 6);
      int by = b.y - (int)((3000 - left) / 90);
      gfx->fillCircle(bx, by, b.r, UI_WHITE);
      gfx->drawCircle(bx, by, b.r, 0x7E3D);
      gfx->fillCircle(bx - b.r / 3, by - b.r / 3, b.r / 4, UI_BG_DAY);
    }
  } else {
    // the bubbles pop: sparkles
    for (int i = 0; i < 8; i++) {
      auto &b = bubbles[i];
      int sx = b.x + (i % 3) * 6 - 6, sy = b.y - 18;
      uint16_t col = (i % 2) ? UI_BAR_WARN : UI_WHITE;
      gfx->fillRect(sx - 6, sy - 1, 13, 3, col);
      gfx->fillRect(sx - 1, sy - 6, 3, 13, col);
    }
  }
}

// ---------- PMD pet: behavior ----------

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
  // guard: never more iterations than frames, in case a corrupt .bin has ms[]=0
  // despite the load-time sanitizing (defense in depth)
  for (uint8_t guard = 0; guard < a.frames && t >= a.ms[i]; guard++) {
    t -= a.ms[i];
    i = (i + 1) % a.frames;
  }
  return i;
}

// draws an action anchored by its base (center-x, ground) and returns its scale
// integer scale so the visible part of the sprite (without the canvas's transparent padding) fits
// in maxW x maxH: the canvas height includes a lot of empty space and falls well short
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

// a shiny's sparkles: four little gold stars around it (the box it fills on screen), each
// twinkling on its own beat
#define SPARKLE_GOLD C565(0xff, 0xcd, 0x28)
void drawSparkle(int x, int y, int r) {
  if (r < 2) return;
  gfx->fillRect(x - 1, y - r, 3, 2 * r + 1, SPARKLE_GOLD);
  gfx->fillRect(x - r, y - 1, 2 * r + 1, 3, SPARKLE_GOLD);
  gfx->fillRect(x - 1, y - 1, 3, 3, UI_WHITE);
}
void drawSparkles(int left, int top, int right, int bottom, uint32_t now) {
  int w = right - left, h = bottom - top;
  if (w <= 0 || h <= 0) return;
  int big = h < 60 ? 7 : 10;
  const int8_t spot[4][2] = { { -10, 10 }, { 104, -4 }, { 108, 70 }, { 0, 90 } };  // % of the box
  for (int i = 0; i < 4; i++) {
    uint32_t beat = (now + i * 370) % 1400;  // grows, holds, fades, rests
    int r = beat < 300 ? big * beat / 300 : beat < 600 ? big : beat < 900 ? big * (900 - beat) / 300 : 0;
    drawSparkle(left + w * spot[i][0] / 100, top + h * spot[i][1] / 100, r);
  }
}

// draws an action of a specific PmdMon (m); drawPmdAct uses the global pmd
void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS) {
  const PmdAct &a = m.acts[actId];
  if (!a.frames) return;
  uint8_t sBase = m.acts[PMD_IDLE].h ? 170 / m.acts[PMD_IDLE].h : 5;
  if (sBase < 2) sBase = 2;
  if (sBase > maxS) sBase = maxS;
  uint8_t s = sBase;
  while (s > 2 && a.h * s > 250) s--;  // actions with a large frame (attack)
  if (gPmdForceScale) s = gPmdForceScale;
  uint8_t fi = pmdFrameAt(a, t, loop);
  const uint8_t *fr = a.data + (uint32_t)fi * a.w * a.h;
  // anchor by the feet (a.base), not by the canvas height: that way actions
  // with different padding (Hurt, Eat...) all end up at the same ground height
  int x0 = cx - a.w * s / 2, y0 = groundY - (a.base ? a.base : a.h) * s;
  int left = a.w, right = -1, top = -1;  // what it fills, for the sparkles
  for (int r = 0; r < a.h; r++) {
    const uint8_t *row = fr + r * a.w;
    forRuns(a.w, [&](int c) { return indexedKey(row[c], sil); },
            [&](int c0, int len, int k) {
              gfx->fillRect(x0 + c0 * s, y0 + r * s, len * s, s, sil ? INK_K : m.pal[k]);
              if (c0 < left) left = c0;
              if (c0 + len > right) right = c0 + len;
              if (top < 0) top = r;
            });
  }
  if (m.shiny && !sil && right > 0) drawSparkles(x0 + left * s, y0 + top * s, x0 + right * s, groundY, millis());
}
void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS) {
  drawPmdActM(pmd, actId, cx, groundY, t, loop, sil, maxS);
}

// With others on screen (n), yours is always a size step bigger than them, whatever the species:
// sprites are capped by height, so a tall buddy of the same species would otherwise come out the
// same size as yours. Yours at least step 3; theirs one step below, down to the smallest (2).
uint8_t partyMineScale(int n) {
  uint8_t s = pmd.acts[PMD_IDLE].h ? 170 / pmd.acts[PMD_IDLE].h : 5;
  if (s < 3) s = 3;
  uint8_t cap = partyScale(n, true);
  return s > cap ? cap : s;
}
uint8_t partyOthersScale(int n) {
  uint8_t s = partyScale(n, false), mine = partyMineScale(n);
  return s >= mine ? (mine > 2 ? mine - 1 : 2) : s;
}

// picks the pet's next whim when it is happy
void behNext() {
  uint32_t now = millis();
  beh.t0 = now;
  int r = random(100);
  if (r < 35 && (pmd.has(PMD_WALKL) || pmd.has(PMD_WALKR))) {
    beh.mode = 1;  // walk
    beh.targetX = 150 + random(176);
    beh.until = now + 15000;
  } else if (r < 60) {
    // random gesture among the available ones
    // (Hop excluded: jumps too high; Sit excluded: looks backwards)
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
    beh.mode = 0;  // look straight ahead
    beh.until = now + 2000 + random(3000);
  }
}

void drawPetPMD() {
  uint32_t now = millis();

  if (pet.evolving()) {
    drawEvolveFX(now);
    return;
  }
  if (evoPmd.loaded) evoPmd.unload();  // evolution finished: free the previous form

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
    // happy: the planner decides (idle / walk / gesture); in a row of buddies it stays put
    if (now > beh.until) behNext();
    if (partyOthers() && beh.mode == 1) beh.mode = 0;
    if (beh.mode != 1) walkLast = 0;  // when walking again, the time stopped isn't counted
    if (beh.mode == 1) {
      float d = beh.targetX - beh.x;
      if (fabsf(d) < 4) {
        behNext();
        act = PMD_IDLE;
      } else {
        // 30 px/s by design (3 px per frame at 10 fps), measuring real time: with fewer frames
        // per second it walks just as fast
        uint32_t dt = walkLast ? now - walkLast : 100;
        uint32_t cap = frameIntervalMs(powerCtx(now)) + 100;  // a bigger gap (another screen in between) is not a long step
        if (dt > cap) dt = cap;
        walkLast = now;
        float step = 30.0f * dt / 1000.0f;
        if (step > fabsf(d)) step = fabsf(d);  // a slow frame doesn't overshoot the destination
        beh.x += d > 0 ? step : -step;
        act = (d > 0) ? PMD_WALKR : PMD_WALKL;
      }
    } else {
      act = (beh.mode == 2) ? beh.act : PMD_IDLE;
      loop = false;
    }
    if (!pmd.has(act)) act = PMD_IDLE;
  }

  int n = partyOthers();
  int px = n ? PARTY_MINE_X : (int)beh.x;
  if (n) gPmdForceScale = partyMineScale(n);
  drawPmdAct(act, px, PET_GROUND, now - beh.t0, loop || act == PMD_IDLE, false, partyScale(n, true));
  gPmdForceScale = 0;

  if (pet.showHeart()) drawMap(SPR_HEART, 32, px + (n > 2 ? 20 : 50), PET_GROUND - (n > 2 ? 140 : 190), n > 2 ? 1 : 2, false);
  drawVisitor(now);
}

// animated sprite from the SD: integer zoom per pixel, frames at their own pace
void drawPetSD() {
  int s = mon.scale;
  int w = mon.w * s, h = mon.h * s;
  int x = CX - w / 2;
  int y = PET_CY - h / 2;

  bool sil = false;
  if (pet.evolving()) {
    sil = (millis() / 300) % 2;
  } else if (pet.mood() == MOOD_HAPPY && (millis() / 500) % 2) {
    y -= 6;  // little hop
  }

  uint16_t fm = mon.frameMs ? mon.frameMs : 100;
  uint16_t fi = pet.sleeping ? 0 : (millis() / fm) % mon.frames;
  const uint8_t *fr = mon.data + (uint32_t)fi * mon.w * mon.h;
  for (int r = 0; r < mon.h; r++) {
    const uint8_t *row = fr + r * mon.w;
    forRuns(mon.w, [&](int c) { return indexedKey(row[c], sil); },
            [&](int c0, int len, int k) { gfx->fillRect(x + c0 * s, y + r * s, len * s, s, sil ? INK_K : mon.pal[k]); });
  }

  // emotes instead of expressions (imported sprites have no anchors)
  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + w - 30, y - 50, 2, false);
}

// closed eye: erases the 3x4 eye and draws the eyelid
void overlayEye(const Species &sp, int x, int y, int s, int col) {
  gfx->fillRect(x + col * s, y + sp.eyeRow * s, 3 * s, 4 * s, sp.bodyColor);
  gfx->fillRect(x + col * s, y + (sp.eyeRow + 2) * s, 3 * s, s, INK_K);
}

// erases the base smile and paints an open mouth (eating) or a frown (sad)
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

// Gap between the label and its bar in the needs row. It was fixed
// at 48 px, exactly what 4 Latin letters take at scale 2, with no margin. The
// Japanese labels are wider (ごきげん is 4 kana, ~64 px) and ran
// into the bar. It is derived from the longest label, with 48 as the floor
// so nothing changes in Latin languages.
// Requires text size 2 to be already set, because textW() needs it.
static int barLabelGap() {
  const StrId ids[] = { S_BAR_FOOD, S_BAR_JOY, S_BAR_ENE, S_BAR_HYG };
  int width = 0;
  for (StrId id : ids) {
    int w = textW(T(id), 2);
    if (w > width) width = w;
  }
  // below the original threshold it stays EXACTLY 48, so nothing moves in
  // Latin languages; above it, space is added so the label can breathe
  return width <= 48 ? 48 : width + 8;
}

void drawBar(int x, int y, const char *label, uint8_t val) {
  gfx->setTextColor(inkColor());
  setSize(2);
  setCur(x, y);
  printT(label);
  int gap = barLabelGap();
  // the two columns start at 78 and 244: the first one's bar has to
  // end before the second one's label, and both are drawn the same width
  int bw = 232 - (78 + gap);
  if (bw > 100) bw = 100;   // with Latin labels it comes out 100, as it was
  int bx = x + gap, bh = 15;
  uint16_t fill = (val >= 50) ? UI_BAR_OK : (val >= 25) ? UI_BAR_WARN : UI_BAR_BAD;
  gfx->fillRoundRect(bx, y, bw, bh, 4, UI_TRACK);
  int fw = (bw - 4) * val / 100;
  if (fw > 0) gfx->fillRoundRect(bx + 2, y + 2, fw, bh - 4, 3, fill);
}

// the icons on their own, no box; while sleeping only LIGHT works, the rest are shadows
void drawButtons() {
  for (int i = 0; i < 4; i++) {
    bool off = pet.sleeping && i != 2;
    drawMap(buttons[i].icon, HOME_ICON_N, buttons[i].cx - BTN_HALF, buttons[i].cy - BTN_HALF, 3, off);
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
  if (bathUntil) return "Splish splash!";  // universal onomatopoeia
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

// ---------- the close button ----------
// One round X, at the bottom in the middle, closes whatever it's on: the menus, the card, the
// Pokedex, the zones, the clock. (Swipes still work too; games in play keep swiping up to quit.)
#define CLOSE_Y 422  // its centre on most screens
#define CLOSE_R 21
void drawCloseButtonAt(int cy) {  // a gray disc with a white X
  gfx->fillCircle(CX, cy, CLOSE_R, C565(0x8e, 0x8c, 0x96));
  const int a = 8;
  for (int o = -1; o <= 1; o++) {
    gfx->drawLine(CX - a + o, cy - a, CX + a + o, cy + a, UI_WHITE);
    gfx->drawLine(CX + a + o, cy - a, CX - a + o, cy + a, UI_WHITE);
  }
}
void drawCloseButton() { drawCloseButtonAt(CLOSE_Y); }
bool closeHitAt(int16_t x, int16_t y, int cy) {  // a little more generous than what's drawn
  int dx = x - CX, dy = y - cy;
  return dx * dx + dy * dy <= (CLOSE_R + 10) * (CLOSE_R + 10);
}
bool closeHit(int16_t x, int16_t y) { return closeHitAt(x, y, CLOSE_Y); }

// draws an n x n pixel map, scaled; silhouette=true paints it in ink
void drawMap(const char *const *map, int n, int x, int y, int s, bool silhouette) {
  for (int r = 0; r < n; r++) {
    const char *row = map[r];
    forRuns(n, [&](int c) { return charKey(row[c], silhouette); },
            [&](int c0, int len, int k) { gfx->fillRect(x + c0 * s, y + r * s, len * s, s, silhouette ? INK_K : spriteColor((char)k)); });
  }
}

// ---------- press preview (press.h) ----------

// the game screens' tap, by whichever is open (same order as handleTouch checks them)
void gameTap(int16_t x, int16_t y) {
  if (sdOpen) sdTap(x, y);
  else if (vbOpen) vbTap();
  else if (battleOpen) battleTap(x, y);
  else if (quizOpen) quizTap(x, y);
  else if (gamesMenuOpen) gamesMenuTap(x, y);
  else if (zoneScreen) zoneTap(x, y);
  else if (runOpen) runTap();
  else if (dungeonOpen && !dnHeld) dungeonTap(x, y);
}

static bool pressIn(int16_t x, int16_t y, int rx, int ry, int rw, int rh) {
  return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}
static bool pressSet(PressTarget &t, int x, int y, int w, int h, const char *label, const char *const *icon = nullptr) {
  t = { (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, label, icon };
  return true;
}

// The control at (x, y) on the open screen, false if there is none there. It mirrors the
// screens' own tap code, so it is only for real controls: petting, aiming, walking and the
// "tap anywhere to close" screens keep working as they did, without a preview.
bool pressTargetAt(int16_t x, int16_t y, PressTarget &t) {
  static char buf[52];
  // --- game screens (handleTouch's first branch) ---
  if (battleOpen) {
    if (bPhase != BP_CHOOSE) return false;
    if (y >= 392 && x >= 160 && x <= 306) return pressSet(t, 160, 394, 146, 38, T(S_BTL_RUN));
    if (y < 344 || y > 386) return false;
    for (uint8_t i = 0; i < bMeMoves.count; i++) {
      int x0, x1;
      battleMoveRect(i, x0, x1);
      if (x >= x0 && x <= x1) return pressSet(t, x0, 346, x1 - x0, 40, MOVE_TBL[bMeMoves.ids[i]].name);
    }
    return false;
  }
  if (sdOpen) {
    if (sdPhase != SD_ASK || sdMyPick >= 0 || y < SD_OPT_Y) return false;
    int i = (y - SD_OPT_Y) / (SD_OPT_H + SD_OPT_GAP);
    if (i > 2) return false;
    int inset = i == 2 ? 26 : 0;
    return pressSet(t, 83 + inset, SD_OPT_Y + i * (SD_OPT_H + SD_OPT_GAP), 300 - 2 * inset, SD_OPT_H, dexName(sdOpts[i]));
  }
  if (quizOpen) {
    if (quizPhase != QP_ASK || y < QUIZ_OPT_Y) return false;
    int i = (y - QUIZ_OPT_Y) / (QUIZ_OPT_H + QUIZ_OPT_GAP);
    if (i > 2) return false;
    int inset = i == 2 ? 26 : 0;
    return pressSet(t, 83 + inset, QUIZ_OPT_Y + i * (QUIZ_OPT_H + QUIZ_OPT_GAP), 300 - 2 * inset, QUIZ_OPT_H, dexName(quizOpts[i]));
  }
  if (gamesMenuOpen) {
    int row = (y - GAMES_ROW_Y) / (GAMES_ROW_H + GAMES_ROW_GAP);
    if (y < GAMES_ROW_Y || row >= GAMES_COUNT || x < GAMES_ROW_X || x > GAMES_ROW_X + GAMES_ROW_W) return false;
    return pressSet(t, GAMES_ROW_X, GAMES_ROW_Y + row * (GAMES_ROW_H + GAMES_ROW_GAP), GAMES_ROW_W, GAMES_ROW_H, gameLabel(row));
  }
  if (zoneScreen == ZS_PICK) {
    for (uint8_t z = 0; z < ZONE_COUNT; z++) {
      int dx = x - ZONE_XY[z][0], dy = y - ZONE_XY[z][1];
      if (dx * dx + dy * dy < (ZONE_ICON_R + 6) * (ZONE_ICON_R + 6) || (abs(dx) < 66 && dy > 0 && dy < ZONE_ICON_R + 44))
        return pressSet(t, ZONE_XY[z][0] - ZONE_ICON_R, ZONE_XY[z][1] - ZONE_ICON_R, 2 * ZONE_ICON_R, 2 * ZONE_ICON_R, T(ZONE_NAME[z]));
    }
    return false;
  }
  if (zoneScreen) {
    if (zoneDetail) return false;  // an entry: tap anywhere to go back
    if (y >= ZONE_BTN_Y && y < ZONE_BTN_Y + ZONE_BTN_H && x >= ZONE_BTN_X && x < ZONE_BTN_X + ZONE_BTN_W)
      return pressSet(t, ZONE_BTN_X, ZONE_BTN_Y, ZONE_BTN_W, ZONE_BTN_H, T(S_ZONE_SEARCH));
    return false;  // Pokedex grids (here and below) swipe between pages: a slow swipe must not pick a square
  }
  if (gameOpen || runOpen || dungeonOpen || vbOpen) return false;  // aiming, jumping, walking, volleying: no buttons

  // --- everything else (same order as onTap) ---
  if (pet.awaitingStarter()) {
    for (int i = 0; i < 3; i++) {
      int ry = STARTER_ROW_Y + i * (STARTER_ROW_H + STARTER_ROW_GAP);
      if (pressIn(x, y, 70, ry, 326, STARTER_ROW_H)) return pressSet(t, 70, ry, 326, STARTER_ROW_H, dexName(STARTER_DEX[i]));
    }
    return false;
  }
  if (pet.awaitingEggPick()) {
    if (eggPickDetail) {
      if (y >= PICK_BTN_Y && y < PICK_BTN_Y + PICK_BTN_H && abs(x - CX) <= 110)
        return pressSet(t, CX - 110, PICK_BTN_Y, 220, PICK_BTN_H, T(S_CHOOSE_BTN));
      if (closeHit(x, y)) return pressSet(t, CX - CLOSE_R, CLOSE_Y - CLOSE_R, 2 * CLOSE_R, 2 * CLOSE_R, "X");
    }
    return false;
  }
  if (buddyMenuOpen) {
    if (abs(x - CX) > 130 || y < BUDDY_ROW_Y || y >= BUDDY_ROW_Y + BUDDY_ROWS * BUDDY_ROW_STEP) return false;
    int row = (y - BUDDY_ROW_Y) / BUDDY_ROW_STEP;
    return pressSet(t, CX - 130, BUDDY_ROW_Y + row * BUDDY_ROW_STEP, 260, BUDDY_ROW_H, T(BUDDY_ROW_LABEL[row]));
  }
  if (playMenuOpen) {
    for (int i = 0; i < PLAY_GAME_COUNT; i++) {
      int gy = PLAY_ROW_Y + i * (PLAY_ROW_H + 10);
      if (abs(x - CX) <= 150 && y >= gy && y < gy + PLAY_ROW_H) return pressSet(t, CX - 150, gy, 300, PLAY_ROW_H, T(PLAY_GAMES[i]));
    }
    if (closeHit(x, y)) return pressSet(t, CX - CLOSE_R, CLOSE_Y - CLOSE_R, 2 * CLOSE_R, 2 * CLOSE_R, "X");
    return false;
  }
  if (tradeOpen) {
    if (trPhase != TR_REVIEW) return false;
    if (y >= PICK_BTN_Y - 40 && y < PICK_BTN_Y - 40 + PICK_BTN_H && abs(x - CX) <= 110)
      return pressSet(t, CX - 110, PICK_BTN_Y - 40, 220, PICK_BTN_H, T(S_BUDDY_TRADE));
    if (closeHit(x, y)) return pressSet(t, CX - CLOSE_R, CLOSE_Y - CLOSE_R, 2 * CLOSE_R, 2 * CLOSE_R, "X");
    return false;
  }
  if (galleryOpen) return false;
  if (kbOpen) {
    if (x < KB_X || y < KB_Y) return false;
    int col = (x - KB_X) / KB_W, row = (y - KB_Y) / KB_H;
    if (col >= KB_COLS || row >= 5) return false;
    int i = row * KB_COLS + col;
    if (i == 28) strlcpy(buf, "<-", sizeof(buf));
    else if (i == 29) strlcpy(buf, "OK", sizeof(buf));
    else { buf[0] = KB_KEYS[i]; buf[1] = 0; }
    return pressSet(t, KB_X + col * KB_W, KB_Y + row * KB_H, KB_W - 6, KB_H - 6, buf);
  }
  if (clockOpen) {
#ifdef BOARD_KNOMI_V2
    if (clockPage == 1) {
      WifiSetupState ws = wifiSetupState();
      if (ws == WS_OFF && pressIn(x, y, 96, 236, 274, 48)) return pressSet(t, 96, 236, 274, 48, T(S_WIFI_CHANGE));
      if (ws == WS_OFF && pressIn(x, y, 96, UPD_BTN_Y, 274, 48)) {
        if (otaState() == OTA_AVAILABLE) snprintf(buf, sizeof(buf), T(S_UPD_TO_FMT), otaNewVersion());
        else strlcpy(buf, T(S_UPD_CHECK), sizeof(buf));
        return pressSet(t, 96, UPD_BTN_Y, 274, 48, buf);
      }
      if (ws != WS_LIST) return false;  // the password keyboard has its own preview
      int n = wifiSetupNetCount();
      if (y >= WIFI_ROW_Y && y < WIFI_ROW_Y + WIFI_ROWS * WIFI_ROW_H) {
        int r = (y - WIFI_ROW_Y) / WIFI_ROW_H, idx = wifiListPage * WIFI_ROWS + r;
        if (idx >= n) return false;
        return pressSet(t, 62, WIFI_ROW_Y + r * WIFI_ROW_H, 342, WIFI_ROW_H - 6, wifiSetupNet(idx));
      }
      if (y >= 334 && y <= 376) {
        if (x < 170) return pressSet(t, 96, 334, 70, 40, "<");
        if (x > 296) return pressSet(t, 300, 334, 70, 40, ">");
        return pressSet(t, 176, 334, 114, 40, "SCAN");
      }
      if (y > 376 && y < 440) return pressSet(t, 150, 384, 166, 40, T(S_CANCEL));
      return false;
    }
#endif
    if (y >= 190 && y <= 248) {
      static const int16_t BX[4] = { 104, 170, 252, 318 };
      for (int i = 0; i < 4; i++) {
        if (x < BX[i] || x >= BX[i] + 58) continue;
        snprintf(buf, sizeof(buf), "%s %s", T(i < 2 ? S_HOUR : S_MIN), i % 2 ? "+" : "-");
        return pressSet(t, BX[i], 190, 58, 58, buf);
      }
      return false;
    }
    if (y >= LANG_PILL_Y && y <= LANG_PILL_Y + LANG_PILL_H) {
      if (x >= 34 && x < 130) return pressSet(t, 34, LANG_PILL_Y, 96, LANG_PILL_H, audioEnabled() ? T(S_SND_ON) : T(S_SND_OFF));
      if (x >= LANG_PILL_X && x < LANG_PILL_X + LANG_PILL_W) {
        snprintf(buf, sizeof(buf), "%s >", LANG_CODES[gLang]);
        return pressSet(t, LANG_PILL_X, LANG_PILL_Y, LANG_PILL_W, LANG_PILL_H, buf);
      }
      return false;
    }
    if (pressIn(x, y, 133, 340, 200, 48)) return pressSet(t, 133, 340, 200, 48, "OK");
    return false;
  }
  if (pet.ceremony) return false;
  if (cardOpen) {
#ifndef BOARD_KNOMI_V2
    if (!cardAch && cardPage == 1 && pressIn(x, y, 96, 300, 274, 40)) return pressSet(t, 96, 300, 274, 40, T(S_BTL_START));
#endif
    return false;
  }
  if (choiceKind) {
    const StrId *o;
    static const StrId EVO[2] = { S_EVO_TAP, S_EVO_KEEP }, RESET[2] = { S_RESET_GO, S_RESET_KEEP },
                       YESNO[2] = { S_YES, S_NO }, RUN[2] = { S_RUNAWAY_CARE, S_RUNAWAY_GO }, FAR[2] = { S_FAR_GO, S_FAR_STAY };
    o = choiceKind == 1 ? EVO : choiceKind == 3 ? RESET : choiceKind == 5 ? YESNO : choiceKind == 4 ? RUN : FAR;
    if (pressIn(x, y, 93, 206, 280, 52)) return pressSet(t, 93, 206, 280, 52, T(o[0]));
    if (pressIn(x, y, 93, 268, 280, 52)) return pressSet(t, 93, 268, 280, 52, T(o[1]));
    return false;
  }
  if (confirmUntil) {
    if (pressIn(x, y, 118, 252, 100, 52)) return pressSet(t, 118, 252, 100, 52, T(S_YES));
    if (pressIn(x, y, 248, 252, 100, 52)) return pressSet(t, 248, 252, 100, 52, T(S_NO));
    return false;
  }
  if (feedMenuUntil) {
    static const char *const *const ICON[4] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_CANDY };
    int count = pet.canHaveCandy() ? 4 : 3;
    int x0 = CX - count * 33;
    if (y < 288 || y > 352 || x < x0 || x > x0 + count * 66) return false;
    int item = (x - x0) / 66;
    if (item >= count) item = count - 1;
    return pressSet(t, x0 + item * 66, 288, 66, 64, nullptr, ICON[item]);
  }
  if (pet.isEgg()) return false;
  int bt = buddyAt(x, y);
  if (bt >= 0) {
    int n = partyBuddies(), k = 0;
    for (int i = 0; i <= bt; i++) k += bPeers[i].on;
    const char *nm = buddyNameOf(bt);
    if (y <= BUDDY_PILL_Y + BUDDY_PILL_H + 6) pressSet(t, 60, BUDDY_PILL_Y, 346, BUDDY_PILL_H, nm);
    else pressSet(t, partyBuddyX(n, k) - partyHalf(n), PET_GROUND - partyTall(n), 2 * partyHalf(n), partyTall(n), nm);
    t.soft = true;  // buddies sit where swipes start: they give way to them
    return true;
  }
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  if (pet.wantEvolveButton() && pressIn(x, y, EVO_BTN_X, EVO_BTN_Y, EVO_BTN_W, EVO_BTN_H))
    return pressSet(t, EVO_BTN_X, EVO_BTN_Y, EVO_BTN_W, EVO_BTN_H, T(S_EVO_TAP));
  if (pressIn(x, y, FAR_BTN_X, FAR_BTN_Y, FAR_BTN_W, FAR_BTN_H) && (pet.canRunawayNow() || pet.wantFarewellButton())) {
    snprintf(buf, sizeof(buf), T(pet.canRunawayNow() ? S_RUNAWAY_BTN : S_FAREWELL_BTN), nm);
    return pressSet(t, FAR_BTN_X, FAR_BTN_Y, FAR_BTN_W, FAR_BTN_H, buf);
  }
  for (int i = 0; i < 4; i++) {
    int dx = x - buttons[i].cx, dy = y - buttons[i].cy;
    if (dx * dx + dy * dy <= BTN_HIT * BTN_HIT)
      return pressSet(t, buttons[i].cx - BTN_HALF, buttons[i].cy - BTN_HALF, 2 * BTN_HALF, 2 * BTN_HALF, nullptr, buttons[i].icon);
  }
  return false;
}

// Drawn last on every frame (from the canvas flush): the outline on the control and its
// large copy beside the finger, in the Wi-Fi keyboard's colours.
void pressOverlay() {
  if (!pressOn) return;
  const PressTarget &t = pressT;
  const uint16_t ring = TYPE_ACCENT[TY_WATER];
  for (int i = 0; i < 4; i++) gfx->drawRoundRect(t.x - 3 - i, t.y - 3 - i, t.w + 6 + 2 * i, t.h + 6 + 2 * i, 12 + i, ring);
  if (t.icon) {
    const int s = 5, side = HOME_ICON_N * s + 24;  // only the home buttons and the food menu carry an icon
    PressBox b = pressPopupBox(tXl, tYl, side + 16, side + 16);
    gfx->fillRoundRect(b.x, b.y, b.w, b.h, 16, ring);
    gfx->fillRoundRect(b.x + 8, b.y + 8, side, side, 12, UI_WHITE);
    drawMap(t.icon, HOME_ICON_N, b.x + 20, b.y + 20, s, false);
    return;
  }
  if (!t.label || !t.label[0]) return;
  uint8_t size = 5;
  setSize(size);
  while (size > 2 && textW(t.label, size) > 340) setSize(--size);
  int16_t tw = textW(t.label, size), th = (gCjkFont ? 16 : 8) * gTextSize;
  PressBox b = pressPopupBox(tXl, tYl, tw + 40, th + 36);
  gfx->fillRoundRect(b.x, b.y, b.w, b.h, 16, ring);
  gfx->setTextColor(UI_WHITE);
  setCur(b.x + (b.w - tw) / 2, b.y + 18);
  printT(t.label);
}
