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

char *data[6][6] = {
	[0][0] = "name",
	[0][1] = "ty_sales",
	[0][2] = "ly_sales",
	[0][3] = "projection",
	[0][4] = "sold",
	[0][5] = "negative",

	[1][0] = "XX 1",
	[1][1] = "3000",
	[1][2] = "3300",
	[1][3] = "3300",
	[1][4] = "-1.5",
	[1][5] = "-2",

	[2][0] = "XX 2",
	[2][1] = "2400",
	[2][2] = "2100",
	[2][3] = "3301",
	[2][4] = "1",
	[2][5] = "-4",

	[3][0] = "XX 3",
	[3][1] = "4000",
	[3][2] = "4100",
	[3][3] = "3302",
	[3][4] = "2",
	[3][5] = "-6",

	[4][0] = "XX 4",
	[4][1] = "4700",
	[4][2] = "4100",
	[4][3] = "3303",
	[4][4] = "3",
	[4][5] = "-8",

	[5][0] = "XX 5",
	[5][1] = "2500",
	[5][2] = "5500",
	[5][3] = "3304",
	[5][4] = "8",
	[5][5] = "-10",
};

char *line_data[8][13] = {
	[0][0] = "dow",
	[0][1] = "rest1",
	[0][2] = "rest2",
	[0][3] = "rest3",
	[0][4] = "rest4",
	[0][5] = "rest5",
	[0][6] = "rest6",
	[0][7] = "os1",
	[0][8] = "os2",
	[0][9] = "os3",
	[0][10] = "os4",
	[0][11] = "os5",
	[0][12] = "os6",

	[1][0] = "Sunday",
	[1][1] = "100",
	[1][2] = "120",
	[1][3] = "130",
	[1][4] = "110",
	[1][5] = "120",
	[1][6] = "130",
	[1][7] = "5",
	[1][8] = "10",
	[1][9] = "-2",
	[1][10] = "-4",
	[1][11] = "4",
	[1][12] = "-1",

	[2][0] = "Monday",
	[2][1] = "120",
	[2][2] = "110",
	[2][3] = "120",
	[2][4] = "140",
	[2][5] = "150",
	[2][6] = "160",
	[2][7] = "-2",
	[2][8] = "4",
	[2][9] = "3",
	[2][10] = "-2",
	[2][11] = "2",
	[2][12] = "-2",

	[3][0] = "Tuesday",
	[3][1] = "110",
	[3][2] = "130",
	[3][3] = "110",
	[3][4] = "120",
	[3][5] = "130",
	[3][6] = "120",
	[3][7] = "12",
	[3][8] = "12",
	[3][9] = "-5",
	[3][10] = "2",
	[3][11] = "-4",
	[3][12] = "5",

	[4][0] = "Wednesday",
	[4][1] = "30",
	[4][2] = "50",
	[4][3] = "40",
	[4][4] = "90",
	[4][5] = "30",
	[4][6] = "40",
	[4][7] = "-5",
	[4][8] = "-2",
	[4][9] = "10",
	[4][10] = "12",
	[4][11] = "13",
	[4][12] = "-4",

	[5][0] = "Thusday",
	[5][1] = "60",
	[5][2] = "100",
	[5][3] = "90",
	[5][4] = "80",
	[5][5] = "70",
	[5][6] = "85",
	[5][7] = "5",
	[5][8] = "4",
	[5][9] = "3",
	[5][10] = "2",
	[5][11] = "1",
	[5][12] = "0",

	[6][0] = "Friday",
	[6][1] = "100",
	[6][2] = "120",
	[6][3] = "130",
	[6][4] = "110",
	[6][5] = "120",
	[6][6] = "130",
	[6][7] = "-2",
	[6][8] = "-3",
	[6][9] = "-4",
	[6][10] = "-5",
	[6][11] = "-6",
	[6][12] = "-7",

	[7][0] = "Saturday",
	[7][1] = "120",
	[7][2] = "110",
	[7][3] = "120",
	[7][4] = "140",
	[7][5] = "150",
	[7][6] = "160",
	[7][7] = "-2",
	[7][8] = "2",
	[7][9] = "-2",
	[7][10] = "2",
	[7][11] = "-2",
	[7][12] = "2",
};

char *pie_data[5][2] = {
	[0][0] = "emotion",
	[0][1] = "value",

	[1][0] = "Happy",
	[1][1] = "25",

	[2][0] = "Sad",
	[2][1] = "25",

	[3][0] = "Grumpy",
	[3][1] = "25",

	[4][0] = "Sleepy",
	[4][1] = "25",
};

char *pie_data2[18][2] = {
	[0][0] = "food",
	[0][1] = "value",

	[1][0] = "Pizza",
	[1][1] = "20",

	[2][0] = "Chicken",
	[2][1] = "30",

	[3][0] = "Meat",
	[3][1] = "20",

	[4][0] = "Potatos",
	[4][1] = "10",

	[5][0] = "Vegies",
	[5][1] = "10",

	[6][0] = "Fruit",
	[6][1] = "15",

	[7][0] = "Fish",
	[7][1] = "5",

	[8][0] = "Pasta",
	[8][1] = "20",

	[9][0] = "Ice Cream",
	[9][1] = "10",

	[10][0] = "Fresca",
	[10][1] = "50",

	[11][0] = "Water",
	[11][1] = "20",

	[12][0] = "Soda",
	[12][1] = "20",

	[13][0] = "Milk",
	[13][1] = "5",

	[14][0] = "Oj",
	[14][1] = "5",

	[15][0] = "Cake",
	[15][1] = "20",

	[16][0] = "Pie",
	[16][1] = "10",

	[17][0] = "Pudding",
	[17][1] = "5",
};

char *sales_data[5][3] = {
	[0][0] = "dow",
	[0][1] = "net",
	[0][2] = "check",

	[1][0] = "Monday",
	[1][1] = "8000",
	[1][2] = "5.00",

	[2][0] = "Tuesday",
	[2][1] = "6000",
	[2][2] = "5.50",

	[3][0] = "Wednesday",
	[3][1] = "6500",
	[3][2] = "4.50",

	[4][0] = "Thursday",
	[4][1] = "5000",
	[4][2] = "7.00",
};

char *sales_data2[9][5] = {
	[0] = { "company", "region", "district", "store", "sales" },

	[1] = { "Foo, Inc.", "Region A","District X", "Store #1", "1500.00" },
	[2] = { "Foo, Inc.", "Region A","District X", "Store #2", "1300.00" },
	[3] = { "Foo, Inc.", "Region A","District Y", "Store #3", "2000.00" }, 
	[4] = { "Foo, Inc.", "Region A","District Y", "Store #4", "1800.00" },
	[5] = { "Foo, Inc.", "Region B","District M", "Store #11", "2500.00" },
	[6] = { "Foo, Inc.", "Region B","District M", "Store #12", "2300.00" },
	[7] = { "Foo, Inc.", "Region B","District N", "Store #13", "3000.00" },
	[8] = { "Foo, Inc.", "Region B","District N", "Store #14", "2800.00" },
};

int main(int argc, char **argv) {
	rlib *r;

	if (argc == 1) {
		printf("usage: %s [ pdf | xml | txt | csv | html ]\n", argv[0]);
		return 1;
	}

	r = rlib_init();
	rlib_add_datasource_array(r, "local_array");
	rlib_add_query_array_as(r, "local_array", data, 6, 6, "data");
	rlib_add_query_array_as(r, "local_array", line_data, 8, 13, "line_data");
	rlib_add_query_array_as(r, "local_array", pie_data, 5, 2, "pie_data");
	rlib_add_query_array_as(r, "local_array", pie_data2, 18, 2, "pie_data2");
	rlib_add_query_array_as(r, "local_array", sales_data, 5, 3, "sales_data");
	rlib_add_query_array_as(r, "local_array", sales_data2, 9, 5, "sales_data2");
	rlib_set_output_parameter(r, "html_image_directory", "/tmp");
	rlib_set_output_parameter(r, "trim_links", "1");
	rlib_add_report(r, "graph.xml");
	rlib_set_output_format_from_text(r, argv[1]);
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);

	return 0;
}
