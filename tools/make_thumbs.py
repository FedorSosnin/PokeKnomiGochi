#!/usr/bin/env python3
"""Generates /mons/thumbs.bin: thumbnails of all 151 for the gallery (cropped to the Pokemon, at
most 40x40), then the 151 shiny ones.

They are derived from the front frame (Idle, frame 0) of the already packed PMD
sprites (tools/sdcard/mons/pNNN.bin and psNNN.bin, TPK2 format) -> legal thumbnails
(CC BY-NC), same style as the main screen. Entries 1..151 are the regular ones and
152..302 the shiny ones (dex + 151): firmware that only knows the first 151 reads them
as before. TPTH format (little-endian):

  char[4] "TPTH"
  uint16  count
  uint32  offset[count]    (from the start of the file, 1-based: offset[0]=dex 1, offset[151]=shiny dex 1)
  blobs:  u8 w, u8 h, u8 palCount, u16 pal[palCount], u8 data[w*h] (0xFF transparent)

  python3 tools/make_thumbs.py
"""
import os
import struct

DIR = os.path.join(os.path.dirname(__file__), 'sdcard', 'mons')
CELL = 40


def read_pmd_idle_frame0(path):
    """Frame 0 of the Idle action (id 0 = front view) of a TPK2 PMD sprite."""
    with open(path, 'rb') as f:
        buf = f.read()
    if buf[:4] != b'TPK2':
        raise ValueError('magic TPK2')
    nacts = buf[4]
    (palcount,) = struct.unpack_from('<H', buf, 5)
    pal = list(struct.unpack_from(f'<{palcount}H', buf, 7))
    p = 7 + palcount * 2
    for _ in range(nacts):
        aid, w, h, nf = buf[p], buf[p + 1], buf[p + 2], buf[p + 3]
        p += 4 + nf * 2  # header + ms[]
        if aid == 0:     # PMD_IDLE
            return w, h, pal, buf[p:p + w * h]  # frame 0
        p += w * h * nf
    raise ValueError('no Idle action (id 0)')


def crop(w, h, data):
    """Only what the Pokemon fills: the frame's empty margins would otherwise count against the 40
    px, and a tall frame came out shrunk (rows and columns dropped) though the Pokemon fits."""
    cols = [x for x in range(w) if any(data[y * w + x] != 0xFF for y in range(h))]
    rows = [y for y in range(h) if any(data[y * w + x] != 0xFF for x in range(w))]
    x0, x1, y0, y1 = cols[0], cols[-1] + 1, rows[0], rows[-1] + 1
    out = bytearray()
    for y in range(y0, y1):
        out += data[y * w + x0:y * w + x1]
    return x1 - x0, y1 - y0, bytes(out)


def shrink(w, h, pal, data):
    # scale to CELL x CELL with nearest neighbor, keeping the aspect ratio
    scale = min(CELL / w, CELL / h, 1.0)
    nw, nh = max(1, round(w * scale)), max(1, round(h * scale))
    out = bytearray()
    used = {}
    newpal = []
    for y in range(nh):
        sy = min(h - 1, int(y / scale)) if scale < 1 else y
        for x in range(nw):
            sx = min(w - 1, int(x / scale)) if scale < 1 else x
            idx = data[sy * w + sx]
            if idx == 0xFF:
                out.append(0xFF)
                continue
            c = pal[idx]
            if c not in used:
                used[c] = len(newpal)
                newpal.append(c)
            out.append(used[c])
    return nw, nh, newpal, bytes(out)


def main():
    blobs = []
    for n in range(1, 2 * 151 + 1):
        dex, shiny = (n - 1) % 151 + 1, n > 151
        path = os.path.join(DIR, f'p{"s" if shiny else ""}{dex:03d}.bin')
        w, h, pal, data = read_pmd_idle_frame0(path)
        w, h, data = crop(w, h, data)
        nw, nh, npal, ndata = shrink(w, h, pal, data)
        if len(npal) > 255:
            raise ValueError(f'{n}: palette {len(npal)}')
        blob = struct.pack('<3B', nw, nh, len(npal))
        blob += struct.pack(f'<{len(npal)}H', *npal)
        blob += ndata
        blobs.append(blob)

    head = 4 + 2 + 4 * len(blobs)
    offsets, pos = [], head
    for b in blobs:
        offsets.append(pos)
        pos += len(b)

    out = os.path.join(DIR, 'thumbs.bin')
    with open(out, 'wb') as f:
        f.write(b'TPTH')
        f.write(struct.pack('<H', len(blobs)))
        f.write(struct.pack(f'<{len(blobs)}I', *offsets))
        for b in blobs:
            f.write(b)
    print(f"saved {out}: {pos / 1024:.0f} KB, {len(blobs)} thumbnails")


if __name__ == '__main__':
    main()
