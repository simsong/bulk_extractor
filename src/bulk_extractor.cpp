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

/****************************************************************
 *** COMMAND LINE OPTIONS
 ****************************************************************/

/* Global find options and the find_list */

FindOptsStruct FindOpts;         // singleton
regex_list     find_list;

/* global alert_list and stop_list
 * These should probably become static class variables
 */

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
    scan_accts,
    scan_base16,
    scan_base64,
    scan_kml,
    scan_email,
    // scan_httpheader,
    scan_gps,
    scan_net,
    scan_find,
    scan_wordlist,
    scan_aes,
    scan_json,
#ifdef HAVE_LIBLIGHTGREP
    scan_accts_lg,
    scan_base16_lg,
    scan_email_lg,
    scan_gps_lg,
    scan_lightgrep,
#endif
#ifdef USE_LIFT
    scan_lift,  // not ready for prime time
#endif
    // scan_extx,  // not ready for prime time
#ifdef HAVE_EXIV2
    scan_exiv2,
#endif
#ifdef HAVE_HASHID
    scan_hashid,
#endif
    scan_elf,
    scan_exif,
    scan_zip,
#ifdef USE_RAR
    scan_rar,
#endif
    scan_gzip,
    scan_outlook,
    scan_pdf,
    scan_winpe,
    scan_hiberfile,
    scan_winprefetch,
    scan_windirs,
    scan_vcard,
    scan_bulk,
    scan_xor,
    0};


