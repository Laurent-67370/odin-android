#!/usr/bin/env python3
"""The music of GLOWBUG GARDEN, composed for it: two tracks that loop
(tools/synth.py writes the notes: NOTE:TICKS, - a rest, K H S the drums; a
tick a sixteenth). generate() gives them to build.py."""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
from synth import channel, rep, checked  # noqa: E402


def title():
    """D major, a night garden: D Bm G A"""
    bass = rep('D3:8 A2:8 B2:8 F#2:8 G2:8 D3:8 A2:8 E2:8', 2)
    arp = rep(' '.join([rep('D5:1 F#5:1 A5:1 F#5:1', 4), rep('B4:1 D5:1 F#5:1 D5:1', 4),
                        rep('G4:1 B4:1 D5:1 B4:1', 4), rep('A4:1 C#5:1 E5:1 C#5:1', 4)]), 2)
    lead = ' '.join(['A5:6 F#5:2 D5:8', 'F#5:4 E5:4 D5:8', 'B5:6 A5:2 G5:8', 'E5:12 -:4',
                     'A5:6 B5:2 A5:8', 'F#5:4 A5:4 D6:8', 'D6:4 B5:4 G5:4 B5:4', 'A5:16'])
    return {'tick_frames': 9, 'channels': [
        channel(bass, 'tri', 11, 0, 'bass'),
        channel(arp, 'sine', 7, 5, 'pluck'),
        channel(lead, 'pulse25', 8, -3, 'lead'),
    ]}


def play():
    """G major, light and bouncing: G Em C D"""
    bass = rep(' '.join(['G2:2 -:2 D3:2 -:2 G2:2 -:2 B2:2 D3:2', 'E2:2 -:2 B2:2 -:2 E2:2 -:2 G2:2 B2:2',
                         'C3:2 -:2 G2:2 -:2 C3:2 -:2 E3:2 G2:2', 'D3:2 -:2 A2:2 -:2 D3:2 -:2 F#2:2 A2:2']), 2)
    lead = ' '.join(['B5:2 D6:2 B5:2 G5:2 A5:4 B5:4', 'G5:2 B5:2 G5:2 E5:2 D5:8', 'E5:2 G5:2 C6:2 B5:2 A5:4 G5:4',
                     'F#5:4 A5:4 D6:8', 'D6:2 B5:2 G5:2 B5:2 D6:4 E6:4', 'B5:4 G5:4 E5:8', 'C6:2 E6:2 C6:2 G5:2 A5:4 B5:4',
                     'A5:4 F#5:4 G5:8'])
    drums = rep('K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2', 8)
    return {'tick_frames': 7, 'channels': [
        channel(bass, 'pulse', 9, 0, 'bass'),
        channel(lead, 'pulse25', 8, -3, 'lead'),
        channel(drums, '', 6, 0, 'drum'),
    ]}


def generate():
    return checked([('title', title()), ('play', play())])
