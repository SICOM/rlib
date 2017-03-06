/*
 *  Copyright (C) 2017 SICOM Systems, INC.
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
#include <rlib_input.h>

int main(int argc, char **argv) {
	const char *array[2][1] = {
		[0][0] = "field",
		[1][0] = ""
	};
	rlib *r;

	r = rlib_init();
	rlib_add_datasource_array(r, "arrays"); 
	rlib_add_query_array_as(r, "arrays", array, 2, 1, "data");
	rlib_add_report(r, "emptystr.xml");
	rlib_add_parameter(r, "param", "");
	if (argc >= 2)
		rlib_set_output_format_from_text(r, argv[1]);
	else
		rlib_set_output_format_from_text(r, "pdf");
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);
	return 0;
}
