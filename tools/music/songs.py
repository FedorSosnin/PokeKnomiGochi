#!/usr/bin/env python3
"""PokeKnomiGochi's background music (V2, on a computer and the web): every song, written for this game.

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
    ('brass',  'MW_P50', 8, 220, 0.72, 60, 0, 12, 0.14, 6.0, 200, 25),   # the battles' tune
    ('pluck',  'MW_P25', 2, 70, 0.22, 45, 0, 8, 0, 0, 0, 0),             # short notes
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

# ------------------------------------------------------------------ Home: by day, the pet's own tune
s = add(song('HOME', 'Home', 104, key=('F', 'major'), insts=('lead', 'stab', 'bass'), vols=(13, 8, 15, 9)))
BEAT = '16: k . h . s . h . k . h k s . h .'
FILL = '16: k . h . s . h . s . s s T . t .'
s.section('I', 2, 'F | C7', None, '8: . x . x . x . x', '8: R . 5 . O . 5 A', [BEAT, FILL])
s.section('A', 8, 'F | Dm | Bb | C | F | Dm | Gm7 C7 | F', '''
  o5 r8 c8 f8 a8 o6 c4 o5 a8 f8 | a4 g8 f8 d4. f8 | f8 g8 f8 d8 o4 b-4 o5 c8 d8 | e4. d8 c4 r4 |
  r8 c8 f8 a8 o6 c8 d8 c8 o5 a8 | o6 d4 c8 o5 a8 f4 d8 f8 | g4 b-8 a8 g8 e8 c8 e8 | f2 r8 c8 d8 e8 |''',
          '8: . x . x . x . x', '8: R . 5 . O . 5 A', [BEAT] * 7 + [FILL])
s.section('A2', 8, 'F | Dm | Bb | C | Am | Dm | Gm7 C7 | F', '''
  o5 f8 a8 o6 c8 o5 a8 o6 f4 e8 d8 | d4 c8 o5 a8 f4 a8 o6 c8 | d4. c8 o5 b-4 a8 g8 | g4 a8 g8 e4 c4 |
  c4 e8 a8 o6 c4 o5 b8 a8 | a4 f8 d8 f4 a4 | g8 f8 g8 a8 b-8 a8 g8 e8 | f2. r4 |''',
          '8: . x . x . x . x', '8: R . 5 . O . 5 A', [BEAT] * 7 + [FILL])
s.section('B', 8, 'Bb | C | Am | Dm | Bb | C | Gm7 | C7', '''
  o6 d4. c8 o5 b-4 f4 | g4. a8 g4 e4 | a4. g8 a4 o6 c4 | d2 o5 a4 f4 |
  o6 d4. c8 o5 b-4 o6 d4 | e4. d8 c4 o5 g4 | b-4 a8 g8 f4 d4 | e4 g4 b-4 g8 e8 |''',
          'thirds', '4: R - 5 -', ['16: k . h . . . h . k . h . . . h .'] * 7 + [FILL],
          insts=('lead', 'soft', 'bass'), vols=(13, 7, 15, 8))
s.form(['I'], ['A', 'A2', 'B', 'A2'])


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
