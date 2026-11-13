#pragma once
// V2: each Pokemon's cry, in the manner of the old handhelds' sound chip (Fedor: the sounds "should be closer to how
// they sound in pokemon games"): no words, a creature's call. Two to four short notes on two pulse waves, the second
// a harmony or a near-unison above the first so they buzz against each other, each note sweeping up or down, a fast
// warble on it, the pulses' width flipping halfway, and noise under it, a growl or a hiss, the rougher the type. The
// last note is held and falls away.
//
// Ours, not the games': every cry is worked out here from the Pokemon itself, not copied or transcribed from any
// game. Its size and speed set the pitch and the pace (small and quick ones high and fast, big slow ones low and
// long), its type the rasp and the warble, and its number seeds the rest, so a Pokemon always makes the same cry.
// Petted it's a little higher and shorter; after a win it's the whole cry, a moment after the level-up blip.
// cryBuild() runs on the game's thread; CryPlayer plays the result on the audio thread (host_audio.cpp), and the
// Shiny Pokedex page plays the same pieces (host/voice_json.h, tools/voices.json).
#include <stdint.h>
#include <math.h>
#include "dex.h"
#include "music.h"  // VOICE_PET, VOICE_WIN

struct CrySeg {   // a piece of a cry
  uint16_t ms;
  float f0, f1;   // the first pulse's pitch, sliding (Hz)
  float v0, v1;   // its level, from v0 to v1 (0..1)
  uint8_t duty;   // 0 12.5%, 1 25%, 2 50% (the second pulse takes the next)
  float vib;      // the warble: semitones...
  float vibHz;    // ...and how fast
  float harm;     // the second pulse, semitones above the first (0: none)
  float hv;       // ...its level against the first's
  float rasp;     // the noise under it, against the first's level
  float nhz;      // ...its clock: low a growl, high a hiss
};
#define CRY_MAX 16
struct Cry {
  uint8_t n = 0;
  uint16_t delayMs = 0;  // a pause first (after a win, the level-up blip; petted, the heart)
  CrySeg seg[CRY_MAX];
};

// how high a species calls, in semitones from A 440: first forms high, final ones lower; quick ones up, sturdy (much
// HP) ones down; legends lower still; and a few that look nothing like their stats
static inline float cryPitch(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return 4;
  const DexEntry &d = DEX_TBL[dex];
  int stage = 0;
  for (int16_t c = dex, moved = 1; moved && stage < 3;) {
    moved = 0;
    for (int16_t p = 1; p <= DEX_COUNT; p++)
      if (DEX_TBL[p].evolvesTo == c) {
        c = p;
        stage++;
        moved = 1;
        break;
      }
  }
  if (dex > DEX_EEVEE && dex <= DEX_EEVEE + 3) stage = 1;
  float s = (stage == 0 ? 7.0f : stage == 1 ? 2.0f : -3.0f) + (d.bSpe - 70) / 14.0f - (d.bHp - 60) / 14.0f;
  if (d.rarity == R_LEGENDARY) s -= 3;
  switch (dex) {
    case 35: case 39: s += 9; break;   // Clefairy, Jigglypuff
    case 36: case 40: case 113: s += 6; break;  // Clefable, Wigglytuff, Chansey
    case 95: case 143: s -= 6; break;  // Onix, Snorlax
    case 130: s -= 6; break;           // Gyarados
    case 150: s -= 7; break;           // Mewtwo
  }
  return s < -14 ? -14 : s > 12 ? 12 : s;
}

// a type's rasp (how much noise growls under the call) and warble (how much it wavers), 0..1
static inline float cryRasp(uint8_t t) {
  switch (t) {
    case TY_ROCK: return 0.65f;
    case TY_GROUND: return 0.55f;
    case TY_POISON: case TY_DRAGON: return 0.5f;
    case TY_GHOST: case TY_FIGHTING: return 0.42f;
    case TY_FIRE: return 0.38f;
    case TY_ELECTRIC: case TY_BUG: return 0.3f;
    case TY_NORMAL: case TY_ICE: return 0.18f;
    default: return 0.12f;  // water, grass, psychic
  }
}
static inline float cryWarble(uint8_t t) {
  switch (t) {
    case TY_GHOST: return 1.0f;
    case TY_WATER: return 0.85f;
    case TY_PSYCHIC: case TY_ICE: return 0.75f;
    default: return 0.4f;
  }
}

// its number's dice: the same every time
struct CryDice {
  uint32_t s;
  explicit CryDice(int16_t dex) : s(0x9E3779B9u ^ (uint32_t)(dex * 2654435761u)) { next(); next(); }
  uint32_t next() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
  }
  float in(float a, float b) { return a + (b - a) * (next() % 10000) / 10000.0f; }
  int pick(int n) { return (int)(next() % (uint32_t)n); }
};

static inline float cryHz(float semis) { return 440.0f * powf(2.0f, semis / 12.0f); }

static inline void cryBuild(Cry &c, int16_t dex, uint8_t mood) {
  c.n = 0;
  bool win = mood == VOICE_WIN;
  c.delayMs = win ? 320 : 140;
  if (dex < 1 || dex > DEX_COUNT) dex = 25;
  const DexEntry &d = DEX_TBL[dex];
  CryDice r(dex);
  float base = cryPitch(dex) + (win ? 0.0f : 2.0f);
  float rasp = cryRasp(d.type), warble = cryWarble(d.type);
  // its pace: quick, small ones fast, slow big ones long (ms a note)
  float unit = 200.0f - d.bSpe * 0.5f - base * 2.0f;
  unit = unit < 110 ? 110 : unit > 280 ? 280 : unit;
  if (!win) unit *= 0.92f;
  int notes = 2 + r.pick(3);  // 2..4
  // its own voice, the same in every note: the second pulse's interval (a near unison buzzes; a third, a fifth or an
  // octave sings), the noise's clock (rough types growl low), the warble's speed
  static const float HARM[6] = { 0.18f, 0.25f, 3.0f, 4.0f, 7.0f, 12.0f };
  float harm = HARM[r.pick(6)];
  float hv = r.in(0.45f, 0.8f);
  float nhz = rasp > 0.35f ? r.in(2200, 5500) : r.in(9000, 18000);
  float vibHz = r.in(14, 26);
  int duty = r.pick(3);
  // its shape: where each note starts against its pitch, and which way it sweeps
  float at = r.in(-1, 4);
  for (int k = 0; k < notes; k++) {
    bool last = k == notes - 1;
    float s = base + at, sweep;
    if (last) {
      sweep = -r.in(4, 10);  // held, falling away
    } else {
      sweep = r.in(-6, 5);
      at = at + sweep * 0.5f + r.in(-3, 4);  // the next starts near where this one went
      if (at > 7) at -= 5;
      if (at < -6) at += 5;
    }
    uint16_t ms = (uint16_t)(unit * r.in(0.75f, 1.25f) * (last ? (win ? 2.3f : 1.9f) : 1.0f));
    float vib = (0.25f + warble * 1.1f) * r.in(0.6f, 1.2f);
    float rs = rasp * r.in(0.7f, 1.2f) * (last ? 1.2f : 1.0f);
    if (rs > 0.85f) rs = 0.85f;
    float f0 = cryHz(s), fm = cryHz(s + sweep * 0.5f), f1 = cryHz(s + sweep);
    float vm = last ? 0.55f : 0.8f, v1 = last ? 0.0f : 0.62f;
    int d1 = r.pick(3) ? duty : (duty + 1) % 3;  // the width flips halfway, now and then
    if (c.n + 2 > CRY_MAX) break;
    c.seg[c.n++] = { (uint16_t)(ms / 2), f0, fm, 1.0f, vm, (uint8_t)duty, vib, vibHz, harm, hv, rs, nhz };
    c.seg[c.n++] = { (uint16_t)(ms - ms / 2), fm, f1, vm, v1, (uint8_t)d1, vib, vibHz, harm, hv, rs, nhz };
    duty = d1;
  }
}

// ---------------------------------------------------------------- playing it
#define CRY_TONE 0.11f   // its loudness: about the effects' (two pulses and the noise together)
#define CRY_NOISE 0.10f

class CryPlayer {
 public:
  void begin(float rate) {
    rate_ = rate;
    ease_ = 1.0f - expf(-1.0f / (0.0015f * rate));  // a level settles in 1.5 ms: no clicks between the pieces
  }
  void start(const Cry &c) {
    c_ = c;
    i_ = 0;
    t_ = 0;
    wait_ = (uint32_t)(c.delayMs * rate_ / 1000.0f);
    tAll_ = 0;
    on_ = c.n > 0;
  }
  bool active() const { return on_ || env_ > 1e-4f; }

  float sample() {
    float target = 0;
    if (on_ && wait_) {
      wait_--;
    } else if (on_) {
      const CrySeg &s = c_.seg[i_];
      uint32_t len = (uint32_t)(s.ms * rate_ / 1000.0f);
      float k = len ? (float)t_ / len : 1.0f;
      target = s.v0 + (s.v1 - s.v0) * k;
      float w = s.vib * sinf(6.2831853f * s.vibHz * (float)tAll_ / rate_);
      freq_ = (s.f0 + (s.f1 - s.f0) * k) * powf(2.0f, w / 12.0f);
      ratio_ = s.harm > 0 ? powf(2.0f, s.harm / 12.0f) : 0.0f;
      duty_ = s.duty;
      hv_ = s.hv;
      rasp_ = s.rasp;
      nhz_ = s.nhz;
      tAll_++;
      if (++t_ >= len) {
        t_ = 0;
        if (++i_ >= c_.n) on_ = false;
      }
    }
    env_ += (target - env_) * ease_;
    if (env_ < 1e-5f || freq_ <= 0) return 0.0f;
    float out = pulse(ph_, freq_, duty_);
    if (ratio_ > 0) out += hv_ * pulse(ph2_, freq_ * ratio_, (uint8_t)((duty_ + 1) % 3));
    out *= CRY_TONE;
    if (rasp_ > 0) {
      np_ += nhz_ / rate_;
      while (np_ >= 1.0f) {
        np_ -= 1.0f;
        uint16_t bit = (lfsr_ ^ (lfsr_ >> 1)) & 1;
        lfsr_ = (lfsr_ >> 1) | (bit << 14);
      }
      out += ((lfsr_ & 1) ? 1.0f : -1.0f) * rasp_ * CRY_NOISE;
    }
    return out * env_;
  }

 private:
  static float blep(float t, float dt) {
    if (t < dt) {
      t /= dt;
      return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
      t = (t - 1.0f) / dt;
      return t * t + t + t + 1.0f;
    }
    return 0.0f;
  }
  float pulse(float &ph, float f, uint8_t duty) {  // a band-limited pulse, centred
    float dt = f / rate_;
    if (dt > 0.45f) dt = 0.45f;
    float dc = duty == 0 ? 0.125f : duty == 1 ? 0.25f : 0.5f;
    float v = (ph < dc ? 1.0f : -1.0f) + blep(ph, dt);
    float t2 = ph - dc;
    if (t2 < 0) t2 += 1.0f;
    v -= blep(t2, dt);
    ph += dt;
    if (ph >= 1.0f) ph -= 1.0f;
    return v - (2 * dc - 1);
  }
  Cry c_;
  float rate_ = 48000, ease_ = 0.01f;
  uint8_t i_ = 0, duty_ = 2;
  uint32_t t_ = 0, wait_ = 0, tAll_ = 0;
  bool on_ = false;
  float env_ = 0, freq_ = 0, ratio_ = 0, hv_ = 0, rasp_ = 0, nhz_ = 0, ph_ = 0, ph2_ = 0, np_ = 0;
  uint16_t lfsr_ = 1;
};
