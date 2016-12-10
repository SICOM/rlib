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

if HAVE_PHP
PHP_EXAMPLES=php
endif

if HAVE_PYTHON
PYTHON_EXAMPLES=python
endif

if HAVE_JAVA
JAVA_EXAMPLES=java
endif

if HAVE_PERL
PERL_EXAMPLES=perl
endif

if HAVE_CSHARP
CSHARP_EXAMPLES=csharp
endif

SUBDIRS = \
	common \
	$(PHP_EXAMPLES) \
	$(PYTHON_EXAMPLES) \
	$(JAVA_EXAMPLES) \
	$(PERL_EXAMPLES) \
	$(CSHARP_EXAMPLES) 

EXTRA_DIST = README
