BULK_EXTRACTOR REGRESSION TESTING
=================================

This directory contains programs and test data for bulk_extractor (BE) regression testing.

The Python program regress.py is the regression driver. This program will:
  - Determine an output directory for the next bulk_extractor run
  - Determine options for running bulk_extractor
  - Run bulk_extractor
  - Analyze the results and print a report.

There are several different kinds of BE tests:

1 - Testing with known files to make sure that known items are present.

2 - Testing during development to make sure that the program doesn't crash.

3 - Performance testing.

4 - Testing between versions to determine the differences between version.


By default, regress.py runs the compiled bulk_extractor executable in
../src/bulk_extractor, but this can be changed with the --exe flag.

In normal operation the regression program runs bulk_extractor and
then sorts the results of the feature files. Sorting the feature files
allows different runs to be directly compared with the Linux "diff -r"
command. (Without sorting, the order of the files is indeterminate as
a result of multi-threading.)


Currently regression testing is only supported on the Macintosh and
Linux platforms. Regression testing of the Windows executable can be
performed on Linux using the "WINE" emulator.


1. Testing with Known Files
===========================

usage:  $ python3 regress.py --datacheck       

The directory tests/Data/ contains a set of files that have been specially
constructed for bulk_extractor testing.

The file tests/data_check.txt contains a set of features that are
known to be contained in the Data/ directory. Each feature is chosen
so as to test a different part of the bulk_extarctor program.

When the regress.py program runs with the '--datacheck' option,
regress.py runs bulk_extractor on the Data/ directory with the '-R'
option, recursively processing all of the files that the directory
contains. This typically takes less than a minute, as all of the files
are small. When BE finishes the regress.py program loads all of the BE
results into memory and then reads the data_features.txt file to make
sure that all of the features were found.

The --datacheck option only checks to make sure that features in the
data_check.txt are found at the correct offset---it does not verify
that the extracted features are correct. Although not idea, in
practice this avoids problems from BE's carving deduplication (which
sometimes results in a filename, and other times results in the
respones <CACHED> if another copy of the file was found.)

Sample output:
    $ python regress.py --datacheck
    run_outdir:  regress-1.5.3-10
    ../src/bulk_extractor -o regress-1.5.3-10 -S write_feature_sqlite3=YES -S write_feature_files=YES -e all -e outlook -R Data
    hashdb: hashdb_mode=none
    bulk_extractor version: 1.5.3
    Hostname: r4.ncr.nps.edu
    Input file: Data
    Output directory: regress-1.5.3-10
    Disk Size: 161
    Threads: 32
    15:45:20 File Data/beth.odt (2.48%) Done in  0:00:03 at 15:45:23
    15:45:20 File Data/mywinprefetch_cat (5.59%) Done in  0:00:01 at 15:45:21
    15:45:20 File Data/testfile2_ANSI.txt (8.70%) Done in  0:00:00 at 15:45:20
    ...
    Data/Base64_files/README.docx􀀜-2685-ZIP-6260 found
    Data/Base64_files/README.docx􀀜-2685-ZIP-14165 found
    Data/Base64_files/README.docx􀀜-2685-ZIP-18739 found
    Total features found: 73
    Total features not found: 0
    $ 

As indicated above, there should be zero features that are not
found. This assures that all of the features in the data_check.txt
file are present. 



2. Testing during development to make sure that BE doesn't crash
=============================================================
During development, regress.py can run bulk_extractor on one of
three test disk images. This testing is largely to make sure that
the program does not crash as new features are added. It can also be
used for performance testing over time.

In this mode, bulk_extractor is run on one of three disk images that are defined in regress.py:

default_infile   = "nps-2009-ubnist1/ubnist1.gen3.raw"
fast_infile      = "nps-2010-emails/nps-2010-emails.raw"
full_infile      = "nps-2009-domexusers/nps-2009-domexusers.raw"

Use the --fast option to specify the fast_infile, and --full to
specify the full file.





3. Performance Testing
=======================

Performance testing is essentially the same as testing to make sure
that BE doesn't crash: the program is run with a known file and the
time that it takes to run is recorded (in report.xml) and reported.



4. Testing between versions to determine the differences between versions
======================================================================
To test the difference between versions, the most straightforward
approach is to run BE with each version and then use the program
python/bulk_diff.py to report on the differences.


TO PERFORM REGRESSION TESTING
=============================

Bulk_extractor requires a disk image for regression testing.


