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

using namespace std;

#include "be13_api/bulk_extractor_i.h"

/* bulk_extractor.cpp */

/****************************************************************
 *** SCANNER PROCESSORS - operate on the scanners
 ****************************************************************/

/* support.cpp */

void		truncate_at(string &line,char ch);
std::string	ssprintf(const char *fmt,...);
bool		ends_with(const std::string &,const std::string &with);
bool		ends_with(const std::wstring &,const std::wstring &with);

/* C++ string splitting code from http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c */
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim); 

/****************************************************************/

#include "word_and_context_list.h"

/* The global lists for alerting and stopping */
extern word_and_context_list alert_list;		/* should be flagged */
extern word_and_context_list stop_list;		/* should be ignored */

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
/* Special support for find */
extern "C" scanner_t scan_find;
extern regex_list find_list;
void add_find_pattern(const string &pat);
void process_find_file(const char *findfile);

#ifdef HAVE_EXIV2
extern "C" scanner_t scan_exiv2;
#endif
extern "C" scanner_t scan_aes;
extern "C" scanner_t scan_bulk;
extern "C" scanner_t scan_elf;
extern "C" scanner_t scan_exif;
extern "C" scanner_t scan_gzip;
extern "C" scanner_t scan_hiberfile;
extern "C" scanner_t scan_json;
extern "C" scanner_t scan_lightgrep;
extern "C" scanner_t scan_pdf;
extern "C" scanner_t scan_winpe;
extern "C" scanner_t scan_winprefetch;
extern "C" scanner_t scan_zip;
extern "C" scanner_t scan_rar;
extern "C" scanner_t scan_windirs;
extern "C" scanner_t scan_xor;

#endif
#endif

