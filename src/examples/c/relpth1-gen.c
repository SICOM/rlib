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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <rlib.h>

int main(void) {
	struct stat st;
	char *buf;
	int fd;
	rlib *r;

	stat("relpath/relpath.xml", &st);

	buf = malloc(st.st_size + 1);

	fd = open("relpath/relpath.xml", O_RDONLY);
	if (read(fd, buf, st.st_size) != st.st_size) {
		close(fd);
		return 1;
	}
	close(fd);

	buf[st.st_size] = '\0';

	r = rlib_init();
	rlib_add_search_path(r, "relpath");
	rlib_add_report_from_buffer(r, buf);
	rlib_set_output_format(r, RLIB_FORMAT_HTML);
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);

	free(buf);

	return 0;
}
