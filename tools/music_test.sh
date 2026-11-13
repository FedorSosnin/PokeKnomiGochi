#!/bin/zsh
# The music's test copy of PokeMacGochi, apart from the real app: its own save folder, its own buddy port (it never
# visits the real pets), and another bundle id. It opens on the home with a Charizard awake, so the home's music plays
# (by day Home, from dusk to dawn Night). The test state isn't saved (NOSAVE): each launch starts the same.
#   tools/music_test.sh [n]     n: start on track n (2: Home by day, whatever the hour)
# The Terminal window takes console commands:
#   MUSIC n    plays track n whatever the screen (src/music.h: 1 Hello, 2 Home, 3 Night, 4 Menu, 5 Travel, 6 Tower,
#              7-11 the Forest's, Beach's, Cave's, Mansion's and Field's battles, 12 Legend, 13 Tower battle,
#              14 Buddy battle, 15 Victory, 16 Berry Drop, 17 PokeRun, 18 PikaSurf, 19 PokeDungeon, 20 Who's That?,
#              21 Volley, 22 Evolution, 23 Goodbye); MUSIC alone gives the music back to the screens
#   ZONES, GAMES, DUNGEON, RUN, BATTLE, WILD 150, TOWER...   open the screens that have their own
# Close it with its menu (the Poke Ball) or Cmd-Q.
set -e
cd "${0:A:h}/.."
SRC=build/mac/PokeMacGochi.app
APP=build/mac/MusicTest.app
if [[ ! -d $SRC ]]; then mac/build_app.sh; fi
if [[ ! -d $APP || $SRC/Contents/MacOS/PokeMacGochi -nt $APP/Contents/MacOS/PokeMacGochi ]]; then
  rm -rf $APP && cp -R $SRC $APP
  /usr/libexec/PlistBuddy -c 'Set :CFBundleIdentifier tv.dsrp.pokemacgochi.musictest' $APP/Contents/Info.plist
  /usr/libexec/PlistBuddy -c 'Set :CFBundleName Music Test' $APP/Contents/Info.plist
  codesign --force --deep --sign - $APP 2>/dev/null
fi
SAVE="$HOME/Library/Application Support/PokeMacGochi-MusicTest"
mkdir -p "$SAVE"
{ sleep 2; echo NOSAVE; echo "TOWERPET 6 30 60 10 60"; [[ -n "$1" ]] && echo "MUSIC $1"; cat; } |
  POKEMACGOCHI_SAVE_DIR="$SAVE" POKEMACGOCHI_BUDDY_PORT=47391 $APP/Contents/MacOS/PokeMacGochi --console
