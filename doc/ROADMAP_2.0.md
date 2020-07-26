Bulk Extractor 2.0.   Planned Release: September 30, 2020
=========================================================

This is the planning document for bulk_extractor version 2.0.

Bulk_extractor was a funded project of the US Government from 2006
through 2014. The project's development is being continued by the
development team on a volunteer basis. As such, the goals and intended
feature set of version 2.0 is being significantly re-scoped.

Goals for the 2.0 release
-------------------------

* Production quality. Version 1.x of bulk_extractor was a research
  tool that also found usefuleness in operational settings. Verison
  2.x is a production tool. As such:

  - Research scanners have been removed from the master branch. They
    can still be researched by making them shared-libraries and using
    the bulk_extractor plug-in system.

  - Unit tests have been added.

* Improved software development practices.

  - Continious integration is employed to validate each commit.

  - Development will take place in feature branches which will be
    added to the master branch only if CI tests pass. 

* Sensible defaults for production operation.  With the undertanding
  that most users do not understand command-line options,
  bulk_extractor now runs with fewer command-line options.

* Standards-compliant. Where possible, we are adopting C++14 features
  that are now widely available.

* Experimental features have been removed. Experiments are now
  conducted with plugin-s.

* BE2.0 will be released as a pure command-line tool. The user
  interface with the windows installer (and embedded CLI) will be
  released afterwards.

* SQL will be turned on by default and the program will provide the
  user with instructions on how to use it. Performance will be
  analyzed to determine the fastest way to create the text feature
  files, the SQLite3 database, and the histograms. 

* Include other easy-to-output feature files by default, such as
  collect all email messages.

* Integration with The Sleuth Kit for file enumeration

# Architecture Changes
- [ ]  be13_api will build a .a library
- [ ]  be13_api will include tests and have its own makefile system.
- [ ]  dfxml will be a submodule of be13_api
- [ ]  Find all of the files first, scan them in file-block order, then scan all of the remaining blocks. We can do this on a 1TB disk with 4K sectors. That's 256 million 4k blocks, or 32MiB of RAM, which is fine. 

Release 2.0 Implementation Plan
-------------------------------
Bulk_extractor depends on the [be13_api](https://github.com/simsong/be13_api) and on [dfxml](https://github.com/simsong/dfxml). In the past, functionaly requirements were specified, code was written, testing was done, and reasonably reliable software was pushed out to users.

BE2 is adopting test-driven-development and continious integration. The goal is to make testing more efficient (because it will happen all the time) move more effort into the design and development, allowing more time for performance improvements and regression testing.

Most of the development on BE was done from 1993 to 2010. As a result, the code-base is a combination of legacy C, C++, and C++ with the Standard Template Library. Between 2005 and 2015 there was significant development of the C++11 and C++14 standards. Support for C++14 is now widespread, so BE2 will require C++14 for compiling and deployment. This means that many lines of code that were written for BE1 can now be replaced by a single line of code, thanks to C++14.

BE2 is not a complete rewrite of BE1, but it is a **significant** rewrite.

Organizational change:

* [DFXML](https://github.com/simsong/dfxml) can now be independently tested with unit tests. This is done by `make check` and is also done by the CI system.

* [be13_api](https://github.com/simsong/be13_api) can now be independently tested. Because BE13_API depends on DFXML, dfxml is a sub-module of be13_api. This is a change from 1.0, in which be13_api referenced DFXML but didn't include it. 

* Because BE doesn't need two copies of DFXML, it no longer has DFXML as a sub-module.

Here is the proposed plan for development of Release 2.0. This list will be transferred to a GitHub issue at some point.

- [ ] Complete stand-alone tests for DFXML.
- [ ] Port DFXML to C++14.
- [ ] Complete stand-alone tests for BE13_API with no modules actually being tested.
- [ ] Expand the coverage of BE13_API tests for each *.cpp file.
- [ ] Complete a stand-alone tester for a BE scanner.
- [ ] Create a unit test for each BE2 scanner
- [ ] Create a unit test for the BE2 framework.
- [ ] Create a unit test for the full BE2 system to compile
- [ ] Implement additional scanners as desired.
- [ ] Begin end-to-end regression testing, making sure that all features identified by BE1.5 are still identified by BE2.0
- [ ] Begin performance testing.

Features Removed from Release 2.0
=================================
## Remove Scanners
These scanners are removed. Each can be used if you make it a plug-in:
* scan_lift
* scedan 
* hashdb

## Other removed functionality
* Support for AFF and AFF4 input file formats.

## Compiling on non-standard platforms
Where possible, C++14 

Release 2.0 Features
====================

## New scanners:
* iCalendar carver
* Email message carver.

## Ideas on hold
These would make good capstone or master's projects:

- MPEG carver 

- AVI carver 

- 7Zip Scanner (scan_lzma)

- Timestamp scanner

- scan_windir:
  - Add support for MBR and GPRT decoding (can we just hijack the SleuthKit code?)

- scan_lzma — detect the presence of LZMA-compressed data, report it,
   and recursively re-process it. (Model scan_zip).

- scan_bzip2 — detect the presence of bzip2-compressed data, report
   it, and recursively re-process it. (Model scan_zip).

- scan_msi — detect the presence of MSI-compressed data, report it,
   and recursively re-process it. Find the code for MSI compression in
   The Unarchiver. (Model scan_zip).

- scan_cab — detect the presence of CAB-compressed data, report it,
   and recursively re-process it. Find the code for CAB compression in
   The Unarchiver. (Model scan_zip).

- scan_ntfs — detect the presence of NTFS-compressed data, report it,
   and recursively re-process it. This is especially difficult because
   NTFS compression has no magic numbers, so trial compression needs
   to be done! (Model scan_hiber).

- Update scan_net to carve PPP packets (alegedly common with 3G and 4G modem cards)

- C# bridge, so scanners can be written in C#

- Codepage / CJKV identification
  - typically Windows-Codepage 1252  and / or UTF-8

- Human Language identification.
 - Identify the kind of language that's present. 
   - http://sourceforge.net/projects/la-strings/
   - http://lucene.apache.org/nutch/apidocs-0.8.x/org/apache/nutch/analysis/lang/LanguageIdentifier.html
   - http://github.com/vcl/cue.language
   - http://alias-i.com/lingpipe/demos/tutorial/langid/read-me.html
   - http://textcat.sourceforge.net/

## Ideas for 2.0:

- scan_mime — Unquote MIME messages and re-process them. 

- Modify DFXML so that absolute path of disk image is reported.
  http://stackoverflow.com/questions/143174/c-c-how-to-obtain-the-full-path-of-current-directory

- make feature_recorder::get_name raise an exception.

- Python bridge, so scanners can be written in python
  - Requires that each Python interperter be run in its own address space,
    as python is not thread-safe

- Explore integration of http://itextpdf.com/itext.php for PDF text extraction.
  - rewrite scan_pdf?

- Allow bulk_extractor to scan just unallocated area. 

   Unallocated lists can come from:
   1 - Real-time analysis of disk using sleuthkit
   2 - DFXML file
   3 - list of blocks from sleuthkit blk_find

   Not clear we want to this in bulk_extractor, rather than just having it scan from stdin?

- More options for suppression:
   - Suppress known sectors (hash list of sector hashes?)

- Improve documentation
  - Document the feature file syntax
    - The syntax of Feature files will be documented.
    - Basically: We have Feature Files and Histogram Files.
    - These files have tab-delineated data.
    - BOM is ignored.
    - Lines starting with "#" are ignored.
    - Entries in most Feature files contain three fields:
    - 1) Offset in decimal or else a forensic path,
    - 2) the Feature (which might be XML)
    - 3) the "context." (which might be XML)
    - Entries in gps.txt and exif.txt contain three fields: 1) offset, 2) MD5SUM, 3) formatted content.
    - Entries in Histogram files contain two fields:
    - 1) histogram count prefixed by "n=" and
    - 2) the Feature.
    - All bytes below space (" ") are converted to Octal and are escaped with "\".

- scan_winprefetch
  - Add ability to extract executable's location from prefetch hash value
	http://www.woanware.co.uk/?page_id=173	

  - Ability to detect and analyze SuperFetch files
	http://www.forensicswiki.org/wiki/SuperFetch

- scan_plist
 - create. Give it the ability to find and decode Mac plist files (binary and XML)

- scan_im:
   - Skype
   - Pidgon
   - Google Talk
   - Yahoo! Messenger including decryption (XOR of the @yahoo account name)
   - QQ Messenger including decryption (Blowfish with the key being the QQ account number?)
   - etc.

- Windows Jump List scanner?

- VM detection?  ie:
   - VirtualBox; VMware; QEMU/KVM; Parallels; Virtual PC

- IE Download history [See notes](iehistory.txt)

Testing
=======

Bulk_extractor needs a systematic approach to internal unit tests and
overall system tests.

## Unit Tests:

sbuf_t - tests
 - test each constructor & destructors
 - test find and copy

path-printer - 
  - Test bulk_extractor program to extract known items from known disk images.
  - Use the nps-emails disk iamge
    case 1 - output a given page
    case 2 - output a subset of a given page
    case 3 - output a forensic path with a GZIP
    case 4 - output a forensic path with a BASE64

## End-to-end tests

- Input/Ouput Testing
  - regress.py - currently runs bulk_extractor on a few test images
     - Add code to validate output

- open source memory testing tools

Input / Output Validation
Validate that with a given known input that the output has been properly produced.  
-IO Test Case 1: (Based on B. Allen's suggestion) Start with a union data set - i.e. collect the results of all 
BE identified features, then using BEViewer to inspect the features.  
-- Goals: Identification of error rates: false positives, false negatives

Performance Testing: 
- PT Test Case 1: Enabled All
-- Objective: Test the overall performance of bulk extractor with regards to memory utilization, cpu utilization,
and overall execution time on a chosen data set
--- Goals: Characterization of Bulk Extractor and all scanners enabled

- PT Test Case 2: Individual Scanner 
-- Objective: Test the individual scanner with bulk extractor to characterize memory utilization, cpu utilization,
and execution time on a chosen data set
--- Goals: Characterization of individual scanners to ascertain the performance of an individual scanner

Security Evaluation Testing:
- SET Test Case 1: Fortify Testing
-- Objective: Taking bulk extractor source code and evaluating if the code baseline has vulnerabilities.
-- Goals: Identification and corrections of any security issues


- scanner for emails and usernames.  <simsong@acm.org> "Simson L. Garfinkel"

- simplify beregex_vector, word_and_context_list, and regex_list into a single structure.

- Sanity checks for BASE16, BASE64 and BASE85scanners

- scan_rar — Upgrade to RAR5

- Windows shortcut files & IE history

- Improved regression testing for release:
  - bulk_diff.py
  - identify_files.py
  - Benchmark testing for execution against reference disk images

- Escape processing to search term histogram

- Make sure identify_filenames will not process histogram files and it should produce an excel file.
- Performance optimization 

- Add NIST hacking case to regression testing.

- UTF-16 email addresses sometimes have the last character removed; figure out why and fix.

- Rewrite BEViewer as HTML/JavaScript application

- Display the file path, if there is one, of selected Features.
We may use fiwalk and identify_filenames to additionally display the file
associated with the Feature that is currently navigated to.

- Revise, document and deploy  multi-drive correlator






See Also
========

https://github.com/simsong/bulk_extractor/issues

