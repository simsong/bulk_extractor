################################################################
#
# Enable all the compiler debugging we can find
# Simson L. Garfinkel
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


# Check G++
# We don't use these warnings:
# -Waggregate-return -- aggregate returns are GOOD; they simplify code design
# We can use these warnings after ZLIB gets upgraded:
# -Wundef  --- causes problems with zlib
# -Wcast-qual
# -Wmissing-format-attribute  --- Just too annoying
AC_LANG_PUSH(C++)
AC_CHECK_HEADERS([string])
CXX_WARNINGS_TO_TEST="-Wall -MD -Wpointer-arith \
    -Wshadow -Wwrite-strings -Wcast-align  \
    -Wredundant-decls -Wdisabled-optimization \
    -Wfloat-equal -Wmultichar -Wmissing-noreturn \
    -Woverloaded-virtual -Wsign-promo \
    -funit-at-a-time"

if test x"${mingw}" != "xyes" ; then
  # add the warnings we don't want to do on mingw
  CXX_WARNINGS_TO_TEST="$CXX_WARNINGS_TO_TEST  -Weffc++"
fi

AC_MSG_NOTICE([C++ Warnings to test: $CXX_WARNINGS_TO_TEST])

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
