// Plays every song (music_data.h) into a WAV file, to listen to them without the game: its intro and its loop twice
// (a jingle once), at the computers' 48 kHz, as music_synth.h plays them in the game. `make -C host music`.
//   music_wav <folder> [name]    only the songs whose track name contains name
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include "music_synth.h"

static const char *const TRACK_IDS[MUS_COUNT] = {
  "none", "hello", "home", "night", "menu", "travel", "tower", "forest", "beach", "cave", "mansion", "field",
  "legend", "tower_fight", "buddy", "victory", "berry", "run", "surf", "dungeon", "quiz", "volley", "evolve", "goodbye",
};

static void put32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void put16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : ".";
  const char *only = argc > 2 ? argv[2] : nullptr;
  const int rate = 48000;
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
    int loopFrom = (int)(s.introTicks * spt);
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
  }
  return 0;
}
