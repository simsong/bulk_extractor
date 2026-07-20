# be20 internal component

This directory contains the scanner framework used by bulk_extractor 2.x. It is maintained and released as part of the bulk_extractor source tree, not as a standalone library or Git submodule. See `VENDORED.md` for the imported revisions and upstream provenance.

The component provides:

- `sbuf_t` byte buffers, forensic positions, and slice handling;
- scanner registration, lifecycle, scheduling, and recursive processing;
- feature recorders, histograms, and output coordination;
- path printing, packet adapters, regular expressions, Unicode helpers, and utilities; and
- the integrated dfxml_cpp and utfcpp dependencies.

## Build integration

`src/Makefile.am` includes `be20_api/Makefile.defs`, builds the production sources once as the internal `libbe20.a` convenience library, and links both bulk_extractor and its tests against that library. `Makefile.defs` is the authoritative production/test source manifest; files are never added to the executable merely because they appear in this directory.

Build and run all tests from the bulk_extractor repository root:

```sh
./bootstrap.sh
./configure
make
make check
```

The `test_be20_api` executable is a parent Automake test target. The former standalone configure, bootstrap, CI, and submodule workflows are intentionally unsupported.

## API status

The API is internal. There is no installed library, public-header compatibility promise, or runtime scanner plug-in ABI. Scanners compiled into bulk_extractor use `scanner_params`, `scanner_set`, `feature_recorder`, and `sbuf_t` directly.

## Path printing

Path printing uses scanners to decode a forensic path:

| Path | Action |
|---|---|
| `0-PRINT` | Print the contents at location zero. |
| `0-PRINT/r` | Print the length, CRLF, and raw contents. |
| `0-PRINT/h` | Hex-dump the contents. |
