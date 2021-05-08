#ifndef SCANNERS_H
#define SCANNERS_H

#include "be13_api/scanner_params.h"
extern scanner_t *scanners_builtin[];

/****************************************************************
 *** SCANNER PROCESSORS - operate on the scanners
 ****************************************************************/

/****************************************************************/


/* flex-based scanners */
extern "C" scanner_t scan_email;
extern "C" scanner_t scan_accts;
extern "C" scanner_t scan_kml;
extern "C" scanner_t scan_gps;

/* Regular scanners */
extern "C" scanner_t scan_wordlist;
extern "C" scanner_t scan_net;
extern "C" scanner_t scan_base16;
extern "C" scanner_t scan_base64;
extern "C" scanner_t scan_vcard;
extern "C" scanner_t scan_lift;
extern "C" scanner_t scan_extx;
extern "C" scanner_t scan_find;

#ifdef HAVE_EXIV2
extern "C" scanner_t scan_exiv2;
#endif
#ifdef HAVE_HASHDB
extern "C" scanner_t scan_hashdb;
#endif
extern "C" scanner_t scan_aes;
extern "C" scanner_t scan_bulk;
extern "C" scanner_t scan_sceadan;
extern "C" scanner_t scan_elf;
extern "C" scanner_t scan_exif;
extern "C" scanner_t scan_gzip;
extern "C" scanner_t scan_hiberfile;
extern "C" scanner_t scan_httplogs;
extern "C" scanner_t scan_json;
#ifdef HAVE_LIBLIGHTGREP
extern "C" scanner_t scan_accts_lg;
extern "C" scanner_t scan_base16_lg;
extern "C" scanner_t scan_email_lg;
extern "C" scanner_t scan_gps_lg;
extern "C" scanner_t scan_lightgrep;
#endif
extern "C" scanner_t scan_facebook;
extern "C" scanner_t scan_ntfsusn;
extern "C" scanner_t scan_pdf;
extern "C" scanner_t scan_msxml;
extern "C" scanner_t scan_winlnk;
extern "C" scanner_t scan_winpe;
extern "C" scanner_t scan_winprefetch;
extern "C" scanner_t scan_zip;
extern "C" scanner_t scan_rar;
extern "C" scanner_t scan_windirs;
extern "C" scanner_t scan_xor;
extern "C" scanner_t scan_outlook;
extern "C" scanner_t scan_sqlite;

    // scanners provided by 4n6ist:
extern "C" scanner_t scan_utmp;
extern "C" scanner_t scan_ntfsmft;
extern "C" scanner_t scan_ntfslogfile;
extern "C" scanner_t scan_ntfsindx;
extern "C" scanner_t scan_evtx;

#endif
