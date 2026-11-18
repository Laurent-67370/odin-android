#!/usr/bin/env python3
"""The music of SHARD STORM, composed for it: three tracks that loop
(tools/synth.py writes the notes: NOTE:TICKS, - a rest, K H S the drums; a
tick a sixteenth). generate() gives them to build.py."""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
from synth import channel, rep, checked  # noqa: E402


def title():
    """C major, airy: C Am F G, four bars twice"""
    bass = rep('C3:8 G2:8 A2:8 E2:8 F2:8 C3:8 G2:8 D3:8', 2)
    arp = rep(' '.join([rep('C5:1 E5:1 G5:1 E5:1', 4), rep('A4:1 C5:1 E5:1 C5:1', 4),
                        rep('A4:1 C5:1 F5:1 C5:1', 4), rep('B4:1 D5:1 G5:1 D5:1', 4)]), 2)
    lead = ' '.join(['G5:6 E5:2 C6:8', 'A5:6 G5:2 E5:8', 'F5:4 A5:4 C6:4 A5:4', 'G5:12 -:4',
                     'E6:6 D6:2 C6:8', 'C6:6 B5:2 A5:8', 'A5:4 G5:4 F5:4 A5:4', 'G5:8 D5:8'])
    return {'tick_frames': 9, 'channels': [
        channel(bass, 'tri', 11, 0, 'bass'),
        channel(arp, 'pulse25', 6, 5, 'pluck'),
        channel(lead, 'pulse', 9, -3, 'lead'),
    ]}


def level_a():
    """G minor, a steady groove: Gm Eb F D"""
    riff = ' '.join(['G2:2 G2:2 D3:2 G2:2 Bb2:2 G2:2 F2:2 G2:2', 'Eb2:2 Eb2:2 Bb2:2 Eb2:2 G2:2 Eb2:2 D2:2 Eb2:2',
                     'F2:2 F2:2 C3:2 F2:2 A2:2 F2:2 Eb2:2 F2:2', 'D2:2 D2:2 A2:2 D2:2 F#2:2 D2:2 A2:2 C3:2'])
    bass = rep(riff, 2)
    lead = ' '.join(['-:64', 'D5:4 Bb4:2 G4:2 D5:4 Eb5:4', 'G5:6 F5:2 Eb5:8', 'F5:4 D5:2 C5:2 A4:4 C5:4',
                     'D5:8 F#5:4 A5:4'])
    stabs = rep(' '.join(['-:4 G4:2 -:2 -:4 Bb4:2 -:2', '-:4 G4:2 -:2 -:4 Bb4:2 -:2',
                          '-:4 A4:2 -:2 -:4 C5:2 -:2', '-:4 F#4:2 -:2 -:4 A4:2 -:2']), 2)
    drums = rep('K:2 H:2 S:2 H:2 K:2 K:2 S:2 H:2', 8)
    return {'tick_frames': 8, 'channels': [
        channel(bass, 'pulse', 10, 0, 'bass'),
        channel(lead, 'saw', 9, -3, 'lead'),
        channel(stabs, 'tri', 7, 4, 'pluck'),
        channel(drums, '', 7, 0, 'drum'),
    ]}


def level_b():
    """F major, bright and quick: F C Dm Bb, then Gm C"""
    bass = ' '.join(['F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 C3:2 A2:2', 'C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 G2:2 E2:2',
                     'D2:2 D3:2 D2:2 D3:2 D2:2 D3:2 A2:2 F2:2', 'Bb1:2 Bb2:2 Bb1:2 Bb2:2 G2:2 G3:2 C3:2 C2:2'])
    arp = ' '.join([rep('F4:1 A4:1 C5:1 A4:1', 4), rep('E4:1 G4:1 C5:1 G4:1', 4), rep('F4:1 A4:1 D5:1 A4:1', 4),
                    rep('F4:1 Bb4:1 D5:1 Bb4:1', 2) + ' ' + rep('E4:1 G4:1 C5:1 G4:1', 2)])
    lead = ' '.join(['C6:2 A5:2 F5:2 A5:2 C6:4 F6:4', 'E6:2 C6:2 G5:2 C6:2 E6:8',
                     'D6:2 A5:2 F5:2 A5:2 D6:4 C6:2 A5:2', 'Bb5:4 D6:4 C6:4 -:4'])
    drums = rep('K:2 H:1 H:1 S:2 H:2 K:2 H:2 S:2 H:2', 4)
    return {'tick_frames': 7, 'channels': [
        channel(bass, 'tri', 11, 0, 'bass'),
        channel(arp, 'pulse25', 6, 4, 'pluck'),
        channel(lead, 'pulse', 9, -3, 'lead'),
        channel(drums, '', 7, 0, 'drum'),
    ]}


def generate():
    return checked([('title', title()), ('level_a', level_a()), ('level_b', level_b())])
