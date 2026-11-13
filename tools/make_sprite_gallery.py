#!/usr/bin/env python3
"""The KnomiDex (the sprite gallery), published with the installer at https://fedorsosnin.github.io/PokeKnomiGochi/sprites/
(.github/workflows/pages.yml builds it there on every deploy; nothing it makes is kept in git).

All 151 Pokemon, regular and shiny, in every view the game has: standing, turning round (the Pokedex entry's
turn), walking, from behind, asleep, hopping, attacking, hurt, attacking from behind (a battle), and the extras
some have (eating, posing, nodding, sitting, a deep breath). It reads the packs the game loads
(tools/sdcard/mons/pNNN.bin and psNNN.bin, made by pack_pmd.py), and the page plays them the way the game does:
each frame on its own feet (sdmon.cpp's anchors, Ivysaur's legs and Magikarp's nap included), frame times as packed,
and a shiny's four gold sparkles (drawSparkles()). The cry button on a Pokemon's page plays its cry, as
the game plays it (src/cry.h): the pieces are tools/voices.json, which `make -C host voices` writes.

Each Pokemon's page also says what the game's Pokedex says about it (its fact), its base stats, how it evolves, every
move it learns with the game's power for it and when it's on its battle buttons, and what it's strong and weak
against; several picked together are compared (match-ups). All of that is the game's own tables and battle rules
(dex.h, moves.h, dexfacts.h, battle.h), as tools/dex.json, which `make -C host dex` writes (host/dex_json.h; the
host tests check it's up to date). The owned marks are the viewer's own, kept in their browser.

  <out>/index.html            the page (tools/sprite_gallery.html, with the data put in)
  <out>/img/NNN-name.png      each form's frames, one row per view (indexed PNG; the page cuts them up)
  <out>/img/NNN-name-shiny.png

Each sprite is credited to its artists, from SpriteCollab's tracker.json and credit_names.txt (fetched into
tools/pmd_cache/, not in git; --refresh fetches them again).

  python3 tools/make_sprite_gallery.py [out folder, default build/sprites] [--refresh]
"""
import json
import os
import struct
import sys
import urllib.request

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import dex_data  # noqa: E402

MONS = os.path.join(HERE, 'sdcard', 'mons')
CACHE = os.path.join(HERE, 'pmd_cache')
PAGE = os.path.join(HERE, 'sprite_gallery.html')  # the page, which this fills in
VOICES = os.path.join(HERE, 'voices.json')         # each one's cry (make -C host voices)
DEX = os.path.join(HERE, 'dex.json')               # the game's Pokedex, moves and type chart (make -C host dex)
COLLAB = 'https://raw.githubusercontent.com/PMDCollab/SpriteCollab/master'
WALKL, WALKR, SLEEP, WALKD = 1, 2, 3, 12
# the room a card gives a sprite, in CSS px (the page's canvases): what it fits a Pokemon's standing view into
FIT_W, FIT_H = 118, 96


def fetch(name, refresh):
    path = os.path.join(CACHE, name)
    if refresh or not os.path.exists(path):
        os.makedirs(CACHE, exist_ok=True)
        req = urllib.request.Request(f'{COLLAB}/{name}', headers={'User-Agent': 'Mozilla/5.0'})
        open(path, 'wb').write(urllib.request.urlopen(req, timeout=60).read())
    return path


def credits(refresh):
    """dex -> (the regular sprite's artists, the shiny one's), by name."""
    tracker = json.load(open(fetch('tracker.json', refresh), encoding='utf-8'))
    names = {}
    for line in open(fetch('credit_names.txt', refresh), encoding='utf-8').read().splitlines()[1:]:
        cols = line.split('\t')
        if len(cols) >= 2:
            names[cols[1].strip()] = cols[0].strip()

    def who(node):
        c = node.get('sprite_credit') or {}
        ids = [c.get('primary')] + list(c.get('secondary') or [])
        # (an artist SpriteCollab lists without a name is only a Discord ID there: say so instead)
        who = [names.get(i) or ('unnamed artist' if i.startswith('<@') else i) for i in ids if i]
        return ', '.join(dict.fromkeys(who)) or 'unknown'

    out = {}
    for num, *_ in dex_data.DEX:
        e = tracker[f'{num:04d}']
        out[num] = (who(e), who(e['subgroups']['0000']['subgroups']['0001']))
    return out


def read_pack(path):
    """A pack's palette (RGB) and its actions: id -> (w, h, nf, ms, data), the tail's and back views' too."""
    b = open(path, 'rb').read()
    if b[:4] != b'TPK2':
        raise RuntimeError(f'{path}: not TPK2')
    (pc,) = struct.unpack_from('<H', b, 5)
    pal = [((c >> 11) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31)
           for c in struct.unpack_from(f'<{pc}H', b, 7)]
    acts = {}

    def section(p, n):
        for _ in range(n):
            aid, w, h, nf = b[p:p + 4]
            ms = [m or 100 for m in struct.unpack_from(f'<{nf}H', b, p + 4)]
            d = p + 4 + nf * 2
            acts[aid] = (w, h, nf, ms, b[d:d + w * h * nf])
            p = d + w * h * nf
        return p

    p = section(7 + pc * 2, b[4])
    while b[p:p + 4] in (b'TPKX', b'TPKY'):
        p = section(p + 5, b[p + 4])
    return pal, acts


def anchors(dex, aid, w, h, nf, data):
    """sdmon.cpp's (V2): each frame's feet (low), the lowest of them (base), the highest row drawn (top), and the
    columns it fills (left, right), so the page stands each frame where the game does."""
    frames = [data[f * w * h:(f + 1) * w * h] for f in range(nf)]
    low, base, top, left, right = [], 1, h, w, 0
    for fr in frames:
        rows = [r for r in range(h) if any(fr[r * w + c] != 0xFF for c in range(w))]
        low.append(rows[-1] + 1 if rows else 0)
        if rows:
            base = max(base, rows[-1] + 1)
            top = min(top, rows[0])
            cols = [c for c in range(w) if any(fr[r * w + c] != 0xFF for r in rows)]
            left, right = min(left, cols[0]), max(right, cols[-1] + 1)
    low = [v or base for v in low]
    if top >= base:
        top = 0
    facing_you = aid == WALKD or (aid < WALKD and aid not in (WALKL, WALKR))
    if (dex == 3 or (dex == 2 and aid != SLEEP)) and facing_you:  # Ivysaur and Venusaur stand by their legs
        legs_base = 0
        for f, fr in enumerate(frames):
            cols = [c for c in range(w) if any(fr[r * w + c] != 0xFF for r in range(h))]
            legs = 0
            if cols:
                for k in range(3):
                    for c in (cols[0] + k, cols[-1] - k):
                        for r in range(h - 1, -1, -1):
                            if fr[r * w + c] != 0xFF:
                                legs = max(legs, r + 1)
                                break
            if legs:
                low[f] = legs
            legs_base = max(legs_base, low[f])
        if legs_base:
            base = legs_base
    sinks = []
    if dex in (50, 51):  # Diglett and Dugtrio stand in their mound
        sinks.append(4 if dex == 50 else 5)
    if aid == SLEEP and dex in (9, 129):  # Blastoise and Magikarp asleep lie on their bodies
        sinks.append(4 if dex == 9 else 3)
    for s in sinks:
        low = [v - s if v > s else 1 for v in low]
        base = base - s if base > s else 1
    return low, base, top, left, right


def form(dex, path, out_png):
    """One form's sheet (one row per view, its frames side by side) and what the page needs to play it."""
    pal, acts = read_pack(path)
    ids = sorted(acts)
    sheet_w = max(acts[a][0] * acts[a][2] for a in ids)
    sheet_h = sum(acts[a][1] for a in ids)
    sheet = Image.new('P', (sheet_w, sheet_h), 255)
    meta, y = {}, 0
    for a in ids:
        w, h, nf, ms, data = acts[a]
        for f in range(nf):
            sheet.paste(Image.frombytes('P', (w, h), data[f * w * h:(f + 1) * w * h]), (f * w, y))
        low, base, top, left, right = anchors(dex, a, w, h, nf, data)
        meta[a] = [y, w, h, base, top, left, right, ms, low]
        y += h
    flat = [v for c in pal for v in c]
    sheet.putpalette(flat + [0] * (768 - len(flat)))
    sheet.save(out_png, transparency=255, optimize=True)
    _, _, _, base, top, left, right, _, _ = meta[0]
    return meta, (right - left, base - top)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    out = os.path.abspath(args[0] if args else os.path.join(ROOT, 'build', 'sprites'))
    who = credits('--refresh' in sys.argv)
    img = os.path.join(out, 'img')
    os.makedirs(img, exist_ok=True)
    for old in os.listdir(img):
        if old.endswith('.png'):
            os.remove(os.path.join(img, old))

    voices = json.load(open(VOICES, encoding='utf-8')) if os.path.exists(VOICES) else {}
    dex = json.load(open(DEX, encoding='utf-8'))
    types = [t.lower() for t in dex['types']]
    mons = []
    for num, slug, name, typ, *_ in dex_data.DEX:
        game = dex['mons'][str(num)]
        if types[game['type']] != typ:
            sys.exit(f'{num} {name}: tools/dex.json says {types[game["type"]]}, dex_data.py {typ} (make -C host dex)')
        base = f'{num:03d}-{slug}'
        reg, (sw, sh) = form(num, os.path.join(MONS, f'p{num:03d}.bin'), os.path.join(img, f'{base}.png'))
        shi, _ = form(num, os.path.join(MONS, f'ps{num:03d}.bin'), os.path.join(img, f'{base}-shiny.png'))
        fit = min(FIT_W / max(sw, 1), FIT_H / max(sh, 1))
        scale = 3 if fit >= 3 else 2 if fit >= 2 else round(fit, 2)  # whole pixels when it fits
        mons.append({'n': num, 'id': base, 'name': name, 'type': typ, 'c': dex_data.TYPE_ACCENTS.get(typ, '#888'),
                     's': scale, 'cr': list(who[num]), 'f': [reg, shi], 'v': voices.get(str(num), []), 'g': game})

    data = json.dumps(mons, separators=(',', ':'))
    tables = json.dumps({'types': types, 'colors': [dex_data.TYPE_ACCENTS[t] for t in types], 'chart': dex['chart'],
                         'moves': dex['moves']}, separators=(',', ':'))
    page = open(PAGE, encoding='utf-8').read()
    page = page.replace('/*DATA*/[]', data.replace('</', '<\\/')).replace('/*GAME*/{}', tables.replace('</', '<\\/'))
    open(os.path.join(out, 'index.html'), 'w', encoding='utf-8').write(page)
    size = sum(os.path.getsize(os.path.join(img, f)) for f in os.listdir(img))
    print(f'{os.path.relpath(out, ROOT)}: {len(mons)} Pokemon, {len(os.listdir(img))} sheets ({size // 1024} KB), '
          f'index.html {os.path.getsize(os.path.join(out, "index.html")) // 1024} KB')


if __name__ == '__main__':
    main()
