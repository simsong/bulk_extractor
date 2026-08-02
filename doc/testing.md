# Test-only controls

`BE_TEST_CRASH_AFTER_WORK_START=N` is an integration-test hook. It makes
bulk_extractor terminate with exit status 86 immediately after it flushes the
N-th top-level `debug:work_start` record. `N` must be a positive integer.

The hook exists to test the interrupted-report restart path. It is not a
recovery mechanism and must not be set for ordinary processing. The restart
self-test uses a child process so that the abrupt termination and flushed
`report.xml` are exercised as they are in a real interrupted run.
