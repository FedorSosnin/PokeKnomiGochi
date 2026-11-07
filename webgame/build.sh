#!/bin/zsh
# Builds PokeWebGochi into web/play/ (published with the web installers by GitHub Pages):
# the game compiled to WebAssembly with the host layer, the Knomi's compressed sprite pack
# (1.2 MB instead of the 40 MB folder) and the CJK fonts. Needs Emscripten (brew install emscripten).
#   webgame/build.sh
#   UI_V2=0 WEB_OUT=/some/folder webgame/build.sh    the other look, somewhere that isn't published
set -e
cd "${0:A:h}/.."
OUT=${WEB_OUT:-web/play}
B=build/web
rm -rf $B/res
mkdir -p $OUT $B/res/fonts
python3 -I tools/pack_flash.py $B/res/sprites.tpkz >/dev/null
cp host/fonts/*.u8g2 $B/res/fonts/
python3 -I host/gen_sketch_unit.py PokeKnomiGochi.ino $B/sketch_unit.cpp

em++ -std=c++17 -O2 -DBOARD_HOST -DBOARD_KNOMI_V2 ${UI_V2:+-DUI_V2=$UI_V2} -I host -I host/shim -I . \
  webgame/main.cpp \
  host/host_canvas.cpp host/host_prefs.cpp host/host_runtime.cpp host/host_board.cpp host/host_buddy.cpp host/host_audio.cpp \
  pet.cpp i18n.cpp sdmon.cpp ota_update.cpp $B/sketch_unit.cpp \
  -sUSE_ZLIB=1 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=64MB -lidbfs.js \
  -sEXPORTED_FUNCTIONS=_main,_malloc,_webStart,_webFrame,_webPointer,_webHover,_webTilt,_webArrows,_webButton,_webSave,_webSoundOn,_webSetSound,_webAudio,_webAudioRate,_webTiltSensor,_webRelayUp,_webRelayPeers,_webRelayIn,_webRelayUrl,_webRelayV4Url,_webNetTime,_webAutoUpdate,_webSetAutoUpdate,_webQuietForUpdate \
  -sEXPORTED_RUNTIME_METHODS=FS,IDBFS,HEAPU8,HEAP16,UTF8ToString \
  --preload-file $B/res@/res \
  -o $OUT/pokewebgochi.js
# one id for this build: the page asks for its files with it (no mix of old and new from caches),
# and a tab left open compares it with version.json to offer the new version. The page counts too, so
# a change to it alone is a new version
BUILD=$(cat webgame/shell.html $OUT/pokewebgochi.js $OUT/pokewebgochi.wasm $OUT/pokewebgochi.data | shasum | cut -c1-10)
sed "s/__BUILD__/$BUILD/g" webgame/shell.html > $OUT/index.html
printf '{"build":"%s"}\n' "$BUILD" > $OUT/version.json
cp webgame/assets/* $OUT/  # icons, the shared-link preview, the home-screen manifest
du -ch $OUT/* | tail -1
