#!/usr/bin/env python3
"""Focused regression tests for version-aware bulk_diff escape comparison."""

import io
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "python"))
import bulk_diff
import bulk_extractor_reader


def write_report(directory, version, value):
    pathlib.Path(directory, "report.xml").write_text(
        "<dfxml><version>{}</version></dfxml>".format(version), encoding="utf-8")
    pathlib.Path(directory, "email_histogram.txt").write_bytes(b"n=1\t" + value + b"\n")
    pathlib.Path(directory, "email.txt").write_bytes(b"0\t" + value + b"\tcontext\n")


class BulkDiffEscapeTest(unittest.TestCase):
    def test_legacy_hex_matches_current_octal_only_in_auto_mode(self):
        with tempfile.TemporaryDirectory() as root:
            legacy = pathlib.Path(root, "legacy")
            current = pathlib.Path(root, "current")
            legacy.mkdir()
            current.mkdir()
            write_report(legacy, "1.6.0", b"bad\\xff")
            write_report(current, "2.2.0", b"bad\\377")

            auto_output = io.StringIO()
            automatic = bulk_diff.BulkDiff(str(legacy), str(current), out=auto_output,
                                           both=True, escape_mode="auto")
            self.assertEqual(automatic.normalize(automatic.b1, b"bad\\xff"), b"bad\\377")
            self.assertEqual(automatic.normalize(automatic.b2, b"bad\\377"), b"bad\\377")
            self.assertEqual(bulk_extractor_reader.legacy_hex_to_octal(b"bad\\xqf"), b"bad\\xqf")
            automatic.compare_histograms()
            automatic.compare_features()
            self.assertIn("No differences", auto_output.getvalue())
            self.assertNotIn("differs", auto_output.getvalue())

            raw_output = io.StringIO()
            raw = bulk_diff.BulkDiff(str(legacy), str(current), out=raw_output,
                                     both=True, escape_mode="raw")
            self.assertNotEqual(raw.normalize(raw.b1, b"bad\\xff"), raw.normalize(raw.b2, b"bad\\377"))
            raw.compare_histograms()
            raw.compare_features()
            self.assertNotIn("No differences", raw_output.getvalue())
            self.assertIn("differs", raw_output.getvalue())


if __name__ == "__main__":
    unittest.main()
