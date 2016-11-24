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
#ifndef _RPDF_H_
#define _RPDF_H_

#include <glib.h>
#include <hpdf.h>
#include <rpdf.h>	/* for supported page sizes */

#define RPDF_DPI (72.0)

#define RPDF_IMAGE_JPEG 1
#define RPDF_IMAGE_PNG  2

#define RPDF_TYPE_FONT       1
#define RPDF_TYPE_TEXT       2
#define RPDF_TYPE_LINE       3
#define RPDF_TYPE_RECT       4
#define RPDF_TYPE_FILL       5
#define RPDF_TYPE_COLOR      6
#define RPDF_TYPE_IMAGE      7
#define RPDF_TYPE_MOVE       8
#define RPDF_TYPE_CLOSEPATH  9
#define RPDF_TYPE_STROKE    10
#define RPDF_TYPE_WIDTH     11
#define RPDF_TYPE_ARC       12
#define RPDF_TYPE_TEXT_CB   13

#define RPDF_PORTRAIT			HPDF_PAGE_PORTRAIT
#define RPDF_LANDSCAPE			HPDF_PAGE_LANDSCAPE

#define RPDF_PAPER_LETTER		HPDF_PAGE_SIZE_LETTER
#define RPDF_PAPER_LEGAL		HPDF_PAGE_SIZE_LEGAL
#define RPDF_PAPER_A3			HPDF_PAGE_SIZE_A3
#define RPDF_PAPER_A4			HPDF_PAGE_SIZE_A4
#define RPDF_PAPER_A5			HPDF_PAGE_SIZE_A5
#define RPDF_PAPER_B4			HPDF_PAGE_SIZE_B4
#define RPDF_PAPER_B5			HPDF_PAGE_SIZE_B5
#define RPDF_PAPER_EXECUTIVE	HPDF_PAGE_SIZE_EXECUTIVE
#define RPDF_PAPER_COMM10		HPDF_PAGE_SIZE_COMM10
#define RPDF_PAPER_US4x6		HPDF_PAGE_SIZE_US4x6
#define RPDF_PAPER_US4x8		HPDF_PAGE_SIZE_US4x8
#define RPDF_PAPER_US5x7		HPDF_PAGE_SIZE_US5x7

#define RPDF_FONT_STYLE_REGULAR		"regular"
#define RPDF_FONT_STYLE_BOLD		"bold"
#define RPDF_FONT_STYLE_ITALIC		"oblique"
#define RPDF_FONT_STYLE_BOLDITALIC	"bold oblique"

struct rpdf;

struct rpdf *rpdf_new(void);
void rpdf_set_compression(struct rpdf *pdf, gboolean use_compression);
gboolean rpdf_get_compression(struct rpdf *pdf);
void rpdf_set_title(struct rpdf *pdf, const gchar *title);
void rpdf_set_subject(struct rpdf *pdf, const gchar *subject);
void rpdf_set_author(struct rpdf *pdf, const gchar *author);
void rpdf_set_keywords(struct rpdf *pdf, const gchar *keywords);
void rpdf_set_creator(struct rpdf *pdf, const gchar *creator);
void rpdf_translate(struct rpdf *pdf, gdouble x, gdouble y);
void rpdf_rotate(struct rpdf *pdf, gdouble angle);
gint rpdf_new_page(struct rpdf *pdf, gint paper, gint orientation);
void rpdf_get_current_page_size(struct rpdf *pdf, gdouble *x, gdouble *y);
gboolean rpdf_set_page(struct rpdf *pdf, gint page);
gboolean rpdf_set_font(struct rpdf *pdf, const gchar *font, const gchar *style, const gchar *encoding, gdouble size);
void rpdf_set_font_size(struct rpdf *pdf, gdouble size);
gpointer rpdf_text_callback(struct rpdf *pdf, gdouble x, gdouble y, gdouble angle, gint len) __attribute__((warn_unused_result));
void rpdf_finalize_text_callback(struct rpdf *pdf, gpointer user_data, const gchar *text);
void rpdf_text(struct rpdf *pdf, gdouble x, gdouble y, gdouble angle, const gchar *text);
void rpdf_image(struct rpdf *pdf, gdouble x, gdouble y, gdouble width, gdouble height, gint image_type, gchar *file_name);
void rpdf_link(struct rpdf *pdf, gdouble start_x, gdouble start_y, gdouble end_x, gdouble end_y, const gchar *url);
void rpdf_moveto(struct rpdf *pdf, gdouble x, gdouble y);
void rpdf_set_line_width(struct rpdf *pdf, gdouble width);
void rpdf_lineto(struct rpdf *pdf, gdouble x, gdouble y);
void rpdf_closepath(struct rpdf *pdf);
void rpdf_stroke(struct rpdf *pdf);
void rpdf_rect(struct rpdf *pdf, gdouble x, gdouble y, gdouble width, gdouble height);
void rpdf_fill(struct rpdf *pdf);
void rpdf_setrgbcolor(struct rpdf *pdf, gdouble r, gdouble g, gdouble b);
gdouble rpdf_text_width(struct rpdf *pdf, const gchar *text);
void rpdf_arc(struct rpdf *pdf, gdouble x, gdouble y, gdouble radius, gdouble start_angle, gdouble end_angle);
gchar *rpdf_get_buffer(struct rpdf *pdf, guint *length);
void rpdf_free(struct rpdf *pdf);

#endif /* _RPDF_H_ */
