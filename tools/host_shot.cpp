// A headless run of the host build for screenshots and checks: no window, a script of steps, PPM files out.
//   clang++ -std=c++17 -O2 -I host tools/host_shot.cpp build/host/libpokehost.a -framework CoreFoundation -o build/host_shot
//   build/host_shot <resources> <save dir> <script>
// Script lines: WAIT ms | SER <console line> | TAP x y | SWIPE x0 y0 x1 y1 | HOVER x y (-1 -1: off) | SHOT file.ppm
// Run it with POKEMACGOCHI_BUDDY_PORT set to a test port, as every test copy is, so it never meets a real Knomi.
#include "host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

static void run(int ms) {
  for (int t = 0; t < ms; t += 16) {
    hostTick();
    usleep(16000);
  }
}

int main(int argc, char **argv) {
  if (argc < 4) { fprintf(stderr, "usage: host_shot <resources> <save dir> <script>\n"); return 1; }
  hostInit(argv[1], argv[2]);
  run(800);
  FILE *f = fopen(argv[3], "r");
  if (!f) { perror(argv[3]); return 1; }
  char line[512];
  std::vector<uint8_t> px(HOST_CANVAS_SIZE * HOST_CANVAS_SIZE * 4);
  while (fgets(line, sizeof(line), f)) {
    line[strcspn(line, "\r\n")] = 0;
    int a, b, c, d;
    char path[400];
    if (sscanf(line, "WAIT %d", &a) == 1) run(a);
    else if (!strncmp(line, "SER ", 4)) { hostSerialPush(line + 4); run(120); }
    else if (sscanf(line, "TAP %d %d", &a, &b) == 2) {
      hostPointer(a, b, true); run(120);
      hostPointer(a, b, false); run(400);
    } else if (sscanf(line, "HOVER %d %d", &a, &b) == 2) {  // the mouse over the screen, no button (a computer's hover)
      hostPointerHoverAt(a, b); run(200);
    } else if (sscanf(line, "SWIPE %d %d %d %d", &a, &b, &c, &d) == 4) {
      hostPointer(a, b, true); run(60);
      for (int k = 1; k <= 8; k++) { hostPointer(a + (c - a) * k / 8, b + (d - b) * k / 8, true); run(30); }
      hostPointer(c, d, false); run(400);
    } else if (sscanf(line, "SHOT %399s", path) == 1) {
      run(60);
      hostFrameRGBA(px.data());
      FILE *o = fopen(path, "wb");
      fprintf(o, "P6\n%d %d\n255\n", HOST_CANVAS_SIZE, HOST_CANVAS_SIZE);
      for (size_t i = 0; i < px.size(); i += 4) fwrite(&px[i], 1, 3, o);
      fclose(o);
      printf("shot %s\n", path);
    }
  }
  hostFlushSave();
  return 0;
}
