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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "rpdf.h"

#define FONTS 4
#define STYLES 4

void encode_text_into_pdf(struct rpdf *pdf, gchar *utf8_text, gchar *encoding) {
	char *fonts[FONTS] = { "FreeMono", "DejaVu Sans Mono", "Liberation Mono", "Century Schoolbook L" };
	char *styles[STYLES] = { RPDF_FONT_STYLE_REGULAR, RPDF_FONT_STYLE_ITALIC, RPDF_FONT_STYLE_BOLD, RPDF_FONT_STYLE_BOLDITALIC };
	int i, j;
	gdouble h = 9.5;
	gchar *text;
	GIConv conv;
	gsize foo1;
	conv = g_iconv_open(encoding, "UTF-8");

	rpdf_new_page(pdf, RPDF_PAPER_LETTER, RPDF_PORTRAIT);

	text = g_convert_with_iconv(utf8_text, strlen(utf8_text), conv, &foo1, &foo1, NULL);

	for (i = 0; i < FONTS; i++) {
		for (j = 0; j < STYLES; j++) {
			rpdf_set_font(pdf, fonts[i], styles[j], encoding, 12.0);
			rpdf_text(pdf, 1.0, h, 0.0, text);
			h = h - 0.25;
		}
	}

	g_free(text);
	g_iconv_close(conv);
}

int main(void) {
	gpointer delayed1, delayed2;
	ssize_t	byteswritten;
	size_t pos;
	guint pdf_size;
	char *buf;

	struct rpdf *pdf = rpdf_new();
	rpdf_new_page(pdf, RPDF_PAPER_LEGAL, RPDF_LANDSCAPE);
/*	rpdf_set_font(pdf, "Times-Italic", RPDF_FONT_STYLE_REGULAR, "MacRomanEncoding", 30.0); */
/*	rpdf_text(pdf, 1.0, 10.0, 0.0, "FARK - BOB KRATZ LOVES BACON ))"); */
	rpdf_set_font(pdf, "Times-Italic", RPDF_FONT_STYLE_REGULAR, "MacRomanEncoding", 20.0);
	rpdf_text(pdf, 2.0, 9.0, 0.0, "WOOT");
	delayed1 = rpdf_text_callback(pdf, 2.0, 9.5, 0.0, 50);
	delayed2 = rpdf_text_callback(pdf, 2.0, 10.0, 0.0, 50);
	rpdf_set_font(pdf, "Courier", RPDF_FONT_STYLE_REGULAR, "MacRomanEncoding", 16.0);
	rpdf_text(pdf, 3.0, 8.0, 0.0, "FARK");
	rpdf_set_font(pdf, "Courier", RPDF_FONT_STYLE_REGULAR, "MacRomanEncoding", 20.0);
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
	/*
	 * rpdf_fill now also does a "stroke" command in PDF
	 * libharu follows an internal state so this would
	 * cause a problem and the PDF wouldn't be generated.
	rpdf_stroke(pdf);
	 */

	rpdf_image(pdf, 0, 0, 50, 50, RPDF_IMAGE_JPEG, "logo.jpg"); 
	rpdf_image(pdf, 1, 1, 50, 50, RPDF_IMAGE_PNG, "logo.png"); 

	rpdf_new_page(pdf, RPDF_PAPER_LETTER, RPDF_PORTRAIT);
	rpdf_set_font(pdf, "Times-Italic", RPDF_FONT_STYLE_REGULAR, "MacRomanEncoding", 30.0);
//	rpdf_text(pdf, 1.0, 10.0, 0.0, "FARK - BOB KRATZ LOVES BACON ))");
	rpdf_set_page(pdf, 1);
	rpdf_set_font(pdf, "Times-Italic", RPDF_FONT_STYLE_REGULAR, "WinAnsiEncoding", 15.0);

	{
		char nielsen[250] = {};
		gchar *text;
		GIConv conv;
		gsize foo1;
		conv = g_iconv_open("ISO-8859-1", "UTF-8");

		//é = egrave = std = ___ mac=217 win=350 pdf=350
		//supposed to be 233
		//sprintf(nielsen, "BOBb N SEZ [Bouchées de poulet] [%c]", 236);
		//sprintf(nielsen, "[Croûtons] [Bouchées] [UNITÉ] [DÉPÔTS] [égère] [fouettŽs]");
		sprintf(nielsen, "[Croûtons] [Bouchées] [UNITÉ] [DÉPÔTS] [égère] [fouettZs]");
		//fprintf(stderr, "1=%d, 2=%d add=%d [%s]\n", foo1, foo2, nielsen8859, error->message);

		text = g_convert_with_iconv(nielsen, strlen(nielsen), conv, &foo1, &foo1, NULL);

		rpdf_text(pdf, 1.0, 10.0, 0.0, text);

		g_free(text);
		g_iconv_close(conv);
	}

	/*
	 * Demonstrate font embedding.
	 * libharu 2.2.1 (current version on Fedora) doesn't support UTF-8 encoding
	 *
	 * Every string must contain characters that can be represented
	 * in the same (single-byte) encoding.
	 *
	 * Not all fonts contains all characters (correctly or at all)
	 * as we can see with FreeMono regular, bold or bold italic.
	 */
	encode_text_into_pdf(pdf, "árvíztűrő tükörfúrógép ÁRVÍZTŰRŐ TÜKÖRFÚRÓGÉP", "ISO-8859-2");
	encode_text_into_pdf(pdf, "good day Добрый День", "ISO-8859-5");

/*	rpdf_image(pdf, 1, 1, 100, 100, RPDF_IMAGE_JPEG, "logo.jpg"); */
	rpdf_finalize_text_callback(pdf, delayed2, "IT WORKED THE SECOND TIME!");
	rpdf_finalize_text_callback(pdf, delayed1, "IT WORKED!");
	pos = 0;
	buf = rpdf_get_buffer(pdf, &pdf_size);
	while (pos < pdf_size) {
		byteswritten = write(1, buf + pos, pdf_size - pos);
		if (byteswritten > 0)
			pos += byteswritten;
		else
			break; /* ignore the error */
	}
	/*
	 * rpdf_get_buffer now returns an on-demand g_malloc()-ed
	 * area that must be freed.
	 */
	g_free(buf);
	rpdf_free(pdf);	
	return 0;
}
