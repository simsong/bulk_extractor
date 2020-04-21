/*
 * bulk_extractor.cpp:
 * Feature Extraction Framework...
 *
 */

#include "bulk_extractor.h"
#include "image_process.h"
#include "threadpool.h"
#include "be13_api/aftimer.h"
#include "histogram.h"
#include "findopts.h"
#include "dfxml/src/dfxml_writer.h"
#include "dfxml/src/hash_t.h"

#include "phase1.h"

#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <set>
#include <setjmp.h>
#include <vector>
#include <queue>
#include <unistd.h>
#include <ctype.h>

/************************
 *** SCANNER PLUG-INS ***
 ************************/

/* scanner_def is the class that is used internally to track all of the plug-in scanners.
 * Some scanners are compiled-in; others can be loaded at run-time.
 * Scanners can have multiple feature recorders. Multiple scanners can record to the
 * same feature recorder.
 */

/* An array of the built-in scanners */
scanner_t *scanners_builtin[] = {
    scan_base64,
    scan_kml,
    scan_httplogs,
    scan_net,
    scan_find,
    scan_wordlist,
    scan_aes,
    scan_json,
#if defined(USE_FLEXSCANNERS)
    scan_accts,
    scan_base16,
    scan_email,
    scan_gps,
#endif
#if defined(HAVE_LIBLIGHTGREP) && defined(USE_LIGHTGREP)
    scan_accts_lg,
    scan_base16_lg,
    scan_email_lg,
    scan_gps_lg,
    scan_lightgrep,
#endif
#ifdef HAVE_EXIV2
    scan_exiv2,
#endif
#ifdef HAVE_HASHDB
    scan_hashdb,
#endif
    scan_elf,
    scan_exif,
    scan_zip,
#ifdef USE_RAR
    scan_rar,
#endif
    scan_gzip,
    scan_evtx,
    scan_ntfsindx,
    scan_ntfslogfile,
    scan_ntfsmft,
    scan_ntfsusn,
    scan_utmp,
    scan_outlook,
    scan_pdf,
    scan_msxml,
    scan_winpe,
    scan_hiberfile,
    scan_winlnk,
    scan_winprefetch,
    scan_windirs,
    scan_vcard,
    scan_sceadan,
    scan_xor,
    scan_sqlite,
    scan_facebook,
    // scanners provided by 4n6ist:
    scan_utmp,
    scan_ntfsmft,
    scan_ntfslogfile,
    scan_ntfsindx,
    scan_evtx,
// these are in the old_scanners directory. They never worked well:
    // scan_extx,  // not ready for prime time
    // scan_httpheader,
    0};
