#!/usr/bin/env python3
"""Builds the homebrew games for the SEGA Hikaru: for each folder with a
game.json (its MAME-style set name and its title), build/<name>.zip (two
EPROMs) and build/<name>.json, its entry for Odin's list of games.

    python3 build.py [--game FOLDER] [--games path/to/odin-games.json]

A game folder holds main.c, assets/*.png (its sprites), sounds.py and
music.py; src/ is the library they share, tools/font5x7.txt their font.

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
LOAD = 0x8C100000          # where the program runs (link.ld)
PROG_AT = 0x1000           # where it is in the EPROMs
CHIP = 2 << 20             # two 2 MB EPROMs, 16 bits each in turn
LIBRARY = ['crt0.S', 'hikaru.c', 'gfx.c', 'sound.c', 'music.c']


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


def sprites_source(game, gen):
    """assets/*.png as sprites.h and sprites.c: ABGR1555, 0 where transparent"""
    from PIL import Image
    names = sorted(f[:-4] for f in os.listdir(os.path.join(game, 'assets')) if f.endswith('.png'))
    h = ['/* Made by build.py from assets/ */', '#include "gfx.h"']
    c = ['/* Made by build.py from assets/ */', '#include "sprites.h"']
    for n in names:
        im = Image.open(os.path.join(game, 'assets', n + '.png')).convert('RGBA')
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


def sounds_source(game, gen):
    """sounds.py as sounds.h and sounds.c: 16-bit samples at 22050 Hz"""
    import importlib.util
    spec = importlib.util.spec_from_file_location('sounds', os.path.join(game, 'sounds.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    table = module.generate()
    h = ['/* Made by build.py from tools/sounds.py */', '#ifndef SOUNDS_H', '#define SOUNDS_H', '#include <stdint.h>',
         'typedef struct { const int16_t *data; unsigned length; int loop_start; } sound_t;',
         'enum {']
    h += ['\tSND_%s,' % name.upper() for name, _, _ in table]
    h += ['\tSOUND_COUNT', '};', 'extern const sound_t sounds[SOUND_COUNT];', '#endif']
    c = ['/* Made by build.py from tools/sounds.py */', '#include "sounds.h"']
    for name, samples, _ in table:
        if len(samples) % 2:
            samples = samples + [0]
        c.append('static const int16_t %s_data[] = {' % name)
        for i in range(0, len(samples), 16):
            c.append('\t' + ', '.join(str(v) for v in samples[i:i + 16]) + ',')
        c.append('};')
    c.append('const sound_t sounds[SOUND_COUNT] = {')
    for name, samples, loop in table:
        length = len(samples) + (len(samples) % 2)
        c.append('\t{ %s_data, %d, %d },' % (name, length, -1 if loop is None else loop))
    c.append('};')
    open(os.path.join(gen, 'sounds.h'), 'w').write('\n'.join(h) + '\n')
    open(os.path.join(gen, 'sounds.c'), 'w').write('\n'.join(c) + '\n')


def music_source(game, gen):
    """music.py as music_data.h and music_data.c"""
    import importlib.util
    spec = importlib.util.spec_from_file_location('music', os.path.join(game, 'music.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    tracks = module.generate()
    h = ['/* Made by build.py from tools/music.py */', '#ifndef MUSIC_DATA_H', '#define MUSIC_DATA_H',
         '#include <stdint.h>',
         '/* A note: its pitch in 1024ths of its sound (0: a rest), its length in ticks, its sound */',
         'typedef struct { uint16_t pitch; uint8_t ticks; uint8_t sound; } mnote_t;',
         '/* A channel: its notes, its level (0-15) and pan, its envelope (AICA registers 10 and 14) */',
         'typedef struct { const mnote_t *notes; uint16_t count; uint8_t level; int8_t pan;'
         ' uint16_t rates, decay; } mchannel_t;',
         'typedef struct { uint8_t tick_frames; uint8_t count; const mchannel_t *channels; } mtrack_t;',
         'enum {']
    h += ['\tMUSIC_%s,' % name.upper() for name, _ in tracks]
    h += ['\tMUSIC_COUNT', '};', 'extern const mtrack_t music_tracks[MUSIC_COUNT];', '#endif']
    c = ['/* Made by build.py from tools/music.py */', '#include "music_data.h"', '#include "sounds.h"']
    for name, track in tracks:
        for n, ch in enumerate(track['channels']):
            c.append('static const mnote_t %s_%d[] = {' % (name, n))
            for pitch, ticks, sound in ch['notes']:
                c.append('\t{ %d, %d, %s },' % (pitch, ticks, 'SND_' + sound.upper() if sound else '0'))
            c.append('};')
        c.append('static const mchannel_t %s_channels[] = {' % name)
        for n, ch in enumerate(track['channels']):
            ar, d1r, dl, d2r, rr = ch['envelope']
            rates = d2r << 11 | d1r << 6 | ar
            decay = 0xF << 10 | dl << 5 | rr          # KRS off
            c.append('\t{ %s_%d, %d, %d, %d, 0x%04X, 0x%04X },' % (name, n, len(ch['notes']), ch['level'], ch['pan'],
                                                                  rates, decay))
        c.append('};')
    c.append('const mtrack_t music_tracks[MUSIC_COUNT] = {')
    for name, track in tracks:
        c.append('\t{ %d, %d, %s_channels },' % (track['tick_frames'], len(track['channels']), name))
    c.append('};')
    open(os.path.join(gen, 'music_data.h'), 'w').write('\n'.join(h) + '\n')
    open(os.path.join(gen, 'music_data.c'), 'w').write('\n'.join(c) + '\n')


def compile_program(game, name, flags=()):
    gen = os.path.join(OUT, name, 'gen')
    os.makedirs(gen, exist_ok=True)
    font_header(gen)
    sprites_source(game, gen)
    sounds_source(game, gen)
    music_source(game, gen)
    elf, binary = os.path.join(OUT, name, name + '.elf'), os.path.join(OUT, name, name + '.bin')
    subprocess.check_call(['sh4-linux-gnu-gcc', '-m4', '-ml', '-O2', '-static', '-fno-pic', '-nostdlib',
                           '-ffreestanding', '-fno-builtin', '-Wall', '-Wextra',
                           '-Wl,--build-id=none', '-Wl,-z,noexecstack', '-Wl,--no-warn-rwx-segments',
                           '-I', os.path.join(HERE, 'src'), '-I', gen, *flags,
                           '-T', os.path.join(HERE, 'src', 'link.ld')] +
                          [os.path.join(HERE, 'src', s) for s in LIBRARY] + [os.path.join(game, 'main.c')] +
                          [os.path.join(gen, 'sprites.c'), os.path.join(gen, 'sounds.c'),
                           os.path.join(gen, 'music_data.c'), '-o', elf, '-lgcc'])
    subprocess.check_call(['sh4-linux-gnu-objcopy', '-O', 'binary', elf, binary])
    return open(binary, 'rb').read()


def rom_of(program, title):
    rom = bytearray(b'\xff' * (2 * CHIP))
    rom[0:8] = b'SAMURAI\0'
    rom[0x10:0x10 + len(title)] = title.encode()
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


def entry_of(chips, names, name, title, aim=False):
    def control(name, cid, switch, role, pad=None):
        c = {'name': name, 'id': cid, 'switch': switch, 'role': role}
        if pad:
            c['pad'] = pad
        return c
    sets = lambda n, s, c: {'name': n, 'size': s, 'crc': c}
    alt = lambda n, files, optional=False: dict({'name': n, 'type': 'alternative', 'data': files},
                                                **({'optional': True} if optional else {}))
    return {
        'name': name, 'title': title, 'mach': 'hikaru',
        # a game of this folder: an import may put a newer one in its place
        'homebrew': True,
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
        ] + ([
            # A sight, as Brave Firefighters' nozzle: on the phone, the finger
            {'name': 'X AXIS', 'id': 'sight_x', 'analog': 0, 'low': 'left', 'role': 'gun_x'},
            {'name': 'Y AXIS', 'id': 'sight_y', 'analog': 1, 'low': 'up', 'role': 'gun_y'},
        ] if aim else []),
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


# Where the zips are published, with the APK
RELEASE = 'https://github.com/Laurent-67370/odin-android/releases/download/odin-apk/'


def build(game, flags, games_json):
    about = json.load(open(os.path.join(game, 'game.json')))
    name, title = about['name'], about['title']
    program = compile_program(game, name, flags)
    chips = rom_of(program, title)
    names = (name + '.ic29', name + '.ic30')
    with zipfile.ZipFile(os.path.join(OUT, name + '.zip'), 'w', zipfile.ZIP_DEFLATED) as z:
        for n, c in zip(names, chips):
            z.writestr(n, bytes(c))
    entry = entry_of(chips, names, name, title, about.get('aim', False))
    # Its description in its archive too: importing it adds the game to the
    # list, without a new APK
    with zipfile.ZipFile(os.path.join(OUT, name + '.zip'), 'a', zipfile.ZIP_DEFLATED) as z:
        z.writestr('odin-game.json', json.dumps(entry, indent=1))
    json.dump(entry, open(os.path.join(OUT, name + '.json'), 'w'), indent=1)
    if games_json:
        games = json.load(open(games_json))
        games['roms'] = [g for g in games['roms'] if g['name'] != name] + [entry]
        json.dump(games, open(games_json, 'w'), indent=1)
    print('%s (%s): program %d bytes, build/%s.zip' % (name, title, len(program), name))
    return {'name': name, 'title': title, 'genre': about.get('genre', ''), 'crc': '%08x' % zlib.crc32(chips[0]),
            'url': RELEASE + name + '.zip'}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--game', action='append', help="the folder of a game to build (default: every game)")
    parser.add_argument('--games', help="add the entries to this odin-games.json")
    parser.add_argument('--start-wave', type=int, help="for the tests: a game starts at this wave (or level)")
    parser.add_argument('--define', action='append', default=[], help="for the tests: -D for the compiler")
    args = parser.parse_args()

    folders = args.game or sorted(d for d in os.listdir(HERE) if os.path.isfile(os.path.join(HERE, d, 'game.json')))
    flags = (['-DSTART_WAVE=%d' % args.start_wave] if args.start_wave else []) + ['-D' + d for d in args.define]
    os.makedirs(OUT, exist_ok=True)
    built = [build(os.path.join(HERE, folder), flags, args.games) for folder in folders]
    # The list Odin's "Get homebrew games" reads (OdinActivity), published
    # with the zips: written when every game is built
    if not args.game:
        json.dump({'games': built}, open(os.path.join(OUT, 'homebrew.json'), 'w'), indent=1)


if __name__ == '__main__':
    main()
