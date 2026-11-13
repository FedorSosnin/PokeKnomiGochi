#!/bin/zsh
# Signs Catch 151 for Mac and Windows for their in-app updates (the files keep their old names for the copies out there): builds build/mac/PokeMacGochi.zip from the
# app, then writes PokeMacGochi.zip.sig and PokeWinGochi.exe.sig (ECDSA P-256 over SHA-256, DER),
# and checks both against the public key in update_key.h. Upload the zip and both .sig files with the
# release, next to the DMG and the .exe.
#   zsh tools/sign_update.sh
set -e
cd "$(dirname "$0")/.."
KEY=${POKEKNOMIGOCHI_SIGNING_KEY:-$HOME/.config/pokeknomigochi/update-signing-key.pem}
[ -f "$KEY" ] || { echo "no signing key at $KEY"; exit 1; }
rm -f build/mac/PokeMacGochi.zip
ditto -c -k --keepParent build/mac/PokeMacGochi.app build/mac/PokeMacGochi.zip
PUB=$(mktemp)
openssl ec -in "$KEY" -pubout -out "$PUB" 2>/dev/null
# the key that signs must be the one the apps carry
WANT=$(openssl ec -in "$KEY" -pubout -outform DER 2>/dev/null | tail -c 65 | xxd -p | tr -d '\n')
HAVE=$(grep 'UPDATE_PUBLIC_KEY\[' src/update_key.h | grep -o '0x[0-9a-f][0-9a-f]' | sed 's/0x//' | tr -d '\n')
[ "$WANT" = "$HAVE" ] || { echo "update_key.h does not match the signing key"; rm -f "$PUB"; exit 1; }
for f in build/mac/PokeMacGochi.zip build/win/PokeWinGochi.exe; do
  openssl dgst -sha256 -sign "$KEY" -out "$f.sig" "$f"
  openssl dgst -sha256 -verify "$PUB" -signature "$f.sig" "$f" >/dev/null
  echo "signed $f ($(wc -c < "$f" | tr -d ' ') bytes)"
done
rm -f "$PUB"
