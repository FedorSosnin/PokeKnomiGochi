#!/usr/bin/env python3
"""Turns a raw frame of the game (466x466 RGBA) into the app icon: a Pokeball filling Apple's
rounded-square template (red top, black band, white bottom, edge to edge), with the round screen
in the middle, zoomed in on the pet so it reads at Dock size. Writes a 1024 px PNG; make_icon.sh
does the rest.

  python3 mac/make_icon.py frame.rgba icon_1024.png
"""
import struct
import sys
import zlib

CANVAS = 466
SIZE = 1024
MARGIN = 100 / 1024    # Apple's template: an 824 px rounded square in a 1024 px canvas
CORNER = 185 / 1024
BAND = 84 / 1024       # the black band across the middle
SCREEN = 600 / 1024    # diameter of the round screen
RING = 34 / 1024       # black ring around the screen
# the part of the frame shown in the screen: a square around the pet, so it fills the circle
CROP_Y, CROP_HALF = 252, 60  # Gengar, tongue and all, above the stats; across, centred on him (pet_center)
RED, WHITE, BLACK = (222, 33, 33), (255, 255, 255), (0, 0, 0)


def png(path, w, h, rows):
    raw = b''.join(b'\0' + bytes(r) for r in rows)
    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def in_rounded_square(px, py):
    lo, hi, r = SIZE * MARGIN, SIZE * (1 - MARGIN), SIZE * CORNER
    if not (lo <= px <= hi and lo <= py <= hi):
        return False
    cx = min(max(px, lo + r), hi - r)
    cy = min(max(py, lo + r), hi - r)
    return (px - cx) ** 2 + (py - cy) ** 2 <= r * r


def pet_center(frame):
    """Where the pet is across the frame: the middle of its pixels (body, tongue and dark outline)
    around where it stands. Its idle pose sways a few pixels, so the crop follows it."""
    xs = []
    for y in range(170, 320):
        for x in range(110, 356):
            r, g, b = frame[(y * CANVAS + x) * 4:(y * CANVAS + x) * 4 + 3]
            if (b > 120 and g < 120 and r > 60) or (r > 150 and g < 100 and b < 100) or (r < 40 and g < 40 and b < 50):
                xs.append(x)
    return (min(xs) + max(xs) + 1) / 2 if xs else CANVAS / 2


def main():
    frame = open(sys.argv[1], 'rb').read()
    assert len(frame) == CANVAS * CANVAS * 4
    crop_x = pet_center(frame)
    c = SIZE / 2
    r_screen = SIZE * SCREEN / 2
    r_ring = r_screen + SIZE * RING
    ss = 4  # 4x4 samples per pixel for smooth edges
    rows = []
    for y in range(SIZE):
        row = bytearray(SIZE * 4)
        for x in range(SIZE):
            acc = [0, 0, 0, 0]
            for sy in range(ss):
                for sx in range(ss):
                    px, py = x + (sx + 0.5) / ss, y + (sy + 0.5) / ss
                    if not in_rounded_square(px, py):
                        continue
                    d = ((px - c) ** 2 + (py - c) ** 2) ** 0.5
                    if d <= r_screen:
                        u = int(crop_x - CROP_HALF + (px - (c - r_screen)) / (2 * r_screen) * 2 * CROP_HALF)
                        v = int(CROP_Y - CROP_HALF + (py - (c - r_screen)) / (2 * r_screen) * 2 * CROP_HALF)
                        i = (min(max(v, 0), CANVAS - 1) * CANVAS + min(max(u, 0), CANVAS - 1)) * 4
                        col = frame[i:i + 3]
                    elif d <= r_ring or abs(py - c) < SIZE * BAND / 2:
                        col = BLACK
                    else:
                        col = RED if py < c else WHITE
                    acc[0] += col[0]
                    acc[1] += col[1]
                    acc[2] += col[2]
                    acc[3] += 255
            n = acc[3] // 255
            if n:
                row[x * 4:x * 4 + 4] = bytes((acc[0] // n, acc[1] // n, acc[2] // n, acc[3] // (ss * ss)))
        rows.append(row)
    png(sys.argv[2], SIZE, SIZE, rows)


if __name__ == '__main__':
    main()
