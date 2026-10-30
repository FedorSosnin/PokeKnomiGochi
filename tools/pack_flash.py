#!/usr/bin/env python3
"""Empaqueta tools/sdcard/mons/* en un blob para placas sin SD (BTT Knomi V2).

Se graba en crudo en la particion "ffat" y lo lee flashpak.h. Formato (little endian):

  char[4] "TPKZ"
  u32 nEntries
  por entrada: char name[16], u32 offset, u32 rawSize, u32 compSize
  datos zlib

  python3 tools/pack_flash.py [salida]
"""
import os
import struct
import sys
import zlib

SRC = os.path.join(os.path.dirname(__file__), 'sdcard', 'mons')
LIMIT = 0x9E0000  # tamano de la particion ffat en app3M_fat9M_16MB


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
        sys.exit(f'{off} bytes no caben en la particion ({LIMIT})')
    os.makedirs(os.path.dirname(out) or '.', exist_ok=True)
    with open(out, 'wb') as f:
        f.write(b'TPKZ' + struct.pack('<I', len(names)))
        f.write(b''.join(entries))
        f.write(b''.join(blobs))
    print(f'{len(names)} ficheros, {raw_total / 1e6:.1f} MB -> {off / 1e6:.2f} MB: {out}')


if __name__ == '__main__':
    main()
