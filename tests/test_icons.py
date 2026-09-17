"""Pixel-preserving icon translation, label-safe margins, native package pixels."""
import importlib.util
import json
from pathlib import Path
import struct
import unittest
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('sokoban_icon_generator', ROOT / 'tools/make_icons.py')
generator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(generator)


class IconTests(unittest.TestCase):
    def test_original_artwork_survives_exact_translation(self):
        for variant in ('uns', 'sel'):
            with self.subTest(variant=variant):
                with Image.open(ROOT / f'docs/public-captures/icon-{variant}-before.png') as source:
                    before = source.copy()
                with Image.open(ROOT / f'assets/icon-{variant}.png') as source:
                    after = source.copy()
                self.assertEqual(after.mode, 'RGB')
                self.assertEqual(after.size, (92, 64))
                self.assertIsNone(ImageChops.difference(before, generator.draw_icon(variant == 'sel', 0)).getbbox())
                self.assertIsNone(ImageChops.difference(after, generator.draw_icon(variant == 'sel')).getbbox())
                bg = before.getpixel((0, 0))
                original = {(x, y): before.getpixel((x, y)) for y in range(64) for x in range(92)
                            if before.getpixel((x, y)) != bg}
                shifted = {(x, y): after.getpixel((x, y)) for y in range(64) for x in range(92)
                           if after.getpixel((x, y)) != bg}
                self.assertEqual(shifted, {(x, y-3): pixel for (x, y), pixel in original.items()})
                self.assertEqual(ImageChops.difference(after, Image.new('RGB', after.size, bg)).getbbox(),
                                 (8, 1, 84, 47))
                for y in (0, *range(47, 64)):
                    self.assertTrue(all(after.getpixel((x, y)) == bg for x in range(92)))

    def test_committed_numeric_audit(self):
        report = json.loads((ROOT / 'docs/ICON_AUDIT.json').read_text())
        for variant in ('uns', 'sel'):
            measured = report['variants'][variant]
            self.assertEqual(measured['before']['artwork_bounds_inclusive'], [8, 4, 83, 49])
            self.assertEqual(measured['after']['artwork_bounds_inclusive'], [8, 1, 83, 46])
            self.assertEqual(measured['after']['top_margin'], 1)
            self.assertEqual(measured['after']['bottom_margin'], 17)

    def test_both_native_rgb565_records_match_png(self):
        package = ROOT / 'dist/SOKOBAN.g3a'
        if not package.exists():
            self.skipTest('native package not built in this source checkout')
        raw = package.read_bytes()
        for variant, offset in (('uns', 0x1000), ('sel', 0x4000)):
            with self.subTest(variant=variant):
                with Image.open(ROOT / f'assets/icon-{variant}.png') as source:
                    rgb = source.convert('RGB')
                    expected = b''.join(struct.pack('>H', ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
                                        for r, g, b in (rgb.getpixel((x,y)) for y in range(rgb.height) for x in range(rgb.width)))
                self.assertEqual(raw[offset:offset+len(expected)], expected)


if __name__ == '__main__':
    unittest.main()
