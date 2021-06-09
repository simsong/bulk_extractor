/* A list of the compiled-in -scanners
 * Included twice by bulk_extractor_scanners.cpp
 */

#ifndef BULK_EXTRACTOR_SCANNERS_H_FIRST_INCLUDE
#define BULK_EXTRACTOR_SCANNERS_H_FIRST_INCLUDE
#include "be13_api/scanner_set.h"
extern "C" scanner_t *scanners_builtin[];
#endif

#ifndef SCANNER
#define SCANNER(scanner) extern "C" scanner_t scan_ ## scanner;
#endif

/* flex-based scanners */
SCANNER(base16)
SCANNER(email)
SCANNER(accts)
SCANNER(gps)

/* Regular scanners */

SCANNER(base64)
SCANNER(elf)
//SCANNER(exif)
#ifdef HAVE_EXIV2
SCANNER(exiv2)
#endif
//SCANNER(facebook)
//SCANNER(find)
//SCANNER(hiberfile)
//SCANNER(httplogs)
SCANNER(kml)
//SCANNER(msxml)
//SCANNER(net)
//SCANNER(ntfsusn)
//SCANNER(outlook)
//SCANNER(rar)
//SCANNER(sqlite)
//SCANNER(windirs)
//SCANNER(winlnk)
//SCANNER(winpe)
//SCANNER(winprefetch)
SCANNER(aes)
SCANNER(gzip)
SCANNER(json)
SCANNER(pdf)

SCANNER(xor)
SCANNER(vcard)
SCANNER(wordlist)
SCANNER(zip)


#ifdef HAVE_LIBLIGHTGREP
//SCANNER(accts_lg)
//SCANNER(base16_lg)
//SCANNER(email_lg)
//SCANNER(gps_lg)
//SCANNER(lightgrep)
#endif


// scanners provided by 4n6ist:
//SCANNER(utmp)
//SCANNER(ntfsmft)
//SCANNER(ntfslogfile)
//SCANNER(ntfsindx)
//SCANNER(evtx)
