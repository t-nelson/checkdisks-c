AC_INIT([checkdisks], [0.0.1], [@t-nelson on GitHub])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])

# Work around stupid autoconf default cflags. pt 1
SAVE_CFLAGS="x${CFLAGS}"

AC_PROG_CC
AC_PROG_RANLIB

AM_PROG_CC_C_O

# Work around stupid autoconf default cflags. pt 2
if test "$SAVE_CFLAGS" = "x"; then
  CFLAGS=""
fi

AC_ARG_ENABLE([debug],
              [AC_HELP_STRING([--disable-debug],
                              [Disable debugging facilities])],
              [use_debug=$enableval],
              [use_debug=yes])

if test x$use_debug = xyes; then
  DBG_CFLAGS="-g -Wall -Werror"
else
  RLS_CFLAGS=-O2
  RLS_CPPFLAGS=-DNDEBUG
fi

AC_CONFIG_FILES([Makefile]
                [src/Makefile]
               )

AM_CPPFLAGS=-D_GNU_SOURCE

AC_SUBST(DBG_CFLAGS)
AC_SUBST(RLS_CFLAGS)
AC_SUBST(RLS_CPPFLAGS)
AC_SUBST(AM_CPPFLAGS)

AC_CHECK_LIB([pci],   [main],, AC_MSG_ERROR([Missing a required library!]))
AC_CHECK_LIB([m],     [main],, AC_MSG_ERROR([Missing a required library!]))

PKG_CHECK_MODULES([DBUS], [dbus-1])

AC_OUTPUT
