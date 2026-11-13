# Music (V2: the Mac, Windows and the web)

Background music for every screen: the home, the menus, Travel and each of its zones, Knomi Tower, a battle theme
for each zone, the legends, the tower's guardians, a buddy, the six minigames, evolving and a goodbye. All of it is original, written
for this game as chiptune in the same square-wave family as the effects. No tune comes from any other game.

The Knomi has no speaker, so it gets none of it: no music code and no songs go into its firmware (`HAS_MUSIC` is 0),
and its settings page is unchanged. The Waveshare has a speaker, but its audio task plays one effect at a time and
turns the amplifier off between them. Music there would need a streaming mixer and would keep the amplifier on, so
it's left for later.

## What plays where

`src/music_v2.h` picks the track each pass of the loop, in the order `render()` draws the screens:

| Screen | Track |
|---|---|
| Choosing a starter, picking an egg | Hello |
| Home by day | Home, in the manner of the old handhelds' hometowns: fades out after 2 minutes untouched, back on the next tap |
| Home from dusk to dawn, the pet awake | Night: Home's tune, slower, on a bell |
| The pet asleep | nothing |
| Card, Pokédex, trophies, settings, buddies, games menu, trade | Menu |
| Knomi Island | Travel |
| In a zone (its grid and its entries) | The zone's own: Forest, Beach, Cave, Mansion or Field, calmer than its battle |
| Knomi Tower (outside, picking a team, between floors) | Knomi Tower |
| A battle | Its zone's theme: Forest, Beach, Cave, Mansion or Field. A wild one from home or the island uses the zone its Pokémon lives in |
| A legendary Pokémon, the tower's Legend | Legendary Battle |
| A tower floor's guardian | Tower Battle |
| A buddy | Buddy Battle |
| A battle won, a catch, a tower floor won | Victory, once |
| A battle lost, run from, a catch that broke free; the throw itself | nothing (a quick fade) |
| Berry Drop, PokeRun, PikaSurf, PokeDungeon, Volley | Each its own |
| Who's That? and Showdown | Who's That? |
| Evolving | Evolution, once, under the evolve effect |
| A goodbye | Goodbye |

Leaving the home, the menus, Travel, a zone or the tower and coming back within 90 seconds picks the song up where it
was.
Otherwise a song starts on its first beat.

## The switches

- **Sound** and **Music** each have a slider on the settings page, four notches from 0 (off) to 3, its knob showing
  the number (Fedor: "a slider with 4 notches. From 0 to 3"). A tap on a notch moves it there; a finger sliding along
  it moves the choice and it lands where the finger lifts. Sound's is the effects and the cries, Music's the music,
  each on its own (Fedor: "For sound, I want the two options to be split. the music and all other sounds"; until
  2.1.3 Sound's was everything and Music's a share under it). 3 is how they always played, and the default. Saved as `snd` and `sndv`, `mus` and `musv`:
  turned off by Mute, it comes back on at its level.
- **Mute Sound** in the Mac and Windows menus and the web's **Sound** button turn Sound off and on. The menus' **Music**
  submenu (Off, Low, Medium, High) and the web's **Music** button (a step down each click) set the music's.
- With the music off, the effects still play. With Sound off, the music still does.
- A hidden window or tab fades the music out until it's shown again.

## The sound

`src/music_synth.h` is a four-voice player:
- two pulse waves (12.5%, 25% or 50% duty), band-limited so high notes stay clean;
- a stepped triangle for the bass, or a 4-bit wave like the old handhelds' (the home's);
- a drum voice (noise, with a falling tone for the kick and the toms).

Each note follows its instrument's envelope. Held notes get vibrato, slurred notes slide, and a chord can play as a
fast arpeggio on one voice.

`host/host_audio.cpp` mixes the music under the effects at about 8 dB below them. When an effect plays, the music
ducks to 45% within a few milliseconds and comes back over about a quarter of a second, so the effects stay clear.

## Cries

Each Pokémon has its own cry, made the way the old handhelds' sound chip made sounds (`src/cry.h`; Fedor: they
"should be closer to how they sound in pokemon games"). It isn't a word, it's a creature's call:
- it changes in steps, 60 a second: each step the pitch moves on (a sweep, or a trill between two notes), the volume
  steps along its envelope, and the pulse's width turns through a pattern, which makes it buzz and phase;
- two pulse waves sound together, the second a near unison that beats against the first, or a fifth or an octave;
- noise underneath with its own envelope: a growl, a hiss, or the chip's short loop, metallic;
- two to four notes, the last one held and fading;
- a line of evolutions shares one shape, the way a family sounds alike, the later forms lower and longer.

All of it is ours, worked out from the Pokémon itself. No cry is copied or transcribed from any game.

- **What sets it:** size and speed set the pitch. First forms are higher than final ones, small quick Pokémon high,
  big slow ones low, legends lower still. A few are set by hand, such as Jigglypuff high and Snorlax low. The line's
  type sets the rasp and the trill (rock and ground growl, water and ghosts waver, electric ones crackle), and the
  line's first form seeds the shape, so a Pokémon always makes the same cry.
- **When:** petting yours at home (at most every 2.5 seconds; the hearts each time), a little higher and shorter; after
  a battle it wins (in the tower, whoever fought), the whole cry; patting a Pokédex entry's sprite.
- **Where:** a computer and the web, with the effects and at Sound's level.
- **The Shiny Pokédex:** tapping a Pokémon there plays its cry too. `make -C host voices` writes the cries into
  `tools/voices.json` for the page builder, and the host tests check that file matches `cry.h`.

## Writing a song

The songs are text in `tools/music/songs.py`. `tools/music/compose.py` explains the notation:
- MML for the tunes;
- chord symbols a bar;
- patterns that play the bass, the harmony and the drums on the chords.

Then:

```bash
python3 tools/music/songs.py   # writes src/music_data.h; prints any melody note on a strong beat that isn't in its chord
make -C host music             # every song into build/music/*.wav, and build/music/index.html to play them
```

The compiler checks that every bar adds up, that a song's four voices end together, and that every track in
`src/music.h` has a song. `test/test_tools.py` checks that `src/music_data.h` matches the songs.

To hear a track in the game, run `tools/music_test.sh` (or `tools/Music Test.command`). It opens a test copy of the
Mac app with its own save and buddy port. `tools/music_test.sh 2` starts it on Home whatever the hour. In its Terminal
window, `MUSIC n` plays track n whatever the screen, and `MUSIC` alone gives the music back to the screens.
