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
#ifndef _RPDF_INTERNAL_H_
#define _RPDF_INTERNAL_H_

#include <rpdf.h>

/*
 * Set this to 1 if RLIB sends UTF-8 strings
 * instead of using the pre-set encoding.
 */
#define RLIB_SENDS_UTF8 0

struct rpdf_page_info {
	HPDF_Page page;
	HPDF_Font current_font;
	gdouble font_size;
	gdouble r, g, b;
};

struct rpdf_delayed_text {
	gint page_number;
	HPDF_Font font;
	gdouble fontsize;
	gdouble x;
	gdouble y;
	gdouble angle;
	gdouble r, g, b;
	gint len;
};

struct rpdf {
	HPDF_Doc pdf;

	/* For debugging */
	const char *func;
	int line;

	gint current_page;
	gint page_count;
	struct rpdf_page_info **page_info;

	GHashTable *fonts;
	GHashTable *fontfiles;
#if RLIB_SENDS_UTF8
	GHashTable *convs;
#endif

	gint use_compression;
};

#endif /* _RPDF_INTERNAL_H_ */
