#!/usr/bin/env python3
"""The sounds of NOVA SIEGE, synthesised here: 16-bit samples at 22050 Hz,
played by the AICA at OCT -1 (src/sound.c). No recording of anything.

generate() gives [(name, samples, loop_start or None)]."""
import math, random

RATE = 22050


def _env(n, attack=0.002, decay=1.0):
    """an envelope over n samples: a short attack, then an exponential fall"""
    a = max(1, int(attack * RATE))
    out = []
    for i in range(n):
        out.append(min(1.0, i / a) * math.exp(-decay * i / n * 5.0))
    return out


def _square(f, i):
    return 1.0 if math.sin(2 * math.pi * f * i / RATE) >= 0 else -1.0


def _clip(xs, volume):
    return [max(-32767, min(32767, int(x * volume * 32767))) for x in xs]


def shoot():
    n = int(0.11 * RATE)
    out, phase = [], 0.0
    for i, e in enumerate(_env(n, decay=0.8)):
        f = 1500 - 1150 * i / n
        phase += f / RATE
        out.append((1.0 if (phase % 1.0) < 0.5 else -1.0) * e)
    return _clip(out, 0.32)


def noise_burst(seconds, colour, decay, rumble=0.0, seed=1):
    rng = random.Random(seed)
    n = int(seconds * RATE)
    out, level = [], 0.0
    for i, e in enumerate(_env(n, decay=decay)):
        level += (rng.uniform(-1, 1) - level) * colour   # a one-pole low-pass: lower colour, duller
        x = level * 2.2 + rumble * math.sin(2 * math.pi * 55 * i / RATE)
        out.append(x * e)
    return out


def boom():
    return _clip(noise_burst(0.24, 0.45, 1.2, seed=2), 0.55)


def blast():
    return _clip(noise_burst(0.95, 0.12, 0.9, rumble=0.5, seed=3), 0.75)


def step():
    """a step of the formation: a low thump; its four notes are pitches"""
    n = int(0.09 * RATE)
    out = []
    for i, e in enumerate(_env(n, decay=1.4)):
        out.append((0.7 * _square(98, i) + 0.3 * math.sin(2 * math.pi * 49 * i / RATE)) * e)
    return _clip(out, 0.45)


def siren():
    """the raider: a warble, looped while it crosses"""
    n = int(0.25 * RATE)
    out, phase = [], 0.0
    for i in range(n):
        f = 700 + 220 * math.sin(2 * math.pi * 4 * i / n)
        phase += f / RATE
        out.append(math.sin(2 * math.pi * phase) * 0.8 + (0.2 if (phase % 1) < 0.5 else -0.2))
    return _clip(out, 0.22)


def notes(freqs, note_s, decay=1.0, volume=0.3, shape='square'):
    out = []
    for f in freqs:
        n = int(note_s * RATE)
        for i, e in enumerate(_env(n, decay=decay)):
            if f <= 0:
                out.append(0.0)  # a rest
                continue
            s = _square(f, i) if shape == 'square' else math.sin(2 * math.pi * f * i / RATE)
            out.append(s * e)
    return _clip(out, volume)


def clang():
    """the boss hit: two inharmonic tones, fast decay"""
    n = int(0.16 * RATE)
    out = []
    for i, e in enumerate(_env(n, decay=1.8)):
        out.append((math.sin(2 * math.pi * 523 * i / RATE) + 0.7 * math.sin(2 * math.pi * 1381 * i / RATE)) * e)
    return _clip(out, 0.3)


def swoop():
    """a diver leaving the formation: a falling whistle"""
    n = int(0.35 * RATE)
    out, phase = [], 0.0
    for i, e in enumerate(_env(n, attack=0.02, decay=0.5)):
        f = 1800 - 1300 * (i / n) ** 0.7
        phase += f / RATE
        out.append(math.sin(2 * math.pi * phase) * e)
    return _clip(out, 0.25)


WAVE_LEN = 64   # a cycle of an instrument: 22050 / 64 = 344.5 Hz at its own speed


def wave(shape):
    """one cycle, looped by the AICA: the instruments of the music"""
    out = []
    for i in range(WAVE_LEN):
        p = i / WAVE_LEN
        if shape == 'pulse':          # a square, its edges a little soft
            x = 1.0 if p < 0.5 else -1.0
            x = x * 0.9 + 0.1 * math.sin(2 * math.pi * p)
        elif shape == 'saw':
            x = 2 * p - 1
            x = 0.85 * x + 0.15 * math.sin(2 * math.pi * p)
        elif shape == 'tri':
            x = 4 * p - 1 if p < 0.5 else 3 - 4 * p
        else:
            x = math.sin(2 * math.pi * p)
        out.append(x)
    return _clip(out, 0.5)


def kick():
    n = int(0.16 * RATE)
    out, phase = [], 0.0
    for i, e in enumerate(_env(n, attack=0.001, decay=1.3)):
        f = 40 + 110 * math.exp(-i / (0.03 * RATE))
        phase += f / RATE
        out.append(math.sin(2 * math.pi * phase) * e)
    return _clip(out, 0.75)


def hat():
    rng = random.Random(7)
    n = int(0.05 * RATE)
    out, prev = [], 0.0
    for i, e in enumerate(_env(n, attack=0.0005, decay=2.0)):
        x = rng.uniform(-1, 1)
        out.append((x - prev) * e)   # a high-pass: the hiss only
        prev = x
    return _clip(out, 0.35)


def snare():
    rng = random.Random(9)
    n = int(0.14 * RATE)
    out = []
    for i, e in enumerate(_env(n, attack=0.001, decay=1.6)):
        out.append((rng.uniform(-1, 1) * 0.8 + 0.4 * math.sin(2 * math.pi * 190 * i / RATE)) * e)
    return _clip(out, 0.5)


def generate():
    C5, E5, G5, C6, D5, A5 = 523.25, 659.26, 783.99, 1046.5, 587.33, 880.0
    return [
        ('shoot', shoot(), None),
        ('boom', boom(), None),
        ('blast', blast(), None),
        ('step', step(), None),
        ('siren', siren(), 0),
        ('bonus', notes([C6, G5, C6, E5 * 2], 0.06, volume=0.25), None),
        ('cleared', notes([C5, E5, G5, C6, G5, C6], 0.09, decay=0.6, volume=0.28), None),
        ('start', notes([G5, C5, E5, G5, C6], 0.08, decay=0.6, volume=0.28), None),
        ('over', notes([G5, E5, D5, C5 / 2], 0.18, decay=0.4, volume=0.3, shape='sine'), None),
        ('clang', clang(), None),
        ('swoop', swoop(), None),
        ('warning', notes([A5, 0, A5, 0, A5], 0.12, decay=0.2, volume=0.25), None),
        # the music's instruments: looped cycles, and drums
        ('pulse', wave('pulse'), 0),
        ('saw', wave('saw'), 0),
        ('tri', wave('tri'), 0),
        ('kick', kick(), None),
        ('hat', hat(), None),
        ('snare', snare(), None),
    ]
