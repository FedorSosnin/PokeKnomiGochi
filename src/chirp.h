#pragma once
// V2: a Pokemon saying its name in chirps (Fedor: "Make each ones sound like its name is being said, but in a chiptune
// sound"). Each syllable of the name is a quick pitched blip on a pulse wave:
//   - its vowel sets the tone: i bright and high, e, a open, o, u round and low;
//   - its consonants how it starts and ends: a click (p, t, k...), a hiss (s, sh, f...), both (ch, j), or a soft
//     slide in (m, n, l, r...);
//   - the name's rhythm is its syllables', its last one held, rising.
// Smaller Pokemon chirp higher, bigger and slower ones lower. Petted it's bright and quick; after a win, prouder.
// Our own sound, made from the name: not any game's cries. Names in Latin letters, Japanese kana and Korean Hangul.
// chirpBuild() runs on the game's thread; ChirpPlayer plays the result on the audio thread (host_audio.cpp).
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "dex.h"
#include "music.h"  // VOICE_PET, VOICE_WIN

struct ChirpSeg {  // a piece of the voice
  uint16_t ms;
  float f0, f1;    // the tone, sliding from f0 to f1 (Hz; 0: none)
  float vol;       // ...its level
  uint8_t duty;    // 0 12.5%, 1 25%, 2 50%
  uint8_t noise;   // 0 none, 1 a click, 2 a hiss
};
#define CHIRP_MAX 48
struct Chirp {
  uint8_t n = 0;
  uint16_t delayMs = 0;  // a pause first (after a win, the level-up blip)
  ChirpSeg seg[CHIRP_MAX];
};

// ---------------------------------------------------------------- the name's syllables
enum : uint8_t { CS_NONE, CS_PLOSIVE, CS_FRIC, CS_AFFRIC, CS_SOFT };
struct ChirpSyl {
  char v;          // a e i o u
  uint8_t on, off; // how it starts and ends (CS_*)
  uint8_t lng;     // held longer (a long vowel)
};
#define CHIRP_SYL_MAX 8

static inline bool chirpVowel(char c) { return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'; }
static inline uint8_t chirpCons(char c) {
  switch (c) {
    case 'p': case 'b': case 't': case 'd': case 'k': case 'g': case 'c': case 'q': case 'x': return CS_PLOSIVE;
    case 's': case 'z': case 'f': case 'v': case 'h': return CS_FRIC;
    case 'j': return CS_AFFRIC;
    default: return CS_SOFT;  // m n l r w y
  }
}

// the letters of a name in Latin script, lower case: the font's accented letters (one byte, the i18n strings' codes)
// and UTF-8's become plain ones; anything else is left out
static inline int chirpLetters(const char *name, char *out, int max) {
  int n = 0;
  for (const uint8_t *p = (const uint8_t *)name; *p && n < max - 1; p++) {
    uint8_t c = *p;
    char l = 0;
    if (c >= 'A' && c <= 'Z') l = (char)(c - 'A' + 'a');
    else if (c >= 'a' && c <= 'z') l = (char)c;
    else if (c == 0xC3 && p[1]) {  // UTF-8, the Latin-1 letters
      uint8_t d = *++p;
      l = d >= 0xA0 && d <= 0xA5 ? 'a' : d == 0xA7 ? 's' : d >= 0xA8 && d <= 0xAB ? 'e' : d >= 0xAC && d <= 0xAF ? 'i'
          : d >= 0xB2 && d <= 0xB6 ? 'o' : d >= 0xB9 && d <= 0xBC ? 'u' : d == 0x9F ? 's' : 0;
    } else if (c >= 0x80) {  // the font's (code page 437)
      switch (c) {
        case 0x81: case 0x96: case 0x97: l = 'u'; break;
        case 0x82: case 0x88: case 0x89: case 0x8A: l = 'e'; break;
        case 0x83: case 0x84: case 0x85: case 0xA0: l = 'a'; break;
        case 0x87: case 0xE1: l = 's'; break;
        case 0x8B: case 0x8C: case 0xA1: l = 'i'; break;
        case 0x93: case 0x94: case 0xA2: l = 'o'; break;
        case 0xA3: l = 'u'; break;
        case 0xA4: l = 'n'; break;
      }
    }
    if (l) out[n++] = l;
  }
  out[n] = 0;
  return n;
}

// the vowel a run of vowels sounds like
static inline char chirpNucleus(const char *v, int n) {
  if (n >= 2) {
    char a = v[0], b = v[1];
    if ((a == 'e' && (b == 'e' || b == 'a')) || (a == 'i' && b == 'e') || (a == 'u' && b == 'i')) return 'i';
    if ((a == 'o' && (b == 'o' || b == 'u')) || (a == 'e' && b == 'u') || (a == 'u' && b == 'e')) return 'u';
    if (a == 'a' && b == 'u') return 'o';
    if (a == 'a' && b == 'i') return 'e';
  }
  return v[0] == 'y' ? 'i' : v[0];
}

static inline int chirpLatin(const char *name, ChirpSyl *syl) {
  char s[40];
  int n = chirpLetters(name, s, sizeof(s));
  // a final silent e (Arcanine, Salameche), not the -le of Squirtle
  int vowels = 0;
  for (int i = 0; i < n; i++) vowels += chirpVowel(s[i]);
  if (n >= 3 && s[n - 1] == 'e' && !chirpVowel(s[n - 2]) && s[n - 2] != 'l' && vowels >= 2) s[--n] = 0;
  // into vowel runs and consonants (ch, sh, th and ph one each; y a vowel unless a vowel follows it)
  struct Tok { bool v; char c; uint8_t k; };
  Tok t[40];
  int nt = 0;
  for (int i = 0; i < n && nt < 40;) {
    bool yv = s[i] == 'y' && !(i + 1 < n && chirpVowel(s[i + 1]));
    if (chirpVowel(s[i]) || yv) {
      int j = i;
      while (j < n && (chirpVowel(s[j]) || (s[j] == 'y' && !(j + 1 < n && chirpVowel(s[j + 1]))))) j++;
      t[nt++] = { true, chirpNucleus(s + i, j - i), 0 };
      i = j;
      continue;
    }
    if (i + 1 < n && s[i + 1] == 'h' && (s[i] == 'c' || s[i] == 's' || s[i] == 't' || s[i] == 'p')) {
      t[nt++] = { false, s[i], (uint8_t)(s[i] == 'c' ? CS_AFFRIC : CS_FRIC) };
      i += 2;
      continue;
    }
    t[nt++] = { false, s[i], chirpCons(s[i]) };
    i++;
  }
  // syllables: one a vowel run; of the consonants between two, the first closes the one before when there are two or
  // more, the rest open the next
  int ns = 0, i = 0;
  uint8_t on = CS_NONE;
  while (i < nt && !t[i].v) {  // what comes before the first vowel opens it
    if (on == CS_NONE) on = t[i].k;
    i++;
  }
  while (i < nt && ns < CHIRP_SYL_MAX) {
    ChirpSyl &y = syl[ns++];
    y = { t[i].c, on, CS_NONE, 0 };
    i++;
    int j = i;
    while (j < nt && !t[j].v) j++;
    int cons = j - i;
    if (j >= nt) {  // the end: what's left closes it
      if (cons) y.off = t[j - 1].k;
      break;
    }
    on = CS_NONE;
    if (cons >= 2) {
      y.off = t[i].k;
      on = t[i + 1].k;
    } else if (cons == 1) {
      on = t[i].k;
    }
    i = j;
  }
  return ns;
}

// Japanese kana, U+3041..U+3094 (katakana are moved onto them): each a syllable's vowel and how it starts; Q a
// small tsu (the one before stops short), N the n (the one before hums on), x and y the small vowels (they change the
// one before's)
static const char CHIRP_KANA_V[] =
    "aaiiuueeooaaiiuueeooaaiiuueeooaaiiQuueeooaiueoaaaiiiuuueeeoooaiueoaauuooaiueoaaieoNu";
static const char CHIRP_KANA_C[] =
    "x0x0x0x0x0ppppppppppfffcffffffppccQccppppsssssfppfppfppfppfppsssssysysyssssssxssssNf";
// Korean Hangul: the 19 first consonants, the 21 vowels, the 28 last consonants
static const char CHIRP_KO_ON[] = "ppsppssppff0ppppppf";
static const char CHIRP_KO_V[] = "aeaeoeoeoaeeouoeiuuii";
static const char CHIRP_KO_OFF[] = "0pppssspsssssssssppffspppppf";

static inline uint8_t chirpClassOf(char c) {
  return c == 'p' ? CS_PLOSIVE : c == 'f' ? CS_FRIC : c == 'c' ? CS_AFFRIC : c == 's' ? CS_SOFT : CS_NONE;
}
static inline uint32_t chirpNextCp(const uint8_t *&p) {
  uint32_t c = *p++;
  if (c < 0x80) return c;
  int more = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1;
  c &= 0x3F >> more;
  for (int i = 0; i < more && (*p & 0xC0) == 0x80; i++) c = (c << 6) | (*p++ & 0x3F);
  return c;
}

// a name's syllables: kana or Hangul where it has them, else its Latin letters
static inline int chirpSyllables(const char *name, ChirpSyl *syl) {
  int ns = 0;
  for (const uint8_t *p = (const uint8_t *)name; *p;) {
    uint32_t c = chirpNextCp(p);
    if (c >= 0x30A1 && c <= 0x30F6) c -= 0x60;  // katakana as hiragana
    if (c == 0x30FC) {                           // a long vowel
      if (ns) syl[ns - 1].lng = 1;
    } else if (c >= 0x3041 && c <= 0x3094) {
      int k = (int)(c - 0x3041);
      char v = CHIRP_KANA_V[k], cl = CHIRP_KANA_C[k];
      if ((cl == 'x' || cl == 'y') && ns) syl[ns - 1].v = v;
      else if (cl == '0' && ns && syl[ns - 1].v == v) syl[ns - 1].lng = 1;  // the same vowel again: held (chuu)
      else if (cl == 'Q') { if (ns) syl[ns - 1].off = CS_PLOSIVE; }
      else if (cl == 'N') { if (ns) syl[ns - 1].off = CS_SOFT; }
      else if (ns < CHIRP_SYL_MAX) syl[ns++] = { v, chirpClassOf(cl), CS_NONE, 0 };
    } else if (c >= 0xAC00 && c <= 0xD7A3) {
      int k = (int)(c - 0xAC00);
      if (ns < CHIRP_SYL_MAX)
        syl[ns++] = { CHIRP_KO_V[(k % 588) / 28], chirpClassOf(CHIRP_KO_ON[k / 588]), chirpClassOf(CHIRP_KO_OFF[k % 28]), 0 };
    }
  }
  return ns ? ns : chirpLatin(name, syl);
}

// ---------------------------------------------------------------- the voice
// how high a species chirps, in semitones from A 440: first forms high, final ones lower; quick ones up, sturdy
// (much HP) ones down; legends lower still; and a few that look nothing like their stats
static inline float chirpPitch(int16_t dex) {
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

static inline float chirpHz(float semis) { return 440.0f * powf(2.0f, semis / 12.0f); }

static inline void chirpAdd(Chirp &c, uint16_t ms, float f0, float f1, float vol, uint8_t duty, uint8_t noise) {
  if (c.n < CHIRP_MAX && ms) c.seg[c.n++] = { ms, f0, f1, vol, duty, noise };
}

// a name, said by a species: into c
static inline void chirpBuild(Chirp &c, const char *name, int16_t dex, uint8_t mood) {
  c.n = 0;
  c.delayMs = mood == VOICE_WIN ? 320 : 140;  // after the level-up blip, or the heart
  ChirpSyl syl[CHIRP_SYL_MAX];
  int n = name ? chirpSyllables(name, syl) : 0;
  if (!n) {  // nothing to say: one chirp
    syl[0] = { 'a', CS_NONE, CS_NONE, 0 };
    n = 1;
  }
  bool win = mood == VOICE_WIN;
  float base = chirpPitch(dex) + (win ? 0.0f : 2.0f);
  uint16_t sylMs = win ? 105 : 88;
  int timbre = dex > 0 ? dex % 3 : 0;  // some voices thinner, some rounder
  for (int k = 0; k < n; k++) {
    const ChirpSyl &y = syl[k];
    bool last = k == n - 1;
    float vs = y.v == 'i' ? 2.0f : y.v == 'e' ? 1.0f : y.v == 'o' ? -1.0f : y.v == 'u' ? -2.0f : 0.0f;
    float contour = n == 1 ? 2.0f : last ? (win ? 4.0f : 3.0f) : (k % 2 ? -2.0f : 0.0f);
    float p = chirpHz(base + vs + contour);
    int duty = y.v == 'i' ? 0 : y.v == 'e' ? 1 : 2;
    duty += timbre == 1 ? -1 : timbre == 2 ? 1 : 0;
    duty = duty < 0 ? 0 : duty > 2 ? 2 : duty;
    float vol = win ? 1.0f : 0.85f;
    // how it starts
    float from = p * 0.89f;  // (two semitones under)
    if (y.on == CS_PLOSIVE) chirpAdd(c, 7, 0, 0, 0, (uint8_t)duty, 1);
    else if (y.on == CS_AFFRIC) { chirpAdd(c, 6, 0, 0, 0, (uint8_t)duty, 1); chirpAdd(c, 22, 0, 0, 0, (uint8_t)duty, 2); }
    else if (y.on == CS_FRIC) chirpAdd(c, 32, 0, 0, 0, (uint8_t)duty, 2);
    else if (y.on == CS_SOFT) from = p * 0.75f;  // (five under: a slide up)
    // its vowel: a quick slide onto the note, then held; the last one longer, rising
    uint16_t body = (uint16_t)(sylMs * (last ? (win ? 2.2f : 1.6f) : 1.0f) * (y.lng ? 1.7f : 1.0f));
    chirpAdd(c, 20, from, p, vol, (uint8_t)duty, 0);
    chirpAdd(c, body > 20 ? body - 20 : 1, p, last ? p * (win ? 1.12f : 1.09f) : p * 0.96f, vol, (uint8_t)duty, 0);
    // how it ends
    float end = last ? p * (win ? 1.12f : 1.09f) : p * 0.96f;
    if (y.off == CS_PLOSIVE) chirpAdd(c, 10, 0, 0, 0, (uint8_t)duty, 0);
    else if (y.off == CS_FRIC || y.off == CS_AFFRIC) chirpAdd(c, 34, 0, 0, 0, (uint8_t)duty, 2);
    else if (y.off == CS_SOFT) chirpAdd(c, 42, end, end * 0.94f, vol * 0.45f, (uint8_t)duty, 0);
    if (!last) chirpAdd(c, 14, 0, 0, 0, (uint8_t)duty, 0);
  }
}

// ---------------------------------------------------------------- playing it
#define CHIRP_TONE 0.15f   // its loudness: about the effects'
#define CHIRP_NOISE 0.09f

class ChirpPlayer {
 public:
  void begin(float rate) {
    rate_ = rate;
    ease_ = 1.0f - expf(-1.0f / (0.0015f * rate));  // a level settles in 1.5 ms: no clicks between the pieces
  }
  void start(const Chirp &c) {
    c_ = c;
    i_ = 0;
    t_ = 0;
    wait_ = (uint32_t)(c.delayMs * rate_ / 1000.0f);
    on_ = c.n > 0;
  }
  bool active() const { return on_ || tone_ > 1e-4f || noise_ > 1e-4f; }

  float sample() {
    float tv = 0, nv = 0, f = 0;
    uint8_t duty = 2, nk = 0;
    if (on_ && wait_) {
      wait_--;
    } else if (on_) {
      const ChirpSeg &s = c_.seg[i_];
      uint32_t len = (uint32_t)(s.ms * rate_ / 1000.0f);
      float k = len ? (float)t_ / len : 1.0f;
      if (s.f0 > 0) {
        f = s.f0 + (s.f1 - s.f0) * k;
        tv = s.vol * CHIRP_TONE;
      }
      duty = s.duty;
      nk = s.noise;
      if (nk == 1) nv = CHIRP_NOISE * 1.4f * (1 - k) * (1 - k);
      else if (nk == 2) nv = CHIRP_NOISE;
      if (++t_ >= len) {
        t_ = 0;
        if (++i_ >= c_.n) on_ = false;
      }
    }
    tone_ += (tv - tone_) * ease_;
    noise_ += (nv - noise_) * ease_;
    float out = 0;
    if (f > 0) freq_ = f;
    if (tone_ > 1e-5f && freq_ > 0) {
      float dt = freq_ / rate_;
      float dc = duty == 0 ? 0.125f : duty == 1 ? 0.25f : 0.5f;
      float v = ph_ < dc ? 1.0f : -1.0f;
      v += chirpBlep(ph_, dt);
      float t2 = ph_ - dc;
      if (t2 < 0) t2 += 1.0f;
      v -= chirpBlep(t2, dt);
      out += (v - (2 * dc - 1)) * tone_;
      ph_ += dt;
      if (ph_ >= 1.0f) ph_ -= 1.0f;
    }
    if (noise_ > 1e-5f) {
      np_ += (nk == 1 ? 9000.0f : 26000.0f) / rate_;
      while (np_ >= 1.0f) {
        np_ -= 1.0f;
        uint16_t bit = (lfsr_ ^ (lfsr_ >> 1)) & 1;
        lfsr_ = (lfsr_ >> 1) | (bit << 14);
      }
      out += ((lfsr_ & 1) ? 1.0f : -1.0f) * noise_;
    }
    return out;
  }

 private:
  static float chirpBlep(float t, float dt) {
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
  Chirp c_;
  float rate_ = 48000, ease_ = 0.01f;
  uint8_t i_ = 0;
  uint32_t t_ = 0, wait_ = 0;
  bool on_ = false;
  float tone_ = 0, noise_ = 0, freq_ = 0, ph_ = 0, np_ = 0;
  uint16_t lfsr_ = 1;
};
