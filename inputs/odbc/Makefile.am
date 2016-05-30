## libsrc/Makefile.am for RLIB

## Copyright (c) 2003-2006 SICOM Systems Inc (bdoan@sicompos.com)
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

if !HAVE_ODBC_BUILTIN

AM_CFLAGS = $(LIBGLIB_CFLAGS) $(RLIB_CFLAGS) $(RLIB_WERROR) -I${top_srcdir}/libsrc
odbcincludedir=$(includedir)/rlib
lib_LTLIBRARIES = libr-odbc.la
libr_odbc_la_SOURCES = odbc.c
libr_odbc_la_LIBADD = $(LIBGLIB_LIBS) $(ODBC_LIBS) $(top_builddir)/libsrc/libr.la
DEF_FILE=libr-odbc.def
if WIN32
def_ldflags=-Wl,--output-def,$(DEF_FILE)
endif
libr_odbc_la_LDFLAGS = \
	-version-info $(RLIB_LT_CURRENT):$(RLIB_LT_REVISION):$(RLIB_LT_AGE) \
	$(def_ldflags) $(EXTRA_LDFLAGS) -avoid-version

endif
