#!/usr/bin/env python3
"""Knomi serial console. Opening the port resets the board, so a single session
runs all the steps in order:

  knomi_console.py "wait 40" "cmd STATS" "shot a.png" "cmd TAP 233 400" "wait 1" "shot b.png"
  knomi_console.py "tapuntil 120 365 battle: won" "shot fin.png"   taps until that text shows up
"""
import glob
import struct
import sys
import time
import serial
from PIL import Image


def open_port():
    s = serial.Serial()
    s.port = (glob.glob('/dev/cu.wchusbserial*') + glob.glob('/dev/cu.usbserial*'))[0]
    s.baudrate = 115200
    s.timeout = 0.2
    s.dtr = False
    s.rts = False
    s.open()
    return s


def log_for(s, secs):
    end = time.time() + secs
    while time.time() < end:
        line = s.readline()
        if line:
            print(line.decode('utf8', 'replace').rstrip(), flush=True)


def cmd(s, text):
    s.write(text.encode() + b'\n')
    end = time.time() + 15
    while time.time() < end:
        line = s.readline().decode('utf8', 'replace').rstrip()
        if line:
            print(line, flush=True)
        if line == 'DONE':
            return
    print('(no DONE)', text)


def shot(s, path):
    s.write(b'SHOT\n')
    end = time.time() + 5
    while time.time() < end:
        line = s.readline()
        if line.startswith(b'SHOT '):
            break
    else:
        print('screenshot: no response')
        return
    parts = line.split()
    w, h = int(parts[1]), int(parts[2])
    rot = int(parts[3]) if len(parts) > 3 else 0
    need = w * h * 2
    s.timeout = 15
    raw = s.read(need)
    s.timeout = 0.2
    s.read_until(b'DONE', 8)  # end of the screenshot: so the next command does not read it
    if len(raw) != need:
        print(f'captura corta: {len(raw)} de {need}')
        return
    img = Image.new('RGB', (w, h))
    img.putdata([((c >> 8) & 0xF8, (c >> 3) & 0xFC, (c << 3) & 0xF8) for c in struct.unpack(f'<{w * h}H', raw)])
    # the panel rotates with the screen: the screenshot is saved as seen in the hand
    img.rotate(90 * rot).resize((w * 2, h * 2), Image.NEAREST).save(path)
    print('ok', path, flush=True)


def tap_until(s, x, y, text):
    end = time.time() + 120
    last = 0
    while time.time() < end:
        if time.time() - last > 1:
            s.write(f'TAP {x} {y}\n'.encode())
            last = time.time()
        line = s.readline().decode('utf8', 'replace').rstrip()
        if line and line != 'DONE':
            print(line, flush=True)
        if text in line:
            return
    print('(did not appear)', text)


def main():
    s = open_port()
    for step in sys.argv[1:]:
        verb, _, arg = step.partition(' ')
        if verb == 'wait':
            log_for(s, float(arg))
        elif verb == 'cmd':
            cmd(s, arg)
        elif verb == 'shot':
            shot(s, arg)
        elif verb == 'tapuntil':
            x, y, text = arg.split(' ', 2)
            tap_until(s, x, y, text)


if __name__ == '__main__':
    main()
