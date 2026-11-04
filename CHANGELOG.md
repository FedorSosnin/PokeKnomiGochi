# Changelog

All notable changes to the firmware. Versions match the number shown at the
bottom of the clock/settings screen (swipe down) and `web/manifest.json`.

Updating from the [web installer](https://socquique.github.io/TamaPoke/web/)
**without** ticking "Erase device" keeps your Pokémon.

## [Unreleased]

- **PokeWinGochi, the game on Windows:** the same round window as the Mac app, for Windows 10 and 11, in one `.exe` with nothing to install. It meets Knomis, Macs and browsers as buddies, lives in the taskbar's notification area, and checks for its own updates.
- **Shinies are rarer:** about 1 wild Pokémon in 100 is shiny now, instead of 1 in 50.

## [1.33] - 2026-11-03

- **Vibration in the browser on Android:** PokeWebGochi buzzes softly with taps, meals, medals, catches and the other sound effects, even with the sound muted. A **Vibration** button under the screen turns it off; it only shows on phones whose browser can vibrate (iPhones can't).
- **No more "Zz" next to the moon** while your Pokémon sleeps: the "Zzz..." under its name already says so.

## [1.32] - 2026-11-03

- **Poké Ball Volley** uses the new Poké Ball from the home screen.
- **The Poké Ball has a round button**, like the real one: a thick black ring around a white center, instead of a square. It's the same ball on the home screen, in catches and in Poké Ball Volley.

## [1.31] - 2026-11-03

- **Wi-Fi updates work with buddies around:** the update check and install failed ("Update failed") while the Knomi was connected to the buddy relay, because two secure connections at once don't fit in its memory. The update now closes the relay first and reconnects it afterwards. Knomis on 1.30 or older need one USB install from the web installer to get this fix.
- **"... wants to tell you" only once:** after you choose to stay together, the goodbye isn't offered again by itself (it used to come back daily and after every restart). A note says how to bring it back: hold your finger on your Pokémon.
- **A bigger ✕:** the close button is a little larger and much easier to hit, especially just beside and below it at the bottom of the round screen.
- **Lag report:** the Knomi logs any pass of its loop longer than 150 ms over USB, with how much went to the network and to drawing, and the slowest in its 5-minute health line.

## [1.30] - 2026-11-03

- **Friends' stats:** on Party Boost, tap a friend to see its stats first: level, HP, ATK, DEF and SPD, and a buddy's Pokédex count. Swipe to its Battle, Trade and Play menu, and the ✕ goes back to Party Boost. The visitor has a stats page too, with its minutes left. Tapping a buddy's Pokémon on the main screen still opens its menu first, with its stats a swipe away. Buddies now send their stats; one on an older version shows "?".
- **Candy refills energy** to full, as well as food and joy.
- **The visitor brings a candy:** a third one on the day it visits ("PIDGEY brought a candy!"), shown on its stats page.
- **Candies left:** a red badge on the food menu's candy says how many are left today.

## [1.29] - 2026-11-03

- **Party Boost:** while a buddy or the daily visitor is around, energy drains half as fast and your Pokémon has +2 ATK, DEF and SPD in wild battles and PokeDungeon. It's the same however many friends are around. The green button at the top now says **PARTY BOOST +2**. Tap it for who's here and what you get, and tap a buddy there for its menu. The Battle Stats page shows the +2.
- **The daily visitor stays an hour,** standing in the row with any buddies. It steps out while yours sleeps and comes back the same day, and remembers its time across restarts. Wild battles are no longer blocked while it visits.

## [1.28] - 2026-11-03

- **Swipes over buddies:** a swipe that starts or ends on a buddy (or its name at the top) stays a swipe, so the card, the Pokédex, the achievements and the clock open as they should. A buddy's name box only shows when your finger rests on it, and a tap still opens its menu.
- **PokeWebGochi:** with the mouse over it, the Poké Ball ring grows to twice its width, like the Mac app's. A change to the page alone now also counts as a new version, so an open tab offers it.

## [1.27] - 2026-11-03

- **Training page:** the card has a new page after the stats, showing ATK, DEF and SPD training (0–100) at full size, with **MAX** when it's full and what trains each one: winning battles trains attack, every 12 hours in a row of good care trains defense, and mini games train speed. Weight has a row too: food adds it, play burns it.
- **Battle Stats:** the stats page is now called Battle Stats.
- **Clearer wins:** a won battle says "ATK TRAINING +6", or "ATK TRAINING MAXED" once there's nothing left to train.
- The README and How it works list what trains each stat.

## [1.26] - 2026-11-03

- **A close button everywhere:** a round gray ✕ at the bottom of every menu and screen closes it: the games menu, the card, the Pokédex, the zones, the clock and settings, the buddy menu, Play together, the egg choice and trades. It replaces "tap: back", "swipe up to go back" and the Back buttons. Swipes still work, and games in play still quit with a swipe up.
- **Achievements get their own screen:** swipe from right to left on the home screen. Every medal, at most 6 to a page in a 2×3 grid so none is cut off, earned ones green with a white check mark.
- **Swipes:** from left to right opens the Pokédex, from right to left the achievements, up the card (now profile, stats and progress) and down the clock.
- **Battle Wins:** the stats page says "Battle Wins", higher up and with a trophy.
- **Caught!** The Poké Ball bursts with light when a catch holds.

## [1.25] - 2026-11-03

- **New look for the home screen:** the four buttons are bigger pixel-art icons without boxes: a Razz Berry, a Poké Ball, a crescent moon and a bubble.
- **New food:** the berries are now a Razz, an Oran and a Pinap Berry, with a new wrapped candy. Each Pokémon still loves the same one as before, and its card now shows that berry's icon once you've found it.
- **Catch rings:** while you aim, rings show around the Pokémon. Land the ball in the small green circle and it always holds, in the bold ring three times in four, in the faint outer ring one time in four. The catch uses the home screen's Poké Ball.
- **Attack effects:** every move shows a little of its type: flames, splashing water, leaves, sparks, ice crystals, bubbles, mind rings, pebbles, gusts or an impact burst.
- **A lost battle earns nothing:** no training and no bond, but your Pokémon is still tired out.
- **Renaming is easy to find:** a pencil sits beside the name on the card, and the name is centered above the Pokémon.
- **Zone Pokédex:** says "Swipe up to go back" at the bottom.
- **Mac:** the ring grows when the mouse is over the window, and only the ring moves it: the whole screen is for touching. Over the screen the mouse is a pointing finger that presses when you click. The app icon is now Gengar.
- **Mac and web:** all four arrow keys walk in PokeDungeon. In the browser, too, the mouse is a finger.

## [1.24] - 2026-11-03

- **Shiny Pokémon in the wild:** about 1 wild battle in 50 brings a shiny, in its official shiny colors with gold sparkles twinkling around it. The battle opens with "Shiny ...!". Beat it and you get Poké Balls even if you already have the regular one.
- **Sparkles:** every shiny twinkles: your own, a buddy's, a visitor, a wild one, and in the Pokédex.
- **Both forms in the Pokédex:** the regular and the shiny form of a species are kept apart. A square with the shiny gets a gold sparkle (and shows the shiny if that's the only one you have), and an entry with both shows them side by side.
- **Trade shinies:** the trade grid shows each form you have as its own square, so a shiny can be traded on its own and the regular one stays. Only the exact form you're raising can't be traded.
- **Zone medals:** a medal for each zone you complete, legendary included: Forest, Beach, Cave, Mansion and Field. They have their own Zones page on the card.
- Every one of the 151 can be caught in the zones; a test now makes sure it stays that way.
- Both players need this version to trade shinies.

## [1.23] - 2026-11-03

- **Poké Ball Volley:** a second game under **Play** with a buddy. Each screen is your half of the court, your Pokémon is the paddle, and a ball over the top of your screen drops in at the top of your buddy's. Tilt or drag to move. Every shot curves, more off the edge of your Pokémon, so standing in the middle doesn't win. The ball speeds up slowly the longer it stays in the air, so a match to 3 points lasts about a minute. Same rewards as Showdown, and it counts for the **Playmate** medal.
- **Who's That? Showdown** now has 10 questions instead of 5.
- **Trades cost a Pokémon:** the one you give is gone from your Pokédex. Your own partner can't be traded, and with only one Pokémon, Trade is greyed out.
- **The catch takes three shakes:** after a hit, the ball rocks three times with a tone and a count to 3. Your Pokémon has a 75% chance to stay in; if it breaks out, it runs away ("... got away!").
- **Buddies while your Pokémon sleeps** stand on both sides again, instead of piling up on the right.
- A buddy's invite to play no longer says it wants to trade.
- Both players need this version for the new games and rules.

## [1.22] - 2026-11-02

- **Play with a buddy:** the buddy menu now has **Play**, next to Battle and Trade, with games only buddies can play together. The first is **Who's That? Showdown**: the same silhouette on both screens at once, the first to tap the right name gets the point, a wrong tap is out for the round, best of 5. Both get joy and bond, the winner a bit more, and the new **Playmate** trainer medal. Both players need this version.

## [1.21] - 2026-11-02

- **Yours stands out:** whenever buddies or a daily visitor are on screen, your Pokémon stands in the middle and a size bigger than everyone else, even next to buddies of the same species. Buddies and visitors stand at the sides.

## [1.20] - 2026-11-02

- **PokeWebGochi:** the game in any browser at https://pokewebgochi.3dprintsos.com, the Knomi screen centered on a black page. It saves in the browser by itself, with **Export save** and **Import save** for backups, plays in one tab at a time, has Small, Medium and Large sizes, and uses a phone's own tilt. It updates itself: a tab left open offers the new version.
- **Buddies through the relay:** browsers can't send the Knomi's local Wi-Fi messages, so a small relay on Cloudflare passes the same messages between devices on the same home network. Knomis and PokeMacGochi join it too, so everything meets everything. It stores nothing, and a device that is alone sends nothing.

## [1.19.1] - 2026-11-02

- The first version delivered over Wi-Fi: no other changes.

## [1.19] - 2026-11-02

- **Buddy parties:** up to 4 buddies (Knomis and PokeMacGochis in any mix) show up at once. With two or more, everyone stands in a row, smaller; tap one to battle or trade with it, and swipe in the buddy menu to switch. New trainer medals: **First buddy** and **Party of 5**.
- **Updates over Wi-Fi (Knomi):** the settings page has a **Check for update** button, and the Knomi also checks once a day while the screen is dimmed. Updates download from the web installer's site into the spare firmware slot, along with the sprites when they changed. The save is never touched, and a new version that fails to start rolls back.
- **Update check (PokeMacGochi):** the Mac app looks for a new version at launch and once a day, tells you once, and has **Check for Updates** in its menu.
- **Public:** the repository and the web installer at https://fedorsosnin.github.io/PokeKnomiGochi/ are now public.
- **Weight matters:** every meal adds a little weight and eating while already full adds more, so overfeeding makes your Pokémon chubby and sluggish. It burns off slowly and faster with games and battles. The Fit medal now needs weight 20 or less.
- **Zone Pokédex:** tap a Pokémon in a battle zone's grid to open its full Pokédex entry.
- **Press and slide:** holding a finger on any button, menu row or key shows it large above your finger, the way the Wi-Fi keyboard does. It only fires when you lift, so if you landed on the wrong one you can slide to the right one first. A quick flick still swipes.
- **Trainer medals:** a new card page with medals that stay with you across every Pokémon you raise: find a shiny, register 10, 50, 100 and all 151 in the Pokédex, and raise a Pokémon to level 100.
- **Stats page:** the card's second page is now called Stats instead of Battle.
- **PokeMacGochi:** the window no longer dims when you leave it alone; only a sleeping pet dims it. Its size and place are kept with your save, and opening it a second time says where the running one is.
- **PokeMacGochi:** the same game as a small round window on a Mac desktop, in a Pokéball ring, with arrow keys for tilt and its own save. Build it with `mac/build_app.sh`; see `host/README.md`.
- **Buddies:** other PokeKnomiGochis on the same Wi-Fi show up on your screen, and you can battle them or trade Pokédex entries. The Knomi stays connected to Wi-Fi to make this work.
- **3D-printed case:** a Poké Ball case for the Knomi, with the print file and recommended colors.
- **Auto-rotating screen (Knomi):** turn it any way you like and the screen, touch and tilt follow. It starts ready for your right hand with the cable going off to the left. It holds still during Tilt Catch and when aiming a Poké Ball.
- **12-hour clock:** the clock now shows the time with AM and PM.
- **Joy is earned:** only the first 3 pets each hour add joy, though it still loves every one. Candy is limited to 2 a day and leaves the menu after that.
- **Tilt Catch feeds your Pokémon:** it now gives +5 food and only costs energy.
- **How it works:** new guide with charts of what everything does.
- **Battle zones (Knomi):** Games > Battle opens five zones to hunt in, with progress and silhouettes of what's missing. Filling a zone brings out its legendary. The Games menu now goes Battle, PokeDungeon, PokeRun, Who's That?, Tilt Catch.
- **PokeDungeon:** explore floor after floor with your own Pokémon. Tap to step or fight, find the stairs, and see how deep you get.
- **Shiny odds:** the next Pokémon has a 25% shiny chance after a goodbye or release.
- **Run-away:** the button now asks first, and offers to take care of the Pokémon instead of letting it go.
- **Next Pokémon:** after a goodbye or release, choose your next Pokémon from your Pokédex. A run-away still gives a random common egg.
- **Moves:** every Pokémon fights with its own level-up moves from Red and Blue, instead of one move per type.
- **Catch results:** each throw ends in a large banner, and a missed ball visibly bounces away.
- **Catching:** winning no longer registers a species; lean to steer a thrown Poké Ball at it.
- **Games menu:** Tilt Catch (replaces the ball game), Who's That Pokémon? and PokeRun. Swipe up from the lower half of the screen to quit any game with nothing earned.
- **Knomi web installer:** `web/knomi/` flashes the firmware and sprites from the browser.
- **PokeRun:** tap to jump rocks and bushes in a forest that keeps getting faster; your best time is saved. The tuning lives in `pokerun.h`, with tests.
- **Daily visitor:** a Pokémon drops by to hang out, then says goodbye.
- **Pokédex info:** tap a registered Pokémon for its type, evolution and a fun fact (English only).
- **Knomi settings page:** swipe left on the clock screen; pick a network and type its password on the device.
- **Wild battles replace the training bag:** turn-based, with the Gen-1 damage formula and type
  chart (one type per species). TACKLE from the start, a type move at level 10. Winning trains
  STRENGTH and registers the rival in the Pokédex. The rules live in `battle.h`, with tests.
- **BTT Knomi V2 support** (`BOARD_KNOMI_V2`): 240x240 panel, sprites in flash, NTP clock, BOOT button (tap: battle, hold 1-3 s: screen, hold 10 s: erase the game, after confirming).

## [1.18] - 2026-10-27

Three contributions from **ViezeVingertjes** (PRs #25, #26 and #27), merged together.

### Added

- **Battery percentage from a LiPo discharge curve** (`batcurve.h`). On battery
  the figure comes from a typical single-cell curve over a smoothed voltage, and
  only goes down while discharging, so the charge-inflated voltage right after
  unplugging does not raise it. On USB or while charging the PMU's own gauge is
  still used.
- **Safe power-off.** When the smoothed voltage stays under 3.30 V for 30 s (and
  not in the first 30 s after boot) the pet is saved and the PMU powers off. The
  PMU is also set to cut at 3.1 V as a backstop. Assumes a 4.2 V LiPo/Li-ion
  cell, like the one the board is designed for.
- **Power policy** (`power.h`): the frame rate follows activity (10 fps in use,
  down to 1 fps when the screen is almost off, 2 fps while the pet sleeps
  untouched), static screens (keyboard, clock, card pages 2-4, starter choice)
  are painted once, the speaker amplifier is off while the pet sleeps or the
  screen is dimmed, and under 20 % battery the brightness is capped and frames
  are slower. About 68 % fewer frames reach the panel over 10 idle minutes.
- **Light sleep** with the screen off or almost off, on battery only. A touch
  wakes it. Never with USB connected, so the serial port for the SD card stays
  available.

### Changed

- Sprites are drawn as runs of equal pixels instead of one rectangle per pixel
  (same output, far fewer draw calls), and `loop()` yields 1 ms on passes that
  do not draw.
- The pet walks by real time, so a lower frame rate does not slow it down.
- Battery is read over I2C every 10 s instead of 2 s when the screen is off.

## [1.17] - 2026-09-18

### Added

- **Korean, the eighth language.** Full UI (85 strings), the three medal name
  lengths and the 151 gen-1 species names, on the same `STRINGS[LANG][StrId]`
  structure as every other language. The text measurement centralised in 1.15
  carries almost all of it; the four byte-based spots a UTF-8 row turned up are
  listed under Fixed.
- `u8g2_font_unifont_t_korean2` for Hangul, selected per language in
  `applyLangFont()` rather than through a single `CJK_FONT`. The Korean added
  uses 296 distinct syllables, all of them in KS X 1001, the 2350-syllable set
  that matches the size of `korean2`; `korean1` holds 478. Coverage was checked
  as membership in KS X 1001 rather than by walking the font table, and the
  ASCII in the strings is assumed present. Not yet checked on the board:
  whether `CJK_SIZE_DIV` suits `korean2` as it does the Japanese subset.

### Fixed

- **The release dialog could cut a Korean name mid-character.**
  `renderRelease()` built into `char q[28]`. `"%s 놓아줄까요?"` is 17 bytes
  before the name, so names of four syllables or more (34 of the 151) did not
  fit, and `snprintf` truncated inside a 3-byte sequence, leaving a tail the
  font cannot decode. Now 48; the longest case needs 33 bytes including the
  terminator.
- **Three places sized and positioned text with `strlen()`**, which counts
  bytes: the stat-card header, the species name under a nickname, and the
  gallery detail header. On a UTF-8 row the centring offset and the auto-shrink
  threshold worked off a byte count rather than a width. All three now go
  through `textW()` / `centerX()`. With the classic font `textW()` returns
  `strlen()*6*size`, the same expression as before, so the six Latin languages
  land on the same coordinate at the same size, checked at every length from 1
  to 25.
- **Three Japanese texts were cut short**, all already in 1.15 and 1.16.
  Their buffers were sized when every string was one byte per character, and
  `snprintf` truncates without a trace. The minigame record read `きろく 2`
  with a best of 219, since `char rec[12]` left room for one digit. The streak
  milestone banner never fit at all: `"%u にちれんぞく！"` passes 20 bytes with a
  single digit, and the cut landed mid-character. The streak line on the stat
  card lost digits from 100 days on. The release dialog fixed above was the
  fourth.
- **Every formatted text is now checked against its buffer, in all eight
  languages.** A new test in `test/test_tools.py` finds each
  `snprintf(buf, sizeof(buf), T(...))` in the sketch, reads the size of `buf`,
  and computes the worst case per language from the argument types (5 digits
  for `%u`/`%d`, 10 for `%lu`, and a declared maximum for each `%s`). It is what
  caught the three above, and it would have caught all four. To keep it passing
  by type rather than by typical value, four more buffers get headroom they had
  not yet needed: the medal count, the next-level line, the evolution countdown
  and the profile line.
- **`tools/test_i18n_formats.py` reads only the first `len(LANGS)` language
  blocks**, and `LANGS` listed six, so later rows were skipped without a
  warning. It now lists all eight, and the docstring notes that it needs to
  stay complete.
- **`test/test_i18n.cpp`'s `LANG_NAME[LANG_COUNT]` had six entries**, fewer
  than `LANG_COUNT`, so every entry past the sixth was null. Four of the tests
  build a label from `LANG_NAME[lang]` on every iteration, not only when a
  check fails, so each run passed a null pointer to `%s`. Now lists all eight.

## [1.16] - 2026-09-09

### Fixed

- **Japanese was silently dropping every `！` and `？`.** 21 exclamation marks
  and 4 question marks — a quarter of the Japanese strings, including the mood
  line on the main screen (`ごきげん！` rendered as `ごきげん`). The cause is that
  `u8g2_font_unifont_t_japanese1` carries kana and kanji but no CJK punctuation
  at all — no `！？。、「」` — and `Arduino_GFX` draws nothing for a glyph it can't
  find, without even advancing the cursor, so the loss is invisible in the
  source and on screen. Switched to `japanese3`, the only U8g2 Japanese font
  that has them. Same unifont family: the metrics header is byte-identical and
  all 226 codepoints the firmware uses have byte-identical bitmaps, so nothing
  moves. Costs 102 KB of flash (705 KB → 810 KB, 22% → 25% of the app
  partition); RAM is unchanged.
- **Nidoran♀ and Nidoran♂ both showed as ニドラン in Japanese**, leaving #29
  and #32 indistinguishable in the Pokédex. No U8g2 unifont subset carries
  ♀ (U+2640) or ♂ (U+2642) — not `japanese*`, `korean*` or `chinese*` — and
  `Arduino_GFX` draws nothing for a glyph it can't find, without even
  advancing the cursor. `gen_names.py` already substituted F and M on the
  Latin path; it now does the same on the UTF-8 one, so the names read
  ニドランF and ニドランM. Convention chosen by @usakomint, who preferred it
  over ニドラン(女)/(男) as closer to a species name and shorter on a small
  round screen.

Both confirmed on hardware by @usakomint, who also — without knowing it —
supplied the evidence for the first one: the missing `！` was visible in a
photo he posted of the v1.15 main screen.

## [1.15] - 2026-09-08

### Added

- **Japanese.** UI, medals and all 151 species names in katakana (フシギダネ,
  ピカチュウ). Selectable from the settings screen like any other language.

### Changed

- Text measurement and cursor positioning are now centralized (`textW()`,
  `centerX()`, `setCur()`, `setSize()`, `printT()`), replacing ~180 sites that
  assumed one byte per character and a fixed 6 px width. Latin languages render
  identically — the equivalence is arithmetic, not approximate. This is what
  makes CJK possible without a per-language branch at every draw site, and
  Chinese and Korean now only need their strings.
- Bar positions on the stat card and the care row are derived from the width of
  the widest translated label instead of fixed coordinates, so they stay aligned
  in any language. With Latin labels the numbers come out identical to before.
- New dependency: **U8g2**, for its CJK font data only.

Japanese font handling, readability and layout were all tuned against real
hardware by @usakomint, who found and diagnosed every issue in the process.

## [1.11] - 2026-09-07

### Fixed

- **The whole loop froze for exactly 1 second when touching the screen.**
  `touch.getPoint()` hung on a sleeping CST9217 for the ESP32 I2C driver's
  default 1000 ms timeout, which `Wire.setTimeOut(50)` doesn't bound because
  SensorLib doesn't route through it. Most visible in the ball minigame, where
  taps come fast and two or three stalls ran together — reported as "freezes for
  2–3 seconds on ball tap". An address-only Wire transaction before the read
  fixes it. Measured on hardware over equivalent play (~470 touch reads): 5
  stalls in 60 s before, 0 after. Reported by @gingsiro.

## [1.10] - 2026-09-07

### Fixed

- TPK2 sprites with zero-length frame durations spun `pmdFrameAt()` forever,
  hanging the render loop. These files arrive over USB, so they aren't trusted
  input: durations are clamped at load time and the loop is bounded regardless.
- `thumbs.bin` was mapped into PSRAM without bounding its size or checking that
  the offsets it contains point inside what was actually read. A truncated file
  (plausible — the transfer takes minutes) could read past the allocation.
- The serial `PUT` handler ignored `f.write()`'s return value, so a full SD card
  still reported `DONE` while leaving a truncated file behind. It also took the
  destination path straight from the serial line, so a crafted `PUT` could write
  anywhere on the card; writes are now confined to `/mons/`.
- Touches just outside the Pokédex grid and the on-screen keyboard were treated
  as cell 0. The guard divided first and then checked for a negative index, but
  integer division truncates toward zero, so it never saw one. On a round screen
  that strip is exactly where a finger lands near the edge.

All four from @ajtudela.

## [1.9] - 2026-09-07

### Fixed

- All ~13 game timers compared `millis()` against an absolute deadline, which
  gives the wrong answer once `millis()` wraps at ~49.7 days of uptime: a timer
  could stay active forever, or a ceremony cut off instantly.
- The sprite transfer restarted from scratch on every attempt. It now records
  each confirmed file, so a reload after a dropped connection doesn't redo the
  full ~40 MB / ~10 min.
- Minigame physics advanced per frame rather than per unit of time, so the ball
  fell measurably slower with a large sprite on screen (Charizard) than a small
  one (Diglett) — high scores weren't comparable between species.

### Security

- `esp-web-tools` was loaded from a floating `@10` range with no integrity
  check. Pinned to 10.4.0 with a verified sha384 SRI hash.

All from @ajtudela.

## [1.8] - 2026-09-07

### Fixed

- **The level counter wrapped to 0 after ~10.6 days of play.** `level()`
  returned `uint8_t` and, at 60 minutes per level, the level is the hour count —
  so it rolled over at 256. Reachable in ordinary play, since "stay together"
  postpones the farewell indefinitely. Now `uint16_t`, capped at 999.
- Two related truncations found while fixing it: `calcStat()` took the level as
  `uint8_t`, so stats would have overflowed anyway; and `canEvolveNow()` compared
  against `(uint8_t)(evolveLevel + careMistakes)`, which wrapped with many care
  mistakes and let evolution happen early. The stat card already computed that
  threshold as `int`, so the screen and the actual check could disagree.

Diagnosed by @ajtudela.

## [1.7] - 2026-09-07

### Fixed

- **The UI had no accents, and the assumption behind that was wrong.** The code
  stated in two places that the font had none. In fact `glcdfont.h` is a full
  256-glyph CP437 table and `write()` doesn't filter bytes ≥ 0x80. Spanish now
  has its tildes and ñ, German its umlauts, French its accents. Characters are
  written as single bytes in octal, never UTF-8, because the UI centres with
  `strlen(t) * 6`. Only strings whose length was unchanged were touched.
  Uppercase accented vowels other than É aren't in CP437, so `EVOLUCION` and
  `PROGRES` stay unaccented, as do Portuguese ã/õ.
- German grammar, from the same contributor: "Wähle deinen Starter" (accusative),
  "Leb wohl" as two words, "Entwicklung" rather than "Entwickelt", and more.

Discovered by @danielberndt.

## [1.6] - 2026-09-07

### Fixed

- **Short sound effects were inaudible.** The amplifier was switched on 8 ms
  before each effect, far less than the NS4150B needs to settle, so a 35 ms tap
  finished before the speaker woke up. The boot jingle survived because it's
  ~440 ms — which is why sound seemed to work at startup and nowhere else. The
  amplifier now stays powered while sound is on and the pet is awake, matching
  Waveshare's reference. Notes and amplitude are unchanged: this doesn't alter
  the sound design, it just makes it audible.

Diagnosed by @djyf1.

## [1.5] - 2026-08-19

### Added

- Localized Pokémon names in French and German (Bulbizarre, Bisasam…). Only
  those two differ in gen 1; Spanish, Italian and Portuguese officially use the
  English names. Names come from PokéAPI via `tools/gen_names.py`.

## [1.4] - 2026-08-07

### Fixed

- **Bond was mathematically stuck.** It could gain at most 12 points a day, but
  a single care mistake cost 3 and could repeat every 30 minutes — up to 144 a
  day. With poops draining hygiene, mistakes chained and bond never recovered.
  Two players independently reported it pinned at exactly 10, which is the
  first-day ceiling of 13 minus one mistake. Daily cap 8 → 20, penalty 3 → 1,
  cooldown 30 → 60 min.

## [1.3] - 2026-08-07

### Fixed

- **The egg hatched by itself during starter selection.** Game time kept running
  while the starter screen was open, so taking more than 3 minutes to choose
  meant the egg hatched into the species already rolled — no egg animation, and
  the player's choice ignored. Three community reports matched (Pikachu twice,
  Eevee once). Time now freezes until a starter is chosen.

## [1.2] - 2026-06

### Added

- Firmware version shown at the bottom of the clock/settings screen.
