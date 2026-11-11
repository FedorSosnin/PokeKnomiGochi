#!/usr/bin/env python3
"""V2's trophies: the trophy room's 25 medals as pixel art in the game's own style, drawn the way the food and the
Pokemon are: a dark outline, four or five shades lit from the top left, a white glint. Each is drawn twice from the
same shapes: small for the room (2 screen px a pixel, a Knomi pixel each) and big for its card (3 px a pixel, the
food's size).

  python3 tools/trophies.py         # render build/trophies.png: every trophy, won and not, small and big
  python3 tools/trophies.py emit    # ...and write trophies_v2.h
"""
import math
import os
import subprocess
import sys


# ---------------------------------------------------------------- colours
def C(h, a=255, won=False):
    """a colour: a = 255 solid, less is see-through (glass); won: only on a medal won (sparkles)"""
    return (h.lower(), a, won)


def R(*hs):
    return [C(h) for h in hs]


K = C('#1b1b25')  # the game's outline
WH = C('#ffffff')
# ramps, light to dark: glint, light, mid, shade, deep
GOLD = R('#fff6c8', '#ffd95e', '#f0b030', '#c07a10', '#7e4a08')
SILVER = R('#ffffff', '#e2e7f0', '#b4bccb', '#7f889c', '#535a6c')
BRONZE = R('#ffe2c4', '#eca46c', '#c8743a', '#8e4a22', '#5a2c12')
MARBLE = R('#ffffff', '#efebf6', '#cdc7de', '#9f99b6', '#6e688a')
EBONY = R('#9a8cb0', '#74688c', '#544a6a', '#3a3250', '#262036')
WOOD = R('#d8a878', '#b07a4c', '#8a5a34', '#643e22', '#422614')
VELVET = R('#ffb0b0', '#ee6a66', '#c83e44', '#922630', '#5e1620')
CREAM = R('#ffffff', '#fff6d8', '#f8e0a4', '#e0bc70', '#a8803a')
FOREST = R('#eaffb8', '#a8e070', '#58b868', '#3c8a4c', '#245a32')
SEA = R('#ffffff', '#a6dcf6', '#58aee4', '#3478bc', '#1e4c84')
CRYSTAL = R('#f0fffa', '#a8f2de', '#4ec8b0', '#2c8e88', '#1a5a5e')
STONE = R('#c8c0b8', '#a09890', '#7c746c', '#5a524c', '#3c3632')
GRAVE = R('#f4f0fa', '#c4bcd6', '#948cae', '#6a6288', '#463e62')  # the Mansion's tombstone, a cold lavender grey
PETAL = R('#fffbe0', '#ffe676', '#f8c232', '#d88c18', '#9a5a10')
SEEDS = R('#e0a060', '#b8743a', '#8c5026', '#643418', '#40200e')
RED = R('#ffd8d0', '#ff8a72', '#e8503a', '#b53224', '#7a1e16')
BLUE = R('#e0f4ff', '#8cc8f0', '#4f93c4', '#336aa0', '#1f4678')
GREEN = R('#e0ffc0', '#94dc7c', '#58b868', '#3c8a4c', '#25603a')
PURPLE = R('#f0e4ff', '#b89ce4', '#8064bc', '#5a4490', '#3a2a68')
ORANGE = R('#fff0c8', '#ffb070', '#f5863d', '#c85f22', '#8a3e14')
PINK = R('#fff0f6', '#ffbcd0', '#f08aa4', '#c75f80', '#8a3a5a')
MINT = R('#f2fff8', '#c4f2de', '#8fd6b4', '#5fae8c', '#3a7e62')
YELLOW = R('#fff8c8', '#ffe470', '#f8d038', '#d0a018', '#9a7010')
BALLWHITE = R('#ffffff', '#ffffff', '#eceff4', '#b4bac6', '#80869a')
ULTRA = R('#7a7a8e', '#56566a', '#3c3c4a', '#2a2a34', '#1c1c24')
MASTER = R('#c8a8f0', '#a07ad8', '#7848b8', '#5a3494', '#3e2268')
SKY = R('#e8f8ff', '#c4e8f8', '#a0d6f0', '#80c0e4', '#5ea4d0')
GRASS = R('#d0f0a0', '#a6dc74', '#7cc04e', '#5a9a3a', '#3e7228')
TV = R('#c8ccd8', '#a0a6b4', '#7a8090', '#565c6c', '#3a3e4a')


# ---------------------------------------------------------------- the canvas
class Cv:
    def __init__(s, w, h):
        s.w, s.h = w, h
        s.p = [[None] * w for _ in range(h)]

    def get(s, x, y):
        return s.p[y][x] if 0 <= x < s.w and 0 <= y < s.h else None

    def put(s, x, y, c):
        if 0 <= x < s.w and 0 <= y < s.h:
            s.p[y][x] = c

    def outline(s, c=K):
        """the game's outline round everything solid (4-neighbour, so its corners come out round)"""
        add = []
        for y in range(s.h):
            for x in range(s.w):
                if s.p[y][x] is not None:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    q = s.get(x + dx, y + dy)
                    if q is not None and q[1] == 255 and q != c:
                        add.append((x, y))
                        break
        for x, y in add:
            s.p[y][x] = c
        return s

    def over(s, o, ox=0, oy=0, clip=None):
        for y in range(o.h):
            for x in range(o.w):
                if o.p[y][x] is not None and (clip is None or (x + ox, y + oy) in clip):
                    s.put(x + ox, y + oy, o.p[y][x])
        return s

    def trim(s):
        """cut down to what's drawn"""
        pts = [(x, y) for y in range(s.h) for x in range(s.w) if s.p[y][x] is not None]
        x0, x1 = min(p[0] for p in pts), max(p[0] for p in pts)
        y0, y1 = min(p[1] for p in pts), max(p[1] for p in pts)
        o = Cv(x1 - x0 + 1, y1 - y0 + 1)
        o.p = [row[x0:x1 + 1] for row in s.p[y0:y1 + 1]]
        return o

    def stamp(s, rows, keys, ox, oy):
        for j, row in enumerate(rows):
            for i, ch in enumerate(row):
                if ch != '.':
                    s.put(ox + i, oy + j, keys[ch])
        return s


def mask(w, h, fn, f=1.0, ox=0.0, oy=0.0):
    """the pixels whose centres fn() holds for, fn in design units (f pixels a unit)"""
    return {(x, y) for y in range(h) for x in range(w) if fn((x + .5) / f - ox, (y + .5) / f - oy)}


LIGHT = (-0.48, -0.62, 0.62)
_n = math.sqrt(sum(v * v for v in LIGHT))
LIGHT = tuple(v / _n for v in LIGHT)


def sph(nx, ny, hi=True):
    """a round surface's shade: 0 glint .. 4 deep, for its normal's x and y"""
    r2 = min(1.0, nx * nx + ny * ny)
    nz = math.sqrt(1 - r2)
    d = LIGHT[0] * nx + LIGHT[1] * ny + LIGHT[2] * nz
    if d > .95 and hi:
        return 0
    return 1 if d > .70 else 2 if d > .32 else 3 if d > -.05 else 4


def spans(m):
    rows, cols = {}, {}
    for x, y in m:
        a, b = rows.get(y, (x, x))
        rows[y] = (min(a, x), max(b, x))
        a, b = cols.get(x, (y, y))
        cols[x] = (min(a, y), max(b, y))
    return rows, cols


def pillow(cv, m, rp, shift=0, hi=True, sq=0.92):
    """a soft round shape: each pixel shaded by where it sits across its row and down its column"""
    rows, cols = spans(m)
    for x, y in m:
        a, b = rows[y]
        c, d = cols[x]
        nx = 0 if b == a else (2 * (x - a) / (b - a) - 1) * sq
        ny = 0 if d == c else (2 * (y - c) / (d - c) - 1) * sq
        cv.put(x, y, rp[min(4, max(0, sph(nx, ny, hi) + shift))])


def bevel(m, x, y, b=1):
    lit = dark = 0
    for d in range(1, b + 1):
        if not lit and ((x - d, y) not in m or (x, y - d) not in m or (x - d, y - d) not in m):
            lit = d
        if not dark and ((x + d, y) not in m or (x, y + d) not in m or (x + d, y + d) not in m):
            dark = d
    return lit, dark


def line(cv, x0, y0, x1, y1, c, wd=1):
    n = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
    for i in range(n + 1):
        t = i / n
        x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
        for dx in range(wd):
            for dy in range(wd):
                cv.put(int(math.floor(x)) + dx, int(math.floor(y)) + dy, c)


def part(w, h, m, fill):
    """one piece on its own, outlined: put over the others, its outline separates it from them"""
    cv = Cv(w, h)
    for x, y in m:
        cv.put(x, y, fill(x, y) if callable(fill) else fill)
    return cv.outline()


def glint(cv, pts):
    for x, y in pts:
        if cv.get(x, y) is not None:
            cv.put(x, y, WH)


# ---------------------------------------------------------------- the zone badges
def badge(zone, big):
    N = 40 if big else 16
    f = N / 16
    b = 2 if big else 1
    cv = Cv(N, N)
    if zone == 0:  # the Forest: a leaf, its midrib light, its far half in shade, the stalk down to the left
        cx, cy, L, wd = 8.5, 7.5, 7.2, 4.1
        A = (.7071, -.7071)

        def pq(u, v):
            return (u - cx) * A[0] + (v - cy) * A[1], (u - cx) * .7071 + (v - cy) * .7071

        def leaf(u, v):
            p, q = pq(u, v)
            if abs(p) > L:
                return False
            t = p / L
            return abs(q) <= wd * (1 - t * t) * (1 - .18 * t)
        m = mask(N, N, leaf, f)
        for x, y in m:
            p, q = pq((x + .5) / f, (y + .5) / f)
            base = 1 if q < 0 else 2
            lit, dark = bevel(m, x, y, b)
            lv = base - 1 if lit and not dark else base + 1 + (1 if big and dark == 1 else 0) if dark and not lit else base
            cv.put(x, y, FOREST[max(0, min(4, lv))])
        bx, by = cx - L * A[0], cy - L * A[1]
        line(cv, bx * f, by * f, (bx - 2.3) * f, (by + 2.3) * f, FOREST[3], 2 if big else 1)
        for i in range(int(12 * f)):  # the midrib
            p = -L + 1.0 + i / f * .9
            if p > L - 1.6:
                break
            cv.put(int((cx + p * A[0]) * f), int((cy + p * A[1]) * f), FOREST[0] if big else FOREST[1])
        if big:  # the veins off it
            for p0 in (-3.6, -1.2, 1.2, 3.4):
                for s in (-1, 1):
                    for k in range(14):
                        tt = k / 14
                        p, q = p0 + tt * 1.9, s * tt * 2.5
                        u, v = cx + p * A[0] + q * .7071, cy + p * A[1] + q * .7071
                        x, y = int(u * f), int(v * f)
                        if (x, y) in m and bevel(m, x, y, 1) == (0, 0):
                            cv.put(x, y, FOREST[1] if s < 0 else FOREST[3])
        cv.outline()
        glint(cv, [(int(7.2 * f), int(4.6 * f))] + ([(int(7.2 * f) + 1, int(4.6 * f)), (int(7.2 * f), int(4.6 * f) + 1)] if big else []))
    elif zone == 1:  # the Beach: a drop of the sea, a wave across it
        dcx, dcy, r, tip = 8.0, 9.8, 5.4, 1.2

        def drop(u, v):
            if (u - dcx) ** 2 + (v - dcy) ** 2 <= r * r:
                return True
            if tip <= v <= dcy:
                return abs(u - dcx) <= r * ((v - tip) / (dcy - tip)) ** .8
            return False
        m = mask(N, N, drop, f)
        pillow(cv, m, SEA, sq=.95)
        if big:
            for i in range(60):
                u = 4.2 + i / 60 * 7.6
                v = 11.2 + .75 * math.sin((u - 4.2) * 1.25)
                x, y = int(u * f), int(v * f)
                for k in (0, 1):
                    if (x, y + k) in m:
                        cv.put(x, y + k, SEA[0] if k == 0 else SEA[1])
        else:
            for x, v in ((5, 11), (6, 10), (7, 11), (8, 11), (9, 10), (10, 11)):
                if (x, v) in m:
                    cv.put(x, v, SEA[1])
        cv.outline()
        glint(cv, [(int(6.0 * f), int(7.4 * f))] + ([(int(6.0 * f), int(7.4 * f) + 1), (int(6.0 * f), int(7.4 * f) + 2), (int(6.0 * f) + 1, int(7.4 * f) - 1)] if big else []))
    elif zone == 2:  # the Cave: three crystals growing out of a rock
        def prism(cx, half, peak, sh, bot, lean):
            def fn(u, v):
                lx = u - (cx - (bot - v) * lean)
                return abs(lx) <= half and v <= bot and v >= peak + (sh - peak) * abs(lx) / half
            m = mask(N, N, fn, f)

            def col(x, y):
                u, v = (x + .5) / f, (y + .5) / f
                lx = u - (cx - (bot - v) * lean)
                if v < sh + (1 - abs(lx) / half) * 0:
                    top = v < peak + (sh - peak) * abs(lx) / half + (1.4 if big else 1.0)
                else:
                    top = False
                if lx < -half * .3:
                    lv = 1
                elif lx > half * .35:
                    lv = 3
                else:
                    lv = 2
                if top and lv > 0:
                    lv -= 1
                return CRYSTAL[lv]
            return part(N, N, m, col)
        cv.over(prism(4.3, 2.1, 5.6, 7.4, 13.6, .20))
        cv.over(prism(11.8, 2.0, 6.6, 8.3, 13.6, -.20))
        cv.over(prism(8.0, 2.6, 1.3, 3.9, 13.6, 0))
        rock = mask(N, N, lambda u, v: ((u - 8) / 7.0) ** 2 + ((v - 14.4) / 1.9) ** 2 <= 1 and v <= 15.0, f)
        cv.over(part(N, N, rock, lambda x, y: STONE[1 if (y + .5) / f < 13.8 else 2 if x < N * .6 else 3]))
        glint(cv, [(int(7.0 * f), int(5.2 * f))] + ([(int(7.0 * f), int(5.2 * f) + 1), (int(7.0 * f), int(5.2 * f) + 2)] if big else []))
    elif zone == 3:  # the Mansion: a tombstone, a Poke Ball carved in it, grass at its foot
        def stone(u, v):
            if 3.4 <= u <= 12.6 and 6.2 <= v <= 13.6:
                return True
            return (u - 8) ** 2 + (v - 6.2) ** 2 <= 4.6 ** 2 and v <= 6.2
        m = mask(N, N, stone, f)
        for x, y in m:
            lit, dark = bevel(m, x, y, b)
            lv = 1 if lit and not dark else (4 if big and dark == 1 else 3) if dark and not lit else 2
            cv.put(x, y, GRAVE[lv])
        if big:  # a crack in from its side
            for x, y in ((9, 25), (10, 26), (10, 27), (11, 28), (12, 28), (12, 29)):
                if (x, y) in m:
                    cv.put(x, y, GRAVE[4])
        cv.outline()
        base = mask(N, N, lambda u, v: 1.8 <= u <= 14.2 and 13.6 <= v <= 15.0, f)
        cv.over(part(N, N, base, lambda x, y: GRAVE[2] if (y + .5) / f < 14.3 else GRAVE[3]))
        # a Poke Ball carved in it, its top half sunk a shade deeper (Fedor: "an impressed version of a Pokeball. So
        # like as if it was carved into the stone"; first a cross, then RIP: "nothing religious")
        if big:
            bx, by, R = 20.0, 16.5, 8.2
            rows = []
            for y in range(N):
                r = ''
                for x in range(N):
                    dx, dy = x + .5 - bx, y + .5 - by
                    d = math.hypot(dx, dy)
                    groove = (R - 1.2 < d <= R) or (abs(dy) <= .6 and 3.2 <= d <= R) or (2.1 < d <= 3.2)
                    r += '#' if groove else '.'
                rows.append(r)
            for y in range(N):
                for x in range(N):
                    dx, dy = x + .5 - bx, y + .5 - by
                    if dy < -.6 and math.hypot(dx, dy) < R - 1.2 and math.hypot(dx, dy) > 3.2:
                        cv.put(x, y, GRAVE[3])
            engrave(cv, rows, 0, 0, GRAVE[4], GRAVE[1])
        else:
            ball = ["..###..", ".#...#.", "#.###.#", "###.###", "#.###.#", ".#...#.", "..###.."]
            for x, y in ((6, 6), (7, 6), (8, 6), (5, 7), (9, 7)):  # the top half, sunk
                cv.put(x, y, GRAVE[3])
            for j, r in enumerate(ball):
                for i, ch in enumerate(r):
                    if ch == '#':
                        cv.put(4 + i, 5 + j, GRAVE[4])
        tufts = [(3, 13), (4, 12), (12, 13), (11, 13)] if not big else []
        for x, y in tufts:
            cv.put(x, y, FOREST[2])
        if big:  # grass round its foot
            for gx in (6, 9, 12, 27, 30, 33):
                for k, hh in enumerate((3, 5, 2)):
                    for t in range(hh):
                        cv.put(gx + k, 34 - t, FOREST[1] if k == 0 else FOREST[3])
            cv.outline()
        glint(cv, [(int(5.0 * f), int(3.3 * f))] + ([(int(5.0 * f) + 1, int(3.3 * f)), (int(5.0 * f), int(3.3 * f) + 1)] if big else []))
    elif zone == 5:  # Knomi Tower (MED_TOWER): a little stone tower, battlements, a lit window, a gold crown over it
        def body(u, v):
            if 4.6 <= u <= 11.4 and 7.0 <= v <= 15.2:  # the tower
                return True
            if 3.8 <= u <= 12.2 and 6.0 <= v <= 7.4:  # the ledge under the battlements
                return True
            return 4.6 <= v <= 6.2 and 3.8 <= u <= 12.2 and int((u - 3.8) / 1.68) % 2 == 0  # three merlons
        m = mask(N, N, body, f)
        t = Cv(N, N)
        for x, y in m:
            lit, dark = bevel(m, x, y, b)
            lv = 1 if lit and not dark else (4 if big and dark == 1 else 3) if dark and not lit else 2
            if big and (y + .5) / f > 7.6 and int((y + .5) / f - 7.6) % 2 == 0 and lv == 2:
                lv = 3 if (x // (2 * b)) % 3 == 0 else 2  # mortar courses
            t.put(x, y, GRAVE[lv])
        win = mask(N, N, lambda u, v: (7.0 <= u <= 9.0 and 9.6 <= v <= 12.4) or (u - 8.0) ** 2 + (v - 9.6) ** 2 <= 1.0, f)
        for x, y in win:  # the window, lit gold
            t.put(x, y, GOLD[1] if (x + .5) / f < 8.0 else GOLD[2])
        door = mask(N, N, lambda u, v: 6.9 <= u <= 9.1 and 13.4 <= v <= 15.2, f)
        for x, y in door:
            t.put(x, y, EBONY[3])
        cv.over(t.outline())

        def crown(u, v):  # a band and three sharp points, a gap above the battlements
            if 4.8 <= u <= 11.2 and 2.5 <= v <= 3.5:
                return True
            for px in (5.3, 8.0, 10.7):
                if 0.2 <= v <= 2.5 and abs(u - px) <= (v - 0.2) * .30 + .22:
                    return True
            return False
        cm = mask(N, N, crown, f)
        cv.over(part(N, N, cm, lambda x, y: GOLD[1] if (y + .5) / f < 2.5 and x < N / 2 else GOLD[2] if (y + .5) / f < 2.5 or x < N * .6 else GOLD[3]))
        if big:  # a red jewel on the band
            for x, y in mask(N, N, lambda u, v: (u - 8.0) ** 2 + (v - 3.0) ** 2 <= .36, f):
                cv.put(x, y, RED[1] if x < N / 2 else RED[2])
        glint(cv, [(int(5.6 * f), int(1.4 * f))] + ([(int(5.6 * f), int(1.4 * f) + 1), (int(8.0 * f), int(1.2 * f))] if big else []))
    else:  # the Field: a sunflower, eight petals round a seed head
        pet = Cv(N, N)
        for k in range(8):
            a = k * math.pi / 4 + math.pi / 8
            pcx, pcy = 8 + 4.5 * math.cos(a), 8 + 4.5 * math.sin(a)

            def petal(u, v, a=a, pcx=pcx, pcy=pcy):
                du, dv = u - pcx, v - pcy
                rr = du * math.cos(a) + dv * math.sin(a)
                tt = -du * math.sin(a) + dv * math.cos(a)
                return (rr / 2.9) ** 2 + (tt / 1.85) ** 2 <= 1
            m = mask(N, N, petal, f)
            p = Cv(N, N)
            pillow(p, m, PETAL, shift=0 if k % 2 else 1, hi=False)
            if big:
                p.outline()
            pet.over(p)
        cv.over(pet)
        if not big:
            cv.outline()
        disc = mask(N, N, lambda u, v: (u - 8) ** 2 + (v - 8) ** 2 <= 3.1 ** 2, f)
        d = Cv(N, N)
        pillow(d, disc, SEEDS, hi=False)
        for x, y in disc:
            if (x + y) % 2 == 0 and bevel(disc, x, y, 1) == (0, 0) and big:
                d.put(x, y, SEEDS[min(4, SEEDS.index(d.get(x, y)) + 1)])
        d.outline()
        cv.over(d)
        if big:
            cv.outline()
        glint(cv, [(int(6.6 * f), int(6.6 * f))] + ([(int(6.6 * f) + 1, int(6.6 * f))] if big else []))
    return cv


# ---------------------------------------------------------------- the Pokedex statues: a ball on a pedestal
BALL_ROWS = {  # the game's balls, 18 x 18 (species.h's SPR_HOME_BALL, master_v2.h's Ultra and Master Balls)
    'poke': [
        "......kkkkkk......", "....kkrrrrrrkk....", "...kwwrrrrrrrRk...", "..kwwrrrrrrrrRRk..",
        ".krwrrrkkkkrrrRRk.", ".krrrrkkkkkkrrRRk.", "krrrrkkkwwkkkrRRRk", "kkkkkkkwwwskkkkkkk",
        "kkkkkkkwwsskkkkkkk", "kwwwwkkksskkkwwwSk", "kwwwwwkkkkkkwwwSSk", ".kwwwwwkkkkwwwwSk.",
        ".kwwwwwwwwwwwwSSk.", "..kwwwwwwwwwwSSk..", "...kSwwwwwwwSSk...", "....kkSSSSSSkk....",
        "......kkkkkk......"],
    'great': [  # the Great Ball: blue, a red flash up each side
        "......kkkkkk......", "....kknnnnnnkk....", "...kwwnnnnnnnNk...", "..kwrrnnnnnnrRNk..",
        ".knwrrnkkkknrRNNk.", ".knnrrkkkkkkrRNNk.", "knnnrkkkwwkkkRNNNk", "kkkkkkkwwwskkkkkkk",
        "kkkkkkkwwsskkkkkkk", "kwwwwkkksskkkwwwSk", "kwwwwwkkkkkkwwwSSk", ".kwwwwwkkkkwwwwSk.",
        ".kwwwwwwwwwwwwSSk.", "..kwwwwwwwwwwSSk..", "...kSwwwwwwwSSk...", "....kkSSSSSSkk....",
        "......kkkkkk......"],
    'ultra': [
        "......kkkkkk......", "....kkyqqqqykk....", "...kwyyqqqqyyQk...", "..kqwyyqqqqyyQQk..",
        ".kqqqyykkkkyyQQQk.", ".kqqqykkkkkkyQQQk.", "kqqqqkkkwwkkkQQQQk", "kkkkkkkwwwskkkkkkk",
        "kkkkkkkwwsskkkkkkk", "kwwwwkkksskkkwwwSk", "kwwwwwkkkkkkwwwSSk", ".kwwwwwkkkkwwwwSk.",
        ".kwwwwwwwwwwwwSSk.", "..kwwwwwwwwwwSSk..", "...kSwwwwwwwSSk...", "....kkSSSSSSkk....",
        "......kkkkkk......"],
    'master': [
        "......kkkkkk......", "....kkwqqqqwkk....", "...kmmwwqqwwmmk...", "..kqmmwqwwqwmmQk..",
        ".kqqqqqkkkkqqqQQk.", ".kqqqqkkkkkkqqQQk.", "kqqqqkkkwwkkkqQQQk", "kkkkkkkwwwskkkkkkk",
        "kkkkkkkwwsskkkkkkk", "kwwwwkkksskkkwwwSk", "kwwwwwkkkkkkwwwSSk", ".kwwwwwkkkkwwwwSk.",
        ".kwwwwwwwwwwwwSSk.", "..kwwwwwwwwwwSSk..", "...kSwwwwwwwSSk...", "....kkSSSSSSkk....",
        "......kkkkkk......"],
}
BALL_KEYS = {
    'poke': {'r': C('#e8503a'), 'R': C('#b53224')},
    'great': {'n': C('#4f93c4'), 'N': C('#2f63a0'), 'r': C('#e8503a'), 'R': C('#b53224')},
    'ultra': {'q': C('#3a3a48'), 'Q': C('#262630'), 'y': C('#f8d040')},
    'master': {'q': C('#7848b8'), 'Q': C('#4e2c80'), 'm': C('#e85aa8')},
}
for _k in BALL_KEYS.values():
    _k.update({'k': K, 'w': WH, 's': C('#aab0bc'), 'S': C('#787e8c')})
BALL_CAP = {'poke': RED, 'great': BLUE, 'ultra': ULTRA, 'master': MASTER}
STATUES = ['poke', 'great', 'ultra', 'master']  # 10, 50, 100 and all 151 registered
PLAQUE = [BRONZE, SILVER, GOLD, GOLD]


def big_ball(kind, D):
    """a ball D pixels across (its outline round it), shaded as a sphere, in the small sprite's pattern"""
    cv = Cv(D, D)
    r = D / 2 - 1
    c = D / 2
    for y in range(D):
        for x in range(D):
            nx, ny = (x + .5 - c) / r, (y + .5 - c) / r
            if nx * nx + ny * ny > 1:
                continue
            lv = sph(nx, ny)
            band = abs(ny) <= 0.10
            if band:
                cv.put(x, y, K)
                continue
            if ny > 0:
                cv.put(x, y, BALLWHITE[lv])
                continue
            col = BALL_CAP[kind][lv]
            lon = nx / math.sqrt(max(1e-6, 1 - ny * ny))  # how far round, along its latitude
            if kind == 'great' and ((abs(nx) - .60) / .22) ** 2 + ((ny + .50) / .34) ** 2 <= 1:
                col = RED[lv]
            if kind == 'ultra' and .40 <= abs(lon) <= .66:
                col = YELLOW[lv]
            if kind == 'master':
                for sx in (-1, 1):
                    if ((nx - sx * .56) / .19) ** 2 + ((ny + .56) / .19) ** 2 <= 1:
                        col = PINK[max(1, lv)]
            cv.put(x, y, col)
    # the button: a ring of outline, white inside
    for y in range(D):
        for x in range(D):
            d = math.hypot(x + .5 - c, y + .5 - c)
            if d <= r * .34:
                cv.put(x, y, K if d > r * .20 else BALLWHITE[sph((x + .5 - c) / (r * .2), (y + .5 - c) / (r * .2))])
    if kind == 'master':  # its M
        M = ["w...w", "ww.ww", "w.w.w", "w...w", "w...w"]
        cv.stamp(M, {'w': WH}, int(c - 2.5), int(c - r * .62))
    cv.outline()
    glint(cv, [(int(c - r * .5), int(c - r * .55)), (int(c - r * .5) + 1, int(c - r * .55)), (int(c - r * .5), int(c - r * .55) + 1)])
    return cv


def pedestal(cv, x0, y0, w, big, plaque):
    """a marble pedestal under a statue: a slab, a column with its plaque, a foot; its top at y0"""
    if big:
        hs, hc, hf, inset = 5, 6, 4, 4
    else:
        hs, hc, hf, inset = 3, 4, 3, 3
    W, H = cv.w, cv.h
    foot = {(x, y) for x in range(x0 + 1, x0 + w - 1) for y in range(y0 + hs + hc, y0 + hs + hc + hf)}
    col = {(x, y) for x in range(x0 + inset, x0 + w - inset) for y in range(y0 + hs - 1, y0 + hs + hc + 1)}
    slab = {(x, y) for x in range(x0, x0 + w) for y in range(y0, y0 + hs)}

    def slabc(x, y, x0=x0, w=w, y0=y0, hs=hs):
        if y == y0:
            return MARBLE[1]
        if x == x0 + w - 1 or y == y0 + hs - 1:
            return MARBLE[3]
        return MARBLE[2]

    def colc(x, y):
        u = (x - (x0 + inset)) / max(1, (w - 2 * inset - 1)) * 2 - 1
        lv = 1 if u < -.45 else 2 if u < .3 else 3 if u < .75 else 4
        if big and lv == 2 and (x - x0) % 3 == 0:  # flutes
            lv = 3
        return MARBLE[lv]
    cv.over(part(W, H, foot, slabc_f(x0, w, y0 + hs + hc, hf)))
    cv.over(part(W, H, col, colc))
    pw, ph = (10, 3) if big else (6, 2)
    px0, py0 = x0 + (w - pw) // 2, y0 + hs + (hc - ph) // 2 + (1 if big else 0)
    pl = {(x, y) for x in range(px0, px0 + pw) for y in range(py0, py0 + ph)}
    cv.over(part(W, H, pl, lambda x, y: plaque[1] if y == py0 else plaque[3] if (x == px0 + pw - 1 or y == py0 + ph - 1) and big else plaque[2]))
    cv.over(part(W, H, slab, slabc))


def slabc_f(x0, w, y0, h):
    def f(x, y):
        if y == y0:
            return MARBLE[1]
        if x == x0 + w - 2 or y == y0 + h - 1:
            return MARBLE[3]
        return MARBLE[2]
    return f


def statue(i, big):
    kind = STATUES[i]
    if big:
        D, w = 34, 38
        cv = Cv(w + 2, D + 17)
        pedestal(cv, 1, D - 1, w, True, PLAQUE[i])
        cv.over(big_ball(kind, D), (cv.w - D) // 2, 0)
    else:
        cv = Cv(24, 30)
        pedestal(cv, 1, 16, 22, False, PLAQUE[i])
        cv.stamp(BALL_ROWS[kind], BALL_KEYS[kind], 3, 0)
    return cv


# ---------------------------------------------------------------- the shiny medal: a golden Pikachu under a glass dome
def gem_case(big):
    f = 1.65 if big else 1.0
    W, H = (37, 46) if big else (23, 28)
    cv = Cv(W, H)
    cx = 11.5
    # the dome: its rim solid, glass inside, a glare up its left
    dome = mask(W, H, lambda u, v: ((u - cx) / 10.0) ** 2 + ((v - 13.4) / 12.2) ** 2 <= 1 and v <= 22.6, f)
    for x, y in dome:
        cv.put(x, y, C('#d6f0ff', 70))
    rim = {(x, y) for x, y in dome if any((x + dx, y + dy) not in dome for dx, dy in ((1, 0), (-1, 0), (0, -1)))}
    for x, y in rim:
        cv.put(x, y, C('#e6f6ff'))
    for i in range(40):
        a = math.pi * (1.06 + i / 40 * .32)
        u, v = cx + 7.6 * math.cos(a), 13.4 + 9.6 * math.sin(a)
        cv.put(int(u * f), int(v * f), C('#ffffff', 200))
        if big:
            cv.put(int(u * f) + 1, int(v * f), C('#ffffff', 140))
    cv.outline()
    # the base: dark wood, a gold band
    base = mask(W, H, lambda u, v: 1.0 <= u <= 22.0 and 22.6 <= v <= 26.8, f)
    cv.over(part(W, H, base, lambda x, y: GOLD[1] if (y + .5) / f < 23.6 else WOOD[3] if (x + .5) / f > 18 or (y + .5) / f > 26 else WOOD[2]))
    # a velvet cushion for it to stand on
    cush = mask(W, H, lambda u, v: ((u - cx) / 6.2) ** 2 + ((v - 21.6) / 1.9) ** 2 <= 1, f)
    c2 = Cv(W, H)
    pillow(c2, cush, VELVET, hi=False)
    cv.over(c2.outline())
    # a golden Pikachu standing on it (Fedor: "a fully golden Pikachu statue"), its face cut into the gold
    ox, oy = cx - 7.0, 5.5

    def pk(fn):
        return mask(W, H, lambda u, v: fn(u - ox, v - oy), f)

    def ell(ex, ey, rx, ry):
        return pk(lambda u, v: ((u - ex) / rx) ** 2 + ((v - ey) / ry) ** 2 <= 1)

    def seg(x0, y0, x1, y1, wd):
        def fn(u, v):
            dx, dy = x1 - x0, y1 - y0
            t = ((u - x0) * dx + (v - y0) * dy) / (dx * dx + dy * dy)
            return 0 <= t <= 1 and abs((u - x0) * dy - (v - y0) * dx) / math.hypot(dx, dy) <= wd / 2
        return pk(fn)

    def tri(a, b, c):
        def fn(u, v):
            def side(p, q):
                return (q[0] - p[0]) * (v - p[1]) - (q[1] - p[1]) * (u - p[0])
            s1, s2, s3 = side(a, b), side(b, c), side(c, a)
            return (s1 >= 0 and s2 >= 0 and s3 >= 0) or (s1 <= 0 and s2 <= 0 and s3 <= 0)
        return pk(fn)

    def gold(m, shift=0, tips=None):  # a piece of it: its lines cut in deep gold, not drawn in ink
        c = Cv(W, H)
        pillow(c, m, GOLD, shift=shift)
        if tips:
            for x, y in m:
                if (y + .5) / f - oy < tips:
                    c.put(x, y, GOLD[3] if (x + .5) / f - ox < 7 else GOLD[4])
        return c.outline(GOLD[4])
    st = Cv(W, H)
    st.over(gold(seg(9.4, 13.2, 11.4, 11.2, 1.3) | seg(10.6, 11.6, 12.6, 9.0, 1.8) | seg(11.8, 9.4, 13.9, 5.4, 2.6), 1))  # the tail
    st.over(gold(tri((4.0, 4.7), (5.9, 3.5), (2.3, -2.2)), 0, -.1))  # the long ears, their tips darker
    st.over(gold(tri((8.1, 3.5), (10.0, 4.7), (11.7, -2.2)), 0, -.1))
    st.over(gold(pk(lambda u, v: ((u - 7) / (3.1 + .3 * (v - 11.4))) ** 2 + ((v - 11.5) / 3.3) ** 2 <= 1)
                 | ell(5.2, 14.6, 1.6, .85) | ell(8.8, 14.6, 1.6, .85)))  # the body, on its feet
    if big:
        st.over(gold(ell(5.5, 10.6, .95, .8) | ell(8.5, 10.6, .95, .8)))  # its paws, held up
    st.over(gold(ell(7.0, 6.5, 4.5, 3.5)))  # its head
    cv.over(st.outline())  # (ink only round the outside)
    cut = C('#5a3206')  # deeper than the gold's own shade: the eyes and mouth are hollows
    for (x, y) in ell(5.4, 6.2, .75, 1.0 if big else .7) | ell(8.6, 6.2, .75, 1.0 if big else .7):  # its eyes, cut in
        cv.put(x, y, cut)
    if big:
        for ex in (5.2, 8.4):
            cv.put(int((ox + ex) * f), int((oy + 5.8) * f), GOLD[1])
        for x, y in ell(3.9, 7.9, .85, .75) | ell(10.1, 7.9, .85, .75):  # its cheeks, raised and lit
            cv.put(x, y, GOLD[1])
        mx, my = int((ox + 7.0) * f), int((oy + 7.9) * f)
        for dx, dy in ((-2, 0), (-1, 1), (0, 0), (1, 1), (2, 0)):  # its mouth
            cv.put(mx + dx, my + dy, cut)
        cv.put(mx, my - 1, cut)  # its nose
    glint(cv, [(int((ox + 4.4) * f), int((oy + 4.2) * f))] + ([(int((ox + 4.4) * f) + 1, int((oy + 4.2) * f))] if big else []))
    # sparkles in the glass (only once it's won)
    sp = C('#ffffff', 255, True)
    for u, v in ((3.9, 9.6), (18.6, 17.4), (16.6, 4.6)):
        x, y = int(u * f), int(v * f)
        r = 2 if big else 1
        for d in range(-r, r + 1):
            cv.put(x + d, y, sp)
            cv.put(x, y + d, sp)
    return cv


# ---------------------------------------------------------------- the level 100 cups
CUPS = [  # MED_LV100 then MED_LV100_3 / 5 / 10: bronze with a star, silver 3, gold 5, the grand cup 10
    (BRONZE, '*', False), (SILVER, '3', False), (GOLD, '5', False), (GOLD, '10', True)]
CUP_SIZE_S = [(18, 24), (20, 26), (22, 28), (26, 31)]
CUP_SIZE_B = [(30, 38), (32, 40), (34, 42), (38, 46)]
DIG_S = {
    '0': ["###", "#.#", "#.#", "#.#", "###"], '1': [".#", "##", ".#", ".#", ".#"],
    '2': ["###", "..#", "###", "#..", "###"], '3': ["###", "..#", ".##", "..#", "###"],
    '5': ["###", "#..", "###", "..#", "###"], '7': ["###", "..#", ".#.", ".#.", ".#."],
}
DIG_B = {
    '0': [".###.", "##.##", "##.##", "##.##", "##.##", "##.##", ".###."],
    '1': [".##", "###", ".##", ".##", ".##", ".##", ".##"],
    '2': [".###.", "##.##", "...##", "..##.", ".##..", "##...", "#####"],
    '3': ["####.", "...##", "...##", ".###.", "...##", "...##", "####."],
    '5': ["#####", "##...", "####.", "...##", "...##", "##.##", ".###."],
    '7': ["#####", "...##", "..##.", "..##.", ".##..", ".##..", ".##.."],
}
STAR_S = ["..#..", "#####", ".###.", ".#.#."]
STAR_B = ["....#....", "...###...", "#########", ".#######.", "..#####..", "..##.##..", ".##...##."]


def text_rows(s, font):
    rows = None
    for ch in s:
        g = font[ch]
        rows = g[:] if rows is None else [a + '.' + b for a, b in zip(rows, g)]
    return rows


def engrave(cv, rows, ox, oy, deep, light):
    """a mark cut in the metal: dark, lit along its lower right edge"""
    for j, row in enumerate(rows):
        for i, ch in enumerate(row):
            if ch == '#':
                cv.put(ox + i, oy + j, deep)
    for j, row in enumerate(rows):
        for i, ch in enumerate(row):
            if ch == '#':
                for dx, dy in ((1, 0), (0, 1)):
                    ii, jj = i + dx, j + dy
                    inside = 0 <= jj < len(rows) and 0 <= ii < len(rows[jj]) and rows[jj][ii] == '#'
                    if not inside and cv.get(ox + ii, oy + jj) not in (None, K, deep):
                        cv.put(ox + ii, oy + jj, light)


def emboss(cv, rows, ox, oy, light, deep):
    """a mark raised from the metal: bright, a dark rim round it"""
    for j, row in enumerate(rows):
        for i, ch in enumerate(row):
            if ch == '#':
                cv.put(ox + i, oy + j, light)
    for j, row in enumerate(rows):
        for i, ch in enumerate(row):
            if ch == '#':
                for dx, dy in ((1, 0), (0, 1), (-1, 0), (0, -1)):
                    ii, jj = i + dx, j + dy
                    inside = 0 <= jj < len(rows) and 0 <= ii < len(rows[jj]) and rows[jj][ii] == '#'
                    if not inside and cv.get(ox + ii, oy + jj) not in (None, K, light):
                        cv.put(ox + ii, oy + jj, deep)


def cyl(u):
    return 2 if u < -.84 else 1 if u < -.42 else 0 if u < -.24 else 2 if u < .30 else 3 if u < .74 else 4


def cup(i, big):
    metal, mark, grand = CUPS[i]
    W, H = (CUP_SIZE_B if big else CUP_SIZE_S)[i]
    cv = Cv(W, H)
    cx = (W - 1) / 2
    hb = max(3, round(H * .15))  # the base, two tiers of ebony, a plate on it
    yb0 = H - 1 - hb
    wb = round(W * .66) // 2 * 2 + (W % 2)
    lower = {(x, y) for x in range(int(cx - wb / 2 + .5), int(cx + wb / 2 + .5)) for y in range(yb0, H - 1)}
    ht = max(2, round(hb * .55))
    wu = round(W * .48) // 2 * 2 + (W % 2)
    upper = {(x, y) for x in range(int(cx - wu / 2 + .5), int(cx + wu / 2 + .5)) for y in range(yb0 - ht, yb0)}

    def ebc(x, y, s):
        xs = sorted(xx for xx, yy in s if yy == y)
        ys = sorted(yy for xx, yy in s if xx == x)
        if y == ys[0]:
            return EBONY[1]
        if x == xs[-1] or y == ys[-1]:
            return EBONY[3]
        return EBONY[2]
    cv.over(part(W, H, lower, lambda x, y: ebc(x, y, lower)))
    pw = max(4, round(wb * .5))
    ph = 2 if big else 1
    py = yb0 + (hb - ph) // 2
    plate = {(x, y) for x in range(int(cx - pw / 2 + .5), int(cx + pw / 2 + .5)) for y in range(py, py + ph)}
    for x, y in plate:
        cv.put(x, y, metal[1] if y == py else metal[3])
    cv.over(part(W, H, upper, lambda x, y: ebc(x, y, upper)))
    # the metal: bowl, handles, stem, foot (and the grand cup's lid)
    ytop = 1 + (round(H * .13) if grand else 0)
    ybb = round(H * .56)
    Rw = W * (.30 if grand else .31)
    yf1 = yb0 - ht - 1  # the foot's bottom row
    hf = max(2, round(H * .08))
    metal_m = {}

    def hw_at(y):
        if ytop <= y <= ybb:
            t = (y - ytop) / max(1, ybb - ytop)
            return Rw * (1 - .80 * t ** 2.3)
        if yf1 - hf < y <= yf1:  # the foot, flaring out
            t = (y - (yf1 - hf)) / hf
            return W * (.09 + .11 * t)
        if ybb < y <= yf1 - hf:  # the stem, a knot halfway
            mid = (ybb + yf1 - hf) / 2
            return W * (.10 if abs(y - mid) < (1.5 if big else 1) else .055) + .5
        return 0
    for y in range(ytop, yf1 + 1):
        hw = hw_at(y)
        for x in range(W):
            if abs(x - cx) <= hw:
                u = (x - cx) / max(hw, .5)
                lv = cyl(u) if hw >= 3 else (1 if u < 0 else 3)
                if ytop <= y <= ybb and (y - ytop) / max(1, ybb - ytop) > .78 and lv < 4:
                    lv += 1
                metal_m[(x, y)] = metal[lv]
    # the rim, seen a little from above: its lip lit, the inside dark
    for x in range(W):
        if (x, ytop) in metal_m:
            metal_m[(x, ytop)] = metal[1]
        for k in range(1, 3 if big else 2):
            if (x, ytop + k) in metal_m and abs(x - cx) < Rw - (1.6 if big else 1.2):
                metal_m[(x, ytop + k)] = metal[4] if k == 1 else metal[3]
    body = part(W, H, set(metal_m), lambda x, y: metal_m[(x, y)])
    # the handles: a loop each side, its upper half lit
    hy = ytop + (ybb - ytop) * .34
    ro = W * (.16 if not grand else .17)
    ri = ro - (2.4 if big else 1.25)
    hm = {}
    for s in (-1, 1):
        hcx = cx + s * (Rw - .4)
        for y in range(H):
            for x in range(W):
                d = math.hypot(x - hcx, (y - hy) * .9)
                if ri < d <= ro and s * (x - cx) > Rw * .78:
                    hm[(x, y)] = metal[1] if y < hy - ro * .3 else metal[3] if y > hy + ro * .3 else metal[2]
    cv.over(part(W, H, set(hm), lambda x, y: hm[(x, y)]))
    cv.over(body)
    if grand:  # a lid with a knob
        lid = {(x, y) for y in range(H) for x in range(W)
               if y < ytop and ((x - cx) / (Rw * .78)) ** 2 + ((y - ytop + .5) / (ytop - 1.2)) ** 2 <= 1 and y >= 2}
        cv.over(part(W, H, lid, lambda x, y: metal[sph((x - cx) / (Rw * .78), (y - ytop) / (ytop - 1.0))]))
        kr = 2.2 if big else 1.2
        knob = {(x, y) for y in range(H) for x in range(W) if math.hypot(x - cx, y - (1 + kr)) <= kr}
        cv.over(part(W, H, knob, lambda x, y: metal[sph((x - cx) / kr, (y - 1 - kr) / kr)]))
    # the mark on its front: a star, or how many Pokemon raised to level 100
    font, star = (DIG_B, STAR_B) if big else (DIG_S, STAR_S)
    rows = star if mark == '*' else text_rows(mark, font)
    mw, mh = len(rows[0]), len(rows)
    my = int(ytop + (ybb - ytop) * .40 - mh / 2 + (1 if big else 1))
    if mark == '*':
        emboss(cv, rows, int(round(cx - mw / 2 + .5)), my, metal[0], metal[4])
    else:
        engrave(cv, rows, int(round(cx - mw / 2 + .5)), my, metal[4], metal[0])
    if grand:  # and a ruby under it
        gx, gy = int(round(cx)), my + mh + (2 if big else 1)
        rr = 1.6 if big else .8
        for y in range(gy - 3, gy + 4):
            for x in range(gx - 3, gx + 4):
                if abs(x - cx) + abs(y - gy) <= rr * 1.6:
                    cv.put(x, y, RED[1] if x < cx and y < gy else RED[2] if x < cx or y < gy else RED[3])
    cv.outline()
    return cv


# ---------------------------------------------------------------- the buddy frames
CRIT = {  # a little Pokemon in a photo: its colours, ears, cheeks
    'pika': (YELLOW, 'pika'), 'eevee': (R('#f8e0c0', '#d8a670', '#b07a46', '#86562c', '#5a3818'), 'eevee'),
    'bulb': (MINT, 'bulb'), 'char': (ORANGE, 'char'), 'squirt': (BLUE, 'round'), 'gengar': (PURPLE, 'spiky'),
}


def critter(kind, cx, cy, r, big, W, H, face=1):
    rp, ears = CRIT[kind]
    cv = Cv(W, H)
    body = {(x, y) for y in range(H) for x in range(W) if ((x + .5 - cx) / r) ** 2 + ((y + .5 - cy) / (r * .92)) ** 2 <= 1}
    extra = set()
    if ears in ('pika', 'eevee'):
        for s in (-1, 1):
            ex = cx + s * r * .62
            for k in range(int(r * (1.1 if ears == 'pika' else .9)) + 1):
                wdt = max(0, (r * (.42 if ears == 'eevee' else .30)) * (1 - k / (r * 1.1 + .01)))
                for x in range(W):
                    if abs(x + .5 - (ex + s * k * .25)) <= wdt + .3:
                        extra.add((x, int(cy - r * .7 - k)))
    if ears == 'spiky':
        for s in (-1, 0, 1):
            for k in range(int(r * .7) + 1):
                extra.add((int(cx + s * r * .55), int(cy - r * .8 - k)))
    pillow(cv, body | extra, rp, hi=False)
    if ears == 'pika':  # black tips
        tips = sorted(extra, key=lambda p: p[1])[:2 if not big else 4]
        for p in tips:
            cv.put(p[0], p[1], K)
    if ears == 'bulb':  # a bulb on its back
        bm = {(x, y) for y in range(H) for x in range(W) if ((x + .5 - (cx - face * r * .25)) / (r * .62)) ** 2 + ((y + .5 - (cy - r * .82)) / (r * .5)) ** 2 <= 1}
        b2 = Cv(W, H)
        pillow(b2, bm, GREEN, hi=False)
        cv.over(b2)
    cv.outline()
    # its eyes (and Pikachu's cheeks)
    ey = int(cy - r * .05)
    if big:
        for s in (-1, 1):
            ex = int(cx + s * r * .38 + face * r * .12)
            cv.put(ex, ey, K)
            cv.put(ex, ey + 1, K)
        if ears == 'pika':
            for s in (-1, 1):
                cv.put(int(cx + s * r * .66), ey + 2, C('#e8503a'))
    else:
        for s in (-1, 1):
            cv.put(int(cx + s * r * .45 + face * .3), ey, K)
    return cv


def frame(kind, big):
    W, H = (46, 40) if big else (26, 22)
    t = 6 if big else 4
    cv = Cv(W, H)
    # the picture: sky, a hill, Pokemon
    pic = {(x, y) for x in range(t - 1, W - t + 1) for y in range(t - 1, H - t + 1)}
    ph = H - 2 * (t - 1)
    gy = t - 1 + int(ph * .62)
    for x, y in pic:
        if y >= gy - (1 if (x * 7 % 11) < 3 else 0) - int(1.5 * math.sin(x * .35)):
            cv.put(x, y, GRASS[1] if y == gy else GRASS[2] if (x + y) % 5 else GRASS[3])
        else:
            cv.put(x, y, SKY[min(4, 1 + (y - t + 1) * 3 // max(1, gy - t + 1))])
    s = 2.2 if big else 1.0
    if kind == 0:  # Buddy: two friends, a heart over them
        cast = [('pika', -.24, 1), ('eevee', .24, -1)]
        hx, hy = W / 2 - .5, t + (5.5 if big else 2.6)
        hm = {(x, y) for y in range(H) for x in range(W)
              if (lambda a, b: (a * a + b * b - 1) ** 3 - a * a * b ** 3 <= 0)((x + .5 - hx) / (2.0 * s), -(y + .5 - hy) / (1.9 * s))}
        hc = Cv(W, H)
        for x, y in hm:
            hc.put(x, y, PINK[1] if x < hx and y < hy else PINK[2])
        if big:
            hc.outline()
        cv.over(hc, clip=pic)
    elif kind == 1:  # Party: five, three behind and two in front
        cast = [('bulb', -.31, 1), ('gengar', 0, 1), ('squirt', .31, -1), ('char', -.16, 1), ('pika', .16, -1)]
    else:  # Playmate: two playing, a handheld between them
        cast = [('pika', -.29, 1), ('char', .29, -1)]
    for n, (kd, dx, face) in enumerate(cast):
        r = (4.8 if big else 2.4) if kind != 1 else (3.6 if big else 1.8)
        back = kind == 1 and n < 3
        cy = gy + (r * .1 if kind != 1 else (-r * .7 if back else r * .6))
        cx = W / 2 + dx * (W - 2 * t)
        cv.over(critter(kd, cx, cy, r, big, W, H, face), clip=pic)
    if kind == 2:
        tw, th = (8, 6) if big else (4, 3)
        tx, ty = int(W / 2 - tw / 2), int(gy + (1 if big else 0) - th / 2)
        tvm = {(x, y) for x in range(tx, tx + tw) for y in range(ty, ty + th)}
        tv = part(W, H, tvm, lambda x, y: C('#7ce0a0') if tx < x < tx + tw - 1 and ty < y < ty + th - (2 if big else 1) else TV[1] if y == ty else TV[2])
        cv.over(tv, clip=pic)
    # the frame: gold, lit on its top and left, a stud at each corner
    fr = {(x, y) for x in range(W) for y in range(H) if not (t <= x < W - t and t <= y < H - t)}
    for x, y in fr:
        if x in (0, W - 1) or y in (0, H - 1) or x in (t - 1, W - t) and t - 1 <= y <= H - t or y in (t - 1, H - t) and t - 1 <= x <= W - t:
            cv.put(x, y, K)
            continue
        if y < t - 1 and x < W - y - 1 or x < t - 1 and y < H - x - 1:
            lv = 1
        else:
            lv = 3
        if big and (y in (1,) or x in (1,)) and lv == 1:
            lv = 0 if (x + y) % 6 == 0 else 1
        if big and (y == t - 2 and t - 2 <= x <= W - t + 1 or x == t - 2 and t - 2 <= y <= H - t + 1):
            lv = 2 if lv == 1 else 4
        cv.put(x, y, GOLD[lv])
    st = 2 if big else 1
    for sx, sy in ((1, 1), (W - 1 - st, 1), (1, H - 1 - st), (W - 1 - st, H - 1 - st)):
        for dx in range(st):
            for dy in range(st):
                cv.put(sx + dx + (1 if big else 0), sy + dy + (1 if big else 0), GOLD[0])
    return cv


# ---------------------------------------------------------------- the rosettes
ROSETTE = [  # the pet's eight: its ribbon, the mark in its middle
    (GREEN, '10'), (BLUE, '25'), (PURPLE, '50'), (RED, 'berry'), (ORANGE, 'flame'), (PINK, 'heart'), (GOLD, 'star'),
    (MINT, 'dumbbell')]
EMB_S = {  # small marks, in the medallion
    'berry': (["...g.", ".bbg.", "bwbbb", "bbbbB", ".bBB."], {'g': C('#58b868'), 'b': C('#4f93c4'), 'B': C('#2a5a9a'), 'w': WH}),
    'flame': (["..r..", ".rr..", ".ryr.", "ryyyr", ".ryr."], {'r': C('#e8503a'), 'y': C('#f8c030')}),
    'heart': (["rr.rr", "rwrrR", "rrrrR", ".rrR.", "..R.."], {'r': C('#e8503a'), 'R': C('#b53224'), 'w': WH}),
    'star': (["..y..", "yyyyY", ".yyY.", ".y.Y."], {'y': C('#f8c030'), 'Y': C('#c07a10')}),
    'dumbbell': (["ss...sS", "sssssSS", "ss...SS"], {'s': C('#7f889c'), 'S': C('#535a6c')}),
}


def emblem_big(kind, W, H, cx, cy):
    """a big mark, drawn from its shape: shaded and outlined"""
    cv = Cv(W, H)
    if kind == 'heart':
        m = {(x, y) for y in range(H) for x in range(W)
             if (lambda a, b: (a * a + b * b - 1) ** 3 - a * a * b ** 3 <= 0)((x + .5 - cx) / 5.4, -(y + .5 - cy + .6) / 5.0)}
        pillow(cv, m, RED)
    elif kind == 'star':
        def star(u, v):
            a = math.atan2(v, u) + math.pi / 2
            rr = math.hypot(u, v)
            k = (a / (2 * math.pi / 5)) % 1
            lim = 6.2 * (1 - .56 * (1 - abs(1 - 2 * k)) ** 1.0)
            return rr <= lim
        m = {(x, y) for y in range(H) for x in range(W) if star(x + .5 - cx, y + .5 - cy - .4)}
        pillow(cv, m, YELLOW)
    elif kind == 'flame':
        def fl(u, v, s=1.0):
            v2 = v + 1.2
            if v2 > 0:
                return u * u / (4.2 * s) ** 2 + v2 * v2 / (4.0 * s) ** 2 <= 1
            return v2 >= -7.6 * s and abs(u + v2 * .18) <= 4.2 * s * max(0.0, 1 + v2 / (7.6 * s)) ** 1.3
        m = {(x, y) for y in range(H) for x in range(W) if fl(x + .5 - cx, y + .5 - cy)}
        for x, y in m:
            cv.put(x, y, ORANGE[2] if x >= cx + 1 else ORANGE[1])
        inner = {(x, y) for y in range(H) for x in range(W) if fl(x + .5 - cx, y + .5 - cy - 1.4, .55)}
        for x, y in inner:
            cv.put(x, y, YELLOW[1] if x < cx else YELLOW[2])
    elif kind == 'berry':
        m = {(x, y) for y in range(H) for x in range(W) if math.hypot(x + .5 - cx, (y + .5 - cy - 1.0) * 1.05) <= 5.2}
        pillow(cv, m, BLUE)
        lf = {(x, y) for y in range(H) for x in range(W) if ((x + .5 - cx - 2.2) / 2.6) ** 2 + ((y + .5 - cy + 4.6) / 1.4) ** 2 <= 1}
        l2 = Cv(W, H)
        pillow(l2, lf, GREEN, hi=False)
        cv.outline()
        cv.over(l2.outline())
        return cv
    elif kind == 'dumbbell':
        m = {(x, y) for y in range(H) for x in range(W)
             if (abs(x + .5 - cx) <= 6.4 and abs(y + .5 - cy) <= 1.0) or (3.4 <= abs(x + .5 - cx) <= 6.4 and abs(y + .5 - cy) <= 3.6)}
        for x, y in m:
            cv.put(x, y, SILVER[1] if y + .5 < cy - 1 else SILVER[3] if y + .5 > cy + 1.5 else SILVER[2])
    return cv.outline()


def rosette(i, big):
    rp, mark = ROSETTE[i]
    W, H = (33, 44) if big else (16, 21)
    f = W / 16
    cx, cy = W / 2, 7.6 * f
    cv = Cv(W, H)
    # two tails, a V cut in each end
    for s in (-1, 1):
        def tail(u, v, s=s):
            v0, v1 = 9.0, 20.2
            if not (v0 <= v <= v1):
                return False
            t = (v - v0) / (v1 - v0)
            xc = 8 + s * (1.6 + t * 2.2)
            if abs(u - xc) > 1.9:
                return False
            return v <= v1 - 1.6 * (1 - abs(u - xc) / 1.9)
        m = mask(W, H, tail, f)
        cv.over(part(W, H, m, lambda x, y, s=s: rp[2] if (s < 0) != ((x + .5) / f > 8 + s * (1.6 + ((y + .5) / f - 9) / 11.2 * 2.2)) else rp[3]))
    # the ruffled disc, its pleats lit and shaded in turn
    n = 16 if big else 12

    def disc(u, v):
        a = math.atan2(v - 7.6, u - 8)
        return math.hypot(u - 8, v - 7.6) <= 7.3 - .55 * (1 - abs(math.cos(n * a / 2)))
    dm = mask(W, H, disc, f)

    def dc(x, y):
        u, v = (x + .5) / f - 8, (y + .5) / f - 7.6
        a = math.atan2(v, u)
        k = int(((a + math.pi) / (2 * math.pi)) * n) % 2
        lv = sph(u / 7.4, v / 7.4, False)
        return rp[max(1, min(4, lv + k))]
    cv.over(part(W, H, dm, dc))
    # the medallion in its middle, the mark on it
    rm = 4.6
    mm = mask(W, H, lambda u, v: math.hypot(u - 8, v - 7.6) <= rm, f)
    med = Cv(W, H)
    pillow(med, mm, CREAM, hi=False, sq=.8)
    cv.over(med.outline() if big else med)
    if big:
        if mark.isdigit():
            rows = text_rows(mark, DIG_B)
            engrave(cv, rows, int(round(cx - len(rows[0]) / 2)), int(round(cy - len(rows) / 2)), rp[4], CREAM[0])
        else:
            cv.over(emblem_big(mark, W, H, cx, cy))
    else:
        if mark.isdigit():
            rows = text_rows(mark, DIG_S)
            for j, row in enumerate(rows):
                for k, ch in enumerate(row):
                    if ch == '#':
                        cv.put(int(round(cx - len(rows[0]) / 2)) + k, int(round(cy - len(rows) / 2)) + j, rp[4])
        else:
            rows, keys = EMB_S[mark]
            cv.stamp(rows, keys, int(round(cx - len(rows[0]) / 2)), int(round(cy - len(rows) / 2)))
    glint(cv, [(int(3.0 * f), int(3.4 * f))] + ([(int(3.0 * f) + 1, int(3.4 * f))] if big else []))
    return cv


# ---------------------------------------------------------------- all 26, by medal number
def art(m, big):
    return art_(m, big).trim()


def art_(m, big):
    if 17 <= m <= 21:
        return badge(m - 17, big)
    if m == 25:  # Knomi Tower's badge, in the case with the zones'
        return badge(5, big)
    if 9 <= m <= 12:
        return statue(m - 9, big)
    if m == 8:
        return gem_case(big)
    if m == 13:
        return cup(0, big)
    if 22 <= m <= 24:
        return cup(m - 21, big)
    if 14 <= m <= 16:
        return frame(m - 14, big)
    return rosette(m, big)


MED_COUNT = 26
ORDER = [17, 18, 19, 20, 21, 25, 9, 10, 11, 12, 8, 13, 22, 23, 24, 14, 15, 16, 0, 1, 2, 3, 4, 5, 6, 7]  # the room's


# ---------------------------------------------------------------- looking at them
def hexrgb(h):
    return int(h[1:3], 16), int(h[3:5], 16), int(h[5:7], 16)


SIL, SIL_D = '#4a2e1e', '#3a2214'


def paint(img, IW, cv, ox, oy, s, won):
    for y in range(cv.h):
        for x in range(cv.w):
            c = cv.p[y][x]
            if c is None:
                continue
            h, a, wonly = c
            if not won:
                if a < 255 or wonly:
                    continue
                h, a = (SIL_D if c == K else SIL), 255
            r, g, b = hexrgb(h)
            for dy in range(s):
                for dx in range(s):
                    i = ((oy + y * s + dy) * IW + ox + x * s + dx) * 3
                    if i < 0 or i + 2 >= len(img):
                        continue
                    if a == 255:
                        img[i:i + 3] = bytes((r, g, b))
                    else:
                        img[i] = (img[i] * (255 - a) + r * a) // 255
                        img[i + 1] = (img[i + 1] * (255 - a) + g * a) // 255
                        img[i + 2] = (img[i + 2] * (255 - a) + b * a) // 255


def sheet(path):
    """every trophy: small at 2x (won, then in shadow) over big at 3x, on the room's wall"""
    cell_s, cell_b = 64, 150
    cols = 9
    rows_n = (MED_COUNT + cols - 1) // cols
    IW = cols * cell_b
    IH = rows_n * (cell_s + cell_b + 10)
    img = bytearray(bytes(hexrgb('#8a5a3a')) * (IW * IH))
    for n, m in enumerate(ORDER):
        c, r = n % cols, n // cols
        x0, y0 = c * cell_b, r * (cell_s + cell_b + 10)
        sm, bg = art(m, False), art(m, True)
        paint(img, IW, sm, x0 + 8, y0 + 4 + (cell_s - 8 - sm.h * 2), 2, True)
        paint(img, IW, sm, x0 + 8 + 70, y0 + 4 + (cell_s - 8 - sm.h * 2), 2, False)
        paint(img, IW, bg, x0 + (cell_b - bg.w * 3) // 2, y0 + cell_s + (cell_b - bg.h * 3), 3, True)
    ppm = path[:-4] + '.ppm'
    with open(ppm, 'wb') as fh:
        fh.write(b'P6 %d %d 255\n' % (IW, IH) + bytes(img))
    subprocess.run(['magick', ppm, path], check=True)
    os.remove(ppm)
    print('saved', path)


# ---------------------------------------------------------------- the header
def emit(path='trophies_v2.h'):
    pal = [None]
    idx = {}

    def pi(c):
        if c not in idx:
            idx[c] = len(pal)
            pal.append(c)
        return idx[c]
    pi(K)
    out = []
    data = []
    for big in (False, True):
        for m in range(MED_COUNT):
            cv = art(m, big)
            px = [0 if c is None else pi(c) for row in cv.p for c in row]
            data.append((m, big, cv.w, cv.h, px))
    assert len(pal) <= 256, len(pal)
    out.append('#pragma once\n// GENERATED by tools/trophies.py - edit there and run:\n//   python3 tools/trophies.py emit\n')
    out.append('// The trophy room\'s %d medals as pixel art' % MED_COUNT + ', small for the room (2 px a pixel) and big for a medal\'s card (3 px),\n')
    out.append('// each pixel an index in TR_PAL (0: nothing). TR_PAL_A: 255 solid, less is glass seen through; TR_PAL_WON:\n')
    out.append('// drawn only on a medal won (its sparkles). Index 1 is the outline.\n\n')
    out.append('struct TrArt { uint8_t w, h; const uint8_t *px; };\n\n')
    out.append('static const uint32_t TR_PAL[%d] = {\n' % len(pal))
    for i in range(0, len(pal), 8):
        out.append('  ' + ', '.join('0x000000' if c is None else '0x' + c[0][1:].upper() for c in pal[i:i + 8]) + ',\n')
    out.append('};\n')
    out.append('static const uint8_t TR_PAL_A[%d] = {\n' % len(pal))
    for i in range(0, len(pal), 16):
        out.append('  ' + ', '.join('0' if c is None else str(c[1]) for c in pal[i:i + 16]) + ',\n')
    out.append('};\n')
    out.append('static const uint8_t TR_PAL_WON[%d] = {\n' % len(pal))
    for i in range(0, len(pal), 16):
        out.append('  ' + ', '.join('0' if c is None else str(int(c[2])) for c in pal[i:i + 16]) + ',\n')
    out.append('};\n\n')
    for m, big, w, h, px in data:
        out.append('static const uint8_t TR_PX_%s%d[%d] = {\n' % ('B' if big else 'S', m, w * h))
        for j in range(0, len(px), w):
            out.append('  ' + ','.join(str(v) for v in px[j:j + w]) + ',\n')
        out.append('};\n')
    for big in (False, True):
        out.append('\nstatic const TrArt TR_ART_%s[%d] = {\n' % ('BIG' if big else 'SMALL', MED_COUNT))
        for m, b, w, h, px in data:
            if b == big:
                out.append('  { %d, %d, TR_PX_%s%d },\n' % (w, h, 'B' if big else 'S', m))
        out.append('};\n')
    open(path, 'w').write(''.join(out))
    print('saved %s: %d colours, %d bytes of pixels' % (path, len(pal), sum(w * h for _, _, w, h, _ in data)))


if __name__ == '__main__':
    os.makedirs('build', exist_ok=True)
    sheet('build/trophies.png')
    if 'emit' in sys.argv:
        emit()
