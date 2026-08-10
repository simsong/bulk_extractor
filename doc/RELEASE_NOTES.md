# bulk_extractor release notes

This file consolidates release highlights that were previously spread among
`ChangeLog`, version roadmaps, Git tags, and announcements in
[`doc/announce`](announce/). The older sections are summaries of those sources,
not exhaustive changelogs. Some legacy announcement dates disagree with tag
dates; the version history below therefore uses dates only where the repository
history is clear.

The historical roadmaps were planning documents, not release records. Their
relevant context is incorporated below; preserved, commit-specific copies are
linked from the [historical source map](#historical-source-map).

## 2.2.0 (draft)

**Status:** Unreleased. The source version is currently
`v2.2.0alpha1`.

### Executive summary

bulk_extractor 2.2.0 is a reliability, security, and release-engineering
update. It fixes CVE-2026-24857 in the bundled RAR decoder, removes fragile
submodule checkout requirements, improves hostile-input and shutdown handling,
and adds tested Windows, package, and documentation delivery paths. The release
also adds practical extraction improvements, including WiFi PCAP preservation,
Windows raw-device input, and runtime scanner modules.

From the last commit reachable on 1 July 2026 (`898db1b`) through the current
head (`f6c69639`), the release changes 439 versioned paths and
109,638 lines: 100,899 additions (92.0%) and 8,739 deletions (8.0%). Vendor
imports account for 267 paths (60.8%): 155 `be20_api` paths (35.3%), 105
`spdlog` paths (23.9%), and seven DFXML-schema paths (1.6%). The remaining 172
paths (39.2%) are project source, tests, documentation, build, CI, and release
work. The BE2.2.0 milestone has closed 87 issues and has three still open;
the comparison range contains 109 merged pull-request commits.

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

- Began migrating the historical GitHub wiki into source-controlled
  documentation. The new [installation guide](installation.md) is the
  maintained build reference; wiki pages now direct readers to the applicable
  source-tree documentation.
- The project license statement now clearly identifies post-January-2015
  development as GPL-3.0-or-later. Original NPS works retain their distinct
  U.S.-Government status; bulk_extractor must not be described as a
  public-domain project. RPM package metadata now includes the documented SPDX
  license identifiers, including the Autoconf exception used by bundled macros.
- A standalone 64-bit Windows `.exe` is built with MinGW on Ubuntu and is
  checked to ensure that it imports no non-system Windows DLLs. GitHub Actions
  publishes it as a downloadable artifact. The executable includes static
  libewf support and its Windows runtime workflow scans an E01 fixture;
  attaching it to the 2.2.0 release remains a release task
  ([PR #543](https://github.com/simsong/bulk_extractor/pull/543),
  [#655](https://github.com/simsong/bulk_extractor/issues/655)).
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
  link type and report WiFi frame metadata in `wifi.txt`; carved payloads are
  checked for a valid protocol version, frame type, and minimum header length
  ([PR #559](https://github.com/simsong/bulk_extractor/pull/559),
  [issue #617](https://github.com/simsong/bulk_extractor/issues/617)).
- The carving guide documents recorder-specific `-S <recorder>_carve_mode=`
  settings, defaults, and feature-file behavior ([#264](https://github.com/simsong/bulk_extractor/issues/264)).
- Release managers can run the AWS large-image matrix from one interactive
  Bash launcher, with live SSH log progress and S3 result archives; it does not
  require CloudFormation or a GitHub Actions workflow ([#624](https://github.com/simsong/bulk_extractor/issues/624)).

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
- The progress display now provides the explicit `available_memory_bytes`
  metric alongside the legacy `available_memory` alias, and renders byte-valued
  status metrics in MiB ([#240](https://github.com/simsong/bulk_extractor/issues/240)).
- Email extraction now enforces independent 64-octet local-part and 253-octet
  domain limits for ASCII and UTF-16 input, avoiding truncated suffix features
  from overlong addresses ([#585](https://github.com/simsong/bulk_extractor/issues/585)).
- `bulk_diff.py` now compares legacy hexadecimal byte escapes with current
  octal escapes by default, with an explicit raw mode for byte-for-byte audit
  comparisons ([#225](https://github.com/simsong/bulk_extractor/issues/225)).
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
- Follow-up parser hardening rejects overflowing scaled-size arguments,
  malformed Windows volume paths, invalid or truncated network headers, and
  unsafe HTTP-log backtracking before accessing their bytes.
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

- A build configured with `--disable-rar` now omits RAR-only tests without
  breaking `make check`, and explicitly warns that RAR coverage was not run.
- A multi-stage Debian Bookworm container image provides a reproducible,
  unprivileged environment for scanning regular image files. It documents its
  libewf and Lightgrep limitations, omits the unused PCAP runtime library, and
  supplements native platform CI rather than replacing it
  ([#159](https://github.com/simsong/bulk_extractor/issues/159)).
- Builds now enable basic compiler stack-canary protection (`-fstack-protector`)
  when both selected C and C++ compilers support it ([#376](https://github.com/simsong/bulk_extractor/issues/376)).
- AddressSanitizer now runs on every pull request while redundant workflow
  execution has been reduced
  ([PR #514](https://github.com/simsong/bulk_extractor/pull/514)).
- Snap builds use native Snapcraft and LXD commands plus Node 24-compatible
  artifact upload tooling, removing deprecated Node 20 action dependencies.
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
- Renamed and updated the MinGW notes for the current Windows CI artifact,
  static dependency checks, E01 runtime coverage, and raw-device limitations.
- Rewrote the installed `bulk_extractor(1)` manual for the 2.2 command-line
  interface, including current logging, scanner controls, path-printer aliases,
  output-directory requirements, and supported documentation.
- Restored the substantive version 1 user- and programmer-manual material that
  remains applicable to 2.2, including theory of operation, forensic paths,
  feature files and histograms, investigation workflows, worked public-corpus
  examples, troubleshooting, detailed scanner architecture, recorder and sbuf
  guidance, development examples, and coding practices. The historical
  BEViewer workflow is retained in an appendix, clearly marked as unavailable
  in 2.2 and as requirements for its planned return in the 2.x series.
- Added a user-facing migration guide covering the observable differences from
  version 1.x to version 2 and from version 2.1 to 2.2. Expanded the programmer
  manual's high-level scanner lifecycle coverage with illustrated help,
  disabled-scanner, concurrent-scan, and exception paths, while retaining
  `doc/scanner_api.md` as the normative scanner contract.
- LaTeX documentation CI now builds draft pull requests and runs for every
  change under `doc/`, so manual-source and supporting-file errors are caught
  before a pull request is marked ready for review. For pull requests, the
  code, coverage, and Windows workflows ignore changes limited to
  documentation and this LaTeX workflow. The user-manual build uses an
  explicit, portable asset manifest rather than a GNU `make`-specific wildcard.
- Removed the unmaintained standalone HTML overview; the current LaTeX guide
  and published documentation site are the supported user documentation.
- Removed the unmaintained version-1 performance notebook with obsolete
  benchmarks, platforms, and SQL tuning guidance.
- Moved the historical source and technical-debt audit to
  [`doc/RELEASE_2.2.0_PLANNING.md`](RELEASE_2.2.0_PLANNING.md) as the 2.2.0
  release-planning record; the live technical-debt backlog remains in
  [Discussion #545](https://github.com/simsong/bulk_extractor/discussions/545)
  and its linked issues.
- Documented a controlled release procedure and release-issue template, with
  isolated artifact assembly, macOS and container `distcheck` gates, and
  source-level downstream submission paths for Debian/Kali and
  Fedora/openSUSE ([#621](https://github.com/simsong/bulk_extractor/issues/621),
  [#622](https://github.com/simsong/bulk_extractor/issues/622),
  [#623](https://github.com/simsong/bulk_extractor/issues/623),
  [#626](https://github.com/simsong/bulk_extractor/issues/626)).
- Added Debian source and binary package metadata plus a clean Bookworm
  `make release-deb` build and installed-package smoke test. Debian package
  versioning is derived from `configure.ac`; archive submission and signing
  remain maintainer-controlled ([#622](https://github.com/simsong/bulk_extractor/issues/622)).
  `make release-deb` creates Debian 3.0 (quilt) source packages from the
  `make dist` archive, requires an annotated release tag at `HEAD` and a clean
  checkout, and copies generated `.deb`, `.dsc`, `.changes`, `.buildinfo`,
  `.orig.tar.gz`, and `.debian.tar.*` artifacts to `$(RELEASE_ARTIFACT_DIR)`
  on the host.
- Added tag-driven draft-release assembly for the source archive, tested
  Windows executable, and tested amd64/arm64 Snap packages. The Python driver
  derives the version from `configure.ac`, can assemble and checksum supplied
  artifacts with `--dry-run`, and never publishes a GitHub Release without a
  release-manager action
  ([#621](https://github.com/simsong/bulk_extractor/issues/621)).
- The release workflow now assembles and verifies artifacts in a read-only job;
  only its separate draft-publishing job receives repository write permission.
- Added a step-by-step release-manager runbook covering the release issue,
  final version promotion, signed tag, automated or manual draft, asset review,
  public publication, and downstream follow-up.
- Release managers can run an AWS OIDC large-image gate that uses a disposable,
  fixed-size instance with an eight-hour shutdown cap, encrypted temporary
  storage, least-privilege input/output access, cleanup, and an attested,
  redacted result summary. AWS Budget alerts supplement, but do not replace,
  the runtime cap ([#624](https://github.com/simsong/bulk_extractor/issues/624)).
- Added a strict-confinement Snap package and native amd64/arm64 build and
  install-test workflow. Stable Snap Store publication is limited to annotated
  release tags, protected release-manager credentials, and a project-owned
  publisher account; raw-device access remains an explicitly connected,
  Store-reviewed interface ([#626](https://github.com/simsong/bulk_extractor/issues/626)).
- Added a tagged, clean-checkout `make release-rpm` gate that builds and smoke
  tests binary RPMs and SRPMs in pinned Fedora and openSUSE environments for
  GitHub download testing; distribution archives continue to build submitted
  source packages themselves ([#623](https://github.com/simsong/bulk_extractor/issues/623)).

### Known limitations and release work

- Keep the Windows artifact workflow green and attach its `.exe` to the 2.2.0
  GitHub release.
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
reporting and testability, and introduced the initial version-2 E01 path. Its
planning record proposed C++14, continuous integration, systematic unit and
end-to-end tests, and a narrower command-line distribution. It also identified
future scanner and carving work that was not a release commitment.

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
The contemporary roadmap also recorded longer-term work on scanner development,
carving, testing, and BEViewer; those proposals were not commitments for 1.6.0.

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
[1.5.2 notes](announce/announce_1.5.2.md). The associated roadmap carried
forward proposals for future scanners, carvers, testing, and user-interface
work; it does not define the shipped 1.5.x scope.

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
[1.4.1 notes](announce/announce_1.4.1.txt). The associated roadmap recorded
planned scanner, raw-device, restart, and validation work, including proposals
deferred to later releases.

## Historical source map

- [`ChangeLog`](../ChangeLog) records selected changes from the version-1 and
  version-2 development periods.
- [`doc/announce`](announce/) contains announcements for 1.2, 1.3, 1.3.1, 1.4,
  1.4.1, 1.5, 1.5.2, 1.6.0, and 2.1.0.
- The original planning records are preserved at the commit that last contained
  them: [1.4 roadmap](https://github.com/simsong/bulk_extractor/blob/d79d91bfee1a601189045a3dfa873b9c3086b493/doc/ROADMAP_1.4.md),
  [1.5 roadmap](https://github.com/simsong/bulk_extractor/blob/d79d91bfee1a601189045a3dfa873b9c3086b493/doc/ROADMAP_1.5.md),
  [1.6 roadmap](https://github.com/simsong/bulk_extractor/blob/d79d91bfee1a601189045a3dfa873b9c3086b493/doc/ROADMAP_1.6.md),
  and [2.0 roadmap](https://github.com/simsong/bulk_extractor/blob/d79d91bfee1a601189045a3dfa873b9c3086b493/doc/ROADMAP_2.0.md).
- Git tags and their trees remain the authoritative source for the exact code in
  each release.
