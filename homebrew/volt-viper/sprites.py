#!/usr/bin/env python3
"""The sprites of VOLT VIPER, drawn as text: this script writes them to
assets/*.png. A letter is a colour of PAL, '.' is transparent. The viper
and the walls are drawn by the game itself."""
import os
from PIL import Image

PAL = {'.': (0, 0, 0, 0), 'w': (255, 255, 240, 255), 'y': (255, 230, 90, 255), 'o': (250, 160, 40, 255),
       'O': (170, 90, 20, 255), 'c': (120, 240, 255, 255), 'b': (40, 150, 230, 255), 'B': (20, 70, 140, 255),
       'm': (250, 110, 230, 255), 'M': (150, 40, 150, 255)}

SPRITES = {
# an energy cell: a battery with a bolt
'cell': """
......OOOO......
.....OyyyyO.....
...OOOOOOOOOO...
..OyyyyyyyyyyO..
..OyyyyyywyyyO..
..OyyyyywwyyyO..
..OyyyywwwyyyO..
..OyyywwwwwyyO..
..OyyyywwwyyyO..
..OyyyywwyyyyO..
..OyyyywyyyyyO..
..OoyyyyyyyyoO..
..OooyyyyyyooO..
...OOOOOOOOOO...
................
................
""",
# a spark: worth more, for a while
'spark': """
.......w........
.......m........
......wmw.......
......mmm.......
.w...mmwmm...w..
..mmmmwwwmmmm...
...mmwwwwwmm....
wmmwwwwwwwwwmmw.
...mmwwwwwmm....
..mmmmwwwmmmm...
.w...mmwmm...w..
......mmm.......
......wmw.......
.......m........
.......w........
................
""",
}


def main():
    here = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'assets')
    os.makedirs(here, exist_ok=True)
    for name, art in SPRITES.items():
        rows = [r.replace(' ', '.') for r in art.strip('\n').split('\n')]
        im = Image.new('RGBA', (len(rows[0]), len(rows)))
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                im.putpixel((x, y), PAL[ch])
        im.save(os.path.join(here, name + '.png'))


if __name__ == '__main__':
    main()
