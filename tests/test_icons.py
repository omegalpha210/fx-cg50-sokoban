"""Icon acceptance: equal tile footprints, safe bounds and real package pixels."""
import importlib.util
import json
from pathlib import Path
import struct
import unittest
from unittest.mock import patch
from PIL import Image, ImageChops, ImageColor, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('sokoban_icon_generator', ROOT / 'tools/make_icons.py')
generator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(generator)


def bounds_against(image, background):
    return ImageChops.difference(image, Image.new('RGB', image.size, background)).getbbox()


class IconTests(unittest.TestCase):
    def test_native_canvas_safe_area_and_deterministic_assets(self):
        for variant in ('uns', 'sel'):
            with self.subTest(variant=variant), Image.open(ROOT / f'assets/icon-{variant}.png') as source:
                after = source.copy()
                self.assertEqual(after.mode, 'RGB')
                self.assertEqual(after.size, (92, 64))
                self.assertIsNone(ImageChops.difference(after, generator.draw_icon(variant == 'sel')).getbbox())
                bg = after.getpixel((0, 0))
                self.assertEqual(bounds_against(after, bg), (6, 2, 86, 42))
                for y in (*range(2), *range(42, 64)):
                    self.assertTrue(all(after.getpixel((x, y)) == bg for x in range(92)))
                allowed = {ImageColor.getrgb(c) for c in generator.PALETTE.values()}
                allowed.add(bg)
                self.assertTrue({color for _, color in after.getcolors()} <= allowed)

    def test_wall_and_crate_have_equal_real_outer_footprints(self):
        floor = ImageColor.getrgb(generator.PALETTE['floor'])
        for symbol in 'WC':
            tile = generator.draw_tile(symbol)
            self.assertEqual(tile.size, (10, 10))
            self.assertEqual(bounds_against(tile, floor), (0, 0, 10, 10))
            self.assertTrue(all(tile.getpixel((x, y)) != floor for x in range(10) for y in range(10)))
        crate = generator.draw_tile('C')
        orange = ImageColor.getrgb(generator.PALETTE['crate'])
        pixels = [(x, y) for y in range(10) for x in range(10) if crate.getpixel((x, y)) == orange]
        self.assertEqual((min(x for x,y in pixels), min(y for x,y in pixels),
                          max(x for x,y in pixels), max(y for x,y in pixels)), (1, 1, 8, 8))

    def test_every_object_uses_same_integer_grid(self):
        icon = generator.draw_icon()
        self.assertEqual(len(generator.SCENE), 4)
        for row, cells in enumerate(generator.SCENE):
            self.assertEqual(len(cells), 8)
            for column, symbol in enumerate(cells):
                left, top, right, bottom = generator.cell_bounds(column, row)
                self.assertEqual((left, top), (6 + 10*column, 2 + 10*row))
                self.assertEqual((right-left+1, bottom-top+1), (10, 10))
                crop = icon.crop((left, top, right+1, bottom+1))
                self.assertIsNone(ImageChops.difference(crop, generator.draw_tile(symbol)).getbbox())

    def test_player_crate_goal_relationship_and_legibility(self):
        positions = {s: [(c,r) for r,line in enumerate(generator.SCENE) for c,t in enumerate(line) if t==s]
                     for s in 'PCT'}
        self.assertEqual(positions, {'P': [(2,2)], 'C': [(3,2)], 'T': [(4,2)]})
        floor = ImageColor.getrgb(generator.PALETTE['floor'])
        player, goal = generator.draw_tile('P'), generator.draw_tile('T')
        self.assertEqual(bounds_against(player, floor), (2,1,9,9))
        self.assertEqual(bounds_against(goal, floor), (3,3,8,8))
        self.assertEqual(player.getpixel((5,2)), (0,0,0))
        self.assertEqual(goal.getpixel((5,5)), floor)

    def test_selected_variant_preserves_all_artwork(self):
        normal, selected = generator.draw_icon(), generator.draw_icon(True)
        self.assertNotEqual(normal.getpixel((0,0)), selected.getpixel((0,0)))
        self.assertIsNone(ImageChops.difference(normal.crop((6,2,86,42)),
                                              selected.crop((6,2,86,42))).getbbox())

    def test_generator_does_not_bake_text(self):
        with patch.object(ImageDraw.ImageDraw, 'text', side_effect=AssertionError('icon text forbidden')), \
             patch.object(ImageDraw.ImageDraw, 'multiline_text', side_effect=AssertionError('icon text forbidden')):
            generator.draw_icon()
            generator.draw_icon(True)

    def test_committed_numeric_audit(self):
        report = json.loads((ROOT / 'docs/ICON_AUDIT.json').read_text())
        self.assertEqual(report['logical_tile'], [10,10])
        self.assertEqual(report['wall_footprint'], report['crate_footprint'])
        for variant in ('uns', 'sel'):
            measured = report['variants'][variant]
            self.assertEqual(measured['before']['artwork_bounds_inclusive'], [8,1,83,46])
            self.assertEqual(measured['after']['artwork_bounds_inclusive'], [6,2,85,41])
            self.assertEqual(measured['after']['top_margin'], 2)
            self.assertEqual(measured['after']['bottom_margin'], 22)

    def test_both_native_rgb565_records_match_png(self):
        package = ROOT / 'dist/SOKOBAN.g3a'
        if not package.exists():
            self.skipTest('native package not built in this source checkout')
        raw = package.read_bytes()
        for variant, offset in (('uns', 0x1000), ('sel', 0x4000)):
            with self.subTest(variant=variant), Image.open(ROOT / f'assets/icon-{variant}.png') as source:
                rgb = source.convert('RGB')
                expected = b''.join(struct.pack('>H', ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
                                    for r,g,b in (rgb.getpixel((x,y)) for y in range(rgb.height) for x in range(rgb.width)))
                self.assertEqual(raw[offset:offset+len(expected)], expected)


if __name__ == '__main__':
    unittest.main()
