#pragma once
// V2: background music. Each pass of the loop the game says which track its screen wants (music_v2.h picks it), and
// music_synth.h plays it under the effects, faded from one to the next. Only on a computer and the web: the Knomi has
// no speaker, and the Waveshare's audio task plays one effect at a time, so on the boards there is no music code and
// no song in the firmware (HAS_MUSIC 0). The songs are written in tools/music/songs.py, which makes music_data.h.
#include <stdint.h>

enum MusicTrack : uint8_t {
  MUS_NONE,
  MUS_HELLO,   // choosing a starter, picking an egg
  MUS_HOME,    // the home by day
  MUS_NIGHT,   // ...in the evening and at night, the pet awake
  MUS_MENU,    // the card, the Pokedex, the trophies, settings, buddies, the games menu
  MUS_TRAVEL,  // Knomi Island and the zones' grids
  MUS_TOWER,   // Knomi Tower, outside and between its floors
  MUS_FOREST,  // a battle in each zone, in zones.h's order (MUS_FOREST + zone)
  MUS_BEACH,
  MUS_CAVE,
  MUS_MANSION,
  MUS_FIELD,
  MUS_LEGEND,       // a legendary one: the zones' and the tower's Legend
  MUS_TOWER_FIGHT,  // a tower floor's guardian
  MUS_BUDDY,        // a battle with a buddy
  MUS_VICTORY,      // a battle won, a catch: once, then quiet until the screen changes
  MUS_BERRY,        // the minigames
  MUS_RUN,
  MUS_SURF,
  MUS_DUNGEON,
  MUS_QUIZ,     // Who's That? and Showdown
  MUS_VOLLEY,
  MUS_EVOLVE,   // while it evolves
  MUS_GOODBYE,  // a goodbye
  MUS_COUNT
};

#ifdef BOARD_HOST
#define HAS_MUSIC 1
void musicWant(uint8_t track);  // the track this screen wants: what plays follows it (host_audio.cpp)
bool musicEnabled();            // the Music switch, saved as mus, on by default (SOUND off silences it too)
void musicSetEnabled(bool on);
#else
#define HAS_MUSIC 0
#endif
