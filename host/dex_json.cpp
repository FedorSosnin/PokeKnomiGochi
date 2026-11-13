// Writes the game's Pokedex, moves and type chart as JSON for the Catch 151 Dex page (dex_json.h).
// `make -C host dex`.
//   dex_json <file>
#include <stdio.h>
#include "dex_json.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: dex_json <file>\n");
    return 2;
  }
  FILE *f = fopen(argv[1], "w");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  std::string s = dexPageJson();
  fwrite(s.data(), 1, s.size(), f);
  fclose(f);
  printf("%s: %d Pokemon, %d moves\n", argv[1], DEX_COUNT, (int)(sizeof(MOVE_TBL) / sizeof(MOVE_TBL[0])));
  return 0;
}
