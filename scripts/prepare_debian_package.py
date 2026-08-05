#!/usr/bin/env python3
"""Generate Debian changelog metadata from configure.ac."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
match = re.search(r"AC_INIT\(\[BULK_EXTRACTOR\],\[([^]]+)\]", (root / "configure.ac").read_text())
if match is None:
    raise SystemExit("cannot read version from configure.ac")
version = match.group(1).replace("-DEVELOP", "~develop") + "-1"
(root / "debian/changelog").write_text(
    f"bulk-extractor ({version}) UNRELEASED; urgency=medium\n\n"
    "  * Build package from the versioned upstream source.\n\n"
    " -- bulk_extractor maintainers <bugs@digitalcorpora.org>  Thu, 01 Jan 1970 00:00:00 +0000\n")
