# bulk_extractor and be20_api source and technical-debt audit

Date: 2026-07-19

## Executive conclusions

This audit recommended bringing `src/be20_api` back into the
`bulk_extractor` repository while retaining it as a distinct internal
component. That integration is now implemented: be20_api, dfxml_cpp, utfcpp,
and the DFXML schemas are ordinary tracked source; `libbe20.a` is the explicit
internal component; and the parent and component tests consume that target.
There are no remaining Git links or recursive-submodule build steps.

The desirability and feasibility of reintegration were both **high**. The
migration retained the exact parent-pinned revisions and records their commit
IDs and upstream URLs in `src/be20_api/VENDORED.md` and
`dfxml_schema/VENDORED.md`. The user confirmed that no supported independent
be20_api consumer remains; tcpflow uses the abandoned be14 API. The result is
an in-tree library component rather than an undifferentiated source glob.

The most urgent source issues are independent of that restructuring:

1. `be20_api/pcap_fake.cpp` trusts a packet's `caplen` and can read more data
   than the buffer allocated from the file header's `snaplen`. A malformed PCAP
   can therefore cause a heap overflow when the fallback PCAP implementation is
   used.
2. `sbuf_t`, the bounds-checking primitive used by every scanner, has multiple
   correctness and memory-safety defects: its `memcmp` skips byte zero and
   underflows at length zero, `wbuf` permits a one-byte overrun, slice page-size
   arithmetic underflows, and several offset checks can wrap.
3. Returning after a phase-1 `DiskWriteError` leaves the notification thread in
   phase 1. Its destructor then asserts or waits forever.
4. Runtime scanner loading is advertised in user and API documentation, and
   the CLI collects plug-in directories, but both loader entry points always
   throw and the directories are never consumed.
5. The optional Lightgrep scanner sources have not been migrated to the current
   API. Enabling Lightgrep exposes duplicate `switch` cases, invalid member
   access, and references to the removed `recursion_control_block`.

The documentation is not a single consistent description of the current
program. The root README is partly current, the manual and LaTeX books mostly
describe 1.4/1.5, the 2.0 roadmap is an unlabeled planning snapshot, and the
be20_api README describes a library and plug-in system that do not exist in the
build. The generated/operational CI configuration also contains known-invalid
steps.

## Audit scope and method

This was a complete repository audit, not a review of only recently changed
files. The following were inventoried and analyzed:

- every tracked first-party C, C++, Flex, Python, Java, shell, Autoconf,
  Automake, CI, test, and documentation source in `bulk_extractor`;
- every tracked first-party implementation, header, test, build, CI, and
  documentation source in the `src/be20_api` checkout;
- the generated Flex C++ files as derived artifacts, with their `.flex` files
  treated as authoritative;
- bundled RAR, TSK header, `cxxopts`, Catch, thread-pool, utfcpp, and dfxml_cpp
  code for integration, versioning, build, and license risk, rather than as new
  line-by-line audits of their upstream implementations; and
- the former parent and nested submodule state, source-list generation, configure-time
  composition, test entry points, and workflows.

“Complete” here means that no tracked source area was excluded. It does not
mean that static inspection proves the absence of defects. Dynamic sanitizer,
fuzz, Windows, Lightgrep, and no-libpcap coverage was unavailable in this
checkout, as described under **Validation status**. The default EWF path was
built and exercised by the E01 test fixture.

The parent was initially inspected at its `main` checkout. At the start of the
audit, its gitlink recorded be20_api commit
`625f5b3ddd9d6baf4aaa7c10a6cefb5e2bfb1a8b`, while the be20_api worktree was
at clean `main` commit
`00f03b21b9f3da8c613828c6fb5ab05b8f661a79`. That mismatch is material:
`625f5b3` is three commits beyond `00f03b2` on the separate `fix-noreturn`
branch and includes RE2-related build fixes. Findings were checked against the
files present there. The build repair reconciled the checkout to the parent's
recorded `625f5b3`, and the integration imported that exact revision.

## Current architecture and actual dependency boundary

### bulk_extractor

The parent has four main layers:

1. `main.cpp` and `bulk_extractor.cpp` parse configuration, construct the
   scanner and recorder graph, manage output, and run the phases.
2. `image_process.*`, `phase1.*`, `notify_thread.*`, and the restarter own
   input traversal, page scheduling, hashing, progress, and restart behavior.
3. `scan_*`, the Flex sources, decompression helpers, EXIF support, RAR code,
   TSK structures, and PCAP writer implement extraction.
4. be20_api supplies buffers and forensic positions, scanner registration and
   scheduling, feature recorders and histograms, threading, regex/unicode
   helpers, path printing, DFXML, and platform abstractions.

### be20_api

The first-party be20_api source falls into these cohesive areas:

- `sbuf`, `sbuf_stream`, and `pos0`: byte storage, bounds, ownership, and
  forensic addressing;
- `scanner_config`, `scanner_params`, `scanner_set`, and `threadpool`: scanner
  lifecycle and scheduling;
- `feature_recorder*`, `histogram_def`, and `atomic_unicode_histogram`: output
  and aggregation;
- `path_printer`, `packet_info`, and `pcap_fake`: path and packet adapters;
- `regex_vector`, `unicode_escape`, `word_and_context_list`, atomics, timers,
  statistics, and utilities; and
- standalone tests plus bundled Catch/thread-pool and nested dfxml_cpp/utfcpp
  dependencies.

### Former dependency boundary

Before integration, these facts made be20_api an in-tree implementation
dependency in all but Git layout:

- `configure.ac` directly `m4_include`d be20_api and dfxml_cpp macros.
- `src/Makefile.am` added be20_api and utfcpp private include directories.
- `etc/makefile_builder.py` walked `src/be20_api` and emitted all of its `.cpp` and
  `.h` files into the parent's generated `src/Makefile.auto_defs`.
- `bulk_extractor` and `test_be` compiled those files directly. There was no
  be20_api link dependency.
- The standalone be20_api `Makefile.am` built `test_be20_api`, not the
  `be20_api.a` promised by its README.
- Parent code directly named be20_api types throughout its public and private
  implementation.

The integration removes that mismatch. `src/be20_api/Makefile.defs` now lists
the reviewed production sources explicitly, `src/Makefile.am` builds them once
as `libbe20.a`, and both executables and component tests use that library.
Test-only sources have their own manifest. Root `m4/` owns the Autoconf macros.

## Reintegration assessment and result

### Desirability: high; completed

Reintegration now provides these results:

- makes a parent commit self-contained and removes the former gitlink/worktree
  skew;
- eliminates the most failure-prone nested checkout and bootstrap sequence;
- lets source, tests, CI, API documentation, and versioning change atomically;
- replaces filesystem discovery of implementation files with an explicit source
  manifest and component target;
- exposes optional configurations to the same CI matrix as the executable;
- makes ownership and deprecation decisions visible in one issue tracker; and
- removes misleading independence: today be20_api has neither an installed
  artifact nor a compatibility gate.

The strongest argument against reintegration would have been an active
independent consumer. The user confirmed there is none: tcpflow remains on the
abandoned be14 API. be20_api had no package, installed-header set,
semantic-version policy, or compatibility test that the move could preserve.

### Feasibility: high; migration completed

The code was already compiled in the desired physical arrangement, so moving
it required no ABI conversion. The identified risks were handled as follows:

- The exact parent-pinned be20_api commit `625f5b3` was imported, not the older
  `00f03b2` `main` tip.
- The pinned dfxml_cpp, utfcpp, and DFXML schema revisions were imported as
  ordinary source, eliminating hidden nested Git links.
- Provenance documents retain the upstream URLs and exact commit IDs; imported
  histories and license files remain in the source tree.
- Root and be20_api license texts are broadly compatible (US-government/public
  domain material plus MIT and separately identified third-party code), but
  both manifests contain stale paths and descriptions. Third-party notices must
  be normalized during the move, not lost.
- There is no supported external consumer requiring a standalone release.

### Implemented target

The stable path remains `src/be20_api/` to avoid an unnecessary include churn.
Automake builds one explicit `libbe20.a` convenience library from a reviewed
manifest. `bulk_extractor`, `test_be`, and `test_be20_api` link that target;
`test_dfxml` is a separate parent-managed component test. The namespace and
component boundary remain, while the Git submodule boundary is gone.

Do not continue generating the source list by recursively walking the tree.
An explicit source list prevents test programs, demos, broken prototypes, or
new upstream files from silently entering production builds.

The default macOS build, all three parent-managed tests, and `make distcheck`
pass. Optional ASan/UBSan, no-libpcap, Lightgrep, Linux, and Windows matrices
remain follow-up validation work rather than blockers to the repository move.

## Significant source-code debt

Priorities use the following meanings:

- **P0**: plausible memory corruption, hang, or integrity failure on supported
  input/error paths; fix before the next release.
- **P1**: broken advertised behavior, unsafe core invariant, or configuration
  that cannot work; fix in the next engineering cycle.
- **P2**: material reliability, test, build, or maintainability debt.
- **P3**: cleanup or documentation debt with limited immediate runtime risk.

### P0: memory safety and termination

#### Unbounded PCAP packet read

`be20_api/pcap_fake.cpp:111` allocates `header.snaplen` bytes. The packet loop
then reads the per-packet `hdr.caplen` at lines 165-177 directly into that
buffer without requiring `caplen <= snaplen`. Both fields are controlled by the
input. `snaplen` itself is also accepted without a sensible upper bound. This
is a heap overflow and denial-of-service vector for any consumer of the
no-libpcap offline-reader fallback. The fallback is compiled into the current
parent source set, although no active parent call site was found; it remains a
public be20_api implementation and a likely external-consumer path.

Store the allocation size, reject impossible headers, require
`caplen <= snaplen` and `caplen <= len` where appropriate, validate timestamp
fields, and add malformed/truncated PCAP tests under ASan. Fuzz both native
libpcap and fallback configurations.

#### `sbuf_t` violates its bounds-checking contract

`sbuf_t` is the trust boundary between hostile bytes and every scanner, so its
defects have system-wide impact:

- `be20_api/sbuf.h:294-300`: both `memcmp` variants start at `at + 1` and
  `cbuf + 1`, so they never compare the first byte. `len == 0` underflows to a
  huge `size_t`. The safe wrapper validates space but delegates to the broken
  implementation.
- `be20_api/sbuf.cpp:431-442`: `wbuf` rejects `i > bufsize`, permitting the
  invalid write at `i == bufsize`; its `i < 0` check can never be true because
  `i` is unsigned.
- `be20_api/sbuf.cpp:336-351`: page-size adjustment has its condition reversed.
  An offset within the primary page does not reduce `pagesize`; an offset past
  the primary page is subtracted and underflows. The later clamp can label
  margin bytes as primary-page bytes and cause duplicate or invalid scanning.
- `be20_api/sbuf.cpp:62-87`: slice constructors clamp sizes but still form
  `src.buf + offset` even when `offset` exceeds the source, which is undefined
  pointer arithmetic.
- `be20_api/sbuf.h:314-388`: checks written as `i + width > bufsize` can wrap.
  They should be expressed as `i > bufsize || width > bufsize - i` (or via one
  checked helper).
- `get32u_unsafe` and its big-endian peer shift integer-promoted bytes before
  converting to `uint32_t`; shifting a value into signed-int overflow is
  undefined. Cast before shifting.

Replace the duplicated inline checks with one overflow-safe range primitive and
add table-driven boundary tests at zero, exactly-at-end, one-past-end,
`SIZE_MAX`, page/margin crossings, and zero-length comparisons. Run those tests
under ASan and UBSan.

#### Notification-thread hang on disk-full handling

`notify_thread::~notify_thread()` unconditionally calls `join()`, and `join()`
asserts that `phase != BE_PHASE_1` before joining. The thread itself runs while
the phase is 1. `bulk_extractor.cpp:615-620` returns immediately on
`DiskWriteError` without changing the notification phase. With assertions the
destructor aborts; without assertions it can block forever.

Use value-owned `std::thread`/`std::jthread` and an unconditional stop protocol
that is invoked on every exit path. A scope guard should stop workers and the
notifier before returning or propagating. Add a substantive integration test
using a recorder that produces a real write failure; do not mock the thread.

### P1: core reliability and advertised features

#### File mapping and ownership failures

`be20_api/sbuf.cpp:373-405` does not validate `open()` or `mmap()` failure,
treats descriptor zero as “not owned” because the destructor only handles
`fd > 0`, and does not define a safe zero-length mapping path. In the no-`mmap`
branch, the allocated buffer is passed to the non-owning constructor and never
stored in `malloced`, leaking every mapped file. `sbuf_malloc` also constructs
an object after an unchecked `malloc` result.

Use RAII handles and an owning factory with explicit mapping/allocation
variants. Test empty files, fd 0, open denial, truncated reads, `MAP_FAILED`, and
the no-`mmap` build.

The destructor's `std::runtime_error(...)` expressions at lines 151-166 merely
construct and discard exceptions; they do not enforce the live-child or
platform invariants. The non-owning constructor erases rather than inserts its
instance in debug leak tracking. `range_exception_t::what()` uses a shared
static buffer and races between scanner threads. These should be corrected as
part of the same ownership rewrite.

#### Runtime plug-ins do not exist despite the interface and CLI

`scanner_set::add_scanner_file()` and `add_scanner_directory()` immediately
throw; their implementations are inside `#if 0`. `bulk_extractor.cpp` builds a
default search path, reads `BE_PATH`, and accepts `-P`, but never calls either
loader. The option loop also stops after the first supplied `-P` value. The root
manual, 2.0 roadmap, scanner comments, and be20_api README all describe dynamic
scanner loading as operational.

Choose one product decision:

- implement a versioned, tested plug-in ABI and exercise a real shared-library
  scanner on each supported platform; or
- remove the loader methods, `-P`, `BE_PATH`, and all plug-in claims.

Leaving security-sensitive dynamic-loading code disabled but advertised is the
worst of both choices.

#### Optional Lightgrep configuration is source-broken

Lightgrep is off by default, masking source that no longer matches
`scanner_params`. Examples include:

- `scan_lightgrep.cpp` has two `case PHASE_INIT` labels and mixes `sp.info.`
  with `sp.info->`;
- `scan_email_lg.cpp` calls `scan_lg(..., rcb)` although `rcb` is undefined;
- the Lightgrep sources and `doc/writing_pattern_scanners.md` still use the
  removed recursion-control interface; and
- all Lightgrep registrations in `bulk_extractor_scanners.h` are commented out
  even when `HAVE_LIBLIGHTGREP` is set.

Remove this configuration and archive its guide, or port it and add a required
CI job. A configure switch that selects uncompilable, unregistered code is not
a supported optional feature.

#### Short reads expose uninitialized bytes to scanners

`process_ewf::sbuf_alloc()` and `process_raw::sbuf_alloc()` allocate the
requested `count` and only reject negative/zero results. Neither shrinks the
buffer nor rejects a positive short read. The EWF path can therefore scan
uninitialized tail bytes; the raw implementation assumes the fstream read
completed rather than using `gcount()`.

Implement a checked read-exact loop or construct the sbuf using the actual byte
count. Test a reader that legitimately returns short chunks without mocking an
external service.

#### Input selection and split-image formatting are unsafe

`image_process::open()` lowercases `fn.extension()` but compares it with
`"e01"` instead of `".e01"`; its fallback searches only uppercase `.E01`.
Lowercase E01 images can silently be interpreted as raw. Directory detection
is likewise case-sensitive.

`make_list_template()` retains the user's path as a `snprintf` format and only
replaces `000`/`001` with `%03d`. Any other `%` in the filename becomes another
format conversion without a corresponding argument. Avoid printf templates;
construct sibling filenames with string replacement or filesystem operations.

If `image_process::open()` allocates an implementation whose `open()` returns
nonzero or throws, the raw pointer leaks. Return `std::unique_ptr<image_process>`
and transfer ownership only after successful initialization.

#### Packet address accessors use byte offsets as element offsets

`be20_api/packet_info.h` defines IPv4 source/destination offsets as 12/16 bytes
and IPv6 offsets as 8/24 bytes, but accessors add them to pointers already cast
to `in_addr*`/`ip6_addr*`. Pointer arithmetic therefore scales the offsets by
the structure size. Several Ethernet/IP casts can also be unaligned, and the
Ethernet length check is shorter than the structure subsequently accessed.

Packet dispatch is currently disabled in `scanner_set`, which limits immediate
reachability but also means tests do not protect this code. Either repair and
test the packet API using byte-wise parsing, or remove the dormant subsystem.

#### Feature-recorder undefined behavior and incomplete low-memory path

`feature_recorder_file::banner_stamp()` dereferences `line.end()` while trying
to trim a line ending. Dereferencing the end iterator is undefined; `getline`
already removes `\n`. Use `line.back()` after an emptiness check if `\r` must be
removed.

`histograms_write_largest()` is explicitly unimplemented even though the
interface identifies it as the low-memory relief mechanism. The file-based
histogram loop reaches EOF and then repeats up to ten times without clearing or
rewinding the stream. Specify and test a bounded spill/merge algorithm or
remove the nonfunctional fallback.

### P2: correctness and maintainability

#### Phase-1 sampling and error semantics

- The random distribution is `[0, max_blocks]`, inclusive, although valid
  blocks end at `max_blocks - 1`. The invalid endpoint can seek to EOF.
- The parser rejects a fraction of exactly 1 while its error says `0<f<=1`.
- `sampling_passes` is parsed but never used.
- The default random engine is deterministically seeded, but reproducibility is
  neither exposed nor documented.
- A broad `catch (std::exception)` around buffer acquisition and scheduling
  records every synchronous scanner/output exception as a read error and then
  continues. Typed read failures should be separated from scanner and output
  failures.
- The disk-error polling path calls `exit(1)`, bypassing structured shutdown,
  while another disk-error path returns 6. Exit semantics are inconsistent.
- `max_minute_wait` is a CLI option, and `Config::max_wait_time` exists, but the
  parsed value is never assigned or enforced.
- A SHA-1 is written to DFXML, but `Phase1::image_hash` is never assigned, so the
  console “Hash of Disk Image” branch is dead.

#### Scanner-specific debt

All authoritative scanner sources were inspected. The following table records
their status by implementation family; “no isolated blocker” means no defect
as severe as those above was established by static review, not proof of
correctness.

| Scanner/source family | Assessment |
|---|---|
| Flex: `accts`, `base16`, `email`, `gps`, `vehicle` | Authoritative `.flex` sources coexist with very large generated, ignored `.cpp` files in developer trees. Generation/version reproducibility is not enforced. The email source uses the broken `sbuf::memcmp`, and this family has limited boundary/fuzz testing. |
| Lightgrep: `accts_lg`, `base16_lg`, `email_lg`, `gps_lg`, `lightgrep` | API-stale and not registered; the enabled build is broken. `lg_patterns.cpp` and the pattern guide contain unresolved boundary-context work. |
| Recursive decoders: `base64`, `gzip`, `hiberfile`, `pdf`, `rar`, `xor`, `zip` | High attack surface, manual bounds/format parsing, and few malformed-corpus tests. These should be continuous fuzz targets with recursion/output budgets. No isolated blocker beyond shared sbuf/ownership issues was established. |
| Structured binary: `elf`, `evtx`, `exif`, `exiv2`, `net`, `ntfsindx`, `ntfslogfile`, `ntfsmft`, `ntfsusn`, `outlook`, `sqlite`, `utmp`, `windirs`, `winlnk`, `winpe`, `winprefetch` | Extensive casts from hostile byte buffers to native structures make alignment, packing, integer-overflow, and endian correctness platform-dependent. Replace casts incrementally with checked sbuf reads. `evtx` also leaks each synthetic header: memory from `malloc` is wrapped in a non-owning `sbuf_new` and never freed. |
| Text/record: `aes`, `facebook`, `find`, `httplogs`, `json`, `kml`, `msxml`, `vcard`, `wordlist` | Mostly bounded by sbuf/recorder behavior. `find` retains global state; `vcard` labels itself incomplete; `wordlist` contains an unfinished SQL path. Add direct tests for parsing boundaries and scanner lifecycle. |
| `ccns2`, header prototypes, `lg_patterns`, `jpeg_dump`, `stand`, and `old_scanners` | These are present in the tree but not consistently registered in the production scanner list. Classify each as production, experimental, generated, or archival and exclude nonproduction code from distribution/build discovery. |

The scanners share a recurring design risk: input validation often relies on
`assert` or native-structure layout. Assertions disappear in release builds and
must not validate hostile media. The safest modernization is not a wholesale
rewrite; it is a checked byte-reader API followed by conversion of the
highest-risk recursive and binary parsers.

#### Additional concrete leaks and lifecycle debt

- `scan_evtx.cpp:197-228` allocates a synthetic header with `malloc`, wraps it
  in the explicitly non-owning `sbuf_new`, deletes only the sbuf, and leaks the
  header for every reconstructed file.
- Core API ownership is dominated by raw pointers, manual `new`/`delete`, and
  manual child/reference counts. This makes exceptional exits particularly
  dangerous. Convert owners first (`unique_ptr`), then use references or
  non-owning pointers for observers.
- Directory recursion relies on the implementation's iteration order and
  default symlink/error behavior. Define deterministic ordering and explicit
  permission/symlink policy for reproducible forensic output.
- `find_offset()` linearly scans split-image segments for every read. This is
  correct for small segment counts but unnecessarily scales with input layout;
  an ordered lookup is simple once behavior is covered.
- The source contains duplicated includes, obsolete comments, dead branches,
  mixed C and C++ error handling (`exit`, `err`, exceptions), and inconsistent
  integer types for file sizes and read counts.

#### Build-system debt

- Resolved by the integration: `src/be20_api/Makefile.defs` explicitly separates
  production and test-only sources. Adding a file no longer changes production
  membership implicitly, and generated `config.h` files are not imported.
- Generated and ignored Autotools state can silently retain absolute package
  manager paths. The initial checkout demonstrated this with stale RE2 and
  Abseil paths; clean regeneration removed them from the active build.
- Resolved by the integration: the parent consistently requires C++17, missing
  RE2 is fatal, `HAVE_RE2` is defined once, and the imported macro no longer
  sorts RE2 compiler or linker flags.
- Top-level `Makefile.am` retains commented/dead shared-library and Python 2.7
  install logic.
- `bootstrap.sh` no longer performs any Git or submodule mutation. It regenerates
  the remaining manifests and delegates Autotools regeneration to `autoreconf`.
- Platform setup scripts are mutable machine provisioning scripts rather than
  reproducible dependency declarations. The macOS script hard-codes Homebrew
  paths and edits shell startup state; older Fedora/CentOS scripts remain
  alongside the active configurations.

### P2: test debt

The existing Catch tests contain valuable sbuf, utility, scanner, and end-to-end
logic checks. The debt is coverage selection and configuration, not simply test
count:

- Resolved in the integration: the be20 thread-pool test queues six buffers on
  four workers, joins them, and verifies worker, queue, and recorder results.
  It no longer contains an unconditional 60-second watchdog delay. Cancellation,
  backpressure, and exception propagation still need focused tests.
- Parent CI sets `DEBUG_FAST=TRUE`; several expensive image/scanner tests return
  early under that setting. The default gate therefore omits important
  segmented-image and E01/regression behavior.
- Many registered scanners lack focused tests named for their parser or edge
  cases, especially EVTX, the NTFS group, Outlook, SQLite, UTMP, WinLNK,
  WinDirs, and optional scanners.
- Python tests are not an Automake test target (`tests/Makefile.am` has the
  Python test assignment commented out). The regression driver is manual.
- ThreadSanitizer is present as a workflow step whose condition can never match.
- No required no-libpcap, Lightgrep, Windows-parent, malformed corpus, or fuzz
  gate protects the paths with the greatest portability/input risk.

Do not add assertion-free “runs without crashing” tests merely to increase
coverage. Highest-value additions are boundary tables for sbuf, malformed PCAP,
short-reader and split-name tests, real disk-write shutdown, scanner lifecycle,
and small curated malformed corpora for recursive/binary scanners.

### P2/P3: Python and support tools

The Python directory mixes Python 2 and Python 3 assumptions. The dormant
extension Makefile targets Python 2.7, while modules use constructs such as
`xrange`, `has_key`, `unicode`, old print syntax, and `cStringIO`. Other tools
are Python 3. `bulk_diff.py` imports the non-standard `path` package without a
dependency manifest, and graphing requires NumPy/Matplotlib without packaging
metadata. The Java tree contains a legacy generated configuration with version
1.5.5 and no current parent build.

Decide which tools are supported, port them to one Python baseline, package
their dependencies and entry points, and run meaningful fixture-based tests.
Move unsupported Python/Java artifacts to a clearly labeled archive.

## Documentation consistency audit

### Documents that should be corrected now

| Document | Conflict with current source | Required action |
|---|---|---|
| `README.md` | Calls this the 2.1 development branch although `configure.ac` is 2.1.1; tested platforms stop in 2023; names `DEBUG_BENCHMARK_CPU` and `DEBUG_DUMP_DATA`, while code reads `DEBUG_BENCHMARK` and `DEBUG_SCANNER_DUMP_DATA`; describes comma-separated scanner ignores although implementation uses substring matching; release notes name `be13_api`; Windows status is not tied to an active parent CI job. | Make it the concise canonical build/use/status page generated or checked against current help and CI. |
| `src/DEBUG.md` | Lists only a subset of variables and uses names different from the root README. | Derive a single table from named constants/access sites and document separators/precedence. |
| `src/scanners.md` | Contains only an unchecked list, omits status and some source distinctions, and is not synchronized with `bulk_extractor_scanners.h`. | Generate scanner status/help from registration metadata or delete the checklist. |
| `tests/README.md` | Examples and output are for 1.5.3 and include removed SQLite flags and old corpus paths; claims do not describe the current Catch/Automake gate. | Rewrite around `make check`, required fixtures, `DEBUG_FAST`, and supported regression modes. |
| `python/README.txt` | Omits several tools, Python versions, dependencies, packaging state, and unsupported modules. | Rewrite after deciding supported tools. |
| `LICENSE.md` | References old/nonexistent `src/be13_api` and old scanner paths/descriptions. | Re-inventory first- and third-party notices before moving files. |

### Documents that describe removed or nonfunctional behavior

| Document | Stale content |
|---|---|
| `doc/latex_manuals/BEUsage.txt` | Captured help for version 1.5.0; advertises obsolete options including `-q nn`, `-m`, and `-P`. It should be regenerated from current `--help` plus maintained prose. |
| `doc/writing_pattern_scanners.md` | Describes the old Lightgrep/recursion-control API, old registration locations, and scanners that are now commented out or uncompilable. |
| `doc/programmer_manual/BEProgrammersManual.tex` | Documents the former two-argument scanner ABI, old plug-in loading, and old source organization. |
| `doc/latex_manuals/BEUsersManual.tex`, `BEUsage.txt`, and worked examples | Explicitly describe versions 1.4/1.5, old installers, Python 2.7, old options, old scanner output, and obsolete distribution URLs. |

These should not remain adjacent to current docs without a banner. Either port
them and add a CI build/link check, or move them under `doc/archive/1.x/` with a
clear statement that they are historical and may be incorrect for 2.x.

### Historical planning/status documents

- `doc/ROADMAP_1.4.md`, `ROADMAP_1.5.md`, and `ROADMAP_1.6.md` are historical
  records but are not labeled archival.
- `doc/ROADMAP_2.0.md` still says C++14, `master`, be13_api, working plug-ins,
  SQL by default, and a dependency topology contradicted by the source. It also
  mixes completed plans with unchecked work.
- `src/TODO.md` contains old crash traces and untriaged claims rather than an
  actionable current backlog.
- `src/be20_api/TODO.md` is a 2021 work log containing completed, superseded,
  and unresolved items without current ownership.
- Root `NEWS`, be20_api `NEWS`, and both ChangeLogs do not form a coherent
  chronological release history.

Keep historical documents immutable if useful, but add archive headers and
move live work to tracked issues or a dated, owned roadmap.

### be20_api documentation conflicts

`src/be20_api/README.md`, `README_WIN.md`, and `doc/unit-tests.txt` now describe
the internal `libbe20.a`, parent build, and Catch/Makefile test path. The DFXML
and schema READMEs also identify their vendored in-tree status. Historical
`TODO.md`, NEWS, ChangeLog, and status notes remain archival documentation debt.

### CI as executable documentation

The build and coverage workflows now provide one three-platform build matrix
and one coverage job. They avoid duplicate push and pull-request runs, use the
current GitHub output mechanism, and name uploaded distributions from the
actual version-step output. Several release-oriented workflows remain
known-invalid:

- `create-release-installer.yml` runs `chmod +x && bootstrap.sh` (missing the
  chmod operand), uses old action versions, and references
  `steps.create_release.outputs.upload_url` without a `create_release` step.
- Coverity still checks out `actions/checkout@main`; mutable action references
  should be pinned even though recursive checkout is no longer required.
- Some third-party actions are pinned to mutable `master` branches.
- The obsolete standalone be20_api workflows were removed during integration.

Repair and test the release workflow. Pin actions to maintained major versions
or commit SHAs and add a lightweight workflow syntax/reference check.

## Explanation of the former submodule update error

The reported messages:

```text
error: Entry 'src/be20_api/.github/workflows/build-ubuntu-macos.yml' not uptodate. Cannot merge.
error: Submodule 'src/be20_api' could not be updated.
error: Cannot update submodule:
        src/be20_api
```

mean that Git entered the `src/be20_api` repository to move or merge it to the
commit recorded by the parent, but that tracked workflow file did not match the
submodule index/worktree state Git expected. Updating would have overwritten or
merged through local content, so Git stopped rather than lose it. This commonly
occurs with `git submodule update --merge`, or after a checkout has modified a
tracked file without committing/stashing it. “Not uptodate” refers to the
submodule's own index/worktree, not to the GitHub Actions run status.

Before the integration, the obstruction was cleared and the parent and
worktree mismatch was reconciled. At the start of the audit they differed:

```text
parent gitlink:             625f5b3ddd9d6baf4aaa7c10a6cefb5e2bfb1a8b
initial submodule worktree: 00f03b21b9f3da8c613828c6fb5ab05b8f661a79
```

That was why the parent reported `M src/be20_api` and `git submodule status`
prefixed it with `+`: the parent expected the `fix-noreturn` descendant rather
than the checked-out be20_api `main` tip. Running
`git submodule update --init --recursive` after confirming the workflow file
was clean detached be20_api at `625f5b3`, as recorded by the parent.

This recovery procedure is now historical: `.gitmodules` and every Git-link
entry have been removed. `src/be20_api`, dfxml_cpp, utfcpp, and `dfxml_schema`
are ordinary files, so this class of checkout/update failure can no longer
occur in bulk_extractor.

For an older checkout that still predates integration, inspect all three states:

```sh
git -C src/be20_api status --short
git -C src/be20_api diff -- .github/workflows/build-ubuntu-macos.yml
git -C src/be20_api diff --cached -- .github/workflows/build-ubuntu-macos.yml
```

If changes matter, commit them on an intentional be20_api branch or stash them:

```sh
git -C src/be20_api stash push -m 'preserve workflow edit before submodule update' -- .github/workflows/build-ubuntu-macos.yml
git submodule update --init --recursive
```

If inspection establishes that the change is disposable, restore that one file
in the submodule and then update:

```sh
git -C src/be20_api restore --staged -- .github/workflows/build-ubuntu-macos.yml
git -C src/be20_api restore -- .github/workflows/build-ubuntu-macos.yml
git submodule update --init --recursive
```

The final command detaches `src/be20_api` at `625f5b3`, which is normal for
a submodule. Do not fix this by merely running `git pull` on be20_api `main`:
that keeps the submodule at a commit different from the one the parent records.
If the parent should instead follow `00f03b2`, that is a deliberate dependency
update: test it, stage the parent gitlink, and commit the new pointer.

## Validation status

All compilation and test execution used the repository Makefiles. The initial
generated build state was stale: it named a removed RE2 include directory,
required an unavailable Automake executable, and linked a removed Abseil
dylib. After updating Homebrew, reconciling submodules, running
`make distclean`, and regenerating the Autotools files, the build is green on
Apple Silicon macOS:

- Homebrew dependencies are present and `brew missing` reports no broken
  dependencies. The directly relevant versions at validation time include RE2 2025-11-05,
  Abseil 20260107.1, Automake 1.18.1_1, Autoconf 2.73, libtool 2.6.2,
  libxml2 2.15.3, libewf 20140816, json-c 0.19, expat 2.8.2, and Exiv2 0.28.8.
- `./bootstrap.sh`, `./configure`, and `make -j4` complete successfully without
  the former duplicate Automake directory rules.
- `make check -j4` passes all three registered programs: `test_be`,
  `test_be20_api`, and `test_dfxml` (3 passes, no skips or failures). With
  `DEBUG_FAST` unset, the segmented 30 MB image, E01, email, scanner, and other
  non-fast paths execute. The DFXML CPUID test checks the supported x86
  contract without falsely requiring an x86 vendor on Apple Silicon.
- `make distcheck` passes the clean archive configure/build/test,
  install/uninstall, and redistribution checks and produces
  `bulk_extractor-2.1.1.tar.gz`, including the vendored sources, licenses,
  provenance records, schemas, component tests, and fixtures.
- The Git index contains no mode-160000 entries, `.gitmodules` is removed, and
  the build and workflows do not initialize submodules.
- `src/bulk_extractor` links the installed RE2, Abseil, libewf, and other
  current Homebrew libraries rather than the stale Cellar versions.

This establishes a reproducible default build baseline. It does not invalidate
the static defects above; those still require focused regression tests and the
optional/sanitizer configurations recommended in this document.

## Recommended order of work

1. **Contain P0 risk:** fix and test PCAP length validation, all sbuf boundary
   primitives, and disk-error thread shutdown. Add ASan/UBSan gates.
2. **Preserve the integrated build baseline:** update vendored dependencies
   intentionally with provenance records, prevent generated dependency paths
   from entering source control, and retain `make check` plus `make distcheck`
   as release gates.
3. **Resolve false features:** either implement or remove plug-ins and
   Lightgrep. Correct the CLI and docs in the same changes.
4. **Repair input/ownership paths:** short reads, E01 detection, split filename
   construction, file mapping, packet accessors, histogram fallback, and EVTX
   ownership.
5. **Consolidate tests and CI:** exercise real optional configurations and
   malformed corpora; remove duplicate/broken workflows and pro-forma tests.
6. **Rebuild documentation:** one current README/man page/component guide;
   archive 1.x manuals and historical roadmaps with prominent version banners.

The repository move is complete and reduces the cost of maintaining these
fixes. It does not change their runtime priority or substitute for focused
regression tests.
