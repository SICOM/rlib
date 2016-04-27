/*
 *  Copyright (C) 2003-2006 SICOM Systems, INC.
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
#include <rlib_input.h>

int main(int argc, char **argv) {
/*	char *filename;
	char buf[MAXSTRLEN];
	struct rlib_report *rep;
	if(argc != 2) {
		fprintf(stderr, "Usage: rlibc filename.xml\nOr: rlibc --help\n\n");
		return 1;
	}
	filename = argv[1];
	if(!strcmp(filename, "--help")) {
		fprintf(stderr, "rlibc converts a rlib xml file to a .rlib file for faster loading at runtime\n\n");
		return 1;	
	}

	rep = parse_report_file(filename);
	if(rep == NULL) {
		fprintf(stderr, "Invalid RLIB XML FILE %s\n", filename);
		return 1;
	}

	sprintf(buf, "%s.rlib", filename);
	save_report(rep, buf);
*/
	return 0;
}
