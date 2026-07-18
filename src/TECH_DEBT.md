# bulk_extractor and be20_api source and technical-debt audit

Date: 2026-07-18

## Executive conclusions

This audit recommends bringing `src/be20_api` back into the
`bulk_extractor` repository while retaining it as a distinct internal
component. The present submodule boundary provides little isolation:
`bulk_extractor` includes be20_api's Autoconf macros and private headers and
compiles be20_api's `.cpp` files directly into both `bulk_extractor` and
`test_be`. It does not consume a released or installed be20_api library. The
boundary therefore adds nested-submodule, version-skew, CI, and developer
workflow costs without enforcing a binary or source API.

The desirability of reintegration is **high** and its mechanical feasibility
is **high**. Overall migration risk is **moderate** because the repositories
have separate histories, be20_api has two nested submodules, the parent pins a
non-`main` be20_api commit, and be20_api may have consumers outside this
checkout. The correct target is an in-tree library or object-library component,
not an undifferentiated copy of the files.

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
- parent and nested submodule state, source-list generation, configure-time
  composition, test entry points, and workflows.

“Complete” here means that no tracked source area was excluded. It does not
mean that static inspection proves the absence of defects. Dynamic sanitizer,
fuzz, Windows, Lightgrep, and no-libpcap coverage was unavailable in this
checkout, as described under **Validation status**. The default EWF path was
built and exercised by the E01 test fixture.

The parent was inspected at its current `main` checkout. At the start of the
audit, its gitlink recorded be20_api commit
`625f5b3ddd9d6baf4aaa7c10a6cefb5e2bfb1a8b`, while the be20_api worktree was
at clean `main` commit
`00f03b21b9f3da8c613828c6fb5ab05b8f661a79`. That mismatch is material:
`625f5b3` is three commits beyond `00f03b2` on the separate `fix-noreturn`
branch and includes RE2-related build fixes. Findings were checked against the
files present there. The build repair subsequently reconciled the checkout to
the parent's recorded `625f5b3`; reintegration should use that commit unless a
separate dependency-update decision selects another revision.

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

### The boundary is already porous

The following facts make be20_api an in-tree implementation dependency in all
but Git layout:

- `configure.ac` directly `m4_include`s be20_api and dfxml_cpp macros.
- `src/Makefile.am` adds be20_api and utfcpp private include directories.
- `etc/makefile_builder.py` walks `src/be20_api` and emits all of its `.cpp` and
  `.h` files into the parent's generated `src/Makefile.auto_defs`.
- `bulk_extractor` and `test_be` compile those files directly. There is no
  be20_api link dependency.
- The standalone be20_api `Makefile.am` builds `test_be20_api`, not the
  `be20_api.a` promised by its README.
- Parent code directly names be20_api types throughout its public and private
  implementation.

This coupling means a submodule update can change the parent's source set and
configure behavior without a versioned interface, while a parent checkout can
simultaneously pin a be20_api revision that is not on be20_api `main`.

## Reintegration assessment

### Desirability: high

Reintegration would:

- make a parent commit self-contained and remove the current gitlink/worktree
  skew;
- eliminate the most failure-prone nested checkout and bootstrap sequence;
- let source, tests, CI, API documentation, and versioning change atomically;
- replace filesystem discovery of implementation files with an explicit source
  manifest and component target;
- expose optional configurations to the same CI matrix as the executable;
- make ownership and deprecation decisions visible in one issue tracker; and
- remove misleading independence: today be20_api has neither an installed
  artifact nor a compatibility gate.

The strongest argument against reintegration would be an active independent
consumer that tracks be20_api releases. This checkout provides claims that
tcpflow can consume it, but no consumer manifest, packaged headers, install
target, semantic-version policy, or compatibility tests. That claim must be
verified against the actual external repositories before deleting the separate
repository.

### Feasibility: high, with moderate migration risk

The code is already compiled in the desired physical arrangement, so moving it
does not require an ABI conversion. The risks are repository and product risks:

- The parent wants `625f5b3` but be20_api `main` is `00f03b2`. Importing the
  worktree blindly would discard fixes expected by the parent.
- be20_api contains nested dfxml_cpp and utfcpp gitlinks. Their canonical
  versions and future update mechanism must be chosen explicitly.
- Separate history should be retained for blame, license provenance, and
  bisectability.
- Root and be20_api license texts are broadly compatible (US-government/public
  domain material plus MIT and separately identified third-party code), but
  both manifests contain stale paths and descriptions. Third-party notices must
  be normalized during the move, not lost.
- Any real external consumer needs either a maintained standalone release or a
  supported extraction mechanism.

### Recommended target

Import the canonical be20_api history under a stable path such as
`src/be20/`. Define one explicit `libbe20` convenience/static target (or an
Automake object library) with a reviewed public-header list. Link
`bulk_extractor`, its tests, and any scanner test harness against that target.
Keep the namespace/component boundary, but remove the Git submodule boundary.

Do not continue generating the source list by recursively walking the tree.
An explicit source list prevents test programs, demos, broken prototypes, or
new upstream files from silently entering production builds.

If an external consumer is confirmed, the preferred monorepo outcome is still
possible: install the documented public headers and library, run a small
consumer build in CI, and optionally publish a read-only split of `src/be20`.
Keeping a writable submodule is not required to preserve reuse.

### Migration sequence

1. Reconcile `00f03b2`, `625f5b3`, and be20_api remote branches. Make the
   intended source state build and test standalone before importing it.
2. Identify actual consumers. Record which headers and behaviors are public;
   treat everything else as private.
3. Import be20_api history with a prefix-preserving history operation. Avoid a
   one-time untraceable file copy.
4. Resolve dfxml_cpp and utfcpp deliberately: vendor pinned source with notices,
   use one top-level submodule each, or use a package dependency. Do not retain
   hidden nested gitlinks.
5. Add the explicit `libbe20` target and make both parent and be20 tests link it.
   Remove recursive source discovery.
6. Make required and optional configurations green: default, ASan/UBSan,
   no-libpcap, EWF, SQLite, Exiv2, RAR, Lightgrep (or delete Lightgrep), macOS,
   Linux, and Windows where supported.
7. Merge READMEs, versioning, release notes, CI, and issue tracking. Mark legacy
   manuals and roadmaps archival.
8. In one final parent commit, replace the gitlink, remove the misleading
   `src/be13_api` submodule name, and document the new update/release process.

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

- The generated source manifest recursively includes every be20_api `.cpp` and
  `.h`. Production membership changes implicitly when a file is added. The
  current production list even contains `test_image_reader.cpp`,
  `scan_sha1_test.cpp`, and a generated `config.h`.
- Generated and ignored Autotools state can silently retain absolute package
  manager paths. The initial checkout demonstrated this with stale RE2 and
  Abseil paths; clean regeneration removed them from the active build.
- be20_api `configure.ac` says “Enforce C++20” but requests C++17.
- `be20_configure.m4` detects missing RE2 only with a notice, although
  `regex_vector.h` includes RE2 unconditionally; it also defines `HAVE_RE2`
  twice.
- Configure scripts sort and de-duplicate compiler/linker flags. Flag order can
  be semantically significant, especially for libraries and paired options.
- Top-level `Makefile.am` retains commented/dead shared-library and Python 2.7
  install logic.
- Until the build repair accompanying this review, `bootstrap.sh` checked
  duplicate paths and the obsolete/nonexistent nested `utfcpp/extern/ftest`
  path, retried submodule updates from inside its loop, and could continue
  after a failed generator command. It now validates the actual submodule
  layout once, fails fast, and delegates regeneration to `autoreconf`.
- Platform setup scripts are mutable machine provisioning scripts rather than
  reproducible dependency declarations. The macOS script hard-codes Homebrew
  paths and edits shell startup state; older Fedora/CentOS scripts remain
  alongside the active configurations.

### P2: test debt

The existing Catch tests contain valuable sbuf, utility, scanner, and end-to-end
logic checks. The debt is coverage selection and configuration, not simply test
count:

- The be20 thread-pool test launches an empty pool, joins it, and then waits for
  a watchdog that always sleeps 60 seconds. It does not schedule or verify work,
  cancellation, backpressure, or exception propagation. Replace it with
  deterministic substantive concurrency tests.
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
| `.gitmodules` | The key is `src/be13_api` while the path and repository are be20_api. | Rename during reintegration or immediately when the gitlink is next changed. |
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
| `doc/Makefile.am` | Still adds a `src/be13_api` include path. |

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

`src/be20_api/README.md` has the highest concentration of contradictions:

- It says be20_api was developed for bulk_extractor 1.3 and used unchanged
  through 2.0, while the same file says BE13_API was renamed and the source has
  a 2.x scanner lifecycle.
- It says runtime shared-library/DLL scanners are equivalent to built-ins; the
  loader always throws.
- It says the build makes `be20_api.a`; `Makefile.am` only builds the test
  executable.
- It directs users to pull `master`; the checkout uses `main`.
- It names `tcplow`, has an unfinished sentence, and gives no supported public
  header/version policy.
- Its `main` coverage badge points at `slg-dev`.

`src/be20_api/doc/unit-tests.txt` says libcester is in use, while the tests use
the bundled Catch header. `INSTALL`, `TODO.md`, and the status report are stale
development notes. Replace these with one accurate component README covering
scope, public headers, build integration, ownership, and test commands.

### CI as executable documentation

The workflows make claims about supported builds and releases, but several are
known-invalid:

- `ci-cd.yml` and `continuous-integration-pip.yml` substantially duplicate one
  another.
- Both use deprecated `::set-output`, then upload an artifact using nonexistent
  `steps.ctx.outputs.version` instead of `steps.extract_version.outputs.version`.
- `create-release-installer.yml` runs `chmod +x && bootstrap.sh` (missing the
  chmod operand), uses old action versions, and references
  `steps.create_release.outputs.upload_url` without a `create_release` step.
- Coverity checks out `actions/checkout@main` and does not request recursive
  submodules.
- Some third-party actions are pinned to mutable `master` branches.
- be20_api's macOS workflow still installs PCRE although its NEWS says RE2
  replaced PCRE, and its Codecov upload is named `sleuthkit-codecov`.

Consolidate to one required build matrix, one coverage workflow if needed, and
one tested release workflow. Pin actions to maintained major versions or commit
SHAs and add a lightweight workflow syntax/reference check.

## Explanation of the submodule update error

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

The obstruction is not present in the current worktree: the workflow file and
the be20_api index are clean. At the start of the audit, the parent and
worktree differed:

```text
parent gitlink:             625f5b3ddd9d6baf4aaa7c10a6cefb5e2bfb1a8b
initial submodule worktree: 00f03b21b9f3da8c613828c6fb5ab05b8f661a79
```

That was why the parent reported `M src/be20_api` and `git submodule status`
prefixed it with `+`: the parent expected the `fix-noreturn` descendant rather
than the checked-out be20_api `main` tip. Running
`git submodule update --init --recursive` after confirming the workflow file
was clean detached be20_api at `625f5b3`, as recorded by the parent. The parent
and all nested submodules now have clean, unprefixed status.

Before retrying, inspect all three states:

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

The final command should detach `src/be20_api` at `625f5b3`, which is normal for
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
  dependencies. The directly relevant versions include RE2 2025-11-05,
  Abseil 20260107.1, Automake 1.18.1_1, Autoconf 2.73, libtool 2.6.2,
  libxml2 2.15.3, libewf 20140816, json-c 0.19, expat 2.8.2, and Exiv2 0.28.8.
- `./bootstrap.sh`, `./configure`, and `make -j4` complete successfully.
- `make check` passes its one registered test program (`test_be`: 1 pass, no
  skips or failures). With `DEBUG_FAST` unset, the segmented 30 MB image, E01,
  email, scanner, and other non-fast paths execute.
- `make distcheck` passes the clean archive configure/build/test,
  install/uninstall, and redistribution checks and produces
  `bulk_extractor-2.1.1.tar.gz`.
- `src/bulk_extractor` links the installed RE2, Abseil, libewf, and other
  current Homebrew libraries rather than the stale Cellar versions.

This establishes a reproducible default build baseline. It does not invalidate
the static defects above; those still require focused regression tests and the
optional/sanitizer configurations recommended in this document.

## Recommended order of work

1. **Contain P0 risk:** fix and test PCAP length validation, all sbuf boundary
   primitives, and disk-error thread shutdown. Add ASan/UBSan gates.
2. **Preserve the restored build baseline:** keep be20_api at an intentional
   parent-recorded commit, prevent generated dependency paths from entering
   source control, and retain `make check` plus `make distcheck` as release
   gates.
3. **Resolve false features:** either implement or remove plug-ins and
   Lightgrep. Correct the CLI and docs in the same changes.
4. **Repair input/ownership paths:** short reads, E01 detection, split filename
   construction, file mapping, packet accessors, histogram fallback, and EVTX
   ownership.
5. **Reintegrate be20_api:** preserve history, add an explicit component target,
   flatten dependency management, and delete recursive source discovery.
6. **Consolidate tests and CI:** exercise real optional configurations and
   malformed corpora; remove duplicate/broken workflows and pro-forma tests.
7. **Rebuild documentation:** one current README/man page/component guide;
   archive 1.x manuals and historical roadmaps with prominent version banners.

This ordering deliberately separates urgent runtime correctness from the
repository move. Reintegration will reduce the cost of maintaining the fixes,
but it should not delay them or obscure their review history.
