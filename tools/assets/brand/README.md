# Catch 151: the name and the mark

Pixel art, drawn a pixel at a time by `tools/brand_assets.py` in the game's own colours, so it stays crisp at 1x and at
any whole multiple. Use the SVGs where you can (they scale crisp at any size); show a PNG at its real size or a whole
multiple of it, with `image-rendering: pixelated`.

```bash
~/.venvs/esp/bin/python3 tools/brand_assets.py     # needs Pillow; rewrites this folder, src/brand_logo.h and webgame/assets/logo.svg
```

| File | What |
|---|---|
| `catch151.svg`, `.png`, `@2x`, `@3x` | The game: its icon, then CATCH 151 |
| `catch151-large.*` | The same drawn larger (12 px letters, a 28 px icon), not scaled: the game's About page shows it |
| `catch151-name.*` | The name alone |
| `catch151-dex.*` | Its Pokédex: the sparkle icon, CATCH 151 and a gold DEX sign |
| `catch151-for-knomi.svg`, `-mac`, `-windows`, `-web` (and `@2x.png`) | The game on a device: a wooden sign after the name |
| `catch151-icon.svg`, `-16/32/48/64.png` | The game's icon, each size drawn at its own pixels |
| `catch151-icon-180/192/512/1024.png` | The same for app icons: a design scaled whole (60 x 3, 64 x 3, 64 x 8, 64 x 16) |
| `catch151-dex-icon.*` | The Pokédex's icon, the same sizes |

- **The icon** is the apps' icon frame (a red top, a dark band, a white bottom) round the Knomi's round screen. The game's
  screen shows its island map, the five places round the Wild middle as the Travel page draws them. The Pokédex's shows a
  shiny's sparkle (`drawSparkle()`'s gold and white).
- **The name** is CATCH in cream and 151 in red, in chunky 8 px letters drawn for it (12 px in the large one), with one
  dark outline and a shadow.
- **In the game and on the web:** `src/brand_logo.h` is the large logo for the About page (Settings' last page), drawn
  2 px a pixel so it lands whole on the Knomi's 240 px panel (3x would blur there). `webgame/assets/logo.svg` is the
  regular one, at the top of the web game's page (`webgame/shell.html`).
- **The signs** are the game's wooden name board (`v2SignIn()`) in its 5x7 font: gold for DEX, wood for a device.
- The 1024 px icon is the art alone. A Mac app icon still needs Apple's margin round it (`mac/make_icon.py` does that
  for today's icon).
