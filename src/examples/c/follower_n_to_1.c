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

char *data[7][5] = {
	[0][0] = "first_name",
	[0][1] = "last_name",
	[0][2] = "color",
	[0][3] = "group",
	[0][4] = "breakfast",

	[1][0] = "Bob",
	[1][1] = "Doan",
	[1][2] = "blue",
	[1][3] = "1",
	[1][4] = "Green Eggs And Spam I Am I Am",

	[2][0] = "Terry",
	[2][1] = "Doan",
	[2][2] = "blue",
	[2][3] = "1",
	[2][4] = "Ham and Eggs",

	[3][0] = "Eric",
	[3][1] = "Buruschkin",
	[3][2] = "green",
	[3][3] = "1",
	[3][4] = "Green Eggs And Spam I Am I Am",

	[4][0] = "Mike",
	[4][1] = "Roth",
	[4][2] = "yellow",
	[4][3] = "2",
	[4][4] = "Green Eggs And Spam I Am I Am",

	[5][0] = "Bob",
	[5][1] = "Kratz",
	[5][2] = "pink",
	[5][3] = "2",
	[5][4] = "Green Eggs And Spam I Am I Am",

	[6][0] = "Steve",
	[6][1] = "Tilden",
	[6][2] = "purple",
	[6][3] = "2",
	[6][4] = "Dude",
};

char *more_data[4][3] = {
	[0][0] = "first_name",
	[0][1] = "last_name",
	[0][2] = "initials",

	[1][0] = "Bob",
	[1][1] = "Doan",
	[1][2] = "WRD",

	[2][0] = "Terry",
	[2][1] = "Doan",
	[2][2] = "TD",

	[3][0] = "Eric",
	[3][1] = "Buruschkin",
	[3][2] = "ERB",
};

int main(int argc, char **argv) {
	rlib *r;

	if (argc == 1) {
		printf("usage: %s [ pdf | xml | txt | csv | html ]\n", argv[0]);
		return 1;
	}

	r = rlib_init();
	rlib_add_datasource_array(r, "local_array");
	rlib_add_query_array_as(r, "local_array", data, 7, 5, "data");
	rlib_add_query_array_as(r, "local_array", more_data, 4, 3, "more_data");
	rlib_add_resultset_follower_n_to_1(r, "data", "data.last_name", "more_data", "more_data.last_name");
	rlib_add_report(r, "follower.xml");
	rlib_set_output_format_from_text(r, argv[1]);
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);

	return 0;
}
