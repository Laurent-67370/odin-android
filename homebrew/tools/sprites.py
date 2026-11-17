#!/usr/bin/env python3
"""The sprites of NOVA SIEGE, drawn as text here: this script writes them
to assets/*.png. Edit the drawings, run it, then build.py.

A letter is a colour of PAL, '.' is transparent."""
import os
from PIL import Image

PAL = {'.': (0, 0, 0, 0), 'w': (240, 240, 250, 255), 'p': (240, 90, 220, 255), 'P': (150, 40, 140, 255),
       'c': (80, 220, 250, 255), 'C': (30, 120, 170, 255), 'g': (110, 230, 90, 255), 'G': (40, 130, 50, 255),
       'r': (250, 70, 60, 255), 'R': (150, 30, 30, 255), 'y': (250, 220, 80, 255), 'o': (250, 150, 40, 255),
       'b': (90, 150, 250, 255), 'B': (40, 70, 160, 255), 'k': (10, 10, 20, 255)}

SPRITES = {
# The top row, 30 points: a horned watcher
'watcher_0': """
...p....p...
....p..p....
...pppppp...
..ppwkkwpp..
.pppwkkwppp.
.pPpppppPpp.
..p.p..p.p..
.p........p.
""",
'watcher_1': """
...p....p...
....p..p....
...pppppp...
..ppkwwkpp..
.pppkwwkppp.
.ppPppppPpp.
.p..p..p..p.
..p......p..
""",
# The middle rows, 20 points: a drifting jelly
'jelly_0': """
....cccc....
..cccccccc..
.cCcwcCwcCc.
.cccccccccc.
..c.c..c.c..
.c..c..c..c.
..c..cc..c..
............
""",
'jelly_1': """
....cccc....
..cccccccc..
.cCwcCcwcCc.
.cccccccccc.
..c.c..c.c..
..c.c..c.c..
.c.c....c.c.
............
""",
# The bottom rows, 10 points: a shelled beetle
'beetle_0': """
..g......g..
...g....g...
..gGGggGGg..
.ggwkggkwgg.
gggggggggggg
g.gGgggGg.g.
g.g......g.g
..gg....gg..
""",
'beetle_1': """
..g......g..
g..g....g..g
g.gGGggGGg.g
gggwkggkwggg
.gggggggggg.
..gGgggGg...
.g........g.
g..........g
""",
# Armoured, 40 points, two hits: a plated crawler (its cracked look after a hit)
'tank_0': """
...oooooo...
..oyyyyyyo..
.oyokkkkoyo.
oooyowwoyooo
oyyyyyyyyyyo
.o.oyooyo.o.
o..o....o..o
..o......o..
""",
'tank_1': """
...oooooo...
..oyyyyyyo..
.oyowwwwoyo.
oooyokkoyooo
oyyyyyyyyyyo
.o.oyooyo.o.
.o.o....o.o.
o..........o
""",
'tank_cracked': """
...oo.ooo...
..oy.yy.yo..
.oyokk.koyo.
oo.yowwoy.oo
oyy.yyyy.yyo
.o.oyo.yo.o.
o..o....o..o
..o......o..
""",
# The mothership of every fifth wave
'mothership': """
............rrrrrrrr............
.........rrrRRRRRRRRrrr.........
.......rrRRRwwwwwwwwRRRrr.......
.....rrRRRRwwkkwwkkwwRRRRrr.....
...rrRRRRRRwwkkwwkkwwRRRRRRrr...
..rRRRRRRRRRwwwwwwwwRRRRRRRRRr..
.rrrrrrrrrrrrrrrrrrrrrrrrrrrrrr.
rRyyRRyyRRyyRRyyRRyyRRyyRRyyRRyr
rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr
.rRRR..RRRR..RRRRRR..RRRR..RRRr.
..rR....rR....rRRr....rR....Rr..
...o.....o.....oo.....o.....o...
...y.....y.....yy.....y.....y...
""",
# The player: a delta-wing fighter
'fighter': """
.......w.......
......bwb......
......bwb......
.....bbcbb.....
..b..bcccb..b..
.bbbbbbbbbbbbb.
bbBbbBbbbBbbBbb
.....o...o.....
""",
# The bonus: a raider crossing the top
'raider': """
.....rrrrrr.....
...rrRrrrrRrr...
..rrrrrrrrrrrr..
.ryyrryyrryyrry.
rrrrrrrrrrrrrrrr
..rRR..rr..RRr..
...r........r...
""",
# Explosions
'boom': """
..y...o...y.
...y..o..y..
.o..yyyy..o.
...yowwoy...
oyywwwwwwyyo
...yowwoy...
.o..yyyy..o.
...y..o..y..
""",
'blast': """
.r..r...r..r....
..r..r.r..r.....
...rrryrrr......
.r.ryyyyyr.r....
..rryywyyrr.....
.r.ryyyyyr.r....
...rrryrrr......
..r..r.r..r.....
""",
}

here = os.path.dirname(os.path.abspath(__file__))
out = os.path.join(here, '..', 'assets')
os.makedirs(out, exist_ok=True)
for name, art in SPRITES.items():
    rows = art.strip('\n').split('\n')
    width = max(len(r) for r in rows)
    im = Image.new('RGBA', (width, len(rows)))
    for y, r in enumerate(rows):
        for x, ch in enumerate(r.ljust(width, '.')):
            im.putpixel((x, y), PAL[ch])
    im.save(os.path.join(out, name + '.png'))
print('%d sprites in assets/' % len(SPRITES))
