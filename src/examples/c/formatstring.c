/*
 *  Copyright (C) 2016 SICOM Systems, INC.
 *
 *  Author: Zoltán Böszörményi <zboszormenyi@sicom.com>
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

char *data[2][2] = {
	[0][0] = "date",
	[0][1] = "number",

	[1][0] = "2016-01-01 10:27:37",
	[1][1] = "100.0"
};

int main(int argc, char **argv) {
	rlib *r;

	if (argc == 1) {
		printf("usage: %s [ pdf | xml | txt | csv | html ]\n", argv[0]);
		return 1;
	}

	r = rlib_init();
	rlib_add_datasource_array(r, "local_array");
	rlib_add_query_array_as(r, "local_array", data, 2, 2, "data");
	rlib_add_report(r, "formatstring.xml");
	rlib_set_output_format_from_text(r, argv[1]);
	rlib_set_output_parameter(r, "debugging", "yes");
	rlib_set_output_parameter(r, "only_quote_strings", "yes");
	rlib_set_locale(r, "en_US");
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);

	return 0;
}
