#!/usr/bin/env python3
"""Original 10px-grid warehouse icon. No imported artwork, fonts or text.

Every wall and crate has the same 10x10 outer footprint. Player and goal stay
inside that grid. Draw directly at native resolution with opaque flat colors;
there is no resampling/antialiasing step. Rows 42..63 are a clear OS-label margin.
"""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
CANVAS = (92, 64)
TILE = 10
ORIGIN = (6, 2)
# An original icon scene, not a map from the upstream puzzle pack.
SCENE = (
    'WWWWWW..',
    'W......W',
    'W.PCT..W',
    'WWW.....',
)
PALETTE = {
    'floor': '#d7eddb', 'wall_edge': '#195e3d', 'wall': '#2d8e57',
    'crate_edge': '#9d530d', 'crate': '#fba437', 'crate_cross': '#bd6915',
    'player': '#000000', 'goal': '#67451c',
}
BACKGROUNDS = ('#ffffff', '#16372c')


def cell_bounds(column, row):
    """Inclusive pixel bounds shared by every logical cell."""
    x, y = ORIGIN[0] + column * TILE, ORIGIN[1] + row * TILE
    return x, y, x + TILE - 1, y + TILE - 1


def draw_tile(symbol):
    """Return one opaque 10x10 tile, including its floor if unoccupied."""
    if symbol not in '.WPCT':
        raise ValueError(f'Unknown icon tile: {symbol}')
    tile = Image.new('RGB', (TILE, TILE), PALETTE['floor'])
    d = ImageDraw.Draw(tile)
    if symbol in 'WC':
        edge, fill = ('wall_edge', 'wall') if symbol == 'W' else ('crate_edge', 'crate')
        d.rectangle((0, 0, 9, 9), fill=PALETTE[edge])
        d.rectangle((1, 1, 8, 8), fill=PALETTE[fill])
        if symbol == 'C':
            d.line((2, 2, 7, 7), fill=PALETTE['crate_cross'])
            d.line((7, 2, 2, 7), fill=PALETTE['crate_cross'])
    elif symbol == 'P':
        d.rectangle((4, 1, 6, 3), fill=PALETTE['player'])
        d.rectangle((3, 4, 7, 7), fill=PALETTE['player'])
        d.rectangle((2, 5, 8, 6), fill=PALETTE['player'])
        d.point((3, 8), fill=PALETTE['player'])
        d.point((7, 8), fill=PALETTE['player'])
    elif symbol == 'T':
        # A small hollow diamond; visually distinct from the filled orange crate.
        d.line(((5, 3), (7, 5), (5, 7), (3, 5), (5, 3)), fill=PALETTE['goal'])
    return tile


def draw_icon(selected=False):
    image = Image.new('RGB', CANVAS, BACKGROUNDS[bool(selected)])
    for row, cells in enumerate(SCENE):
        for column, symbol in enumerate(cells):
            x, y, _, _ = cell_bounds(column, row)
            image.paste(draw_tile(symbol), (x, y))
    return image


def main():
    for selected, variant in ((False, 'uns'), (True, 'sel')):
        draw_icon(selected).save(ROOT / 'assets' / f'icon-{variant}.png')


if __name__ == '__main__':
    main()
