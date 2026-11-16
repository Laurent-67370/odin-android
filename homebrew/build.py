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
SOURCES = ['crt0.S', 'hikaru.c', 'main.c']


def compile_program():
    os.makedirs(OUT, exist_ok=True)
    elf, binary = os.path.join(OUT, NAME + '.elf'), os.path.join(OUT, NAME + '.bin')
    subprocess.check_call(['sh4-linux-gnu-gcc', '-m4', '-ml', '-O2', '-static', '-fno-pic', '-nostdlib',
                           '-ffreestanding', '-fno-builtin', '-Wall', '-Wextra',
                           '-Wl,--build-id=none', '-Wl,-z,noexecstack', '-Wl,--no-warn-rwx-segments',
                           '-T', os.path.join(HERE, 'src', 'link.ld')] +
                          [os.path.join(HERE, 'src', s) for s in SOURCES] + ['-o', elf, '-lgcc'])
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
