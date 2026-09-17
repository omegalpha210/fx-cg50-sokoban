#!/usr/bin/env python3
"""Measure icon pixels and reproduce public previews.

Optional --reference-dir reads DIFF EQ without changing it. Its actual artwork
comparison is written only to excluded docs/captures; public output retains
numeric placement evidence and the project's original SOKOBAN artwork.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw
from make_icons import TILE, SCENE, ORIGIN

ROOT = Path(__file__).resolve().parents[1]
PUBLIC = ROOT / 'docs/public-captures'


def measure(path):
    with Image.open(path) as source:
        image = source.convert('RGB')
        background = image.getpixel((0, 0))
        bounds = ImageChops.difference(image, Image.new('RGB', image.size, background)).getbbox()
        assert bounds is not None, 'empty icon artwork'
        left, top, right, bottom = bounds
        return {'mode': source.mode, 'canvas': list(image.size), 'background_rgb': list(background),
                'artwork_bounds_inclusive': [left, top, right-1, bottom-1],
                'top_margin': top, 'bottom_margin': image.height-bottom,
                'bytes': path.stat().st_size, 'colors': len(image.getcolors(image.width*image.height)),
                'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def report(reference=None):
    result = {'milestone': 'v0.1.0-beta.2', 'redesign': True, 'baked_in_text': False,
              'antialiasing': False, 'logical_tile': [TILE, TILE],
              'wall_footprint': [TILE, TILE], 'crate_footprint': [TILE, TILE],
              'crate_inner_fill': [8, 8], 'grid': [len(SCENE[0]), len(SCENE)],
              'grid_origin': list(ORIGIN), 'safe_bottom_limit': 43,
              'bounds_convention': 'inclusive non-background pixel bounds', 'variants': {}}
    for variant in ('uns', 'sel'):
        result['variants'][variant] = {
            'before': measure(PUBLIC / f'icon-{variant}-before.png'),
            'after': measure(ROOT / f'assets/icon-{variant}.png')}
        if reference:
            result['variants'][variant]['diffeq_reference'] = measure(reference / f'assets/icon-{variant}.png')
    return result


def paste_icon(sheet, path, position, scale):
    with Image.open(path) as icon:
        sheet.paste(icon.resize((92*scale, 64*scale), Image.Resampling.NEAREST), position)


def comparison(reference=None):
    local_reference = reference is not None
    sheet = Image.new('RGB', (824, 720), '#e5e9e7')
    d = ImageDraw.Draw(sheet)
    title = 'DIFF EQ placement reference / original SOKOBAN redesign' if local_reference else 'SOKOBAN beta.1 / beta.2 redesign'
    d.text((20, 12), title + ' - 4x nearest-neighbor', fill='black')
    for row, variant in enumerate(('uns', 'sel')):
        for column, stage in enumerate(('before', 'after')):
            path = (reference / f'assets/icon-{variant}.png' if local_reference else PUBLIC / f'icon-{variant}-before.png') if stage == 'before' else ROOT / f'assets/icon-{variant}.png'
            x, y = 20 + column*412, 48 + row*320
            label = ('DIFF EQ' if local_reference else 'OLD beta.1') if stage == 'before' else 'NEW beta.2'
            d.text((x,y), f'{label} / {variant}', fill='black')
            paste_icon(sheet, path, (x,y+22), 4)
            values = measure(path)
            d.text((x,y+283), f"Bounds {values['artwork_bounds_inclusive']}; bottom {values['bottom_margin']}px", fill='black')
    d.text((20,696), 'Placement comparison only. Physical CASIO Main Menu / OS label must be retested.', fill='black')
    return sheet


def previews(reference=None):
    PUBLIC.mkdir(parents=True, exist_ok=True)
    comparison().save(PUBLIC / 'icon-before-after.png')
    for variant in ('uns','sel'):
        with Image.open(ROOT / f'assets/icon-{variant}.png') as icon:
            icon.resize((736,512),Image.Resampling.NEAREST).save(PUBLIC / f'icon-{variant}-8x.png')
    # Intentional diagram, not a claim to reproduce the CASIO OS font/label position.
    sheet = Image.new('RGB',(824,400),'#e5e9e7'); d = ImageDraw.Draw(sheet)
    d.text((20,12),'ILLUSTRATIVE SAFE-AREA MOCK - NOT CASIO OS RENDERING',fill='black')
    for column,variant in enumerate(('uns','sel')):
        x,y=20+column*412,68
        with Image.open(ROOT/f'assets/icon-{variant}.png') as icon:
            canvas=icon.copy()
        overlay=ImageDraw.Draw(canvas)
        overlay.rectangle((0,42,91,63), fill='#fff0cd' if variant=='uns' else '#42503a')
        overlay.line((0,42,91,42),fill='#ad8124')
        overlay.text((21,52),'SOKOBAN',fill='black' if variant=='uns' else 'white')
        sheet.paste(canvas.resize((368,256),Image.Resampling.NEAREST),(x,y))
        d.text((x,45),f'{variant}: artwork y2..41 / reserve y42..63',fill='black')
        d.text((x,340),'22px reserved. Sample label at y52 is illustrative.',fill='black')
    d.text((20,376),'Actual assets have plain backgrounds here; no text, stripe, or tinted band is baked in.',fill='black')
    sheet.save(PUBLIC/'icon-label-safe-mock.png')
    if reference:
        output=ROOT/'docs/captures/icon-diffeq-placement.png'
        output.parent.mkdir(parents=True,exist_ok=True)
        comparison(reference).save(output)


def markdown(result):
    lines = ['# CASIO Main Menu icon redesign — v0.1.0-beta.2', '',
        'The user-supplied 92×64 screenshot was visually compared with the existing PNG. The beta.1 generator used 11×11 wall blocks and a 17×17 crate, matching the reported oversize. Its artwork ended at y46 and hardware feedback still found it close to the OS label. This milestone redraws the scene on a common grid; it is not another translation of that bitmap.', '',
        '## Source and exact geometry', '',
        '- Old source: `tools/make_icons.py` at public tag `v0.1.0-beta.1`; original PNG bytes are preserved as `docs/public-captures/icon-uns-before.png` and `icon-sel-before.png`.',
        '- New source: deterministic `tools/make_icons.py`; outputs `assets/icon-uns.png` and `assets/icon-sel.png`. Both are opaque RGB, **92×64**. No external sprites, commercial Sokoban art, CASIO art, text or fonts are used.',
        '- Shared integer grid: **8 columns × 4 rows, 10×10 px per cell**, origin `(6,2)`. Wall and crate outer footprints both fill one **10×10** cell; orange inner fill is **8×8**, inset by one pixel.',
        '- The black player fits in a 7×8 footprint and the dark-gold hollow target in 5×5, each inside one cell. Player, crate, target occupy consecutive cells `(2,2)`, `(3,2)`, `(4,2)` (zero-based).',
        '- Walls use flat dark green outlines/green fill, floor pale mint, crate orange, player black. A partial warehouse wall layout leaves open floor and avoids a heavy enclosing border. The icon scene is project-authored, not an upstream puzzle.',
        '- Native-resolution drawing uses integer rectangles/lines/points with no antialiasing or image scaling. The selected variant changes only the surrounding background to dark green; the entire 80×40 artwork is identical, retaining player/crate/target visibility.',
        '- New bounds are **(6,2)..(85,41)**. Top margin is **2px**, bottom margin **22px** (previously 17px). Rows 42..63 contain only the variant background. The lower edge is below the conservative limit y43.', '',
        '## Actual packaging pipeline', '',
        '`CMakeLists.txt::generate_g3a` passes both PNGs directly to fxgxa through `ICONS`; there is no fxconv icon transformation. Installed `GenerateG3A.cmake` maps these to `--icon-uns` and `--icon-sel`. `tools/package.sh` repeats that packaging deterministically when SOURCE_DATE_EPOCH is set. fxgxa stores two 92×64×2 RGB565 records (11,776 bytes each) at offsets 0x1000 and 0x4000. Their exact bytes are checked against the PNGs. Icons are G3A metadata, not runtime graphics/cache.', '',
        'App name `SOKOBAN`, internal ID `@SOKOBAN`, and save namespace remain unchanged. The OS name comes from separate G3A name/language-label metadata, not icon text. CASIO package version advances to `00.01.0002`; the game/runtime payload is unchanged.', '',
        '## Measured assets and placement reference', '',
        'DIFF EQ was inspected read-only, including its 92×64 normal/selected PNGs, 4× supersampled generator and CMake declarations. Its generator explicitly leaves the lower title strip free. All faint antialiasing pixels count in the bounds below. It is a placement reference only; none of its art is incorporated into SOKOBAN. This redesign deliberately uses a larger lower margin than that reference because of the reported physical overlap.', '',
        '| Variant / stage | Inclusive bounds | Top px | Bottom px | PNG bytes |',
        '|---|---|---:|---:|---:|']
    for variant, stages in result['variants'].items():
        for stage, v in stages.items():
            b=v['artwork_bounds_inclusive']
            lines.append(f"| {variant} / {stage} | ({b[0]},{b[1]})..({b[2]},{b[3]}) | {v['top_margin']} | {v['bottom_margin']} | {v['bytes']} |")
    lines += ['', '## Previews and reproduction', '',
        '- [Current beta.1 versus new beta.2, both variants](public-captures/icon-before-after.png).',
        '- [New unselected, 8× nearest-neighbor](public-captures/icon-uns-8x.png); [selected, 8×](public-captures/icon-sel-8x.png).',
        '- [Illustrative label-safe-area mock](public-captures/icon-label-safe-mock.png). The tinted reserve and sample label are explanatory overlays, absent from both icon assets. The sample y52 label is not a measured or emulated CASIO label position.',
        '- An optional actual DIFF EQ/SOKOBAN side-by-side is generated locally as `docs/captures/icon-diffeq-placement.png`. Reference artwork stays out of the public snapshot; the numeric evidence above is public.', '',
        'Run `python3 tools/make_icons.py`, then `python3 tools/icon_audit.py`. Optional `--reference-dir "$SOKOBAN_DIFFEQ_REFERENCE"` refreshes read-only reference measurements and the local comparison. `--check` verifies committed numeric/Markdown evidence without needing that checkout. SHA-256 and PNG color counts are in `ICON_AUDIT.json`.', '',
        'Eight icon tests cover deterministic PNGs, opaque flat palette, native canvas, clear top/bottom margins, equal measured wall/crate footprints, all-cell grid alignment, player/crate/goal placement, preserved selected artwork, absence of text drawing, audit dimensions and both native RGB565 records. No runtime game/UI/input/storage source is changed.', '',
        '**HARDWARE TEST REQUIRED:** actual OS-label gap, no top clipping, natural wall/crate scale, identifiable player/goal/push-puzzle scene, selected/unselected contrast, relative size beside other apps and separation comparable to DIFF EQ. Host geometry and mock labels cannot establish physical OS acceptance. See the priority icon checks in [HARDWARE_RETEST.md](HARDWARE_RETEST.md).']
    return '\n'.join(lines)+'\n'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-dir',type=Path)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    result=report(args.reference_dir)
    output=ROOT/'docs/ICON_AUDIT.json'
    if not args.reference_dir and output.exists():
        previous=json.loads(output.read_text())
        for variant in ('uns','sel'):
            if 'diffeq_reference' in previous['variants'][variant]:
                result['variants'][variant]['diffeq_reference']=previous['variants'][variant]['diffeq_reference']
    for path,content in {output:json.dumps(result,indent=2)+'\n',ROOT/'docs/ICON_AUDIT.md':markdown(result)}.items():
        if args.check:
            if not path.exists() or path.read_text()!=content:
                parser.error(f'stale {path.relative_to(ROOT)}')
        else:
            path.write_text(content)
    if not args.check:
        previews(args.reference_dir)
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
