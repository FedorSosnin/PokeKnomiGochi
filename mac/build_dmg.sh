#!/bin/zsh
# Packs build/mac/PokeMacGochi.dmg: the app, a shortcut to Applications and how to open it the first time.
# The app is only signed ad hoc (no Apple Developer account), so Gatekeeper asks once.
set -e
cd "${0:A:h}/.."
mac/build_app.sh >/dev/null

STAGE=build/mac/dmg
DMG=build/mac/PokeMacGochi.dmg
rm -rf "$STAGE" "$DMG"
mkdir -p "$STAGE"
cp -R build/mac/PokeMacGochi.app "$STAGE/"
ln -s /Applications "$STAGE/Applications"
cat > "$STAGE/How to open.txt" <<'TXT'
PokeMacGochi: a Pokemon virtual pet for your Mac desktop

1. Drag PokeMacGochi into the Applications folder.
2. Open it once. macOS says it can't check it for malware, because the app isn't
   signed by an Apple developer account. Click Done.
3. Open System Settings > Privacy & Security, scroll down and click "Open Anyway"
   next to PokeMacGochi, then confirm. From then on it opens normally.

   Or, in Terminal:  xattr -dr com.apple.quarantine /Applications/PokeMacGochi.app

Playing
- Click, hold and swipe on the screen like a touch screen.
- Left and right arrows tilt (Berry Drop, throwing Poke Balls).
- Space is the button: a wild battle.
- Drag the Pokeball ring to move the window. Right-click it, or use the Pokeball
  in the menu bar, for About, Size, Mute Sound and Quit.

Buddies
- Knomis and other Macs on the same Wi-Fi show up as buddies you can battle and
  trade with. Click Allow when macOS asks about the local network. If you clicked
  Don't Allow, turn it on in System Settings > Privacy & Security > Local Network.

Your pet is saved on this Mac, separate from any Knomi.
https://github.com/FedorSosnin/PokeKnomiGochi
TXT
hdiutil create -quiet -volname PokeMacGochi -srcfolder "$STAGE" -fs HFS+ -format UDZO -ov "$DMG"
rm -rf "$STAGE"
echo "OK -> $DMG ($(du -h "$DMG" | cut -f1))"
