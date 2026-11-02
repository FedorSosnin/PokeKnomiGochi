#!/bin/zsh
# Builds PokeKnomiGochi for the BTT Knomi V2 and flashes it (firmware + sprites) over USB.
# Does not erase the NVS: keeps the save game and the wifi stored by the previous firmware.
#   tools/knomi_flash.sh [port]          build and flash
#   tools/knomi_flash.sh --build-only
set -e
cd "${0:A:h}/.."
FQBN="esp32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"
OUT=build-knomi
arduino-cli compile --fqbn "$FQBN" --output-dir "$OUT" \
  --build-property "compiler.cpp.extra_flags=-DBOARD_KNOMI_V2" \
  --build-property "compiler.c.extra_flags=-DBOARD_KNOMI_V2" .
python3 -I tools/pack_flash.py "$OUT/sprites.tpkz"
[[ "$1" == "--build-only" ]] && exit 0
PORTS=(/dev/cu.wchusbserial*(N) /dev/cu.usbserial*(N))
PORT="${1:-$PORTS[1]}"
BOOTAPP=$(ls ~/Library/Arduino15/packages/esp32/hardware/esp32/*/tools/partitions/boot_app0.bin | tail -1)
ESPTOOL=$(ls ~/Library/Arduino15/packages/esp32/tools/esptool_py/*/esptool | tail -1)
"$ESPTOOL" --port "$PORT" --baud 230400 write-flash \
  0x0 "$OUT/PokeKnomiGochi.ino.bootloader.bin" \
  0x8000 "$OUT/PokeKnomiGochi.ino.partitions.bin" \
  0xe000 "$BOOTAPP" \
  0x10000 "$OUT/PokeKnomiGochi.ino.bin" \
  0x610000 "$OUT/sprites.tpkz"
