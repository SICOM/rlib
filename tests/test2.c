/*
 *  Copyright (C) 2003 SICOM Systems, INC.
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
 
#include <rlib.h>
#include <input.h>


struct _data {
	char name[MAXSTRLEN];
	char address[MAXSTRLEN];
};

int main(int argc, char **argv) {
	char *conn;
	rlib *r;

	if(argc != 2) {
		fprintf(stderr, "%s requires 2 arguments POSTGREE conn str\n", argv[0]);
		fprintf(stderr, "You provided %d\n", argc-1);
		return -1;
	}
	
	conn = argv[1];

	fprintf(stderr, "CONN IS %s\n", conn);

	r = rlib_init(NULL);
	rlib_add_datasource_postgre(r, "local_postgre", conn);
	rlib_add_query_as(r, "local_postgre", "select * from example", "example");
	rlib_add_report(r, "report.xml", NULL);
	rlib_set_output_format(r, RLIB_FORMAT_PDF);
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);
	return 0;
}
