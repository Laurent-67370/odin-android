#!/usr/bin/env python3
"""What the games' sounds.py and music.py share: an envelope, waves and drums
for sounds.py (22050 Hz, 16 bits), the note lines of music.py."""
import math, random, re

RATE = 22050
WAVE_LEN = 64   # a cycle of an instrument: 22050 / 64 = 344.5 Hz at its own speed


def env(n, attack=0.002, decay=1.0):
    a = max(1, int(attack * RATE))
    return [min(1.0, i / a) * math.exp(-decay * i / n * 5.0) for i in range(n)]


def square(f, i):
    return 1.0 if math.sin(2 * math.pi * f * i / RATE) >= 0 else -1.0


def clip(xs, volume):
    return [max(-32767, min(32767, int(x * volume * 32767))) for x in xs]


def notes(freqs, note_s, decay=1.0, volume=0.3, shape='square'):
    out = []
    for f in freqs:
        n = int(note_s * RATE)
        for i, e in enumerate(env(n, decay=decay)):
            if f <= 0:
                out.append(0.0)
                continue
            s = square(f, i) if shape == 'square' else math.sin(2 * math.pi * f * i / RATE)
            out.append(s * e)
    return clip(out, volume)


def sweep(f0, f1, seconds, volume, decay=1.0, shape='sine'):
    n = int(seconds * RATE)
    out, phase = [], 0.0
    for i, e in enumerate(env(n, decay=decay)):
        phase += (f0 + (f1 - f0) * i / n) / RATE
        x = math.sin(2 * math.pi * phase) if shape == 'sine' else (1.0 if (phase % 1) < 0.5 else -1.0)
        out.append(x * e)
    return clip(out, volume)


def noise(seconds, colour, decay, volume, rumble=0.0, seed=1):
    rng = random.Random(seed)
    n = int(seconds * RATE)
    out, level = [], 0.0
    for i, e in enumerate(env(n, decay=decay)):
        level += (rng.uniform(-1, 1) - level) * colour
        out.append((level * 2.2 + rumble * math.sin(2 * math.pi * 55 * i / RATE)) * e)
    return clip(out, volume)


def wave(shape):
    out = []
    for i in range(WAVE_LEN):
        p = i / WAVE_LEN
        if shape == 'pulse':
            x = (1.0 if p < 0.5 else -1.0) * 0.9 + 0.1 * math.sin(2 * math.pi * p)
        elif shape == 'pulse25':
            x = 1.0 if p < 0.25 else -1.0
        elif shape == 'saw':
            x = 0.85 * (2 * p - 1) + 0.15 * math.sin(2 * math.pi * p)
        elif shape == 'tri':
            x = 4 * p - 1 if p < 0.5 else 3 - 4 * p
        else:
            x = math.sin(2 * math.pi * p)
        out.append(x)
    return clip(out, 0.5)


def kick():
    n = int(0.16 * RATE)
    out, phase = [], 0.0
    for i, e in enumerate(env(n, attack=0.001, decay=1.3)):
        phase += (40 + 110 * math.exp(-i / (0.03 * RATE))) / RATE
        out.append(math.sin(2 * math.pi * phase) * e)
    return clip(out, 0.75)


def hat():
    rng = random.Random(7)
    n = int(0.05 * RATE)
    out, prev = [], 0.0
    for i, e in enumerate(env(n, attack=0.0005, decay=2.0)):
        x = rng.uniform(-1, 1)
        out.append((x - prev) * e)
        prev = x
    return clip(out, 0.35)


def snare():
    rng = random.Random(9)
    n = int(0.14 * RATE)
    return clip([(rng.uniform(-1, 1) * 0.8 + 0.4 * math.sin(2 * math.pi * 190 * i / RATE)) * e
                 for i, e in enumerate(env(n, attack=0.001, decay=1.6))], 0.5)


# ---- music: lines of notes -------------------------------------------------

WAVE_BASE = RATE / WAVE_LEN
NAMES = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
DRUMS = {'K': 'kick', 'H': 'hat', 'S': 'snare'}
# AICA envelopes: (AR, D1R, DL, D2R, RR), rates 0-31 (31 the fastest)
ENVELOPES = {
    'bass': (31, 9, 8, 5, 16),
    'lead': (29, 7, 4, 3, 14),
    'arp': (31, 13, 12, 8, 18),
    'pad': (22, 4, 2, 1, 10),
    'pluck': (31, 15, 14, 10, 20),
    'drum': (31, 0, 0, 0, 31),
}


def frequency(note):
    m = re.fullmatch(r'([A-G])([#b]?)(-?\d)', note)
    if not m:
        raise ValueError('not a note: %s' % note)
    semis = NAMES[m.group(1)] + {'#': 1, 'b': -1, '': 0}[m.group(2)]
    return 440.0 * 2 ** ((12 * (int(m.group(3)) + 1) + semis - 69) / 12)


def channel(text, instrument, level, pan=0, envelope='lead'):
    out = []
    for token in text.split():
        name, ticks = token.split(':')
        if name == '-':
            out.append((0, int(ticks), ''))
        elif name in DRUMS:
            out.append((1024, int(ticks), DRUMS[name]))
        else:
            out.append((round(frequency(name) / WAVE_BASE * 1024), int(ticks), instrument))
    return {'notes': out, 'level': level, 'pan': pan, 'envelope': ENVELOPES[envelope]}


def rep(text, n):
    return ' '.join([text] * n)


def checked(tracks):
    for name, t in tracks:
        lengths = {sum(n[1] for n in c['notes']) for c in t['channels']}
        if len(lengths) != 1:
            raise ValueError('the channels of %s do not last as long: %s' % (name, sorted(lengths)))
    return tracks
