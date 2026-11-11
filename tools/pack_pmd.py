#!/usr/bin/env python3
"""Packer for PMD (SpriteCollab) sprites for the main screen.

Generates /mons/pNNN.bin (and shiny psNNN.bin) in the multi-action TPK2 format:

  char[4] "TPK2"
  u8  nActs
  u16 palCount
  u16 pal[palCount]                  (RGB565)
  per action:
    u8 id, u8 w, u8 h, u8 nFrames
    u16 ms[nFrames]
    u8 data[w*h*nFrames]             (indices, 0xFF transparent)
  then the tail (V2):
    char[4] "TPKX"
    u8  nTail
    nTail more actions, laid out as above
  then the back views (V2, 2.0.6 on):
    char[4] "TPKY"
    u8  nBack
    nBack more actions, laid out as above

Acciones: 0 Idle, 1 WalkL, 2 WalkR, 3 Sleep, 4 Eat, 5 Hurt, 6 Attack,
7 Pose, 8 Hop, 9 Nod, 10 DeepBreath, 11 Sit. Missing ones are skipped.
In the tail: 12 WalkD (walking toward you) and 13 WalkU (walking away), for V2's walk, and the
diagonals 14 WalkDR, 15 WalkUR, 16 WalkUL, 17 WalkDL, for the Pokedex entry's turn.
After the tail, the back views: 18 Idle, 19 Attack and 20 Hurt seen from behind, facing up-right (row 3 of
each sheet), for your own Pokemon in a battle, facing the foe as in the Game Boy games (Fedor). Only a sheet
with all eight rows gives one; without it, the Pokemon faces you as before. They are a section of their own
because the loaders before them stop at an action number they don't know: 2.0.5's reads the tail and stops,
so a Knomi that goes back to it after a failed update still loads these packs.
1.51's loader stops after nActs and never reads the tail, so these packs still work there.

  python3 tools/pack_pmd.py             # all 151, normal + shiny
  python3 tools/pack_pmd.py 7 25        # specific dex numbers
  python3 tools/pack_pmd.py normal 1 4  # normal only
  python3 tools/pack_pmd.py walk        # adds (or redoes) the tail on the packs already in sdcard/mons,
                                        # leaving everything before it byte for byte (fetches only Walk)
  python3 tools/pack_pmd.py back        # adds (or redoes) the back views on the packs already there, leaving
                                        # everything before them (fetches only Idle, Attack and Hurt)
"""
import os
import struct
import sys
import urllib.request
import xml.etree.ElementTree as ET
from PIL import Image

OUT = os.path.join(os.path.dirname(__file__), 'sdcard', 'mons')
CACHE = os.path.join(os.path.dirname(__file__), 'pmd_cache')
BASE = 'https://raw.githubusercontent.com/PMDCollab/SpriteCollab/master/sprite'
SLOW = 1.4          # the original PMD pace feels fast on the tamagotchi
MIN_MS = 70
ALPHA_T = 128

# (id, action name, sheet row) — row None = 0 if there is only one
# sheet directions: 0 down, 2 RIGHT, 6 LEFT (verified on the board)
ACTIONS = [
    (0, 'Idle', 0),
    (1, 'Walk', 6),   # left
    (2, 'Walk', 2),   # right
    (3, 'Sleep', 0),
    (4, 'Eat', 0),
    (5, 'Hurt', 0),
    (6, 'Attack', 0),
    (7, 'Pose', 0),
    (8, 'Hop', 0),
    (9, 'Nod', 0),
    (10, 'DeepBreath', 0),
    (11, 'Sit', 0),
]
# V2's walk, in the tail: Walk row 0 (down, toward you) and row 4 (up, away from you), then the four
# diagonals (rows 1, 3, 5, 7: down-right, up-right, up-left, down-left) for the Pokedex entry's turn
TAIL = [
    (12, 'Walk', 0),
    (13, 'Walk', 4),
    (14, 'Walk', 1),
    (15, 'Walk', 3),
    (16, 'Walk', 5),
    (17, 'Walk', 7),
]
# the back views (V2): standing, attacking and hurt from behind, facing up-right, toward the foe in a battle;
# with the action seen from the front that each is checked against (cut from row 0 of the same sheet)
BACK = [
    (18, 'Idle', 3, 0),
    (19, 'Attack', 3, 6),
    (20, 'Hurt', 3, 5),
]


def fetch(url, dest):
    if os.path.exists(dest):
        return True
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        data = urllib.request.urlopen(req, timeout=30).read()
        open(dest, 'wb').write(data)
        return True
    except Exception:
        return False


def rgb565(r, g, b):
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)


def load_animdata(folder):
    path = os.path.join(folder, 'AnimData.xml')
    anims = {}
    tree = ET.parse(path)
    for a in tree.getroot().find('Anims'):
        name = a.find('Name').text
        if a.find('FrameWidth') is None:
            # alias (CopyOf): points to another animation
            copy = a.find('CopyOf')
            if copy is not None:
                anims[name] = ('copy', copy.text)
            continue
        anims[name] = (int(a.find('FrameWidth').text), int(a.find('FrameHeight').text),
                       [int(d.text) for d in a.find('Durations')], name)
    # resolve aliases (the PNG is the original animation's)
    for k, v in list(anims.items()):
        if isinstance(v, tuple) and v[0] == 'copy':
            anims[k] = anims.get(v[1])
    return anims


def cut_frames(im, fw, fh, durs, row, index):
    """The frames of one sheet row as palette indices (index(rgb) gives a colour's), and their times."""
    rows = im.size[1] // fh
    r = row if rows > row else 0
    nf = min(len(durs), im.size[0] // fw, 24)
    data = bytearray()
    for i in range(nf):
        fr = im.crop((i * fw, r * fh, (i + 1) * fw, (r + 1) * fh))
        for px in fr.getdata():
            data.append(0xFF if px[3] < ALPHA_T else index(px[:3]))
    ms = [max(MIN_MS, round(d * 1000 / 60 * SLOW)) for d in durs[:nf]]
    return nf, ms, bytes(data)


def write_acts(f, acts):
    for aid, fw, fh, nf, ms, data in acts:
        f.write(struct.pack('<4B', aid, fw, fh, nf))
        f.write(struct.pack(f'<{nf}H', *ms))
        f.write(data)


def write_tail(f, tail, magic=b'TPKX'):
    if tail:
        f.write(magic + struct.pack('<B', len(tail)))
        write_acts(f, tail)


def sheet_folder(dexnum, shiny):
    sub = '/0000/0001' if shiny else ''
    return os.path.join(CACHE, f'{dexnum:04d}{"s" if shiny else ""}'), f'{BASE}/{dexnum:04d}{sub}'


def pack(dexnum, shiny=False):
    folder, base = sheet_folder(dexnum, shiny)
    if not fetch(f'{base}/AnimData.xml', os.path.join(folder, 'AnimData.xml')):
        raise RuntimeError('no AnimData.xml')
    anims = load_animdata(folder)

    colmap, pal = {}, []

    def index(k):
        if k not in colmap:
            if len(pal) >= 255:
                # nearest (rare in PMD, short palettes)
                k2 = min(colmap, key=lambda c: sum((a-b)**2 for a, b in zip(c, k)))
                colmap[k] = colmap[k2]
            else:
                colmap[k] = len(pal)
                pal.append(k)
        return colmap[k]

    packed, tail, back = [], [], []
    for aid, name, row in ACTIONS + TAIL + [b[:3] for b in BACK]:
        if name not in anims or anims[name] is None:
            continue
        fw, fh, durs, srcname = anims[name]
        png = os.path.join(folder, f'{srcname}-Anim.png')
        if not fetch(f'{base}/{srcname}-Anim.png', png):
            continue
        im = Image.open(png).convert('RGBA')
        if aid >= 18 and im.size[1] // fh <= row:
            continue  # a sheet of one row: no back view (it faces you)
        nf, ms, data = cut_frames(im, fw, fh, durs, row, index)
        (back if aid >= 18 else tail if aid >= 12 else packed).append((aid, fw, fh, nf, ms, data))

    if not any(p[0] == 0 for p in packed):
        raise RuntimeError('no Idle')

    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, f'p{"s" if shiny else ""}{dexnum:03d}.bin')
    with open(path, 'wb') as f:
        f.write(b'TPK2')
        f.write(struct.pack('<BH', len(packed), len(pal)))
        for r, g, b in pal:
            f.write(struct.pack('<H', rgb565(r, g, b)))
        write_acts(f, packed)
        write_tail(f, tail)
        write_tail(f, back, b'TPKY')
    kb = os.path.getsize(path) / 1024
    print(f"  -> p{'s' if shiny else ''}{dexnum:03d}.bin: {len(packed)} actions + {len(tail)} in the tail, "
          f"{len(pal)} colors, {kb:.0f} KB")


def rgb565_of(k):
    return rgb565(*k)


def read_pack(path):
    """A pack's palette, its counted actions (by id, and as bytes) and its tail (as bytes, magic and all)."""
    buf = open(path, 'rb').read()
    if buf[:4] != b'TPK2':
        raise RuntimeError('not TPK2')
    nacts = buf[4]
    (palcount,) = struct.unpack_from('<H', buf, 5)
    pal = list(struct.unpack_from(f'<{palcount}H', buf, 7))
    p = 7 + palcount * 2
    acts = {}
    for _ in range(nacts):
        aid, w, h, nf = buf[p:p + 4]
        size = 4 + nf * 2 + w * h * nf
        acts[aid] = buf[p:p + size]
        p += size
    body = buf[7 + palcount * 2:p]
    tail = b''
    if buf[p:p + 4] == b'TPKX':
        q = p + 5
        for _ in range(buf[p + 4]):
            aid, w, h, nf = buf[q:q + 4]
            q += 4 + nf * 2 + w * h * nf
        tail, p = buf[p:q], q
    return nacts, pal, acts, body, tail


def add_back(dexnum, shiny=False):
    """Puts the back views (BACK) on a pack that's already there, after its tail, keeping every byte of its
    actions: its colours keep their numbers and new ones go on the end of the palette. Fetches only AnimData.xml
    and the Idle, Attack and Hurt sheets. Each sheet is checked against the action packed from its front row:
    the same frames, colour for colour (a retouched colour standing for an old one is allowed: the back view
    then takes the pack's colour). A sheet that has changed more than that is skipped, and the Pokemon faces you
    in that action as before."""
    path = os.path.join(OUT, f'p{"s" if shiny else ""}{dexnum:03d}.bin')
    nacts, pal, acts, body, tail = read_pack(path)
    folder, base = sheet_folder(dexnum, shiny)
    if not fetch(f'{base}/AnimData.xml', os.path.join(folder, 'AnimData.xml')):
        raise RuntimeError('no AnimData.xml')
    anims = load_animdata(folder)
    back, notes = [], []
    for aid, name, row, front in BACK:
        if name not in anims or anims[name] is None:
            notes.append(f'no {name}')
            continue
        fw, fh, durs, srcname = anims[name]
        png = os.path.join(folder, f'{srcname}-Anim.png')
        if not fetch(f'{base}/{srcname}-Anim.png', png):
            notes.append(f'no {name} sheet')
            continue
        im = Image.open(png).convert('RGBA')
        if im.size[1] // fh <= row:
            notes.append(f'{name}: one row')
            continue
        remap = {}
        if front in acts:  # the sheet against the action packed from its front row
            a = acts[front]
            codes = {}
            nf, ms, data = cut_frames(im, fw, fh, durs, 0, lambda k: codes.setdefault(rgb565_of(k), len(codes)))
            backc = {c: v for v, c in codes.items()}
            old = [None if i == 0xFF else pal[i] for i in a[4 + a[3] * 2:]]
            new = [None if i == 0xFF else backc[i] for i in data]
            same = (a[1], a[2], a[3]) == (fw, fh, nf) and [v is None for v in old] == [v is None for v in new]
            if same:
                kept = set()
                for o, n in zip(old, new):
                    if o == n:
                        kept.add(n)
                    elif remap.setdefault(n, o) != o:
                        same = False
                        break
                if kept & set(remap):
                    same = False
            if not same:
                notes.append(f'{name}: the sheet has changed since it was packed, skipped')
                continue

        def index(k):
            v = rgb565_of(k)
            v = remap.get(v, v)
            if v in pal:
                return pal.index(v)
            if len(pal) >= 255:
                return min(range(len(pal)), key=lambda i: abs(pal[i] - v))
            pal.append(v)
            return len(pal) - 1

        nf, ms, data = cut_frames(im, fw, fh, durs, row, index)
        back.append((aid, fw, fh, nf, ms, data))
    with open(path, 'wb') as f:
        f.write(b'TPK2')
        f.write(struct.pack('<BH', nacts, len(pal)))
        f.write(struct.pack(f'<{len(pal)}H', *pal))
        f.write(body)
        f.write(tail)
        write_tail(f, back, b'TPKY')
    print(f"  -> p{'s' if shiny else ''}{dexnum:03d}.bin: {len(back)} back views"
          f"{' (' + '; '.join(notes) + ')' if notes else ''}, {os.path.getsize(path) / 1024:.0f} KB")


def add_tail(dexnum, shiny=False):
    """Puts the tail (V2's walk rows) on a pack that's already there, keeping every byte before it: its
    colours keep their numbers and new ones go on the end of the palette. Fetches only AnimData.xml and
    Walk. Also checks the sheet against the pack: the left and right walks cut from it now must match the
    packed ones, or SpriteCollab has changed the sprite since (then it says so and leaves the pack alone)."""
    path = os.path.join(OUT, f'p{"s" if shiny else ""}{dexnum:03d}.bin')
    buf = open(path, 'rb').read()
    if buf[:4] != b'TPK2':
        raise RuntimeError('not TPK2')
    nacts = buf[4]
    (palcount,) = struct.unpack_from('<H', buf, 5)
    pal = list(struct.unpack_from(f'<{palcount}H', buf, 7))
    p = 7 + palcount * 2
    acts = {}
    for _ in range(nacts):
        aid, w, h, nf = buf[p:p + 4]
        size = 4 + nf * 2 + w * h * nf
        acts[aid] = buf[p:p + size]
        p += size
    body = buf[7 + palcount * 2:p]  # the counted actions, as they are

    folder, base = sheet_folder(dexnum, shiny)
    if not fetch(f'{base}/AnimData.xml', os.path.join(folder, 'AnimData.xml')):
        raise RuntimeError('no AnimData.xml')
    anims = load_animdata(folder)
    if 'Walk' not in anims or anims['Walk'] is None:
        raise RuntimeError('no Walk')
    fw, fh, durs, srcname = anims['Walk']
    png = os.path.join(folder, f'{srcname}-Anim.png')
    if not fetch(f'{base}/{srcname}-Anim.png', png):
        raise RuntimeError('no Walk sheet')
    im = Image.open(png).convert('RGBA')

    # The sheet must still be the one the left and right walks were cut from: the same frames, compared by
    # colour. Retouched colours alone (each new one standing for one old one) are allowed: the tail is then
    # cut with the pack's colours, so the Pokemon looks the same whichever way it walks.
    remap, kept = {}, set()
    for aid, row in ((1, 6), (2, 2)):
        if aid not in acts:
            continue
        a = acts[aid]
        codes = {}
        nf, ms, data = cut_frames(im, fw, fh, durs, row, lambda k: codes.setdefault(rgb565_of(k), len(codes)))
        back = {c: v for v, c in codes.items()}
        old = [None if i == 0xFF else pal[i] for i in a[4 + a[3] * 2:]]
        new = [None if i == 0xFF else back[i] for i in data]
        if (a[1], a[2], a[3]) != (fw, fh, nf) or list(struct.unpack_from(f'<{nf}H', a, 4)) != ms or \
                [v is None for v in old] != [v is None for v in new]:
            raise RuntimeError(f'the Walk sheet has changed since action {aid} was packed')
        for o, n in zip(old, new):
            if o == n:
                kept.add(n)
            elif remap.setdefault(n, o) != o:
                raise RuntimeError(f'the Walk sheet has changed since action {aid} was packed')
    if kept & set(remap):  # a colour that stands for another in one place and for itself in another
        raise RuntimeError('the Walk sheet has changed since the walks were packed')
    if remap:
        print(f'  (the sheet has {len(remap)} retouched colours: the tail keeps the pack\'s)')

    def index(k):  # by its RGB565, which is what the palette holds
        v = rgb565_of(k)
        v = remap.get(v, v)
        if v in pal:
            return pal.index(v)
        if len(pal) >= 255:
            return min(range(len(pal)), key=lambda i: abs(pal[i] - v))
        pal.append(v)
        return len(pal) - 1

    tail = []
    for aid, name, row in TAIL:
        nf, ms, data = cut_frames(im, fw, fh, durs, row, index)
        tail.append((aid, fw, fh, nf, ms, data))
    rest = b''  # the back views after the old tail, if any: kept as they are
    q = 7 + palcount * 2 + len(body)
    if buf[q:q + 4] == b'TPKX':
        q += 5
        for _ in range(buf[q - 1]):
            aid, w, h, nf = buf[q:q + 4]
            q += 4 + nf * 2 + w * h * nf
    if buf[q:q + 4] == b'TPKY':
        rest = buf[q:]
    with open(path, 'wb') as f:
        f.write(b'TPK2')
        f.write(struct.pack('<BH', nacts, len(pal)))
        f.write(struct.pack(f'<{len(pal)}H', *pal))
        f.write(body)
        write_tail(f, tail)
        f.write(rest)
    added = len(pal) - palcount
    print(f"  -> p{'s' if shiny else ''}{dexnum:03d}.bin: tail of {len(tail)}"
          f"{f', {added} new colours' if added else ''}, {os.path.getsize(path) / 1024:.0f} KB")


if __name__ == '__main__':
    args = sys.argv[1:]
    normal_only = 'normal' in args
    walk_only = 'walk' in args
    back_only = 'back' in args
    nums = [int(a) for a in args if a.isdigit()] or list(range(1, 152))
    failures = []
    for n in nums:
        for sh in ([False] if normal_only else [False, True]):
            try:
                print(f"#{n:03d}{' shiny' if sh else ''}")
                (add_tail if walk_only else add_back if back_only else pack)(n, sh)
            except Exception as e:
                print(f"  FAILED: {e}")
                failures.append((n, sh))
    print(f"FAILURES: {failures}" if failures else "ALL PACKED")
