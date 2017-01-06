/*
 *  Copyright (C) 2003-2016 SICOM Systems, INC.
 *
 *  Authors: Bob Doan <bdoan@sicompos.com>
 *  Updated for PHP 7: Zoltán Böszörményi <zboszormenyi@sicom.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#define LE_RLIB_NAME "rlib"

#if PHP_MAJOR_VERSION < 7
typedef int z_str_len_t;
#else
typedef size_t z_str_len_t;
#endif

struct rlib_inout_pass {
	rlib *r;
	int format;
};

typedef struct rlib_inout_pass rlib_inout_pass;
struct environment_filter * rlib_php_new_environment();
gint rlib_add_datasource_php_array(void *r, gchar *input_name);
