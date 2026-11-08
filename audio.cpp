#include "audio.h"
#include "savename.h"
#include "pin_config.h"

#ifdef BOARD_KNOMI_V2
// The Knomi V2 has no codec or speaker: the game calls the same and nothing sounds.
void audioBegin() {}
void sfxPlay(uint8_t) {}
void audioSetEnabled(bool) {}
bool audioEnabled() { return false; }
void audioSetSleeping(bool) {}
bool audioBusy() { return false; }
void audioSetIdle(bool) {}
#else
#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>
#include <Preferences.h>
#include "sfx_notes.h"

// ---------------------------------------------------------------------------
// PokeKnomiGochi audio: ES8311 codec (DAC -> PA amplifier -> speaker) over I2S.
// ES8311 init ported from Espressif's official driver (esp-bsp), fixed to
// MCLK=4.096MHz (256*fs), 16kHz, 16-bit, I2S slave. The effects are square
// tones (Game Boy style) synthesized in a separate task so as not to
// block the game loop.
// ---------------------------------------------------------------------------

#define ES8311_ADDR 0x18
#define SAMPLE_RATE 16000

static I2SClass i2s;
static bool gReady = false;
static bool gOn = true;
static bool gSleeping = false;
static bool gIdle = false;          // no recent interaction (see power.h): the amplifier draws power at idle
static uint32_t gAmpHoldUntil = 0;  // an effect requested at idle keeps the amplifier on until here
static bool gAmpHold = false;       // (separate flag: so the comparison doesn't fail when millis() wraps around)
static uint32_t gAmpOnAt = 0;       // when the amplifier was turned on
static QueueHandle_t gQ = nullptr;

// The NS4150B takes well over 8 ms to settle after each power-off, so
// turning it on right before each effect swallowed the short ones: the startup
// jingle (440 ms) was heard and the 35 ms beeps were not. It is left on while
// there is sound and the pet is awake, as in Waveshare's verified
// example, and only turned off when sleeping or muting.
static void updateAmplifierPower() {
  // gReady avoids turning it on on a board where the codec didn't start: there the
  // amplifier would only add hiss, because nothing is going to sound.
  if (gAmpHold && (int32_t)(gAmpHoldUntil - millis()) <= 0) gAmpHold = false;
  bool idleOff = gIdle && !gAmpHold;
  static int last = -1;  // the pin is only written if it changes
  int want = (gReady && gOn && !gSleeping && !idleOff) ? HIGH : LOW;
  if (want != last) {
    last = want;
    if (want == HIGH) gAmpOnAt = millis();
    digitalWrite(PA, want);
  }
}

// ---- codec I2C ----
static bool esW(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ES8311_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}
static uint8_t esR(uint8_t reg) {
  Wire.beginTransmission(ES8311_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(ES8311_ADDR, 1);
  return Wire.available() ? Wire.read() : 0;
}

// INIT sequence VERIFIED ON THIS BOARD (PlaneRadar2.0 project, same
// Waveshare 1.75). Key: clock DERIVED FROM BCLK (reg01=0xBF, no external MCLK)
// and internal reference feeding the DAC (reg44=0x58); without those two the codec
// answered over I2C but no audio came out. 16kHz, 16-bit, I2S slave.
static bool es8311Init() {
  Wire.beginTransmission(ES8311_ADDR);
  if (Wire.endTransmission() != 0) return false;

  // open()
  esW(0x0D, 0xFA); esW(0x44, 0x08); esW(0x44, 0x08);  // power up + first-write quirk
  esW(0x01, 0x30); esW(0x02, 0x00); esW(0x03, 0x10); esW(0x16, 0x24);
  esW(0x04, 0x10); esW(0x05, 0x00); esW(0x0B, 0x00); esW(0x0C, 0x00);
  esW(0x10, 0x1F); esW(0x11, 0x7F);
  esW(0x00, 0x80); esW(0x00, 0x80);                   // reset clock, slave
  esW(0x01, 0xBF);                                    // clk src = BCLK (no external MCLK)
  { uint8_t r = esR(0x06); r &= ~0x20; esW(0x06, r); }  // SCLK not inverted
  esW(0x13, 0x10); esW(0x1B, 0x0A); esW(0x1C, 0x6A);
  esW(0x44, 0x58);                                    // internal reference -> feeds the DAC

  // config_sample(): BCLK*8 = DIG_MCLK
  esW(0x02, 0x18); esW(0x05, 0x00); esW(0x03, 0x10); esW(0x04, 0x20);
  { uint8_t r = esR(0x07); r &= 0xC0; esW(0x07, r); }
  esW(0x08, 0xFF);
  { uint8_t r = esR(0x06); r &= 0xE0; r |= 0x03; esW(0x06, r); }  // bclk_div=4

  // 16-bit I2S format
  esW(0x09, 0x0C); esW(0x0A, 0x0C);

  // start() DAC slave
  esW(0x00, 0x80); esW(0x01, 0xBF); esW(0x09, 0x0C); esW(0x0A, 0x0C);
  esW(0x17, 0xBF); esW(0x0E, 0x02); esW(0x12, 0x00); esW(0x14, 0x1A);
  esW(0x0D, 0x01); esW(0x15, 0x40); esW(0x37, 0x08); esW(0x45, 0x00);

  // volume + unmute
  esW(0x32, 0xBF);                                    // DAC volume ~0 dB
  { uint8_t r = esR(0x31); r &= 0x9F; esW(0x31, r); }  // unmute
  return true;
}

// ---- square-wave tone synthesizer ----
static int16_t buf[256 * 2];  // interleaved stereo (L=R)

#if UI_V2
// V2: plays a tone (or silence if f==0), its frequency kept to a fraction of a sample (a 32-bit phase), so high notes
// stay in tune; a legato effect's notes (the tap) run into each other, the wave carried on, the ramps only where it
// starts and ends, and its last note fading to 55% (sfx_notes.h, sfxLegato())
static uint32_t gAcc = 0;
static void playTone(uint16_t f, uint16_t ms, bool legato, bool first, bool last) {
  int total = SAMPLE_RATE * ms / 1000;
  uint32_t step = f ? (uint32_t)(((uint64_t)f << 32) / SAMPLE_RATE) : 0;
  if (!legato || first) gAcc = 0;
  int done = 0;
  while (done < total) {
    int n = total - done; if (n > 256) n = 256;
    for (int i = 0; i < n; i++) {
      int16_t s = 0;
      if (step) {
        int idx = done + i;
        int32_t amp = 5000;
        if (legato && last) amp -= amp * 45 / 100 * idx / total;
        s = (int16_t)((gAcc & 0x80000000u) ? amp : -amp);
        if ((!legato || first) && idx < 64) s = (int16_t)(s * idx / 64);                     // attack
        else if ((!legato || last) && idx > total - 96) s = (int16_t)(s * (total - idx) / 96);  // decay
        gAcc += step;
      }
      buf[i * 2] = s; buf[i * 2 + 1] = s;
    }
    i2s.write((uint8_t *)buf, n * 4);
    done += n;
  }
}
#else
// plays a tone (or silence if f==0) with an anti-click attack/decay ramp
static void playTone(uint16_t f, uint16_t ms) {
  int total = SAMPLE_RATE * ms / 1000;
  int half = f ? (SAMPLE_RATE / (2 * f)) : 0;  // half period in samples
  const int16_t amp = 5000;
  int phase = 0, done = 0;
  bool high = true;
  while (done < total) {
    int n = total - done; if (n > 256) n = 256;
    for (int i = 0; i < n; i++) {
      int16_t s = 0;
      if (f) {
        s = high ? amp : -amp;
        int idx = done + i;
        if (idx < 64) s = (int16_t)(s * idx / 64);                 // attack
        else if (idx > total - 96) s = (int16_t)(s * (total - idx) / 96);  // decay
        if (++phase >= half) { phase = 0; high = !high; }
      }
      buf[i * 2] = s; buf[i * 2 + 1] = s;
    }
    i2s.write((uint8_t *)buf, n * 4);
    done += n;
  }
}
#endif

static void audioTask(void *) {
  uint8_t id;
  for (;;) {
    if (xQueueReceive(gQ, &id, portMAX_DELAY) && gOn && !gSleeping && gReady &&
        id < SFX_COUNT) {
      const SfxDef &d = SFX[id];
      // the NS4150B takes more than 8 ms to settle: if it was just turned on, the effect waits
      int32_t since = (int32_t)(millis() - gAmpOnAt);
      if (since >= 0 && since < 50) vTaskDelay(pdMS_TO_TICKS(50 - since));
#if UI_V2
      for (uint8_t i = 0; i < d.len; i++) playTone(d.n[i].f, d.n[i].ms, sfxLegato(id), i == 0, i + 1 == d.len);
#else
      for (uint8_t i = 0; i < d.len; i++) playTone(d.n[i].f, d.n[i].ms);
#endif
    }
  }
}

void audioBegin() {
  // I2S first: starts the MCLK the codec needs to lock on
  pinMode(PA, OUTPUT);
  digitalWrite(PA, LOW);   // until we know whether sound is enabled

  i2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
  if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    Serial.println("I2S begin failed");
    return;
  }
  if (!es8311Init()) { Serial.println("ES8311 not responding (audio off)"); return; }

  Preferences p;
  p.begin(SAVE_NS, true);
  gOn = p.getBool("snd", true);
  p.end();

  gReady = true;
  updateAmplifierPower();
  gQ = xQueueCreate(8, sizeof(uint8_t));
  xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 1, nullptr, 0);
  sfxPlay(SFX_HATCH);  // startup jingle (confirms it sounds)
}

bool audioBusy() { return gAmpHold && (int32_t)(gAmpHoldUntil - millis()) > 0; }

void sfxPlay(uint8_t id) {
  if (gReady && gOn && gQ) {
    if (gIdle) {  // an alert at idle (e.g. "ready to evolve") has to be heard
      gAmpHoldUntil = millis() + 4000;
      gAmpHold = true;
      updateAmplifierPower();
    }
    xQueueSend(gQ, &id, 0);  // drops if the queue is full
  }
}

void audioSetEnabled(bool on) {
  gOn = on;
  updateAmplifierPower();
  Preferences p;
  p.begin(SAVE_NS, false);
  p.putBool("snd", on);
  p.end();
}
bool audioEnabled() { return gOn; }

void audioSetSleeping(bool sleeping) {
  if (gSleeping == sleeping) return;
  gSleeping = sleeping;
  updateAmplifierPower();  // asleep there are no effects: amp off, no power draw
}

void audioSetIdle(bool idle) {
  gIdle = idle;
  updateAmplifierPower();  // also when an alert's hold expires
}

#endif  // BOARD_KNOMI_V2
