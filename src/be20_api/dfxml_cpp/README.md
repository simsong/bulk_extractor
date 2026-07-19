# DFXML C++ component

Digital Forensics XML (DFXML) records metadata and provenance for forensic
tools. bulk_extractor uses the C++17 reader, writer, hashing, and execution
environment support in this directory.

This source is vendored inside bulk_extractor and is not a Git submodule or a
separately built shared library. Its upstream revision and license provenance
are recorded in `../VENDORED.md` and `COPYING.md`. The parent build selects the
required headers through `src/Makefile.defs` and runs `test_dfxml` as part of
`make check`.

The DFXML schemas used for distribution and validation are vendored at the
bulk_extractor repository's top-level `dfxml_schema/` directory.
