#!/bin/zsh
# Packs build/mac/Catch151.dmg: Catch 151.app, a shortcut to Applications and how to open it the first time.
# (The build keeps build/mac/PokeMacGochi.app: the update zip carries it under that name for the copies out
# there, tools/sign_update.sh. The release uploads this DMG as Catch151.dmg and, for apps up to 2.1.4 that
# link to it, as PokeMacGochi.dmg too.)
# The app is only signed ad hoc (no Apple Developer account), so Gatekeeper asks once.
set -e
cd "${0:A:h}/.."
mac/build_app.sh >/dev/null

STAGE=build/mac/dmg
DMG=build/mac/Catch151.dmg
rm -rf "$STAGE" "$DMG"
mkdir -p "$STAGE"
cp -R build/mac/PokeMacGochi.app "$STAGE/Catch 151.app"
ln -s /Applications "$STAGE/Applications"
cat > "$STAGE/How to open.txt" <<'TXT'
Catch 151 for Mac: a Pokemon game for your Mac desktop

1. Drag Catch 151 into the Applications folder. (Had PokeMacGochi? Quit it and
   drag Catch 151 in anyway: it uses the same save. Then delete PokeMacGochi.)
2. Open it once. macOS says it can't check it for malware, because the app isn't
   signed by an Apple developer account. Click Done.
3. Open System Settings > Privacy & Security, scroll down and click "Open Anyway"
   next to Catch 151, then confirm. From then on it opens normally.

   Or, in Terminal:  xattr -dr com.apple.quarantine "/Applications/Catch 151.app"

Playing
- Click, hold and swipe on the screen like a touch screen.
- Left and right arrows tilt (Berry Drop, throwing Poke Balls).
- Space is the button: a wild battle.
- Drag the Pokeball ring to move the window. Right-click it, or use the Pokeball
  in the menu bar, for About, Size, CRT Screen, Mute Sound, Music and Quit.

Buddies
- Knomis and other Macs on the same Wi-Fi show up as buddies you can battle and
  trade with. Click Allow when macOS asks about the local network. If you clicked
  Don't Allow, turn it on in System Settings > Privacy & Security > Local Network.

Your pet is saved on this Mac, separate from any Knomi.
https://github.com/FedorSosnin/Catch151
TXT
hdiutil create -quiet -volname "Catch 151" -srcfolder "$STAGE" -fs HFS+ -format UDZO -ov "$DMG"
rm -rf "$STAGE"
echo "OK -> $DMG ($(du -h "$DMG" | cut -f1))"
