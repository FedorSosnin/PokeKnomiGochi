// Game effects as PCM: the same square-wave tones that play on the Waveshare (sfx_notes.h),
// for whoever wants to play them. The audio thread calls hostAudioRender(); the queue has a single
// producer and a single consumer, so no locks are needed on that thread.
#include "audio.h"
#include "host.h"
#include "sfx_notes.h"
#include <Preferences.h>
#include <atomic>

#define SAMPLE_RATE HOST_AUDIO_RATE
#define QUEUE_LEN 8

static std::atomic<bool> gOn{true};
static bool gSleeping = false;
static uint8_t gQueue[QUEUE_LEN];
static std::atomic<uint32_t> gHead{0}, gTail{0};

void audioBegin() {
  Preferences p;
  p.begin("tamapoke", true);
  gOn = p.getBool("snd", true);
  p.end();
}

static HostHaptic gHaptic = nullptr;
void hostSetHaptic(HostHaptic fn) { gHaptic = fn; }

void sfxPlay(uint8_t id) {
  if (gSleeping || id >= SFX_COUNT) return;
  if (gHaptic) gHaptic(id);  // with the sound muted too: it has its own switch
  if (!gOn) return;
  uint32_t head = gHead.load(std::memory_order_relaxed);
  if (head - gTail.load(std::memory_order_acquire) >= QUEUE_LEN) return;  // drop if full
  gQueue[head % QUEUE_LEN] = id;
  gHead.store(head + 1, std::memory_order_release);
}

void audioSetEnabled(bool on) {
  gOn = on;
  Preferences p;
  p.begin("tamapoke", false);
  p.putBool("snd", on);
  p.end();
}
bool audioEnabled() { return gOn; }
bool hostSoundEnabled(void) { return gOn; }
void hostSetSoundEnabled(bool on) { audioSetEnabled(on); }
void audioSetSleeping(bool sleeping) { gSleeping = sleeping; }
bool audioBusy() { return false; }
void audioSetIdle(bool) {}

// ---- synthesizer (audio thread) ----
static const SfxDef *gDef = nullptr;
static uint8_t gNote = 0;
static int gDone = 0, gTotal = 0, gHalf = 0, gPhase = 0;
static bool gHigh = true;

static void startNote() {
  const Note &n = gDef->n[gNote];
  gTotal = SAMPLE_RATE * n.ms / 1000;
  gHalf = n.f ? SAMPLE_RATE / (2 * n.f) : 0;
  gDone = gPhase = 0;
  gHigh = true;
}

extern "C" void hostAudioRender(int16_t *out, int frames) {
  for (int i = 0; i < frames; i++) {
    if (!gDef) {
      uint32_t tail = gTail.load(std::memory_order_relaxed);
      if (tail == gHead.load(std::memory_order_acquire)) {
        out[i] = 0;
        continue;
      }
      gDef = &SFX[gQueue[tail % QUEUE_LEN]];
      gTail.store(tail + 1, std::memory_order_release);
      gNote = 0;
      startNote();
    }
    int16_t s = 0;
    if (gHalf) {
      const int16_t amp = 5000;
      s = gHigh ? amp : -amp;
      if (gDone < 64) s = (int16_t)(s * gDone / 64);                                // attack
      else if (gDone > gTotal - 96) s = (int16_t)(s * (gTotal - gDone) / 96);       // decay
      if (++gPhase >= gHalf) { gPhase = 0; gHigh = !gHigh; }
    }
    out[i] = s;
    if (++gDone >= gTotal) {
      if (++gNote >= gDef->len) gDef = nullptr;
      else startNote();
    }
  }
}
