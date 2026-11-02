#!/bin/zsh
# Builds PokeMacGochi.app in build/mac/. It only needs Xcode's Command Line Tools and python3.
#   mac/build_app.sh           builds
#   mac/build_app.sh --open    and opens it
set -e
cd "${0:A:h}/.."

make -C host -j8 >/dev/null

APP=build/mac/PokeMacGochi.app
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources/fonts"
clang++ -std=c++17 -fobjc-arc -O2 -Wall -Wextra -Wno-unused-parameter \
  -I host mac/main.mm build/host/libpokehost.a \
  -framework AppKit -framework AudioToolbox -framework QuartzCore \
  -o "$APP/Contents/MacOS/PokeMacGochi"
cp mac/Info.plist "$APP/Contents/Info.plist"
cp host/fonts/*.u8g2 "$APP/Contents/Resources/fonts/"
rsync -a --delete --include='*.bin' --exclude='*' tools/sdcard/mons/ "$APP/Contents/Resources/mons/"

# Local (ad hoc) signing: without it, an Apple silicon Mac will not run it. Not for distribution.
codesign --force --sign - "$APP"

echo "OK -> $APP ($(du -sh "$APP" | cut -f1))"
[[ "$1" == "--open" ]] && open "$APP"
exit 0
