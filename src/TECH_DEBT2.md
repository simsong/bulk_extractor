# Technical-debt remediation checklist

This checklist is derived from `TECH_DEBT.md`. Each unchecked line identifies one independently reviewable defect or debt item; closing an item requires a substantive fix and appropriate validation.

## P0: memory safety and termination

- [ ] Reject PCAP records whose `caplen` exceeds the buffer allocated from `snaplen`.
- [ ] Impose a defensible upper bound on input-controlled PCAP `snaplen` allocations.
- [ ] Make `sbuf_t::memcmp` compare byte zero instead of beginning at byte one.
- [ ] Make zero-length `sbuf_t::memcmp` return equality without underflowing its length.
- [ ] Make `sbuf_t::wbuf` reject the one-past-end index `i == bufsize`.
- [ ] Remove the ineffective negative-index test on unsigned `sbuf_t::wbuf` indexes.
- [ ] Correct `sbuf_t` slice primary-page arithmetic so an in-page offset reduces `pagesize`.
- [ ] Make `sbuf_t` slices beginning in the margin have a zero primary-page size instead of an underflowed size.
- [ ] Prevent `sbuf_t` child constructors from forming pointers beyond the source buffer when an offset is out of range.
- [ ] Replace overflow-prone `i + width > bufsize` checks in `sbuf_t` integer readers with subtraction-based checked ranges.
- [ ] Cast bytes to unsigned destination widths before shifting in `sbuf_t` integer readers.
- [ ] Ensure every phase-1 `DiskWriteError` path stops and joins the notification thread without asserting or hanging.

## P1: ownership, input handling, and advertised features

- [ ] Check `open()` failure before attempting to map a file in `sbuf_t::map_file`.
- [ ] Check `mmap()` for `MAP_FAILED` before constructing a mapped `sbuf_t`.
- [ ] Treat file descriptor zero as an owned descriptor that must be closed.
- [ ] Define a safe, portable zero-length file mapping path.
- [ ] Retain and free the allocated buffer in the no-`mmap` `map_file` implementation.
- [ ] Check allocation failure before constructing an `sbuf_t` in `sbuf_malloc`.
- [ ] Replace discarded `std::runtime_error` temporaries in the `sbuf_t` destructor with enforceable invariants or diagnostics.
- [ ] Insert rather than erase non-owning `sbuf_t` instances in debug leak tracking.
- [ ] Make `range_exception_t::what()` thread-safe instead of returning a shared mutable static buffer.
- [ ] Either implement `scanner_set::add_scanner_file` or remove the nonfunctional API.
- [ ] Either implement `scanner_set::add_scanner_directory` or remove the nonfunctional API.
- [ ] Remove or implement the unused `BE_PATH` scanner search path.
- [ ] Remove or implement the unused `-P` scanner-directory option.
- [ ] Process every supplied `-P` value instead of stopping after the first.
- [ ] Remove plug-in claims from user/API documentation unless a versioned, tested plug-in ABI is implemented.
- [ ] Remove the duplicate `PHASE_INIT` case in `scan_lightgrep.cpp`.
- [ ] Correct the inconsistent pointer/member access in `scan_lightgrep.cpp`.
- [ ] Remove references to the deleted recursion-control interface from Lightgrep scanner sources.
- [ ] Fix the undefined `rcb` argument in `scan_email_lg.cpp`.
- [ ] Register working Lightgrep scanners when Lightgrep is enabled, or remove the build option.
- [ ] Port or archive `doc/writing_pattern_scanners.md` with the obsolete Lightgrep implementation.
- [ ] Handle positive short reads in the EWF image reader without exposing uninitialized tail bytes.
- [ ] Use the actual `gcount()` result or a read-exact loop in the raw image reader.
- [ ] Recognize lowercase `.e01` extensions correctly instead of comparing the extension with `e01`.
- [ ] Make E01 directory/segment detection case-insensitive or explicitly documented.
- [ ] Stop treating user-controlled split-image filenames as `snprintf` format strings.
- [ ] Use RAII in `image_process::open()` so failed or throwing implementations do not leak.
- [ ] Fix IPv4 packet address accessors that apply byte offsets as `in_addr` element offsets.
- [ ] Fix IPv6 packet address accessors that apply byte offsets as `ip6_addr` element offsets.
- [ ] Replace potentially unaligned Ethernet/IP structure casts with checked byte-wise parsing.
- [ ] Validate enough Ethernet bytes for the complete structure actually accessed.
- [ ] Stop dereferencing `line.end()` in `feature_recorder_file::banner_stamp`.
- [ ] Implement or remove the advertised `histograms_write_largest` low-memory mechanism.
- [ ] Clear and rewind file-based histogram streams before retrying after EOF.

## P2: phase processing and scanner correctness

- [ ] Make random sampling choose only valid block indexes below `max_blocks`.
- [ ] Accept a sampling fraction of exactly one or correct the documented valid range.
- [ ] Use or remove the parsed `sampling_passes` setting.
- [ ] Expose and document the sampling random seed if deterministic sampling is intended.
- [ ] Separate typed read failures from scanner and output exceptions in phase-1 scheduling.
- [ ] Replace phase-1 disk-error `exit(1)` calls with structured shutdown and consistent return semantics.
- [ ] Assign and report `Config::max_wait_time`, or remove the unused `max_minute_wait` option.
- [ ] Assign `Phase1::image_hash` so the console image-hash result is reachable, or remove the dead output branch.
- [ ] Add malformed-corpus and fuzz coverage for recursive Base64, gzip, hiberfile, PDF, RAR, XOR, and ZIP decoders.
- [ ] Replace hostile-input native-structure casts incrementally in ELF, EVTX, EXIF, network, NTFS, Outlook, SQLite, UTMP, WinLNK, WinPE, and WinPrefetch scanners.
- [ ] Free or transfer ownership of every synthetic EVTX header allocation.
- [ ] Eliminate unsupported unfinished SQL behavior in the wordlist scanner.
- [ ] Remove global mutable scanner state from the find scanner.
- [ ] Complete or explicitly mark the vCard scanner as experimental.
- [ ] Classify `ccns2`, header prototypes, `lg_patterns`, `jpeg_dump`, `stand`, and `old_scanners` as production, experimental, generated, or archival.
- [ ] Replace assertion-based hostile-media validation with checked runtime failures.
- [ ] Convert owning raw pointers and manual child/reference counts to RAII ownership.
- [ ] Define deterministic directory traversal ordering and explicit symlink/permission behavior.
- [ ] Replace repeated linear split-image segment lookup in `find_offset()` with an ordered lookup.
- [ ] Standardize mixed `exit`, `err`, exception, file-size, and read-count conventions.

## Build and dependency debt

- [x] Replace recursive be20_api source discovery with an explicit reviewed production source manifest.
- [x] Remove test-only `test_image_reader.cpp` and `scan_sha1_test.cpp` from the production executable source set unless they are intentional runtime features.
- [x] Stop generated `config.h` files from entering source/distribution manifests.
- [ ] Prevent generated Autotools files from retaining absolute package-manager Cellar paths.
- [x] Correct be20_api's C++20 comment or actually require C++20 instead of C++17.
- [x] Make missing RE2 a configure error because production headers require it.
- [x] Remove the duplicate `HAVE_RE2` definition.
- [x] Stop sorting compiler and linker flags where ordering is semantically significant.
- [ ] Remove dead shared-library and Python 2.7 installation logic from the top-level Makefile.
- [ ] Replace mutable platform-provisioning scripts with reproducible dependency declarations.
- [ ] Remove hard-coded Homebrew prefixes and repeated shell-startup edits from the macOS setup script.
- [ ] Retire or clearly archive obsolete Fedora, CentOS, Debian, and Amazon Linux setup scripts.

## Test debt

- [ ] Replace the empty thread-pool test and unconditional 60-second watchdog sleep with substantive deterministic work, cancellation, and exception tests.
- [ ] Stop setting `DEBUG_FAST=TRUE` in the only required parent CI gate, or add a required full test gate.
- [ ] Add focused parser and boundary tests for EVTX, NTFS, Outlook, SQLite, UTMP, WinLNK, WinDirs, and optional scanners.
- [ ] Connect supported Python tests to a maintained Makefile test target.
- [ ] Fix or remove the ThreadSanitizer workflow step whose condition can never match.
- [ ] Add required no-libpcap, malformed-PCAP, and fallback packet-reader tests.
- [ ] Add required supported Lightgrep tests or delete Lightgrep support.
- [ ] Add a supported Windows parent-build test.
- [ ] Add continuous malformed-corpus fuzz targets for the highest-risk parsers.
- [ ] Add boundary tables for zero, end, one-past-end, `SIZE_MAX`, page/margin crossings, and zero-length `sbuf_t` operations.
- [ ] Add a real disk-write failure integration test that verifies orderly worker and notifier shutdown.

## Python, Java, and support-tool debt

- [ ] Decide and document which Python utilities remain supported.
- [ ] Port supported Python utilities from Python 2 constructs to one Python 3 baseline.
- [ ] Declare the non-standard `path`, NumPy, and Matplotlib dependencies used by supported tools.
- [ ] Package supported Python entry points instead of relying on ad hoc scripts.
- [ ] Move unsupported Python artifacts to a clearly labeled archive.
- [ ] Remove, update, or archive the legacy Java configuration that still identifies version 1.5.5.

## Documentation debt

- [x] Update `README.md` from “2.1 development” to the actual 2.1.1 version/status.
- [x] Refresh the README's supported-platform information beyond its 2023 snapshot.
- [x] Correct README debug-variable names to `DEBUG_BENCHMARK` and `DEBUG_SCANNER_DUMP_DATA`.
- [x] Correct the README's comma-separated scanner-ignore claim to match substring behavior, or fix the implementation.
- [x] Replace stale `be13_api` names in release and build documentation.
- [x] Tie Windows support claims to an active parent CI build or qualify them.
- [x] Correct the stale `src/be13_api` submodule key in `.gitmodules` while the submodule exists.
- [ ] Consolidate `src/DEBUG.md` and README debug settings into one accurate table.
- [ ] Generate or maintain `src/scanners.md` from actual scanner registration metadata.
- [ ] Rewrite `tests/README.md` for 2.1.1, `make check`, current fixtures, and `DEBUG_FAST` behavior.
- [ ] Rewrite `python/README.txt` after supported Python tools are selected.
- [ ] Re-inventory stale first- and third-party paths and notices in `LICENSE.md`.
- [ ] Regenerate or archive the version-1.5 help captured in `doc/latex_manuals/BEUsage.txt`.
- [ ] Port or archive the programmer manual's obsolete two-argument scanner ABI and plug-in instructions.
- [ ] Port or archive the 1.4/1.5 user manuals and worked examples.
- [x] Remove the obsolete `src/be13_api` include path from `doc/Makefile.am`.
- [ ] Mark the 1.4, 1.5, and 1.6 roadmaps as historical.
- [ ] Mark or replace `ROADMAP_2.0.md`, which mixes obsolete plans and completed work.
- [ ] Convert root and be20_api TODO/status work logs into a current owned backlog or archive them.
- [ ] Reconcile root and be20_api NEWS/ChangeLog histories into a coherent release history.
- [x] Rewrite the be20_api README so it does not claim an unchanged 1.3-era API.
- [x] Remove the be20_api README claim that runtime DLL/shared-library scanners work like built-ins.
- [x] Remove the be20_api README claim that its build produces `be20_api.a`, or add the library target.
- [x] Replace be20_api README references to the `master` branch with the actual supported branch.
- [x] Correct the `tcplow` typo and unfinished be20_api README text.
- [x] Define the supported be20_api public-header and versioning policy.
- [x] Point the be20_api coverage badge at the correct repository/branch.
- [x] Replace the obsolete libcester claim in `src/be20_api/doc/unit-tests.txt` with the actual Catch/Makefile test path.
- [ ] Consolidate stale be20_api INSTALL, TODO, and status notes into an accurate component guide.

## CI and release-workflow debt

- [ ] Consolidate the substantially duplicated `ci-cd.yml` and `continuous-integration-pip.yml` workflows.
- [ ] Replace deprecated GitHub Actions `::set-output` usage.
- [ ] Replace nonexistent `steps.ctx.outputs.version` references with the actual version-step output.
- [ ] Fix the operand-less `chmod +x` command in `create-release-installer.yml`.
- [ ] Update obsolete action versions in the release workflow.
- [ ] Add the missing release-creation step or remove its nonexistent `upload_url` reference.
- [x] Make Coverity checkout the direct dependency submodules while submodules remain.
- [ ] Stop pinning third-party actions to mutable `master` branches.
- [ ] Remove obsolete PCRE installation from the be20_api macOS workflow.
- [ ] Rename the be20_api Codecov artifact from the unrelated `sleuthkit-codecov` name.
- [ ] Add a lightweight workflow syntax and reference validation gate.

## Repository integration debt

- [x] Eliminate the `src/be20_api` Git-link boundary now that no independent be20_api consumer remains.
- [x] Preserve be20_api source provenance and license history when importing it into bulk_extractor.
- [x] Resolve the non-`main` `625f5b3` be20_api revision intentionally during import.
- [x] Eliminate or deliberately top-level the nested dfxml_cpp and utfcpp Git links.
- [x] Build be20_api as an explicit internal component rather than compiling recursively discovered files into every target.
- [x] Make bulk_extractor and component tests consume the same explicit be20 source target.
- [x] Remove obsolete standalone be20_api bootstrap/configure/CI files after integration.
- [x] Keep the independently maintained root `dfxml_schema` repository as an intentional direct parent submodule.
