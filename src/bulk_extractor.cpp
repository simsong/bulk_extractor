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

#ifdef HAVE_EXPAT_H
#include <expat.h>
#endif

#ifdef HAVE_MCHECK
#include <mcheck.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

/****************************************************************
 *** COMMAND LINE OPTIONS
 ****************************************************************/

// Global options that can be set without using the option system

uint64_t debug=0;
int      opt_silent= 0;

/* Global find options and the find_list */

FindOptsStruct FindOpts;         // singleton
regex_list     find_list;

/* global alert_list and stop_list
 * These should probably become static class variables
 */
word_and_context_list alert_list;		/* shold be flagged */
word_and_context_list stop_list;		/* should be ignored */

/**
 * Singletons:
 * histograms_t histograms - the set of histograms to make
 * feature_recorder_set fs - the collection of feature recorders.
 * xml xreport             - the DFXML output.
 * image_process p         - the image being processed.
 * 
 * Note that all of the singletons are passed to the phase1() function.
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
    //    scan_httpheader,
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
    //scan_extx,  // not ready for prime time
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
    scan_pdf,
    scan_winpe,
    scan_hiberfile,
    scan_winprefetch,
    scan_windirs,
    scan_vcard,
    scan_bulk,
    scan_xor,
    0};


/* Here is the bulk_extractor API */
struct BEFILE_t {
};

typedef struct BEFILE_t BEFILE;
typedef int be_callback(int32_t flag,
                        uint32_t arg,
                        const char *feature_recorder_name,
                        const char *feature,size_t feature_len,
                        const char *context,size_t context_len);


BEFILE *bulk_extractor_open()
{
    return 0;
}

int bulk_extractor_analyze_buf(BEFILE *bef,be_callback cb,uint8_t *buf,size_t buflen)
{
    return 0;
}

int bulk_extractor_close(BEFILE *bef)
{
    return 0;
}

