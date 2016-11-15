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

char *data[9][5] = {
	[0][0] = "company",
	[0][1] = "region",
	[0][2] = "district",
	[0][3] = "store",
	[0][4] = "sales",

	[1][0] = "Foo, Inc.",
	[1][1] = "Region A",
	[1][2] = "District X",
	[1][3] = "Store #1",
	[1][4] = "1500.00",

	[2][0] = "Foo, Inc.",
	[2][1] = "Region A",
	[2][2] = "District X",
	[2][3] = "Store #2",
	[2][4] = "1300.00",

	[3][0] = "Foo, Inc.",
	[3][1] = "Region A",
	[3][2] = "District Y",
	[3][3] = "Store #3",
	[3][4] = "2000.00",

	[4][0] = "Foo, Inc.",
	[4][1] = "Region A",
	[4][2] = "District Y",
	[4][3] = "Store #4",
	[4][4] = "1800.00",

	[5][0] = "Foo, Inc.",
	[5][1] = "Region B",
	[5][2] = "District M",
	[5][3] = "Store #11",
	[5][4] = "2500.00",

	[6][0] = "Foo, Inc.",
	[6][1] = "Region B",
	[6][2] = "District M",
	[6][3] = "Store #12",
	[6][4] = "2300.00",

	[7][0] = "Foo, Inc.",
	[7][1] = "Region B",
	[7][2] = "District N",
	[7][3] = "Store #13",
	[7][4] = "3000.00",

	[8][0] = "Foo, Inc.",
	[8][1] = "Region B",
	[8][2] = "District N",
	[8][3] = "Store #14",
	[8][4] = "2800.00",
};

int main(int argc, char **argv) {
	rlib *r;

	if (argc == 1) {
		printf("usage: %s [ pdf | xml | txt | csv | html ]\n", argv[0]);
		return 1;
	}

	r = rlib_init();
	rlib_add_datasource_array(r, "local_array");
	rlib_add_query_array_as(r, "local_array", data, 9, 5, "data");
	rlib_add_report(r, "breaks.xml");
	rlib_set_output_format_from_text(r, argv[1]);
	rlib_set_output_parameter(r, "debugging", "yes");
	rlib_set_output_parameter(r, "only_quote_strings", "yes");
	rlib_set_locale(r, "en_US");
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);

	return 0;
}
