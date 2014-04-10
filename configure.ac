#                                 -*- Autoconf -*-
# bulk_extractor configure.ac
#
# Process this file with autoconf to produce a configure script.
# Order is largely irrevellant, although it must start with AC_INIT and end with AC_OUTPUT
# See http://autotoolset.sourceforge.net/tutorial.html
# and http://www.openismus.com/documents/linux/automake/automake.shtml

AC_PREREQ(2.57)
AC_INIT(BULK_EXTRACTOR, 1.4.5devel, bugs@digitalcorpora.org)

AC_CONFIG_MACRO_DIR(m4)

# 
# NOTE 1 - plugins/Makefile is referenced so that the plugins Makefile is created.
# However, plugins is not listed as a SUBDIR for Makefile.am in this directory.
# That prevents the plugins from automatically being built each time.
#
# NOTE 2 - src_win/Makefile is not automatically generated. This is necessary
# so that it can be run even after a 'make distclean' is done
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile doc/latex_manuals/Makefile \
			  man/Makefile python/Makefile \
                          plugins/Makefile \
			  specfiles/Makefile specfiles/bulk_extractor.spec.m4 \
			  java_gui/Makefile tests/Makefile ])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_PREFIX_PROGRAM(bulk_extractor) dnl build for same location

# Programs we will be using
AC_PROG_CC
AC_PROG_CC_C99 
AC_PROG_CXX
AM_PROG_CC_C_O                    dnl allow per-produce flags
AC_PROG_INSTALL
AC_PROG_LEX
# LT_INIT([disable-shared])

# use C++11 mode if available; HAVE_CXX11 is defined in config.h if so.  Don't
# use the GNU C++11 extensions for portability's sake (noext).
AC_LANG_PUSH(C++)
AX_CXX_COMPILE_STDCXX_11(noext, optional)
AC_LANG_POP()

# Endian check is required for MD5 implementation
AC_C_BIGENDIAN

# These are needed for bulk_extractor. They are not needed for tcpflow
AC_DEFINE([BULK_EXTRACTOR],1,[We are compiling bulk_extractor])
# AC_DEFINE([USE_HISTOGRAMS],1,[We are using histograms])
# AC_DEFINE([USE_STOP_LIST],1,[We are using the stop lists])
# AC_DEFINE([USE_ALERT_LIST],1,[We are using the alert list])

## 
## BEViewer
AC_ARG_ENABLE(BEViewer,
    [AS_HELP_STRING([--disable-BEViewer],[disable BEViewer even if components for building it are available])], 
      use_BEViewer=no, 
      use_BEViewer=yes)
if test "x$use_BEViewer" == "xyes"; then
    # validate availability of requisite programs

    # javac
    AC_CHECK_PROG(has_javac, javac, true, false)
    if test x"$has_javac" == "xtrue" ; then
      # javac must be version 6 or later, indicated by the minor number
      javac_version=`javac -version 2>&1`
      javac_minor=`echo "$javac_version" | sed -e 's/javac 1\.//' | sed -e 's/\..*//'`
      if test $javac_minor ; then
        if test $javac_minor -lt "6" ; then
          has_javac=false
        fi
      else
        has_javac=false
      fi
    fi

    if test x"$has_javac" == "xfalse" ; then
        AC_MSG_NOTICE([javac 6 or later is not available for compiling BEViewer.  Try 'apt-get install java-1.7.0-openjdk-devel' or 'yum install java-1.7.0-openjdk-devel' or 'port install java-1.7.0-openjdk-devel' or whatever package manager you happen to be using....])
    fi

    # jar
    AC_CHECK_PROG(has_jar, jar, true, false)
    if test x"$has_jar" == "xfalse" ; then
        AC_MSG_NOTICE([jar is not available for compiling BEViewer.  Try 'apt-get install java-1.7.0-openjdk-devel' or 'yum install java-1.7.0-openjdk-devel' or 'port install java-1.7.0-openjdk-devel' or whatever package manager you happen to be using....])
    fi
fi
AM_CONDITIONAL([WANT_BE_VIEWER], [test x$has_javac == xtrue -a x$has_jar == xtrue])

##
## NSIS Windows Installer
AC_ARG_ENABLE(win_installer,
    [AS_HELP_STRING([--enable-win_installer],[enable genration of the Windows installer, this also requires BEViewer])], 
      request_win_installer=yes, 
      request_win_installer=no)
if test x"$request_win_installer" == "xyes" ; then
    AC_CHECK_PROG(has_nsis, makensis, true, false)
    if test x"$has_nsis" == "xfalse" ; then
      AC_MSG_NOTICE([nsis is not available for creating the Windows installer.  Try 'apt-get install mingw32-nsis' or 'yum install mingw32-nsis' or 'port install mingw32-nsis' or whatever package manager you happen to be using....])
    fi
fi
AM_CONDITIONAL([WANT_WIN_INSTALLER], [test x"$has_nsis" == xtrue -a x"$has_javac" == xtrue -a x"$has_jar" == xtrue])

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
## See if we are running on mingw
# http://osdir.com/ml/gnu.mingw.devel/2003-09/msg00040.html
# Note: Windows 95 WINVER=0x400
# Windows 98 WINVER=0x400  _WIN32_WINDOWS=0x0410
# Windows Me WINVER=0x400  _WIN32_WINDOWS=0x0490
# Windows NT 4.0 WINVER=0x0400  _WIN32_WINNT=0x0400
# Windows NT 4.0 SP3 WINVER=0x0400 _WIN32_WINNT=0x0403
# Windows 2000 WINVER=0x500 _WIN32_WINNT=0x0500
# Windows XP WINVER=0x501 _WIN32_WINNT=0x0501
# Windows Server 2003 WINVER=0x502 _WIN32_WINNT=0x0502
#
# mingw32 includes  i686-w64-mingw32 and  x86_64-w64-mingw32

mingw="no"
case $host in
  *-*-*linux*-*) 
     AC_DEFINE([__LINUX__],1,[Linux operating system functions])
     ;;

  *mingw*)   
     LIBS="$LIBS -lpsapi -lws2_32 -lgdi32"  
     CPPFLAGS="-DUNICODE -D_UNICODE -D__MSVCRT_VERSION__=0x0601 -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -g $CPPFLAGS"
     CPPFLAGS="$CPPFLAGS --static "
     CFLAGS="$CFLAGS --static -static-libgcc -static-libstdc++"
     CXXFLAGS="$CXXFLAGS -Wno-format "  # compiler mingw-4.3.0 is broken on I64u formats
     CXXFLAGS="$CXXFLAGS --static -static-libgcc -static-libstdc++"
     LDFLAGS="$LDFLAGS --static"
     mingw="yes"
     ;;		 		     
esac

if test x"${mingw}" == "xno" ; then
  # Bring additional directories where things might be found into our
  # search path. I don't know why autoconf doesn't do this by default
  for spfx in /usr/local /opt/local /sw /usr/local/ssl; do
    AC_MSG_NOTICE([checking ${spfx}/include])
    if test -d ${spfx}/include; then
        CPPFLAGS="-I${spfx}/include $CPPFLAGS"
        LDFLAGS="-L${spfx}/lib $LDFLAGS"
        AC_MSG_NOTICE([ *** ADDING ${spfx}/include to CPPFLAGS *** ])
        AC_MSG_NOTICE([ *** ADDING ${spfx}/lib to LDFLAGS *** ])
    fi
  done
  AC_MSG_NOTICE([ CPPFLAGS = ${CPPFLAGS} ])        
  AC_MSG_NOTICE([ LDFLAGS = ${LDFLAGS} ])        
fi

if test -r /bin/uname.exe ; then
  if test `uname -o` == "Msys" ; then
    AC_MSG_NOTICE([Compiling with Msys. Setting flags appropriately.])
    LIBS="$LIBS -lws2_32 -lgdi32"
    LDFLAGS="$LDFLAGS -Wl,--enable-auto-import"
  fi
fi
################################################################
#
# Enable all the compiler debugging we can find
#
# This is originally from PhotoRec, but modified substantially by Simson
# Figure out which flags we can use with the compiler. 
#
# These I don't like:
# -Wdeclaration-after-statement -Wconversion
# doesn't work: -Wunreachable-code 
# causes configure to crash on gcc-4.2.1: -Wsign-compare-Winline 
# causes warnings with unistd.h:  -Wnested-externs 
# Just causes too much annoyance: -Wmissing-format-attribute 
# Don't use: -Wno-unknown-warning-option

# Check GCC and clang
WARNINGS_TO_TEST="-MD -Wpointer-arith -Wmissing-declarations -Wmissing-prototypes \
    -Wshadow -Wwrite-strings -Wcast-align -Waggregate-return \
    -Wbad-function-cast -Wcast-qual -Wundef -Wredundant-decls -Wdisabled-optimization \
    -Wfloat-equal -Wmultichar -Wc++-compat -Wmissing-noreturn -funit-at-a-time \
    -Qunused-arguments -Wno-deprecated-register"

if test x"${mingw}" == "xno" ; then
  # add the warnings we do not want to do on mingw
  WARNINGS_TO_TEST="$WARNINGS_TO_TEST -Wall -Wstrict-prototypes"
fi

echo "Warnings to test: $WARNINGS_TO_TEST"

for option in $WARNINGS_TO_TEST
do
  SAVE_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $option"
  AC_MSG_CHECKING([whether gcc understands $option])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
      [has_option=yes],
      [has_option=no; CFLAGS="$SAVE_CFLAGS"])
  AC_MSG_RESULT($has_option)
  unset has_option
  unset SAVE_CFLAGS
  if test $option = "-Wmissing-format-attribute" ; then
    AC_DEFINE(HAVE_MISSING_FORMAT_ATTRIBUTE_WARNING,1,
		[Indicates that we have the -Wmissing-format-attribute G++ warning])
  fi
done
unset option

################################################################
## Includes

m4_include([src/dfxml/src/dfxml_configure.m4])
m4_include([src/be13_api/be13_configure.m4])
m4_include([src/sceadan/sceadan_configure.m4])

################################################################

## sceadan support
## Enabled for 1.5 release
AC_ARG_ENABLE([sceadan],
              AS_HELP_STRING([--enable-sceadan], [enable UTSA SCEADAN SVM for type identification]),
	      [AC_DEFINE(USE_SCEADAN, 1, [Use SCEADAN FILE TYPE CLASSIFIER]) sceadan="yes"],
              [])
AM_CONDITIONAL([SCEADAN_ENABLED], [test "yes" = "$sceadan"])


## rar support
## Enabled for 1.4 release!
AC_ARG_ENABLE([rar],
              AS_HELP_STRING([--disable-rar], [Disable RAR decompression]),
              [],
	      [AC_DEFINE(USE_RAR, 1, [Use RAR decompression]) rar="yes"])
AM_CONDITIONAL([RAR_ENABLED], [test "yes" = "$rar"])



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
AC_ARG_ENABLE([mcheck], AS_HELP_STRING([--enable-mcheck], \
              [enable memory analysis support for detecting memory leaks \
               and detecting multiply deallocated memory resources]), \
              AC_DEFINE(HAVE_MCHECK, 1, \
              [define the output file using global variable MALLOC_TRACE, \
               for example "MALLOC_TRACE=./mcheck.out; export MALLOC_TRACE". \
               Then post-process unreadable file mcheck.out using the mtrace command line tool, \
               for example "mtrace /usr/local/bin/bulk_extractor mcheck.out".  \
               Unfortunately, only addresses with written code are looked up, \
               in otherwords, use of malloc is looked up, but use of new is not.]))


################################################################
# Check G++
# We don't use these warnings:
# -Waggregate-return -- aggregate returns are GOOD; they simplify code design
# We can use these warnings after ZLIB gets upgraded:
# -Wundef  --- causes problems with zlib
# -Wcast-qual 
# -Wmissing-format-attribute  --- Just too annoying
# don't use: -Wno-unknown-warning-option
AC_LANG_PUSH(C++)
AC_CHECK_HEADERS([string])

CXX_WARNINGS_TO_TEST="-Wall -MD -Wpointer-arith \
    -Wshadow -Wwrite-strings -Wcast-align  \
    -Wredundant-decls -Wdisabled-optimization \
    -Wfloat-equal -Wmultichar -Wmissing-noreturn \
    -Woverloaded-virtual -Wsign-promo -funit-at-a-time -Qunused-arguments -Wno-deprecated-register"

clang=no
if (g++ --version 2>&1 | grep clang > /dev/null) ; 
then 
   clang=yes
fi
if test x$CXX == "xclang++" ; then
  clang=yes
fi

if test $clang == "no" ; then
  # -Wstrict-null-sentinel is not supported under clang
  CXX_WARNINGS_TO_TEST="$CXX_WARNINGS_TO_TEST -Wstrict-null-sentinel"
fi

if test x"${mingw}" == "xno" ; then
  # add the warnings we don't want to do on mingw
  CXX_WARNINGS_TO_TEST="$CXX_WARNINGS_TO_TEST  -Weffc++"
fi

for option in $CXX_WARNINGS_TO_TEST
do
  SAVE_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $option"
  AC_MSG_CHECKING([whether g++ understands $option])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
      [has_option=yes],
      [has_option=no; CXXFLAGS="$SAVE_CXXFLAGS"])
  AC_MSG_RESULT($has_option)
  unset has_option
  unset SAVE_CXXFLAGS
done
unset option
AC_LANG_POP()    

################################################################
# PTHREAD support
# With special nods to compiling under mingw

if test  x"$mingw" = x"yes";  then
  CFLAGS="$CFLAGS -mthreads "
  CPPFLAGS="-DPTW32_STATIC_LIB $CPPFLAGS"
  CXXFLAGS="$CXXFLAGS -mthreads "
  AC_DEFINE(HAVE_PTHREAD,1,[Defined to POSIX threads for mingw])
else
  m4_include([m4/ax_pthread.m4])
  AC_MSG_CHECKING([Running pthread test from http://www.gnu.org/software/autoconf-archive/ax_pthread.html..])
  AX_PTHREAD([
              echo Using settings from AX_PTHREAD
              LIBS="$PTHREAD_LIBS $LIBS"
              CFLAGS="  $PTHREAD_CFLAGS $CFLAGS"
              CXXFLAGS="$PTHREAD_CFLAGS $CXXFLAGS "
              CC="$PTHREAD_CC"],
            [AC_MSG_ERROR(bulk_extractor requires pthreads)])
fi

AC_CHECK_HEADERS([pthread.h])
AC_CHECK_LIB([pthreadGC2],[pthread_create])

################################################################
##
## Multithreading stuff

# On mingw, be sure to use the static version and be sure we are using mthread option
# (which should be a no-op on later version of G++ anyway)

AC_CHECK_FUNCS([pthread_win32_process_attach_np pthread_win32_process_detach_np pthread_win32_thread_attach_np pthread_win32_thread_detach_np ])

AC_MSG_CHECKING([for __sync_fetch_and_add])
AC_LINK_IFELSE(
   [AC_LANG_SOURCE([
    int main(void) { int i; return __sync_fetch_and_add(&i, 0); }
   ])],
   [
     AC_MSG_RESULT([yes])
     AC_DEFINE(HAVE___SYNC_FETCH_AND_ADD,1,[Defined if __sync_fetch_and_add was detected])
    ],
   [AC_MSG_RESULT([no])]
)

# end PTHREAD SUPPORT
################################################################

# Specific include files and functions for bulk-extractor
AC_HEADER_STDC
AC_TYPE_INT64_T
AC_SYS_LARGEFILE
AC_SYS_POSIX_TERMIOS

## Check for headers used by bulk Extractor
## do not put pthread here
AC_CHECK_HEADERS([alloca.h dirent.h dlfcn.h err.h errno.h fcntl.h inttypes.h libgen.h limits.h malloc.h mmap.h pwd.h signal.h stdarg.h stdint.h stdio.h strings.h string.h stdlib.h sys/cdefs.h sys/disk.h sys/fcntl.h sys/ioctl.h sys/mman.h sys/mmap.h sys/mount.h sys/param.h sys/socket.h sys/stat.h sys/types.h sys/time.h sys/resource.h sys/sysctl.h time.h unistd.h windows.h CoreServices/CoreServices.h])
AC_CHECK_FUNCS([err errx getuid getpwuid gethostname getrusage gmtime_r isxdigit ishexnumber le64toh localtime_r _lseeki64 inet_ntop pread64 pread printf mmap munmap MD5 mkstemp mktemp random srandom srandomdev sleep SleepEx strptime usleep vasprintf warn warnx])
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
AC_ARG_WITH([opt], AC_HELP_STRING([--without-opt], [Drop all -O C flags]))
# Or maybe just tone it down a bit?
AC_ARG_WITH([o3], AC_HELP_STRING([--without-o3], [Do not force O3 optimization; use default level]))

#
# ZLIB is required:
#  Note you cannot put comments in the AC_MSG_ERROR for some reason
AC_CHECK_LIB([z],[uncompress],,
	AC_MSG_ERROR([zlib libraries not installed; try installing zlib-devel zlib-dev zlib-devel zlib1g-dev or libz-dev]))

## EXPAT is now a main line requirements for restarting
AC_CHECK_HEADERS([expat.h])
AC_CHECK_LIB([expat],[XML_ParserCreate])

################################################################
## regex support
## there are several options
## tre is better than regex
AC_CHECK_LIB([stdc++],[main])
AC_CHECK_LIB([sicudt],[main])
AC_CHECK_HEADERS([unicode/ucnv.h])
AC_SEARCH_LIBS([uset_close], [sicuuc])
AC_CHECK_LIB([sicuuc],[main])
AC_CHECK_HEADERS([lightgrep/api.h])
AC_CHECK_LIB([lightgrep], [lg_create_pattern])
AC_CHECK_HEADERS([regex.h tre/tre.h])
AC_CHECK_LIB([regex],[regcomp])
AC_CHECK_LIB([tre],[tre_regcomp])
AC_CHECK_FUNCS([regcomp tre_regcomp tre_version])

################################################################
## AFFLIB support
## AFFLIB requires OpenSSL
## Link in openssl if it is available, because afflib requires it.
## (Note that bulk_extractor itself no longer requires openssl)
AC_DEFINE([HAVE_STL],1,[We have the Standard Template Library])
AC_CHECK_HEADERS([openssl/x509.h openssl/pem.h])
AC_CHECK_HEADER([afflib/afflib.h],
	    	[AC_DEFINE(HAVE_AFFLIB_AFFLIB_H,1,[Do we have afflib/afflib.h?])]
		[AC_CHECK_LIB([afflib],af_get_pagesize,,
	   AC_MSG_WARN([AFFLIB not found or the version of AFFLIB installed does not support af_get_pagesize; no AFF support]))],
	[])


################################################################
## LIBEWF support
libewf=yes
AC_CHECK_HEADER([libewf.h],
	[AC_DEFINE(HAVE_LIBEWF_H,1,[Do we have libewf.h?])]
	[AC_CHECK_LIB([ewf],[libewf_get_version],,
		[AC_MSG_WARN([libewf not found; no E01 support])]
		[libewf=no])],
	[AC_MSG_WARN([libewf.h not found; no E01 support])]
	[libewf=no])
AC_CHECK_FUNCS([libewf_handle_get_utf8_header_value_notes libewf_error_backtrace_fprint])


################################################################
## hashid support
AC_ARG_ENABLE([hashid],
    [AS_HELP_STRING([--disable-hashid], [disable hashid scanner support])],
    [hashid=no],
    [hashid=yes])

if test x"$hashid" == xyes ; then
  AC_CHECK_LIB([zmq],[zmq_ctx_new],[LIBS="-lzmq $LIBS"],
    [AC_MSG_WARN([Could not find zeromq, zeromq v3 is required to build the hashid scanner])
    hashid=no])

  if test x"$mingw" = x"yes";  then
    # the win32 configuration of libxml2 requires zlib and iconv libraries
    AC_CHECK_LIB([z], [gzdopen],[LIBS="-lz $LIBS"], [AC_MSG_ERROR([Could not find zlib library])])
    AC_CHECK_LIB([iconv], [iconv],[LIBS="-liconv $LIBS"], [AC_MSG_ERROR([Could not find win-iconv library])])
  fi

  AC_CHECK_LIB([xml2], [xmlSAXUserParseFile],[LIBS="-lxml2 $LIBS"],
    [AC_MSG_WARN([Could not find libxml2 library required to build the hashid scanner])
    hashid=no])

  if test x"$hashid" == xyes ; then
    AX_BOOST_BASE([1.53],
      [AX_BOOST_SYSTEM
      AX_BOOST_FILESYSTEM
      AX_BOOST_IOSTREAMS
      LIBS="$BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_IOSTREAMS_LIB $LIBS"],
      [AC_MSG_WARN([Boost 1.53 or newer is required to build the hashid scanner])
      hashid=no])
  fi

  if test x"$hashid" == xyes ; then
    AC_LANG_PUSH(C++)
    AC_CHECK_HEADER([hashdb.hpp], [],
      [AC_MSG_WARN([Could not find hashdb header file required to build the hashid scanner])
      hashid=no])
    AC_CHECK_LIB([hashdb], [hashdb_version], [],
      [AC_MSG_WARN([Could not find hashdb library required to build the hashid scanner])
      hashid=no])
    AC_LANG_POP()
  fi

fi
if test x"$hashid" == xyes ; then
  AC_DEFINE(HAVE_HASHID,1,[define 1 to build the hashid scanner])
else
  AC_MSG_NOTICE([the hashid scanner will not be built])
fi


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
    AC_TRY_COMPILE([#include <exiv2/image.hpp>
    	    	    #include <exiv2/exif.hpp>
                    #include <exiv2/error.hpp>],
      		    [Exiv2::ImageFactory::open(0,0);],
  	  	  exiv2=yes,
  		  exiv2=no)
    if test "${exiv2}" = yes; then
      AC_DEFINE(HAVE_EXIV2,1,[define 1 if EXIV2 exists and works])
      echo Using EXIV2
      if test "${mingw}" = no ; then
        LIBS="$LIBS -lexiv2"
      else
        LIBS="$LIBS -lexiv2.dll"	# static version does not work on mingw
      fi
      dnl see if we have the error feature
      AC_TRY_COMPILE([#include <exiv2/image.hpp>
    	              #include <exiv2/exif.hpp>
                      #include <exiv2/error.hpp>],
      		     [Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);],
  		   [AC_DEFINE(HAVE_EXIV2__LOGMSG__SETLEVEL,1,[define 1 if EXIV2 exists and works])])
    fi
  AC_LANG_POP()    
fi

AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Wshadow"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_SHADOW,1,[define 1 if GCC supports -Wshadow])]
)
AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Wundef"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_UNDEF,1,[define 1 if GCC supports -Wundef])]
)
AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Wcast-qual"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_CAST_QUAL,1,[define 1 if GCC supports -Wcast-qual])]
)
AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Weffcpp"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_EFFCPP,1,[define 1 if GCC supports -Weffc++])]
)
AC_TRY_COMPILE([#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"],[int a=3;],
  [AC_DEFINE(HAVE_DIAGNOSTIC_SUGGEST_ATTRIBUTE,1,[define 1 if GCC supports -Wsuggest-attribute=noreturn])]
)
  
################################################################
# do we have a broken strchr?
# This prevents errors in base64_forensics.cpp
AC_LANG_PUSH(C++)
AC_TRY_COMPILE([#include <string.h>],
               [static const char Base64[] = "ABCDEFGH";int ch=3;char *pos = strchr(Base64, ch);],
               conforming_strchr=yes,
               conforming_strchr=no)
if test "$conforming_strchr" = yes; then
  AC_MSG_NOTICE([strchr is conforming])
  AC_DEFINE(HAVE_CONFORMING_STRCHR, 1,[define to 1 if strchr takes const char*])
else
  AC_MSG_NOTICE([strchr is not conforming])
fi
AC_LANG_POP()

## If we re compiling on mingw, add another -lgdi32 to be safe
## This is required becuase some libraries reference functions in gdi32 and, for whatever reason,
## the linker is not smart enough to re-check the library
AC_MSG_NOTICE([MINGW is $mingw])
if test x"$mingw" = x"yes" ; then
  LIBS="$LIBS -lgdi32"
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

# generate position-independent code for creating the shared library
CFLAGS="$CFLAGS -fPIC"
CXXFLAGS="$CXXFLAGS -fPIC"


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

if test x"${libewf}" != "xyes" ; then
  AC_MSG_WARN([libewf-devel must be installed for E01 support.])
fi  

# if making BEViewer is not disabled then warn about any missing requisite components
if test "x$use_BEViewer" == "xyes"; then
  if test x$has_javac == xfalse -o x$has_jar == xfalse ; then
    AC_MSG_WARN([javac for BEViewer is unavailable because one or more requisite components are not available:])
  fi
  if test "x$has_javac" == "xfalse" ; then
    echo javac Version 6 or later must be installed to allow javac support for BEViewer.
  fi
  if test "x$has_jar" == "xfalse" ; then
    echo jar must be installed to allow javac support for BEViewer.
  fi
else
  AC_MSG_NOTICE([NOTE: BEViewer is disabled.])
fi

AC_OUTPUT

## Create the BEViewer Java config file
echo "// This file is generated by autoconf.  Do not edit this file." > $srcdir/java_gui/src/Config.java
echo "public final class Config {" >> $srcdir/java_gui/src/Config.java
echo "  private Config() { }" // no constructor >> $srcdir/java_gui/src/Config.java
echo "  /** BEViewer Version {@value} */" >> $srcdir/java_gui/src/Config.java
echo "  public static final String VERSION = \"$PACKAGE_VERSION\";" >> $srcdir/java_gui/src/Config.java
echo "}" >> $srcdir/java_gui/src/Config.java

## Finally, record the values of CFLAGS, CPPFLAGS, and CXXFLAGS for DFXML
echo "#define CPPFLAGS \"$CPPFLAGS\"" >> config.h
echo "#define CFLAGS   \"$CFLAGS\""   >> config.h
echo "#define CXXFLAGS \"$CXXFLAGS\"" >> config.h
echo "#define LIBS     \"$LIBS\""     >> config.h
echo "#define LDFLAGS  \"$LDFLAGS\""  >> config.h
if test x"$GIT_COMMIT" != "x" ; then
  echo "#define GIT_COMMIT  \"$GIT_COMMIT\""  >> config.h
fi

