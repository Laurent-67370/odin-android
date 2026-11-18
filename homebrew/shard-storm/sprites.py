#!/usr/bin/env python3
"""The sprites of SHARD STORM, drawn as text: this script writes them to
assets/*.png. A letter is a colour of PAL, '.' is transparent. The bricks,
the paddle and the capsules are drawn by the game itself."""
import os
from PIL import Image

PAL = {'.': (0, 0, 0, 0), 'w': (250, 250, 255, 255), 'c': (140, 230, 255, 255), 'b': (60, 140, 230, 255),
       'B': (30, 70, 150, 255), 'p': (230, 120, 250, 255), 'P': (130, 50, 170, 255), 'y': (255, 230, 120, 255)}

SPRITES = {
'ball': """
..cwwc..
.cwwwwc.
cwwwwwcb
wwwwwcbb
wwwwccbB
cwwccbbB
.cbbbBB.
..bBBB..
""",
# a crystal shard, for the title
'shard': """
.......w........
......wcw.......
......wcpw......
.....wccpPw.....
.....wcwpPw.....
....wccwppPw....
....wcwwppPw....
...wccwwpppPw...
...wcwwwpppPw...
..wccwwwppppPw..
..wcwwwwppppPw..
...wcwwwppppw...
....wcwwpppw....
.....wcwppw.....
......wcpw......
.......ww.......
""",
}

here = os.path.dirname(os.path.abspath(__file__))
out = os.path.join(here, 'assets')
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
