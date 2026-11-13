#!/bin/zsh
# The icon before Catch 151 (up to 2.1.4): mac/AppIcon.icns from a real frame of the game, rendered without a window
# and with a throwaway save. Since 2.2.0 the apps' icons are Catch 151's (tools/assets/brand/catch151-icon-*.png,
# made by tools/brand_assets.py); running this would put the old one back.
set -e
cd "${0:A:h}/.."
make -C host -j8 >/dev/null
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/res" "$TMP/AppIcon.iconset"
ln -s "$PWD/tools/sdcard/mons" "$TMP/res/mons"
ln -s "$PWD/host/fonts" "$TMP/res/fonts"
clang++ -std=c++17 -O2 -I host mac/icon_shot.cpp build/host/libpokehost.a -o "$TMP/icon_shot"
# midday today, as the game counts it (local time as epoch)
NOON=$(python3 -c "import time; t=time.time()+time.localtime().tm_gmtoff; print(int(t//86400*86400+12*3600))")
# a port of its own: no buddy on the network wanders into the icon
POKEMACGOCHI_BUDDY_PORT=47919 "$TMP/icon_shot" "$TMP/res" "$TMP/save" "$NOON" > "$TMP/frame.rgba"
python3 -I mac/make_icon.py "$TMP/frame.rgba" "$TMP/icon_1024.png"
for s in 16 32 128 256 512; do
  sips -z $s $s "$TMP/icon_1024.png" --out "$TMP/AppIcon.iconset/icon_${s}x${s}.png" >/dev/null
  sips -z $((s * 2)) $((s * 2)) "$TMP/icon_1024.png" --out "$TMP/AppIcon.iconset/icon_${s}x${s}@2x.png" >/dev/null
done
iconutil -c icns "$TMP/AppIcon.iconset" -o mac/AppIcon.icns
cp "$TMP/icon_1024.png" build/mac/icon_1024.png 2>/dev/null || true
echo "OK -> mac/AppIcon.icns"
