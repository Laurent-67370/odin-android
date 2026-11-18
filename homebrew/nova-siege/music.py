#!/usr/bin/env python3
"""The music of NOVA SIEGE, composed for it: three tracks that loop. A
channel is a line of notes, NOTE:TICKS (C4:2, F#3:4, Bb2:1), - for a rest,
K, H and S for the kick, the hat and the snare; a tick is a sixteenth. Every
channel of a track lasts as long, so that they loop together.

generate() gives the tracks as build.py writes them to music.c."""
import re

WAVE_BASE = 22050 / 64          # an instrument at its own speed (tools/sounds.py)
NAMES = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}

# AICA envelopes: (AR, D1R, DL, D2R, RR), rates 0-31 (31 the fastest)
ENVELOPES = {
    'bass': (31, 9, 8, 5, 16),
    'lead': (29, 7, 4, 3, 14),
    'arp': (31, 13, 12, 8, 18),
    'pad': (22, 4, 2, 1, 10),
    'drum': (31, 0, 0, 0, 31),
}


def frequency(note):
    m = re.fullmatch(r'([A-G])([#b]?)(-?\d)', note)
    if not m:
        raise ValueError('not a note: %s' % note)
    semis = NAMES[m.group(1)] + {'#': 1, 'b': -1, '': 0}[m.group(2)]
    midi = 12 * (int(m.group(3)) + 1) + semis
    return 440.0 * 2 ** ((midi - 69) / 12)


def channel(text, instrument, level, pan=0, envelope='lead'):
    notes = []
    for token in text.split():
        name, ticks = token.split(':')
        ticks = int(ticks)
        if name == '-':
            notes.append((0, ticks, ''))
        elif name in ('K', 'H', 'S'):
            notes.append((1024, ticks, {'K': 'kick', 'H': 'hat', 'S': 'snare'}[name]))
        else:
            notes.append((round(frequency(name) / WAVE_BASE * 1024), ticks, instrument))
    return {'notes': notes, 'level': level, 'pan': pan, 'envelope': ENVELOPES[envelope]}


def bars(*parts):
    return ' '.join(parts)


def rep(text, n):
    return ' '.join([text] * n)


def title():
    """A minor, calm: Am F C G Am F G E, eight bars"""
    bass = bars('A2:4 A2:4 E3:4 A2:4', 'F2:4 F2:4 C3:4 F2:4', 'C3:4 C3:4 G2:4 C3:4', 'G2:4 G2:4 D3:4 G2:4',
                'A2:4 A2:4 E3:4 A2:4', 'F2:4 F2:4 C3:4 F2:4', 'G2:4 G2:4 D3:4 G2:4', 'E2:4 E2:4 B2:4 E2:4')
    arp = bars(rep('A4:1 C5:1 E5:1 C5:1', 4), rep('A4:1 C5:1 F5:1 C5:1', 4), rep('G4:1 C5:1 E5:1 C5:1', 4),
               rep('G4:1 B4:1 D5:1 B4:1', 4), rep('A4:1 C5:1 E5:1 C5:1', 4), rep('A4:1 C5:1 F5:1 C5:1', 4),
               rep('G4:1 B4:1 D5:1 B4:1', 4), rep('G#4:1 B4:1 E5:1 B4:1', 4))
    lead = bars('E5:4 D5:2 C5:2 B4:4 C5:4', 'A4:8 C5:4 F5:4', 'E5:6 D5:2 C5:4 G4:4', 'D5:8 B4:8',
                'E5:4 A5:4 G5:2 F5:2 E5:4', 'F5:4 E5:2 D5:2 C5:8', 'B4:4 C5:4 D5:4 G5:4', 'G#5:8 E5:8')
    hats = rep('-:2 H:2', 32)
    return {'tick_frames': 9, 'channels': [
        channel(bass, 'tri', 11, 0, 'bass'),
        channel(arp, 'pulse', 6, 5, 'arp'),
        channel(lead, 'saw', 9, -3, 'lead'),
        channel(hats, '', 5, 4, 'drum'),
    ]}


def game():
    """E minor, driving: Em C D B, eight bars, the tune in the second half"""
    riff = bars('E2:2 E2:2 E3:2 E2:2 G2:2 E2:2 A2:2 B2:2', 'C2:2 C2:2 C3:2 C2:2 E2:2 C2:2 G2:2 B2:2',
                'D2:2 D2:2 D3:2 D2:2 F#2:2 D2:2 A2:2 C3:2', 'B1:2 B1:2 B2:2 B1:2 D#2:2 B1:2 F#2:2 A2:2')
    bass = rep(riff, 2)
    lead = bars('-:64', 'B4:2 -:2 B4:2 G4:2 E5:4 D5:4', 'C5:4 B4:2 A4:2 G4:8', 'A4:2 -:2 A4:2 F#4:2 D5:4 C5:4',
                'B4:8 D#5:4 F#5:4')
    pad = bars('-:16 E4:16 -:16 F#4:16', 'G4:16 E4:16 F#4:16 D#4:16')
    drums = rep('K:2 H:2 K:2 H:2 S:2 H:2 K:2 H:2', 8)
    return {'tick_frames': 8, 'channels': [
        channel(bass, 'pulse', 10, 0, 'bass'),
        channel(lead, 'saw', 9, -3, 'lead'),
        channel(pad, 'tri', 7, 4, 'pad'),
        channel(drums, '', 8, 0, 'drum'),
    ]}


def boss():
    """D minor, fast: Dm Bb C A, four bars"""
    bass = bars(rep('D2:1 D2:1 D3:1 D2:1', 4), rep('Bb1:1 Bb1:1 Bb2:1 Bb1:1', 4), rep('C2:1 C2:1 C3:1 C2:1', 4),
                rep('A1:1 A1:1 A2:1 A1:1', 2) + ' ' + rep('A1:1 C#2:1 E2:1 A2:1', 2))
    lead = bars('D5:2 -:2 F5:2 -:2 A5:4 G5:2 F5:2', 'F5:2 -:2 D5:2 -:2 Bb4:4 C5:2 D5:2',
                'E5:2 -:2 G5:2 -:2 C6:4 Bb5:2 A5:2', 'C#5:4 E5:4 A5:4 G5:2 E5:2')
    stabs = bars('D4:2 -:6 D4:2 -:6', 'Bb3:2 -:6 Bb3:2 -:6', 'C4:2 -:6 C4:2 -:6', 'A3:2 -:6 C#4:2 -:2 E4:2 -:2')
    drums = rep('K:2 H:1 H:1 S:2 H:2 K:1 K:1 H:2 S:2 H:2', 4)
    return {'tick_frames': 6, 'channels': [
        channel(bass, 'saw', 10, 0, 'bass'),
        channel(lead, 'pulse', 9, -3, 'lead'),
        channel(stabs, 'tri', 8, 4, 'arp'),
        channel(drums, '', 8, 0, 'drum'),
    ]}


def generate():
    tracks = [('title', title()), ('game', game()), ('boss', boss())]
    for name, t in tracks:
        lengths = {sum(n[1] for n in c['notes']) for c in t['channels']}
        if len(lengths) != 1:
            raise ValueError('the channels of %s do not last as long: %s' % (name, sorted(lengths)))
    return tracks
