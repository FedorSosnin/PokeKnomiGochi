#!/bin/bash
# Regenerates web/knomi/firmware/ (firmware + sprites) for the Knomi web installer.
# Usage: bash tools/build_knomi_web.sh   (the project folder must be named PokeKnomiGochi)
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
cp "$B/PokeKnomiGochi.ino.bootloader.bin" "$OUT/bootloader.bin"
cp "$B/PokeKnomiGochi.ino.partitions.bin" "$OUT/partitions.bin"
DATA=$(arduino-cli config get directories.data)
cp "$(ls "$DATA"/packages/esp32/hardware/esp32/*/tools/partitions/boot_app0.bin | tail -1)" "$OUT/boot_app0.bin"
cp "$B/PokeKnomiGochi.ino.bin" "$OUT/firmware.bin"
cp "$B/sprites.tpkz" "$OUT/sprites.bin"
du -ch "$OUT"/*.bin | tail -1

# what Knomis read for Wi-Fi updates (ota.h): the version and the sprites' size and sha256,
# so a Knomi only downloads the sprites again when they changed
VER=$(sed -n 's/^#define FW_VERSION "\(.*\)"/\1/p' PokeKnomiGochi.ino)
SIZE=$(wc -c < "$OUT/sprites.bin" | tr -d ' ')
SHA=$(shasum -a 256 "$OUT/sprites.bin" | cut -d' ' -f1)
printf '{\n  "version": "%s",\n  "sprites_size": %s,\n  "sprites_sha256": "%s"\n}\n' "$VER" "$SIZE" "$SHA" > web/knomi/update.json
sed -i.bak "s/\"version\": \"[^\"]*\"/\"version\": \"$VER-knomi\"/" web/knomi/manifest.json && rm web/knomi/manifest.json.bak
echo "update.json: v$VER"
