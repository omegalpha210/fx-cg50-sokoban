"""All-map geometry contract and agreement with committed audit evidence."""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("sokoban_layout_audit", ROOT / "tools/layout_audit.py")
audit = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(audit)


class LayoutTests(unittest.TestCase):
    def test_all_maps_fit_square_centered_and_do_not_regress(self):
        report = audit.audit()
        self.assertEqual(report["level_count"], 60)
        self.assertEqual(report["header_pixels_reclaimed"], 24)
        self.assertEqual(report["minimum_before"], 8)
        self.assertEqual(report["minimum_after"], 9)
        for row in report["levels"]:
            with self.subTest(level=row["level"]):
                result = row["after"]
                self.assertGreaterEqual(result["tile"], row["before"]["tile"])
                self.assertEqual(result["width"], row["map_width"] * result["tile"])
                self.assertEqual(result["height"], row["map_height"] * result["tile"])
                self.assertGreaterEqual(result["x"], 124)
                self.assertGreaterEqual(result["y"], 4)
                self.assertLessEqual(result["x"] + result["width"], 392)
                self.assertLessEqual(result["y"] + result["height"], 200)
                self.assertLessEqual(abs((result["x"] - 124) - (392 - result["x"] - result["width"])), 1)
                self.assertLessEqual(abs((result["y"] - 4) - (200 - result["y"] - result["height"])), 1)
                self.assertTrue((result["tile"] + 1) * row["map_width"] > 268 or
                                (result["tile"] + 1) * row["map_height"] > 196)

    def test_audit_evidence_matches_current_sources(self):
        report = audit.audit()
        saved = json.loads((ROOT / "docs/validation/layout-audit.json").read_text())
        self.assertEqual(saved, report)
        self.assertEqual((ROOT / "docs/LAYOUT_AUDIT.md").read_text(), audit.markdown(report))

    def test_bottleneck_ties_are_not_double_counted(self):
        report = audit.audit()
        self.assertEqual(sum(report["bottlenecks_after"].values()), 60)
        largest = report["levels"][58]
        self.assertEqual((largest["before"]["tile"], largest["after"]["tile"]), (8, 9))
        self.assertEqual(largest["after"]["bottleneck"], "tie")


if __name__ == "__main__":
    unittest.main()
