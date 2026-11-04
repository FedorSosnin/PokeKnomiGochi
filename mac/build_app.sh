#!/bin/zsh
# Builds PokeMacGochi.app in build/mac/. It only needs Xcode's Command Line Tools and python3.
#   mac/build_app.sh           builds
#   mac/build_app.sh --open    and opens it
set -e
cd "${0:A:h}/.."

# The oldest macOS it runs on, the same as LSMinimumSystemVersion in mac/Info.plist. Without it, clang
# targets the Mac it builds on, and older ones refuse to open the app.
export MACOSX_DEPLOYMENT_TARGET=$(/usr/libexec/PlistBuddy -c 'Print :LSMinimumSystemVersion' mac/Info.plist)
# The host library must be built for the same target: rebuild it if it was built for another one.
STAMP=build/host/.macos-target
if [[ "$(cat $STAMP 2>/dev/null)" != "$MACOSX_DEPLOYMENT_TARGET" ]]; then
  make -C host clean >/dev/null
  mkdir -p build/host && echo "$MACOSX_DEPLOYMENT_TARGET" > $STAMP
fi
make -C host -j8 >/dev/null

APP=build/mac/PokeMacGochi.app
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources/fonts"
clang++ -std=c++17 -fobjc-arc -O2 -Wall -Wextra -Wno-unused-parameter -Wunguarded-availability \
  -I host mac/main.mm build/host/libpokehost.a \
  -framework AppKit -framework AudioToolbox -framework QuartzCore \
  -o "$APP/Contents/MacOS/PokeMacGochi"
cp mac/Info.plist "$APP/Contents/Info.plist"
cp mac/AppIcon.icns "$APP/Contents/Resources/AppIcon.icns"
cp host/fonts/*.u8g2 "$APP/Contents/Resources/fonts/"
rsync -a --delete --include='*.bin' --exclude='*' tools/sdcard/mons/ "$APP/Contents/Resources/mons/"

# Local (ad hoc) signing: without it, an Apple silicon Mac will not run it. Not for distribution.
codesign --force --deep --sign - "$APP"

echo "OK -> $APP ($(du -sh "$APP" | cut -f1))"
[[ "$1" == "--open" ]] && open "$APP"
exit 0
