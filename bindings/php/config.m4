dnl config.m4 for extension rlib

PHP_ARG_ENABLE(rlib, whether to enable rlib module,
[  --enable-rlib             Enable rlib module])

if test "$PHP_RLIB" != "no"; then
	AC_MSG_CHECKING(for pkg-config)
	if test ! -f "$PKG_CONFIG"; then
		PKG_CONFIG=`which pkg-config`
	fi
	if test ! -f "$PKG_CONFIG"; then
		AC_MSG_RESULT(not found)
		AC_MSG_ERROR(pkg-config is not found)
	fi
	AC_MSG_RESULT(found)

	AC_MSG_CHECKING(for glib-2.0)
	if $PKG_CONFIG --exists glib-2.0; then
		AC_MSG_RESULT(found)
	else
		AC_MSG_RESULT(not found)
		AC_MSG_ERROR(glib-2.0 is not found)
	fi
	GLIB_CFLAGS="`$PKG_CONFIG --cflags glib-2.0`"
	GLIB_LIBS="`$PKG_CONFIG --libs glib-2.0`"

	dnl
	dnl These below must be built before they can be used
	dnl
	LIBRLIB_CFLAGS="-I../../libsrc"
	# We don't want errors telling:
	# /usr/bin/grep: /usr/lib64/librpdf.la: No such file or directory
	LIBRLIB_LIBS="-L../../rpdf/.libs -L../../libsrc/.libs -lr"

	CFLAGS="-Wall -Werror $LIBRLIB_CFLAGS $CFLAGS $GLIB_CFLAGS"
	LDFLAGS="$LIBRLIB_LIBS $LDFLAGS $GLIB_LIBS"

	PHP_NEW_EXTENSION(rlib, [array_data_source.c environment.c php.c], $ext_shared)
	PHP_EVAL_INCLINE($CFLAGS)
	PHP_EVAL_LIBLINE($LDFLAGS, RLIB_SHARED_LIBADD)
fi
