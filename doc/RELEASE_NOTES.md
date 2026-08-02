# bulk_extractor release notes

This file consolidates release highlights that were previously spread among
`ChangeLog`, version roadmaps, Git tags, and announcements in
[`doc/announce`](announce/). The older sections are summaries of those sources,
not exhaustive changelogs. Some legacy announcement dates disagree with tag
dates; the version history below therefore uses dates only where the repository
history is clear.

## 2.2.0 (draft)

**Status:** Unreleased. The source version is currently
`2.2.0-DEVELOP`.

### Release theme

Version 2.2.0 is a reliability and maintainability release. A concentrated
source, build, CI, documentation, and test audit found defects in core buffer
handling, hostile-input parsing, shutdown, scanner selection, and feature
recording. The resulting fixes substantially improve reliability, although they
do not establish that every malformed-input defect has been found.

Much of this reliability campaign was performed with Codex: Codex analyzed the
codebase, converted findings into tracked changes, implemented focused tests and
fixes, and prepared documentation. The changes were reviewed and validated
through the project's normal pull-request and CI process.

### Highlights

- A standalone 64-bit Windows `.exe` is built with MinGW on Ubuntu and is
  checked to ensure that it imports no non-system Windows DLLs. GitHub Actions
  publishes it as a downloadable artifact; attaching it to the 2.2.0 release
  remains a release task
  ([PR #543](https://github.com/simsong/bulk_extractor/pull/543)).
- Windows raw-device input now uses explicit Win32 device handles and
  `IOCTL_DISK_GET_LENGTH_INFO` for physical disks, volumes, and named volumes,
  rather than relying on C++ filesystem metadata or calculated disk geometry
  ([issue #258](https://github.com/simsong/bulk_extractor/issues/258)).
- The source tree is self-contained: `be20_api`, DFXML, schemas, and UTF support
  are now versioned in this repository instead of being supplied through fragile
  Git submodules
  ([PR #498](https://github.com/simsong/bulk_extractor/pull/498)).
- The new VIN scanner extracts and validates vehicle identification numbers
  ([PR #494](https://github.com/simsong/bulk_extractor/pull/494)).
- Runtime scanner plug-ins are supported again through a versioned factory
  interface, `-P`, and `BE_PATH`, with an end-to-end integration test
  ([PR #528](https://github.com/simsong/bulk_extractor/pull/528)).
- Network captures now preserve IEEE 802.11 records with their correct PCAP
  link type and report WiFi frame metadata in `wifi.txt`
  ([PR #559](https://github.com/simsong/bulk_extractor/pull/559)).
- The carving guide documents recorder-specific `-S <recorder>_carve_mode=`
  settings, defaults, and feature-file behavior ([#264](https://github.com/simsong/bulk_extractor/issues/264)).

### Reliability and correctness

- Restart records the start of each page so a resumed run deliberately skips
  pages that were in progress at the crash, avoiding repeated data-dependent
  crashes; the behavior is covered by a controlled-crash regression test
  ([#202](https://github.com/simsong/bulk_extractor/issues/202)).
- Restart now reports the number of deliberately skipped pages and the archived
  interrupted report path when it resumes a non-quiet run
  ([#319](https://github.com/simsong/bulk_extractor/issues/319)).
- Removed the obsolete numeric debug mask and its misleading scanner-control
  documentation. `-d`/`--debug` now explicitly enables debug-level diagnostic
  logging; scanner selection remains controlled by `-x` and `-e`
  ([#403](https://github.com/simsong/bulk_extractor/issues/403)).
- Removed the unregistered, API-stale Lightgrep Base16 implementation; the
  maintained Flex Base16 scanner remains the supported opt-in implementation
  ([#246](https://github.com/simsong/bulk_extractor/issues/246)).
- RAR extraction now preserves full seek offsets and bounded in-memory writes,
  recovering every member of a multi-file RAR fixture ([#212](https://github.com/simsong/bulk_extractor/issues/212)).
- Progress displays now adapt to Windows console-width changes, matching the
  periodic terminal-width refresh already used on Unix-like systems
  ([#311](https://github.com/simsong/bulk_extractor/issues/311)).
- Email extraction now enforces independent 64-octet local-part and 253-octet
  domain limits for ASCII and UTF-16 input, avoiding truncated suffix features
  from overlong addresses ([#585](https://github.com/simsong/bulk_extractor/issues/585)).
- Hardened the bundled RAR PPM decoder's dictionary-copy boundary handling,
  preventing a malformed archive from writing past its ring buffer
  (fixes CVE-2026-24857; [#601](https://github.com/simsong/bulk_extractor/issues/601)).
- Email extraction now accepts syntactically valid two-to-63-character TLDs,
  including current TLDs such as `.solutions`, without requiring a stale
  scanner-specific allow-list ([#586](https://github.com/simsong/bulk_extractor/issues/586)).
- Parser hardening now rejects truncated hibernation-file block headers before
  reading their length fields, and unexpected top-level exceptions are reported
  as diagnostics with a nonzero exit status
  ([PR #605](https://github.com/simsong/bulk_extractor/pull/605)).
- Hardened `sbuf` bounds, arithmetic, and ownership behavior, including
  zero-length and one-past-end cases
  ([PR #511](https://github.com/simsong/bulk_extractor/pull/511)).
- Corrected packet address and bounds parsing and bounded fallback PCAP reads for
  malformed or truncated packets
  ([PR #516](https://github.com/simsong/bulk_extractor/pull/516),
  [PR #530](https://github.com/simsong/bulk_extractor/pull/530)).
- Restored IPv6 TCP, UDP, and ICMPv6 checksum validation, safe IPv6 packet
  bounds handling, and correct raw-IP carving
  ([PR #554](https://github.com/simsong/bulk_extractor/pull/554)).
- Made E01 and split-image selection safe for literal percent characters,
  lowercase segment names, and exceptional paths; raw and EWF short reads no
  longer expose unread buffer tails to scanners
  ([PR #517](https://github.com/simsong/bulk_extractor/pull/517),
  [PR #518](https://github.com/simsong/bulk_extractor/pull/518)).
- Made mapped-file and disk-error cleanup exception-safe
  ([PR #519](https://github.com/simsong/bulk_extractor/pull/519),
  [PR #524](https://github.com/simsong/bulk_extractor/pull/524)).
- Fixed notifier and disk-write error shutdown so worker failures are reported
  and cleaned up instead of hanging or terminating incorrectly
  ([PR #513](https://github.com/simsong/bulk_extractor/pull/513)).
- `-Z` now removes stale nested output directories as well as files before a
  new run ([#239](https://github.com/simsong/bulk_extractor/issues/239)).
- Fixed scanner controls: `jpeg_carve_mode=0` now disables JPEG carving, and
  `-x all -e outlook` enables Outlook as requested
  ([PR #525](https://github.com/simsong/bulk_extractor/pull/525),
  [PR #527](https://github.com/simsong/bulk_extractor/pull/527)).
- JPEG carving now observes the recorder's configured minimum and maximum
  carve sizes; `jpeg_min_carve_size` and `jpeg_max_carve_size` provide
  scanner-specific overrides ([#242](https://github.com/simsong/bulk_extractor/issues/242)).
- Preserved recorder banners and triggering features across CRLF input,
  histogram setup, and allocation-failure paths
  ([PR #531](https://github.com/simsong/bulk_extractor/pull/531),
  [PR #533](https://github.com/simsong/bulk_extractor/pull/533),
  [PR #535](https://github.com/simsong/bulk_extractor/pull/535)).
- Bounded and normalized derived ZIP-carving filenames while retaining source
  metadata ([PR #539](https://github.com/simsong/bulk_extractor/pull/539)).
- ZIP component carvings now use the `zip_carved/` output directory and `zip_carved.txt` feature file, matching
  the convention used by other carvers ([#336](https://github.com/simsong/bulk_extractor/issues/336)).
- Prevented empty MSXML extraction from causing recursion and changed residual
  `sbuf` diagnostics from an abort to a DFXML warning
  ([PR #537](https://github.com/simsong/bulk_extractor/pull/537)).
- Applied `--max_minute_wait` to phase-1 work and shutdown, reporting timeout
  failures instead of blocking indefinitely
  ([PR #540](https://github.com/simsong/bulk_extractor/pull/540)).
- Made recursive input traversal deterministic and safe around symlinks and
  permission-denied paths, and proved the Windows path through the original
  Unicode-filename regression case
  ([PR #541](https://github.com/simsong/bulk_extractor/pull/541),
  [PR #549](https://github.com/simsong/bulk_extractor/pull/549)).
- Restored stop-list and alert-list processing, including the normal and
  diverted feature outputs, with CLI-level regression tests
  ([PR #552](https://github.com/simsong/bulk_extractor/pull/552),
  [PR #553](https://github.com/simsong/bulk_extractor/pull/553)).
- Added carving for validated RawTherapee `Image8` RGB thumbnail records,
  writing PPM output without an image-sized intermediate copy
  ([PR #556](https://github.com/simsong/bulk_extractor/pull/556)).
- URL feature extraction no longer includes surrounding HTML `&quot;` markup, and
  the utmp scanner recognizes both little- and big-endian Linux records
  ([PR #568](https://github.com/simsong/bulk_extractor/pull/568),
  [PR #563](https://github.com/simsong/bulk_extractor/pull/563)).
- Windows IP feature formatting now uses a Windows socket-address formatter and
  is exercised against the NTLM PCAP fixture in the Windows runtime workflow
  ([PR #573](https://github.com/simsong/bulk_extractor/pull/573),
  [PR #574](https://github.com/simsong/bulk_extractor/pull/574)).
- `--find-case-sensitive` makes `-f` and `-F` RE2 patterns case-sensitive;
  their historical case-insensitive matching remains the default
  ([#483](https://github.com/simsong/bulk_extractor/issues/483)).
- Fixed a SQLite-size arithmetic overflow and reduced the scheduled Coverity
  workflow token to read-only repository contents
  ([PR #538](https://github.com/simsong/bulk_extractor/pull/538)).
- `report.xml` now declares and conforms to the bundled DFXML 1.2.0 schema,
  whose XML Schema 1.0 content models are deterministic for current validators;
  bulk_extractor-specific runtime, configuration, source-detail, and final
  report data are preserved in a separate extension namespace and covered by
  an `xmllint --schema` regression test ([#244](https://github.com/simsong/bulk_extractor/issues/244)).

### Build, configuration, and testing

- A multi-stage Debian Bookworm container image provides a reproducible,
  unprivileged environment for scanning regular image files. It documents its
  libewf and Lightgrep limitations and supplements native platform CI rather
  than replacing it ([#159](https://github.com/simsong/bulk_extractor/issues/159)).
- Builds now enable basic compiler stack-canary protection (`-fstack-protector`)
  when both selected C and C++ compilers support it ([#376](https://github.com/simsong/bulk_extractor/issues/376)).
- AddressSanitizer now runs on every pull request while redundant workflow
  execution has been reduced
  ([PR #514](https://github.com/simsong/bulk_extractor/pull/514)).
- Optional Exiv2 configuration is honored, tested, and disabled by default; its
  version is recorded in DFXML when enabled
  ([PR #532](https://github.com/simsong/bulk_extractor/pull/532)).
- Scanner lifecycle rules and a loadable-scanner template are now documented
  ([PR #529](https://github.com/simsong/bulk_extractor/pull/529)).
- Focused regression tests now cover the repaired buffer, packet, short-read,
  shutdown, scanner-selection, plug-in, banner, and histogram contracts.
- The planned Windows workflow builds with MinGW on Ubuntu, builds static RE2
  and its dependencies, verifies DLL imports, and publishes
  `bulk_extractor64.exe` as a GitHub Actions artifact
  ([PR #543](https://github.com/simsong/bulk_extractor/pull/543)).
- Debian Bookworm now has an explicit compatibility build; optional-dependency
  tests correctly skip unavailable libewf and `xmllint` paths, while full test
  suites remain the responsibility of platform build jobs
  ([PR #565](https://github.com/simsong/bulk_extractor/pull/565),
  [PR #567](https://github.com/simsong/bulk_extractor/pull/567),
  [PR #572](https://github.com/simsong/bulk_extractor/pull/572),
  [PR #577](https://github.com/simsong/bulk_extractor/pull/577)).
- Builds configured without the project `-O3` optimization now say so at startup
  ([PR #564](https://github.com/simsong/bulk_extractor/pull/564)).

### Documentation and project maintenance

- Consolidated release history and this 2.2.0 draft in one versioned document
  ([PR #544](https://github.com/simsong/bulk_extractor/pull/544)).
- Moved the living technical-debt scoreboard to
  [Discussion #545](https://github.com/simsong/bulk_extractor/discussions/545),
  with GitHub issues remaining the actionable work records
  ([PR #546](https://github.com/simsong/bulk_extractor/pull/546)).
- Added code-of-conduct, contribution, issue, and pull-request guidance, and
  documented the required Codex GitHub identity
  ([PR #551](https://github.com/simsong/bulk_extractor/pull/551),
  [PR #542](https://github.com/simsong/bulk_extractor/pull/542)).
- Replaced the generic Autotools `INSTALL` template with project-specific
  release-archive and Git-checkout instructions, and expanded/published the
  current scanner-development manuals
  ([PR #561](https://github.com/simsong/bulk_extractor/pull/561),
  [PR #558](https://github.com/simsong/bulk_extractor/pull/558),
  [PR #557](https://github.com/simsong/bulk_extractor/pull/557)).

### Known limitations and release work

- Keep the Windows artifact workflow green and attach its `.exe` to the 2.2.0
  GitHub release. The current build disables libewf, so the `.exe` does not
  read E01 images directly.
- Lightgrep remains an unsupported, source-broken optional configuration and
  should not be represented as a working 2.2.0 feature.
- BEViewer is not bundled with bulk_extractor 2.
- The built-in RAR implementation supports RAR versions 1 through 3 and does not
  reliably handle every archive or every UTF-8 component name.
- The focused repairs and current corpus do not prove safety for every hostile
  or malformed input. Additional malformed-corpus, fuzz, and scanner-specific
  boundary testing remains useful.
- Replace this draft status with the final date, tag, commit, and release
  artifact/check summary when 2.2.0 is published.

For the detailed July 2026 audit and delivery record, see
[`doc/RECENT_WORK_REPORT.md`](RECENT_WORK_REPORT.md).

## 2.1.1 (2024-04-27)

This maintenance release repaired JPEG carving help and test behavior, removed
obsolete C++11 compiler flags, and maintained CI and Coverity configuration.

## 2.1.0 (2024-01-24)

This was the first bulk_extractor 2 release recommended for general use. The
major user-visible correction was replacing the C++ standard-library regular
expression engine with Google's RE2. RE2 avoids catastrophic backtracking, so
open-ended `-F` expressions such as `[a-z]*@company.com` no longer hang.

Version 2 also delivered substantially better multicore performance and
portability than version 1. BEViewer was not bundled; the Outlook and hibernation
scanners were disabled by default pending stronger tests; and 192-bit AES-key
scanning was no longer enabled by default.

See the original [2.1.0 announcement](announce/announce_2.1.0.md).

## 2.0 series (2022–2024, reconstructed)

### 2.0.0

Version 2 was a significant rewrite begun in 2016 to modernize the program for
current C++, improve multicore performance, make ownership and exception
handling safer, and establish continuous integration and focused unit testing.
It narrowed the distribution to the command-line program: BEViewer was no
longer bundled, AFF/AFF4 support and research-oriented scanners were removed,
and production-oriented defaults replaced the broader experimental posture of
version 1.

The rewrite also reorganized scanner and feature-recorder APIs, improved DFXML
reporting and testability, and introduced the initial version-2 E01 path. See
the contemporary [2.0 roadmap](ROADMAP_2.0.md) for the design goals; it is a
planning record rather than a final release announcement.

### Maintenance through 2.0.3

The early maintenance releases restored and expanded MinGW Windows
cross-compilation on Fedora, corrected Unix block-device sizing and `utmp`
parsing, repaired `--disable-rar`, documented bootstrap builds, and fixed source
distribution packaging involving submodules. CI permissions and dependencies,
including Flex, were also made more explicit.

### Maintenance after 2.0.3 through 2.0.6

These releases fixed an AES scanner buffer overrun, AddressSanitizer and CI
failures, scanner-selection ordering, ignored command-line controls, and build
artifact generation. They also improved Gentoo and libewf build support and
continued the Ubuntu MinGW build work.

The version-2 maintenance history is preserved in the repository
[`ChangeLog`](../ChangeLog) and the Git history for tags `v2.0.0` through
`v2.0.6`.

## Legacy 1.x releases (reconstructed)

### 1.6.0

The 1.6 line added and improved scanners for Windows forensic artifacts,
including EVTX, NTFS MFT, NTFS log file and index records, `utmp`, PE carving,
DLL-name extraction, and remote Windows shortcut fields. It also fixed a
wordlist scanner state-machine crash and improved BEViewer navigation, report
refresh, copying, hashdb context, and display-page size. Windows installer
placement for the 64-bit executable was corrected.

The surviving [1.6.0 announcement](announce/announce_1.6.0.md) is explicitly a
pre-release draft. This summary combines it with the repository `ChangeLog` and
tagged source rather than treating that draft as a final historical record.

### 1.5.x

Version 1.5 added optional SQLite feature output, an embeddable shared library
with a Python module, and in-memory histograms. New scanners covered Base64,
Facebook HTML, hashdb, HTTP logs, Outlook compressible encryption, Sceadan,
SQLite, Windows shortcuts, and optional Lightgrep variants.

Carving support was expanded and corrected for encoded JPEG, ZIP, RAR, and
SQLite content, including duplicate suppression. Scanner controls reduced
network-carving and other common false positives, while Base64 recovery and PII
recognition were expanded. Version 1.5.2 corrected Outlook Compressible
Encryption, added RFC 4648 Base64 handling, and introduced the MSXML scanner for
Microsoft Office Open XML documents.

See the original [1.5 announcement](announce/announce_1.5.md) and
[1.5.2 notes](announce/announce_1.5.2.md).

### 1.4.x

Version 1.4 introduced RAR archive/component processing, JPEG carving, ZIP
component carving, Lightgrep searching, block-hash scanning, XOR searching,
variable context windows, and random sampling. It improved performance by
letting scanners limit when and on which buffers they ran.

The release significantly reduced false positives in Windows directory,
network, ELF, PE, and Ethernet-address detection; increased recursive scan depth
from five to seven; improved PDF extraction; and standardized ZIP timestamps.
Carved files were split into bounded directories for manageability.

Command-line compatibility changed: block size and word-size controls moved from
`-B` and `-W` to `-S` parameters. The plug-in system was also substantially
refactored.

See the original [1.4 announcement](announce/announce_1.4.txt) and
[1.4.1 notes](announce/announce_1.4.1.txt).

## Historical source map

- [`ChangeLog`](../ChangeLog) records selected changes from the version-1 and
  version-2 development periods.
- [`doc/announce`](announce/) contains announcements for 1.2, 1.3, 1.3.1, 1.4,
  1.4.1, 1.5, 1.5.2, 1.6.0, and 2.1.0.
- [`doc/ROADMAP_2.0.md`](ROADMAP_2.0.md) records the goals and migration plan for
  the version-2 rewrite.
- Git tags and their trees remain the authoritative source for the exact code in
  each release.
