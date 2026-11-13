# Music (V2: the Mac, Windows and the web)

Background music for every screen: the home, the menus, Travel, Knomi Tower, a battle theme for each zone, the
legends, the tower's guardians, a buddy, the six minigames, evolving and a goodbye. All of it is original, written
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
| Home by day | Home: fades out after 2 minutes untouched, back on the next tap |
| Home from dusk to dawn, the pet awake | Night: Home's tune, slower, on a bell |
| The pet asleep | nothing |
| Card, Pokédex, trophies, settings, buddies, games menu, trade | Menu |
| Knomi Island and the zones' grids | Travel |
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

Leaving the home, the menus, Travel or the tower and coming back within 90 seconds picks the song up where it was.
Otherwise a song starts on its first beat.

## The switches

- **SOUND** (the settings page, Mute Sound in the Mac and Windows menus, the web's Mute) is the master switch. Off
  means silence, music included.
- **Music** has its own row on the settings page under Sound, saved as `mus`, on by default. The Mac and Windows
  menus have a **Music** item and the web page a **Music** button: they all work the same switch. With it off, the
  effects still play.
- A hidden window or tab fades the music out until it's shown again.

## The sound

`src/music_synth.h` is a four-voice player:
- two pulse waves (12.5%, 25% or 50% duty), band-limited so high notes stay clean;
- a stepped triangle for the bass;
- a drum voice (noise, with a falling tone for the kick and the toms).

Each note follows its instrument's envelope. Held notes get vibrato, slurred notes slide, and a chord can play as a
fast arpeggio on one voice.

`host/host_audio.cpp` mixes the music under the effects at about 8 dB below them. When an effect plays, the music
ducks to 45% within a few milliseconds and comes back over about a quarter of a second, so the effects stay clear.

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
Mac app with its own save and buddy port. In its Terminal window, `MUSIC n` plays track n whatever the screen, and
`MUSIC` alone gives the music back to the screens.
