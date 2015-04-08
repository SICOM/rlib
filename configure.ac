dnl Configure template for RLIB

dnl Copyright (c) 2003-2006 SICOM Systems Inc (bdoan@sicompos.com)
dnl RLIB is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.

dnl RLIB is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.

dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
dnl 02111-1307, USA.

dnl Report bugs to rlib-devel@lists.sourceforge.net
dnl
dnl Process this file with autoconf to produce a configure script.

AC_INIT([rlib],[2.0.6],[http://bugzilla.sicompos.com])
AC_CANONICAL_TARGET
AC_CONFIG_SRCDIR(libsrc/rlib.h)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE()
AC_CONFIG_HEADERS(config.h)
AM_MAINTAINER_MODE

RLIB_VERSION=2.0.6
AC_SUBST(RLIB_VERSION)

RLIB_LT_CURRENT=4
RLIB_LT_AGE=3
RLIB_LT_REVISION=8
AC_SUBST(RLIB_LT_CURRENT)
AC_SUBST(RLIB_LT_REVISION)
AC_SUBST(RLIB_LT_AGE)

dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S
AM_PROG_AR
LT_INIT([dlopen disable-static win32-dll])

dnl AC_LIBLTDL_CONVENIENCE
AC_SUBST(INCLTDL)
AC_SUBST(LIBLTDL)

AC_CHECK_HEADERS(langinfo.h sys/time.h)

AC_CHECK_FUNCS([localtime_r strfmon])

dnl Checks for libraries.
AC_CHECK_LIB(c, exit)
AC_CHECK_LIB(m, exp)
AC_CHECK_HEADER([zlib.h],
	[AC_CHECK_LIB(z, zlibVersion,
		enable_compressed_pdf=yes,
		enable_compressed_pdf=no)],
	[enable_compressed_pdf=no])
if test x$enable_compressed_pdf = xyes; then
	AC_DEFINE([RPDF_COMPRESS_STREAM], [], [RPDF can compress streams])
	ZLIB_LIBS="-lz"
fi
AC_SUBST(ZLIB_LIBS)

PKG_CHECK_MODULES(LIBXML, libxml-2.0 >= 2.6.0)
AC_SUBST(LIBXML_LIBS)
AC_SUBST(LIBXML_CFLAGS)

PKG_CHECK_MODULES(LIBGLIB, glib-2.0 >= 0.0.1)
AC_SUBST(LIBGLIB_LIBS)
AC_SUBST(LIBGLIB_CFLAGS)

PKG_CHECK_MODULES(LIBGMODULE, gmodule-2.0 >= 0.0.1)
AC_SUBST(LIBGMODULE_LIBS)
AC_SUBST(LIBGMODULE_CFLAGS)

case "$target" in
  *-*-cygwin*|*-*-mingw*)
	ac_save_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -mms-bitfields -c conftest.$ac_ext"
	AC_MSG_CHECKING([whether $CC supports -mms-bitfields])
	echo "int some_variable = 0;" > conftest.$ac_ext
	compiler_ms_bitfields=no
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([[]], [[int some_variable = 0;]])], [dnl
		# The compiler can only warn and ignore the option if not recognized
		# So say no if there are warnings
		if test -s conftest.err; then
			compiler_ms_bitfields=no
		else
			compiler_ms_bitfields=yes
		fi
	])
	AC_MSG_RESULT($compiler_ms_bitfields)
	CFLAGS="$ac_save_CFLAGS"

	if test "$compiler_ms_bitfields" = "no"; then
		ac_save_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS -fnative-struct -c conftest.$ac_ext"
		AC_MSG_CHECKING([whether $CC supports -fnative-struct])
		echo "int some_variable = 0;" > conftest.$ac_ext
		compiler_native_struct=no
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[int some_variable = 0;]])], [dnl
			# The compiler can only warn and ignore the option if not recognized
			# So say no if there are warnings
			if test -s conftest.err; then
				compiler_native_struct=no
			else
				compiler_native_struct=yes
			fi
		])
		AC_MSG_RESULT($compiler_native_struct)
		CFLAGS="$ac_save_CFLAGS"
	fi
	if test $compiler_ms_bitfields=yes; then
		CFLAGS="$CFLAGS -mms-bitfields"
	else
		if test $compiler_native_struct=yes; then
			CFLAGS="$CFLAGS -fnative_struct"
		fi
	fi

dnl Enable WIN32 on MingW or Cygwin in mno-cygwin mode
	case "$target" in
		*-*-mingw32*)
			enable_win32=yes
		;;
		*-*-cygwin*)
			AC_EGREP_CPP(pattern,
			[#ifdef WIN32
			yes
			#endif],
			enable_win32=yes, enable_win32=no)
		;;
	esac
	EXTRA_LDFLAGS="-no-undefined"
	;;
  *)
	enable_win32=no
	EXTRA_LDFLAGS=
	;;
esac
AM_CONDITIONAL(WIN32, test x$enable_win32 = "xyes")
AC_SUBST(EXTRA_LDFLAGS)


AC_ARG_ENABLE([mysql],
	[AS_HELP_STRING([--disable-mysql],[disable mysql backend])],,enable_mysql=yes)
if test x$enable_mysql = xyes; then
	dnl Use the standard MySQL detection from mysql.m4
	dnl MYSQL_CLIENT sets MYSQL_CLIENT_CFLAGS and MYSQL_CLIENT_LIBS.
	dnl Lets set out own MYSQL_CFLAGS and MYSQL_LIBS too.
	dnl It will make ./configure bail with an error if detection fails
	dnl instead of silently ignoring it.
	MYSQL_CLIENT()
	MYSQL_CFLAGS=${MYSQL_CLIENT_CFLAGS}
	AC_SUBST(MYSQL_CFLAGS)
	MYSQL_LIBS=${MYSQL_CLIENT_LIBS}
	AC_SUBST(MYSQL_LIBS)
fi
AM_CONDITIONAL(HAVE_MYSQL, test x$enable_mysql = xyes)

AC_ARG_ENABLE([postgres],
	[AS_HELP_STRING([--disable-postgres],[disable postgres backend])],,enable_postgres=yes)

POSTGRES_CFLAGS=""
POSTGRES_LIBS=""
case "$target" in
*-*-cygwin*|*-*-mingw*)
        PG_CONFIG=pg_config.exe
        ;;
*)
        PG_CONFIG=pg_config
        ;;
esac

AC_ARG_WITH([pg-config],
	[AS_HELP_STRING([--with-pg-config=PATH],[A PATH to pg_config script])],,
	[AC_PATH_PROG(POSTGRES_CONFIG, $PG_CONFIG, no)
	 if test "x$POSTGRES_CONFIG" = "xno" ; then
		with_pg_config=no
	 else
		with_pg_config=$POSTGRES_CONFIG
	 fi])

if test x$enable_postgres = xyes; then
	if test "x$with_pg_config" = "xno" ; then
		AC_MSG_ERROR([--without-pg-config were specified or pg_config is not found in PATH when --enable-postgres is specified])
	fi
	AC_DEFINE(HAVE_POSTGRES,1,[Build with POSTGRES support])
	POSTGRES_CFLAGS="-I `"$POSTGRES_CONFIG" --includedir`"
	POSTGRES_LIBS="-L`"$POSTGRES_CONFIG" --libdir` -lpq"
fi
AC_SUBST(POSTGRES_CFLAGS)
AC_SUBST(POSTGRES_LIBS)
AM_CONDITIONAL(HAVE_POSTGRES, test x$enable_postgres = xyes)

dnl .. ODBC checking borrowed from libgda
ODBC_CFLAGS=""
ODBC_LIBS=""
enable_odbc=yes
AC_ARG_WITH(odbc,
[  --with-odbc=<directory>	use ODBC libraries in <directory>],[
if test $withval = no
then
	enable_odbc=false
elif test $withval = yes
then
	dir="/usr/local"
else
	dir=$withval
fi
])
odbcdir=""
if test $enable_odbc = yes
then
    if test x$enable_win32 = xyes ; then
	AC_MSG_CHECKING([whether <sql.h> needs  <windows.h>])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sql.h>]], [[]])], sql_h_needs_windows_h=no,
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <sql.h>]], [[]])], sql_h_needs_windows_h=yes,
			AC_MSG_ERROR([don't know how to compile <sql.h>])
		))
	AC_MSG_RESULT([$sql_h_needs_windows_h])

	ac_save_LIBS="$LIBS"
	LIBS="$LIBS -lodbc32"

	AC_MSG_CHECKING([for SQLAllocHandle in -lodbc])
	if test $sql_h_needs_windows_h = yes; then
		AC_DEFINE([NEED_WINDOWS_H], [1], [We are compiling under Windows])
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <sql.h>]],
		[[SQLHENV hEnv;
SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);]])],
		[have_odbc32=yes],
		[AC_MSG_ERROR([Can't find ODBC libs])])
	else
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sql.h>]],
		[[SQLHENV hEnv;
SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);]])],
		[have_odbc32=yes],
		[AC_MSG_ERROR([Can't find ODBC libs])])
	fi

	AC_MSG_RESULT($have_odbc32)
	if test $have_odbc32=yes; then
		AC_DEFINE(HAVE_ODBC,1,[Build with ODBC support])
		ODBC_CFLAGS=""
		ODBC_LIBS="-lodbc32"
	else
		enable_odbc=no
		AC_MSG_WARN(ODBC input method is not going to be used)
	fi
        LIBS="$ac_save_LIBS"
    else
	AC_MSG_CHECKING(for ODBC files)
	for d in $dir $libdir /usr /usr/local
	do
		if test -f $d/libodbc.so -a -f $d/../include/sql.h
		then
			AC_MSG_RESULT(found ODBC in $d)
			odbclib="-lodbc"
			odbcdir=$d
			break
		fi
		if test -f $d/lib/libodbc.so -a -f $d/include/sql.h
		then
			AC_MSG_RESULT(found ODBC in $d)
			odbclib="-lodbc"
			odbcdir=$d
			break
		fi
		if test -f $d/libiodbc.so -a -f $d/../include/sql.h
		then
			AC_MSG_RESULT(found iODBC in $d)
			odbclib="-liodbc"
			odbcdir=$d
			break
		fi
		if test -f $d/lib/libiodbc.so -a -f $d/include/sql.h
		then
			AC_MSG_RESULT(found iODBC in $d)
			odbclib="-liodbc"
			odbcdir=$d
			break
		fi
	done
	if test x$odbcdir = x
	then
		enable_odbc=no
		AC_MSG_WARN(ODBC input method is not going to be used)
	else
		AC_DEFINE(HAVE_ODBC,1,[Build with ODBC support])
		ODBC_CFLAGS="-I${odbcdir}/include"
		ODBC_LIBS="-L${odbcdir}/lib ${odbclib}"
	fi
    fi
fi
AM_CONDITIONAL(HAVE_ODBC, test x$enable_odbc = xyes)
AC_SUBST(ODBC_CFLAGS)
AC_SUBST(ODBC_LIBS)


AC_ARG_ENABLE(php,              [  --disable-php         disable php frontend],,enable_php=yes)
PHP_CFLAGS=""
PHP_EXTENSION_DIR=""
if test x$enable_php = xyes; then
	AC_PATH_PROG(PHP_CONFIG, php-config, no)
	if test x$PHP_CONFIG = xno; then
		enable_php=no
		echo "*** WARNING: PHP NOT FOUND IN PATH.  RLIB WILL NOT BE COMPILED WITH SUPPORT FOR IT"
	else
		AC_DEFINE(HAVE_PHP,1,[Build with PHP support])
		PHP_CFLAGS=`$PHP_CONFIG --includes`
		PHP_EXTENSION_DIR=`$PHP_CONFIG --extension-dir`
	fi
fi
AC_SUBST(PHP_CFLAGS)
AC_SUBST(PHP_EXTENSION_DIR)
AC_SUBST(HAVE_PHP)
AM_CONDITIONAL(HAVE_PHP, test x$enable_php = xyes)

AC_ARG_ENABLE(gd,              [  --disable-gd         disable gd graphing],,enable_gd=yes)
GD_CFLAGS=""
GD_LIBS=""
GD_LINK=""
GD_LDFLAGS=""
if test x$enable_gd = xyes; then
	AC_PATH_PROG(GD_CONFIG, gdlib-config, no)
	if test "x$GD_CONFIG" = "xno" ; then
		enable_gd=no
		echo "*** WARNING: GD NOT FOUND IN PATH.  RLIB WILL NOT BE COMPILED WITH SUPPORT FOR IT"
	else
		AC_DEFINE(HAVE_GD,1,[Build with GD support])
		GD_CFLAGS=`"$GD_CONFIG" --includes`
		GD_LIBS=`"$GD_CONFIG" --ldflags`
		GD_LINK="-lgd"
		GD_LDFLAGS=`"$GD_CONFIG" --libs`
	fi
fi
AC_SUBST(GD_CFLAGS)
AC_SUBST(GD_LIBS)
AC_SUBST(GD_LINK)
AC_SUBST(GD_LDFLAGS)
AC_SUBST(HAVE_GD)
AM_CONDITIONAL(HAVE_GD, test x$enable_gd = xyes)

AC_ARG_ENABLE(doc,              [  --disable-doc         disable docs],,enable_doc=yes)
if test x$enable_doc = xyes; then
	AC_PATH_PROG(DOC_CONFIG, db2pdf, no)
	if test x$DOC_CONFIG = xno; then
		enable_doc=no
		echo "*** WARNING: DB2PDF NOT FOUND IN PATH.  Sadly the manual won't be built"
	fi
fi
AM_CONDITIONAL(HAVE_DOC, test x$enable_doc = xyes)

AC_ARG_ENABLE([python],
	[AS_HELP_STRING([--disable-python],[disable python binding])],,enable_python=yes)
PYTHON_CFLAGS=""
if test x$enable_python = xyes; then
	AM_PATH_PYTHON([2.2])
	eval python_inc="`${PYTHON} -c "from distutils import sysconfig; print sysconfig.get_python_inc(0)" | tr '\\\\' '/' `"
	eval python_lib="`${PYTHON} -c "from distutils import sysconfig; print sysconfig.get_config_var('BINLIBDEST')" | tr '\\\\' '/' `"
	AC_CHECK_HEADERS(python${PYTHON_VERSION}/Python.h,python_header=yes,python_header=no)
	if test x$python_header = xyes; then
		AC_DEFINE(HAVE_PYTHON,1,[Build with PYTHON support])
		PYTHON_CFLAGS="-I $python_inc"
		PYTHON_LIBRARIES="-L$python_lib -lpython$PYTHON_VERSION"
	else
		enable_python=no
		echo "*** WARNING: PYTHON was found but Python.h was not!   No python bindings for you.  Perhaps you need to install the devel package or something"
	fi
fi
AC_SUBST(HAVE_PYTHON)
AC_SUBST(PYTHON_CFLAGS)
AC_SUBST(PYTHON_LIBRARIES)
AM_CONDITIONAL(HAVE_PYTHON, test x$enable_python = xyes)

enable_java=no
JAVA_CFLAGS=""
AC_ARG_WITH(java,
[  --with-java=<directory>	use JAVA in <directory>],[
if test "$withval" = "no"
then
	enable_java=false
elif test "$withval" = yes
then
	dir="/usr/local"
else
	dir="$withval"
	enable_java=yes
fi
])
javadir=""
if test $enable_java = yes
then
	AC_MSG_CHECKING(for JAVA files)
	for d in "$dir" /usr /usr/local
	do
		if test -f "$d/include/jni.h"
		then
			AC_MSG_RESULT(found JAVA in $d)
			javadir=$d
			break
		fi
	done
	if test "x$javadir" = "x"
	then
		enable_java=no
		AC_MSG_WARN(JAVA bindings are not going to be used)
	else
		AC_DEFINE(HAVE_JAVA,1,[Build with JAVA support])
		case "$target" in
                *-*-cygwin*|*-*-mingw*)
			JAVA_CFLAGS="-I\"${javadir}/include\" -I\"${javadir}/include/win32\""
			;;
		*)
			JAVA_CFLAGS="-I\"${javadir}/include\" -I\"${javadir}/include/linux\""
			;;
		esac
	fi
fi
AM_CONDITIONAL(HAVE_JAVA, test x$enable_java = xyes)
AC_SUBST(JAVA_CFLAGS)

enable_csharp=no
CSHARP_CFLAGS=""
AC_ARG_WITH(csharp,
[  --with-csharp=<directory>	use CSHARP in <directory>],[
if test "$withval" = "no"
then
	enable_csharp=false
elif test "$withval" = "yes"
then
	dir="/usr/local"
else
	dir="$withval"
	enable_csharp=yes
fi
])
csharpdir=""
if test $enable_csharp = yes
then
	AC_MSG_CHECKING(for C SHARP files)
	for d in "$dir" /usr /usr/local
	do
		if test -f "$d/bin/mcs"
		then
			AC_MSG_RESULT(found C SHARP in "$d")
			csharpdir="$d"
			break
		fi
	done
	if test "x$csharpdir" = "x"
	then
		enable_csharp=no
		AC_MSG_WARN(C SHARP bindings are not going to be used)
	else
		AC_DEFINE(HAVE_CSHARP,1,[Build with CSHARP support])
		CSHARP_CFLAGS=""
	fi
fi
AM_CONDITIONAL(HAVE_CSHARP, test x$enable_csharp = xyes)
AC_SUBST(CSHARP_CFLAGS)


perl_where=""
PERL_CFLAGS=""
AC_ARG_ENABLE(perl,              [  --disable-perl         disable perl],,enable_perl=yes)
if test x$enable_perl = xyes; then
	AC_PATH_PROG(PERL_CONFIG, perl no)
	if test x$PERL_CONFIG = xno; then
		enable_perl=no
		echo "*** WARNING: PERL NOT FOUND IN PATH."
	else
		perl_where="`$PERL_CONFIG -e 'use Config; print $Config{archlib};' | tr '\\\\' '/'`"
		PERL_CFLAGS="-I ${perl_where}/CORE"
		PERL_EXTENSION_DIR="${perl_where}"
		PERL_LIBS="-L${perl_where}/CORE -lperl"
	fi
fi
AM_CONDITIONAL(HAVE_PERL, test x$enable_perl = xyes)
AC_SUBST(PERL_CFLAGS)
AC_SUBST(PERL_EXTENSION_DIR)
AC_SUBST(PERL_LIBS)


AC_MSG_CHECKING(whether iconv takes ISO8859-1 or ISO-8859-1)
AC_RUN_IFELSE(
        [AC_LANG_PROGRAM([[#include <iconv.h>]], [[	
                iconv_open("ISO-8859-1","UTF-8");
                if (errno)
                        return -1;

                return 0;
        ]])],
        AC_MSG_RESULT(ISO-8859-1)
        AC_DEFINE(ICONV_ISO,["ISO-8859-1"], [ICONV Check]),
        AC_MSG_RESULT(ISO8859-1)
        AC_DEFINE(ICONV_ISO,["ISO8859-1"], [ICONV Check]),
        dnl Cross-compiling: Assume a sensible MinGW behaviour.
        dnl On Fedora 16's mingw32 suite, mingw32-win-iconv accepts both.
        AC_MSG_RESULT(ISO-8859-1)
        AC_DEFINE(ICONV_ISO,["ISO-8859-1"], [ICONV Check])
)

RLIB_WERROR=""
AC_ARG_ENABLE(warningfun,              [  --enable-warningfun       enable more gcc warning flags],enable_warningfun="$enableval",enable_warningfun=no)
if test x$enable_warningfun = xyes; then
	RLIB_WERROR="-Werror "
fi
AC_SUBST(RLIB_WARNINGS)
AC_SUBST(RLIB_WERROR)

dnl Windows build is decided automatically by AC_CANONICAL_TARGET above
if test x$enable_win32 = xyes; then
	ac_save_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -mno-cygwin -c conftest.$ac_ext"
	AC_MSG_CHECKING([whether $CC supports -mno-cygwin])
	echo "int some_variable = 0;" > conftest.$ac_ext   
	compiler_mnocygwin=no
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[int some_variable = 0;]])], [dnl
		# The compiler can only warn and ignore the option if not recognized
		# So say no if there are warnings
		if test -s conftest.err; then
			compiler_mnocygwin=no
		else
			compiler_mnocygwin=yes
		fi
	])
	AC_MSG_RESULT($compiler_mnocygwin)
	CFLAGS="$ac_save_CFLAGS"

	if test "$compiler_mnocygwin" = "yes"
	then
		NO_CYGWIN="-mno-cygwin"
	fi
fi
AC_SUBST(NO_CYGWIN)

AC_CHECK_HEADER([monetary.h],
	[AC_DEFINE([RLIB_HAVE_MONETARY_H], [], [Money])], [])

dnl -D_GNU_SOURCE is needed.
dnl For llabs(), -D_ISOC99_SOURCE is enough,
dnl but e.g. strcasecmp is a GNU extension.

RLIB_CFLAGS="-D_GNU_SOURCE -Wall -Wchar-subscripts $LIBXML_CFLAGS $LIBGLIB_CFLAGS $LIBGMODULE_CFLAGS $MYSQL_CFLAGS $POSTGRES_CFLAGS $ODBC_CFLAGS $GD_CFLAGS $NO_CYGWIN"
RLIB_LIBS="$LIBXML_LIBS $LIBGLIB_LIBS $LIBGMODULE_LIBS $ICONVLIBS $GD_LDFLAGS $GD_LIBS $GD_LINK "

AC_SUBST(RLIB_CFLAGS)
AC_SUBST(RLIB_LIBS)

RPDF_CFLAGS="-D_GNU_SOURCE $LIBGLIB_CFLAGS $LIBGMODULE_CFLAGS $NO_CYGWIN"
RPDF_LIBS="$LIBGLIB_LIBS $LIBGMODULE_LIBS $ZLIB_LIBS"
AC_SUBST(RPDF_CFLAGS)
AC_SUBST(RPDF_LIBS)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h unistd.h)
AC_C_CONST

AC_ARG_ENABLE(memdebug,              [  --enable-memdebug       enable memory debugging],enable_memdebug="$enableval",enable_memdebug=no)
if test x$enable_memdebug = xyes; then
	AC_DEFINE(DEBUG_RLIB_MEMORY,1,[Debug RLIB memory usage])
fi

AC_ARG_ENABLE(crash,              [  --disable-crash       disable crash detection],enable_memdebug="$enableval",enable_crash=yes)
if test x$enable_crash = xyes; then
    AC_CHECK_HEADERS(sys/resource.h,
	[AC_DEFINE(HAVE_SYS_RESOURCE_H, 1, [We have standard sys/resource.h])
	 have_sys_res_h=yes], have_sys_res_h=no)
    if test x$have_sys_res_h = xyes; then
	AC_DEFINE(ENABLE_CRASH,1,[ENABLE CRASH DETECTION])
    else
	AC_MSG_WARN(Crash detection disabled because of missing sys/resource.h.)
    fi
fi

AC_ARG_ENABLE(utf8,              [  --enable-utf8       enable utf8],enable_utf8=no,enable_utf8=yes)
if test x$enable_utf8 = xyes; then
	AC_DEFINE(DISABLE_UTF8,1,[DISABLE UTF8])
fi

AC_CONFIG_FILES([
Makefile
rlib.pc
rpdf/Makefile
libsrc/Makefile
src/Makefile
src/examples/Makefile
src/examples/common/Makefile
src/examples/c/Makefile
src/examples/php/Makefile
src/examples/python/Makefile
src/examples/java/Makefile
src/examples/perl/Makefile
src/examples/csharp/Makefile
bindings/Makefile
bindings/php/Makefile
bindings/python/Makefile
bindings/perl/Makefile
bindings/java/Makefile
bindings/csharp/Makefile
bindings/interfaces/Makefile
inputs/Makefile
inputs/mysql/Makefile
inputs/odbc/Makefile
inputs/postgres/Makefile
tests/Makefile
doc/Makefile
doc/IMAGES/Makefile
rlib.spec
])

AC_OUTPUT

echo "
rlib-$VERSION:

   prefix:                   ${prefix}
   compiler:                 ${CC}
   Memory debugging:         ${enable_memdebug}
   Warning Fun:              ${enable_warningfun}
   Crash detection:          ${enable_crash}
   UTF8 disabled:            ${enable_utf8}
   WIN32 build:              ${enable_win32}
   Documentation:            ${enable_doc}

   INPUTS:
    MYSQL:                   ${enable_mysql}
    POSTGRES:                ${enable_postgres}
    ODBC:                    ${enable_odbc}

   COMPRESSION:
    PDF:                     ${enable_compressed_pdf}

   GRAPHING:
    PDF:                     Always
    HTML:                    ${enable_gd}
	 
   BINDINGS:
    PHP:                     ${enable_php}
     PHP entension dir:      ${PHP_EXTENSION_DIR}
    PYTHON:                  ${enable_python}
     PYTHON version:         ${PYTHON_VERSION}
     PYTHON script dir:      ${pythondir}
     PYTHON entension dir:   ${pyexecdir}
     PYTHON module dir:      ${pkgpyexecdir}
    JAVA:                    ${enable_java}
    CSHARP:                  ${enable_csharp}
    PERL:                    ${enable_perl}
     PERL entension dir:     ${perl_where}

   ** If you like RLIB send a postcard from where you are in the world to:
   ** Bob Doan - 4140 Skyron Drive - Doylestown - PA - 18902 - USA

   Music:                    RHCP / Virgin Radio	
   Having Problems:          rlib-users@lists.sf.net 	
   Want To Contribute:       rlib-devel@lists.sf.net
   Commercial License:       rlib-sales@sicom.com
	
"
