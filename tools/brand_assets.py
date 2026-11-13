#!/usr/bin/env python3
"""The game's name and mark as pixel art: Catch 151, its Pokedex (Catch 151 Dex), and their icons.

  ~/.venvs/esp/bin/python3 tools/brand_assets.py     (needs Pillow)

Everything is drawn here a pixel at a time in the game's own colours, so it stays crisp at 1x and at any whole multiple:
- the icon: the apps' icon frame (a red top, a dark band, a white bottom, rounded corners) round the Knomi's round
  screen. The game's screen shows its island map (the five places round the Wild middle, as the Travel page draws
  them); the Pokedex's shows a shiny's sparkle (drawSparkle's gold and white, as a four-point star).
- the name: CATCH in cream and 151 in red, in chunky 8 px letters drawn for it, sharing one dark outline with a shadow
  under it (the round buttons' dark ring, lit along the top, shaded along the bottom).
- the signs after the name: the game's wooden name board (v2SignIn) in the game's 5x7 font: gold for DEX, wood for a
  device (FOR KNOMI, FOR MAC...).

Writes tools/assets/brand/ (each picture as an SVG and as PNGs at 1x, 2x and 3x; the icons at the sizes browsers and the
apps use; see its README.md), src/brand_logo.h (the logo, for the game's About page to draw) and webgame/assets/logo.svg
(the top of the web game's page).
"""
import math
import os
import re

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
OUT = os.path.join(ROOT, 'tools', 'assets', 'brand')

# ---- the game's colours ----
INK = '#1b1b25'                                  # the round buttons' ring, the dark band
RED, RED_L, RED_D = '#e8503a', '#f58c78', '#b8321f'  # the X button
OFFW, SHADE = '#f4f1e6', '#d6d0bd'               # the ball's white, in shade
CREAM, CREAM_L, CREAM_D = '#fff3dc', '#ffffff', '#e6d3b0'  # a sign's letters
GOLD_S, GOLD_SL, GOLD_SD = '#ffcd28', '#ffe27a', '#e0a800'  # a shiny's sparkle
SCREEN = '#262633'
SEA, SEA_L, SAND, SAND_L = '#4f93c4', '#7cc4ea', '#e2cf98', '#f8ecc4'  # the island map (v2PaintIsle)
WEDGES = [('#8fce7a', '#a6dc90'), ('#6c7190', '#8a8fa6'), ('#ecdcac', '#f8ecc4'), ('#2e2758', '#3e3470'),
          ('#3f8a4c', '#58b868')]  # Field (top), Cave, Beach, Mansion, Forest, clockwise
WILD = ('#5f9e3e', '#79bd4f')
WOOD_EDGE, WOOD, WOOD_L, GOLD, GOLD_L = '#3a2414', '#8a5a30', '#b07a45', '#c08410', '#f0c04a'  # the signs

# ---- the name's letters: 8 tall, strokes 2 wide ----
BIG = {
    'C': ['.XXXXX.', 'XXXXXXX', 'XX...XX', 'XX.....', 'XX.....', 'XX...XX', 'XXXXXXX', '.XXXXX.'],
    'A': ['.XXXXX.', 'XXXXXXX', 'XX...XX', 'XX...XX', 'XXXXXXX', 'XXXXXXX', 'XX...XX', 'XX...XX'],
    'T': ['XXXXXX', 'XXXXXX', '..XX..', '..XX..', '..XX..', '..XX..', '..XX..', '..XX..'],
    'H': ['XX...XX', 'XX...XX', 'XX...XX', 'XXXXXXX', 'XXXXXXX', 'XX...XX', 'XX...XX', 'XX...XX'],
    '1': ['.XX.', 'XXX.', '.XX.', '.XX.', '.XX.', '.XX.', 'XXXX', 'XXXX'],
    '5': ['XXXXXXX', 'XXXXXXX', 'XX.....', 'XXXXXX.', 'XXXXXXX', '.....XX', 'XXXXXXX', 'XXXXXX.'],
    ' ': ['..'] * 8,
}

# ---- the game's 5x7 font, for the signs (host/glcdfont.h) ----
with open(os.path.join(ROOT, 'host', 'glcdfont.h')) as _f:
    FONT = [int(h, 16) for h in re.findall(r'0x([0-9A-Fa-f]{2})', _f.read().split('{', 1)[1])]


def small_text(s, x0=0, y0=0):
    return {(x0 + i * 6 + x, y0 + y) for i, ch in enumerate(s) for x in range(5) for y in range(8)
            if FONT[ord(ch) * 5 + x] >> y & 1}


class Pic:
    """a picture as {(x, y): '#rrggbb'}"""

    def __init__(self):
        self.p = {}

    def put(self, pts, col):
        for q in pts:
            self.p[q] = col

    def rect(self, x, y, w, h, col):
        self.put({(i, j) for i in range(x, x + w) for j in range(y, y + h)}, col)

    def paste(self, other, dx, dy):
        for (x, y), c in other.p.items():
            self.p[(x + dx, y + dy)] = c

    def norm(self):
        x0 = min(x for x, _ in self.p)
        y0 = min(y for _, y in self.p)
        q = Pic()
        q.p = {(x - x0, y - y0): c for (x, y), c in self.p.items()}
        return q

    def size(self):
        return max(x for x, _ in self.p) + 1, max(y for _, y in self.p) + 1

    def image(self, scale=1):
        from PIL import Image
        q = self.norm()
        w, h = q.size()
        im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
        for (x, y), c in q.p.items():
            im.putpixel((x, y), tuple(int(c[i:i + 2], 16) for i in (1, 3, 5)) + (255,))
        return im.resize((w * scale, h * scale), Image.NEAREST) if scale > 1 else im

    def svg(self):
        """runs of a colour a row, one path a colour; crispEdges keeps it sharp at any size"""
        q = self.norm()
        w, h = q.size()
        paths = {}
        for y in range(h):
            x = 0
            while x < w:
                c = q.p.get((x, y))
                if c is None:
                    x += 1
                    continue
                s = x
                while x < w and q.p.get((x, y)) == c:
                    x += 1
                paths.setdefault(c, []).append('M%d %dh%dv1h-%dz' % (s, y, x - s, x - s))
        body = ''.join('<path fill="%s" d="%s"/>' % (c, ''.join(d)) for c, d in paths.items())
        return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d" '
                'shape-rendering="crispEdges">%s</svg>\n' % (w, h, w, h, body))


# ---- shapes ----
CIRCLE = {  # small circles drawn by hand: each row's width (a computed one comes out square at these sizes)
    2: [2, 2], 4: [2, 4, 4, 2], 6: [4, 6, 6, 6, 6, 4], 8: [4, 6, 8, 8, 8, 8, 6, 4],
    10: [4, 8, 8, 10, 10, 10, 10, 8, 8, 4], 12: [4, 8, 10, 10, 12, 12, 12, 12, 10, 10, 8, 4],
    14: [6, 10, 12, 12, 14, 14, 14, 14, 14, 14, 12, 12, 10, 6],
}


def circle(d, x0=0, y0=0):
    """a round blob d across, its top left at (x0, y0)"""
    if d in CIRCLE:
        rows = CIRCLE[d]
    else:
        r = d / 2
        rows = [2 * round(math.sqrt(max(0, r * r - (y + .5 - r) ** 2))) for y in range(d)]
    return {(x0 + (d - w) // 2 + i, y0 + y) for y, w in enumerate(rows) for i in range(w)}


def edge(px):
    """the pixels of px with a side open to the outside"""
    return {(x, y) for (x, y) in px if any((x + dx, y + dy) not in px for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))}


def around(px):
    """the ring a pixel wide round px, corners too"""
    return {(x + dx, y + dy) for (x, y) in px for dx in (-1, 0, 1) for dy in (-1, 0, 1)} - px


def even(v):
    return int(v / 2 + .5) * 2


CORNER = {16: [2, 1], 20: [2, 1]}  # how far in each top row starts, at the small sizes


def frame(n):
    """the apps' icon frame: rounded corners, a red top, a dark band, a white bottom, lit and shaded rows"""
    if n in CORNER:
        ins = CORNER[n]
    else:
        r = n * .16
        ins = []
        for y in range(int(r)):
            ins.append(max(0, math.ceil(r - math.sqrt(max(0, r * r - (r - y - .5) ** 2)) - .35)))
        while ins and ins[-1] == 0:
            ins.pop()
    band = max(1, round(n / 16))
    lit = max(1, n // 16)
    p = Pic()
    for y in range(n):
        k = ins[y] if y < len(ins) else ins[n - 1 - y] if n - 1 - y < len(ins) else 0
        for x in range(k, n - k):
            dy = y + .5 - n / 2
            if abs(dy) < band:
                col = INK
            elif dy < 0:
                col = RED_L if y < lit else RED
            else:
                col = SHADE if y >= n - lit else OFFW
            p.p[(x, y)] = col
    return p


def screen(n):
    """the round screen: its diameter, the ring's width, and the pixels inside the ring"""
    d = even(n * (.74 if n < 24 else .62))
    ring_w = max(1, round(n / 24))
    o = (n - d) // 2
    disc = circle(d, o, o)
    inner = disc
    for _ in range(ring_w):
        inner = inner - edge(inner)
    return disc, inner


def island(p, n, inner):
    """the Travel page's island, small: the sea, a sandy coast, five places round the Wild middle"""
    di = even(math.sqrt(len(inner) * 4 / math.pi) * (.8 if n < 24 else .68))
    o = (n - di) // 2
    land = circle(di, o, o)
    dw = max(2, even(di * .36))
    wild = circle(dw, (n - dw) // 2, (n - dw) // 2)
    coast = edge(land)
    c = n / 2
    place = {}
    for (x, y) in land - coast:
        if (x, y) in wild:
            place[(x, y)] = 5
        else:
            a = (math.degrees(math.atan2(y + .5 - c, x + .5 - c)) + 90 + 36) % 360
            place[(x, y)] = int(a // 72)
    for q in inner:
        x, y = q
        if q not in land:
            near = n >= 32 and any((x + dx, y + dy) in land for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            p.p[q] = SEA_L if near and (x + y) % 2 else SEA
        elif q in coast:
            a = (math.degrees(math.atan2(y + .5 - c, x + .5 - c)) + 90 + 36) % 360
            p.p[q] = SAND_L if int(a // 72) == 2 else SAND  # (the Beach's coast paler)
        else:
            k = place[q]
            fill, lit = WILD if k == 5 else WEDGES[k]
            col = lit if n >= 32 and place.get((x, y - 1)) != k else fill
            if n >= 48 and k != 5 and (x + y) % 2 == 0:  # dashed seams where two places meet, as on the map
                r, dn = place.get((x + 1, y)), place.get((x, y + 1))
                if (r not in (None, 5, k)) or (dn not in (None, 5, k)):
                    col = INK
            p.p[q] = col


STAR = {  # the sparkle drawn by hand where the screen is small: o gold, w white
    16: ['...oo...', '...oo...', '..oooo..', 'ooowwooo', 'ooowwooo', '..oooo..', '...oo...', '...oo...'],
    20: ['....oo....', '....oo....', '....oo....', '...oooo...', 'oooowwoooo',
         'oooowwoooo', '...oooo...', '....oo....', '....oo....', '....oo....'],
}


def sparkle(p, n, inner):
    """a shiny's sparkle in the middle: a four-point star (|x|^.55 + |y|^.55 <= r^.55), its heart white"""
    p.put(inner, SCREEN)
    if n in STAR:
        rows = STAR[n]
        o = (n - len(rows)) // 2
        for y, r in enumerate(rows):
            for x, ch in enumerate(r):
                if ch != '.':
                    p.p[(o + x, o + y)] = CREAM_L if ch == 'w' else GOLD_S
        return
    r = math.sqrt(len(inner) / math.pi) * .96
    c = n / 2
    for (x, y) in inner:
        dx, dy = abs(x + .5 - c), abs(y + .5 - c)
        if dx ** .55 + dy ** .55 <= r ** .55:
            p.p[(x, y)] = CREAM_L if dx + dy <= (1 if n < 48 else 2) else GOLD_S


def icon(n, what):
    """n x n: the frame, the screen's ring, and the island ('game') or the sparkle ('dex')"""
    p = frame(n)
    disc, inner = screen(n)
    p.put(disc, INK)
    if what == 'game':
        island(p, n, inner)
    else:
        sparkle(p, n, inner)
    return p


LARGE = {  # the same letters drawn larger, for where the logo is the biggest thing (the About page): 12 tall, strokes 3
    'C': ['..XXXXXX..', '.XXXXXXXX.', 'XXXXXXXXXX', 'XXX....XXX', 'XXX.......', 'XXX.......', 'XXX.......', 'XXX.......',
          'XXX....XXX', 'XXXXXXXXXX', '.XXXXXXXX.', '..XXXXXX..'],
    'A': ['..XXXXXX..', '.XXXXXXXX.', 'XXXXXXXXXX', 'XXX....XXX', 'XXX....XXX', 'XXXXXXXXXX', 'XXXXXXXXXX', 'XXXXXXXXXX',
          'XXX....XXX', 'XXX....XXX', 'XXX....XXX', 'XXX....XXX'],
    'T': ['XXXXXXXXX', 'XXXXXXXXX', 'XXXXXXXXX'] + ['...XXX...'] * 9,
    'H': ['XXX....XXX'] * 4 + ['XXXXXXXXXX'] * 3 + ['XXX....XXX'] * 5,
    '1': ['..XXX..', '.XXXX..', 'XXXXX..'] + ['..XXX..'] * 7 + ['XXXXXXX'] * 2,
    '5': ['XXXXXXXXXX', 'XXXXXXXXXX', 'XXX.......', 'XXX.......', 'XXXXXXXXX.', 'XXXXXXXXXX', '.......XXX', '.......XXX',
          'XXX....XXX', 'XXXXXXXXXX', '.XXXXXXXX.', '..XXXXXX..'],
    ' ': ['...'] * 12,
}


def name(word='CATCH 151', red_from=6, font=BIG):
    """the name: chunky letters, cream then red, lit on top and shaded at the bottom, one outline, a shadow"""
    letters, x = [], 0
    for ch in word:
        rows = font[ch]
        letters.append({(x + i, y) for y, r in enumerate(rows) for i, c in enumerate(r) if c == 'X'})
        x += len(rows[0]) + 1
    allpx = set().union(*letters)
    ring = around(allpx)
    h = len(font['C'])
    lit, shade = (1, 7) if h == 8 else (2, 10)  # the top rows lit, the bottom shaded
    p = Pic()
    p.put({(x, y + 1) for (x, y) in allpx | ring} - allpx - ring, INK)  # the shadow
    p.put(ring, INK)
    for i, px in enumerate(letters):
        top, mid, bot = (RED_L, RED, RED_D) if i >= red_from else (CREAM_L, CREAM, CREAM_D)
        for (x, y) in px:
            p.p[(x, y)] = top if y <= lit else bot if y >= shade else mid
    return p


def board(word, wood=WOOD, lit=WOOD_L):
    """v2SignIn at the font's own pixels: a dark edge, the wood, its lit top, two nails, cream letters"""
    w = 6 * len(word) - 1 + 14
    p = Pic()
    p.rect(-1, -1, w + 2, 14, WOOD_EDGE)
    p.rect(0, 0, w, 12, wood)
    p.rect(0, 0, w, 2, lit)
    p.rect(2, 5, 2, 2, WOOD_EDGE)
    p.rect(w - 4, 5, 2, 2, WOOD_EDGE)
    p.put(small_text(word, 7, 3), CREAM)
    return p


def row(*parts, gap=4):
    """pictures side by side, their middles in line"""
    parts = [q.norm() for q in parts]
    h = max(q.size()[1] for q in parts)
    p, x = Pic(), 0
    for q in parts:
        w, qh = q.size()
        p.paste(q, x, (h - qh) // 2)
        x += w + gap
    return p


def save(p, base, scales=(1, 2, 3)):
    with open(os.path.join(OUT, base + '.svg'), 'w') as f:
        f.write(p.svg())
    for s in scales:
        p.image(s).save(os.path.join(OUT, base + ('' if s == 1 else '@%dx' % s) + '.png'), optimize=True)


def save_icon(what, base):
    for n in (16, 32, 48, 64):
        icon(n, what).image().save(os.path.join(OUT, '%s-%d.png' % (base, n)), optimize=True)
    with open(os.path.join(OUT, base + '.svg'), 'w') as f:
        f.write(icon(32, what).svg())
    for size, design in ((180, 60), (192, 64), (512, 64), (1024, 64)):  # the app sizes: a design, scaled whole
        icon(design, what).image(size // design).save(os.path.join(OUT, '%s-%d.png' % (base, size)), optimize=True)


def logo():
    """the logo: the game's icon, then the name"""
    return row(icon(20, 'game'), name())


def logo_large():
    """the logo drawn larger, not scaled (the About page): the icon at 28, the name's 12 px letters"""
    return row(icon(28, 'game'), name(font=LARGE), gap=5)


def header(p):
    """src/brand_logo.h: the logo for the game to draw (the About page): a palette (RGB565, 0 see-through), an index
    a pixel, as about_logo.h is"""
    q = p.norm()
    w, h = q.size()
    pal, idx = [None], []
    for y in range(h):
        for x in range(w):
            c = q.p.get((x, y))
            if c is None:
                idx.append(0)
                continue
            r, g, b = (int(c[i:i + 2], 16) for i in (1, 3, 5))
            v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            if v not in pal:
                pal.append(v)
            idx.append(pal.index(v))
    out = ['// Generated by tools/brand_assets.py: do not edit.\n',
           '// The game\'s logo (its icon, then CATCH 151) as pixel art, %d x %d: a palette (RGB565, 0 = see-through),\n'
           '// an index a pixel\n' % (w, h),
           '#pragma once\n#include <stdint.h>\n',
           '#define BRAND_LOGO_W %d\n#define BRAND_LOGO_H %d\n' % (w, h),
           'static const uint16_t BRAND_LOGO_PAL[%d] = { 0, %s };\n' % (len(pal), ', '.join('0x%04X' % c for c in pal[1:])),
           'static const uint8_t BRAND_LOGO[%d] = {\n' % (w * h)]
    out += ['  ' + ','.join(str(v) for v in idx[y * w:(y + 1) * w]) + ',\n' for y in range(h)]
    return ''.join(out) + '};\n'


HEADER = os.path.join(ROOT, 'src', 'brand_logo.h')
WEB_LOGO = os.path.join(ROOT, 'webgame', 'assets', 'logo.svg')  # the top of the web game's page


def main():
    os.makedirs(OUT, exist_ok=True)
    save(logo(), 'catch151')
    with open(WEB_LOGO, 'w') as f:
        f.write(logo().svg())
    save(logo_large(), 'catch151-large')
    with open(HEADER, 'w') as f:
        f.write(header(logo_large()))
    save(name(), 'catch151-name')
    save(row(icon(20, 'dex'), name(), board('DEX', GOLD, GOLD_L)), 'catch151-dex')
    for dev in ('KNOMI', 'MAC', 'WINDOWS', 'WEB'):
        save(row(icon(20, 'game'), name(), board('FOR ' + dev)), 'catch151-for-' + dev.lower(), scales=(2,))
    save_icon('game', 'catch151-icon')
    save_icon('dex', 'catch151-dex-icon')
    print('brand: %d files in %s' % (len(os.listdir(OUT)), os.path.relpath(OUT, ROOT)))


if __name__ == '__main__':
    main()
