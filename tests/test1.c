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
#include <stdio.h>
 
#include <rlib.h>
#include <rlib_input.h>


struct _data {
	char name[MAXSTRLEN];
	char address[MAXSTRLEN];
};

int main(int argc, char **argv) {
	char *hostname, *username, *password, *database;
	rlib *r;

	if(argc != 5) {
		fprintf(stderr, "%s requires 4 arguments hostname username password database\n", argv[0]);
		fprintf(stderr, "You provided %d\n", argc-1);
		return -1;
	}
	
	hostname = argv[1];
	username = argv[2];
	password = argv[3];
	database = argv[4];

	r = rlib_init();
	rlib_add_datasource_mysql(r, "local_mysql", hostname, username, password, database);
	rlib_add_query_as(r, "local_mysql", "select * from example", "example");
	rlib_add_report(r, "report.xml");
	rlib_set_output_format(r, RLIB_FORMAT_PDF);
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);
	return 0;
}
