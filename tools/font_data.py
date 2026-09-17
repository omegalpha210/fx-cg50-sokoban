#!/usr/bin/env python3
"""Convert gint's proportional 8x11 printable atlas; see asset provenance."""
from pathlib import Path
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]
im = Image.open(ROOT / 'assets/font/font8x9.png').convert('RGB')
glyphs, widths = [], []
for i in range(95):
    col, row = i % (im.width//10), i // (im.width//10)
    g = im.crop((col*10+1, row*13+1, col*10+9, row*13+12))
    left, right = 0, 8
    def blank(x): return all(g.getpixel((x,y)) == (255,255,255) for y in range(11))
    while left+1 < right and blank(left): left += 1
    while right-1 > left and blank(right-1): right -= 1
    widths.append(right-left)
    glyphs.append([sum((g.getpixel((x+left,y)) == (0,0,0)) << x for x in range(right-left)) for y in range(11)])
out = '/* Generated from gint font8x9; see docs/ASSET_PROVENANCE.md. */\n'
out += 'static const unsigned char glyph_width[95]={' + ','.join(map(str,widths)) + '};\n'
out += 'static const unsigned char glyph_rows[95][11]={\n' + ',\n'.join('{'+','.join(map(str,r))+'}' for r in glyphs) + '\n};\n'
(ROOT / 'src/ui/font_data.h').write_text(out)
