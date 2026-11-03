// Renders the game's home screen with Gengar, without a window, for the app icon (see make_icon.sh). Uses a
// throwaway save: never the real one.
//   icon_shot <resources> <save dir> <midday epoch> > frame.rgba
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include "host.h"

void randomSeed(unsigned long seed);  // host_runtime.cpp

static void run(int ms) {
  for (int i = 0; i < ms / 10; i++) {
    hostTick();
    usleep(10000);
  }
}

int main(int argc, char **argv) {
  if (argc != 4) return 2;
  freopen("/dev/null", "w", stderr);
  // the game talks on stdout (serial console): keep the real stdout for the image
  int img = dup(1);
  freopen("/dev/null", "w", stdout);
  hostInit(argv[1], argv[2]);
  randomSeed(1);  // the same pose every time: the pet facing front, centred (make_icon.py crops on it)
  run(300);
  hostPointer(233, 145, true);  // Bulbasaur, the first starter
  hostPointer(233, 145, false);
  run(600);
  hostSerialPush("HATCH");
  run(300);
  hostSerialPush("NOSAVE");
  hostSerialPush("SPEC 94");  // Gengar: the face of the app
  hostSerialPush("AWAKE");
  char line[32];
  snprintf(line, sizeof line, "TIME %s", argv[3]);  // daylight scene
  hostSerialPush(line);
  run(8800);  // past the medal banner that becoming a final form brings; with randomSeed(1), Gengar
              // then faces front in the middle for a while
  // keep the frame with his tongue out: the most tongue red around his mouth
  std::vector<uint8_t> rgba(HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4), px(rgba.size());
  int best = -1;
  for (int f = 0; f < 40; f++) {
    run(40);
    hostFrameRGBA(px.data());
    int red = 0;
    for (int y = 230; y < 320; y++)
      for (int x = 190; x < 276; x++) {
        const uint8_t *p = &px[(y * HOST_CANVAS_SIZE + x) * 4];
        red += p[0] > 180 && p[1] < 90 && p[2] < 90;
      }
    if (red > best) { best = red; rgba = px; }
  }
  return write(img, rgba.data(), rgba.size()) == (ssize_t)rgba.size() ? 0 : 1;
}
