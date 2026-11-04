#!/bin/zsh
# Builds PokeWinGochi.exe in build/win/: one file, the game's sprites and fonts inside. Cross-compiles
# with llvm-mingw (https://github.com/mstorsjo/llvm-mingw), from a Mac or from Linux in CI:
#   windows/build.sh                       (llvm-mingw's bin/ on PATH, or in ~/Tools/llvm-mingw)
# zlib's source (for the compressed sprite pack): $ZLIB_DIR, Emscripten's copy, or downloaded once.
set -e
cd "${0:A:h}/.."
[[ -d $HOME/Tools/llvm-mingw/bin ]] && PATH=$HOME/Tools/llvm-mingw/bin:$PATH
T=x86_64-w64-mingw32
B=build/win
mkdir -p $B/obj $B/res

if [[ -z $ZLIB_DIR ]]; then
  for d in $HOME/emsdk/upstream/emscripten/cache/ports/zlib/zlib-* $B/zlib-1.3.1; do
    [[ -f $d/inflate.c ]] && ZLIB_DIR=$d
  done
fi
if [[ -z $ZLIB_DIR ]]; then
  curl -sSL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz | tar xz -C $B
  ZLIB_DIR=$B/zlib-1.3.1
fi

python3 -I tools/pack_flash.py $B/res/sprites.tpkz >/dev/null
python3 -I host/gen_sketch_unit.py PokeKnomiGochi.ino $B/sketch_unit.cpp

FLAGS=(-O2 -DBOARD_HOST -DBOARD_KNOMI_V2 -I host -I host/shim -I . -I $ZLIB_DIR)
SRC=(host/host_canvas.cpp host/host_prefs.cpp host/host_runtime.cpp host/host_board.cpp host/host_buddy.cpp
     host/host_audio.cpp pet.cpp i18n.cpp sdmon.cpp ota_update.cpp $B/sketch_unit.cpp windows/main.cpp)
OBJ=()
for f in $SRC; do
  o=$B/obj/${f:t:r}.o
  $T-clang++ -std=c++17 $FLAGS -Wno-switch -c $f -o $o &
  OBJ+=$o
done
for f in adler32 crc32 inflate inftrees inffast zutil uncompr; do
  $T-clang -O2 -c $ZLIB_DIR/$f.c -o $B/obj/z_$f.o &
  OBJ+=$B/obj/z_$f.o
done
wait
$T-windres -I windows windows/app.rc -O coff -o $B/obj/app_res.o
# -static: no DLLs to ship beside it; -mwindows: no console window
$T-clang++ -static -mwindows -municode -o $B/PokeWinGochi.exe $OBJ $B/obj/app_res.o \
  -lws2_32 -liphlpapi -lwinhttp -lwinmm -lcomctl32 -lshell32 -lole32 -luuid -lshcore
$T-strip $B/PokeWinGochi.exe
echo "OK -> $B/PokeWinGochi.exe ($(du -h $B/PokeWinGochi.exe | cut -f1))"
