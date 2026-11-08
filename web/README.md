# PokeKnomiGochi web installer

`web/knomi/` is the Knomi's web installer: a page that flashes the game from Chrome or Edge with
[ESP Web Tools](https://esphome.github.io/esp-web-tools/), with no Arduino or drivers. Knomis also download their
Wi-Fi updates from the same place.

## Contents

- `knomi/index.html`: the page.
- `knomi/manifest.json`: ESP Web Tools' config, pointing at the firmware.
- `knomi/firmware/`: the firmware, and the sprites packed for the Knomi's flash.
- `knomi/update.json`: what a Knomi checks for a Wi-Fi update.

`web/play/` holds the last PokeWebGochi build (`webgame/build.sh`), which `tools/publish_web.sh` publishes to its
own site.

## Rebuild

After a firmware change, rebuild `web/knomi/firmware/` with `tools/build_knomi_web.sh`.

## Test locally

ESP Web Tools needs a secure context, `https://` or `http://localhost`:

```bash
cd web/knomi && python3 -m http.server 8000
# open http://localhost:8000 in Chrome or Edge
```

## Deploy (GitHub Pages)

`.github/workflows/pages.yml` publishes `web/knomi/` at https://fedorsosnin.github.io/PokeKnomiGochi/ on every
push to `main` that touches `web/`.

## Limitations

- Desktop Chrome and Edge only: Web Serial isn't in Firefox or Safari.
