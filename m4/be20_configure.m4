#
# mix-ins for be20_api
#

AC_MSG_NOTICE([m4/be20_configure.m4 start])
AC_DEFINE(BE20_CONFIGURE_APPLIED, 1, [be20_configure.m4 was included by autoconf.ac])

################################################################
## Endian check. Used for sbuf code.
AC_C_BIGENDIAN([AC_DEFINE(BE20_API_BIGENDIAN, 1, [Big Endian aarchitecutre - like M68K])],
                AC_DEFINE(BE20_API_LITTLEENDIAN, 1, [Little Endian aarchitecutre - like x86]))

################################################################
## Headers
AC_CHECK_HEADERS([ dlfcn.h fcntl.h limits.h limits/limits.h linux/if_ether.h net/ethernet.h netinet/if_ether.h netinet/in.h pcap.h pcap/pcap.h sqlite3.h sys/cdefs.h sys/mman.h sys/stat.h sys/time.h sys/types.h sys/vmmeter.h unistd.h windows.h windows.h windowsx.h winsock2.h wpcap/pcap.h mach/mach.h mach-o/dyld.h])

AC_CHECK_FUNCS([gmtime_r ishexnumber isxdigit localtime_r unistd.h mmap err errx warn warnx pread64 pread strptime _lseeki64 task_info utimes host_statistics64])

################################################################
## Libraries
## Note that we now require pkg-config

AC_CHECK_LIB([sqlite3],[sqlite3_libversion])
AC_CHECK_FUNCS([sqlite3_create_function_v2 sysctlbyname])

AC_MSG_NOTICE([be20_configure: CPPFLAGS are now $CPPFLAGS])

# re2
AC_LANG_PUSH(C++)
AC_CHECK_HEADERS([re2/re2.h])
PKG_CHECK_MODULES([RE2], [re2],
  [
    AC_MSG_NOTICE([re2 detected])
    AC_DEFINE([HAVE_RE2], [1], [Define if you have the RE2 library]) ],
  [AC_MSG_ERROR([Could not find required RE2 library. Please install libre2-dev or equivalent.])]
)
AC_LANG_POP()

################################################################
## Check on two annoying warnings
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[#pragma GCC diagnostic ignored "-Wredundant-decls"
  int a=3;
]])],
 [AC_DEFINE(HAVE_DIAGNOSTIC_REDUNDANT_DECLS,1,[define 1 if GCC supports -Wredundant-decls])]
)

AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[#pragma GCC diagnostic ignored "-Wcast-align"
 int a=3;
  ]])],
 [AC_DEFINE(HAVE_DIAGNOSTIC_CAST_ALIGN,1,[define 1 if GCC supports -Wcast-align])]
)
AC_MSG_NOTICE([m4/be20_configure.m4 end])
