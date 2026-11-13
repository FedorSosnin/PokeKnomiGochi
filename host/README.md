# Host layer

Runs the game on something that isn't a board. It is split in two so a future web build only has to replace the window:

- **`host/`** is portable C++ with no AppKit: a 466x466 RGB565 software canvas (the subset of Arduino_GFX the sketch uses, same algorithms), input, clock, sprite loading, save storage and PCM sound. `host.h` is the whole interface a front end sees.
- **`mac/`** is only the window: a round, always-on-top `NSWindow`, mouse and keyboard, and an audio queue.
- **`windows/`** is the same for Windows: a round layered window drawn in software, the notification-area icon, `waveOut` sound and WinHTTP for the relay and updates, cross-compiled with llvm-mingw into one `.exe` that carries the sprite pack and fonts.
- **`webgame/`** is the browser's: a canvas, WebAudio and a WebSocket.

## The game is not forked

`PokeKnomiGochi.ino`, `pet.cpp`, `i18n.cpp` and `sdmon.cpp` are the same files the boards compile. The host builds with `-DBOARD_HOST -DBOARD_KNOMI_V2`: to the sketch it is a Knomi with different hardware underneath, so it gets the Knomi's Games menu, Battle zones and physical button. `host_hal.h` provides the classes `knomi_hal.h` provides on the board.

`BOARD_HOST` appears in the shared code only where a desktop should differ from the Knomi:

| Where | Why |
|---|---|
| `PokeKnomiGochi.ino` includes | Picks `host_hal.h` |
| `sdmon.cpp` includes | Reads sprites from a folder through `host_pak.h` |
| `loop()` frame rate | Caps the idle interval at 150 ms, since there is no battery to protect |
| Clock screen | Only shows the time ("Set automatically"): no setting it, no time zones, and no Wi-Fi settings page, because the system owns all three |
| `tiltZero()` | With arrow keys, straight is always zero |

`rtcbat.cpp`, `audio.cpp`, `wifi_setup.cpp` and `buddy_net.cpp` are not compiled here. `host_board.cpp`, `host_buddy.cpp` and `host_audio.cpp` replace them, and `sfx_notes.h` keeps one table of sound effects for both.

Arduino adds prototypes to a `.ino` before compiling it. `gen_sketch_unit.py` does the same, so the host needs neither arduino-cli nor ctags.

## Time

The game runs in ticks: the front end calls `hostTick()`, one pass of `loop()`, and `delay()` does not wait. Nothing blocks, which is what WebAssembly needs.

`millis()` advances at most 250 ms per tick, so it stops when the process stops.

The game's clock (`host_board.cpp`) is UTC from an anchor, moved along by a monotonic clock that keeps counting while the computer sleeps (`CLOCK_MONOTONIC` on macOS, `GetTickCount64()` on Windows, `performance.now()` in the browser), and shown in the system's time zone. A Mac waking from sleep shows up as a gap between that clock and `millis()`: `clockPoll()` reports `CLOCK_GAP` and the game applies it as time away.

The anchor starts as the system clock and moves to a time server as soon as the front end has an answer. When `hostNetTimeWanted()` says so, the front end sends a HEAD request to `HOST_TIME_URL` (the browser asks its own site) and passes the `Date` header to `hostNetTime()`. Then `clockPoll()` reports `CLOCK_TRUSTED`, and `Pet::trustedTime()` applies whatever the game missed since the last trusted time, measured against the minutes it lived meanwhile (`Pet::lifeMin`). So time away from before the app started comes from the server, never from the system clock, and with no answer nothing is applied until one comes (it asks again every 2 minutes). Changing the system clock changes nothing. If it disagrees with the game's by more than 2 minutes, the server is asked again, at most every 15 minutes, which also catches a browser whose clock stopped while the computer slept. `POKEMACGOCHI_TIME_URL` points a test copy at another server.

## Build and test

```bash
mac/build_app.sh     # build/mac/PokeMacGochi.app
mac/build_dmg.sh     # build/mac/Catch151.dmg (Catch 151.app inside), for a release
mac/make_icon.sh     # regenerates mac/AppIcon.icns from a frame of the game (throwaway save)
```

This produces `build/mac/PokeMacGochi.app`. It needs the Xcode Command Line Tools and python3. There is no App Store build and no signing beyond a local ad hoc signature.

```bash
windows/build.sh     # build/win/PokeWinGochi.exe (llvm-mingw on PATH, or in ~/Tools/llvm-mingw)
```

On Windows the host reads the compressed sprite pack, like the browser, and the few POSIX calls it makes (sockets, `mkdir`, replacing the save file, the time zone) have a Windows branch. `--console` takes the Mac's test commands from stdin, plus `SHOT file.bmp`, which saves the window as drawn; Wine runs it for a quick check on a Mac.

```bash
make -C host test
```

`./test/run_tests.sh` runs these tests too.

## Using the app

The save lives in `~/Library/Application Support/PokeMacGochi` (the game's old name: copies out there find their save under it), separate from any board. A second copy of the app refuses to open on the same save.

| Input | Does |
|---|---|
| Click, hold, swipe | Touch |
| Left and right arrows | Tilt |
| All four arrows | Walk in PokeDungeon |
| Space | The Knomi's button: tap for a wild battle, 1-3 s for screen off, 10 s to ask about erasing the save |
| Drag the Pokéball ring | Move the window (the ring grows under the mouse; the screen is only for touching, with the mouse as a finger) |
| Right-click, or the Pokéball in the menu bar | About, Show/Hide, Size, CRT Screen, Mute Sound (the game's own sound setting) and Quit |
| Cmd-Q | Quit |

For testing, launching with `--console` keeps the app from taking keyboard focus and reads the Knomi's serial commands from stdin (`HEALTH`, `GAMES`, `WILD 25`...). It also takes `MOUSE down|drag|up|right x y` and `KEY down|up code`, which go through AppKit's event queue, `MENU n`, which triggers menu item n, and `HOVER on|off`, which grows or shrinks the ring as the mouse would. `POKEMACGOCHI_SAVE_DIR` points the app at another save folder.

## The screen

The window comes in three sizes, Small, Medium and Large (245, 300 and 478 points across, ring included), and opens Large. With the CRT screen off, the game's 466 × 466 frame is copied in whole pixels, never blurred, so it stays sharp at every size. With it on (the default), `host_crt.cpp` draws the frame as a CRT at the window's own pixels: `hostCrtPrepare()` once a frame (colour, the beam's grid, the glow and the halo), then `hostCrtRows()` in bands of rows on several cores (8 bands at once on a Mac, 4 threads on Windows): each of 233 scanlines a soft beam that swells where the picture is bright, the picture bulging as through curved glass, a reflection and a little grain. The glow fades out as a colour nears white, and a colour past full brightness is scaled down whole, so whites and light greys stay apart and keep their tints. `hostCrtPoint()` maps a click back through the glass, so it lands on what's shown. The browser's page draws the same look as WebGL2 shaders (`webgame/shell.html`). Under the window, a soft drop shadow (`host_shadow.h`) sits in a window of its own that clicks pass through: a child window on the Mac, a layered window on Windows.

## Buddies

`host_buddy.cpp` is `buddy_net.cpp` on BSD sockets: the same 65-byte `BuddyMsg` on UDP `BUDDY_PORT` (47115), sent to the broadcast address of every network the machine is on until the buddy's address is known, then straight to it. The id that the Knomi takes from its MAC is a random odd number kept in `buddy.prefs` in the save folder. The port is shared (`SO_REUSEPORT`), so two copies on one Mac with different saves are buddies too; between those, everything stays broadcast, since a direct packet would reach only one of the two sockets. A WebAssembly build has no UDP and keeps the stub. The tests talk to it over loopback and switch it off before running the game, so `make test` never shows up on a real Knomi. `POKEMACGOCHI_BUDDY_PORT` moves the app to another port: test copies on it see each other and no Knomi.

## Not done yet

- **Web build:** not started. It needs a front end that calls `host.h` and a way to serve `mons/` and `fonts/`.

## Fonts

`fonts/*.u8g2` are the two CJK fonts the firmware uses, cut out of U8g2's 37 MB font file by `extract_fonts.py`. They are loaded at startup. `glcdfont.h` is the classic 5x7 font from Adafruit GFX.
