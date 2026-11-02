#!/usr/bin/env python3
"""A fake PokeKnomiGochi on this wifi, to test the buddy feature with a single board.

  python3 tools/fake_buddy.py                       says hello and accepts whatever it is asked
  python3 tools/fake_buddy.py --invite battle       also invites to battle
  python3 tools/fake_buddy.py --invite trade --give 25
  python3 tools/fake_buddy.py --decline             says no to everything
"""
import argparse
import random
import socket
import struct
import time

FMT = '<4sBBIIIBBBBhBBHHHH12s19s'
PORT = 47115
HELLO, INVITE, ACCEPT, DECLINE, FIGHTER, MOVE, OFFER, CONFIRM, LEAVE = range(1, 10)
NAMES = {1: 'HELLO', 2: 'INVITE', 3: 'ACCEPT', 4: 'DECLINE', 5: 'FIGHTER', 6: 'MOVE', 7: 'OFFER', 8: 'CONFIRM', 9: 'LEAVE'}


def pack(mtype, me, to=0, session=0, kind=0, ack=0, turn=0, move=0, dex=0, level=0, shiny=0,
         hp=0, atk=0, df=0, spe=0, nick=b'', reg=b''):
    return struct.pack(FMT, b'PKG1', 1, mtype, me, to, session, kind, ack, turn, move, dex, level, shiny,
                       hp, atk, df, spe, nick, reg)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dex', type=int, default=25)
    ap.add_argument('--level', type=int, default=30)
    ap.add_argument('--nick', default='FAKEBUDDY')
    ap.add_argument('--invite', choices=['battle', 'trade'])
    ap.add_argument('--give', type=int, default=25)
    ap.add_argument('--decline', action='store_true')
    ap.add_argument('--seconds', type=float, default=120)
    a = ap.parse_args()

    me = random.getrandbits(31) | 1
    reg = bytearray(19)
    for d in (a.dex, a.give, 1, 4, 7):
        reg[(d - 1) >> 3] |= 1 << ((d - 1) & 7)
    nick = a.nick.encode()[:11]

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(('', PORT))
    s.settimeout(0.2)

    peer, peer_ip, session = None, None, 0
    invited = False
    last_hello = last_invite = 0
    end = time.time() + a.seconds

    def send(data, ip=None):
        s.sendto(data, (ip or peer_ip or '255.255.255.255', PORT))

    def fighter(ack):
        send(pack(FIGHTER, me, peer, session, ack=ack, dex=a.dex, level=a.level, hp=80, atk=50, df=50, spe=50))

    while time.time() < end:
        now = time.time()
        if now - last_hello > 4:
            last_hello = now
            s.sendto(pack(HELLO, me, dex=a.dex, level=a.level, nick=nick, reg=bytes(reg)), ('255.255.255.255', PORT))
        if a.invite and peer and not invited and now - last_invite > 0.4:  # like the board: until there is a reply
            last_invite = now
            session = session or random.getrandbits(31) | 1
            send(pack(INVITE, me, peer, session, kind=1 if a.invite == 'battle' else 2))
            print('-> INVITE', a.invite, flush=True)
        try:
            data, addr = s.recvfrom(128)
        except socket.timeout:
            continue
        if len(data) != struct.calcsize(FMT):
            continue
        f = struct.unpack(FMT, data)
        mtype, sender, to, sess, kind, ack, turn, move, dex = f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10]
        if sender == me or (to and to != me):
            continue
        if mtype == HELLO:
            if peer != sender:
                peer, peer_ip = sender, addr[0]
                print(f'buddy {f[17].split(bytes(1))[0].decode()} #{dex} Lv.{f[11]} at {addr[0]}', flush=True)
            continue
        print(f'<- {NAMES.get(mtype, mtype)} turn={turn} move={move} dex={dex} ack={ack}', flush=True)
        if mtype in (ACCEPT, DECLINE):
            invited = True
        if mtype == INVITE:
            session = sess
            send(pack(DECLINE if a.decline else ACCEPT, me, peer, session))
            if not a.decline and kind == 1:
                fighter(0)
        elif mtype == ACCEPT and a.invite == 'battle':
            fighter(0)
        elif mtype == ACCEPT and a.invite == 'trade':
            send(pack(OFFER, me, peer, session, dex=a.give))
        elif mtype == FIGHTER and not ack:
            fighter(1)
        elif mtype == MOVE and not ack:
            send(pack(MOVE, me, peer, session, ack=1, turn=turn, move=0))
        elif mtype == OFFER and not ack:
            send(pack(OFFER, me, peer, session, ack=1, dex=a.give))
        elif mtype == CONFIRM and not ack:
            send(pack(CONFIRM, me, peer, session, ack=1, dex=a.give))
        elif mtype == LEAVE:
            print('session over', flush=True)


if __name__ == '__main__':
    main()
