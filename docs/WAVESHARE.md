# Waveshare ESP32-S3-Touch-AMOLED-1.75

PokeKnomiGochi is made for the BTT Knomi V2, but the code also builds for the Waveshare
ESP32-S3-Touch-AMOLED-1.75: a bigger round screen (466×466 AMOLED) with a microSD slot, a battery connector, a
real-time clock and a speaker output. There's no web installer for it yet, and this build hasn't been tried on
the board itself, so treat it as a work in progress.

## The board

- The [ESP32-S3-Touch-AMOLED-1.75](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75): the standard
  version, or the -G (it has GPS, which the game doesn't use). Not the -B, which comes in a case of its own.
- A microSD card for the sprites. Any small card works.
- Optional: a protected 3.7 V LiPo with an MX1.25 plug (check the polarity against the board first), and a small
  speaker.

On the board: a CO5300 display over QSPI, CST9217 touch, an AXP2101 power chip (the battery and the PWR button),
a PCF85063 clock and an ES8311 audio codec. The pins are in `pin_config.h`.

## Build and flash

Set up arduino-cli, the ESP32 core and the libraries as in "Build it yourself" in the [README](../README.md),
then:

```bash
FQBN="esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"
arduino-cli compile --fqbn "$FQBN" .
arduino-cli upload -p /dev/cu.usbmodemXXXX --fqbn "$FQBN" .
```

The OPI PSRAM has to be on: the screen's 466×466 frame lives there.

## Sprites

The sprites go on the microSD. The firmware takes them over USB, so the card can stay in the board (it formats
the card if it needs to):

```bash
python3 tools/pack_pmd.py      # fetches and packs the PMD sprites into tools/sdcard/mons/
python3 tools/make_thumbs.py   # the Pokédex thumbnails, tools/sdcard/mons/thumbs.bin
python3 tools/send_sd.py       # sends tools/sdcard/mons/ to the board's microSD over USB
```

The sprites come from [PMD SpriteCollab](https://github.com/PMDCollab/SpriteCollab), under CC BY-NC 4.0: see
[`CREDITS.md`](../CREDITS.md).
