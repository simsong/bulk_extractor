[![codecov](https://codecov.io/gh/simsong/bulk_extractor/branch/main/graph/badge.svg?token=3w691sdgLu)](https://codecov.io/gh/simsong/bulk_extractor)

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

This is the `bulk_extractor` 2.1 development branch! It is reliable, but if you want to have a well-tested production quality release, download a release from https://github.com/simsong/bulk_extractor/releases.

Building `bulk_extractor`
=========================
We recommend building from sources. We provide a number of `bash` scripts in the `etc/` directory that will configure a clean virtual machine:

```
git clone --recurse-submodules https://github.com/simsong/bulk_extractor.git
./bootstrap.sh
./configure
make
make install
```

For detailed instructions on installing packages and building bulk_extractor, read the wiki page here:
https://github.com/simsong/bulk_extractor/wiki/Installing-bulk_extractor

For more information on bulk_extractor, visit: https://forensics.wiki/bulk_extractor

Tested Configurations
=====================
This release of bulk_extractor requires C++17 and has been tested to compile on the following platforms:

* Amazon Linux as of 2023-05-25
* Fedora 36 (most recently)
* Ubuntu 20.04LTS
* MacOS 13.2.1

You should *always* start with a fresh VM and prepare the system using the appropriate prep script in the `etc/` directory.

Tested Configurations Which bulk_extractor Does Not Work
========================================================
* Debian 10 (is not supported for native builds))

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
|`DEBUG_BENCHMARK_CPU`|Include CPU benchmark information in `report.xml` file|
|`DEBUG_NO_SCANNER_BYPASS`|Disables scanner bypass logic that bypasses some scanners if an sbuf contains ngrams or does not have a high distinct character count.|
|`DEBUG_HISTOGRAMS`|Print debugging information on file-based histograms.|
|`DEBUG_HISTOGRAMS_NO_INCREMENTAL`|Do not use incremental, memory-based histograms.|
|`DEBUG_PRINT_STEPS`|Prints to stdout when each scanner is called for each sbuf|
|`DEBUG_DUMP_DATA`|Hex-dump each sbuf that is to be scanned.|
|`DEBUG_SCANNERS_IGNORE`|A comma-separated list of scanners to ignore (not load). Useful for debugging unit tests.|

Other hints for debugging:

* Run -xall to run without any scanners.
* Run with a random sampling of 0.001% to debug reading image size and a few quick seeks.


BULK_EXTRACTOR 2.0 STATUS REPORT
================================
`bulk_extractor` 2.0 (BE2) is now operational. Although it works with the Java-based viewer, we do not currently have an installer that runs under Windows.

BE2  requires C++17 to compile. It requires `https://github.com/simsong/be13_api.git` as a sub-module, which in turn requires `dfxml` as a sub-module.

The project took longer than anticipated. In addition to updating to C++17, It was used as an opportunity for massive code refactoring and general increase in code quality, testability and reliability. An article about the experiment will appear in a forthcoming issue of [ACM Queue](https://queue.acm.org/)
