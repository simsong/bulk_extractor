#!/usr/bin/env python3
"""Focused regression tests for version-aware bulk_diff escape comparison."""

import io
import pathlib
import sys

import pytest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "python"))
import bulk_diff
import bulk_extractor_reader


def write_report(directory, version, value):
    pathlib.Path(directory, "report.xml").write_text(
        "<dfxml><version>{}</version></dfxml>".format(version), encoding="utf-8")
    pathlib.Path(directory, "email_histogram.txt").write_bytes(b"n=1\t" + value + b"\n")
    pathlib.Path(directory, "email.txt").write_bytes(b"0\t" + value + b"\tcontext\n")


def test_legacy_hex_matches_current_octal_only_in_auto_mode(tmp_path):
    legacy = tmp_path / "legacy"
    current = tmp_path / "current"
    legacy.mkdir()
    current.mkdir()
    write_report(legacy, "1.6.0", b"bad\\xff")
    write_report(current, "2.2.0", b"bad\\377")

    auto_output = io.StringIO()
    automatic = bulk_diff.BulkDiff(
        str(legacy), str(current), out=auto_output, both=True, escape_mode="auto"
    )
    assert automatic.normalize(automatic.b1, b"bad\\xff") == b"bad\\377"
    assert automatic.normalize(automatic.b2, b"bad\\377") == b"bad\\377"
    assert bulk_extractor_reader.legacy_hex_to_octal(b"bad\\xqf") == b"bad\\xqf"
    automatic.compare_histograms()
    automatic.compare_features()
    assert "No differences" in auto_output.getvalue()
    assert "differs" not in auto_output.getvalue()

    raw_output = io.StringIO()
    raw = bulk_diff.BulkDiff(
        str(legacy), str(current), out=raw_output, both=True, escape_mode="raw"
    )
    assert raw.normalize(raw.b1, b"bad\\xff") != raw.normalize(raw.b2, b"bad\\377")
    raw.compare_histograms()
    raw.compare_features()
    assert "No differences" not in raw_output.getvalue()
    assert "differs" in raw_output.getvalue()


def test_histogram_normalization_aggregates_colliding_keys(tmp_path):
    legacy = tmp_path / "legacy"
    current = tmp_path / "current"
    legacy.mkdir()
    current.mkdir()
    write_report(legacy, "1.6.0", b"placeholder")
    write_report(current, "2.2.0", b"placeholder")
    (legacy / "email_histogram.txt").write_bytes(
        b"n=2\tbad\\xff\n"
        b"n=3\tbad\\377\n"
    )
    (current / "email_histogram.txt").write_bytes(b"n=5\tbad\\377\n")

    output = io.StringIO()
    comparison = bulk_diff.BulkDiff(
        str(legacy), str(current), out=output, both=True, escape_mode="auto"
    )
    comparison.compare_histograms()
    assert "No differences" in output.getvalue()


def test_unparseable_version_names_the_report(tmp_path):
    report = tmp_path / "unversioned"
    report.mkdir()
    write_report(report, "unknown", b"placeholder")
    with pytest.raises(ValueError, match=str(report)):
        bulk_extractor_reader.BulkReport(str(report)).escape_format()
