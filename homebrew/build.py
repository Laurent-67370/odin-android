#!/usr/bin/env python3
"""Builds the homebrew for the SEGA Hikaru: odinhb.zip (two EPROMs) and
odinhb.json, its entry for Odin's list of games.

    python3 build.py [--games path/to/odin-games.json]

Needs sh4-linux-gnu-gcc (Debian/Ubuntu: gcc-sh4-linux-gnu).

The ROM, as the BOOTROM reads it (see README.md):
  0x000  "SAMURAI\\0"
  0x13C  20 entries of 10 bytes: the ICs (log2 of the size of a pair in MB;
         FFFF: none)
  0x390  the load table: (bus address, RAM address, length) by 3 words,
         ended by one whose first word is not positive, its second the
         entry point
"""
import argparse, json, os, struct, subprocess, zipfile, zlib

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'build')
NAME, TITLE = 'odinhb', 'Odin Homebrew'
LOAD = 0x8C100000          # where the program runs (link.ld)
PROG_AT = 0x1000           # where it is in the EPROMs
CHIP = 2 << 20             # two 2 MB EPROMs, 16 bits each in turn
SOURCES = ['crt0.S', 'hikaru.c', 'gfx.c', 'main.c']


def font_header(gen):
    """tools/font5x7.txt as font5x7.h: 7 rows of 5 bits a character"""
    glyphs, lines = {}, open(os.path.join(HERE, 'tools', 'font5x7.txt')).read().split('\n')
    lines = [l for l in lines if not l.startswith('# ')]
    i = 0
    while i + 7 < len(lines):
        if len(lines[i]) != 1:
            i += 1
            continue
        rows = lines[i + 1:i + 8]
        glyphs[lines[i]] = [int(r.replace('#', '1').replace('.', '0'), 2) for r in rows]
        i += 8
    first, last = ord(' '), ord('_')
    out = ['/* Made by build.py from tools/font5x7.txt */', '#include <stdint.h>',
           '#define FONT_FIRST %d' % first, '#define FONT_LAST %d' % last,
           'static const uint8_t font5x7[%d][7] = {' % (last - first + 1)]
    for c in range(first, last + 1):
        rows = glyphs.get(chr(c), glyphs['?'])
        out.append('\t{ %s },\t/* %r */' % (', '.join('0x%02X' % r for r in rows), chr(c)))
    out.append('};')
    open(os.path.join(gen, 'font5x7.h'), 'w').write('\n'.join(out) + '\n')


def sprites_source(gen):
    """assets/*.png as sprites.h and sprites.c: ABGR1555, 0 where transparent"""
    from PIL import Image
    names = sorted(f[:-4] for f in os.listdir(os.path.join(HERE, 'assets')) if f.endswith('.png'))
    h = ['/* Made by build.py from assets/ */', '#include "gfx.h"']
    c = ['/* Made by build.py from assets/ */', '#include "sprites.h"']
    for n in names:
        im = Image.open(os.path.join(HERE, 'assets', n + '.png')).convert('RGBA')
        px = []
        raw = im.tobytes()
        for i in range(0, len(raw), 4):
            r, g, b, a = raw[i:i + 4]
            px.append(0 if a < 128 else 0x8000 | (b >> 3) << 10 | (g >> 3) << 5 | (r >> 3))
        h.append('extern const sprite_t spr_%s;' % n)
        c.append('static const uint16_t %s_pixels[] = {' % n)
        for i in range(0, len(px), 16):
            c.append('\t' + ', '.join('0x%04X' % p for p in px[i:i + 16]) + ',')
        c.append('};')
        c.append('const sprite_t spr_%s = { %d, %d, %s_pixels };' % (n, im.width, im.height, n))
    open(os.path.join(gen, 'sprites.h'), 'w').write('\n'.join(h) + '\n')
    open(os.path.join(gen, 'sprites.c'), 'w').write('\n'.join(c) + '\n')


def compile_program():
    os.makedirs(OUT, exist_ok=True)
    gen = os.path.join(OUT, 'gen')
    os.makedirs(gen, exist_ok=True)
    font_header(gen)
    sprites_source(gen)
    elf, binary = os.path.join(OUT, NAME + '.elf'), os.path.join(OUT, NAME + '.bin')
    subprocess.check_call(['sh4-linux-gnu-gcc', '-m4', '-ml', '-O2', '-static', '-fno-pic', '-nostdlib',
                           '-ffreestanding', '-fno-builtin', '-Wall', '-Wextra',
                           '-Wl,--build-id=none', '-Wl,-z,noexecstack', '-Wl,--no-warn-rwx-segments',
                           '-I', os.path.join(HERE, 'src'), '-I', gen,
                           '-T', os.path.join(HERE, 'src', 'link.ld')] +
                          [os.path.join(HERE, 'src', s) for s in SOURCES] +
                          [os.path.join(gen, 'sprites.c'), '-o', elf, '-lgcc'])
    subprocess.check_call(['sh4-linux-gnu-objcopy', '-O', 'binary', elf, binary])
    return open(binary, 'rb').read()


def rom_of(program):
    rom = bytearray(b'\xff' * (2 * CHIP))
    rom[0:8] = b'SAMURAI\0'
    rom[0x10:0x10 + len(TITLE)] = TITLE.encode()
    for i in range(20):
        entry = (2, 0, 0, 0, 0) if i == 0 else (0xFFFF, 0, 0, 0, 0)   # one pair of 4 MB
        rom[0x13C + i * 10:0x146 + i * 10] = struct.pack('>5H', *entry)
    table = struct.pack('<3I', 0x10000000 + PROG_AT, LOAD, len(program))
    table += struct.pack('<3I', 0xFFFFFFFF, LOAD, 0)
    rom[0x390:0x390 + len(table)] = table
    if PROG_AT + len(program) > len(rom):
        raise SystemExit('the program does not fit the EPROMs')
    rom[PROG_AT:PROG_AT + len(program)] = program
    chips = (bytearray(), bytearray())
    for i in range(0, len(rom), 4):
        chips[0].extend(rom[i:i + 2])
        chips[1].extend(rom[i + 2:i + 4])
    return chips


def entry_of(chips, names):
    def control(name, cid, switch, role, pad=None):
        c = {'name': name, 'id': cid, 'switch': switch, 'role': role}
        if pad:
            c['pad'] = pad
        return c
    sets = lambda n, s, c: {'name': n, 'size': s, 'crc': c}
    alt = lambda n, files, optional=False: dict({'name': n, 'type': 'alternative', 'data': files},
                                                **({'optional': True} if optional else {}))
    return {
        'name': NAME, 'title': TITLE, 'mach': 'hikaru',
        'rom_board': {'eprom_mb': CHIP >> 20},
        'controls': [
            control('START', 'start', '1P START', 'start'),
            control('UP', 'up', '1P UP', 'up', ['dpup', '-lefty']),
            control('DOWN', 'down', '1P DOWN', 'down', ['dpdown', '+lefty']),
            control('LEFT', 'left', '1P LEFT', 'left', ['dpleft', '-leftx']),
            control('RIGHT', 'right', '1P RIGHT', 'right', ['dpright', '+leftx']),
            control('BUTTON 1', 'button1', '1P PUSH1', 'button1'),
            control('BUTTON 2', 'button2', '1P PUSH2', 'button2'),
            control('BUTTON 3', 'button3', '1P PUSH3', 'button3'),
        ],
        'sections': [
            alt('mie', [dict(sets('315-6146.bin', 2048, '9b197e35'), set='mie')]),
            alt('io_rom', [dict(sets('315-6215.bin', 16384, '98202738'), set='jvs13551')], True),
            alt('eeprom', [dict(sets('93c46.ic115', 128, '737dc714'), set='hikaru')]),
            alt('rombd_eeprom', [dict(sets('x76f100.ic85', 132, '194c6c18'), set='hikaru')]),
            alt('bootrom', [dict(sets('hikaru_0.96.bin', 2097152, '2aa906a7'), set='hikaru')]),
            {'name': 'eprom', 'type': 'interleave', 'amnt': 2,
             'data': [sets(n, len(c), '%08x' % zlib.crc32(c)) for n, c in zip(names, chips)]},
        ],
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--games', help="add the entry to this odin-games.json")
    args = parser.parse_args()

    program = compile_program()
    chips = rom_of(program)
    names = (NAME + '.ic29', NAME + '.ic30')
    with zipfile.ZipFile(os.path.join(OUT, NAME + '.zip'), 'w', zipfile.ZIP_DEFLATED) as z:
        for n, c in zip(names, chips):
            z.writestr(n, bytes(c))
    entry = entry_of(chips, names)
    json.dump(entry, open(os.path.join(OUT, NAME + '.json'), 'w'), indent=1)
    if args.games:
        games = json.load(open(args.games))
        games['roms'] = [g for g in games['roms'] if g['name'] != NAME] + [entry]
        json.dump(games, open(args.games, 'w'), indent=1)
    print('%s: program %d bytes, build/%s.zip' % (NAME, len(program), NAME))


if __name__ == '__main__':
    main()
