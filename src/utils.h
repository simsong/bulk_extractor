/****************************************************************
 *** utils.h
 *** 
 *** To use utils.c/utils.h, be sure this is in your configure.ac file:

AC_CHECK_HEADERS([err.h err.h sys/mman.h sys/resource.h unistd.h])
AC_CHECK_FUNCS([ishexnumber unistd.h err errx warn warnx pread64 pread _lseeki64 ])

***
 ****************************************************************/



#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <stdint.h>
#include <sys/time.h>

#ifndef __BEGIN_DECLS
#if defined(__cplusplus)
#define __BEGIN_DECLS   extern "C" {
#define __END_DECLS     }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif

__BEGIN_DECLS

#ifdef HAVE_ERR_H
#include <err.h>
#else
void err(int eval,const char *fmt,...) __attribute__((format(printf, 2, 0))) __attribute__ ((__noreturn__));
void errx(int eval,const char *fmt,...) __attribute__((format(printf, 2, 0))) __attribute__ ((__noreturn__));
void warn(const char *fmt, ...) __attribute__((format(printf, 1, 0)));
void warnx(const char *fmt,...) __attribute__((format(printf, 1, 0)));
#endif

#ifndef HAVE_LOCALTIME_R
void localtime_r(time_t *t,struct tm *tm);
#endif

#ifndef HAVE_GMTIME_R
void gmtime_r(time_t *t,struct tm *tm);
#endif

int64_t get_filesize(int fd);

#ifndef HAVE_ISHEXNUMBER
int ishexnumber(int c);
inline int ishexnumber(int c)
{
    switch(c){
    case '0':         case '1':         case '2':         case '3':         case '4':
    case '5':         case '6':         case '7':         case '8':         case '9':
    case 'A':         case 'B':         case 'C':         case 'D':         case 'E':
    case 'F':         case 'a':         case 'b':         case 'c':         case 'd':
    case 'e':         case 'f':
	return 1;
    }
    return 0;
}
#endif
__END_DECLS

/* Useful functions for scanners */
#if defined(__cplusplus)
#include <string>
#define ONE_HUNDRED_NANO_SEC_TO_SECONDS 10000000
#define SECONDS_BETWEEN_WIN32_EPOCH_AND_UNIX_EPOCH 11644473600LL
/*
 * 11644473600 is the number of seconds between the Win32 epoch
 * and the Unix epoch.
 *
 * http://arstechnica.com/civis/viewtopic.php?f=20&t=111992
 */

inline std::string microsoftDateToISODate(const uint64_t &time)
{
    time_t tmp = (time / ONE_HUNDRED_NANO_SEC_TO_SECONDS) - SECONDS_BETWEEN_WIN32_EPOCH_AND_UNIX_EPOCH;
    
    struct tm time_tm;
    gmtime_r(&tmp, &time_tm);
    char buf[256];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &time_tm); // Zulu time
    return std::string(buf);
}
#endif


#endif
