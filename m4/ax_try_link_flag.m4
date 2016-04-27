# ax_try_linker.m4 - Try extra linker option via libtool
#
# Copyright (C) 2015 Sicom Inc.
#
# Written by Zoltán Böszörményi <zboszormenyi@sicom.com>
#
# This file is free software; the Free Software Foundation gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.

# AC_TRY_LINK_FLAG([FLAG,VAR])
# -------------------------
#
# Switch to C language / compiler and try to link a program and
# a shared library with libtool with the passed-in flag applied
# to LDFLAGS. Set the VAR to yes/no. If successful, the flag
# becomes part of LDFLAGS
#
AC_DEFUN([AC_TRY_LINK_FLAG],[
AC_REQUIRE([LT_INIT])
AC_LANG([C])
_LT_LINKER_OPTION([for linker support for $1],[$2],[$1],[],[])
AS_IF([test x$[$2] = xyes],
	[save_LDFLAGS="$LDFLAGS"
	 LDFLAGS="$LDFLAGS $1"
	 AC_MSG_CHECKING([for whether linker flag $1 lets to successfully link a shared library])
	 AC_LANG_CONFTEST([AC_LANG_SOURCE([[int myfunc(void) { return 0; }]])])
	 LIBTOOL_NO_SHELL=`$ECHO $LIBTOOL | $SED -e 's#$(SHELL)##' -e 's#$(top_builddir)#.#'`
	 $LIBTOOL_NO_SHELL --mode=compile $CC -fPIC -fpic -g -O $CPPFLAGS $CFLAGS -c conftest.$ac_ext -o conftest.lo 2>&1 >/dev/null
	 $LIBTOOL_NO_SHELL --mode=link $CC -fPIC -fpic -g -O -shared -module -avoid-version -rpath ${libdir} $CFLAGS $LDFLAGS -o conftest.la conftest.lo 2>&1 >/dev/null
	 link_successful="$?"
	 AS_IF([test x$link_successful = x0],
	 	[AC_MSG_RESULT([yes])],
	 	[AC_MSG_RESULT([no])
	 	 LDFLAGS="$save_LDFLAGS"
	 	])
	 $LIBTOOL_NO_SHELL --mode=clean $RM conftest.la conftest.lo 2>&1 >/dev/null
	],
	[])

])# AC_TRY_LINK_FLAG
