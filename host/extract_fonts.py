#!/usr/bin/env python3
"""Saca las dos fuentes CJK del firmware de la libreria U8g2 a host/fonts/*.u8g2.

u8g2_fonts.c pesa 37 MB; el build del host solo necesita estas dos, y no debe depender
de tener instaladas las librerias de Arduino. Solo hay que repetirlo si cambia la fuente.

  python3 host/extract_fonts.py [ruta/a/u8g2_fonts.c]
"""
import os
import re
import sys

FONTS = ['u8g2_font_unifont_t_japanese3', 'u8g2_font_unifont_t_korean2']
DEFAULT = os.path.expanduser('~/Documents/Arduino/libraries/U8g2/src/clib/u8g2_fonts.c')


def decode_c_strings(text):
    """Bytes de una secuencia de literales C adyacentes."""
    out = bytearray()
    for lit in re.findall(r'"((?:[^"\\]|\\.)*)"', text):
        i = 0
        while i < len(lit):
            c = lit[i]
            if c != '\\':
                out.append(ord(c))
                i += 1
                continue
            m = re.match(r'[0-7]{1,3}', lit[i + 1:])
            if m:
                out.append(int(m.group(), 8))
                i += 1 + len(m.group())
                continue
            out.append({'n': 10, 'r': 13, 't': 9, '"': 34, '\\': 92}[lit[i + 1]])
            i += 2
    return bytes(out)


def main():
    src = open(sys.argv[1] if len(sys.argv) > 1 else DEFAULT, encoding='latin-1').read()
    here = os.path.dirname(os.path.abspath(__file__))
    for name in FONTS:
        m = re.search(r'const uint8_t %s\[(\d+)\] U8G2_FONT_SECTION\("%s"\) =((?:\s*"(?:[^"\\]|\\.)*")+)\s*;' % (name, name), src)
        data = decode_c_strings(m.group(2))
        # el tamano declarado incluye el 0 final que anade C al literal
        assert len(data) + 1 == int(m.group(1)), (name, len(data), m.group(1))
        path = os.path.join(here, 'fonts', name.replace('u8g2_font_', '') + '.u8g2')
        with open(path, 'wb') as f:
            f.write(data + b'\0')
        print(path, len(data) + 1)


if __name__ == '__main__':
    main()
