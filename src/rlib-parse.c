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
#include <string.h>
 
#include <rlib.h>

int main(int argc, char **argv) {
	rlib *r;
	int ret;

	if (argc != 2) {
		printf("Usage:\n");
		printf("%s report.xml\n", argv[0]);
		return 0;
	}

	r = rlib_init();
	rlib_add_report(r, argv[1]);
	ret = rlib_parse(r);
	printf("Parsing %s %s\n", argv[1], (ret ? "FAILED" : "was successful"));
	rlib_free(r);

	return !!ret;
}
