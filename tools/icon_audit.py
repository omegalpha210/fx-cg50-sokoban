#!/usr/bin/env python3
"""Report real icon pixels and create the SOKOBAN-only before/after comparison.

An optional --reference-dir reads DIFF EQ icons for numeric placement evidence.
The reference artwork and private filesystem path are never copied to outputs.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw

ROOT = Path(__file__).resolve().parents[1]


def measure(path):
    with Image.open(path) as source:
        source.load()
        image = source.convert('RGB')
        background = image.getpixel((0, 0))
        bounds = ImageChops.difference(image, Image.new('RGB', image.size, background)).getbbox()
        assert bounds is not None, 'empty icon artwork'
        left, top, right, bottom = bounds
        return {'mode': source.mode, 'canvas': list(image.size), 'background_rgb': list(background),
                'artwork_bounds_inclusive': [left, top, right-1, bottom-1],
                'top_margin': top, 'bottom_margin': image.height-bottom,
                'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def report(reference=None):
    result = {'translation_y': -3, 'scaling': False, 'baked_in_text': False,
              'bounds_convention': 'inclusive non-background pixel bounds',
              'variants': {}}
    for variant in ('uns', 'sel'):
        result['variants'][variant] = {
            'before': measure(ROOT / f'docs/public-captures/icon-{variant}-before.png'),
            'after': measure(ROOT / f'assets/icon-{variant}.png')}
        if reference:
            result['variants'][variant]['diffeq_reference'] = measure(reference / f'assets/icon-{variant}.png')
    return result


def preview():
    sheet = Image.new('RGB', (720, 700), '#e5e9e7')
    draw = ImageDraw.Draw(sheet)
    draw.text((18, 10), 'SOKOBAN: actual icon pixels, nearest-neighbor enlargement (3x)', fill='black')
    for row, variant in enumerate(('uns', 'sel')):
        for col, label in enumerate(('before', 'after')):
            x, y = 24 + col * 350, 55 + row * 325
            path = ROOT / (f'docs/public-captures/icon-{variant}-before.png' if label == 'before' else f'assets/icon-{variant}.png')
            with Image.open(path) as icon:
                sheet.paste(icon.resize((276, 192), Image.Resampling.NEAREST), (x, y+22))
            bounds = measure(path)
            draw.text((x, y), f'{label.upper()} / {variant}', fill='black')
            draw.text((x, y+222), f"Bounds: {bounds['artwork_bounds_inclusive']}", fill='black')
            draw.text((x, y+240), f"Top: {bounds['top_margin']}px; bottom: {bounds['bottom_margin']}px", fill='black')
    draw.text((18, 675), 'Canvas remains 92x64. OS label is separate; actual calculator retest required.', fill='black')
    sheet.save(ROOT / 'docs/public-captures/icon-before-after.png')


def markdown(result):
    lines = ['# CASIO Main Menu icon audit', '',
             'The icon contains original drawn wall/crate/player geometry and **no baked-in SOKOBAN text**. Both variants are opaque RGB PNGs, 92×64. The bottom strip is the image background (white when unselected, dark green when selected), not transparency.', '',
             '`CMakeLists.txt` passes `assets/icon-uns.png` and `assets/icon-sel.png` to `generate_g3a`, which invokes fxgxa to encode the two native RGB565 icon records. The installed `GenerateG3A.cmake` was checked directly: `ICONS` becomes `--icon-uns`/`--icon-sel` and `NAME` becomes `-n`. Installed fxgxa `edit.c:edit_name` writes the header name and language-label fields; `edit_g3a_icon` copies 92×64×2 bytes per variant. Icons are not loaded or cached by the app at runtime. `NAME "SOKOBAN"` supplies the OS app-name metadata, and the internal identity remains `@SOKOBAN`; the CASIO menu draws its app label separately. The project icon generator calls no text/font drawing routine.', '',
             'The reported hardware overlap is therefore a placement issue: the lower edge of the graphic formerly reached y49, leaving 14 clear rows. The source geometry is now translated upward by 3 px, preserving its size, colors, all artwork pixels, and a 1 px top margin. The lower edge is y46 and the bottom margin is 17 px. This is the greatest whole-pixel upward translation that preserves a nonzero top margin; further movement would require reducing the graphic or touching the top edge.', '',
             'The DIFF EQ icon generator explicitly keeps the bottom title strip free for the OS label. Its actual antialiased non-background bounds were measured read-only; only the placement measurements below are retained. No DIFF EQ artwork was copied. Its faint antialiasing fringes are included in the bounds, so top margins need not equal the source geometric endpoints.', '',
             '| Icon | Canvas | Inclusive artwork bounds | Top margin | Bottom margin |',
             '|---|---|---|---:|---:|']
    for variant, stages in result['variants'].items():
        for stage, values in stages.items():
            bounds = values['artwork_bounds_inclusive']
            lines.append(f"| {variant}: {stage} | 92×64 | ({bounds[0]}, {bounds[1]})..({bounds[2]}, {bounds[3]}) | {values['top_margin']} | {values['bottom_margin']} |")
    lines += ['', '[Actual before/after preview](public-captures/icon-before-after.png) uses only SOKOBAN pixels at 3× nearest-neighbor enlargement. The original PNG snapshots were preserved before regeneration. Numeric results and input hashes are in `ICON_AUDIT.json`.', '',
              'Reproduce with `python3 tools/make_icons.py` then `python3 tools/icon_audit.py`. Optional `--reference-dir "$SOKOBAN_DIFFEQ_REFERENCE"` refreshes reference measurements without changing that repository. `python3 tools/icon_audit.py --check` needs no DIFF EQ checkout. Tests verify every artwork pixel survives the translation, both RGB565 package icon records match the new PNGs, and the new safe margins are blank.', '',
              '**HARDWARE RETEST REQUIRED:** actual CASIO Main Menu icon/label separation, selected and unselected contrast, and top-edge visibility. The 3 px change is measurable; host preview alone cannot establish whether the physical OS label gap is sufficient.']
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-dir', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = report(args.reference_dir)
    output = ROOT / 'docs/ICON_AUDIT.json'
    if not args.reference_dir and output.exists():
        previous = json.loads(output.read_text())
        for variant in ('uns', 'sel'):
            reference = previous['variants'][variant].get('diffeq_reference')
            if reference:
                result['variants'][variant]['diffeq_reference'] = reference
    outputs = {output: json.dumps(result, indent=2) + '\n', ROOT / 'docs/ICON_AUDIT.md': markdown(result)}
    for path, content in outputs.items():
        if args.check:
            if not path.exists() or path.read_text() != content:
                parser.error(f'stale {path.relative_to(ROOT)}')
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content)
    if not args.check:
        preview()
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
