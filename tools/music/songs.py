#!/usr/bin/env python3
"""Catch 151's background music (V2, on a computer and the web): every song, written for this game.

  python3 tools/music/songs.py           writes src/music_data.h, and prints what in the songs doesn't fit
  python3 tools/music/songs.py --check   only checks that src/music_data.h is up to date

Then `make -C host music` plays every song into build/music/*.wav, to listen to.

All of it is original: no tune here comes from any other game. compose.py explains the notation.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import compose as C  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# ------------------------------------------------------------------ instruments
# wave, attack ms, decay ms, sustain level, release ms, fade ms, gap ms, vibrato semitones, Hz, delay ms, glide ms
INSTRUMENTS = [
    ('lead',   'MW_P25', 3, 140, 0.65, 70, 0, 14, 0.16, 5.5, 220, 35),   # the tune: a quarter pulse, vibrato on held notes
    ('lead50', 'MW_P50', 4, 160, 0.60, 80, 0, 14, 0.18, 5.0, 240, 35),   # rounder
    ('flute',  'MW_P50', 22, 120, 0.72, 90, 0, 10, 0.22, 5.2, 160, 45),  # soft attack, airy vibrato
    ('thin',   'MW_P12', 3, 150, 0.60, 70, 0, 12, 0.20, 6.0, 180, 30),   # nasal: an eighth pulse
    ('ghost',  'MW_P12', 14, 200, 0.62, 90, 0, 12, 0.32, 6.5, 90, 40),   # wavering
    ('brass',  'MW_P50', 8, 220, 0.72, 60, 0, 12, 0.14, 6.0, 200, 25),   # the battles' tune
    ('pluck',  'MW_P25', 2, 90, 0.34, 45, 0, 8, 0, 0, 0, 0),             # short notes
    ('pluck12', 'MW_P12', 2, 60, 0.18, 40, 0, 6, 0, 0, 0, 0),
    ('bell',   'MW_P12', 2, 500, 0.0, 120, 0, 0, 0, 0, 0, 0),            # struck and dying away
    ('bell25', 'MW_P25', 2, 420, 0.0, 120, 0, 0, 0.06, 4.0, 250, 0),
    ('soft',   'MW_P12', 6, 160, 0.45, 70, 0, 10, 0, 0, 0, 0),           # the harmony under the tune
    ('soft25', 'MW_P25', 6, 180, 0.45, 70, 0, 10, 0, 0, 0, 0),
    ('stab',   'MW_P25', 2, 90, 0.25, 40, 0, 10, 0, 0, 0, 0),            # chord stabs
    ('pad',    'MW_P50', 70, 300, 0.55, 200, 0, 20, 0.08, 4.0, 350, 0),  # held chords, swelling in
    ('pad12',  'MW_P12', 60, 300, 0.55, 200, 0, 20, 0.08, 4.0, 350, 0),
    ('bass',   'MW_TRI', 2, 0, 1.0, 30, 0, 22, 0, 0, 0, 0),              # the triangle, a little gap between notes
    ('bassleg', 'MW_TRI', 2, 0, 1.0, 40, 0, 4, 0, 0, 0, 30),             # ...joined up
    ('basspl', 'MW_TRI', 2, 160, 0.35, 30, 0, 10, 0, 0, 0, 0),           # ...plucked
    # the old handhelds' sound (the home): a half pulse that settles after its attack and wavers once it's held, a
    # quarter pulse picking out the chords under it, and the wave voice's bass, short and bouncy
    ('gblead', 'MW_P50', 2, 260, 0.68, 60, 0, 10, 0.24, 5.6, 260, 0),
    ('gbpick', 'MW_P25', 2, 110, 0.30, 45, 0, 8, 0, 0, 0, 0),
    ('gbharm', 'MW_P25', 2, 240, 0.50, 60, 0, 10, 0.16, 5.6, 300, 0),
    ('gbwave', 'MW_WAVE', 2, 0, 1.0, 25, 0, 26, 0, 0, 0, 0),
]
INST = {name: i for i, (name, *_) in enumerate(INSTRUMENTS)}

# ------------------------------------------------------------------ drums
# letter, name: tone Hz from, to, sweep ms, volume, decay ms; noise Hz, short loop, volume, decay ms
DRUMS = [
    ('k', 'kick',      150, 45, 55, 1.00, 140, 6000, 0, 0.30, 12),
    ('s', 'snare',     230, 160, 40, 0.45, 70, 14000, 0, 0.85, 150),
    ('h', 'hat',       0, 0, 1, 0, 0, 44000, 0, 0.32, 30),
    ('o', 'open hat',  0, 0, 1, 0, 0, 44000, 0, 0.30, 170),
    ('c', 'crash',     0, 0, 1, 0, 0, 30000, 0, 0.50, 800),
    ('t', 'low tom',   190, 110, 120, 0.95, 220, 3000, 0, 0.18, 25),
    ('T', 'high tom',  300, 180, 100, 0.90, 190, 3000, 0, 0.18, 25),
    ('r', 'rim',       0, 0, 1, 0, 0, 22000, 1, 0.40, 22),
    ('S', 'shaker',    0, 0, 1, 0, 0, 30000, 0, 0.20, 50),
    ('w', 'wood',      900, 820, 20, 0.55, 45, 0, 0, 0, 1),
    ('x', 'clap',      0, 0, 1, 0, 0, 9000, 0, 0.70, 110),
]
C.DRUM_KEYS.update({d[0]: i + 1 for i, d in enumerate(DRUMS)})


def song(track, name, bpm, **kw):
    return C.Builder(track, name, bpm, instruments=INST, **kw)


SONGS = []


def add(b):
    SONGS.append(b)
    return b


# ================================================================== the songs
# Drum bars used by several songs (16 steps a bar of four, 12 a bar of three or a shuffle's)
POP = '16: k . h . s . h . k . h k s . h .'
POP_FILL = '16: k . h . s . h . s . s s T . t .'
ROCK = '16: k . h . s . h k k . h . s . h .'
ROCK_FILL = '16: k . h . s . s s T T T . t t s .'
CRASH = '16: c . h . s . h k k . h . s . h .'
DRIVE = '16: k . h k s . h k k . h k s . k h'
DRIVE_FILL = '16: k . h k s . s s s s T T t t s s'
GALLOP = '16: k . h k s . h k k . h k s . h .'
STAB8 = '8: . x . x . x . x'       # the chord on the off-beats
ARP16 = '16: 1 2 3 4 3 2 1 2 1 2 3 4 3 2 1 2'
ARP_UP = '16: 1 2 3 4 1 2 3 4 1 2 3 4 1 2 3 4'

# ------------------------------------------------------------------ Hello: choosing a starter, picking an egg
s = add(song('HELLO', 'Hello', 96, key=('C', 'major'), insts=('flute', 'pluck12', 'bass'), vols=(12, 7, 14, 8)))
HELLO_DR = '16: k . S . r . S . k . S k r . S .'
s.section('I', 2, 'C | G7', None, ARP_UP, '8: R - - 5 - - O -', [HELLO_DR, HELLO_DR])
s.section('A', 8, 'C | G/B | Am | Em/G | F | C/E | Dm7 | G7', """
  o5 g4 e8 g8 o6 c4 o5 b8 a8 | g2 r8 d8 e8 f8 | e4 c8 e8 a4 g8 e8 | g2. r4 |
  a4 f8 a8 o6 c4 o5 a8 f8 | g4 e8 g8 o6 c4. o5 g8 | a4 f8 d8 c4 d8 f8 | d4. o4 b8 g4 r4 |""",
          ARP_UP, '8: R - - 5 - - O -', [HELLO_DR] * 7 + ['16: k . S . r . S . k . r . r r r .'])
s.section('B', 8, 'F | G | Em | Am | Dm7 | G | Csus4 | C', """
  o5 a4. g8 f4 a4 | b4. a8 g4 d4 | g4. f8 e4 g4 | a2 e4 c4 |
  d4 f8 a8 o6 c4 o5 a8 f8 | g4 b8 o6 d8 o5 b4 g8 d8 | f4 e8 f8 g4 f4 | e2. r4 |""",
          'thirds', '4: R - 5 -', [HELLO_DR] * 8, insts=('flute', 'soft', 'bass'), vols=(12, 6, 14, 8))
s.form(['I'], ['A', 'B'])

# ------------------------------------------------------------------ Home: by day, the pet's own tune
# In the manner of the old handhelds' hometowns (an original tune, nothing borrowed): a warm major key, a half pulse
# singing with a late vibrato, a quarter pulse under it picking out the chords then following the tune a third or a
# sixth below, the wave voice's bass bouncing on eighths, only a soft hat for drums; the minor iv and the B7 for the
# wistful turns.
s = add(song('HOME', 'Home', 116, key=('D', 'major'), insts=('gblead', 'gbpick', 'gbwave'), vols=(13, 9, 10, 9),
             bass_lo=38))
HAT = '16: . . h . . . h . . . h . . . h .'
HAT_END = '16: . . h . . . h . . . h . h . h h'
HOME_BASS = '8: R O 5 O R O 5 A'
HOME_PICK = '8: 1 2 3 2 1 2 3 2'
HOME_A = """
  o5 a4. f+8 a8 o6 d8 c+8 d8 | e4 c+8 o5 a8 f+4 a4 | g4 b8 o6 d8 e4 d8 o5 b8 | o6 c+2. o5 a8 b8 |
  o6 d4. c+8 o5 b8 a8 f+8 a8 | g4. f+8 g8 a8 b8 o6 d8 | e4 d8 o5 b8 a4 o6 c+4 | d2. r4 |"""
HOME_A2 = """
  o5 a4. f+8 a8 o6 d8 c+8 d8 | e4 c+8 o5 a8 f+4 a4 | b4. a8 g8 a8 b8 o6 d8 | d4 o5 b-4 g2 |
  a4. f+8 d4 f+4 | f+4 d+8 f+8 a4 b4 | g4 e8 g8 o6 c+4 o5 a4 | d2. r4 |"""
HOME_B = """
  o5 b4. o6 c+8 d4 o5 b4 | o6 c+4. o5 b8 a4 e4 | f+4. g+8 a4 o6 c+4 | d2 o5 b4 f+4 |
  g4. f+8 e4 g4 | c+4 e8 a8 o6 c+4 e4 | d4 o5 b4 b-4 g4 | a4 g4 e4 c+4 |"""
HOME_CH = ('D | F#m7 | G | A | Bm | G | Em7 A7 | D', 'D | F#m7 | G | Gm | D/F# | B7 | Em7 A7 | D',
           'G | A | F#m | Bm | Em7 | A | G Gm | A7')
s.section('I', 2, 'D | A7', None, HOME_PICK, HOME_BASS, [HAT, HAT_END])
s.section('A', 8, HOME_CH[0], HOME_A, HOME_PICK, HOME_BASS, [HAT] * 7 + [HAT_END])
s.section('A2', 8, HOME_CH[1], HOME_A2, 'thirds', HOME_BASS, [HAT] * 7 + [HAT_END],
          insts=('gblead', 'gbharm', 'gbwave'), vols=(13, 8, 10, 9))
s.section('B', 8, HOME_CH[2], HOME_B, 'sixths', '4: R 5 O 5', [HAT] * 7 + [HAT_END],
          insts=('gblead', 'gbharm', 'gbwave'), vols=(13, 8, 10, 9))
s.form(['I'], ['A', 'A2', 'B', 'A2'])

# ------------------------------------------------------------------ Night: the home's tune, slower and softer
s = add(song('NIGHT', 'Night', 84, key=('D', 'major'), insts=('bell25', 'pad12', 'bassleg'), vols=(13, 6, 12, 6),
             bass_lo=38))
NIGHT_DR = '16: . . . . r . . . . . . . r . . .'
s.section('I', 2, 'D | A7', None, '1: x', '2: R 5', None)
s.section('A', 8, HOME_CH[0], HOME_A, '1: x', '2: R 5', [NIGHT_DR])
s.section('A2', 8, HOME_CH[1], HOME_A2, '1: x', '2: R 5', [NIGHT_DR])
s.section('B', 8, HOME_CH[2], HOME_B, 'sixths', '2: R 5', [NIGHT_DR], insts=('bell25', 'soft', 'bassleg'),
          vols=(13, 5, 12, 6))
s.form(['I'], ['A', 'A2', 'B', 'A2'])

# ------------------------------------------------------------------ Menu: the card, the Pokedex, settings, buddies...
s = add(song('MENU', 'Menu', 112, key=('G', 'major'), insts=('lead50', 'stab', 'bass'), vols=(11, 8, 14, 7)))
MENU_DR = '16: k . S . r . S . k . S . r . S S'
CHARL = '8: x . . x . . . .'   # the chord on one and the and of two
s.section('A', 8, 'Gmaj7 | Em7 | Am7 | D7 | Gmaj7 | Em7 | Am7 D7 | G6', """
  o5 r8 b8 o6 d8 o5 b8 a4 g4 | e4. f+8 g4 b4 | o6 c4 o5 b8 a8 e4 g4 | f+2 r4 d8 e8 |
  f+8 g8 b8 o6 d8 o5 b4 a8 g8 | e4 g8 b8 o6 d4 o5 b4 | o6 c4 o5 a8 e8 f+4 a4 | g2. r4 |""",
          CHARL, '4: R 3 5 A', [MENU_DR])
s.section('B', 8, 'Cmaj7 | Bm7 | Am7 | D7 | Cmaj7 | Bm7 | Am7 | D7', """
  o5 e4 g8 b8 o6 c4 o5 b4 | a4. f+8 d4 f+4 | e4. g8 c4 e4 | d4 f+8 a8 o6 c4 o5 a4 |
  g4. e8 g4 b4 | a4. f+8 d4 a4 | g4 e8 c8 e4 g4 | f+4 a4 o6 c4 o5 a4 |""",
          CHARL, '4: R 3 5 A', [MENU_DR])
s.form([], ['A', 'B'])

# ------------------------------------------------------------------ Travel: Knomi Island and the zones
s = add(song('TRAVEL', 'Travel', 126, key=('D', 'major'), insts=('brass', 'stab', 'bass'), vols=(12, 8, 15, 10)))
TRAVEL_H = '8: x . x x . x x .'
TRAVEL_B = '8: R R O R 5 R O R'
s.section('I', 2, 'D | A', None, TRAVEL_H, TRAVEL_B, [ROCK, ROCK_FILL])
s.section('A', 8, 'D | A/C# | Bm | G | Em | A | D/F# G | A', """
  o5 d8 e8 f+8 a8 o6 d4. o5 a8 | o6 c+4 o5 a8 e8 a4 r8 e8 | f+4. e8 d8 c+8 d8 f+8 | g2 b4 a8 g8 |
  e4 g8 b8 o6 e4 d8 o5 b8 | o6 c+4 o5 a4 e4 a4 | f+8 a8 o6 d8 o5 a8 g8 b8 o6 d8 o5 b8 | a4. g8 f+4 e4 |""",
          TRAVEL_H, TRAVEL_B, [CRASH] + [ROCK] * 6 + [ROCK_FILL])
s.section('A2', 8, 'D | A/C# | Bm | G | Em | A | Bm G | A:2 D:2', """
  o5 d8 e8 f+8 a8 o6 d4. o5 a8 | o6 c+4 o5 a8 e8 a4 r8 e8 | f+4. e8 d8 c+8 d8 f+8 | g2 b4 o6 d8 e8 |
  o6 e4. d8 o5 b4 g4 | a4 o6 c+8 o5 a8 e4 c+4 | d4 f+4 g4 b4 | a4 o6 c+4 d2 |""",
          TRAVEL_H, TRAVEL_B, [CRASH] + [ROCK] * 6 + [ROCK_FILL])
s.section('B', 8, 'G | A | F#m | Bm | G | C | A | A7', """
  o5 b4. a8 g4 d4 | c+4. d8 e4 a4 | a4. g8 f+4 c+4 | d4 f+8 b8 o6 d4 o5 b4 |
  o6 d4. c+8 o5 b4 g4 | o6 c4 o5 g8 e8 c4 e4 | e4 a4 o6 c+4 e4 | e4 c+4 o5 a4 g4 |""",
          'thirds', '4: R R 5 5', [POP] * 7 + [ROCK_FILL], insts=('brass', 'soft', 'bass'))
s.form(['I'], ['A', 'A2', 'B', 'A2'])

# ------------------------------------------------------------------ Knomi Tower: outside, and between its floors
s = add(song('TOWER', 'Knomi Tower', 92, key=('D', 'harmonic'), insts=('brass', 'pad', 'bass'), vols=(11, 7, 15, 10)))
TOWER_DR = '16: k . . . . . . . s . . . . . . .'
TOWER_FILL = '16: k . . . . . . . s . . . T . t t'
s.section('I', 2, 'Dm | A', None, '1: x', '4: R R R R', [TOWER_DR, TOWER_FILL])
s.section('A', 8, 'Dm | Bb | Gm | A | Dm | F | Gm | A7', """
  o5 d2 a2 | b-4. a8 g4 f4 | g2. b-4 | a1 |
  d4. e8 f4 a4 | o6 c2 o5 a4 f4 | g4. a8 b-4 o6 d4 | c+2 o5 a4 e4 |""",
          '1: x', '4: R R R R', [TOWER_DR] * 7 + [TOWER_FILL])
s.section('B', 8, 'Bb | C | Am | Dm | Gm | C | A | A7', """
  o5 f2 b-2 | o6 c4. o5 b-8 a4 g4 | a2 e2 | f4. e8 d2 |
  d4 g4 b-4 o6 d4 | e2 c4 o5 g4 | a4. g8 e4 c+4 | e2. r4 |""",
          '2: x x', '4: R R R R', [TOWER_DR] * 7 + [TOWER_FILL])
s.form(['I'], ['A', 'B'])

# ------------------------------------------------------------------ the battles, one a zone
# the Forest: bright, a flute over quick arpeggios
s = add(song('FOREST', 'Forest Battle', 152, key=('E', 'minor'), insts=('flute', 'pluck12', 'bass'),
             vols=(13, 7, 15, 10)))
FOR_B = '8: R R O R R O R O'
s.section('I', 4, 'Em | Em | C | D', None, ARP16, FOR_B, [ROCK, ROCK, ROCK, ROCK_FILL])
FOR_A = """
  o5 e4 b4 g8 a8 b8 g8 | g4. e8 c4 e4 | f+4 a4 d4. e8 | e2 r8 b8 a8 g8 |
  a4 o6 c8 o5 a8 e4 a4 | g4 e8 g8 o6 c4 o5 b8 a8 | f+4 d8 f+8 a4 g8 f+8 | d+2 f+4 b4 |"""
FOR_BM = """
  o5 g4. o6 c8 e4 d8 c8 | d4. o5 a8 f+4 a4 | b4. a8 f+4 d4 | e4 g8 b8 o6 e4 o5 b4 |
  o6 c4. o5 b8 a4 e4 | f+4 a8 o6 d8 f+4 e8 d8 | d4 o5 b8 g8 d4 g4 | f+4 d+4 o4 b4 o5 d+4 |"""
FOR_CA, FOR_CB = 'Em | C | D | Em | Am | C | D | B7', 'C | D | Bm | Em | Am | D | G | B7'
s.section('A', 8, FOR_CA, FOR_A, ARP16, FOR_B, [CRASH] + [ROCK] * 6 + [ROCK_FILL])
s.section('B', 8, FOR_CB, FOR_BM, ARP16, FOR_B, [ROCK] * 7 + [ROCK_FILL])
s.section('A2', 8, FOR_CA, FOR_A, 'thirds', FOR_B, [CRASH] + [DRIVE] * 6 + [DRIVE_FILL],
          insts=('flute', 'soft25', 'bass'), vols=(13, 7, 15, 10))
s.section('B2', 8, FOR_CB, FOR_BM, 'sixths', FOR_B, [DRIVE] * 7 + [DRIVE_FILL],
          insts=('flute', 'soft25', 'bass'), vols=(13, 7, 15, 10))
s.form(['I'], ['A', 'B', 'A2', 'B2'])

# the Beach: swung, long-short (a triplet's two and one)
s = add(song('BEACH', 'Beach Battle', 140, key=('F', 'major'), insts=('lead', 'stab', 'bass'), vols=(13, 8, 15, 10)))
SHUF = '12: k . h s . h k . h s . h'
SHUF_FILL = '12: k . h s . h s s T t t s'
SWING_B = '12: R - 5 O - 5 R - 5 O - A'
SWING_H = '12: . . x . . x . . x . . x'
BEA_A = """
  o5 c6 f12 a6 o6 c12 o5 a4 f4 | d6 f12 b-6 o6 d12 c4 o5 b-4 | g6 e12 c6 e12 g4 b-4 | a4 g12 a6 f4 r4 |
  f6 a12 o6 d6 o5 a12 f4 d4 | b-6 a12 g6 f12 d4 f4 | e6 g12 b-6 a12 g4 e4 | f2. r4 |"""
BEA_B = """
  o5 d4 f4 b-4 o6 d4 | d-4 o5 b-4 g4 f4 | a6 g12 f6 a12 o6 c4 o5 a4 | f+4 a4 o6 c4 o5 a4 |
  b-6 a12 g4 d6 f12 g4 | e4 g4 b-4 g4 | a6 c12 f6 a12 o6 c4 o5 a4 | g4 b-6 a12 g4 e4 |"""
BEA_CA, BEA_CB = 'F | Bb | C7 | F | Dm | Gm7 | C7 | F', 'Bb | Bbm6 | F | D7 | Gm7 | C7 | F | C7'
s.section('I', 2, 'F | C7', None, SWING_H, SWING_B, [SHUF, SHUF_FILL])
s.section('A', 8, BEA_CA, BEA_A, SWING_H, SWING_B, [SHUF] * 7 + [SHUF_FILL])
s.section('B', 8, BEA_CB, BEA_B, SWING_H, SWING_B, [SHUF] * 7 + [SHUF_FILL])
s.section('A2', 8, BEA_CA, BEA_A, 'thirds', SWING_B, [SHUF] * 7 + [SHUF_FILL], insts=('lead', 'soft', 'bass'))
s.section('B2', 8, BEA_CB, BEA_B, 'thirds', SWING_B, [SHUF] * 7 + [SHUF_FILL], insts=('lead', 'soft', 'bass'))
s.form(['I'], ['A', 'B', 'A2', 'B2'])

# the Cave: minor, the tune answered by its echo
s = add(song('CAVE', 'Cave Battle', 136, key=('C', 'harmonic'), insts=('thin', 'thin', 'bass'), vols=(13, 6, 15, 10)))
CAVE_DR = '16: k . . k t . . . k . k . t . T .'
CAVE_FILL = '16: k . . k t . . . T T T . t t t .'
CAVE_B = '16: R . R . O . R . R . O . 5 . O .'
s.section('I', 2, 'Cm | Cm', None, None, CAVE_B, [CAVE_DR, CAVE_FILL])
s.section('A', 8, 'Cm | Ab | Bb | Cm | Fm | Ab | G | G7', """
  o5 c4 r8 g8 e-4 r8 c8 | e-4 r8 c8 o4 a-4 o5 c4 | d4 r8 f8 b-4 a-8 f8 | g2 r4 g8 a-8 |
  a-4 r8 f8 o6 c4 r8 o5 a-8 | e-4 r8 c8 e-4 a-4 | g4. f8 d4 o4 b4 | o5 d2 f4 r4 |""",
          'echo 18', CAVE_B, [CAVE_DR] * 7 + [CAVE_FILL])
s.section('B', 8, 'Ab | Bb | Gm | Cm | Ab | Fm | Dm7b5 | G7', """
  o5 c4 e-8 a-8 o6 c4 o5 a-4 | b-4 r8 f8 d4 b-4 | g4 r8 b-8 o6 d4 o5 b-4 | c2. r4 |
  e-4 a-8 o6 c8 e-4 c4 | c4 o5 a-8 f8 c4 f4 | f4 a-4 d4 f4 | g4 b4 o6 d4 f4 |""",
          'echo 18', CAVE_B, [CAVE_DR] * 7 + [CAVE_FILL])
s.section('C', 8, 'Ab | Ab | Fm | Fm | Bb | Bb | G | G7', """
  o5 c1 | e-1 | f2 a-2 | o6 c1 | o5 b-2 f2 | d1 | d2 g2 | b2 o6 d2 |""",
          ARP16, '8: R R O R R O R O', ['16: k . . . t . . . k . . . t . . .'] * 7 + [CAVE_FILL],
          insts=('thin', 'pluck12', 'bass'), vols=(12, 7, 15, 10))
s.form(['I'], ['A', 'B', 'C'])

# the Mansion: a spooky waltz
s = add(song('MANSION', 'Mansion Battle', 168, meter=3, key=('D', 'harmonic'), insts=('ghost', 'stab', 'bass'),
             vols=(13, 8, 15, 9)))
WALTZ_DR = '16: k . . . r . . . r . . .'
WALTZ_FILL = '16: k . . . T . T . t . t .'
MAN_A = """
  o5 a2 f4 | d2. | g4 b-4 o6 d4 | c+2. | d4 c+8 d8 f4 | d2 o5 b-4 | g4 b-4 o6 c+4 | e2. |"""
MAN_B = """
  o6 f2 d4 | e2 c+4 | d4 o5 a8 b-8 a4 | f2. | g4 a8 b-8 o6 d4 | f2 d4 | c+4 e4 g4 | o5 a2. |"""
MAN_CA, MAN_CB = 'Dm | Dm | Gm | A7 | Bb | Gm | Edim7 | A7', 'Bb | A7 | Dm | Dm | Gm | Bb | A7 | A7'
s.section('I', 4, 'Dm | Dm | A7 | A7', None, '4: . x x', '4: R . .', [WALTZ_DR] * 3 + [WALTZ_FILL])
s.section('A', 8, MAN_CA, MAN_A, '4: . x x', '4: R . .', [WALTZ_DR] * 7 + [WALTZ_FILL])
s.section('B', 8, MAN_CB, MAN_B, '4: . x x', '4: R . 5', [WALTZ_DR] * 7 + [WALTZ_FILL])
s.section('A2', 8, MAN_CA, MAN_A, 'thirds', '4: R . .', [WALTZ_DR] * 7 + [WALTZ_FILL],
          insts=('ghost', 'soft', 'bass'), vols=(13, 6, 15, 9))
s.form(['I'], ['A', 'B', 'A2', 'B'])

# the Field: a march
s = add(song('FIELD', 'Field Battle', 132, key=('Bb', 'major'), insts=('brass', 'stab', 'bass'), vols=(13, 8, 15, 10)))
MARCH = '16: k . s . s . s s k . s . s s s .'
MARCH_FILL = '16: k . s . s . s s s s s s s s s s'
FIE_A = """
  o5 f8. d16 f8. b-16 o6 d4 o5 b-4 | g8. e-16 g8. b-16 o6 e-4 o5 b-4 | a4 o6 c4 o5 a4 f4 | b-2 r4 f8. f16 |
  g8. a16 b-8. g16 d4 g4 | e-8. f16 g8. e-16 b-4 g4 | e4 g4 b-4 o6 c4 | o5 a4 f4 o6 c4 o5 a4 |"""
FIE_CA = 'Bb | Eb | F7 | Bb | Gm | Eb | C7 | F7'
s.section('I', 2, 'Bb | F7', None, STAB8, '4: R L R L', [MARCH, MARCH_FILL])
s.section('A', 8, FIE_CA, FIE_A, STAB8, '4: R L R L', [MARCH] * 7 + [MARCH_FILL])
s.section('B', 8, 'Eb | Bb | Cm7 | F7 | Gm | Eb | F7 | Bb', """
  o5 g4. b-8 o6 e-4 o5 b-4 | o6 d4. c8 o5 b-4 f4 | e-4. f8 g4 b-4 | a4 o6 c8 e-8 c4 o5 a4 |
  b-4. a8 g4 d4 | e-4 g8 b-8 o6 e-4 o5 b-4 | a8. b-16 o6 c8. d16 e-4 c4 | d2 o5 b-4 r4 |""",
          STAB8, '4: R L R L', [MARCH] * 7 + [MARCH_FILL])
s.section('A2', 8, FIE_CA, FIE_A, 'thirds', '4: R L R L', [MARCH] * 7 + [MARCH_FILL], insts=('brass', 'soft25', 'bass'))
s.form(['I'], ['A', 'B', 'A2', 'B'])

# ------------------------------------------------------------------ a legend
s = add(song('LEGEND', 'Legendary Battle', 168, key=('F', 'harmonic'), insts=('brass', 'pluck12', 'bass'),
             vols=(14, 7, 15, 11)))
LEG_B = '16: R . R R R . R R R . R R R . R R'
LEG_A = """
  o5 c4 f4 a-4 o6 c4 | f2 c4 o5 a-4 | f2 a-4 f4 | o6 d-2. r4 |
  o5 b-4 f4 d-4 f4 | b-4 o6 d-4 f4 d-4 | o5 e4 g4 o6 c4 o5 g4 | b-4 g4 e4 c4 |"""
LEG_BM = """
  o5 a-2 f4 d-4 | g4. f8 e-4 b-4 | o6 c2 o5 g4 e-4 | f2. a-4 |
  b-4. a-8 f4 d-4 | f4 a-4 o6 d-4 f4 | e2 g2 | c4 o5 b-4 g4 e4 |"""
LEG_CA, LEG_CB = 'Fm | Fm | Db | Db | Bbm | Bbm | C | C7', 'Db | Eb | Cm | Fm | Bbm | Db | C | C7'
s.section('I', 4, 'Fm | Fm | C | C', None, ARP16, LEG_B, [DRIVE] * 3 + [DRIVE_FILL])
s.section('A', 8, LEG_CA, LEG_A, ARP16, LEG_B, [CRASH] + [DRIVE] * 6 + [DRIVE_FILL])
s.section('B', 8, LEG_CB, LEG_BM, ARP16, LEG_B, [DRIVE] * 7 + [DRIVE_FILL])
s.section('A2', 8, LEG_CA, LEG_A, 'thirds', LEG_B, [CRASH] + [GALLOP] * 6 + [DRIVE_FILL],
          insts=('brass', 'soft25', 'bass'), vols=(14, 7, 15, 11))
s.section('B2', 8, LEG_CB, LEG_BM, 'sixths', LEG_B, [GALLOP] * 7 + [DRIVE_FILL],
          insts=('brass', 'soft25', 'bass'), vols=(14, 7, 15, 11))
s.form(['I'], ['A', 'B', 'A2', 'B2'])

# ------------------------------------------------------------------ Knomi Tower's guardians
s = add(song('TOWER_FIGHT', 'Tower Battle', 162, key=('E', 'harmonic'), insts=('brass', 'stab', 'bass'),
             vols=(13, 8, 15, 11)))
TF_B = '16: R . R R O . R R R . R R O . R R'
TF_H = '8: x . x x . x x .'
TF_A = """
  o5 e8 e8 g8 b8 o6 e4 o5 b4 | o6 e4 d8 o5 b8 g4 b4 | o6 c4. o5 b8 g4 e4 | f+4 a4 o6 d4 o5 a4 |
  g8 f+8 g8 b8 o6 e4 o5 b4 | g4. f+8 e4 b4 | o6 c4 e4 g4 e4 | o5 d+4 f+4 b4 a4 |"""
TF_CA = 'Em | Em | C | D | Em | Em | C | B7'
s.section('I', 2, 'Em | Em', None, TF_H, TF_B, [GALLOP, DRIVE_FILL])
s.section('A', 8, TF_CA, TF_A, TF_H, TF_B, [CRASH] + [GALLOP] * 6 + [DRIVE_FILL])
s.section('B', 8, 'Am | Em | Am | B7 | C | D | B7 | B7', """
  o5 a4. b8 o6 c4 o5 a4 | b4. a8 g4 e4 | o6 c4. d8 e4 c4 | d+2 o5 b2 |
  o6 e4. d8 c4 o5 g4 | a4 f+4 d4 f+4 | b4 a4 f+4 d+4 | f+2 b2 |""",
          TF_H, TF_B, [GALLOP] * 7 + [DRIVE_FILL])
s.section('A2', 8, TF_CA, TF_A, 'thirds', TF_B, [CRASH] + [DRIVE] * 6 + [DRIVE_FILL],
          insts=('brass', 'soft25', 'bass'))
s.form(['I'], ['A', 'B', 'A2', 'B'])

# ------------------------------------------------------------------ a buddy: a friendly rival
s = add(song('BUDDY', 'Buddy Battle', 156, key=('A', 'dorian'), insts=('lead', 'stab', 'bass'), vols=(13, 8, 15, 10)))
FUNK_B = '16: R . . R . . O . R . . R . O . 5'
FUNK_H = '16: . . x . . . x . . . x . . . x .'
FUNK = '16: k . h . s . h k . k h . s . h .'
FUNK_FILL = '16: k . h . s . h k . k s s T . t .'
BUD_A = """
  o5 a8 r8 a8 g8 e4 g8 a8 | r8 f+8 a8 o6 c8 o5 a4 f+4 | a8 r8 a8 o6 c8 e8 d8 c8 o5 a8 | f+4 e8 d8 e4 r4 |
  c4 e8 f8 a4 g8 e8 | g+4 b4 o6 d4 o5 b4 | o6 c4 o5 a8 g8 e4 g4 | g+2 e4 r4 |"""
BUD_CA = 'Am7 | D9 | Am7 | D9 | Fmaj7 | E7 | Am7 | E7'
s.section('I', 2, 'Am7 | D9', None, FUNK_H, FUNK_B, [FUNK, FUNK_FILL])
s.section('A', 8, BUD_CA, BUD_A, FUNK_H, FUNK_B, [FUNK] * 7 + [FUNK_FILL])
s.section('B', 8, 'Fmaj7 | G | Em7 | Am7 | Dm7 | G | Cmaj7 | E7', """
  o5 a4. g8 f4 e4 | d4 g8 b8 o6 d4 o5 b4 | g4. f+8 e4 d4 | e4 a8 o6 c8 e4 c4 |
  d4 o5 a8 f8 d4 f4 | g4 b4 o6 d4 o5 b4 | o6 c4 o5 b8 g8 e4 g4 | g+4 b4 o6 d4 e4 |""",
          FUNK_H, FUNK_B, [FUNK] * 7 + [FUNK_FILL])
s.section('A2', 8, BUD_CA, BUD_A, 'thirds', FUNK_B, [FUNK] * 7 + [FUNK_FILL], insts=('lead', 'soft25', 'bass'))
s.form(['I'], ['A', 'B', 'A2', 'B'])

# ------------------------------------------------------------------ the victory jingle: once
s = add(song('VICTORY', 'Victory', 150, key=('C', 'major'), insts=('brass', 'soft25', 'bassleg'), vols=(14, 9, 15, 11),
             lead=(60, 92)))
s.section('V', 3, 'C | F G | C', """
  o5 c12 e12 g12 o6 c4 o5 g12 o6 c12 e12 g4 | o5 a4 f8 a8 b4 g8 b8 | o6 c2. r4 |""",
          'thirds', 'o3 c4 g4 c4 e4 | f4 c4 g4 d4 | c2. r4 |',
          ['16: k . . . s . . . k . . . s . s s', '16: k . s . k . s . T T t t s s s s', '16: c . . . . . . . . . . . . . . .'])
s.form(['V'], [])

# ------------------------------------------------------------------ the minigames
# Berry Drop: bouncy, wood blocks
s = add(song('BERRY', 'Berry Drop', 128, key=('G', 'major'), insts=('pluck', 'stab', 'bass'), vols=(14, 8, 15, 9)))
BERRY_DR = '16: k . w . s . w . k . w k s . w .'
BERRY_FILL = '16: k . w . s . w . w w w . s s s .'
BER_A = """
  o5 d8 g8 b8 g8 d8 g8 b8 o6 d8 | o5 e8 c8 e8 g8 o6 c4 o5 g4 | f+8 a8 f+8 d8 a4 f+4 | g4 b8 g8 d4 r4 |
  e8 g8 b8 g8 e8 g8 b8 g8 | c8 e8 g8 e8 o6 c4 o5 g4 | c+8 e8 a8 e8 g4 e4 | f+4 a4 o6 c4 o5 a4 |"""
BER_B = """
  o5 g4. e8 g4 o6 c4 | o5 a4. f+8 a4 o6 d4 | o5 b4 a8 f+8 d4 f+4 | e2 r4 b8 g8 |
  e8 g8 o6 c8 o5 g8 e8 g8 o6 c8 e8 | d8 o5 a8 f+8 a8 o6 d4 o5 a4 | b4 g4 d4 g4 | f+8 a8 o6 c8 o5 a8 f+4 d4 |"""
BER_CA, BER_CB = 'G | C | D | G | Em | C | A7 | D7', 'C | D | Bm | Em | C | D | G | D7'
s.section('I', 2, 'G | D7', None, STAB8, '8: R . O . R . O .', [BERRY_DR, BERRY_FILL])
s.section('A', 8, BER_CA, BER_A, STAB8, '8: R . O . R . O .', [BERRY_DR] * 7 + [BERRY_FILL])
s.section('B', 8, BER_CB, BER_B, STAB8, '8: R . O . R . O .', [BERRY_DR] * 7 + [BERRY_FILL])
s.section('A2', 8, BER_CA, BER_A, 'thirds', '8: R . O . R . O .', [BERRY_DR] * 7 + [BERRY_FILL],
          insts=('pluck', 'pluck12', 'bass'))
s.form(['I'], ['A', 'B', 'A2', 'B'])

# PokeRun: running
s = add(song('RUN', 'PokeRun', 176, key=('D', 'major'), insts=('lead', 'pluck12', 'bass'), vols=(13, 7, 15, 10)))
RUN_DR = '16: k . h . s . h . k . h . s . h h'
RUN_A = """
  o5 a8 f+8 a8 o6 d8 o5 a8 f+8 d8 f+8 | g8 b8 o6 d8 o5 b8 g4 b4 | a8 o6 c+8 e8 c+8 o5 a4 e4 | f+4 a4 o6 d2 |
  o5 b8 a8 b8 o6 d8 o5 b8 a8 f+8 d8 | g8 f+8 g8 b8 o6 d4 o5 b4 | e8 g8 b8 g8 o6 e8 o5 b8 g8 e8 | c+4 e4 g4 e4 |"""
RUN_CA = 'D | G | A | D | Bm | G | Em | A7'
s.section('I', 2, 'D | A7', None, ARP16, '8: R O R O R O R O', [RUN_DR, ROCK_FILL])
s.section('A', 8, RUN_CA, RUN_A, ARP16, '8: R O R O R O R O', [CRASH] + [RUN_DR] * 6 + [ROCK_FILL])
s.section('B', 8, 'G | A | F#m | Bm | G | A | D | A7', """
  o5 d4 g4 b4 g4 | c+4 e4 a4 e4 | f+4. e8 c+4 o4 a4 | o5 d4 f+4 b4 o6 d4 |
  o6 d4. c+8 o5 b4 g4 | o6 c+4. o5 b8 a4 e4 | f+8 a8 o6 d8 f+8 e8 d8 o5 a8 f+8 | o6 e4 c+4 o5 a4 g4 |""",
          ARP16, '8: R O R O R O R O', [RUN_DR] * 7 + [ROCK_FILL])
s.section('A2', 8, RUN_CA, RUN_A, 'thirds', '8: R O R O R O R O', [CRASH] + [RUN_DR] * 6 + [ROCK_FILL],
          insts=('lead', 'soft25', 'bass'))
s.form(['I'], ['A', 'B', 'A2', 'B'])

# PikaSurf: surf rock, its tune picked fast
s = add(song('SURF', 'PikaSurf', 164, key=('A', 'minor'), insts=('pluck', 'stab', 'bass'), vols=(14, 7, 15, 10)))
SURF_DR = '16: k . h k s . h . k k h . s . h .'
SURF_FILL = '16: k . h k s . h . T T T T t t t t'
SURF_B = '8: R R O R 5 R O 5'
SURF_A = """
  o5 e4 a4 o6 c4. o5 b8 | a2 e4 a4 | a4 o6 c4 o5 a4 f4 | g2 b4 o6 d4 |
  o5 e4 a4 o6 c4 e4 | e4. d8 c4 o5 a4 | f4 a4 o6 c4 o5 a4 | g+2 b2 |"""
SURF_CA = 'Am | Am | F | G | Am | Am | F | E7'
s.section('I', 2, 'Am | E7', None, FUNK_H, SURF_B, [SURF_DR, SURF_FILL])
s.section('A', 8, SURF_CA, SURF_A, FUNK_H, SURF_B, [CRASH] + [SURF_DR] * 6 + [SURF_FILL], tremolo=6)
s.section('B', 8, 'Dm | G | C | Am | Dm | G | E7 | E7', """
  o5 f4 a4 o6 d4. c8 | o5 b4 g4 d2 | e4 g4 o6 c4. o5 b8 | a2 e2 |
  d4 f4 a4 o6 d4 | d4. c8 o5 b4 g4 | g+4 b4 o6 d4 o5 b4 | e2 r2 |""",
          FUNK_H, SURF_B, [SURF_DR] * 7 + [SURF_FILL], tremolo=6)
s.section('A2', 8, SURF_CA, SURF_A, 'thirds', SURF_B, [CRASH] + [SURF_DR] * 6 + [SURF_FILL], tremolo=6,
          insts=('pluck', 'pluck12', 'bass'))
s.form(['I'], ['A', 'B', 'A2', 'B'])

# PokeDungeon: down the stairs, sparse
s = add(song('DUNGEON', 'PokeDungeon', 104, key=('D', 'dorian'), insts=('bell25', 'soft', 'bass'), vols=(14, 6, 14, 8)))
DUN_DR = '16: k . . . . . r . . . k . . . r .'
DUN_B = '8: R . . R . . R .'
s.section('I', 2, 'Dm | Dm', None, None, DUN_B, [DUN_DR, DUN_DR])
s.section('A', 8, 'Dm | C | Bb | C | Dm | C | Bb | A', """
  o5 d4 r8 f8 a4 r4 | g4 r8 e8 c4 r4 | f4 r8 d8 b-4 r4 | e2 g4 r4 |
  a4 r8 f8 d4 a4 | g4 r8 e8 c4 g4 | f4. d8 b-4 o6 d4 | o5 c+2. r4 |""",
          '8: 1 2 3 2 1 2 3 2', DUN_B, [DUN_DR] * 7 + ['16: k . . . . . r . . . k . t . t .'])
s.section('B', 8, 'Gm | Dm | Gm | Dm | Bb | C | A | A7', """
  o5 b-4. a8 g4 d4 | f4. e8 d4 a4 | g4. a8 b-4 o6 d4 | o5 a4 f4 d4 f4 |
  f4. g8 b-4 f4 | e4. f8 g4 e4 | e4 a4 o6 c+4 o5 a4 | g2 e2 |""",
          '8: 1 2 3 2 1 2 3 2', DUN_B, [DUN_DR] * 7 + ['16: k . . . . . r . . . k . t . t .'])
s.form(['I'], ['A', 'B'])

# Who's That? and Showdown: thinking it over, a clock ticking
s = add(song('QUIZ', "Who's That?", 120, key=('F', 'major'), insts=('pluck', 'pluck12', 'basspl'), vols=(14, 7, 15, 9)))
QUIZ_DR = '16: k . w . r . w . k . w . r . w .'
s.section('I', 2, 'F | C7', None, None, '4: R 5 O 5', ['16: w . . . w . . . w . . . w . . .'] * 2)
s.section('A', 8, 'F | Dm | Gm7 | C7 | F | Dm | G7 | C7', """
  o5 c8 r8 f8 r8 a8 g+8 a8 r8 | d8 r8 f8 r8 a8 g+8 a8 r8 | b-8 a8 g8 f8 d8 r8 f8 r8 | e8 r8 g8 r8 b-4 r4 |
  a8 r8 o6 c8 r8 o5 a8 f8 c8 r8 | f8 r8 a8 r8 f8 d8 o4 a8 r8 | o5 b8 o6 d8 o5 b8 g8 f8 d8 o4 b8 r8 | o5 c4 e4 g4 r4 |""",
          STAB8, '4: R 3 5 A', [QUIZ_DR])
s.section('B', 8, 'Bb | C7 | Am | Dm | Gm7 | C7 | F | C7', """
  o5 d4 f8 b-8 r8 b-8 a8 b-8 | o6 c4 o5 g8 e8 r8 e8 f8 g8 | a4 e8 c8 r8 c8 d8 e8 | f4 d8 o4 a8 r8 o5 d8 e8 f8 |
  g4 d8 o4 b-8 r8 o5 d8 f8 a8 | g4 e8 c8 r8 e8 g8 b-8 | a8 g8 f8 e8 f4 c4 | e8 r8 g8 r8 b-8 r8 o6 c8 r8 |""",
          STAB8, '4: R 3 5 A', [QUIZ_DR])
s.form(['I'], ['A', 'B'])

# Volley: sporty, a clap on two and four
s = add(song('VOLLEY', 'Volley', 144, key=('A', 'major'), insts=('lead50', 'stab', 'bass'), vols=(13, 8, 15, 10)))
VOL_DR = '16: k . h . x . h . k . h . x . h k'
VOL_FILL = '16: k . h . x . h . x x x x x . c .'
VOL_A = """
  o5 e4 a4 o6 c+4. o5 b8 | b4 g+4 e2 | f+4 a4 o6 c+4 o5 a4 | f+2 d4 e4 |
  e4. a8 o6 c+4 e4 | e4 d8 c+8 o5 b4 g+4 | a4 f+8 a8 o6 d4 o5 a4 | g+2 b4 o6 e4 |"""
VOL_CA = 'A | E/G# | F#m | D | A | E | D | E'
s.section('I', 2, 'A | E', None, STAB8, '8: R O R O R O R O', [VOL_DR, VOL_FILL])
s.section('A', 8, VOL_CA, VOL_A, STAB8, '8: R O R O R O R O', [VOL_DR] * 7 + [VOL_FILL])
s.section('B', 8, 'D | E | C#m | F#m | D | E | A | E7', """
  o5 f+4. e8 d4 a4 | g+4. f+8 e4 b4 | e4. d+8 c+4 g+4 | a2 f+4 c+4 |
  d4 f+4 a4 o6 d4 | e4. d8 o5 b4 g+4 | a4 o6 c+4 e4 c+4 | o5 b4 g+4 e4 d4 |""",
          STAB8, '8: R O R O R O R O', [VOL_DR] * 7 + [VOL_FILL])
s.section('A2', 8, VOL_CA, VOL_A, 'thirds', '8: R O R O R O R O', [VOL_DR] * 7 + [VOL_FILL],
          insts=('lead50', 'soft25', 'bass'))
s.form(['I'], ['A', 'B', 'A2', 'B'])

# ------------------------------------------------------------------ evolving: rising, then the new form's chord
s = add(song('EVOLVE', 'Evolution', 132, key=('C', 'major'), insts=('lead', 'soft', 'bassleg'), vols=(14, 8, 15, 11)))
s.section('E', 3, 'Am:2 E7:2 | F:2 G:2 | C', """
  o4 a8 b8 o5 c8 d8 e8 f8 e8 g+8 | a8 g8 f8 a8 b8 a8 g8 b8 | o6 c2. r4 |""",
          '16: x x x x x x x x x x x x x x x x', '8: R R R R R R R R',
          ['16: k . . . s . . . k . . . s . s .', '16: s . s . s . s . s s s s s s s s', '16: c . . . . . . . . . . . . . . .'])
s.form(['E'], [])

# ------------------------------------------------------------------ a goodbye
s = add(song('GOODBYE', 'Goodbye', 72, key=('F', 'major'), insts=('flute', 'soft', 'bassleg'), vols=(12, 6, 13, 0)))
s.section('A', 8, 'F | C/E | Dm | Bb | F/C | Bb | Gm7 | C7', """
  o5 c4 f4 a4. g8 | g2 e4 c4 | d4 f4 a4. g8 | f2. r4 |
  a4 o6 c4 o5 a4. g8 | f4 d4 b-4. a8 | g4. f8 d4 f4 | e2 g2 |""",
          '8: 1 2 3 2 1 2 3 2', '2: R 5', None)
s.form([], ['A'])


# ------------------------------------------------------------------ Travel: each zone's own, calmer than its battle
# (Fedor: "each travel biome needs its own track too"), in the old handhelds' voices as the home is
# the Forest: a flute over the picked chords, a lilt
s = add(song('ZONE_FOREST', 'Forest', 108, key=('G', 'major'), insts=('flute', 'gbpick', 'gbwave'), vols=(13, 8, 10, 8),
             bass_lo=38))
ZF_DR = '16: . . S . . . S . . . S . . . S S'
s.section('I', 2, 'G | D7', None, HOME_PICK, '8: R . 5 . O . 5 .', [ZF_DR] * 2)
s.section('A', 8, 'G | Em | C | D | G | Em | Am7 D7 | G', """
  o5 d4 g8 a8 b4. a8 | g4 e4 b2 | c4 e8 g8 o6 c4. o5 b8 | a2. r4 |
  b4 o6 d8 c8 o5 b4. a8 | g4 b8 g8 e4. g8 | a4 o6 c8 o5 a8 f+4 d4 | g2. r4 |""",
          HOME_PICK, '8: R . 5 . O . 5 .', [ZF_DR])
s.section('B', 8, 'C | D | Bm | Em | Am7 | D | C D | G', """
  o5 e4. f+8 g4 e4 | f+4. g8 a4 f+4 | b4. a8 f+4 d4 | e2 g4 b4 |
  o6 c4. o5 b8 a4 e4 | f+4 a4 o6 d4. c8 | o5 g4 e4 a4 f+4 | g2. r4 |""",
          'thirds', '8: R . 5 . O . 5 .', [ZF_DR], insts=('flute', 'gbharm', 'gbwave'), vols=(13, 7, 10, 8))
s.form(['I'], ['A', 'B'])

# the Beach: a steel drum's swing, lazier than the battle's
s = add(song('ZONE_BEACH', 'Beach', 104, key=('C', 'major'), insts=('bell25', 'gbpick', 'gbwave'), vols=(14, 8, 10, 8),
             bass_lo=38))
ZB_DR = '12: k . S . . S k . S . . S'
s.section('I', 2, 'C | G7', None, SWING_H, SWING_B, [ZB_DR] * 2)
s.section('A', 8, 'C | Am | Dm7 | G7 | C | Am | F G7 | C', """
  o5 e6 g12 o6 c6 o5 g12 e4 g4 | a6 g12 e6 c12 a4 r4 | f6 a12 o6 c6 o5 a12 f4 d4 | g4 b6 a12 g4 r4 |
  o6 c6 o5 b12 o6 c6 e12 g4 e4 | c6 o5 a12 e6 a12 o6 c4 o5 a4 | a6 f12 c6 f12 g6 b12 o6 d6 o5 b12 | o6 c2. r4 |""",
          SWING_H, SWING_B, [ZB_DR])
s.section('B', 8, 'F | C | Dm7 | G7 | Em7 | A7 | Dm7 G7 | C', """
  o5 a4 o6 c6 o5 a12 f4 a4 | g4 e6 c12 e4 g4 | f6 e12 d6 f12 a4 o6 c4 | o5 b2 g4 r4 |
  g6 b12 o6 d6 o5 b12 g4 e4 | c+4 e6 g12 a4 e4 | f4 a4 g4 b4 | o6 c2. r4 |""",
          'thirds', SWING_B, [ZB_DR], insts=('bell25', 'gbharm', 'gbwave'), vols=(14, 7, 10, 8))
s.form(['I'], ['A', 'B'])

# the Cave: drops in the dark, each answered by its echo
s = add(song('ZONE_CAVE', 'Cave', 92, key=('A', 'harmonic'), insts=('bell25', 'bell25', 'gbwave'), vols=(14, 6, 10, 8),
             bass_lo=33))
ZC_DR = '16: . . . . . . r . . . . . . . . .'
ZC_FILL = '16: . . . . . . r . . . t . . . t .'
s.section('I', 2, 'Am | Am', None, None, '8: R . . . O . . .', [ZC_DR, ZC_FILL])
s.section('A', 8, 'Am | F | Dm | E | Am | F | Dm | E7', """
  o5 e4 r8 a8 o6 c4 r4 | o5 a4 r8 f8 c4 r4 | d4 r8 f8 a4 g8 f8 | e2. r4 |
  a4 r8 o6 c8 e4 r4 | d4 c8 o5 a8 f4 r4 | f4 a4 d4 f4 | g+2 r2 |""",
          'echo 18', '8: R . . . O . . .', [ZC_DR] * 7 + [ZC_FILL])
s.section('B', 8, 'F | G | Em | Am | Dm | E | F E | E7', """
  o5 c4 r8 f8 a4 r4 | b4 r8 g8 d4 r4 | e4 r8 g8 b4 r4 | a2 e2 |
  f4 r8 a8 o6 d4 r4 | o5 g+4 r8 b8 e4 r4 | a4 f4 g+4 e4 | d2 r2 |""",
          'echo 18', '8: R . . . O . . .', [ZC_DR] * 7 + [ZC_FILL])
s.form(['I'], ['A', 'B'])

# the Mansion: a slow music box, a little out of tune with itself
s = add(song('ZONE_MANSION', 'Mansion', 120, meter=3, key=('D', 'harmonic'), insts=('bell', 'soft', 'gbwave'),
             vols=(14, 6, 10, 0), bass_lo=38))
ZM_A = """
  o5 d4 f4 a4 | b-2 g4 | a4 o6 c+4 e4 | d2. | f4 d4 o5 b-4 | o6 d4 o5 b-4 g4 | e4 g4 b-4 | a2. |"""
ZM_CA = 'Dm | Gm | A7 | Dm | Bb | Gm | Edim7 | A7'
s.section('I', 2, 'Dm | A7', None, '4: . x x', '4: R . .', None)
s.section('A', 8, ZM_CA, ZM_A, '4: . x x', '4: R . .', None)
s.section('B', 8, 'Gm | Dm | Bb | A7 | Gm | Dm | Edim7 | A7', """
  o5 g4 b-4 o6 d4 | f2 d4 | d4 c8 o5 b-8 f4 | e2 c+4 | d4 g4 b-4 | a2 f4 | g4 b-4 o6 c+4 | o5 a2. |""",
          '4: . x x', '4: R . .', None)
s.section('A2', 8, ZM_CA, ZM_A, 'sixths', '4: R . .', None, insts=('bell', 'bell25', 'gbwave'), vols=(14, 6, 10, 0))
s.form(['I'], ['A', 'B', 'A2', 'B'])

# the Field: out in the open, the old handhelds' route
s = add(song('ZONE_FIELD', 'Field', 124, key=('C', 'major'), insts=('gblead', 'gbpick', 'gbwave'), vols=(13, 8, 10, 7),
             bass_lo=38))
ZFI_B = '8: R O R O 5 O R A'
ZFI_A = """
  o5 g4 e8 g8 o6 c4 d4 | e4 d8 c8 o5 b4 g4 | a4 o6 c8 e8 e4 c4 | c2 o5 a4 r4 |
  g4. a8 g8 e8 c8 e8 | d4 g8 b8 o6 d4 o5 b4 | a4 f4 g4 b4 | o6 c2. r4 |"""
ZFI_CA = 'C | G/B | Am | F | C | G | F G | C'
s.section('I', 2, 'C | G7', None, STAB8, ZFI_B, [POP, POP_FILL])
s.section('A', 8, ZFI_CA, ZFI_A, STAB8, ZFI_B, [POP] * 7 + [POP_FILL])
s.section('B', 8, 'F | G | Em | Am | F | G | Dm7 | G7', """
  o5 a4. g8 f4 c4 | d4. e8 g4 b4 | b4. a8 g4 e4 | a2 e4 c4 |
  f4 a8 o6 c8 f4 c4 | d4 o5 b8 g8 d4 g4 | a4 f4 d4 f4 | g4 b4 o6 d4 f4 |""",
          'thirds', '4: R 5 O 5', [POP] * 7 + [POP_FILL], insts=('gblead', 'gbharm', 'gbwave'), vols=(13, 7, 10, 7))
s.section('A2', 8, ZFI_CA, ZFI_A, 'thirds', ZFI_B, [POP] * 7 + [POP_FILL], insts=('gblead', 'gbharm', 'gbwave'),
          vols=(13, 7, 10, 7))
s.form(['I'], ['A', 'B', 'A2', 'B'])

# ------------------------------------------------------------------ how loud each plays
# Measured (each voice's RMS: host/music_wav.cpp prints the mix's) so they all sit about 8 dB under the effects: the
# home and the menus at -24.5 dBFS, the battles and the games at -24, the night and the goodbye softer, the jingles
# louder; the sparse ones (the dungeon, the quiz) a little under, their notes being short.
GAINS = {
    'HELLO': 0.92, 'HOME': 0.84, 'NIGHT': 1.0, 'MENU': 1.03, 'TRAVEL': 0.81, 'TOWER': 0.84,
    'FOREST': 0.80, 'BEACH': 0.92, 'CAVE': 1.17, 'MANSION': 1.29, 'FIELD': 0.76,
    'LEGEND': 0.78, 'TOWER_FIGHT': 0.79, 'BUDDY': 1.02, 'VICTORY': 0.80,
    'BERRY': 1.24, 'RUN': 0.88, 'SURF': 0.91, 'DUNGEON': 1.29, 'QUIZ': 1.41, 'VOLLEY': 0.89,
    'EVOLVE': 0.90, 'GOODBYE': 0.86,
    'ZONE_FOREST': 0.95, 'ZONE_BEACH': 1.10, 'ZONE_CAVE': 1.55, 'ZONE_MANSION': 1.50, 'ZONE_FIELD': 0.84,
}


# ================================================================== the tracks, in music.h's order

def tracks_from_header():
    with open(os.path.join(ROOT, 'src', 'music.h'), encoding='utf-8') as fh:
        src = fh.read()
    body = src[src.index('enum MusicTrack'):]
    body = body[:body.index('};')]
    names = re.findall(r'^\s*MUS_([A-Z_]+)\b', body, re.M)
    assert names[0] == 'NONE' and names[-1] == 'COUNT'
    return names[1:-1]


def placeholder(track):
    """A song not written yet: a bar of silence, looped."""
    b = C.Builder(track, track.title(), 120, instruments=INST)
    b.section('X', 1, 'C', None, None, None, None)
    b.form([], ['X'])
    return b


def main():
    check = '--check' in sys.argv
    tracks = tracks_from_header()
    built = {}
    for b in SONGS:
        if b.track not in tracks:
            raise SystemExit(f'{b.track} is not in music.h')
        b.gain = GAINS.get(b.track, 1.0)
        built[b.track] = b.build()
    missing = [t for t in tracks if t not in built]
    for t in missing:
        built[t] = placeholder(t).build()
    rows = [(n, [r[0]] + list(r[1:])) for (n, *r) in INSTRUMENTS]
    drums = [(d[0], d[1], d[2:]) for d in DRUMS]
    text = C.header(INST, rows, drums, [built[t] for t in tracks], tracks)
    path = os.path.join(ROOT, 'src', 'music_data.h')
    if check:
        with open(path, encoding='utf-8') as fh:
            if fh.read() != text:
                raise SystemExit('src/music_data.h is out of date: run python3 tools/music/songs.py')
        return
    with open(path, 'w', encoding='utf-8') as fh:
        fh.write(text)
    for t in tracks:
        s = built[t]
        secs = (s['intro'] + s['loop']) / C.TPB * 60 / s['bpm']
        size = sum(len(x) for x in s['streams'])
        print(f'  {t:12s} {s["name"]:16s} {secs:6.1f} s  {size:5d} bytes')
    if missing:
        print('not written yet:', ', '.join(missing))
    for w in C.warnings:
        print('  !', w)


if __name__ == '__main__':
    main()
