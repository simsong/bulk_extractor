# bulk_extractor recent work report

**Period:** 18–21 July 2026

**Audience:** project sponsors, downstream users, and maintainers
**Evidence:** current `main` at `ce00e35`, merged GitHub PRs, closed issues, and the checked-in debt/test material. This is a status report, not a claim that hostile-input defects are exhausted.

## 1. Outcome and analysis performed

The work moved `bulk_extractor` to a more supportable 2.2.0 baseline: its former internal dependencies are now versioned with the product; build/release automation is consolidated; and the highest-risk findings from a full first-party source, build, CI, documentation, and test audit were converted into a tracked remediation program.

| Analysis | Result and action |
|---|---|
| Architecture/dependency audit | `be20_api`, DFXML, schemas, and UTF support were vendored with provenance and an explicit internal library manifest; the fragile recursive-submodule boundary was removed (PR [#498](https://github.com/simsong/bulk_extractor/pull/498)). |
| Hostile-input/core-invariant review | Found and repaired bounds, ownership, short-read, packet-address, E01-name, histogram, and shutdown defects; each change was tracked through an issue/PR where practical. |
| Build/CI/release review | Repaired bootstrap/distribution behavior, reduced duplicate workflow execution, made coverage uploads deterministic, and made AddressSanitizer part of each PR path. |
| Product/documentation review | Reconciled the technical-debt ledger, updated the 2.2.0 status, documented scanner lifecycle/template material, and made optional Exiv2 configuration testable. |
| Test/corpus review | Classified focused logic tests separately from legacy corpus smoke/presence tests; identified missing malformed-input, platform, optional-feature, and fuzz gates. |

## 2. Delivery record: issues closed and PRs merged

Nineteen PRs merged during the period. The table groups related deliveries; links provide the complete change record.

| Delivered capability | Merged PRs | Closed issues |
|---|---|---|
| In-tree dependency integration; 2.2.0 development/release/CI baseline | [#498](https://github.com/simsong/bulk_extractor/pull/498), [#514](https://github.com/simsong/bulk_extractor/pull/514), [#519](https://github.com/simsong/bulk_extractor/pull/519), [#520](https://github.com/simsong/bulk_extractor/pull/520) | — |
| Memory and input safety: `sbuf`, E01 selection/short reads, packet parsing, fallback PCAP | [#511](https://github.com/simsong/bulk_extractor/pull/511), [#516](https://github.com/simsong/bulk_extractor/pull/516), [#517](https://github.com/simsong/bulk_extractor/pull/517), [#518](https://github.com/simsong/bulk_extractor/pull/518), [#524](https://github.com/simsong/bulk_extractor/pull/524), [#530](https://github.com/simsong/bulk_extractor/pull/530) | [#501](https://github.com/simsong/bulk_extractor/issues/501), [#502](https://github.com/simsong/bulk_extractor/issues/502), [#504](https://github.com/simsong/bulk_extractor/issues/504), [#507](https://github.com/simsong/bulk_extractor/issues/507), [#508](https://github.com/simsong/bulk_extractor/issues/508), [#509](https://github.com/simsong/bulk_extractor/issues/509) |
| Reliable operation and user-visible options: disk-write shutdown, scanner selection, JPEG disable, banner | [#513](https://github.com/simsong/bulk_extractor/pull/513), [#525](https://github.com/simsong/bulk_extractor/pull/525), [#527](https://github.com/simsong/bulk_extractor/pull/527), [#533](https://github.com/simsong/bulk_extractor/pull/533) | [#370](https://github.com/simsong/bulk_extractor/issues/370), [#412](https://github.com/simsong/bulk_extractor/issues/412), [#480](https://github.com/simsong/bulk_extractor/issues/480), [#503](https://github.com/simsong/bulk_extractor/issues/503) |
| Feature recorder/histogram resilience | [#531](https://github.com/simsong/bulk_extractor/pull/531), [#535](https://github.com/simsong/bulk_extractor/pull/535) | [#510](https://github.com/simsong/bulk_extractor/issues/510), [#534](https://github.com/simsong/bulk_extractor/issues/534) |
| Maintainability and configuration | [#526](https://github.com/simsong/bulk_extractor/pull/526), [#528](https://github.com/simsong/bulk_extractor/pull/528), [#529](https://github.com/simsong/bulk_extractor/pull/529), [#532](https://github.com/simsong/bulk_extractor/pull/532) | [#420](https://github.com/simsong/bulk_extractor/issues/420), [#433](https://github.com/simsong/bulk_extractor/issues/433) |

## 3. Technical debt: erased versus remaining

**Erased or materially reduced.** The submodule/worktree failure mode; stale source-discovery build logic; duplicated/obsolete CI paths; unsafe `sbuf_t` boundary/ownership behavior; short-read tail exposure; unsafe E01/split-image selection; packet and fallback-PCAP bounds defects; the phase-one notifier shutdown failure; JPEG-disable and explicit-Outlook selection regressions; feature-recorder banner/histogram failures; and Exiv2-disabled build failure.

**Still open and material.** Parent tracker [#497](https://github.com/simsong/bulk_extractor/issues/497) remains open. The immediate choices are: complete or deliberately remove the runtime plug-in interface ([#505](https://github.com/simsong/bulk_extractor/issues/505)); port or retire source-broken Lightgrep ([#506](https://github.com/simsong/bulk_extractor/issues/506)); investigate zero-E01-chunk skipping without compromising forensic correctness ([#500](https://github.com/simsong/bulk_extractor/issues/500)); and finish scanner API documentation ([#522](https://github.com/simsong/bulk_extractor/issues/522)).

The broader ledger also retains real disk-write integration coverage, malformed-corpus/fuzzing, no-libpcap and Windows gates, focused tests for several binary scanners, legacy Python/Java support decisions, stale manuals, RAII conversion, and hostile-media parsing hardening. These should be scheduled rather than represented as completed by the recent fixes.

## 4. Regression-test assessment and test data

| Category | Valid evidence now | Invalid or insufficient as a regression claim |
|---|---|---|
| Focused repaired behavior | Catch tests assert `sbuf` zero/end/one-past-end and ownership behavior; fallback-PCAP tests reject oversized, inconsistent, and truncated packets; end-to-end tests cover E01/short reads, disk-write shutdown, `jpeg_carve_mode=0`, `-x all -e outlook`, plug-in loading, and banners; histogram failpoint test proves the triggering feature is retained. | A test that merely starts the executable or records coverage does **not** prove these behaviors. The new tests have explicit assertions on the repaired contract. |
| Legacy `tests/` corpus | `regress.py --datacheck` uses small curated files and checks 73 expected feature/offset entries; output sorting makes repeat comparisons possible. The E01 and raw fixtures exercise common image paths. | `--datacheck` verifies that listed features are present, not that extracted content is correct; its README documents obsolete 1.5.3 commands/paths. Manual crash/performance runs are smoke checks, not correctness tests, and Python regression tests are not an Automake target. |

**Data in use:** curated emails, Base64, PDF, Office/Outlook, ZIP/RAR/tar/gzip, JPEG/EXIF, ELF, Windows artifacts, UTF text, network captures, small E01 fixtures, EWF split segments, and raw image fragments in `tests/Data`, `tests/Images`, and `src/tests`.

**What it does not show:** proof across real case-scale media; malicious/malformed decoder inputs; all scanner parsers; no-libpcap fallback in required CI; Lightgrep; Windows parent builds; sanitizer/fuzz coverage beyond the exercised configurations; concurrency cancellation/backpressure; or correctness of every extracted feature. It must not be used as a release claim for those gaps.

## 5. Recommended next funded increment

1. Decide plug-in and Lightgrep product scope; either ship supported, versioned configurations with CI or remove unsupported promises.
2. Add curated malformed corpus plus fuzz/no-libpcap/Windows gates, beginning with recursive and binary parsers that consume hostile media.
3. Add a real disk-write failure integration test and complete scanner-specific boundary tests for the highest-risk untested parsers.
4. Reduce operational risk: retire stale manuals/support tools, publish the current test procedure, and continue the ledger under [#497](https://github.com/simsong/bulk_extractor/issues/497).
