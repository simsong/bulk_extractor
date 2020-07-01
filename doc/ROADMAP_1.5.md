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



