/*
 *
 * Bulk Extractor's master include file.
 * This is being phased out.
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

#ifdef _WIN32
/* For some reason this doesn't work properly with mingw */
#undef HAVE_EXTERN_PROGNAME
#endif

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

//#ifdef HAVE_SYS_TIME_H
//#include <sys/time.h>
//#endif

//#ifdef HAVE_SYS_TYPES_H
//# include <sys/types.h>
//#endif

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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
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

//#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
//# define ATTR_FORMAT(param,arg) __attribute__ ((__printf__,param,arg))
//#else
//# define ATTR_FORMAT(spec) /* empty */
//#endif


/* bulk_extractor.cpp */

#include "be13_api/beregex.h"
#include "be13_api/bulk_extractor_i.h"
#include "word_and_context_list.h"

extern scanner_t *scanners_builtin[];
