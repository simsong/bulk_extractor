AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([ #pragma GCC diagnostic ignored "-Wshadow" ],[ return 0; ])],
  [AC_DEFINE(HAVE_DIAGNOSTIC_SHADOW,1,[define 1 if GCC supports -Wshadow])],[])

AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([ #pragma GCC diagnostic ignored "-Wundef"],[return 0;])],
  [AC_DEFINE(HAVE_DIAGNOSTIC_UNDEF,1,
        [define 1 if GCC supports -Wundef])], [])

AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([#pragma GCC diagnostic ignored "-Wcast-qual"],[return 0;])],
  [AC_DEFINE(HAVE_DIAGNOSTIC_CAST_QUAL,1,
        [define 1 if GCC supports -Wcast-qual ])], [])

AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([#pragma GCC diagnostic ignored "-Weffcpp"],[return 0;])],
  [AC_DEFINE(HAVE_DIAGNOSTIC_EFFCPP,1,
        [define 1 if GCC supports -Weffc++ ])], [])

AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"],[return 0;])],
  [AC_DEFINE(HAVE_DIAGNOSTIC_SUGGEST_ATTRIBUTE,1,
        [define 1 if GCC supports -Wsuggest-attribute=noreturn])], [])

AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([#pragma GCC diagnostic ignored "-Wdeprecated-register"],[return 0;])],
  [AC_DEFINE(HAVE_DIAGNOSTIC_DEPRECATED_REGISTER,1,
        [define 1 if GCC supports -Wdeprecated-register])], [])
