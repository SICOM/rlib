## bindings/python/Makefile.am for RLIB

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

AM_CFLAGS = \
	-I${top_srcdir}/libsrc \
	$(LIBGLIB_CFLAGS) $(LIBGMODULE_CFLAGS) \
	$(PYTHON_CFLAGS) -fno-strict-aliasing

pkgpyexec_LTLIBRARIES = rlibmodule.la _rlib.la

rlibmodule_la_SOURCES = rlibmodule.c 

rlibmodule_la_LIBADD = $(top_builddir)/libsrc/libr.la $(PYTHON_LIBRARIES) $(RLIB_LIBS)

if WIN32
NDEF_FILE=rlibmodule.def
ndef_ldflags=-Wl,--output-def,$(NDEF_FILE)
endif

rlibmodule_la_LDFLAGS = \
	-version-info $(RLIB_LT_CURRENT):$(RLIB_LT_REVISION):$(RLIB_LT_AGE) \
	$(ndef_ldflags) $(EXTRA_LDFLAGS) -module -avoid-version

_rlib_la_SOURCES = python.c 

_rlib_la_LIBADD = $(top_builddir)/libsrc/libr.la $(PYTHON_LIBRARIES) $(RLIB_LIBS)

if WIN32
DEF_FILE=rlibpython.def
def_ldflags=-Wl,--output-def,$(DEF_FILE)
endif

_rlib_la_LDFLAGS = \
	-version-info $(RLIB_LT_CURRENT):$(RLIB_LT_REVISION):$(RLIB_LT_AGE) \
	$(def_ldflags) $(EXTRA_LDFLAGS) -module -avoid-version
