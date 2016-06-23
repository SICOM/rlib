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
#include <string.h>
#include <rlib.h>

char *data[6][4] = {
	[0][0] = "first_name",
	[0][1] = "last_name",
	[0][2] = "color",
	[0][3] = "group",

	[1][0] = "Bob",
	[1][1] = "Doan",
	[1][2] = "blue",
	[1][3] = "1",

	[2][0] = "Eric",
	[2][1] = "Eburuschkin",
	[2][2] = "green",
	[2][3] = "1",

	[3][0] = "Mike",
	[3][1] = "Roth",
	[3][2] = "yellow",
	[3][3] = "2",

	[4][0] = "Bob",
	[4][1] = "Kratz",
	[4][2] = "pink",
	[4][3] = "2",

	[5][0] = "Steve",
	[5][1] = "Tilden",
	[5][2] = "purple",
	[5][3] = "2",
};

char *moredata[60][1] = {
	[0][0] = "name",
	[1][0] = "1",
	[2][0] = "2",
	[3][0] = "3",
	[4][0] = "4",
	[5][0] = "5",
	[6][0] = "6",
	[7][0] = "7",
	[8][0] = "8",
	[9][0] = "9",
	[10][0] = "10",
	[11][0] = "11",
	[12][0] = "12",
	[13][0] = "13",
	[14][0] = "14",
	[15][0] = "15",
	[16][0] = "16",
	[17][0] = "17",
	[18][0] = "18",
	[19][0] = "19",
	[20][0] = "20",
	[21][0] = "21",
	[22][0] = "22",
	[23][0] = "23",
	[24][0] = "24",
	[25][0] = "25",
	[26][0] = "26",
	[27][0] = "27",
	[28][0] = "28",
	[29][0] = "29",
	[30][0] = "30",
	[31][0] = "31",
	[32][0] = "32",
	[33][0] = "33",
	[34][0] = "34",
	[35][0] = "35",
	[36][0] = "36",
	[37][0] = "37",
	[38][0] = "38",
	[39][0] = "39",
	[40][0] = "40",
	[41][0] = "41",
	[42][0] = "42",
	[43][0] = "43",
	[44][0] = "44",
	[45][0] = "45",
	[46][0] = "46",
	[47][0] = "47",
	[48][0] = "48",
	[49][0] = "49",
	[50][0] = "50",
	[51][0] = "51",
	[52][0] = "52",
	[53][0] = "53",
	[54][0] = "54",
	[55][0] = "55",
	[56][0] = "56",
	[57][0] = "57",
	[58][0] = "58",
	[59][0] = "59",
};

int main(int argc, char **argv) {
	rlib *r;
	int runquery = 0;

	if (argc == 1)
		runquery = 1;
	else if (strcasecmp(argv[1], "-n") == 0 || strcasecmp(argv[1], "--no-query") == 0)
		runquery = 0;
	else if (strcasecmp(argv[1], "-h") == 0 || strcasecmp(argv[1], "--help") == 0) {
		printf("%s [options]\n", argv[0]);
		printf("-h, --help\tthis help\n");
		printf("-n, --no-query\tDon't run queries needed by the report\n");
		return 0;
	}

	r = rlib_init();
	rlib_add_search_path(r, "parts/subdir2");
	if (runquery) {
		rlib_add_datasource_array(r, "local_array");
		rlib_add_query_array_as(r, "local_array", data, 6, 4, "data");
		rlib_add_query_array_as(r, "local_array", moredata, 60, 1, "moredata");
	}
	rlib_add_report(r, "parts/flow_part_subdir.xml");
	rlib_set_output_format(r, RLIB_FORMAT_PDF);
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);
	return 0;
}
