#!/bin/bash
# Regenerates web/knomi/firmware/ (firmware + sprites) for the Knomi web installer.
# Usage: bash tools/build_knomi_web.sh   (the project folder must be named TamaPoke)
set -e
cd "$(dirname "$0")/.."
FQBN="esp32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"
B=build-knomi
OUT=web/knomi/firmware

arduino-cli compile --fqbn "$FQBN" --output-dir "$B" \
  --build-property "compiler.cpp.extra_flags=-DBOARD_KNOMI_V2" \
  --build-property "compiler.c.extra_flags=-DBOARD_KNOMI_V2" .
python3 -I tools/pack_flash.py "$B/sprites.tpkz"

mkdir -p "$OUT"
cp "$B/TamaPoke.ino.bootloader.bin" "$OUT/bootloader.bin"
cp "$B/TamaPoke.ino.partitions.bin" "$OUT/partitions.bin"
DATA=$(arduino-cli config get directories.data)
cp "$(ls "$DATA"/packages/esp32/hardware/esp32/*/tools/partitions/boot_app0.bin | tail -1)" "$OUT/boot_app0.bin"
cp "$B/TamaPoke.ino.bin" "$OUT/firmware.bin"
cp "$B/sprites.tpkz" "$OUT/sprites.bin"
du -ch "$OUT"/*.bin | tail -1
