# Host layer

Runs the game on something that isn't a board. It is split in two so a future web build only has to replace the window:

- **`host/`** is portable C++ with no AppKit: a 466x466 RGB565 software canvas (the subset of Arduino_GFX the sketch uses, same algorithms), input, clock, sprite loading, save storage and PCM sound. `host.h` is the whole interface a front end sees.
- **`mac/`** is only the window: a round, always-on-top `NSWindow`, mouse and keyboard, and an audio queue.

## The game is not forked

`TamaPoke.ino`, `pet.cpp`, `i18n.cpp` and `sdmon.cpp` are the same files the boards compile. The host builds with `-DBOARD_HOST -DBOARD_KNOMI_V2`: to the sketch it is a Knomi with different hardware underneath, so it gets the Knomi's Games menu, Battle zones and physical button. `host_hal.h` provides the classes `knomi_hal.h` provides on the board.

`BOARD_HOST` appears in the shared code only where a desktop should differ from the Knomi:

| Where | Why |
|---|---|
| `TamaPoke.ino` includes | Picks `host_hal.h` |
| `sdmon.cpp` includes | Reads sprites from a folder through `host_pak.h` |
| `loop()` frame rate | Caps the idle interval at 150 ms, since there is no battery to protect |
| Clock screen | No Wi-Fi settings page, because the system owns Wi-Fi |
| `tiltZero()` | With arrow keys, straight is always zero |

`rtcbat.cpp`, `audio.cpp`, `wifi_setup.cpp` and `buddy_net.cpp` are not compiled here. `host_board.cpp` and `host_audio.cpp` replace them, and `sfx_notes.h` keeps one table of sound effects for both.

Arduino adds prototypes to a `.ino` before compiling it. `gen_sketch_unit.py` does the same, so the host needs neither arduino-cli nor ctags.

## Time

The game runs in ticks: the front end calls `hostTick()`, one pass of `loop()`, and `delay()` does not wait. Nothing blocks, which is what WebAssembly needs.

`millis()` advances at most 250 ms per tick, so it stops when the process stops. A Mac waking from sleep shows up as a gap between the system clock and `millis()`. `clockPoll()` reports that gap and the game applies it through `syncClock()`, the same way it applies time spent switched off.

## Build and test

```bash
mac/build_app.sh
```

This produces `build/mac/PokeMacGochi.app`. It needs the Xcode Command Line Tools and python3. There is no App Store build and no signing beyond a local ad hoc signature.

```bash
make -C host test
```

`./test/run_tests.sh` runs these tests too.

## Using the app

The save lives in `~/Library/Application Support/PokeMacGochi`, separate from any board. A second copy of the app refuses to open on the same save.

| Input | Does |
|---|---|
| Click, hold, swipe | Touch |
| Left and right arrows | Tilt |
| Space | The Knomi's button: tap for a wild battle, 1-3 s for screen off, 10 s to ask about erasing the save |
| Drag the Pokéball ring, or Option-drag | Move the window |
| Right-click, or the Pokéball in the menu bar | Show/Hide, Size and Quit |
| Cmd-Q | Quit |

For testing, launching with `--console` keeps the app from taking keyboard focus and reads the Knomi's serial commands from stdin (`HEALTH`, `GAMES`, `WILD 25`...). It also takes `MOUSE down|drag|up|right x y` and `KEY down|up code`, which go through AppKit's event queue, and `MENU n`, which triggers menu item n. `POKEMACGOCHI_SAVE_DIR` points the app at another save folder.

## Not done yet

- **Buddies:** `host_board.cpp` stubs the network, so the game sees no one. UDP from a Mac would reach any real Knomi on the same Wi-Fi, so it should be a deliberate choice.
- **Web build:** not started. It needs a front end that calls `host.h` and a way to serve `mons/` and `fonts/`.

## Fonts

`fonts/*.u8g2` are the two CJK fonts the firmware uses, cut out of U8g2's 37 MB font file by `extract_fonts.py`. They are loaded at startup. `glcdfont.h` is the classic 5x7 font from Adafruit GFX.
