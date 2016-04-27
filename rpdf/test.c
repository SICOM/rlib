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
 * 
 * $Id$s
 *
 */
 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "rpdf.h"

static void callback_test(gchar *data, gint len, gpointer user_data) {
	sprintf(data, "IT WORKED!");
}

int main(int argc, char **argv) {
	ssize_t	byteswritten;
	size_t	pos;
	gint pdf_size;
	char *buf;

	struct rpdf *pdf = rpdf_new();
	rpdf_new_page(pdf, RPDF_PAPER_LEGAL, RPDF_LANDSCAPE);
/*	rpdf_set_font(pdf, "Times-Italic", "MacRomanEncoding", 30.0); */
/*	rpdf_text(pdf, 1.0, 10.0, 0.0, "FARK - BOB KRATZ LOVES BACON ))"); */
	rpdf_set_font(pdf, "Times-Italic", "MacRomanEncoding", 20.0);
	rpdf_text(pdf, 2.0, 9.0, 0.0, "WOOT");
	rpdf_text_callback(pdf, 2.0, 9.5, 0.0, 50, callback_test, NULL);
	rpdf_set_font(pdf, "Courier", "MacRomanEncoding", 16.0);
	rpdf_text(pdf, 3.0, 8.0, 0.0, "FARK");
	rpdf_set_font(pdf, "Courier", "MacRomanEncoding", 20.0);
	rpdf_text(pdf, 4.0, 7.0, 0.0, "WOOT\\");
	rpdf_text(pdf, 5.0, 6.0, 0.0, "TOOT");
	rpdf_text(pdf, 5.0, 12.0, 0.0, "GOOD TIMES");
	rpdf_text(pdf, 5.0, 13.0, 0.0, "GOOD TIMES");
	rpdf_text(pdf, 5.0, 14.0, 0.0, "GOOD TIMES");

	rpdf_text(pdf, 6.0, 5.0, 90.0, "ROTATE 90");
	rpdf_text(pdf, 7.0, 4.0, 45.0, "ROTATE 45");
	rpdf_link(pdf, 0, 0, 1, 1, "http://www.yahoo.com");
	rpdf_setrgbcolor(pdf, .3, .6, .9);
	rpdf_rect(pdf, 0, 0, 3, 3);
	rpdf_fill(pdf);

	rpdf_set_line_width(pdf, 10);
	rpdf_moveto(pdf, 2, 2);
	rpdf_lineto(pdf, 7, 7);
	rpdf_closepath(pdf); 
	rpdf_stroke(pdf); 

	rpdf_arc(pdf, 2, 2, 2, 0, 360);
	rpdf_fill(pdf);
	rpdf_stroke(pdf); 

	rpdf_image(pdf, 0, 0, 50, 50, RPDF_IMAGE_JPEG, "logo.jpg"); 
	rpdf_image(pdf, 1, 1, 50, 50, RPDF_IMAGE_PNG, "logo.png"); 

	rpdf_new_page(pdf, RPDF_PAPER_LETTER, RPDF_PORTRAIT);
	rpdf_set_font(pdf, "Times-Italic", "MacRomanEncoding", 30.0);
//	rpdf_text(pdf, 1.0, 10.0, 0.0, "FARK - BOB KRATZ LOVES BACON ))");
	rpdf_set_page(pdf, 1);
	rpdf_set_font(pdf, "Times-Italic", "WinAnsiEncoding", 15.0);
	
	//é = egrave = std = ___ mac=217 win=350 pdf=350
	//supposed to be 233
	char nielsen[250] = {};
	sprintf(nielsen, "BOBb N SEZ [Bouchées de poulet] [%c]", 236);
//	sprintf(nielsen, "[Croûtons] [Bouchées] [UNITÉ] [DÉPÔTS] [égère] [fouettŽs]");
	sprintf(nielsen, "[Croûtons] [Bouchées] [UNITÉ] [DÉPÔTS] [égère] [fouettZs]");
//	fprintf(stderr, "1=%d, 2=%d add=%d [%s]\n", foo1, foo2, nielsen8859, error->message);
	
	rpdf_text(pdf, 1.0, 10.0, 0.0, nielsen);
	
/*	rpdf_image(pdf, 1, 1, 100, 100, RPDF_IMAGE_JPEG, "logo.jpg"); */
	rpdf_finalize(pdf);
	pos = 0;
	buf = rpdf_get_buffer(pdf, &pdf_size);
	while (pos < pdf_size) {
		byteswritten = write(1, buf + pos, pdf_size - pos);
		if (byteswritten > 0)
			pos += byteswritten;
		else
			break; /* ignore the error */
	}
	rpdf_free(pdf);	
	return 0;
}
