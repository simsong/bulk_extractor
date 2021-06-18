#                                 -*- Autoconf -*-
# bulk_extractor configure.ac
#
# Process this file with autoconf to produce a configure script.
# Order is largely irrevellant, although it must start with AC_INIT and end with AC_OUTPUT
# See http://autotoolset.sourceforge.net/tutorial.html
# and http://www.openismus.com/documents/linux/automake/automake.shtml
# and http://www.bioinf.uni-freiburg.de/~mmann/HowTo/automake.html

AC_PREREQ([2.71])
AC_INIT([BULK_EXTRACTOR],[2.0.0-dev],[bugs@digitalcorpora.org])
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_AUX_DIR([build-aux])
################################################################
## Includes

m4_include([m4/slg_mingw_support.m4])
m4_include([m4/slg_searchdirs.m4])
m4_include([m4/slg_gcc_all_warnings.m4])
m4_include([src/be13_api/dfxml/src/dfxml_configure.m4])
m4_include([src/be13_api/be13_configure.m4])

#
# NOTE 1 - plugins/Makefile is referenced so that the plugins Makefile is created.
# However, plugins is not listed as a SUBDIR for Makefile.am in this directory.
# That prevents the plugins from automatically being built each time.
#
# NOTE 2 - src_win/Makefile is not automatically generated. This is necessary
# so that it can be run even after a 'make distclean' is done
AC_CONFIG_FILES([Makefile src/Makefile \
			  man/Makefile python/Makefile \
                          plugins/Makefile \
			  specfiles/Makefile specfiles/bulk_extractor.spec.m4 tests/Makefile ])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_PREFIX_PROGRAM(bulk_extractor) dnl build for same location

# Programs we will be using
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O                    dnl allow per-produce flags
AC_PROG_INSTALL
AC_PROG_LEX([noyywrap])
# LT_INIT([disable-shared])

# Must use C++17 mode.

m4_include([m4/ax_cxx_compile_stdcxx.m4])
AC_LANG_PUSH(C++)
AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])
AC_LANG_POP()

# Turn on all warnings
m4_include([src/be13_api/m4/slg_gcc_all_warnings.m4])

# These are needed for bulk_extractor. They are not needed for tcpflow
AC_DEFINE([BULK_EXTRACTOR],1,[We are compiling bulk_extractor])

if test x"$LEX" == x ; then
  AC_MSG_ERROR([flex not required.])
fi

if test `basename "$LEX"` != flex; then
  AC_MSG_ERROR([gnu flex is required.])
fi

## Determine if we can use flex -R
if eval "echo %% | $LEX -R -t > /dev/null 2>&1"
then
  AC_MSG_NOTICE([$LEX supports -R])
else
  AC_MSG_ERROR([$LEX does not support -R.  Please get a modern version of gnu flex])
fi

################################################################
## compile with pthread if its available
SAVE_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -pthread"
AC_MSG_CHECKING([whether C++ compiler understands $option])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
     [AC_MSG_RESULT([Adding -pthread to CXXFLAGS])],
     [AC_MSG_RESULT([Compiler does not understand -pthread]); CXXFLAGS="$SAVE_CXXFLAGS"])
unset SAVE_CXXFLAGS


################################################################
## rar support
AC_ARG_ENABLE([rar],
              AS_HELP_STRING([--disable-rar], [Disable RAR decompression]),
              [],
	      [AC_DEFINE(USE_RAR, 1, [Use RAR decompression]) rar="yes"])
AM_CONDITIONAL([RAR_ENABLED], [test "yes" = "$rar"])


################################################################
## lightgrep enabled
AC_ARG_ENABLE([lightgrep],
              AS_HELP_STRING([--enable-lightgrep], [enable LIGHTGREP]),
	      [AC_DEFINE(USE_LIGHTGREP, 1, [Use LIGHTGREP]) lightgrep="yes"],
              [])
AM_CONDITIONAL([LIGHTGREP_ENABLED], [test "yes" = "$lightgrep"])
AC_ARG_ENABLE([flexscanners],
              AS_HELP_STRING([--disable-flexscanners], [disable FLEX-based scanners]),
              [],
              [AC_DEFINE(USE_FLEXSCANNERS, 1, [Use FLEX-based scanners]) flexscanners='yes'])
AM_CONDITIONAL([FLEXSCANNERS_ENABLED], [test "yes" = "$flexscanners"])


################################################################
## mcheck support
## enable memory analysis support for detecting memory leaks
## and detecting multiply deallocated memory resources
## see http://en.wikipedia.org/wiki/Mtrace
## here is the basic workflow:
##   1. example usage and recompile: "./configure --enable-mcheck"
##   2. export the environment variable for the output file,
##        e.g. "export MALLOC_TRACE=~/malloc_outfile.txt"
##   3. improve readability of malloc_outfile.txt using mtrace,
##        e.g. "mtrace /usr/local/bin/bulk_extractor ~/malloc_outfile.txt
AC_ARG_ENABLE([mcheck],
              [AS_HELP_STRING([--enable-mcheck],
                            [enable memory analysis support for detecting memory leaks
                             and detecting multiply deallocated memory resources])],
              [AC_DEFINE(HAVE_MCHECK, 1,
                [define the output file using global variable MALLOC_TRACE,
                 for example "MALLOC_TRACE=./mcheck.out; export MALLOC_TRACE".
                 Then post-process unreadable file mcheck.out using the mtrace command line tool,
                 for example "mtrace /usr/local/bin/bulk_extractor mcheck.out".
                 Unfortunately, only addresses with written code are looked up,
                 in otherwords, use of malloc is looked up, but use of new is not.])]
                 )


################################################################
## AddressSanitizer support
AC_ARG_ENABLE([address-sanitizer],
              [AS_HELP_STRING([--enable-address-sanitizer],
                              [enabled AddressSanitizer support for detecting a wide variety of
                               memory allocation and deallocation errors])],
              [AC_DEFINE(HAVE_ADDRESS_SANITIZER, 1, [enable AddressSanitizer])
              address_sanitizer="yes"
              CFLAGS="$CFLAGS -fsanitize=address"
              CXXFLAGS="$CXXFLAGS -fsanitize=address"
              ],
              [])


# Specific include files and functions for bulk-extractor
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_TYPE_INT64_T
AC_SYS_LARGEFILE
AC_SYS_POSIX_TERMIOS

## Check for headers used by bulk Extractor
AC_CHECK_HEADERS([alloca.h dirent.h dlfcn.h err.h errno.h fcntl.h inttypes.h libgen.h limits.h malloc.h mmap.h pwd.h signal.h stdarg.h stdint.h stdio.h strings.h string.h stdlib.h sys/cdefs.h sys/disk.h sys/fcntl.h sys/ioctl.h sys/mman.h sys/mmap.h sys/mount.h sys/param.h sys/socket.h sys/stat.h sys/types.h sys/time.h sys/resource.h time.h unistd.h windows.h CoreServices/CoreServices.h])
AC_CHECK_FUNCS([getuid getpwuid gethostname getrusage gmtime_r isxdigit ishexnumber le64toh localtime_r _lseeki64 inet_ntop pread64 pread printf mmap munmap MD5 mkstemp mktemp sleep SleepEx strptime usleep vasprintf])
AC_CHECK_FUNCS([CreateProcess LoadLibrary IncrementAtomic InterlockedIncrement])

## dlopen is now itself in a different library
## Explicitly check for dlopen library before checking for dlopen
##       -ldl (see http://stackoverflow.com/questions/956640/linux-c-error-undefined-reference-to-dlopen)
AC_CHECK_LIB([dl],[dlopen])
AC_CHECK_FUNCS([dlopen dlopen_preflight])

# Test for sin_len
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h wsipx.h])
AC_CHECK_HEADERS([netinet/ip.h], [], [],
[[
#include <sys/types.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
]])

AC_CHECK_MEMBER([struct sockaddr_in.sin_len],
		[AC_DEFINE(HAVE_SOCKADDR_IN_SIN_LEN, 1, [Do we have sockaddr.sin_len?])],
		[],
		[
#include <sys/types.h>
#include <arpa/inet.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
		])

# Should we disable optimization?
AC_ARG_WITH([opt], AS_HELP_STRING([--without-opt],[Drop all -O C flags]))
# Or maybe just tone it down a bit?
AC_ARG_WITH([o3], AS_HELP_STRING([--without-o3],[Do not force O3 optimization; use default level]))

#
# ZLIB is required:
# Note you cannot put comments in the AC_MSG_ERROR for some reason
# Must be before ewf
AC_CHECK_LIB([z],[uncompress],,
	AC_MSG_ERROR([zlib libraries not installed; try installing zlib-devel zlib-dev zlib-devel zlib1g-dev or libz-dev]))

## EXPAT is required for reading the dfxml file for restrarting.
AC_CHECK_HEADERS([expat.h])
AC_CHECK_LIB([expat],[XML_ParserCreate])

################################################################
## Lightgrep support
##
AC_CHECK_LIB([stdc++],[main])

if test x"$lightgrep" == x"yes"; then
  m4_ifndef([PKG_CHECK_MODULES],
            [AC_MSG_ERROR([pkg-config autoconf macros are missing; try installing pkgconfig])])

  if test x"$mingw" == x"yes" ; then
    # get static flags when cross-compiling with mingw
    PKG_CONFIG="$PKG_CONFIG --static"
  else
    # pkg-config doesn't look in /usr/local/lib on some systems
    if test x"$PKG_CONFIG_PATH" != x; then
      export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig
    else
      export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
    fi
  fi

  m4_ifdef([PKG_CHECK_MODULES],
           [PKG_CHECK_MODULES([lightgrep], [lightgrep])])

  AC_DEFINE([HAVE_LIBLIGHTGREP], 1, [Define to 1 if you have liblightgrep.])

  CPPFLAGS="$CPPFLAGS $lightgrep_CFLAGS"
  LIBS="$LIBS `$PKG_CONFIG --libs-only-l lightgrep`"
  LDFLAGS="$LDFLAGS `$PKG_CONFIG --libs-only-L --libs-only-other lightgrep`"
fi

################################################################
## LIBEWF support

AC_ARG_ENABLE([libewf],
    [AS_HELP_STRING([--disable-libewf], [disable libewf support])],
    [libewf=no],
    [libewf=yes])
AC_MSG_NOTICE([libewf is $libewf])
if test x"$libewf" == x"yes" ; then
  AC_CHECK_HEADER([libewf.h],
	[AC_DEFINE(HAVE_LIBEWF_H,1,[Do we have libewf.h?])]
	[AC_CHECK_LIB([ewf],[libewf_get_version],,
		[AC_MSG_WARN([libewf not found; no E01 support])]
		[libewf=no])],
	[AC_MSG_WARN([libewf.h not found; no E01 support])]
	[libewf=no])
  AC_CHECK_FUNCS([libewf_handle_get_utf8_header_value_notes libewf_error_backtrace_fprint libewf_handle_close libewf_handle_free libewf_handle_read_buffer_at_offset libewf_handle_read_random])
fi
AC_MSG_NOTICE([libewf is now $libewf])


################################################################
## exiv2 support
## now that exif is in place, exiv2 is an optional scanner.
## On mingw we also need libiconv
## We should probably test to make sure that exiv2 works too

AC_ARG_ENABLE([exiv2],[AS_HELP_STRING([--enable-exiv2=true to check for exiv2; warning: exiv2 crashes])],
   exiv2=yes,
   exiv2=no)
if test x"$exiv2" == x"yes" ; then
  AC_CHECK_LIB([iconv],[libiconv_open])
  if test "${mingw}" == yes ; then
    AC_MSG_WARN([exiv2 requires use of the DLL which must be present])
    if test `uname -s` = "Linux" ; then
      AC_MSG_WARN([exiv2 is not working properly with mingw and Linux cross-compiling; disabled])
    exiv2=no
    fi
  fi
fi
if test x"$exiv2" == x"yes" ; then
  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([exiv2/image.hpp exiv2/exif.hpp exiv2/error.hpp])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
         #include <exiv2/image.hpp>
    	 #include <exiv2/exif.hpp>
         #include <exiv2/error.hpp>
         ]],
         [[Exiv2::ImageFactory::open(0,0);]])],
         [exiv2=yes],
         [exiv2=no])
    if test "${exiv2}" = yes; then
      AC_DEFINE(HAVE_EXIV2,1,[define 1 if EXIV2 exists and works])
      echo Using EXIV2
      if test "${mingw}" = no ; then
        LIBS="$LIBS -lexiv2"
      else
        LIBS="$LIBS -lexiv2.dll"	# static version does not work on mingw
      fi
      dnl see if we have the error feature
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
            #include <exiv2/image.hpp>
    	    #include <exiv2/exif.hpp>
            #include <exiv2/error.hpp>
            ]],
            [[Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);]],
  	    [AC_DEFINE(HAVE_EXIV2__LOGMSG__SETLEVEL,1,[define 1 if EXIV2 exists and works])])])
    fi
  AC_LANG_POP()
fi

m4_include([m4/slg_check_gcc_diagnostics.m4])

## If compiling on mingw, add another -lgdi32 to be safe
## This is required becuase some libraries reference functions in gdi32 and, for whatever reason,
## the linker is not smart enough to re-check the library
if test x"$mingw" = x"yes" ; then
  LIBS="$LIBS -lgdi32"
else
  CFLAGS="$CFLAGS -fPIC"
  CXXFLAGS="$CXXFLAGS -fPIC"
fi


################################################################
############## drop optimization flags and add -g if requested ################
if test x"${AFF_NOOPT}" != "x" ; then
   with_noopt="yes";
fi

if test "${with_opt}" = "no" ; then
  CFLAGS=`echo   -g "$CFLAGS"   | sed s/-O[[0-9]]//`             # note the double quoting!
  CXXFLAGS=`echo -g "$CXXFLAGS" | sed s/-O[[0-9]]//`
else
  # If we are not stripping the optimizer, add fortify source
  CFLAGS="  $CFLAGS   -D_FORTIFY_SOURCE=2 "
  CXXFLAGS="$CXXFLAGS -D_FORTIFY_SOURCE=2 "

  # and increase optimizer from -O2 to -O3 if not explicitly forbidden
  if test "${with_o3}" != "no" ; then
      CFLAGS=`echo   -g "$CFLAGS"   | sed s/-O2/-O3/`             # note the double quoting!
      CXXFLAGS=`echo -g "$CXXFLAGS" | sed s/-O2/-O3/`
  fi
fi

AC_MSG_NOTICE([*************************************])
AC_MSG_NOTICE([*************************************])
AC_MSG_NOTICE([  PACKAGE_NAME:     $PACKAGE_NAME])
AC_MSG_NOTICE([  PACKAGE_VERSION:  $PACKAGE_VERSION])
AC_MSG_NOTICE([  CC:               $CC])
AC_MSG_NOTICE([  CXX:              $CXX])
AC_MSG_NOTICE([  CPPFLAGS:         $CPPFLAGS])
AC_MSG_NOTICE([  CFLAGS:           $CFLAGS])
AC_MSG_NOTICE([  CXXFLAGS:         $CXXFLAGS])
AC_MSG_NOTICE([  LIBS:             $LIBS])
AC_MSG_NOTICE([  LDFLAGS:          $LDFLAGS])

#
# Generate special warnings
#

if test x"${libewf}" != "xyes" ; then
  AC_MSG_WARN([libewf-devel must be installed for E01 support.])
fi

AC_OUTPUT

## Finally, record the values of CFLAGS, CPPFLAGS, and CXXFLAGS for DFXML
echo "#define CPPFLAGS \"$CPPFLAGS\"" >> config.h
echo "#define CFLAGS   \"$CFLAGS\""   >> config.h
echo "#define CXXFLAGS \"$CXXFLAGS\"" >> config.h
echo "#define LIBS     \"$LIBS\""     >> config.h
echo "#define LDFLAGS  \"$LDFLAGS\""  >> config.h
if test x"$GIT_COMMIT" != "x" ; then
  echo "#define GIT_COMMIT  \"$GIT_COMMIT\""  >> config.h
fi
