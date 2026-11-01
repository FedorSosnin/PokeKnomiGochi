#!/usr/bin/env python3
"""Genera moves.h (ataques de cada especie para el firmware) desde dex_moves.py.

  python3 tools/gen_moves.py

El combate no tiene fallos, ni turnos de carga, ni golpes multiples, asi que cada
ataque guarda una potencia "efectiva" que resume esas reglas de gen 1.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from dex_data import DEX
from dex_moves import MOVES, LEARNSETS

# ortografia de los juegos de Game Boy (12 letras como mucho)
GEN1_NAMES = {
    'Thunder Shock': 'THUNDERSHOCK', 'Thunder Punch': 'THUNDERPUNCH', 'High Jump Kick': 'HI JUMP KICK',
    'Double Slap': 'DOUBLESLAP', 'Vise Grip': 'VICEGRIP', 'Solar Beam': 'SOLARBEAM',
    'Bubble Beam': 'BUBBLEBEAM',
}

# sin dano fijo ni KO directo: no encajan en la formula, pero se acercan a un golpe normal
FIXED_DAMAGE = {'Dragon Rage': 60, 'Night Shade': 60, 'Seismic Toss': 60, 'Super Fang': 60, 'Sonic Boom': 40}
# golpean de 2 a 5 veces (o atrapan varios turnos): se cuentan como 3 golpes
MULTI_HIT = {'Barrage', 'Comet Punch', 'Double Slap', 'Fury Attack', 'Fury Swipes', 'Pin Missile', 'Spike Cannon',
             'Bind', 'Wrap', 'Clamp', 'Fire Spin'}
DOUBLE_HIT = {'Double Kick', 'Bonemerang', 'Twineedle'}
# gastan un turno cargando o recuperandose
TWO_TURNS = {'Solar Beam', 'Razor Wind', 'Skull Bash', 'Sky Attack', 'Dig', 'Fly', 'Hyper Beam'}
# piden reglas que este combate no tiene: KO directo, debilitarse, dormir al rival...
EXCLUDED = {'Guillotine', 'Horn Drill', 'Fissure', 'Self-Destruct', 'Explosion', 'Dream Eater', 'Counter', 'Bide',
            'Psywave'}


def effective_power(name, power, acc):
    if name in EXCLUDED:
        return 0
    if name in FIXED_DAMAGE:
        return FIXED_DAMAGE[name]
    p = power * (3 if name in MULTI_HIT else 2 if name in DOUBLE_HIT else 1) * acc / 100
    return round(p * 0.6 if name in TWO_TURNS else p)


def full_learnsets():
    """Lo aprendido antes de evolucionar se conserva, como en el juego real."""
    evo_level = {num: lvl for num, _, _, _, _, lvl in DEX}
    parent = {evo: num for num, _, _, _, evo, _ in DEX if evo}
    for branch in (134, 135, 136):  # Eevee se ramifica en el codigo del juego
        parent[branch] = 133

    def learned(num):
        out = {}
        if num in parent:
            for lvl, mid in learned(parent[num]):
                if lvl <= evo_level[parent[num]]:
                    out[mid] = lvl
        for lvl, mid in LEARNSETS[num]:
            if effective_power(MOVES[mid][0], MOVES[mid][2], MOVES[mid][3]) > 0:
                out[mid] = min(lvl, out.get(mid, lvl))
        return sorted((lvl, mid) for mid, lvl in out.items())

    return {num: learned(num) for num, *_ in DEX}


def main():
    sets = full_learnsets()
    used = sorted({mid for s in sets.values() for _, mid in s})
    ids = {mid: i + 1 for i, mid in enumerate(used)}  # 0 = STRUGGLE
    out = [
        '#pragma once\n#include <stdint.h>\n#include "dex.h"\n\n',
        '// GENERADO por tools/gen_moves.py desde tools/dex_moves.py - no editar\n\n',
        'struct BtlMove { const char *name; uint8_t type; uint8_t power; };\n',
        'struct LearnEntry { uint8_t level; uint8_t move; };\n\n',
        '// 0: STRUGGLE, lo que usa en gen 1 quien no tiene ningun ataque\n',
        'static const BtlMove MOVE_TBL[] = {\n  { "STRUGGLE", TY_NORMAL, 50 },\n',
    ]
    for mid in used:
        name, typ, power, acc = MOVES[mid]
        shown = GEN1_NAMES.get(name, name.upper())
        out.append(f'  {{ "{shown}", TY_{typ.upper()}, {effective_power(name, power, acc)} }},\n')
    out.append('};\n\n// ataques por especie, en orden de nivel: desde LEARN_START[dex] hasta LEARN_START[dex + 1]\n')
    out.append('static const LearnEntry LEARN_TBL[] = {\n')
    start, n = [0, 0], 0
    for num in range(1, 152):
        out.append('  ' + ''.join(f'{{ {lvl}, {ids[mid]} }}, ' for lvl, mid in sets[num]) + f'// {num}\n')
        n += len(sets[num])
        start.append(n)
    out.append('};\n\nstatic const uint16_t LEARN_START[DEX_COUNT + 2] = {\n  ')
    out.append(', '.join(str(x) for x in start) + '\n};\n')
    path = os.path.join(os.path.dirname(__file__), '..', 'moves.h')
    with open(path, 'w') as f:
        f.write(''.join(out))
    print(f'moves.h: {len(used)} ataques, {n} entradas')


if __name__ == '__main__':
    main()
