#!/usr/bin/env python3
"""The sounds of VOLT VIPER, synthesised (tools/synth.py): 16-bit samples
at 22050 Hz. generate() gives [(name, samples, loop_start or None)]."""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
import synth  # noqa: E402


def generate():
    C5, D5, E5, F5, G5, A5, C6, E6, G6 = 523.25, 587.33, 659.26, 698.46, 783.99, 880.0, 1046.5, 1318.5, 1568.0
    return [
        ('eat', synth.sweep(500, 1300, 0.07, 0.28, decay=1.0, shape='square'), None),
        ('spark', synth.notes([G5, C6, E6, G6, E6, G6], 0.04, volume=0.25), None),
        ('appear', synth.sweep(1800, 2400, 0.05, 0.15, decay=1.2), None),
        ('turn', synth.sweep(240, 200, 0.02, 0.10, decay=1.5, shape='square'), None),
        ('crash', synth.noise(0.6, 0.25, 1.0, 0.6, rumble=0.7, seed=9), None),
        ('cleared', synth.notes([C5, E5, G5, C6, E5, G5, C6, E6], 0.07, decay=0.6, volume=0.27), None),
        ('start', synth.notes([C5, G5, E5, C6], 0.08, decay=0.6, volume=0.27), None),
        ('over', synth.notes([A5, F5, D5, C5 / 2], 0.2, decay=0.4, volume=0.3, shape='sine'), None),
        ('life', synth.notes([C6, E6, G6, C6 * 2], 0.07, volume=0.25, shape='sine'), None),
        # the music's instruments
        ('pulse', synth.wave('pulse'), 0),
        ('pulse25', synth.wave('pulse25'), 0),
        ('tri', synth.wave('tri'), 0),
        ('saw', synth.wave('saw'), 0),
        ('kick', synth.kick(), None),
        ('hat', synth.hat(), None),
        ('snare', synth.snare(), None),
    ]
