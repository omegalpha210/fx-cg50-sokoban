"""Pinned map integrity, format rejection, and terrain conversion checks."""

import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("sokoban_import_maps", ROOT / "tools/import_maps.py")
assert SPEC and SPEC.loader
importer = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = importer
SPEC.loader.exec_module(importer)


def fixture(rows=None, width=None, level=1):
    if rows is None:
        rows = ["  XXXXX  ", "  X@ &X  ", "  XXXXX  "]
    if width is None:
        width = max(map(len, rows))
    return (f"{importer.SEPARATOR}\nMaze: {level}\n"
            "File offset: 148C, DS:00FC, table offset: 0000\n"
            f"Size X: {width}\nSize Y: {len(rows)}\nEnd: 14BD\nLength: 50\n\n"
            + "\n".join(rows) + f"\n\n{importer.SEPARATOR}\n")


class ImporterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw = (ROOT / "assets/maps/upstream/sokoban-maps-60-plain.txt").read_bytes()
        cls.maps = importer.parse_maps(cls.raw.decode("ascii"))

    def parse_one(self, text):
        return importer.parse_maps(text, expected_levels=1)[0]

    def reject(self, text, pattern):
        with self.assertRaisesRegex(importer.MapError, pattern):
            self.parse_one(text)

    def test_all_60_order_and_counts(self):
        self.assertEqual([m.level for m in self.maps], list(range(1, 61)))
        for m in self.maps:
            with self.subTest(level=m.level):
                self.assertEqual(len(m.rows), m.height)
                self.assertEqual(len(m.terrain), m.width * m.height)
                self.assertEqual(len(m.crates), m.terrain.count(importer.GOAL))
                self.assertGreater(len(m.crates), 0)
                self.assertEqual("".join(m.rows).count("@"), 1)
                self.assertIn(m.terrain[m.player], (importer.FLOOR, importer.GOAL))
                self.assertEqual(len(m.crates), len(set(m.crates)))

    def test_largest_map_and_maximum_crates(self):
        m = self.maps[58]
        self.assertEqual((m.width, m.height, len(m.terrain), len(m.crates)), (29, 20, 580, 36))
        self.assertEqual(max(m.width for m in self.maps), 29)
        self.assertEqual(max(m.height for m in self.maps), 20)
        self.assertEqual(max(len(m.crates) for m in self.maps), 36)

    def test_exact_source_hash_and_manifest(self):
        manifest = json.loads((ROOT / "assets/maps/manifest.json").read_text())
        importer.verify_manifest(manifest, ROOT / "assets/maps/upstream")
        self.assertEqual(hashlib.sha256(self.raw).hexdigest(),
                         "9e4814fc9172a8aa0dcc0d049017f62d640a65c193d62dea1602ce05bcbd6f68")

    def test_changed_original_rejected(self):
        with tempfile.TemporaryDirectory(dir=ROOT, prefix=".map-test-") as temp:
            p = Path(temp)
            (p / "source.txt").write_bytes(b"changed")
            manifest = {"files": [{"path": "source.txt", "bytes": 7, "sha256": "0" * 64}]}
            with self.assertRaisesRegex(importer.MapError, "pinned upstream file changed"):
                importer.verify_manifest(manifest, p)

    def test_ampersand_is_both_crate_and_goal(self):
        m = self.parse_one(fixture(["XXXXXX", "X@*& X", "X .  X", "XXXXXX"]))
        self.assertEqual(m.initial_on_goals, 1)
        self.assertEqual(len(m.crates), 2)
        self.assertEqual(m.terrain[8], importer.FLOOR)  # '*'
        self.assertEqual(m.terrain[9], importer.GOAL)   # '&'
        self.assertIn(9, m.crates)
        self.assertEqual(m.terrain.count(importer.GOAL), 2)

    def test_leading_spaces_exterior_interior_and_padding(self):
        rows = ["  XXXXX", "  X@ &X", "  XXXXX"]
        m = self.parse_one(fixture(rows, width=9))
        self.assertEqual(m.rows, tuple(rows))
        self.assertEqual(m.player, 12)
        self.assertEqual(m.terrain[13], importer.FLOOR)
        self.assertEqual(m.padding_cells, 6)
        for y in range(3):
            for x in (0, 1, 7, 8):
                self.assertEqual(m.terrain[y * 9 + x], importer.VOID)

    def test_trailing_spaces_retained(self):
        m = self.parse_one(fixture())
        self.assertEqual(m.rows[0], "  XXXXX  ")
        self.assertEqual(m.padding_cells, 0)
        self.assertEqual(m.metadata_lines[2], "Size X: 9")

    def test_source_symbols_preserved(self):
        symbols = set()
        for m in self.maps:
            raw = "".join(row.ljust(m.width) for row in m.rows)
            symbols.update(raw)
            for i, c in enumerate(raw):
                if c == "X":
                    self.assertEqual(m.terrain[i], importer.WALL)
                elif c in ".&":
                    self.assertEqual(m.terrain[i], importer.GOAL)
                if c in "*&":
                    self.assertIn(i, m.crates)
            self.assertEqual(m.initial_on_goals, raw.count("&"))
        self.assertEqual(symbols, set("X *.&@"))

    def test_all_pack_round_trips(self):
        for m in self.maps:
            packed = importer.pack_terrain(m.terrain)
            decoded = tuple((packed[i // 4] >> (2 * (i % 4))) & 3 for i in range(len(m.terrain)))
            self.assertEqual(decoded, m.terrain)
            for i in range(len(m.terrain), len(packed) * 4):
                self.assertEqual((packed[i // 4] >> (2 * (i % 4))) & 3, 0)

    def test_generated_files_match(self):
        subprocess.run([sys.executable, str(ROOT / "tools/import_maps.py"), "--check"], cwd=ROOT, check=True)

    def test_unknown_symbol_reports_coordinate(self):
        self.reject(fixture().replace("X@ &X", "X@ $X"), "level 1, column 6, row 2: unknown symbol")

    def test_other_sokoban_format_rejected(self):
        for symbol in ("#", "$", "+", "\t", "é"):
            with self.subTest(symbol=symbol):
                self.reject(fixture().replace("X@ &X", "X@ " + symbol + "X"), "unknown symbol")

    def test_player_count(self):
        self.reject(fixture().replace("@", " "), "exactly one player, found 0")
        self.reject(fixture().replace("X@ &X", "X@@&X"), "exactly one player, found 2")

    def test_crate_goal_mismatch(self):
        self.reject(fixture().replace("&", "*"), "crates 1 and goals 0")
        self.reject(fixture().replace("&", "."), "crates 0 and goals 1")
        self.reject(fixture().replace("&", " "), "equal and positive")

    def test_missing_or_duplicate_or_reordered_level(self):
        self.reject(fixture(level=2), "expected ID 1")
        duplicated = fixture().rstrip() + "\n" + fixture().split("\n", 1)[1]
        with self.assertRaisesRegex(importer.MapError, "expected ID 2"):
            importer.parse_maps(duplicated, expected_levels=2)
        with self.assertRaisesRegex(importer.MapError, "expected 60 levels, found 1"):
            importer.parse_maps(fixture())

    def test_malformed_metadata_and_offsets(self):
        self.reject(fixture().replace("Size X:", "Width:"), "malformed metadata")
        self.reject(fixture().replace("Length: 50", "Length: 51"), "Length mismatch")
        self.reject(fixture().replace("DS:00FC", "DS:00FD"), "offset mismatch")

    def test_dimension_mismatch(self):
        self.reject(fixture().replace("Size X: 9", "Size X: 8"), "width 9 exceeds")
        self.reject(fixture().replace("Size Y: 3", "Size Y: 4"), "actual map rows 3")
        self.reject(fixture().replace("Size Y: 3", "Size Y: 25"), "format bounds")
        self.reject(fixture().replace("Size X: 9", "Size X: 0"), "format bounds")

    def test_exposed_occupant_rejected(self):
        self.reject(fixture(["  XXXXX  ", "   @ &X  ", "  XXXXX  "]), "connected to exterior")

    def test_goal_on_boundary_rejected(self):
        self.reject(fixture(["  XXX&X  ", "  X@  X  ", "  XXXXX  "]), "connected to exterior")

    def test_missing_end_separator_and_extra_text_rejected(self):
        self.reject(fixture().removesuffix(importer.SEPARATOR + "\n"), "separator")
        self.reject("junk\n" + fixture(), "extra text")
        self.reject(fixture() + "junk\n", "extra text")


if __name__ == "__main__":
    unittest.main()
