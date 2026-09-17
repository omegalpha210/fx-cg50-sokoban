#!/usr/bin/env python3
"""Original project geometry, no external sprites or icon/font assets.

Keep the 92×64 canvas and move the original geometry up three pixels. A one-pixel
upper margin is retained; all seventeen bottom rows are reserved for visual
separation from the CASIO OS label. No app-name text is baked into the artwork.
"""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ARTWORK_DY = -3


def draw_icon(selected=False, vertical_offset=ARTWORK_DY):
    im = Image.new('RGB', (92, 64), '#16372c' if selected else 'white')
    d = ImageDraw.Draw(im)

    def box(coords):
        x1, y1, x2, y2 = coords
        return x1, y1 + vertical_offset, x2, y2 + vertical_offset

    d.rounded_rectangle(box((8, 4, 83, 49)), radius=5, fill='#d7eddb', outline='#195e3d', width=2)
    for x, y in ((12, 8), (25, 8), (38, 8), (51, 8), (64, 8), (12, 21), (12, 34), (25, 34), (64, 34)):
        d.rectangle(box((x, y, x+10, y+10)), fill='#2d8e57', outline='#1b5838')
    d.rectangle(box((43, 23, 59, 39)), fill='#fba437', outline='#9d530d', width=2)
    d.line(box((46, 26, 56, 36)), fill='#bd6915', width=2)
    d.line(box((56, 26, 46, 36)), fill='#bd6915', width=2)
    d.ellipse(box((29, 22, 34, 27)), fill='black')
    d.rectangle(box((28, 29, 35, 34)), fill='black')
    d.ellipse(box((70, 22, 74, 26)), fill='#67451c')
    return im


def main():
    for selected in (False, True):
        draw_icon(selected).save(ROOT / 'assets' / ('icon-sel.png' if selected else 'icon-uns.png'))


if __name__ == '__main__':
    main()
