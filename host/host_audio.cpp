// Game effects as PCM: the same square-wave tones that play on the Waveshare (sfx_notes.h),
// for whoever wants to play them. The audio thread calls hostAudioRender(); the queue has a single
// producer and a single consumer, so no locks are needed on that thread. V2 mixes the music under them
// (music_synth.h), ducked while an effect plays.
#include "audio.h"
#include "savename.h"
#include "host.h"
#include "sfx_notes.h"
#include <Preferences.h>
#include <atomic>
#if UI_V2
#include "music_synth.h"
#include "cry.h"
#endif

#define SAMPLE_RATE HOST_AUDIO_RATE
#define QUEUE_LEN 8

static std::atomic<bool> gOn{true};
static std::atomic<bool> gSleeping{false};
static uint8_t gQueue[QUEUE_LEN];
static std::atomic<uint32_t> gHead{0}, gTail{0};

#if UI_V2
// the music: the game says which track its screen wants (musicWant), the audio thread plays it. Silent with SOUND or
// the Music switch off, while the pet sleeps, and while the window is hidden
static MusicPlayer gMusic;
static std::atomic<uint8_t> gMusicWant{MUS_NONE};
static std::atomic<bool> gMusicOn{true}, gMusicHidden{false};
static float gDuck = 1.0f;  // the music's level under the effects
#define DUCK_LEVEL 0.45f    // ...while one plays
#define DUCK_IN_MS 4.0f     // how fast it gets out of the way (a time constant)
#define DUCK_OUT_MS 120.0f  // ...and comes back
// the levels, Low, Medium and High (1..3), kept apart from on and off, so switching back on comes back at the same
// one: SOUND's is everything (saved as sndv), the music's its share under it (musv). High is how they always played
static const float LEVEL_GAIN[4] = { 0.0f, 0.3f, 0.55f, 1.0f };
static std::atomic<uint8_t> gSndLv{3}, gMusLv{3};
static float gSndGain = 1.0f, gMusGain = 1.0f;  // (the audio thread's, eased toward the levels: no click)
#define LEVEL_EASE_MS 25.0f

static void saveLevel(const char *key, uint8_t v) {
  Preferences p;
  p.begin(SAVE_NS, false);
  p.putUChar(key, v);
  p.end();
}
// a Pokemon's cry (cry.h): built here on the game's thread, handed over when the audio thread has taken the last one
// (a new one while that's still waiting is dropped)
static CryPlayer gVoice;
static Cry gVoiceNext;
static std::atomic<bool> gVoiceReady{false};
void voiceSay(int16_t dex, uint8_t mood) {
  if (gSleeping || !gOn || gVoiceReady.load(std::memory_order_acquire)) return;
  cryBuild(gVoiceNext, dex, mood);
  gVoiceReady.store(true, std::memory_order_release);
}
void musicWant(uint8_t track) { gMusicWant.store(track, std::memory_order_relaxed); }
bool musicEnabled() { return gMusicOn; }
void musicSetEnabled(bool on) {
  gMusicOn = on;
  Preferences p;
  p.begin(SAVE_NS, false);
  p.putBool("mus", on);
  p.end();
}
uint8_t soundVolume() { return gSndLv; }
void soundSetVolume(uint8_t v) {
  gSndLv = v < 1 ? 1 : v > 3 ? 3 : v;
  saveLevel("sndv", gSndLv);
  if (!gOn) audioSetEnabled(true);
}
uint8_t musicVolume() { return gMusLv; }
void musicSetVolume(uint8_t v) {
  gMusLv = v < 1 ? 1 : v > 3 ? 3 : v;
  saveLevel("musv", gMusLv);
  if (!gMusicOn) musicSetEnabled(true);
}
bool hostMusicEnabled(void) { return gMusicOn; }
void hostSetMusicEnabled(bool on) { musicSetEnabled(on); }
int hostMusicVolume(void) { return gMusLv; }
void hostSetMusicVolume(int v) { musicSetVolume((uint8_t)v); }
void hostMusicHidden(bool hidden) { gMusicHidden = hidden; }
uint8_t hostMusicWanted() { return gMusicWant; }  // (for the tests)
#else
uint8_t hostMusicWanted() { return 0; }
bool hostMusicEnabled(void) { return false; }
void hostSetMusicEnabled(bool) {}
int hostMusicVolume(void) { return 0; }
void hostSetMusicVolume(int) {}
void hostMusicHidden(bool) {}
#endif

void audioBegin() {
  Preferences p;
  p.begin(SAVE_NS, true);
  gOn = p.getBool("snd", true);
#if UI_V2
  gMusicOn = p.getBool("mus", true);
  uint8_t sv = p.getUChar("sndv", 3), mv = p.getUChar("musv", 3);
  gSndLv = sv >= 1 && sv <= 3 ? sv : 3;
  gMusLv = mv >= 1 && mv <= 3 ? mv : 3;
  gSndGain = LEVEL_GAIN[gSndLv];
  gMusGain = LEVEL_GAIN[gMusLv];
  gMusic.begin(SAMPLE_RATE);
  gVoice.begin(SAMPLE_RATE);
#endif
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
  p.begin(SAVE_NS, false);
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
#if UI_V2
// V2: each tone's frequency is kept to a fraction of a sample (a 32-bit phase that wraps), so high notes stay in
// tune; a legato effect's notes run into each other (sfxLegato(): the tap)
static uint32_t gStep = 0, gAcc = 0;
static bool gLegato = false;
#define V2_ATTACK (SAMPLE_RATE * 4 / 1000)  // the ramps, as long as at 16 kHz
#define V2_DECAY (SAMPLE_RATE * 6 / 1000)
#endif

static void startNote() {
  const Note &n = gDef->n[gNote];
  gTotal = SAMPLE_RATE * n.ms / 1000;
  gHalf = n.f ? SAMPLE_RATE / (2 * n.f) : 0;
  gDone = gPhase = 0;
  gHigh = true;
#if UI_V2
  gStep = n.f ? (uint32_t)(((uint64_t)n.f << 32) / SAMPLE_RATE) : 0;
  if (!gLegato || gNote == 0) gAcc = 0;
#endif
}

#if UI_V2
// an effect's sample with the music added under it (playing: an effect sounds, so the music ducks)
// at SOUND's level, the music at its own under that
static inline int16_t withMusic(int16_t s, bool playing) {
  static const float duckIn = 1.0f - expf(-1000.0f / (DUCK_IN_MS * SAMPLE_RATE));
  static const float duckOut = 1.0f - expf(-1000.0f / (DUCK_OUT_MS * SAMPLE_RATE));
  static const float ease = 1.0f - expf(-1000.0f / (LEVEL_EASE_MS * SAMPLE_RATE));
  float voice = gVoice.sample();
  float target = playing || gVoice.active() ? DUCK_LEVEL : 1.0f;
  gDuck += (target - gDuck) * (target < gDuck ? duckIn : duckOut);
  gSndGain += (LEVEL_GAIN[gSndLv.load(std::memory_order_relaxed)] - gSndGain) * ease;
  gMusGain += (LEVEL_GAIN[gMusLv.load(std::memory_order_relaxed)] - gMusGain) * ease;
  float m = 0;
  gMusic.render(&m, 1, gDuck * gMusGain);
  int32_t v = (int32_t)lrintf((s + (m + voice) * 32767.0f) * gSndGain);
  return (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
}
#endif

extern "C" void hostAudioRender(int16_t *out, int frames) {
#if UI_V2
  {
    bool quiet = !gOn || !gMusicOn;
    uint8_t want = quiet || gSleeping || gMusicHidden ? (uint8_t)MUS_NONE : gMusicWant.load(std::memory_order_relaxed);
    gMusic.want(want, quiet);
    if (gVoiceReady.load(std::memory_order_acquire)) {
      gVoice.start(gVoiceNext);
      gVoiceReady.store(false, std::memory_order_release);
    }
  }
#endif
  for (int i = 0; i < frames; i++) {
    if (!gDef) {
      uint32_t tail = gTail.load(std::memory_order_relaxed);
      if (tail == gHead.load(std::memory_order_acquire)) {
#if UI_V2
        out[i] = withMusic(0, false);
#else
        out[i] = 0;
#endif
        continue;
      }
      gDef = &SFX[gQueue[tail % QUEUE_LEN]];
#if UI_V2
      gLegato = sfxLegato(gQueue[tail % QUEUE_LEN]);
#endif
      gTail.store(tail + 1, std::memory_order_release);
      gNote = 0;
      startNote();
    }
    int16_t s = 0;
#if UI_V2
    if (gStep) {
      bool first = gNote == 0, last = gNote + 1 >= gDef->len;
      int32_t amp = 5000;
      if (gLegato && last) amp -= amp * 45 / 100 * gDone / (gTotal ? gTotal : 1);  // the held one fades to 55%
      s = (int16_t)((gAcc & 0x80000000u) ? amp : -amp);
      if ((!gLegato || first) && gDone < V2_ATTACK) s = (int16_t)(s * gDone / V2_ATTACK);  // only where it starts
      else if ((!gLegato || last) && gDone > gTotal - V2_DECAY) s = (int16_t)(s * (gTotal - gDone) / V2_DECAY);  // ends
      gAcc += gStep;
    }
    s = withMusic(s, true);
#else
    if (gHalf) {
      const int16_t amp = 5000;
      s = gHigh ? amp : -amp;
      if (gDone < 64) s = (int16_t)(s * gDone / 64);                                // attack
      else if (gDone > gTotal - 96) s = (int16_t)(s * (gTotal - gDone) / 96);       // decay
      if (++gPhase >= gHalf) { gPhase = 0; gHigh = !gHigh; }
    }
#endif
    out[i] = s;
    if (++gDone >= gTotal) {
      if (++gNote >= gDef->len) gDef = nullptr;
      else startNote();
    }
  }
}
