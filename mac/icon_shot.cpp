// Renders the game's home screen without a window, for the app icon (see make_icon.sh). Uses a
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
  char line[32];
  snprintf(line, sizeof line, "TIME %s", argv[3]);  // daylight scene
  hostSerialPush(line);
  run(2500);
  std::vector<uint8_t> rgba(HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4);
  hostFrameRGBA(rgba.data());
  return write(img, rgba.data(), rgba.size()) == (ssize_t)rgba.size() ? 0 : 1;
}
