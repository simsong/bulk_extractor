==============================================================================
Bulk Extractor 1.5.4  Feature Freeze: TBD  Release: TBD
==============================================================================

OTHER REFERENCES:
- https://github.com/simsong/bulk_extractor/issues

DOCUMENTATION:
- document how to write a new scanner and add it to the mainstream.

BUGFIXES:
- scan_net occasionally throws exceptions. Find out why and stop it.
- scan_net does not properly report timestamps
+ scan_pdf should use multiple strategies for extracting text.

FEATURES:
+ Inverting bytes

+ Work with windows raw-device (e.g.  \\.\physicaldrive0 ) 
  when run as Administrator.
   -  http://msdn.microsoft.com/en-us/library/aa363858(v=vs.85).aspx

+ Track number of bytes processed

+ Construction of a stop-list from standard installs of OS and Apps

+ Replaced hacky XML reading in restart with a proper Expat-based parser.

+ Fixed exception throwing in MyFlexLexer.h so that msg is properly passed as *what().

On Hold:

- scanner for emails and usernames.  <simsong@acm.org> "Simson L. Garfinkel"

- improved testing and validation of CMU LIFT software

- Support for checkpointing using BLCR.

- slg: scan_net.cpp - replace all buffer arithmetic with sbuf pointer get.
 - Figure out why this is causing a assertion failure:
 - /Users/simsong/domex/src/bulk_extractor/trunk/src/bulk_extractor -Z -o out4 -j1 -Y 7805599744 /corp/nps/drives/nps-2011-2tb/nps-2011-2tb.E01
- simplify beregex_vector, word_and_context_list, and regex_list into a single structure.

- Integrate Digital Assembly video carving

- Filter mode - reads from stdin and writes from stdout.
- It's not BASE64 unless you have at least X characters from above 16; 
- It's not BASE16 unless you have at least X characters from above 10
- It's not BASE85 unless you have at least X characters from above 64

- scan_rar — integrate JHUAPL code
   detect the presence of RAR-compressed data, report it,
   and recursively re-process it. Handles both RAR and RAR2

- represent all files examined in report.xml file (.001,.002, etc.)

- Windows shortcut files & IE history

- Improved regression testing for release:
  - bulk_diff.py
  - identify_files.py
  - Benchmark testing for execution against reference disk images

- Escape processing to search term histogram

- Improved restarting, so that each page is retried once.
  (Retry it if we see a single start in the XML file but not two starts.)

- Make sure identify_filenames will not process histogram files and it should produce an excel file.

- Performance optimization 

- Add NIST hacking case to regression testing.

- UTF-16 email addresses sometimes have the last character removed; figure out why and fix.

- Add the classification label of media from .E01 files into the Feature file as a comment.
EWF files have a Notes field in which a classification label may be placed.
This field may be filled with classification labels such as UNCLASSIFIED//FOUO.
bulk_extractor may detect this field and forward a corresponding comment
in generated Feature files such as "# CLASSIFICATION: UNCLASSIFIED".
Classification comments may also be inserted into Feature files using the "-b" banner option.

BEViewer (Requested but not assigned):
- Display the file path, if there is one, of selected Features.
We may use fiwalk and identify_filenames to additionally display the file
associated with the Feature that is currently navigated to.

- Revise, document and deploy  multi-drive correlator

================================================================
Bulk Extractor 1.5:  Sometime in 2014
================================================================


- scan_windir:
  - Add support for MBR and GPRT decoding (can we just hijack the SleuthKit code?)



==============================================================================
Bulk Extractor 2.0.   Sometime in 2016?
==============================================================================


- Source code scanner
  - Will this be part of scan_lift?

- Carvers:
   - MPEG carving (Integrate results of Digital Assembly work)
   - AVI carving
   - Carve iCalendar entries

- 7Zip Scanner (scan_lzma)

- Timestamp scanner

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

- scan_mime — Some way to handle two MIME quoting problems — =\n
   should be replaced by “”, and =40 should be replaced by “@”. But
   should all “=” escapements be handled?

   This will handle:

   user@loc=
   alhost

   user=40localhost

   loc^M
   alhost

- scan_sqlite — Find, identify, and validate sqlite databases. Ideally
  does carving of unallocated SQLITE pages.

- Modify DFXML so that absolute path of disk image is reported.
  http://stackoverflow.com/questions/143174/c-c-how-to-obtain-the-full-path-of-current-directory

- make feature_recroder::get_name raise an exception rather than aborting?

- Update scan_net to carve PPP packets (alegedly common with 3G and 4G modem cards)

- Python bridge, so scanners can be written in python
  - Requires that each Python interperter be run in its own address space,
    as python is not thread-safe

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

==============================================================================
Possible Projects
==============================================================================
- new scanner for Windows iedownloadhistory index.dat file contents
File /users/<username>/appdata/roaming/microsoft/windows/iedownloadhistory/index.dat
contains download history and timestamp information from IE9.

Here is the data structure, contributed by Fornzix on linux_forensics on 6/26/12:
1. Records show up as gibberish until the computer is restarted for
some reason. Even shutting down IE9 didn't help. After the restart,
the records are readable.
2. Individual download records are sized in multiples of 128 bytes
(896,1024,1152,1280,.....).
3. Individual downloads start with "URL" (bytes 1-3).
4. Byte 4 = unknown.
5. Byte 5-6 = These two bytes make a 16 bit Integer which is the
length of the record in 128 byte chunks (i.e. hex 0B 00 = 11, and 11 x
128 = 1408, which is the total record length from "URL" to #12 below).
6. Bytes 17-24 = 8 byte Windows Date / Time when the download
finished.
7. Bytes 81-84 = 4 byte DOS (GMT) Time when download finished (funny
though... it's a few 1000's of a second longer than bytes 17-24)
8. Bytes 193-200 = 8 byte Windows Date / Time when the download
finished. (same as bytes 17-24)
9. Byte 469 = Start of download URL "http".
10. Three hex "00" in a row separate the end of the download URL from
the beginning of the location saved to on the hard drive.
11. There are three hex "00" at the end of the location where the file
was stored on the hard drive.
12. The remainder of the record, which could be considered 'slack
space' is taken up with hex EF:BE:AD:DE which is "DEADBEEF".

================================================================
TESTING
================================================================

Bulk_extractor needs a systematic approach to internal unit tests and
overall system tests.

Unit Tests:

sbuf_t - tests
 - test each constructor & destructors
 - test find and copy

Input/Ouput Testing
regress.py - currently runs bulk_extractor on a few test images
  - Add code to validate output

path-printer - 
  - Test bulk_extractor program to extract known items from known disk images.
  - Use the nps-emails disk iamge
    case 1 - output a given page
    case 2 - output a subset of a given page
    case 3 - output a forensic path with a GZIP
    case 4 - output a forensic path with a BASE64

open source memory testing tools

Input / Output Validation: Validate that with a given known input that the output has been properly produced.  
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



