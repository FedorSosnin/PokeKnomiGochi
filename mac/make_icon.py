#!/usr/bin/env python3
"""Turns a raw frame of the game (466x466 RGBA) into the app icon: the round screen in its Pokeball
ring, like the window, on a transparent square. Writes a 1024 px PNG; make_icon.sh does the rest.

  python3 mac/make_icon.py frame.rgba icon_1024.png
"""
import struct
import sys
import zlib

CANVAS = 466
SIZE = 1024
MARGIN = 100 / 1024    # Apple's template leaves this much air around the art
RING = 26 / 1024       # the window's 6 pt ring, scaled to an icon
OUTLINE = 6 / 1024
BAND = 70 / 1024       # the black band in the middle
RED, WHITE, BLACK = (222, 33, 33), (255, 255, 255), (0, 0, 0)


def png(path, w, h, rows):
    raw = b''.join(b'\0' + bytes(r) for r in rows)
    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def main():
    frame = open(sys.argv[1], 'rb').read()
    assert len(frame) == CANVAS * CANVAS * 4
    c = SIZE / 2
    r_out = SIZE * (0.5 - MARGIN)
    r_in = r_out - SIZE * RING
    ss = 4  # 4x4 samples per pixel for smooth edges
    rows = []
    for y in range(SIZE):
        row = bytearray(SIZE * 4)
        for x in range(SIZE):
            acc = [0, 0, 0, 0]
            for sy in range(ss):
                for sx in range(ss):
                    px, py = x + (sx + 0.5) / ss, y + (sy + 0.5) / ss
                    d = ((px - c) ** 2 + (py - c) ** 2) ** 0.5
                    if d > r_out:
                        continue
                    if d <= r_in:
                        u = int((px - (c - r_in)) / (2 * r_in) * CANVAS)
                        v = int((py - (c - r_in)) / (2 * r_in) * CANVAS)
                        i = (min(v, CANVAS - 1) * CANVAS + min(u, CANVAS - 1)) * 4
                        col = frame[i:i + 3]
                    elif d > r_out - SIZE * OUTLINE or abs(py - c) < SIZE * BAND / 2:
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
