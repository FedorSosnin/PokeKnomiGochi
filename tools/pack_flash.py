#!/usr/bin/env python3
"""Packs tools/sdcard/mons/* into a blob for boards without an SD card (BTT Knomi V2).

It is written raw to the "ffat" partition and read by flashpak.h. Format (little endian):

  char[4] "TPKZ"
  u32 nEntries
  per entry: char name[16], u32 offset, u32 rawSize, u32 compSize
  zlib data

  python3 tools/pack_flash.py [salida]
"""
import os
import struct
import sys
import zlib

SRC = os.path.join(os.path.dirname(__file__), 'sdcard', 'mons')
LIMIT = 0x9E0000  # size of the ffat partition in app3M_fat9M_16MB


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else 'build-knomi/sprites.tpkz'
    names = sorted(n for n in os.listdir(SRC) if n.endswith('.bin'))
    head = 8 + len(names) * 28
    entries, blobs, off = [], [], head
    raw_total = 0
    for n in names:
        with open(os.path.join(SRC, n), 'rb') as f:
            raw = f.read()
        comp = zlib.compress(raw, 9)
        entries.append(struct.pack('<16sIII', n.encode(), off, len(raw), len(comp)))
        blobs.append(comp)
        off += len(comp)
        raw_total += len(raw)
    if off > LIMIT:
        sys.exit(f'{off} bytes do not fit in the partition ({LIMIT})')
    os.makedirs(os.path.dirname(out) or '.', exist_ok=True)
    with open(out, 'wb') as f:
        f.write(b'TPKZ' + struct.pack('<I', len(names)))
        f.write(b''.join(entries))
        f.write(b''.join(blobs))
    print(f'{len(names)} files, {raw_total / 1e6:.1f} MB -> {off / 1e6:.2f} MB: {out}')


if __name__ == '__main__':
    main()
