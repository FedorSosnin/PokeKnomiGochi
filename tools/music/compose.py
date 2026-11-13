"""The music compiler: songs written as note text (songs.py) into the byte streams music_synth.h plays.

A song is a list of sections, each a number of bars with
  - its chords, one bar between bars lines: "C | Am | F G | G7"  (two in a bar share it; "F:3 G:1" by beats);
  - a melody in MML (below), for the first pulse;
  - the harmony, for the second pulse: a pattern played on the chords, the melody a third below, the melody's echo,
    or MML of its own;
  - the bass, for the triangle: a pattern played on the chords, or MML;
  - the drums: a pattern a bar.
Every bar is checked to add up, and the melody's notes on the strong beats are checked against the chords: the
compiler prints what doesn't fit, to look at.

MML: o4 sets the octave (o4 c is middle C, MIDI 60), > and < move it; l8 sets the default length; c d e f g a b
are notes, + or # sharp, - flat, then a length (4 a quarter, 8 an eighth, 12 an eighth of a triplet, 16, 32...),
dots, ^8 to tie on more; r is a rest; & slurs into the next note (no new attack); @name picks an instrument, v12 the
volume (0-15); [ ... ]3 repeats; | is a bar line, checked.
"""
import re

TPB = 24           # ticks per beat (a quarter note)
WHOLE = TPB * 4

NOTE_PC = {'c': 0, 'd': 2, 'e': 4, 'f': 5, 'g': 7, 'a': 9, 'b': 11}
QUALITIES = {
    '': [0, 4, 7], 'm': [0, 3, 7], '7': [0, 4, 7, 10], 'maj7': [0, 4, 7, 11], 'm7': [0, 3, 7, 10],
    'dim': [0, 3, 6], 'dim7': [0, 3, 6, 9], 'aug': [0, 4, 8], 'sus4': [0, 5, 7], 'sus2': [0, 2, 7],
    'add9': [0, 4, 7, 14], '6': [0, 4, 7, 9], 'm6': [0, 3, 7, 9], '9': [0, 4, 7, 10, 14], 'm9': [0, 3, 7, 10, 14],
    '7sus4': [0, 5, 7, 10], 'm7b5': [0, 3, 6, 10], 'madd9': [0, 3, 7, 14], 'mmaj7': [0, 3, 7, 11],
}
SCALES = {'major': [0, 2, 4, 5, 7, 9, 11], 'minor': [0, 2, 3, 5, 7, 8, 10], 'harmonic': [0, 2, 3, 5, 7, 8, 11],
          'dorian': [0, 2, 3, 5, 7, 9, 10], 'mixolydian': [0, 2, 4, 5, 7, 9, 10], 'phrygian': [0, 1, 3, 5, 7, 8, 10]}

DRUM_KEYS = {}     # the drum pattern letters, set by songs.py: {'k': 1, ...}

warnings = []


def warn(msg):
    warnings.append(msg)


class Song:
    """What a song is made of while it's compiled: an item list for each voice, and where its loop starts."""

    def __init__(self, name):
        self.name = name
        self.ch = [[], [], [], []]
        self.loop_at = None
        self.ticks = 0


# ------------------------------------------------------------------ chords

def parse_chord(sym):
    m = re.fullmatch(r'([A-G])([#b]?)([a-z0-9]*)(?:/([A-G])([#b]?))?', sym)
    if not m:
        raise ValueError(f'chord {sym!r}?')
    root = (NOTE_PC[m.group(1).lower()] + {'#': 1, 'b': -1, '': 0}[m.group(2)]) % 12
    if m.group(3) not in QUALITIES:
        raise ValueError(f'chord quality {m.group(3)!r} in {sym!r}?')
    ivs = QUALITIES[m.group(3)]
    bass = root
    if m.group(4):
        bass = (NOTE_PC[m.group(4).lower()] + {'#': 1, 'b': -1, '': 0}[m.group(5)]) % 12
    return {'sym': sym, 'root': root, 'ivs': ivs, 'bass': bass, 'pcs': {(root + i) % 12 for i in ivs}}


def chord_timeline(text, bars, bar_ticks, beat_ticks):
    """'C | Am | F G' -> [(start, length, chord)], checked to fill the bars."""
    out = []
    cells = [c.strip() for c in text.split('|') if c.strip()]
    if len(cells) != bars:
        raise ValueError(f'{len(cells)} bars of chords, {bars} expected: {text!r}')
    for b, cell in enumerate(cells):
        syms = cell.split()
        lens = []
        for s in syms:
            if ':' in s:
                lens.append(int(s.split(':')[1]) * beat_ticks)
            else:
                lens.append(None)
        free = bar_ticks - sum(x for x in lens if x)
        n_free = sum(1 for x in lens if x is None)
        lens = [x if x else free // n_free for x in lens]
        if sum(lens) != bar_ticks:
            raise ValueError(f'bar {b + 1} of chords {cell!r} does not fill the bar')
        t = b * bar_ticks
        for s, ln in zip(syms, lens):
            out.append((t, ln, parse_chord(s.split(':')[0])))
            t += ln
    return out


def chord_at(timeline, tick):
    for (t, ln, c) in timeline:
        if t <= tick < t + ln:
            return c
    return timeline[-1][2]


# ------------------------------------------------------------------ MML

TOKEN = re.compile(r'\s*(?:(o)(\d)|([<>])|(l)(\d+)(\.*)|(@)([a-z_0-9]+)|(v)(\d+)|(\[)|(\])(\d*)|(\|)|'
                   r'([a-gr])([+#-]*)(\d*)(\.*)((?:\^\d+\.*)*)(&?))')


def length_ticks(num, dots, default):
    t = WHOLE // int(num) if num else default
    if num and WHOLE % int(num):
        raise ValueError(f'length {num} does not divide a whole note')
    add = t
    for _ in dots:
        add //= 2
        t += add
    return t


def mml(text, bar_ticks, instruments, where):
    """MML -> items [('note', midi, ticks, slurred) | ('rest', ticks) | ('inst', i) | ('vol', v)], and its ticks."""
    pos = 0
    stack = [[]]
    octave = 4
    default = WHOLE // 8
    slur_next = False
    bar_start_ticks = [0]  # ticks at the last bar line, per repeat level (checked on the flat result)
    text = text.strip()
    while pos < len(text):
        m = TOKEN.match(text, pos)
        if not m or m.end() == pos:
            raise ValueError(f'{where}: MML not understood at {text[pos:pos + 20]!r}')
        pos = m.end()
        g = m.groups()
        cur = stack[-1]
        if g[0]:
            octave = int(g[1])
        elif g[2]:
            octave += 1 if g[2] == '>' else -1
        elif g[3]:
            default = length_ticks(g[4], g[5], default)
        elif g[6]:
            if g[7] not in instruments:
                raise ValueError(f'{where}: instrument @{g[7]}?')
            cur.append(('inst', instruments[g[7]]))
        elif g[8]:
            cur.append(('vol', int(g[9])))
        elif g[10]:
            stack.append([])
        elif g[11]:
            body = stack.pop()
            stack[-1].extend(body * (int(g[12]) if g[12] else 2))
        elif g[13]:
            stack[-1].append(('bar',))
        elif g[14]:
            ln = length_ticks(g[16], g[17], default)
            for tie in re.findall(r'\^(\d+)(\.*)', g[18]):
                ln += length_ticks(tie[0], tie[1], default)
            if g[14] == 'r':
                cur.append(('rest', ln))
                slur_next = False
            else:
                n = NOTE_PC[g[14]] + 12 * (octave + 1)
                for acc in g[15]:
                    n += -1 if acc == '-' else 1
                cur.append(('note', n, ln, slur_next))
                slur_next = bool(g[19])
    if len(stack) != 1:
        raise ValueError(f'{where}: unclosed [')
    items = []
    t = 0
    last_bar = 0
    nbar = 1
    for it in stack[0]:
        if it[0] == 'bar':
            if t - last_bar != bar_ticks:
                raise ValueError(f'{where}: bar {nbar} is {t - last_bar} ticks, not {bar_ticks}')
            last_bar = t
            nbar += 1
            continue
        items.append(it)
        if it[0] in ('note', 'rest'):
            t += it[2] if it[0] == 'note' else it[1]
    if t != last_bar and t - last_bar != bar_ticks:
        raise ValueError(f'{where}: the last bar is {t - last_bar} ticks, not {bar_ticks}')
    return items, t


def item_ticks(items):
    return sum(it[2] if it[0] == 'note' else it[1] for it in items if it[0] in ('note', 'rest'))


def notes_timed(items):
    """[(start, midi, ticks)] of the notes."""
    out = []
    t = 0
    for it in items:
        if it[0] == 'note':
            out.append((t, it[1], it[2]))
            t += it[2]
        elif it[0] == 'rest':
            t += it[1]
    return out


# ------------------------------------------------------------------ patterns on the chords

def pattern_steps(pattern):
    """'8: R . O R' -> (step ticks, symbols)"""
    head, _, body = pattern.partition(':')
    return WHOLE // int(head), body.split()


def from_steps(symbols_at, total, step, pitch_of):
    """Items from a grid: a symbol a step; '-' holds the last note on, '.' is silence, else pitch_of(symbol, tick)
    gives the note (or None for silence, or ('arp', midi, a, b))."""
    items = []
    cur = None   # [kind, pitch, ticks, arp]
    def flush():
        if cur is None:
            return
        if cur[0] == 'rest':
            if items and items[-1][0] == 'rest':
                items[-1] = ('rest', items[-1][1] + cur[2])
            else:
                items.append(('rest', cur[2]))
        else:
            items.append(('arp', cur[3][0], cur[3][1]))
            items.append(('note', cur[1], cur[2], False))
    for k in range(total // step):
        t = k * step
        sym = symbols_at(t)
        if sym == '-' and cur is not None:
            cur[2] += step
            continue
        flush()
        p = None if sym in ('.', '-') else pitch_of(sym, t)
        if p is None:
            cur = ['rest', None, step, None]
        elif isinstance(p, tuple):
            cur = ['note', p[1], step, (p[2], p[3])]
        else:
            cur = ['note', p, step, (0, 0)]
    flush()
    # the arp settings only where they change
    out = []
    arp = None
    for it in items:
        if it[0] == 'arp':
            if (it[1], it[2]) != arp:
                arp = (it[1], it[2])
                out.append(it)
        else:
            out.append(it)
    return out


def lowest_at_or_above(pc, lo):
    n = lo + ((pc - lo) % 12)
    return n


def bass_items(pattern, timeline, total, bar_ticks, lo, scale_pcs):
    """The bass on the chords: R root (the chord's bass note) in [lo, lo+11], O its octave, 5 the fifth, 3 the third,
    7 the seventh (or the octave), L the fifth below, A a step leading into the next chord's root, - hold, . rest."""
    step, syms = pattern_steps(pattern)
    per_bar = bar_ticks // step

    def sym_at(t):
        return syms[(t % bar_ticks) // step % len(syms)] if len(syms) >= per_bar else syms[(t // step) % len(syms)]

    def pitch(sym, t):
        c = chord_at(timeline, t)
        r = lowest_at_or_above(c['bass'], lo)
        third = c['ivs'][1]
        fifth = c['ivs'][2] if len(c['ivs']) > 2 else 7
        if sym == 'R':
            return r
        if sym == 'O':
            return r + 12
        if sym == '5':
            return lowest_at_or_above((c['root'] + fifth) % 12, r)
        if sym == '3':
            return lowest_at_or_above((c['root'] + third) % 12, r)
        if sym == '7':
            return lowest_at_or_above((c['root'] + c['ivs'][3]) % 12, r) if len(c['ivs']) > 3 else r + 12
        if sym == 'L':
            return lowest_at_or_above((c['root'] + fifth) % 12, r) - 12
        if sym == 'A':  # toward the next chord's root: a scale step above or below it
            nxt = None
            for (ct, ln, ch) in timeline:
                if ct > t:
                    nxt = ch
                    break
            if nxt is None:
                nxt = timeline[0][2]
            target = lowest_at_or_above(nxt['bass'], lo)  # where its root will be played
            below = target - 1 if (target - 1) % 12 in scale_pcs else target - 2
            above = target + 1 if (target + 1) % 12 in scale_pcs else target + 2
            return below if abs(below - r) <= abs(above - r) and below >= lo - 2 else above
        raise ValueError(f'bass symbol {sym!r}?')
    return from_steps(sym_at, total, step, pitch)


def voicing(chord, lo, hi, prev):
    """Three of the chord's notes in [lo, hi], led smoothly from the last voicing."""
    pcs = [(chord['root'] + i) % 12 for i in chord['ivs']]
    if len(pcs) > 3:   # a seventh chord: root, third, seventh (the fifth goes)
        pcs = [pcs[0], pcs[1], pcs[3]]
    cands = []
    for start in range(lo, hi):  # each inversion, closed, from a chord note in the range
        if start % 12 not in pcs:
            continue
        chosen = sorted([start] + [start + (p - start) % 12 for p in pcs if p != start % 12])
        if chosen[-1] <= hi:
            cands.append(chosen)
    if not cands:
        raise ValueError(f'no voicing of {chord["sym"]} in {lo}..{hi}')
    if prev is None:
        mid = (lo + hi) / 2
        return min(cands, key=lambda v: abs(sum(v) / 3 - mid))
    return min(cands, key=lambda v: sum(abs(a - b) for a, b in zip(v, prev)))


def harmony_items(pattern, timeline, total, bar_ticks, lo, hi):
    """The second pulse on the chords: 1 2 3 the voicing's notes from the bottom, 4 the lowest an octave up, x the
    whole chord as an arpeggio on its lowest note, - hold, . rest."""
    step, syms = pattern_steps(pattern)
    per_bar = bar_ticks // step
    voicings = {}
    prev = None
    for (t, ln, c) in timeline:
        prev = voicing(c, lo, hi, prev)
        voicings[t] = prev

    def vo_at(t):
        best = None
        for (ct, ln, c) in timeline:
            if ct <= t:
                best = voicings[ct]
        return best

    def sym_at(t):
        return syms[(t % bar_ticks) // step % len(syms)] if len(syms) >= per_bar else syms[(t // step) % len(syms)]

    def pitch(sym, t):
        v = vo_at(t)
        if sym == 'x':
            return ('arp', v[0], v[1] - v[0], v[2] - v[0])
        if sym in '123':
            return v[int(sym) - 1]
        if sym == '4':
            return v[0] + 12
        raise ValueError(f'harmony symbol {sym!r}?')
    return from_steps(sym_at, total, step, pitch)


def thirds_below(items, scale_root, scale_pcs, steps=2):
    """The melody a diatonic third (steps=2) or sixth (steps=5) below."""
    out = []
    for it in items:
        if it[0] != 'note':
            out.append(it)
            continue
        n = it[1]
        if n % 12 in [(scale_root + p) % 12 for p in scale_pcs]:
            k = 0
            m = n
            while k < steps:
                m -= 1
                if m % 12 in [(scale_root + p) % 12 for p in scale_pcs]:
                    k += 1
        else:
            m = n - (3 if steps == 2 else 8)
        out.append(('note', m, it[2], it[3]))
    return out


def trem(items, step):
    """Held notes as quick repeats (a surf guitar's picking)."""
    out = []
    for it in items:
        if it[0] == 'note' and it[2] >= TPB and it[2] % step == 0:
            out += [('note', it[1], step, it[3] if k == 0 else False) for k in range(it[2] // step)]
        else:
            out.append(it)
    return out


def echo(items, delay, total):
    """The melody again, delay ticks late (what runs past the end is cut)."""
    out = [('rest', delay)]
    t = delay
    for it in items:
        if it[0] in ('inst', 'vol'):
            continue
        ln = it[2] if it[0] == 'note' else it[1]
        if t + ln > total:
            ln = total - t
        if ln <= 0:
            break
        out.append(('note', it[1], ln, it[3]) if it[0] == 'note' else ('rest', ln))
        t += ln
    if t < total:
        out.append(('rest', total - t))
    return out


def drum_items(bars, bar_ticks, patterns, where):
    """A pattern a bar: '16: k . h . s . h .' -> the drum voice's notes (each lasts to the next)."""
    items = []
    for b in range(bars):
        pat = patterns[b % len(patterns)]
        step, syms = pattern_steps(pat)
        if len(syms) * step != bar_ticks:
            raise ValueError(f'{where}: drum bar {pat!r} is {len(syms) * step} ticks, not {bar_ticks}')
        for s in syms:
            if s == '.':
                if items and items[-1][0] == 'rest':
                    items[-1] = ('rest', items[-1][1] + step)
                elif items and items[-1][0] == 'note':
                    items[-1] = ('note', items[-1][1], items[-1][2] + step, False)
                else:
                    items.append(('rest', step))
            else:
                if s not in DRUM_KEYS:
                    raise ValueError(f'{where}: drum {s!r}?')
                items.append(('note', DRUM_KEYS[s], step, False))
    return items


# ------------------------------------------------------------------ checks

def check_melody(items, timeline, bar_ticks, where, scale_pcs_abs):
    for (t, n, ln) in notes_timed(items):
        c = chord_at(timeline, t)
        # the strong beats (1 and 3 of four, 1 of three or six), or a long note on any beat
        strong = t % (bar_ticks // 2 if bar_ticks == 4 * TPB else bar_ticks) == 0
        held = t % TPB == 0 and ln >= TPB
        if (strong and ln >= TPB // 2 or held) and n % 12 not in c['pcs']:
            ok_tension = (n - c['root']) % 12 in (2, 9) and n % 12 in scale_pcs_abs  # a 9th or 6th sounds fine held
            if not ok_tension:
                warn(f'{where}: bar {t // bar_ticks + 1} beat {(t % bar_ticks) // TPB + 1}: '
                     f'{name_of(n)} over {c["sym"]}')


def name_of(n):
    return ['c', 'c#', 'd', 'd#', 'e', 'f', 'f#', 'g', 'g#', 'a', 'a#', 'b'][n % 12] + str(n // 12 - 1)


def check_range(items, lo, hi, where):
    for (t, n, ln) in notes_timed(items):
        if not lo <= n <= hi:
            warn(f'{where}: {name_of(n)} at tick {t} is out of {name_of(lo)}..{name_of(hi)}')


# ------------------------------------------------------------------ the bytes

MS_REST, MS_INST, MS_VOL, MS_ARP, MS_LOOP, MS_END, MS_SLUR = 0xF0, 0xF1, 0xF2, 0xF3, 0xFE, 0xFF, 0x80


def encode(items, loop_index):
    out = []
    for i, it in enumerate(items):
        if i == loop_index:
            out.append(MS_LOOP)
        k = it[0]
        if k == 'inst':
            out += [MS_INST, it[1]]
        elif k == 'vol':
            out += [MS_VOL, it[1]]
        elif k == 'arp':
            out += [MS_ARP, (it[1] << 4) | it[2]]
        elif k == 'rest':
            ln = it[1]
            while ln > 0:
                out += [MS_REST, min(ln, 240)]
                ln -= min(ln, 240)
        elif k == 'note':
            n, ln, slur = it[1], it[2], it[3]
            if not 1 <= n <= 108:  # (the drums are 1..)
                raise ValueError(f'note {n} out of range')
            first = True
            while ln > 0:
                part = min(ln, 240)
                out += [n | (MS_SLUR if (slur or not first) else 0), part]
                first = False
                ln -= part
    if loop_index == len(items):
        out.append(MS_LOOP)
    out.append(MS_END)
    return out


# ------------------------------------------------------------------ songs

PATTERN = re.compile(r'^\d+:')


class Builder:
    """A song: its sections, then its form (an intro, then the loop; a jingle has no loop)."""

    def __init__(self, track, name, bpm, meter=4, key=('C', 'major'), gain=1.0, insts=('lead', 'soft', 'bass'),
                 vols=(13, 9, 15, 11), bass_lo=40, harm=(60, 76), lead=(60, 91), instruments=None):
        self.track, self.name, self.bpm, self.gain = track, name, bpm, gain
        self.meter = meter
        self.bar = meter * TPB
        self.key_root = NOTE_PC[key[0][0].lower()] + {'#': 1, 'b': -1}.get(key[0][1:], 0)
        self.scale = SCALES[key[1]]
        self.scale_abs = {(self.key_root + p) % 12 for p in self.scale}
        self.insts, self.vols = insts, vols
        self.bass_lo, self.harm, self.lead = bass_lo, harm, lead
        self.instruments = instruments
        self.sections = {}
        self.intro, self.loop = [], []

    def section(self, name, bars, chords, melody=None, harmony=None, bass=None, drums=None, insts=None, vols=None,
                tremolo=0):
        """tremolo: the melody's notes of a quarter or more played as repeated notes this many ticks long."""
        where = f'{self.name}/{name}'
        total = bars * self.bar
        tl = chord_timeline(chords, bars, self.bar, TPB)
        insts = insts or self.insts
        vols = vols or self.vols
        chans = [[], [], [], []]
        # the melody
        mel = []
        if melody:
            mel, t = mml(melody, self.bar, self.instruments, where + ' melody')
            if t != total:
                raise ValueError(f'{where}: the melody is {t} ticks, {total} expected')
            check_melody(mel, tl, self.bar, where, self.scale_abs)
            check_range(mel, self.lead[0], self.lead[1], where + ' melody')
            if tremolo:
                mel = trem(mel, tremolo)
        else:
            mel = [('rest', total)]
        chans[0] = [('inst', self.instruments[insts[0]]), ('vol', vols[0]), ('arp', 0, 0)] + mel
        # the harmony
        h = harmony
        if h is None:
            hi = [('rest', total)]
        elif PATTERN.match(h):
            hi = harmony_items(h, tl, total, self.bar, self.harm[0], self.harm[1])
        elif h in ('thirds', 'sixths'):
            hi = thirds_below(mel, self.key_root, self.scale, 2 if h == 'thirds' else 5)
        elif h.startswith('echo'):
            hi = echo(mel, int(h.split()[1]), total)
        else:
            hi, t = mml(h, self.bar, self.instruments, where + ' harmony')
            if t != total:
                raise ValueError(f'{where}: the harmony is {t} ticks, {total} expected')
        if item_ticks(hi) != total:
            raise ValueError(f'{where}: the harmony is {item_ticks(hi)} ticks, {total} expected')
        chans[1] = [('inst', self.instruments[insts[1]]), ('vol', vols[1]), ('arp', 0, 0)] + hi
        # the bass
        if bass is None:
            bi = [('rest', total)]
        elif PATTERN.match(bass):
            bi = bass_items(bass, tl, total, self.bar, self.bass_lo, self.scale_abs)
        else:
            bi, t = mml(bass, self.bar, self.instruments, where + ' bass')
            if t != total:
                raise ValueError(f'{where}: the bass is {t} ticks, {total} expected')
        check_range(bi, 28, 72, where + ' bass')
        chans[2] = [('inst', self.instruments[insts[2]]), ('vol', vols[2]), ('arp', 0, 0)] + bi
        # the drums
        di = drum_items(bars, self.bar, drums, where) if drums else [('rest', total)]
        chans[3] = [('vol', vols[3])] + di
        for c in range(4):
            if item_ticks(chans[c]) != total:
                raise ValueError(f'{where}: voice {c} is {item_ticks(chans[c])} ticks, {total} expected')
        self.sections[name] = (chans, total)
        return self

    def form(self, intro, loop):
        self.intro, self.loop = intro, loop
        return self

    def build(self):
        chans = [[], [], [], []]
        loop_at = [None] * 4
        intro_ticks = loop_ticks = 0
        for i, name in enumerate(self.intro + self.loop):
            if i == len(self.intro):
                loop_at = [len(c) for c in chans]
            sec, total = self.sections[name]
            for c in range(4):
                chans[c] += sec[c]
            if i < len(self.intro):
                intro_ticks += total
            else:
                loop_ticks += total
        if not self.loop:
            loop_at = [None] * 4
        streams = [encode(chans[c], loop_at[c] if loop_at[c] is not None else -1) for c in range(4)]
        return {'track': self.track, 'name': self.name, 'bpm': self.bpm, 'gain': self.gain,
                'loops': 1 if self.loop else 0, 'intro': intro_ticks, 'loop': loop_ticks, 'streams': streams}


def c_bytes(data, indent='  ', per=24):
    rows = []
    for i in range(0, len(data), per):
        rows.append(indent + ', '.join(f'0x{b:02X}' for b in data[i:i + per]) + ',')
    return '\n'.join(rows)


def cf(x):
    """A float for C."""
    t = f'{float(x):.6g}'
    return (t if ('.' in t or 'e' in t) else t + '.0') + 'f'


def header(instruments, inst_rows, drums, songs, tracks):
    out = ['// The music\'s data, made by tools/music/songs.py from the songs written there: change them there and run',
           '// it again (python3 tools/music/songs.py). music_synth.h plays them.',
           '#pragma once', '']
    out.append('static const MusicInst MUSIC_INST[] = {')
    for name, r in inst_rows:
        out.append(f'  {{ {r[0]}, {", ".join(cf(x) for x in r[1:])} }},  // {name}')
    out.append('};')
    out.append('static const MusicDrum MUSIC_DRUMS[] = {')
    for key, name, r in drums:
        vals = [cf(x) for x in r[:6]] + [str(int(r[6]))] + [cf(x) for x in r[7:]]
        out.append(f'  {{ {", ".join(vals)} }},  // {key} {name}')
    out.append('};')
    out.append('')
    for s in songs:
        for c in range(4):
            out.append(f'static const uint8_t MS_{s["track"]}_{c}[] = {{')
            out.append(c_bytes(s['streams'][c]))
            out.append('};')
    out.append('')
    out.append('static const MusicSong MUSIC_SONGS[MUS_COUNT - 1] = {')
    by = {s['track']: s for s in songs}
    for t in tracks:
        s = by[t]
        ch = ', '.join(f'MS_{t}_{c}' for c in range(4))
        out.append(f'  {{ "{s["name"]}", {cf(s["bpm"])}, {cf(s["gain"])}, {s["loops"]}, {s["intro"]}, {s["loop"]}, '
                   f'{{ {ch} }} }},  // MUS_{t}')
    out.append('};')
    return '\n'.join(out) + '\n'
