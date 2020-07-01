/*
 *
 * Bulk Extractor's master include file.
 *
 */

#ifndef _BULK_EXTRACTOR_H_
#define _BULK_EXTRACTOR_H_

/* Don't include config.h twice */
#ifndef PACKAGE_NAME
#include "config.h"
#endif

#ifdef WIN32
#  include <winsock2.h>
#  include <windows.h>
#  include <windowsx.h>
#endif

/* required per C++ standard */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifdef _WIN32
/* For some reason this doesn't work properly with mingw */
#undef HAVE_EXTERN_PROGNAME
#endif

#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>


#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif 

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif 

#ifdef HAVE_SYS_DISK_H
# include <sys/disk.h>
#endif

#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif

#ifdef HAVE_SYS_CDEFS_H
# include <sys/cdefs.h>
#endif

#include <pthread.h>                    // must have pthread

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif

/* If we are including inttypes.h, mmake sure __STDC_FORMAT_MACROS is defined */
#ifdef HAVE_INTTYPES_H
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
# include <inttypes.h>
#else
// This should really have been caught during configuration, but just in case...
# error Unable to work without inttypes.h!
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_MMAP_H
#include <mmap.h>
#endif

#ifdef HAVE_SYS_MMAP_H
#include <sys/mmap.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "be13_api/utils.h"

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
# define ATTR_FORMAT(param,arg) __attribute__ ((__printf__,param,arg))
#else
# define ATTR_FORMAT(spec) /* empty */
#endif

#ifdef	__cplusplus
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <list>
#include <iostream>
#include <sstream>
#include <vector>

#include "be13_api/bulk_extractor_i.h"

/* bulk_extractor.cpp */

#include <be13_api/beregex.h>
#include "word_and_context_list.h"

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
#endif

