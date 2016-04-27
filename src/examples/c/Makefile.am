## src/examples/c/Makefile.am for RLIB

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

noinst_PROGRAMS = example relpth1 relpth2 srcpth relprt

AM_CFLAGS = $(RLIB_CFLAGS) -I${top_srcdir}/libsrc

example_LDADD = $(top_builddir)/libsrc/libr.la

relpth1_LDADD = $(top_builddir)/libsrc/libr.la

relpth2_LDADD = $(top_builddir)/libsrc/libr.la

srcpth_LDADD = $(top_builddir)/libsrc/libr.la

relprt_LDADD = $(top_builddir)/libsrc/libr.la

noinst_SCRIPTS = products.xml

products.xml:
	$(LN_S) ../common/products.xml

CLEANFILES = $(noinst_SCRIPTS)
