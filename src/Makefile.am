## bin/Makefile.am for RLIB

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

AM_CFLAGS = $(RLIB_CFLAGS) -I${top_srcdir}/libsrc

if HAVE_MYSQL
EXAMPLES = examples
endif

SUBDIRS = \
	$(EXAMPLES) 

bin_PROGRAMS = rlibc rlib-parse

rlibc_SOURCES = rlibc.c
rlibc_LDADD = $(top_builddir)/libsrc/libr.la $(RLIB_LIBS)

rlib_parse_SOURCES = rlib-parse.c
rlib_parse_LDADD = $(top_builddir)/libsrc/libr.la
