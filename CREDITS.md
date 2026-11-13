# Credits

PokeKnomiGochi is a **non-commercial, personal-use** project. It does not sell or
commercially redistribute any copyrighted material. Pokémon and all related
names, designs and characters are trademarks and © of **Nintendo / Game Freak /
The Pokémon Company**.

This project is not affiliated with or endorsed by any of those companies.

## Sprites and data

| Resource | Source | Use in the project |
|---|---|---|
| **All sprites** (idle, walk, sleep, eat, hurt, attack…) | [PMD Sprite Collaboration (PMDCollab/SpriteCollab)](https://github.com/PMDCollab/SpriteCollab) | Mystery-Dungeon-style animated sprites used everywhere: main screen, stat card, minigame, and the Pokédex grid + detail view |
| **Gen 1 base stats** | [PokéAPI](https://pokeapi.co) | Real ATK/DEF/SPD/HP for each species |

The **SpriteCollab** sprites are the work of its community of artists under their
own terms (Creative Commons Attribution-NonCommercial 4.0). Per-species/per-author
credit is in the original repository's
[tracker.json](https://github.com/PMDCollab/SpriteCollab/blob/master/tracker.json).
Huge thanks to that whole community for an enormous amount of work.

The [Catch 151 Dex](https://fedorsosnin.github.io/PokeKnomiGochi/sprites/) shows every sprite the game uses, regular and shiny, with the artists who made each
one, and says what PokeKnomiGochi changed (16-bit colours, a slower pace, only some views kept, each frame on its
own feet, a few placed by hand) and what it added (the Pokédex's turn, the shiny sparkles and the shadows). It's
built from the packs by `tools/make_sprite_gallery.py`.

> **Important if you reuse this repo:** the packaged sprite files
> (`tools/sdcard/mons/*.bin`) are derived from the sources above. Don't
> redistribute them commercially. If you publish the project, the clean approach
> is to distribute **only the code and scripts**, and have each user download and
> package the sprites from the original sources with `tools/pack_*.py` (or the web
> installer).

## 3D-printed case

| Part | Source |
|---|---|
| **Front half** | [Knomi V2 Creality K1 wall holder / table stand](https://www.printables.com/model/850244-knomi-v2-creality-k1-wall-holder-table-stand) by [Nastra](https://www.printables.com/@Nastra_283431) on Printables |
| **Button and rounded back** | Fedor Sosnin |

The original model is licensed [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/), and this remix is shared under the same license: print, share and remix it with credit, but don't sell it. The changes are the new button and the rounded back.

## Software / hardware

| Component | Author / source |
|---|---|
| GFX Library for Arduino | [moononournation](https://github.com/moononournation/Arduino_GFX) |
| SensorLib (CST9217 touch, PCF85063 RTC) | [Lewis He / lewisxhe](https://github.com/lewisxhe/SensorLib) |
| XPowersLib (AXP2101 PMU) | [Lewis He / lewisxhe](https://github.com/lewisxhe/XPowersLib) |
| U8g2 (CJK font data only; `host/fonts/` holds the two Unifont subsets the firmware uses) | [olikraus](https://github.com/olikraus/u8g2), [GNU Unifont](https://unifoundry.com/unifont/) |
| Classic 5x7 font (`host/glcdfont.h`, BSD license) | [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) |
| Board and pinout | [Waveshare ESP32-S3-Touch-AMOLED-1.75](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75) |
| Board | [BIGTREETECH Knomi V2](https://github.com/bigtreetech/KNOMI) |
| Web installer | [ESP Web Tools](https://esphome.github.io/esp-web-tools/) (Nabu Casa) |
