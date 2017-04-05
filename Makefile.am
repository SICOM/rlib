## Makefile.am for RLIB

## Copyright (c) 2003-2017 SICOM Systems Inc (bdoan@sicompos.com)
## RLIB is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.

## RLIB is distributed in the hope that it will be useful,
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

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = AUTHORS CVS HACKING install-sh README README-JPEG rlib.pc.in  rlib.spec.in

if HAVE_DOC
DOC_DIR=doc
endif


SUBDIRS = rpdf libsrc inputs bindings src tests \
	$(DOC_DIR)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rlib.pc

if HAVE_PHP

phpini_DATA = rlib.ini

PHP_DIRS = $(abs_srcdir)/bindings/php

clean-local:
	$(foreach var,$(PHP_DIRS),$(MAKE) -C $(var) clean ; ) echo ; echo make clean in PHP modules is done ; echo

distclean-local:
	$(foreach var,$(PHP_DIRS),$(MAKE) -C $(var) distclean ; ) echo ; echo make distclean in PHP modules is done ; echo

install-exec-local:
	$(foreach var,$(PHP_DIRS),(cd $(var) && phpize && ./configure LDFLAGS=-L$(DESTDIR)$(libdir)) && ) echo && echo PHP modules configured successfully && echo
	$(foreach var,$(PHP_DIRS),$(MAKE) -C $(var) && ) echo && echo PHP modules are built successfully && echo
	$(foreach var,$(PHP_DIRS),$(MAKE) -C $(var) INSTALL_ROOT=$(DESTDIR) install && ) echo && echo PHP modules are installed successfully && echo

endif
