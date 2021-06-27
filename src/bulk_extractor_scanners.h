/* A list of the compiled-in -scanners
 * Included twice by bulk_extractor_scanners.cpp, once for definitions, the second time to create the array.
 */

#ifndef BULK_EXTRACTOR
#error "config.h must be included before bulk_extractor_scanners.h"
#endif

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

SCANNER(aes)
SCANNER(base64)
SCANNER(elf)
SCANNER(exif)    // JPEG carver

#ifdef HAVE_EXIV2
SCANNER(exiv2)
#endif

SCANNER(evtx)        // scanner provided by 4n6ist:
SCANNER(facebook)
SCANNER(find)
SCANNER(gzip)
//SCANNER(hiberfile)
SCANNER(httplogs)
SCANNER(json)
//SCANNER(kml)
//SCANNER(msxml)
//SCANNER(net)
//SCANNER(ntfsindx)    // scanner provided by 4n6ist:
//SCANNER(ntfslogfile) // scanner provided by 4n6ist:
//SCANNER(ntfsmft)     // scanner provided by 4n6ist:
//SCANNER(ntfsusn)
//SCANNER(outlook)
SCANNER(pdf)
//SCANNER(rar)
//SCANNER(sqlite)
//SCANNER(utmp)        // scanner provided by 4n6ist:
SCANNER(vcard)
//SCANNER(windirs)
//SCANNER(winlnk)
//SCANNER(winpe)
//SCANNER(winprefetch)
//SCANNER(wordlist)
SCANNER(xor)
SCANNER(zip)


#ifdef HAVE_LIBLIGHTGREP
//SCANNER(accts_lg)
//SCANNER(base16_lg)
//SCANNER(email_lg)
//SCANNER(gps_lg)
//SCANNER(lightgrep)
#endif
