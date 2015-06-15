                           (PRE-RELEASE DRAFT)
                    Announcing bulk_extractor 1.6.0
                             Date TBA

                            RELEASE NOTES (DRAFT)

bulk_extractor Version 1.6.0 is still under development for Linux, Mac OS and Windows. 

Release source code and Windows installer:
    http://digitalcorpora.org/downloads/bulk_extractor/

GIT repository:
    https://github.com/simsong/bulk_extractor


1.6.0 Improvements over Version 1.5.x:
================================

* BEViewer usability is improved:
 * BEViewer now shows context information for _hashdb_ feature file
   `identified_blocks.txt`.
 * BEViewer also now shows context information for post-processed _hashdb_
   feature files if text `identified` is part of its filename.
 * The Reports list may now be refreshed using menu option View|Reports|Refresh,
   allowing new feature files to become visible.
 * Fields such as the Image File filename may now be selected and copied.
 * The Media Image display now provides 64KiB per page instead of 4KiB per page.
 * BEViewer may now browse to forensic paths generated using the bulk_extractor
   `-R` recursive scan mode.  On Windows systems, attempting to browse to these
   paths incorrectly resulted in an error.

* New scanners plus fixes to existing scanners:
 * `scan_winpe` more consistently extracts `.dll` filenames.
   `.dll` filenames were not extracted when an inconsistently used data field
   was not set.
 * `scan_winpe` now carves PE files.  The number of values carved is determined
   by the furthest byte allocated, which is either to the end of the
   Certificate Table Data Directory in the Optional Header
   or to the end of the furthest section defined in the Section Table.
 * `scan_winlnk` captures more data fields.
   This change was driven by the need to capture remote links.
 * `scan_hashdb` includes flag `M` to detect monotonically increasing or
   decreasing data by comparing adjacent 4-byte unsigned integer values.


Incompatible changes:
--------------------
None.


Bug Fixes
------------------
* `scan_winpe` more consistently extracts `.dll` filenames.
* The Windows installer is corrected to install to the 64-bit executable directory.

Internal Improvements
--------------------- 
None.

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
scan_msxml   - Extracts text from Microsoft Open Office XML files. 
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
winpe/
```

Planned for 1.7.0:
================

Version 1.7.0 plan is open, pending feedback from users.

Future Plans
============

* WkdmDecompress - http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/iokit/Kernel/WKdmDecompress.c

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
