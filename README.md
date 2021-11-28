[![codecov](https://codecov.io/gh/simsong/bulk_extractor/branch/main/graph/badge.svg?token=3w691sdgLu)](https://codecov.io/gh/simsong/bulk_extractor)

`bulk_extractor` is a high-performance digital forensics exploitation tool.
It is a "get evidence" button that rapidly
scans any kind of input (disk images, files, directories of files, etc)
and extracts structured information such as email addresses, credit card numbers,
JPEGs and JSON snippets without parsing the file
system or file system structures. The results are stored in text files that are easily
inspected, searched, or used as inputs for other forensic processing. bulk_extractor also creates
histograms of certain kinds of features that it finds, such as Google search terms and email addresses,
as previous research has shown that such histograms are especially useful in investigative and law enforcement applications.

Unlike other digital forensics tools, `bulk_extractor` probes every byte of data to see if it is the start of a
sequence that can be decompressed or otherwise decoded. If so, the
decoded data are recursively re-examined. As a result, `bulk_extractor` can find things like BASE64-encoded JPEGs and
compressed JSON objects that traditional carving tools miss.

This is the `bulk_extractor` 2.0 development branch!  For information
about the `bulk_extractor` update, please see [Release 2.0 roadmap](https://github.com/simsong/bulk_extractor/blob/main/doc/ROADMAP_2.0.md).

Building `bulk_extractor`
=========================
To build bulk_extractor in Linux or Mac OS:

1. Make sure required packages have been installed. **You can do this by going into the etc/ directory and looking for a script that installs the necessary packages for your platform.**

2. Then run these commands:

```
./configure
make
make install
```

For detailed instructions on installing packages and building bulk_extractor, read the wiki page here:
https://github.com/simsong/bulk_extractor/wiki/Installing-bulk_extractor

The Windows version of bulk_extractor must be built on Fedora using mingw.

For more information on bulk_extractor, visit: https://forensicswiki.xyz/wiki/index.php?title=Bulk_extractor

Tested Configurations
=====================
This release of bulk_extractor requires C++17 and has been tested to compile on the following platforms:

* Amazon Linux as of 2019-11-09
* Fedora 32
* Ubuntu 20.04LTS
* MacOS 11.5.2

To configure your operating system, please run the appropriate scripts in the [etc/](/etc) directory.


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
`bulk_extractor` 2.0 (BE2) is now operational for development use and testing. However, it has not yet been validated.

BE2  requires C++17 to compile. It requires `https://github.com/simsong/be13_api.git` as a sub-module, which in turn requires `dfxml` as a sub-module.

The project took longer than anticipated. In addition to updating to
C++17, It was used as an opportunity for massive code refactoring and general increase in code quality, testability and reliability.
