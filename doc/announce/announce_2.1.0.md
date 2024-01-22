                   Announcing bulk_extractor 2.1.0
                           January 22, 2024

                            RELEASE NOTES

The digital forensics tool bulk_extractor version 2.1.0 is now available for general use.

Release download point:
    https://github.com/simsong/bulk_extractor/releases

GIT repository:
    https://github.com/simsong/bulk_extractor


I am pleased to announce the general availability of bulk_extractor
version 2.1. This is the first release of bulk_extractor version 2
that is recommended for general use.

Bulk_extractor 2 is a significant rewrite of bulk_extractor. Verison 2
significantly improves the performance and portability of version 1.
The rewrite started in 2016 and was largely completed by January
2021.

Details of the rewrite, including a detailed report of the performance
improvements and lessons learned, can be found in (Sharpening Your
Tools: Updating bulk_extractor for the 2020s)[1], Simson Garfinkel and
Jon Stewart. Communications of the ACM, August 2023,

[1] https://doi.org/10.1145/3600098

Bulk_extractor version 2.1 is the first stable version of
bulk_extractor version 2 that is recommended for general use. It
corrects a problem with the string search scanner that caused
bulk_extractor to hang on open-ended regular expressions such as '[a-z]*@company.com' specified with the -F flag.
With version 2.1, we have replaced the C++17 regex compiler with Google's RE2 regex compiler that avoids backtracking. As a result, these open-ended regular expressions no longer hang.


2.0/2.1 Improvements over Version 1:
===================================

* BE2 is significantly faster on multi-core systems than BE1.

Relase 2.1 Limitations
===================================
* BEViewer is not included in this release. Although it works with Verson 2, it is not yet officially supported.

* scan_outlook and scan_hiberfile are now disabled by default because they did not have unit tests. These scanners can be re-enabled by specifying -eoutlook and -ehiberfile on the command line.

* scan_aes no longer scans for 192-bit AES keys by default, although this behavior and be re-enabled.

Known bugs:
--------------
* The RAR decompressor does not reliably decompress all RAR files, and only supports RAR v1, v2 and v3.

* The RAR scanner will not reliably name carved RAR file components that contain UTF-8 characters in their name.


You can help
================
We are looking for help to implement the following algorithms:

* WkdmDecompress - http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/iokit/Kernel/WKdmDecompress.c

* xz, 7zip, and LZMA/LZMA2 decompression

* lzo decompression

* BZIP2 decompression

* CAB decompression

* Scanning for the start of bitlocker protected volumes.

* NTFS decompression

* Better handling of MIME encoding

* Process more data with -e xor and look for CCN hits. Most will be false positives

* Demonstration of bulk_extractor running on a grid (how fast can it run?)

* Python Bridge - run multiple copies of python to let scanners be written in python

* scan_pipe - runs every sbuf through an external program.
