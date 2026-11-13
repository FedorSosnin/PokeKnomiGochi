#!/bin/zsh
# Knomi Tower's test copy of Catch 151 for Mac, apart from the real app: its own save folder, its own buddy port (it never
# visits the real pets), no auto-update, and another bundle id. It opens on Travel with a Pokemon ready to climb.
#   tools/tower_test.sh [strong|new|master]     strong: a stacked Lv 100 Charizard (DEF 25) and 3-star friends (the default)
#                                        new: a Lv 15 Charmander with a few friends, to see where a new team stops
#                                        master: the strong team with the tower already beaten (its challenges)
# The test state isn't saved (NOSAVE): each launch starts the same. The Terminal window takes console commands too
# (TOWERBEST n, TOWERSTEP n, ZONES, TOWER...). Close the app with its menu (the Poke Ball) or Cmd-Q.
set -e
cd "${0:A:h}/.."
SRC=build/mac/PokeMacGochi.app
APP=build/mac/TowerTest.app
if [[ ! -d $SRC ]]; then mac/build_app.sh; fi
if [[ ! -d $APP || $SRC/Contents/MacOS/PokeMacGochi -nt $APP/Contents/MacOS/PokeMacGochi ]]; then
  rm -rf $APP && cp -R $SRC $APP
  /usr/libexec/PlistBuddy -c 'Set :CFBundleIdentifier tv.dsrp.pokemacgochi.towertest' $APP/Contents/Info.plist
  /usr/libexec/PlistBuddy -c 'Set :CFBundleName Tower Test' $APP/Contents/Info.plist
  codesign --force --deep --sign - $APP 2>/dev/null
fi
SAVE="$HOME/Library/Application Support/PokeMacGochi-TowerTest"
mkdir -p "$SAVE"
seed() {
  echo NOSAVE
  if [[ "$1" == new ]]; then
    echo "TOWERPET 4 15 30 2 20"
    for d in 7 25 1 16; do echo "CATCH $d"; done
  else
    echo "TOWERPET 6 100 100 25 100"  # stacked: the 1-in-4 team (DEF 25, a daily player's)
    for d in 9 26 135 149 94 65 131 130 3 25 59 112; do echo "CATCH $d"; echo "STARS $d 3"; done
  fi
  [[ "$1" == master ]] && echo "TOWERBEST 9"
  echo ZONES
}
{ sleep 2; seed "$1"; cat; } | POKEMACGOCHI_SAVE_DIR="$SAVE" POKEMACGOCHI_BUDDY_PORT=47391 $APP/Contents/MacOS/PokeMacGochi --console
