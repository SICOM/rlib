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

char *salary[13][2] = {
	[0][0] = "date",
	[0][1] = "salary",

	[1][0] = "1945-01-10",
	[1][1] = "10101010101010101010101010101010101010101010101010.1010101010",

	[2][0] = "1945-02-10",
	[2][1] = "01010101010101010101010101010101010101010101010101.0101010101",

	[3][0] = "1945-03-10",
	[3][1] = "10101010101010101010101010101010101010101010101010.1010101010",

	[4][0] = "1945-04-10",
	[4][1] = "01010101010101010101010101010101010101010101010101.0101010101",

	[5][0] = "1945-05-10",
	[5][1] = "10101010101010101010101010101010101010101010101010.1010101010",

	[6][0] = "1945-06-10",
	[6][1] = "01010101010101010101010101010101010101010101010101.0101010101",

	[7][0] = "1945-07-10",
	[7][1] = "10101010101010101010101010101010101010101010101010.1010101010",

	[8][0] = "1945-08-10",
	[8][1] = "01010101010101010101010101010101010101010101010101.0101010101",

	[9][0] = "1945-09-10",
	[9][1] = "10101010101010101010101010101010101010101010101010.1010101010",

	[10][0] = "1945-10-10",
	[10][1] = "01010101010101010101010101010101010101010101010101.0101010101",

	[11][0] = "1945-11-10",
	[11][1] = "10101010101010101010101010101010101010101010101010.1010101010",

	[12][0] = "1945-12-10",
	[12][1] = "01010101010101010101010101010101010101010101010101.0101010101",
};

int main(int argc, char **argv) {
	rlib *r;

	if (argc == 1) {
		printf("usage: %s [ pdf | xml | txt | csv | html ]\n", argv[0]);
		return 1;
	}

	r = rlib_init();
	rlib_add_datasource_array(r, "local_array");
	rlib_add_query_array_as(r, "local_array", salary, 13, 2, "salary");
	rlib_add_report(r, "hyperinflation.xml");
	rlib_set_output_format_from_text(r, argv[1]);
	rlib_set_output_parameter(r, "debugging", "yes");
	rlib_set_output_parameter(r, "only_quote_strings", "yes");
	rlib_set_locale(r, "hu_HU");
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);

	return 0;
}
