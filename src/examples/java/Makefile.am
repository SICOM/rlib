## bin/Makefile.am for RLIB

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

JAVAROOT=.
javadir = .

javafiles = rlib.java rlibJNI.java \
	SWIGTYPE_p_f_p_rlib_p_void__int.java \
	SWIGTYPE_p_rlib.java \
	SWIGTYPE_p_void.java \
	SWIGTYPE_p_float.java \
	SWIGTYPE_p_gchar.java \
	SWIGTYPE_p_rlib_report.java \
	SWIGTYPE_p_rlib_part.java

java_JAVA = $(javafiles) SWIGTYPE_p_rlib.java rlib.java rlibJNI.java example.java

noinst_SCRIPTS = products.xml

products.xml:
	$(LN_S) ../common/products.xml

$(javafiles):
	$(LN_S) ../../../bindings/interfaces/$@

CLEANFILES = *.class $(noinst_SCRIPTS) $(javafiles)
