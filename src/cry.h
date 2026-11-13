#pragma once
// V2: each Pokemon's cry, made the way the old handhelds' sound chip made sounds (Fedor: the sounds "should be closer
// to how they sound in pokemon games"): no words, a creature's call, built the way that chip worked:
//   - it changes in steps, 60 a second: each step the pitch moves on (a sweep up or down, or a trill flicking between
//     two notes), the volume steps down (or up) on its envelope, and the pulse's width turns through a pattern, which
//     is what makes it buzz and phase;
//   - two pulse waves sound together, the second a near unison that beats against the first, or an octave or a fifth;
//   - noise under them with its own envelope: a low growl, a hiss, or the chip's short loop, metallic;
//   - two to four notes, the last one held, fading.
// A line of evolutions shares one shape, as a family sounds alike: the forms differ in pitch and length (the later,
// the longer).
//
// Ours, not the games': every cry is worked out here from the Pokemon itself, not copied or transcribed from any game.
// Its size and speed set the pitch (small and quick ones high, big slow ones low), its type the rasp, the trill and
// the noise, its line's first form seeds the shape, so a Pokemon always makes the same cry. Petted it's a little
// higher and shorter; after a win it's the whole cry, a moment after the level-up blip. cryBuild() runs on the game's
// thread; CryPlayer plays the result on the audio thread (host_audio.cpp), and the KnomiDex page plays the same
// notes (host/voice_json.h, tools/voices.json).
#include <stdint.h>
#include <math.h>
#include "dex.h"
#include "music.h"  // VOICE_PET, VOICE_WIN

#define CRY_FPS 60  // its steps a second

struct CryNote {    // a note of a cry, in steps
  uint8_t frames;
  float f0;         // its pitch at the start (Hz)...
  float sweep;      // ...moving this many semitones a step
  float trill;      // every other step this many semitones up (0: none)
  uint8_t v0;       // its volume, 0..15...
  int8_t vstep;     // ...stepping by this (-1 down, 0, +1 up)...
  uint8_t vevery;   // ...every this many steps
  uint8_t duty;     // its pulse width a step, four in turn, two bits each (0 12.5%, 1 25%, 2 50%, 3 75%)
  float harm;       // the second pulse, semitones above the first
  uint8_t hv;       // ...its volume against the first's, 0..15
  uint8_t n0;       // the noise's volume, 0..15...
  uint8_t nevery;   // ...a step down every this many steps (0: held)
  uint16_t nhz;     // ...its clock
  uint8_t nshort;   // ...the short loop: metallic
};
#define CRY_MAX 4
struct Cry {
  uint8_t n = 0;
  uint16_t delayMs = 0;  // a pause first (after a win, the level-up blip; petted, the heart)
  CryNote note[CRY_MAX];
};

// a line's first form (Eevee for its three), and how far along it a form is
static inline int16_t cryLineBase(int16_t d) {
  if (d > DEX_EEVEE && d <= DEX_EEVEE + 3) return DEX_EEVEE;
  for (bool moved = true; moved;) {
    moved = false;
    for (int16_t s = 1; s <= DEX_COUNT; s++)
      if (DEX_TBL[s].evolvesTo == d) {
        d = s;
        moved = true;
        break;
      }
  }
  return d;
}
static inline int cryStage(int16_t d) {
  if (d > DEX_EEVEE && d <= DEX_EEVEE + 3) return 1;
  int s = 0;
  for (bool moved = true; moved && s < 3;) {
    moved = false;
    for (int16_t p = 1; p <= DEX_COUNT; p++)
      if (DEX_TBL[p].evolvesTo == d) {
        d = p;
        s++;
        moved = true;
        break;
      }
  }
  return s;
}

// how high a species calls, in semitones from A 440: first forms high, final ones lower; quick ones up, sturdy (much
// HP) ones down; legends lower still; and a few that look nothing like their stats
static inline float cryPitch(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return 4;
  const DexEntry &d = DEX_TBL[dex];
  int stage = cryStage(dex);
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

// a type's rasp (how loud the noise under the call), 0..15, and its trill (water and ghosts waver), semitones
static inline uint8_t cryRasp(uint8_t t) {
  switch (t) {
    case TY_ROCK: return 11;
    case TY_GROUND: return 10;
    case TY_POISON: case TY_DRAGON: return 9;
    case TY_GHOST: case TY_FIGHTING: case TY_FIRE: return 8;
    case TY_ELECTRIC: case TY_BUG: return 6;
    case TY_NORMAL: case TY_ICE: return 4;
    default: return 2;  // water, grass, psychic
  }
}
static inline float cryTrill(uint8_t t) {
  switch (t) {
    case TY_GHOST: return 1.6f;
    case TY_WATER: case TY_ICE: return 1.0f;
    case TY_PSYCHIC: return 0.8f;
    default: return 0.0f;
  }
}

// dice: the same for the same seed
struct CryDice {
  uint32_t s;
  explicit CryDice(uint32_t seed) : s(0x9E3779B9u ^ (seed * 2654435761u)) { next(); next(); }
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

// the pulse width's turns, a step each, read from the low bits: plain, phasing, thinning, widening
static const uint8_t CRY_DUTIES[8] = {
  0xAA,  // 50 50 50 50
  0x66,  // 50 25 50 25
  0x5A,  // 50 50 25 25
  0x1B,  // 75 50 25 12.5
  0xE4,  // 12.5 25 50 75
  0x55,  // 25 25 25 25
  0x11,  // 25 12.5 25 12.5
  0xBB,  // 75 50 75 50
};

static inline void cryBuild(Cry &c, int16_t dex, uint8_t mood) {
  c.n = 0;
  bool win = mood == VOICE_WIN;
  c.delayMs = win ? 320 : 140;
  if (dex < 1 || dex > DEX_COUNT) dex = 25;
  int16_t line = cryLineBase(dex);
  int stage = cryStage(dex);
  CryDice r((uint32_t)line);          // the line's shape...
  CryDice own((uint32_t)dex + 1000);  // ...and this form's touch
  float base = cryPitch(dex) + own.in(-1, 1) + (win ? 0.0f : 2.0f);
  float len = (1.0f + 0.18f * stage) * (win ? 1.0f : 0.8f);  // later forms call longer
  uint8_t ltype = DEX_TBL[line].type, rasp = cryRasp(ltype);
  float trill = cryTrill(ltype);
  // the line's voice: the second pulse (a near unison beats; a fifth, an octave sing), the noise's clock and loop
  static const float HARM[6] = { 0.12f, 0.2f, 0.3f, 7.0f, 12.0f, 0.15f };
  float harm = HARM[r.pick(6)];
  uint8_t hv = (uint8_t)(8 + r.pick(6));
  uint16_t nhz = (uint16_t)(rasp >= 8 ? r.in(1800, 5200) : r.in(7000, 16000));
  uint8_t nshort = ltype == TY_ELECTRIC || ltype == TY_GHOST || r.pick(6) == 0;
  int notes = 2 + r.pick(3);  // 2..4
  float at = r.in(0, 5);      // where the first note starts, against its pitch
  for (int k = 0; k < notes && k < CRY_MAX; k++) {
    bool last = k == notes - 1;
    CryNote &n = c.note[c.n++];
    float fr = last ? r.in(16, 30) : r.in(4, 13);
    n.frames = (uint8_t)(fr * len + 0.5f);
    if (n.frames < 3) n.frames = 3;
    n.f0 = cryHz(base + at);
    n.sweep = last ? -r.in(0.05f, 0.3f) : r.in(-0.6f, 0.45f);
    n.trill = trill > 0 && r.pick(3) ? trill * r.in(0.6f, 1.2f) : 0.0f;
    n.v0 = (uint8_t)(last ? 12 + r.pick(4) : 10 + r.pick(6));
    n.vstep = last ? -1 : (int8_t)(r.pick(3) - 1);  // the last fades; the others swell, hold or fall
    n.vevery = (uint8_t)(last ? 1 + (n.frames + 9) / 12 : 2 + r.pick(3));
    n.duty = CRY_DUTIES[r.pick(8)];
    n.harm = harm;
    n.hv = hv;
    int nv = rasp ? rasp + r.pick(3) - 1 : 0;
    n.n0 = (uint8_t)(nv > 15 ? 15 : nv);
    n.nevery = (uint8_t)(last ? 2 : r.pick(3));
    n.nhz = nhz;
    n.nshort = nshort;
    if (!last) {  // the next starts near where this one went
      at += n.sweep * n.frames * 0.6f + r.in(-3, 5);
      if (at > 8) at -= 6;
      if (at < -6) at += 6;
    }
  }
}

// ---------------------------------------------------------------- playing it
#define CRY_TONE 0.09f   // its loudness: about the effects' (two pulses and the noise together)
#define CRY_NOISE 0.085f

class CryPlayer {
 public:
  void begin(float rate) {
    rate_ = rate;
    ease_ = 1.0f - expf(-1.0f / (0.0006f * rate));  // a step's level settles in 0.6 ms: stepped, not clicking
  }
  void start(const Cry &c) {
    c_ = c;
    i_ = fr_ = 0;
    t_ = 0;
    wait_ = (uint32_t)(c.delayMs * rate_ / 1000.0f);
    on_ = c.n > 0;
    if (on_) frame();
  }
  bool active() const { return on_ || env_ > 1e-4f || nenv_ > 1e-4f; }

  float sample() {
    if (on_ && wait_) {
      wait_--;
      return 0.0f;
    }
    if (on_ && ++t_ >= (uint32_t)(rate_ / CRY_FPS)) {  // the next step
      t_ = 0;
      if (++fr_ >= c_.note[i_].frames) {
        fr_ = 0;
        if (++i_ >= c_.n) on_ = false;
      }
      if (on_) frame();
      else vol_ = nvol_ = 0;
    }
    env_ += (vol_ - env_) * ease_;
    nenv_ += (nvol_ - nenv_) * ease_;
    float out = 0;
    if (env_ > 1e-5f) {
      out += pulse(ph_, freq_, duty_);
      if (hv_ > 0) out += hv_ * pulse(ph2_, freq_ * ratio_, duty_);
      out *= env_ * CRY_TONE;
    }
    if (nenv_ > 1e-5f) {
      np_ += nhz_ / rate_;
      while (np_ >= 1.0f) {
        np_ -= 1.0f;
        uint16_t bit = (lfsr_ ^ (lfsr_ >> 1)) & 1;
        lfsr_ = (uint16_t)((lfsr_ >> 1) | (bit << 14));
        if (nshort_) lfsr_ = (uint16_t)((lfsr_ & ~0x40) | (bit << 6));
      }
      out += ((lfsr_ & 1) ? -1.0f : 1.0f) * nenv_ * CRY_NOISE;
    }
    return out;
  }

 private:
  // what this step sounds: the note's pitch, volume and width at its step fr_
  void frame() {
    const CryNote &n = c_.note[i_];
    float semis = n.sweep * fr_ + (n.trill > 0 && (fr_ & 1) ? n.trill : 0.0f);
    freq_ = n.f0 * powf(2.0f, semis / 12.0f);
    int v = n.v0 + (n.vevery ? n.vstep * (int)(fr_ / n.vevery) : 0);
    vol_ = (v < 0 ? 0 : v > 15 ? 15 : v) / 15.0f;
    int nv = n.n0 - (n.nevery ? (int)(fr_ / n.nevery) : 0);
    nvol_ = (nv < 0 ? 0 : nv) / 15.0f;
    duty_ = (n.duty >> (2 * (fr_ & 3))) & 3;
    ratio_ = powf(2.0f, n.harm / 12.0f);
    hv_ = n.hv / 15.0f;
    nhz_ = n.nhz;
    nshort_ = n.nshort;
  }
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
    float dc = duty == 0 ? 0.125f : duty == 1 ? 0.25f : duty == 2 ? 0.5f : 0.75f;
    float v = (ph < dc ? 1.0f : -1.0f) + blep(ph, dt);
    float t2 = ph - dc;
    if (t2 < 0) t2 += 1.0f;
    v -= blep(t2, dt);
    ph += dt;
    if (ph >= 1.0f) ph -= 1.0f;
    return v - (2 * dc - 1);
  }
  Cry c_;
  float rate_ = 48000, ease_ = 0.03f;
  uint8_t i_ = 0, fr_ = 0, duty_ = 2, nshort_ = 0;
  uint32_t t_ = 0, wait_ = 0;
  bool on_ = false;
  float env_ = 0, nenv_ = 0, vol_ = 0, nvol_ = 0, freq_ = 0, ratio_ = 1, hv_ = 0, nhz_ = 0;
  float ph_ = 0, ph2_ = 0, np_ = 0;
  uint16_t lfsr_ = 0x7FFF;
};
