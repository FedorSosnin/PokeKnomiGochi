// Plays every song (music_data.h) into a WAV file, to listen to them without the game: its intro and its loop twice
// (a jingle once), at the computers' 48 kHz, as music_synth.h plays them in the game, and a page (index.html) that
// plays them in a browser. `make -C host music`.
//   music_wav <folder> [name]    only the songs whose track name contains name
//   music_wav --json <file>      every Pokemon's cry as JSON, for the Catch 151 Dex (voice_json.h)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include "music_synth.h"
#include "cry.h"
#include "voice_json.h"

static const char *const TRACK_IDS[MUS_COUNT] = {
  "none", "hello", "home", "night", "menu", "travel", "tower", "forest", "beach", "cave", "mansion", "field",
  "legend", "tower_fight", "buddy", "victory", "berry", "run", "surf", "dungeon", "quiz", "volley", "evolve", "goodbye",
  "zone_forest", "zone_beach", "zone_cave", "zone_mansion", "zone_field",
};

// where each plays (music.h)
static const char *const TRACK_WHERE[MUS_COUNT] = {
  "", "Choosing a starter, picking an egg", "Home, by day (fades out after 2 minutes untouched)",
  "Home, dusk to dawn, the pet awake: the same tune, slower", "The card, the Pokedex, trophies, settings, buddies, the games menu",
  "Knomi Island", "Knomi Tower, outside and between floors", "A battle in the Forest",
  "A battle at the Beach", "A battle in the Cave", "A battle in the Mansion", "A battle in the Field",
  "A legendary Pokemon, and the tower's Legend", "A tower floor's guardian", "A battle with a buddy",
  "A battle won, a catch: once", "Berry Drop", "PokeRun", "PikaSurf", "PokeDungeon", "Who's That? and Showdown",
  "Poke Ball Volley", "While it evolves: once", "A goodbye", "Travel: in the Forest", "Travel: at the Beach",
  "Travel: in the Cave", "Travel: in the Mansion", "Travel: in the Field",
};

static void put32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void put16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }

static bool writeWav(const char *path, const std::vector<float> &buf, int rate) {
  FILE *f = fopen(path, "wb");
  if (!f) {
    perror(path);
    return false;
  }
  int n = (int)buf.size();
  fwrite("RIFF", 1, 4, f);
  put32(f, 36 + n * 2);
  fwrite("WAVEfmt ", 1, 8, f);
  put32(f, 16);
  put16(f, 1);
  put16(f, 1);
  put32(f, rate);
  put32(f, rate * 2);
  put16(f, 2);
  put16(f, 16);
  fwrite("data", 1, 4, f);
  put32(f, n * 2);
  for (int i = 0; i < n; i++) {
    float v = buf[i] * 32767.0f;
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    put16(f, (uint16_t)(int16_t)lrintf(v));
  }
  fclose(f);
  return true;
}

// a Pokemon's cry (cry.h), added to buf
static void cryInto(std::vector<float> &buf, int16_t dex, uint8_t mood, int rate) {
  Cry c;
  cryBuild(c, dex, mood);
  c.delayMs = 0;
  CryPlayer p;
  p.begin((float)rate);
  p.start(c);
  for (int i = 0; i < rate * 4 && p.active(); i++) buf.push_back(p.sample());
  buf.insert(buf.end(), rate / 10, 0.0f);
}

// the cries: a few Pokemon petted and after a win, and all 151 petted, in the Pokedex's order
static void voices(const char *dir, FILE *html, int rate) {
  static const int16_t SOME[] = { 25, 1, 4, 7, 6, 39, 143, 150, 129, 94 };
  if (html) fprintf(html, "<h2>Cries</h2>\n<p>Each Pokemon's own cry, in the old handhelds' sound: petted at home, "
                          "and after a win.</p><table>\n");
  for (int16_t d : SOME) {
    char path[512], low[24];
    snprintf(low, sizeof(low), "%s", DEX_TBL[d].name);
    for (char *q = low; *q; q++) *q = (char)(*q >= 'A' && *q <= 'Z' ? *q + 32 : *q);
    for (int m = 0; m < 2; m++) {
      std::vector<float> buf;
      cryInto(buf, d, m ? VOICE_WIN : VOICE_PET, rate);
      snprintf(path, sizeof(path), "%s/voice_%03d_%s_%s.wav", dir, d, low, m ? "win" : "pet");
      writeWav(path, buf, rate);
    }
    if (html)
      fprintf(html, "<tr><td>#%d</td><td><b>%s</b></td><td>petted <audio controls preload=none src=\"voice_%03d_%s_pet.wav\">"
                    "</audio></td><td>won <audio controls preload=none src=\"voice_%03d_%s_win.wav\"></audio></td></tr>\n",
              d, DEX_TBL[d].name, d, low, d, low);
  }
  std::vector<float> all;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    cryInto(all, d, VOICE_PET, rate);
    all.insert(all.end(), rate / 4, 0.0f);
  }
  char path[512];
  snprintf(path, sizeof(path), "%s/voices_all.wav", dir);
  writeWav(path, all, rate);
  if (html)
    fprintf(html, "<tr><td></td><td><b>All 151</b></td><td colspan=2>petted, in the Pokedex's order (%.0f s) <audio controls "
                  "preload=none src=\"voices_all.wav\"></audio></td></tr>\n</table>\n", all.size() / (double)rate);
  printf("cries: %zu files, all 151 in %.0f s\n", sizeof(SOME) / sizeof(SOME[0]) * 2 + 1, all.size() / (double)rate);
}

int main(int argc, char **argv) {
  if (argc > 2 && !strcmp(argv[1], "--json")) {
    FILE *f = fopen(argv[2], "w");
    if (!f) {
      perror(argv[2]);
      return 1;
    }
    fputs(cryVoicesJson().c_str(), f);
    fclose(f);
    return 0;
  }
  const char *dir = argc > 1 ? argv[1] : ".";
  const char *only = argc > 2 ? argv[2] : nullptr;
  const int rate = 48000;
  char page[512];
  snprintf(page, sizeof(page), "%s/index.html", dir);
  FILE *html = only ? nullptr : fopen(page, "w");
  if (html)
    fprintf(html, "<!doctype html><meta charset=utf-8><title>Catch 151 music</title>\n"
                  "<style>body{font:15px system-ui;margin:24px;background:#16141f;color:#eee}td{padding:6px 10px}"
                  "td:first-child{color:#9a96b4}audio{height:32px}</style>\n<h2>Catch 151 music</h2>\n"
                  "<p>Each song's intro and its loop twice, as the game plays it.</p><table>\n");
  for (int t = 1; t < MUS_COUNT; t++) {
    if (only && !strstr(TRACK_IDS[t], only)) continue;
    const MusicSong &s = MUSIC_SONGS[t - 1];
    double spt = rate * 60.0 / (s.bpm * MS_TICKS_PER_BEAT);
    double secs = (s.introTicks + (s.loops ? 2 : 1) * s.loopTicks) * spt / rate + (s.loops ? 0 : 1.0);
    int n = (int)(secs * rate);
    std::vector<float> buf(n, 0.0f);
    MusicPlayer p;
    p.begin(rate);
    p.want(t);
    p.render(buf.data(), n, 1.0f);
    double peak = 0, sum = 0;
    int loopFrom = s.loops ? (int)(s.introTicks * spt) : 0;  // (a jingle: all of it)
    for (int i = 0; i < n; i++) {
      peak = fmax(peak, fabs(buf[i]));
      if (i >= loopFrom) sum += buf[i] * buf[i];
    }
    double rms = sqrt(sum / (n - loopFrom > 0 ? n - loopFrom : 1));
    char path[512];
    snprintf(path, sizeof(path), "%s/%02d_%s.wav", dir, t, TRACK_IDS[t]);
    FILE *f = fopen(path, "wb");
    if (!f) {
      perror(path);
      return 1;
    }
    fwrite("RIFF", 1, 4, f);
    put32(f, 36 + n * 2);
    fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16);
    put16(f, 1);
    put16(f, 1);
    put32(f, rate);
    put32(f, rate * 2);
    put16(f, 2);
    put16(f, 16);
    fwrite("data", 1, 4, f);
    put32(f, n * 2);
    for (int i = 0; i < n; i++) {
      float v = buf[i] * 32767.0f;
      if (v > 32767) v = 32767;
      if (v < -32768) v = -32768;
      put16(f, (uint16_t)(int16_t)lrintf(v));
    }
    fclose(f);
    printf("%-12s %-16s %6.1f s  peak %.3f  rms %.4f (%.1f dBFS)%s\n", TRACK_IDS[t], s.name, secs, peak, rms,
           20 * log10(rms + 1e-9), peak > 1.0 ? "  CLIPS" : "");
    if (html)
      fprintf(html, "<tr><td>%d</td><td><b>%s</b><br>%s</td><td><audio controls preload=none src=\"%02d_%s.wav\">"
                    "</audio></td></tr>\n", t, s.name, TRACK_WHERE[t], t, TRACK_IDS[t]);
  }
  if (html) fprintf(html, "</table>\n");
  if (!only || strstr("voices", only)) voices(dir, html, rate);
  if (html) fclose(html);
  return 0;
}
