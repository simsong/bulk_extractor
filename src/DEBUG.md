|Debug variables|Meaning|
|---------------|-------|
`DEBUG_DISABLE_FEATURE_RECORDER` | comma-separated list of feature recorders to disable. Specify `all` to disable all feature recorders
`DEBUG_DISABLE_DFXML` | Do not write DFXML
`DEBUG_DISABLE_TIMERS` | Do not run any aftimer (for determining overhead of running the timers.)
`DEBUG_PDF_DUMP_HEX`   | Hex dump all decompressed PDF runs to stderr
`DEBUG_PDF_DUMP_TEXT`  | Dump all extracted PDF text to stderr
`DEBUG_PRINT_STEPS`| print each scanner as it starts
`DEBUG_SCANNER`| dump feature writes to stderr
`DEBUG_SCANNER_DUMP_DATA`| dump scanner input data
`DEBUG_BENCHMARK`| collect scanner timing diagnostics
`DEBUG_SCANNERS_SAME_THREAD`| run scanner callbacks in one thread
`DEBUG_SBUF_GC`| log sbuf garbage-collection activity
`DEBUG_SBUF_GC0`| log initial sbuf garbage-collection activity
`DEBUG_FS_PEDANTIC`| enable feature-recorder consistency checks

Use `-d` (or `--debug`) to set diagnostic logging to the `debug` level. Use
`--log-level` to select another diagnostic level. Scanner selection is
independent: use `-x all` to disable every scanner, followed by `-e NAME` to
enable selected scanners.
