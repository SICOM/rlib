## bin/Makefile.am for RLIB

## Copyright (c) 2003-2017 SICOM Systems Inc (bdoan@sicompos.com)
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

EXTRA_DIST = array2.xml array.xml fixed_part.php flow_part.php pages_across.php \
	array.php example.php fixed_part.xml flow_part.xml pages_across.xml graph.php \
	graph.xml odbc.php follower.php follower.xml images.xml images.php red.jpg \
	green.jpg yellow.jpg hungary.xml hungary.php labels.php labels.xml xml.php \
	data.xml expressions.php expressions.xml multiline.php multiline.xml \
	csv.php data.csv csv.xml gantt.php gantt.xml gantt2.php gantt2.xml \
	gantt_colors.php gantt_colors.xml

noinst_SCRIPTS = products.xml
 
products.xml:
	$(LN_S) $(abs_srcdir)/../common/$@

CLEANFILES = $(noinst_SCRIPTS)
