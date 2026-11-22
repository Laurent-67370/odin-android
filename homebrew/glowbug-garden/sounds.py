#!/usr/bin/env python3
"""The sounds of GLOWBUG GARDEN, synthesised (tools/synth.py): 16-bit samples
at 22050 Hz. generate() gives [(name, samples, loop_start or None)]."""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
import synth  # noqa: E402


def generate():
    C5, D5, E5, F5, G5, A5, B5, C6, E6, G6 = 523.25, 587.33, 659.26, 698.46, 783.99, 880.0, 987.77, 1046.5, 1318.5, 1568.0
    return [
        ('seed', synth.sweep(1400, 1900, 0.035, 0.16, decay=1.2, shape='sine'), None),
        ('flower', synth.notes([C5, E5, G5, C6, E6], 0.05, volume=0.25, shape='sine'), None),
        ('beetle', synth.sweep(300, 1600, 0.18, 0.26, decay=0.8, shape='square'), None),
        ('caught', synth.sweep(900, 120, 0.9, 0.32, decay=0.5), None),
        ('home', synth.sweep(1200, 400, 0.12, 0.12, decay=1.0, shape='square'), None),
        ('cleared', synth.notes([G5, C6, E6, G6, E6, G6, C6 * 2], 0.08, decay=0.6, volume=0.27, shape='sine'), None),
        ('start', synth.notes([E5, G5, A5, G5, C6], 0.09, decay=0.6, volume=0.27), None),
        ('over', synth.notes([B5, G5, E5, C5 / 2], 0.2, decay=0.4, volume=0.3, shape='sine'), None),
        ('life', synth.notes([C6, E6, G6, C6 * 2], 0.07, volume=0.25, shape='sine'), None),
        # the music's instruments
        ('pulse', synth.wave('pulse'), 0),
        ('pulse25', synth.wave('pulse25'), 0),
        ('tri', synth.wave('tri'), 0),
        ('sine', synth.wave('sine'), 0),
        ('kick', synth.kick(), None),
        ('hat', synth.hat(), None),
        ('snare', synth.snare(), None),
    ]
