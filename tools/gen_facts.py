#!/usr/bin/env python3
"""Genera dexfacts.h desde dex_facts.py (texto de la vista detalle de la Pokedex)."""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from dex_data import TYPE_ACCENTS  # noqa: E402
from dex_facts import FACTS  # noqa: E402

TYPE_NAMES = {
    'normal': 'NORMAL', 'fuego': 'FIRE', 'agua': 'WATER', 'planta': 'GRASS',
    'electrico': 'ELECTRIC', 'hielo': 'ICE', 'lucha': 'FIGHTING', 'veneno': 'POISON',
    'tierra': 'GROUND', 'psiquico': 'PSYCHIC', 'bicho': 'BUG', 'roca': 'ROCK',
    'fantasma': 'GHOST', 'dragon': 'DRAGON',
    'volador': 'FLYING',
}
MAX_LEN = 84


def main():
    for num in range(1, 152):
        text = FACTS[num]
        if len(text) > MAX_LEN or not text.isascii() or '"' in text or '\\' in text:
            sys.exit(f'dato {num} no vale ({len(text)} caracteres): {text}')
    out = ['#pragma once\n', '// GENERADO por tools/gen_facts.py desde tools/dex_facts.py - no editar\n\n']
    out.append('static const char *const TYPE_NAME[] = { '
               + ', '.join(f'"{TYPE_NAMES[t]}"' for t in TYPE_ACCENTS) + ' };\n\n')
    out.append('static const char *const DEX_FACT[152] = {\n  "",\n')
    out.extend(f'  "{FACTS[n]}",  // {n}\n' for n in range(1, 152))
    out.append('};\n')
    path = os.path.join(os.path.dirname(__file__), '..', 'dexfacts.h')
    with open(path, 'w', encoding='ascii') as f:
        f.write(''.join(out))
    print('guardado dexfacts.h (151 datos)')


if __name__ == '__main__':
    main()
