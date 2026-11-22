#!/usr/bin/env python3
"""The music of VOLT VIPER, composed for it: two tracks that loop
(tools/synth.py writes the notes: NOTE:TICKS, - a rest, K H S the drums; a
tick a sixteenth). generate() gives them to build.py."""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
from synth import channel, rep, checked  # noqa: E402


def title():
    """E minor, slow and electric: Em C D Bm"""
    bass = rep('E2:4 E2:4 B2:4 E2:4 C2:4 C2:4 G2:4 C2:4 D2:4 D2:4 A2:4 D2:4 B1:4 B1:4 F#2:4 B1:4', 2)
    arp = rep(' '.join([rep('E4:1 G4:1 B4:1 G4:1', 4), rep('C4:1 E4:1 G4:1 E4:1', 4),
                        rep('D4:1 F#4:1 A4:1 F#4:1', 4), rep('B3:1 D4:1 F#4:1 D4:1', 4)]), 2)
    lead = ' '.join(['B4:8 E5:8', 'G5:6 F#5:2 E5:8', 'F#5:8 A5:8', 'F#5:12 -:4',
                     'B5:8 A5:4 G5:4', 'G5:6 E5:2 C5:8', 'D5:4 F#5:4 A5:4 G5:4', 'F#5:16'])
    return {'tick_frames': 9, 'channels': [
        channel(bass, 'tri', 11, 0, 'bass'),
        channel(arp, 'pulse25', 6, 5, 'pluck'),
        channel(lead, 'saw', 8, -3, 'lead'),
    ]}


def play():
    """A minor, driving: Am F G Em"""
    riff = ' '.join(['A2:2 A3:2 A2:2 E3:2 A2:2 A3:2 G3:2 E3:2', 'F2:2 F3:2 F2:2 C3:2 F2:2 F3:2 E3:2 C3:2',
                     'G2:2 G3:2 G2:2 D3:2 G2:2 G3:2 F3:2 D3:2', 'E2:2 E3:2 E2:2 B2:2 E2:2 G#2:2 B2:2 E3:2'])
    bass = rep(riff, 2)
    lead = ' '.join(['E5:4 A5:4 C6:4 B5:2 A5:2', 'C6:6 A5:2 F5:8', 'D5:4 G5:4 B5:4 A5:2 G5:2', 'G#5:8 E5:8',
                     'A5:2 C6:2 E6:4 D6:2 C6:2 B5:4', 'A5:4 F5:4 C6:8', 'B5:4 G5:4 D6:4 B5:4', 'E6:8 G#5:4 B5:4'])
    drums = rep('K:2 H:2 S:2 H:2 K:2 K:2 S:2 H:1 H:1', 8)
    return {'tick_frames': 7, 'channels': [
        channel(bass, 'pulse', 10, 0, 'bass'),
        channel(lead, 'pulse25', 9, -3, 'lead'),
        channel(drums, '', 7, 0, 'drum'),
    ]}


def generate():
    return checked([('title', title()), ('play', play())])
