[![codecov](https://codecov.io/gh/simsong/bulk_extractor/branch/main/graph/badge.svg?token=3w691sdgLu)](https://codecov.io/gh/simsong/bulk_extractor)
<a href="https://scan.coverity.com/projects/simsong-bulk_extractor">
  <img alt="Coverity Scan Build Status" src="https://scan.coverity.com/projects/29726/badge.svg"/></a>

`bulk_extractor` is a high-performance digital forensics exploitation
tool.  It is a "get evidence" button that rapidly scans any kind of
input (disk images, files, directories of files, etc) and extracts
structured information such as email addresses, credit card numbers,
JPEGs and JSON snippets without parsing the file system or file system
structures. The results are stored in text files that are easily
inspected, searched, or used as inputs for other forensic
processing. bulk_extractor also creates histograms of certain kinds of
features that it finds, such as Google search terms and email
addresses, as previous research has shown that such histograms are
especially useful in investigative and law enforcement applications.

Unlike other digital forensics tools, `bulk_extractor` probes every byte of data to see if it is the start of a
sequence that can be decompressed or otherwise decoded. If so, the
decoded data are recursively re-examined. As a result, `bulk_extractor` can find things like BASE64-encoded JPEGs and
compressed JSON objects that traditional carving tools miss.

This source tree builds bulk_extractor 2.2.0. For production use, prefer a tested release from https://github.com/simsong/bulk_extractor/releases.

Building `bulk_extractor`
=========================
We recommend building from sources. We provide a number of `bash` scripts in the `etc/` directory that will configure a clean virtual machine:

```
git clone https://github.com/simsong/bulk_extractor.git
./bootstrap.sh
./configure
make
make check
make install
```

For detailed instructions on installing packages and building bulk_extractor, read the wiki page here:
https://github.com/simsong/bulk_extractor/wiki/Installing-bulk_extractor

For more information on bulk_extractor, visit: https://forensics.wiki/bulk_extractor

Documentation
=============

The generated PDF manuals are published at
https://simsong.github.io/bulk_extractor/ after changes merge to `main`.
It contains the 2.2 operating and developer manuals. Pull requests receive the
same PDFs as a workflow artifact.

Tested Configurations
=====================
This release of bulk_extractor requires C++17. Current validation covers:

* Apple Silicon macOS (local build and `make distcheck`, 2026-07-19)
* Ubuntu 22.04 and current macOS GitHub Actions runners

Older platform preparation scripts under `etc/` are not equivalent to current
CI support and may require maintenance.

Tested Configurations Which bulk_extractor Does Not Work
========================================================
* Debian 10 (not supported for native builds)

RECOMMENDED CITATION
====================
If you are writing a scientific paper and using bulk_extractor, please cite it with:

Garfinkel, Simson, Digital media triage with bulk data analysis and bulk_extractor. Computers and Security 32: 56-72 (2013)
* [Science Direct](https://www.sciencedirect.com/science/article/pii/S0167404812001472)
* [Bibliometrics](https://plu.mx/plum/a/?doi=10.1016/j.cose.2012.09.011&theme=plum-sciencedirect-theme&hideUsage=true)
* [Author's website](https://simson.net/clips/academic/2013.COSE.bulk_extractor.pdf)
```
@article{10.5555/2748150.2748581,
author = {Garfinkel, Simson L.},
title = {Digital Media Triage with Bulk Data Analysis and Bulk_extractor},
year = {2013},
issue_date = {February 2013},
publisher = {Elsevier Advanced Technology Publications},
address = {GBR},
volume = {32},
number = {C},
issn = {0167-4048},
journal = {Comput. Secur.},
month = feb,
pages = {56–72},
numpages = {17},
keywords = {Digital forensics, Bulk data analysis, bulk_extractor, Stream-based forensics, Windows hibernation files, Parallelized forensic analysis, Optimistic decompression, Forensic path, Margin, EnCase}
}
```

ENVIRONMENT VARIABLES
=====================
The following environment variables can be set to change the operation of `bulk_extractor`:

|Variable|Behavior|
|--------|--------|
|`DEBUG_BENCHMARK`|Include CPU benchmark information in `report.xml` file|
|`DEBUG_NO_SCANNER_BYPASS`|Disables scanner bypass logic that bypasses some scanners if an sbuf contains ngrams or does not have a high distinct character count.|
|`DEBUG_HISTOGRAMS`|Print debugging information on file-based histograms.|
|`DEBUG_HISTOGRAMS_NO_INCREMENTAL`|Do not use incremental, memory-based histograms.|
|`DEBUG_PRINT_STEPS`|Prints to stdout when each scanner is called for each sbuf|
|`DEBUG_SCANNER_DUMP_DATA`|Hex-dump each sbuf that is to be scanned.|
|`DEBUG_SCANNERS_IGNORE`|A substring used to identify scanners to ignore. Useful for debugging unit tests.|

Other hints for debugging:

* Run -xall to run without any scanners.
* Run with a random sampling of 0.001% to debug reading image size and a few quick seeks.

LOADABLE SCANNERS
=================
bulk_extractor loads scanner modules named scan_*.so (or scan_*.dylib on
macOS and scan_*.dll on Windows) from the directories supplied with -P or in
BE_PATH. A module exports this C-linkage factory:

    extern "C" scanner_t *bulk_extractor_scanner_v1();

The factory returns a normal scanner_t function. Build modules against the
same bulk_extractor source version as the executable; the scanner's PHASE_INIT
handler must call sp.check_version(). Modules remain loaded until scanner
cleanup completes.

CONTAINER IMAGE
===============

The source tree includes a multi-stage Debian Bookworm `Dockerfile` for
reproducible Linux scans of regular image files. Build and run it with an
input mounted read-only and an output directory mounted read-write. The image
runs unprivileged and intentionally omits libewf and Lightgrep; it is not a
privileged raw-device appliance. See [doc/docker.md](doc/docker.md).

BUILDING ON WINDOWS
===================
Native Windows builds of bulk_extractor are not currently supported.

The `Windows MinGW build` GitHub Actions workflow cross-compiles the executable
on Ubuntu for relevant non-draft pull requests and uploads `bulk_extractor64.exe` in the
`bulk_extractor-windows-x86_64` artifact. The workflow verifies that the PE
executable does not import the MinGW, RE2, Abseil, Expat, zlib, or GNU crypto
runtime DLLs. A Windows runner downloads and runs that exact artifact against
a directory with a Unicode filename. The workflow uses the x86_64 MinGW-w64
POSIX toolchain and static Expat, RE2, and Abseil from a pinned vcpkg checkout.
The CI artifact is currently built without libewf, so it does not include E01
support, is not signed, and is not a release installer. The workflow file and
its maintained build notes are `.github/workflows/mingw.yml` and
`doc/mingw_notes.md`.

WINDOWS RAW DEVICES
===================
On Windows, the executable can scan a raw physical disk, volume, or named
volume by passing its DOS device path as the input. Supported forms are
`C:` (a shorthand for `\\.\C:`), `\\.\PhysicalDriveN`, `\\.\X:`, and
`\\?\Volume{GUID}` (without the trailing
backslash used for a mounted-volume directory). For example:

    bulk_extractor64.exe -o output \\.\PhysicalDrive0

Run from an elevated command prompt and use a separate output directory. The
input is opened read-only with shared read/write access; bulk_extractor does
not lock, mount, alter, or write the device. It obtains the exact length with
the Windows disk-length control code and reads through Win32 handles rather
than treating a device as a C++ regular file. A volume path is not equivalent
to its containing physical disk and may be subject to Windows volume-boundary
rules near its final sectors. See `doc/mingw_notes.md` for operational limits.




BULK_EXTRACTOR RELEASE NOTES
============================

## Release 2.2.0 (July 19, 2026)

Integrated the be20 API and its source dependencies into the bulk_extractor
source tree, eliminating recursive submodule setup. The unified build now
validates bulk_extractor, be20, and DFXML together and fixes thread-pool
shutdown defects found by full-image testing and AddressSanitizer.

## Release 2.1.1 (April 26, 2024)
Renamed jpeg_carved feature recorder to jpeg, so that the jpeg carve mode can be set with -S jpeg_carve_mode=2, rather than -S jpeg_carved_carve_mode=2, which was confusing.

See [Carving](doc/carving.md) for the current per-recorder carve-mode settings
and their effects.


## Release 2.0
`bulk_extractor` 2.0 (BE2) is now operational. Although it works with the Java-based viewer, we do not currently have an installer that runs under Windows.

BE2 requires C++17 to compile. The be20 scanner API, dfxml_cpp, utfcpp, and DFXML schema sources are maintained directly in this repository; no recursive submodule checkout is required.

The project took longer than anticipated. In addition to updating to C++17, It was used as an opportunity for massive code refactoring and general increase in code quality, testability and reliability. An article about the experiment will appear in a forthcoming issue of [ACM Queue](https://queue.acm.org/)
