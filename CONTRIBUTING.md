# Contributing to bulk_extractor

Thank you for helping improve bulk_extractor. Contributions that improve
forensic correctness, reproducibility, portability, performance, documentation,
or test coverage are especially useful.

## Before opening an issue

Search existing issues and Discussions first. Use an issue for a reproducible
defect or a bounded, actionable proposal. Use a
[Discussion](https://github.com/simsong/bulk_extractor/discussions) for support,
usage questions, or early design exploration.

Do not upload forensic images, extracted evidence, credentials, personal data,
or other material you are not authorized to disclose. A minimized synthetic
fixture, a redacted excerpt, a cryptographic hash, and exact reproduction steps
are usually enough to begin investigating.

For a bug report, include the version or commit, operating system and compiler,
the complete command line, relevant configuration output, expected and actual
results, and the smallest safe reproducer. Reports about format recognition or
carving should also state how the expected result was independently verified.

## Development workflow

1. Start from current `main` and create a focused branch.
2. Discuss substantial changes in an issue or Discussion before investing in a
   large implementation.
3. Keep each pull request focused, with documentation updated when behavior,
   build requirements, or output changes.
4. Use C++20 and follow the style of the code you change. Avoid unrelated
   formatting churn.
5. Run the relevant checks through the Makefile. For normal source changes,
   run `make check`; use `make distcheck` when changing build or distribution
   files. State the exact validation and its result in the pull request.

Before changing a scanner or adding a scanner plug-in, read
[the scanner API](doc/scanner_api.md). Start loadable scanners from
[the scanner template](doc/scanner_template.cpp), preserve its phase and
concurrency rules, and add focused tests that exercise the changed behavior.

## Tests and fixtures

Tests must assert a meaningful forensic or program behavior. Do not add tests
solely to raise coverage. Keep fixtures small, deterministic, redistributable,
and free of sensitive data. When a change fixes a false positive, false
negative, offset, recursion, or format-handling bug, include a fixture that
would fail before the fix when practical.

## Pull requests and review

By submitting a contribution, you confirm that you have the right to submit it
under the repository's license. Do not include copied code or test data unless
its license and provenance allow distribution here.

Maintainers may ask for narrower scope, tests, documentation, benchmarks, or
changes before merging. Review focuses on correctness, safety, forensic
reproducibility, portability, maintainability, and performance. Please keep
discussion technical and follow the [Code of Conduct](CODE_OF_CONDUCT.md).
