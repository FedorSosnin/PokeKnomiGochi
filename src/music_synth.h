#pragma once
// V2: the music player (music.h). Four voices, as a handheld of the square-wave effects' era had: two pulse waves
// (12.5%, 25% or 50% duty) for the tune and its harmony, a stepped triangle or a 4-bit wave for the bass and a drum
// voice (noise, with a falling tone under it for the kick and the toms). Each note is shaped by its instrument's
// envelope, with vibrato on held notes, slides between slurred ones and the chord arpeggios a pulse plays as one
// note. The pulses are band-limited (polyBLEP), so their high notes stay clean at 48 kHz.
//
// A song is four byte streams, one a voice (tools/music/songs.py writes them into music_data.h):
//   1..108       a note (MIDI number), then its length in ticks (96 a whole note); +0x80: slurred from the last one
//   MS_REST      then its ticks
//   MS_INST      then the instrument (MUSIC_INST; on the drum voice the notes are MUSIC_DRUMS + 1)
//   MS_VOL       then the voice's volume, 0..15
//   MS_ARP       then two offsets, (a << 4) | b semitones: the note cycles through 0, a, b (0: none)
//   MS_LOOP      where the song comes back to at its end
//   MS_END       the end (all four voices end on the same tick)
// Everything here runs on the audio thread: no allocation, no locks.
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "music.h"

#define MS_REST 0xF0
#define MS_INST 0xF1
#define MS_VOL 0xF2
#define MS_ARP 0xF3
#define MS_LOOP 0xFE
#define MS_END 0xFF
#define MS_SLUR 0x80
#define MS_TICKS_PER_BEAT 24

enum : uint8_t { MW_P12, MW_P25, MW_P50, MW_TRI, MW_WAVE };
// MW_WAVE: a handheld's wave voice, 32 steps of 0..15 (its own shape: a rounded pulse leaning a little), the warm,
// slightly buzzy bass of the old portable consoles; it averages 7.5, so it's centred
static const uint8_t MUSIC_WAVE[32] = { 8, 11, 13, 14, 15, 15, 15, 15, 14, 14, 13, 13, 12, 12, 11, 10,
                                        7, 4, 2, 1, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 5 };
struct MusicInst {
  uint8_t wave;
  float atk, dec, sus, rel;  // the envelope: ms, ms, its level 0..1, ms
  float fade;                // ms over which a held note dies away (0: it holds)
  float gap;                 // ms of silence before the next note, unless that one is slurred
  float vibDepth, vibRate, vibDelay;  // semitones, Hz, and how long a note is held before it wavers (ms)
  float glide;               // ms a slurred note slides to its pitch
};
struct MusicDrum {
  float toneHz0, toneHz1, toneMs, toneVol, toneDecay;  // a triangle falling from Hz0 to Hz1 in toneMs, dying in toneDecay
  float noiseHz;                                       // the noise's clock
  uint8_t noiseShort;                                  // the short (metallic) noise loop
  float noiseVol, noiseDecay;
};
struct MusicSong {
  const char *name;
  float bpm, gain;
  uint8_t loops;                // 0: a jingle, played once
  uint32_t introTicks, loopTicks;
  const uint8_t *ch[4];
};

#include "music_data.h"  // MUSIC_INST, MUSIC_DRUMS, MUSIC_SONGS (tools/music/songs.py)

// each voice's share of the mix: the pulses are loud for their amplitude, the triangle soft
static const float MUSIC_CH_GAIN[4] = { 0.28f, 0.40f, 0.24f, 0.62f };
#define MUSIC_LEVEL 0.29f      // the whole mix, a song's gain on top: about 9 dB under the effects
#define MUSIC_ARP_HZ 50.0f     // an arpeggio's notes go by at this rate

static inline float musicPolyBlep(float t, float dt) {
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

struct MusicVoice {
  uint16_t pos = 0;           // in its stream
  double next = 0;            // the song's sample at which its next event is read
  double gateOff = 0;         // ...and at which this note lets go
  uint8_t inst = 0, vol = 15, arpA = 0, arpB = 0;
  float note = 0, from = 0;   // the note (semitones), and where a slide comes from
  float t = 0;                // ms since the note began
  float slideT = 0;           // ms since a slurred note began sliding
  float env = 0, relFrom = 0;
  uint8_t stage = 0;          // 0 silent, 1 attack, 2 decay, 3 sustain, 4 release
  float phase = 0;
  uint8_t drum = 0;           // the drum voice: which one sounds (0 none)
  float toneHz = 0;
  uint16_t lfsr = 1;
  float noisePh = 0, noiseOut = 1;
  bool slide = false;
};

struct MusicState {
  uint8_t track = MUS_NONE;
  double clock = 0;           // samples into the song
  MusicVoice v[4];
  bool done = false;          // a jingle that has finished
};

class MusicPlayer {
 public:
  uint8_t mute = 0;  // voices left out, a bit each (for listening to one alone: host/music_wav.cpp)
  void begin(float rate) {
    rate_ = rate;
    ms_ = 1000.0f / rate;
  }
  uint8_t playing() const { return st_.track; }

  // the track to play: the one playing fades out first (fast: when the sound was switched off)
  void want(uint8_t track, bool fast = false) {
    if (track >= MUS_COUNT) track = MUS_NONE;
    want_ = track;
    fast_ = fast;
  }

  // the music for n samples, added to out (scaled by gain: the effects duck it)
  void render(float *out, int n, float gain) {
    for (int i = 0; i < n; i++) {
      step();
      if (st_.track == MUS_NONE || fade_ <= 0.0f) continue;
      out[i] += sample() * fade_ * gain;
    }
  }

 private:
  float rate_ = 48000, ms_ = 1000.0f / 48000;
  MusicState st_;
  uint8_t want_ = MUS_NONE;
  bool fast_ = false;
  float fade_ = 0, fadeStep_ = 0;
  bool fadingOut_ = false;
  double now_ = 0;  // samples since begin(), for the resumable tracks
  struct Kept { bool ok = false; double at = 0; MusicState st; };
  Kept kept_[MUS_COUNT];

  static bool resumable(uint8_t t) {
    return t == MUS_HOME || t == MUS_NIGHT || t == MUS_MENU || t == MUS_TRAVEL || t == MUS_TOWER || t == MUS_HELLO;
  }

  double spt() const { return rate_ * 60.0 / (MUSIC_SONGS[st_.track - 1].bpm * MS_TICKS_PER_BEAT); }

  // follows want_: fades the one playing out, then starts the next (resumed where it was if it was left lately)
  void step() {
    now_ += 1;
    if (want_ != st_.track) {
      if (st_.track != MUS_NONE && fade_ > 0.0f && !st_.done) {
        if (!fadingOut_) {
          fadingOut_ = true;
          float ms = fast_ ? 80.0f : want_ == MUS_NONE ? 1500.0f : want_ == MUS_VICTORY ? 60.0f : 350.0f;
          fadeStep_ = fade_ * ms_ / ms;
        }
        fade_ -= fadeStep_;
        if (fade_ > 0.0f) return;
      }
      fade_ = 0;
      fadingOut_ = false;
      if (st_.track != MUS_NONE && resumable(st_.track) && !st_.done) {
        kept_[st_.track].ok = true;
        kept_[st_.track].at = now_;
        kept_[st_.track].st = st_;
      }
      start(want_);
      return;
    }
    if (fadingOut_) {  // wanted again while it was fading: back up
      fadingOut_ = false;
      fadeStep_ = -ms_ / 250.0f;
    }
    if (fadeStep_ < 0.0f) {
      fade_ -= fadeStep_;
      if (fade_ >= 1.0f) {
        fade_ = 1.0f;
        fadeStep_ = 0;
      }
    }
  }

  void start(uint8_t track) {
    Kept &k = kept_[track];
    if (track != MUS_NONE && resumable(track) && k.ok && now_ - k.at < 90.0 * rate_) {
      st_ = k.st;  // where it was, faded in
      k.ok = false;
      fade_ = 0.0001f;
      fadeStep_ = -ms_ / 400.0f;
      return;
    }
    st_ = MusicState();
    st_.track = track;
    if (track == MUS_NONE) return;
    fade_ = 1.0f;  // a new one starts on its first beat
    fadeStep_ = 0;
    for (int c = 0; c < 4; c++) st_.v[c].lfsr = 1;
  }

  static float hz(float note) { return 440.0f * powf(2.0f, (note - 69.0f) / 12.0f); }

  // the stream's next event that sounds (a note, a rest, the end), skipping the settings: is it slurred?
  bool nextSlurred(const uint8_t *s, uint16_t p) const {
    for (;;) {
      uint8_t b = s[p];
      if (b == MS_LOOP) { p++; continue; }
      if (b == MS_INST || b == MS_VOL || b == MS_ARP) { p += 2; continue; }
      return b < MS_REST && (b & MS_SLUR);
    }
  }

  void event(int c) {
    const MusicSong &song = MUSIC_SONGS[st_.track - 1];
    const uint8_t *s = song.ch[c];
    MusicVoice &v = st_.v[c];
    for (;;) {
      uint8_t b = s[v.pos];
      if (b == MS_END) {
        if (!song.loops) {  // a jingle: its last notes ring out, then it's done (sample())
          if (v.stage && v.stage < 4) release(v);
          v.next = 1e300;
          v.gateOff = 0;
          return;
        }
        uint16_t p = 0;  // back to the loop point (every other event is two bytes)
        while (s[p] != MS_LOOP && s[p] != MS_END) p += 2;
        v.pos = s[p] == MS_LOOP ? p + 1 : 0;
        continue;
      }
      if (b == MS_LOOP) { v.pos++; continue; }
      uint8_t a = s[v.pos + 1];
      v.pos += 2;
      if (b == MS_INST) { v.inst = a; continue; }
      if (b == MS_VOL) { v.vol = a; continue; }
      if (b == MS_ARP) { v.arpA = a >> 4; v.arpB = a & 15; continue; }
      double len = a * spt();
      double at = v.next;
      v.next = at + len;
      if (b == MS_REST) {
        if (v.stage && v.stage < 4) release(v);
        v.gateOff = 0;
        return;
      }
      bool slur = b & MS_SLUR;
      float note = (float)(b & 0x7F);
      if (c == 3) {  // a drum
        v.drum = (uint8_t)note;
        v.t = 0;
        v.phase = 0;
        v.toneHz = MUSIC_DRUMS[v.drum - 1].toneHz0;
        v.gateOff = 0;
        return;
      }
      const MusicInst &in = MUSIC_INST[v.inst];
      if (slur && v.stage && v.stage < 4) {  // slurred: no new attack, the vibrato carries on
        v.from = v.note;
        v.slide = in.glide > 0;
        v.slideT = 0;
      } else {
        v.t = 0;
        v.stage = 1;
        v.slide = false;
        if (v.env <= 0) v.phase = 0;
      }
      v.note = note;
      if (!v.slide) v.from = note;
      if (nextSlurred(s, v.pos)) {
        v.gateOff = 0;  // held into the next
      } else {
        double gap = in.gap * rate_ / 1000.0;
        if (gap > len * 0.5) gap = len * 0.5;
        v.gateOff = v.next - gap;
      }
      return;
    }
  }

  void release(MusicVoice &v) {
    v.stage = 4;
    v.relFrom = v.env;
  }

  float sample() {
    if (st_.done) return 0.0f;
    const MusicSong &song = MUSIC_SONGS[st_.track - 1];
    float mix = 0;
    bool over = true;  // a jingle's voices all ended and silent
    for (int c = 0; c < 4; c++) {
      MusicVoice &v = st_.v[c];
      while (st_.clock >= v.next) event(c);
      float x = voice(c, v);
      if (!(mute >> c & 1)) mix += x * MUSIC_CH_GAIN[c] * (v.vol / 15.0f);
      over = over && v.next > 1e299 && !v.stage && !v.drum;
    }
    st_.clock += 1;
    if (over) st_.done = true;
    return mix * song.gain * MUSIC_LEVEL;
  }

  float voice(int c, MusicVoice &v) {
    if (c == 3) return drum(v);
    if (!v.stage) return 0.0f;
    const MusicInst &in = MUSIC_INST[v.inst];
    if (v.gateOff > 0 && st_.clock >= v.gateOff && v.stage < 4) release(v);
    // the envelope
    switch (v.stage) {
      case 1:
        v.env += in.atk > 0 ? ms_ / in.atk : 1.0f;
        if (v.env >= 1.0f) { v.env = 1.0f; v.stage = 2; }
        break;
      case 2:
        v.env -= in.dec > 0 ? (1.0f - in.sus) * ms_ / in.dec : 1.0f;
        if (v.env <= in.sus) { v.env = in.sus; v.stage = 3; }
        break;
      case 3:
        if (in.fade > 0) v.env -= in.sus * ms_ / in.fade;
        if (v.env <= 0) { v.env = 0; v.stage = 0; }
        break;
      case 4:
        v.env -= in.rel > 0 ? v.relFrom * ms_ / in.rel : 1.0f;
        if (v.env <= 0) { v.env = 0; v.stage = 0; }
        break;
    }
    // the pitch: a slide, the arpeggio, the vibrato
    float note = v.note;
    if (v.slide) {
      float k = in.glide > 0 ? v.slideT / in.glide : 1.0f;
      v.slideT += ms_;
      if (k >= 1.0f) v.slide = false;
      else note = v.from + (v.note - v.from) * k;
    }
    if (v.arpA || v.arpB) {
      int n = v.arpB ? 3 : 2;
      int k = (int)(v.t * MUSIC_ARP_HZ / 1000.0f) % n;
      note += k == 1 ? v.arpA : k == 2 ? v.arpB : 0;
    }
    if (in.vibDepth > 0 && v.t > in.vibDelay) {
      float d = (v.t - in.vibDelay) / 200.0f;  // it grows in over 200 ms
      if (d > 1.0f) d = 1.0f;
      note += in.vibDepth * d * sinf(6.2831853f * in.vibRate * (v.t - in.vibDelay) / 1000.0f);
    }
    v.t += ms_;
    float dt = hz(note) / rate_;
    if (dt > 0.45f) dt = 0.45f;
    float out;
    if (in.wave == MW_TRI) {  // 16 steps up, 16 down
      int k = (int)(v.phase * 32.0f);
      int lvl = k < 16 ? k : 31 - k;
      out = (lvl - 7.5f) / 7.5f;
    } else if (in.wave == MW_WAVE) {
      out = (MUSIC_WAVE[(int)(v.phase * 32.0f) & 31] - 7.5f) / 7.5f;
    } else {
      float duty = in.wave == MW_P12 ? 0.125f : in.wave == MW_P25 ? 0.25f : 0.5f;
      out = v.phase < duty ? 1.0f : -1.0f;
      out += musicPolyBlep(v.phase, dt);
      float t2 = v.phase - duty;
      if (t2 < 0) t2 += 1.0f;
      out -= musicPolyBlep(t2, dt);
      out -= 2.0f * duty - 1.0f;  // centred: no thump as the envelope moves
    }
    v.phase += dt;
    if (v.phase >= 1.0f) v.phase -= 1.0f;
    return out * v.env;
  }

  float drum(MusicVoice &v) {
    if (!v.drum) return 0.0f;
    const MusicDrum &d = MUSIC_DRUMS[v.drum - 1];
    float out = 0;
    bool alive = false;
    if (d.toneVol > 0 && v.t < d.toneDecay) {
      float k = v.t < d.toneMs ? v.t / d.toneMs : 1.0f;
      float f = d.toneHz0 * powf(d.toneHz1 / d.toneHz0, k);
      float a = 1.0f - v.t / d.toneDecay;
      int s = (int)(v.phase * 32.0f);
      int lvl = s < 16 ? s : 31 - s;
      out += (lvl - 7.5f) / 7.5f * a * a * d.toneVol;
      v.phase += f / rate_;
      if (v.phase >= 1.0f) v.phase -= 1.0f;
      alive = true;
    }
    if (d.noiseVol > 0 && v.t < d.noiseDecay) {
      v.noisePh += d.noiseHz / rate_;
      while (v.noisePh >= 1.0f) {
        v.noisePh -= 1.0f;
        uint16_t bit = (v.lfsr ^ (v.lfsr >> (d.noiseShort ? 6 : 1))) & 1;
        v.lfsr = (v.lfsr >> 1) | (bit << 14);
        v.noiseOut = (v.lfsr & 1) ? 1.0f : -1.0f;
      }
      float a = 1.0f - v.t / d.noiseDecay;
      out += v.noiseOut * a * a * d.noiseVol;
      alive = true;
    }
    v.t += ms_;
    if (!alive) v.drum = 0;
    return out;
  }
};
