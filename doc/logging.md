# Diagnostic logging

bulk_extractor writes durable diagnostic records to `bulk_extractor.log` in the
output directory. This file is intended for operational diagnostics; it does
not replace the structured DFXML report or contain extracted feature contents.

Use `--log-level` to select `trace`, `debug`, `info`, `warning`, `error`,
`critical`, or `off`. The command-line value takes precedence over `LOG_LEVEL`.
Without either, `-d` selects `debug`; the default is `info`.

Use `--log-file PATH` to choose another destination. If it cannot be opened,
bulk_extractor reports the error and exits rather than silently discarding
diagnostics. Warning and error records flush immediately; normal shutdown
flushes the file before releasing the logger.

The initial implementation provides the logging interface and mirrors
`scanner_set::log()` to DFXML and the diagnostic file. Migration of individual
diagnostic call sites is intentionally incremental; progress/UI output remains
on its current stream.

The implementation vendors header-only spdlog 1.16.0 (including bundled fmt
12.0.0) under `src/third_party/spdlog`; its upstream MIT license is retained.
