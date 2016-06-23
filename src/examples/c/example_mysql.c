/*
 *  Copyright (C) 2003-2016 SICOM Systems, INC.
 *
 *  Authors: Bob Doan <bdoan@sicompos.com>
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

#include <config.h>

#include <stdio.h>
#include <rlib.h>
#include <rlib_input.h>

int main(void) {
	const char *hostname, *username, *password, *database;
	rlib *r;

	hostname = "localhost";
	username = "rlib";
	password = "rlib";
	database = "rlib";

	r = rlib_init();
	if (rlib_add_datasource_mysql(r, "local_mysql", hostname, username, password, database) < 0) {
		rlib_free(r);
		return 1;
	}
	rlib_add_query_as(r, "local_mysql", "select * FROM products", "products");
	rlib_add_report(r, "products.xml");
	rlib_set_output_format(r, RLIB_FORMAT_PDF);
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);
	return 0;
}
