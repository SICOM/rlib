## libsrc/Makefile.am for RLIB

## Copyright (c) 2003-2016 SICOM Systems Inc (bdoan@sicompos.com)
## rlib is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.

## rlib is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
## 02111-1307, USA.

## Report bugs to rlib-devel@lists.sourceforge.net
##
## Process this file with automake to produce Makefile.in

if !HAVE_MYSQL_BUILTIN

AM_CFLAGS = $(RLIB_CFLAGS) -I${top_srcdir}/libsrc
mysqlincludedir = $(includedir)/rlib

lib_LTLIBRARIES = libr-mysql.la

libr_mysql_la_SOURCES = mysql.c
libr_mysql_la_LIBADD = $(LIBGLIB_LIBS) $(MYSQL_LIBS) $(top_builddir)/libsrc/libr.la

DEF_FILE=libr-mysql.def

if WIN32
def_ldflags=-Wl,--output-def,$(DEF_FILE)
endif

libr_mysql_la_LDFLAGS = \
	-version-info $(RLIB_LT_CURRENT):$(RLIB_LT_REVISION):$(RLIB_LT_AGE) \
	$(def_ldflags) $(EXTRA_LDFLAGS) -avoid-version

endif
