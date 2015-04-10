                    Announcing bulk_extractor 1.5
                             July 29, 2014

                            RELEASE NOTES

bulk_extractor Version 1.5 alpha 6 has been released for Linux, Mac OS and
Windows. 

Release source code and Windows installer:
    http://digitalcorpora.org/downloads/bulk_extractor/

GIT repository:
    https://github.com/simsong/bulk_extractor



Major Improvements
==================
* BE now optionally writes features to SQLite databases in addition to
  flat files.

* New scanners, plus many fixes in existing scanners.

* Improved support for carving encoded objects missed by contemporary
  carvers (e.g. ZIP-compressed JPEG files).

* bulk_extractor is now available as a shared library
  (libbulkextractor.so and libbulkextractor.DLL). You can load this
  shared library from Python using the bulk_extractor python module
  (in the python/module directory).

* bulk_extractor now supports in-memory histograms, which allows efficient
  construction of histograms built from a large number of relatively few
  features. The in-memory histograms are used by scan_sceadan, a bulk data
  type classifier built on top of UTSA's SCEADAN statistical file type 
  classifer.

Writing to SQLite databases:
============================
bulk_extractor will write both features and histograms to sqlite
databases. The database contains both the escaped UTF8 features and
unescaped features. (The escaped features include invalid UTF8
characters if the feature contained them.)

By default, bulk_extractor 1.5 will write to feature files, but not to
the SQLite3 database.

* Enable/Disable writing features to SQLite3 database:
    `-S write_feature_sqlite3={YES|NO}`

* Enable/Disable writing features to flat files:
    `-S write_feature_files={YES|NO}`

In our testing, writing to a SQLite database has minimal impact on the
time required to generate output.

  Test dataset: nps-2009-domexusers.E01
  Size of feature files: 112MB
  Size of Sqlite3 databsae: 234M

  Runtime system: Dual Xenon server, 16 cores
  Time to generate output with feature files: 515 seconds
  Time to generate output with SQLite: 524 seconds
  Time to generate output with both: 527 seconds


New Scanners:
===============
Bulk_extractor version 1.5 provides these new scanners:

* scan_base64   --- the Base64 scanner has been rewritten
* scan_facebook --- Finds snippits of HTML with containing facebook strings
* scan_hashdb  --- NPS hash database scanner (can create or search a hashdb)
* scan_httplogs --- A scanner that finds fragments of HTTP logs
* scan_outlook will decrypt "Outlook Compressible Encryption" used on some PST files.  (disabled by default)
* scan_sceadan --- UTSA's "SCEADAN" file type classificaiton engine.
* scan_sqlite ---- SQLite database carving. (Note: only works for unfragmented databases.)
* scan_winlnk --- Windows LNK file detection. 

New LightGrep Scanners:
-----------------------
Bulk_Extractor also now has a number of scanners built to take advantage of
lightgrep. These scanners generally replace the scanners written on
top of gnuflex. These include:  

* scan_accts_lg
* scan_base16_lg
* scan_email_lg
* scan_gps_lg.cpp
* scan_httpheader_lg

These scanners are disabled by default unless bulk_extractor is built with LightGrep support.

Scanners not in use:
--------------------
Scanners that are shipped, but not in use have been moved to the directory src/old_scanners.


Improved Carving Support
========================
Bulk_extractor implements a sophisticated system for carving objects that it discovers.

Bulk_extractor version 1.4 and above support three carving mode for
each kind of data that it can carve:

  mode 0 - do not carve anything found
  mode 1 - carve data if it is encoded (e.g. compressed, BASE64 encoded, etc.)
  mode 2 - carve all recognized data

The following carving modes are specified in the default configuration:

   -S jpeg_carve_mode=1     0=carve none; 1=carve encoded; 2=carve all (exif)
   -S min_jpeg_size=1000    Smallest JPEG stream that will be carved (exif)

JPEG files with valid Exif structures are carved if they are
encoded. With this behavior JPEGs that can be carved with existing
carvers such as Scalpel and PhotoRec will not be carved, but JPEGs
that can only be recovered using bulk_extractor's ability to carve
encoded data will be.

   -S unzip_carve_mode=1    0=carve none; 1=carve encoded; 2=carve all (zip)

Components of ZIP files that have been encoded will be detected and
carved. In practice, this means that ZIPed ZIP files will be
uncompressed and carved, but normal ZIP files will be left as is.


   -S unrar_carve_mode=1    0=carve none; 1=carve encoded; 2=carve all (rar)

RAR1/2/3 files that are encoded will be carved. For example, RAR files
that are sent as email attachments will be carved, but RAR files on
the hard drive will not be carved.  (Note that bulk_extractor does not
support RAR5. Also, during final testing, a bug was discovered in the
RAR decompressor that sometimes results in corruption of
RAR-compressed archives. This problem will not be fixed in
bulk_extractor 1.5, but may be fixed in a later version.)

   -S sqlite_carve_mode=2    0=carve none; 1=carve encoded; 2=carve all (sqlite)

By default, all sqlite files detected will be carved.  Note that only
sqlite3 database files that were stored contigiously on the source
media will be readable.

Bulk_extractor 1.5 carving corrected several implementation bugs in the
bulk_extractor 1.4 carving algorithms. It also now implements
deduplication, which means that the same object will not be carved
twice. This is important when carving email archives, which tend to
contain the same images as attachments to email messages.

Memory carving by the scan_net module can also be controlled, as it
tends to generate a lot of false positives. Network carving is
disabled by default:

   -S carve_net_memory=NO    Memory structures are not carved

By default, Bulk_Extractor will not scan for in-memory TCP/IP structures.


Other Improvements:
====================

Improvements in existing scanners:
---------------------------------

scan_accts:
    - now detects bitcoin addresses and writes them to pii.txt
    - now detects TeamViewer addresses and writes them to pii.txt and pii_teamviewer.txt

SSN recognition: you are now able to specify one of three SSN recognition modes:

    -S ssn_mode=0  SSN’s must be labeled “SSN:”. Dashes or no dashes are okay.
    -S ssn_mode=1  No “SSN” required, but dashes are required.
    -S ssn_mode=2  No dashes required. Allow any 9-digit number 
                   that matches SSN allocation range.

scan_hashid has been renamed scan_hashdb so that it will be consistent
with the other library names.


Overreporting Fixes:
-------------------

We have further improved overreporting problems:

* scan_base16 is now disabled by default (the hex values were not useful).

* min_phone_digits is changed from 6 to 7, so that 6-digit phone numbers will no longer be reported.

Underreporting Fixes
---------------------

* scan_base64 was ignoring a substantial amount of BASE64-encoded
  data. This has been corrected. As a result, substantially more email
  addresses and URLs from BASE64-encoded data such as email
  attachments and SSL certificates will be reported in version 1.5
  compared with version 1.4.


Improvements in Python programs:
--------------------------------

* Minor improvements to regress.py, bulk_diff.py and bulk_extractor_reader.py


Incompatible changes:
--------------------
None that we know of.


Bug Fixes
------------------

* Versions 1.4 through 1.5 beta2 could not handle split-raw files on Windows. 
  Now it can again.

* FLAG_NO_STOPLIST and FLAG_NO_ALERTLIST in feature_recorder.h were the same. 
  They are now different.

* FLAG_NO_QUOTE and FLAG_XML in feature_recorder.h were the same. 
  They are different now.

* The split wordlists contained utf8 escaped words, rather than pure UTF8. 
  It now has pure UTF8.

* A bug in feature_recorder::unquote_string caused strings containing
  the sequence \x5C to be improperly decoded. This was caused by a
  typo in the function hexval (we had a private implementation because
  mingw had no equivalent function.) The primary impact was that
  words containing backslashes did not appear in the split wordlist
  generated by scan_wordlist.

* stop lists and white lists are now assumed to be constants unless
  the characters '*', '[' or '(' are present, in which case they are
  assumed to be regular expressions.

Internal Improvements
--------------------- 

* Introduced an atomic set and histogram to minimize the use of
  cppmutexes in the callers. 

* bulk_extractor is now distributed as both an executable and as a
  library. The library called from C or Python as a shared lib.


Known bugs:
--------------
* The RAR decompressor does not reliably decompress all RAR files. 

* The RAR scanner will not reliably name carved RAR file components
  that contain UTF-8 characters in their name.

PERFORMANCE COMPARISON WITH VERSION 1.4
========================================

This section tracks how performance of bulk_extractor has changed over
time.  

Because of changes to underlying hardware, compilers and scanners, 
we are no longer reporting historical trends. Instead, we are
reporting specific performance comparisons between version 1.4 and 1.5.

    Compiler: gcc 4.8.2 (Red Hat 4.8.2-7)
    Compiler Flags: -pthread -O3 -MD
    Runtime system: Supermicro AMD server with 64 cores

    Disk Image: NPS-2011-2TB
    Media Size: 2,000,054,960,128 bytes
    MD5:        01990709b4a1a179bea86012223cc726

                 Clock Time       User Time   System Time
    Version 1.5:   18938 sec        510000 sec     11060 sec
    Version 1.4:   20109 sec        422000 sec      4791 sec


    Disk Image: UBNIST1.gen3
                Media size:         1.9 GiB (2106589184 bytes)
                MD5:                49a775d8b109a469d9dd01dc92e0db9c




CURRENT CONFIGURATION
=====================

Current list of bulk_extractor scanners:

```
scan_accts   - Looks for phone numbers, credit card numbers, and other numeric info.
scan_aes     - Detects in-memory AES keys from their key schedules
scan_base16  - decodes hexadecimal test
scan_base64  - decodes BASE64 text
scan_elf     - Detects and decodes ELF headers
scan_exif    - Decodes EXIF headers in JPEGs using built-in EXIF parser.
scan_exiv2   - Decodes EXIF headers in JPEGs using libexiv2 (for regression testing)
scan_email   - Scans for email addresses, URLs, and other text-based information.
scan_exif    - Decodes EXIF headers in JPEGs using built-in decoder.
scan_find    - keyword searching
scan_facebook- Facebook HTML
scan_gps     - Detects XML from Garmin GPS devices
scan_gzip    - Detects and decompresses GZIP files and gzip stream
scan_hashdb  - Search for sector hashes/ make a sector hash database
scan_hiber   - Detects and decompresses Windows hibernation fragments
scan_httplog - search for web server logs
scan_outlook - Decrypts Outlook Compressible Encryption
scan_json    - Detects JavaScript Object Notation files
scan_kml     - Detects KML files
scan_lightgrep - performs searches with LightBox Technology's LightGrep.
scan_net     - IP packet scanning and carving
scan_pdf     - Extracts text from some kinds of PDF files
scan_sqlite  - SQLite3 databases (only if they are contigious)
scan_rar     - RAR files
scan_vcard   - Carvees VCARD files
scan_windirs - Windows directory entries
scan_winlkn  - Windows LNK files
scan_winpe   - Windows executable headers.
scan_winprefetch - Extracts fields from Windows prefetch files and file fragments.
scan_wordlist - Builds word list for password cracking
scan_xor     - XOR obfuscation 
scan_zip     - Detects and decompresses ZIP files and zlib streams
```

Current list of bulk_extractor feature files:

```
aes_keys.txt - AES encryption keys
alerts.txt   - Features found on alert list (redlist)
ccn.txt      - credit card numbers
ccn_track2.txt - Track 2 information
domain.txt   - All extracted domain names and IP addresses
email.txt    - extracted email addresses
ether.txt    - extracted ethernet addresses. Currently
               overcollecting due to a failure to consider
               local context.
exif.txt     - All exif fields from JPEGs; extracted as XML.
find.txt     - Hits on find command.
hex.txt      - Notable hexdecimal constants
gps.txt      - Extracted GPS coordinates from Garmin XML and
               GPS-enabled JPEG files
ip.txt       - Extracted IP addresses from scan_net
               cksum-bad indicates checksum test failed;
               those are less likely to actually be IP
               addresses.
json.txt     - Extracted and validated JavaScript Object
               Notation fragments.
kml.txt      - Extracted KML files
rar.txt      - 
report.xml   - The DFXML file that explains what happened.
rfc822.txt   - All extracted RFC822 headers
tcp.txt      - Summaries of all extracted UDP and TCP packets.
telephone.txt- Extracted phone numbers
url.txt      - Extracted URLs
  url_facebook-id - extracted Facebook IDs
  url_microsoft-live - extracted Microsoft Live IDs
  url_searches       - extracted search terms
  url_services       - extracted services from URLs
winprefetch.txt - Windows prefetch files and fragments,
                  recoded as XML for easy processing.
wordlist.txt - All the words 
zip.txt      - Information about all ZIP files and zip
               components.
```

Current list of carving directories:
```
unrar/
jpeg/
unzip/
sqlite/
```

Planned for 1.6:
================

Version 1.6 plan is open, pending feedback from users.

Future Plans
============

* WkdmDecompress - http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/iokit/Kernel/WKdmDecompress.c

* scanner to remove all XML tags (to better extract text from .docx files)

* improvements to identify_filename so it can run on a report in place.

* Find more false positives with CCN validator by scanning through XOR data

* xz, 7zip, and LZMA/LZMA2 decompression

* lzo decompression

* BZIP2 decompression

* CAB decompression

* Scanning for the start of bitlocker protected volumes.

* NTFS decompression

* Better handling of MIME encoding

* Process more data with -e xor and look for CCN hits. Most will be false positives

* Demonstration of bulk_extractor running on a grid (how fast can it run?)


Abandoned Plans
================

* Python Bridge - it would make bulk_extractor run too slow.

* Read from standard input - bulk_extarctor would cause the sending
  process to block. (People typically want to do this with a disk
  imaging tool like dcfldd. The problem is that bulk_extractor is
  inevitably CPU-bound, so this would make the disk imager block.)

* scan_pipe - runs every sbuf through an external program. Again, even
  though this seems like an easy way to integrate an existing program
  with bulk_extractor, the combined system ends up running too slow to
  be useful.  
