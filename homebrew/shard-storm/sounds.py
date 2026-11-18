#!/usr/bin/env python3
"""The sounds of SHARD STORM, synthesised (tools/synth.py): 16-bit samples
at 22050 Hz. generate() gives [(name, samples, loop_start or None)]."""
import math, os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
import synth  # noqa: E402

RATE = synth.RATE


def chime():
    """a brick breaking: a glassy ping, two partials; its pitch set per brick"""
    n = int(0.22 * RATE)
    out = []
    for i, e in enumerate(synth.env(n, attack=0.001, decay=1.6)):
        t = i / RATE
        out.append((math.sin(2 * math.pi * 880 * t) + 0.45 * math.sin(2 * math.pi * 2640 * t) * math.exp(-t * 18)) * e)
    return synth.clip(out, 0.32)


def bounce():
    return synth.sweep(520, 380, 0.06, 0.32, decay=1.4, shape='square')


def clang():
    n = int(0.18 * RATE)
    out = []
    for i, e in enumerate(synth.env(n, decay=1.7)):
        t = i / RATE
        out.append((math.sin(2 * math.pi * 1180 * t) + 0.8 * math.sin(2 * math.pi * 1693 * t)
                    + 0.5 * math.sin(2 * math.pi * 2507 * t)) * e)
    return synth.clip(out, 0.22)


def generate():
    C5, E5, G5, C6, E6, G6, D5, F5, A5 = 523.25, 659.26, 783.99, 1046.5, 1318.5, 1568.0, 587.33, 698.46, 880.0
    return [
        ('chime', chime(), None),
        ('bounce', bounce(), None),
        ('wall', synth.sweep(300, 260, 0.03, 0.18, decay=1.5, shape='square'), None),
        ('clang', clang(), None),
        ('bomb', synth.noise(0.5, 0.2, 1.0, 0.7, rumble=0.6, seed=5), None),
        ('laser', synth.sweep(2200, 700, 0.09, 0.22, decay=1.0, shape='square'), None),
        ('capsule', synth.notes([G5, C6, E6, G6], 0.05, volume=0.25), None),
        ('lose', synth.sweep(600, 90, 0.7, 0.35, decay=0.6), None),
        ('cleared', synth.notes([C5, E5, G5, C6, G5, C6, E6], 0.08, decay=0.6, volume=0.27), None),
        ('start', synth.notes([E5, G5, C6, 0, C6], 0.07, decay=0.6, volume=0.27), None),
        ('over', synth.notes([G5, F5, D5, C5 / 2], 0.2, decay=0.4, volume=0.3, shape='sine'), None),
        ('life', synth.notes([C6, E6, G6, C6 * 2], 0.07, volume=0.25, shape='sine'), None),
        ('launch', synth.sweep(300, 900, 0.08, 0.22, decay=0.8), None),
        # the music's instruments
        ('pulse', synth.wave('pulse'), 0),
        ('pulse25', synth.wave('pulse25'), 0),
        ('tri', synth.wave('tri'), 0),
        ('saw', synth.wave('saw'), 0),
        ('kick', synth.kick(), None),
        ('hat', synth.hat(), None),
        ('snare', synth.snare(), None),
    ]
