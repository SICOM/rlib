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
#ifndef _RPDF_INTERNAL_H_
#define _RPDF_INTERNAL_H_

#include <rpdf.h>

#define RPDF_MAX_CHARS 256

#define DEGREE_2_RAD(x) (x*M_PI/180.0)

struct _rpdf_paper {
	gint type;
	gint x;
	gint y;
	gchar name[64];
};
extern struct _rpdf_paper rpdf_paper [];

extern gint rpdf_courier_width[RPDF_MAX_CHARS];
extern gint rpdf_helvetica_width[RPDF_MAX_CHARS];
extern gint rpdf_helvetica_bold_width[RPDF_MAX_CHARS];
extern gint rpdf_helvetica_italics_width[RPDF_MAX_CHARS];
extern gint rpdf_helvetica_bold_italics_width[RPDF_MAX_CHARS];
extern gint rpdf_times_width[RPDF_MAX_CHARS];
extern gint rpdf_times_bold_width[RPDF_MAX_CHARS];
extern gint rpdf_times_italics_width[RPDF_MAX_CHARS];
extern gint rpdf_times_bold_italics_width[RPDF_MAX_CHARS];
extern gint rpdf_symbol_width[RPDF_MAX_CHARS];
extern gint rpdf_dingbats_width[RPDF_MAX_CHARS];

struct _rpdf_fonts {
	gchar name[64];
	gchar subtype[64];
	gint *widths;
};
extern struct _rpdf_fonts rpdf_fonts[];

struct rpdf_page_info {
	struct _rpdf_paper *paper;
	gint orientation;
	
	gboolean has_translation;
	gdouble translate_x;
	gdouble translate_y;
	
	gboolean has_rotation;
	gdouble rotation_angle;
	
	struct _rpdf_fonts *font;
	struct rpdf_font_object *font_object;
	gdouble font_size;
	GSList *annots;
	GSList *images;

	gint object_number;
};

struct rpdf_annots {
	gint number;
	gdouble start_x;
	gdouble start_y;
	gdouble end_x;
	gdouble end_y;
	gchar *url;
};

struct rpdf_image_jpeg {
	gint process;
	gint width;
	gint height;
	gint components;
	gint precision;
};

struct rpdf_image_png {
	gint width;
	gint height;
	gint bpc;
	gint ct;
	gchar params[128];
	GString *data;
	GString *palette;
	GString *trans;
	GSList *trans_list;
};

struct rpdf_images {
	gint number;
	gint image_type;
	gint length;
	gdouble x;
	gdouble y;
	gdouble width;
	gdouble height;
	gchar *data;
	gpointer metadata;
};

struct rpdf_object {
	gint object_number;
	gboolean put_wrapper;
	GString *contents;
	gchar *stream;
	gint stream_length;
};

struct rpdf_font_object {
	gint number;
	gint object_number;
	gchar name[64];
	gchar encoding[64];
	struct _rpdf_fonts *font;
};

struct rpdf_stream_font {
	struct rpdf_font_object *font_object;
	gdouble size;
};

struct rpdf_stream_text {
	gdouble angle;
	gdouble x;
	gdouble y;
	gchar *text;
};

struct rpdf_stream_text_callback {
	gdouble angle;
	gdouble x;
	gdouble y;
	gint len;
	CALLBACK;
	gpointer user_data;
};

struct rpdf_stream_arc {
	gdouble x;
	gdouble y;
	gdouble radius;
	gdouble start_angle;
	gdouble end_angle;
};

struct rpdf_stream_point {
	gdouble x;
	gdouble y;
};

struct rpdf_stream_rect {
	gdouble x;
	gdouble y;
	gdouble width;
	gdouble height;
};

struct rpdf_stream_color {
	gdouble r;
	gdouble g;
	gdouble b;
};

struct rpdf_stream {
	gint type;
	gpointer data;
};

struct rpdf {
	gchar *header;
	gchar *title;
	gchar *subject;
	gchar *author;
	gchar *keywords;
	gchar *creator;
	
	gint current_page;
	gint page_count;
	gint font_count;
	gint annot_count;
	gint image_count;
	
	gint size;
	gchar *out_buffer;
	gboolean has_images;
	gint object_count;
	GSList *objects; 
	GSList *xref;
	GHashTable *fonts;
	
	GSList **page_contents;
	struct rpdf_page_info **page_info;
	
	gboolean text_on;
	GString *page_data;
	GHashTable *page_fonts;
	GString *working_obj;
	gint font_obj_number;
	GSList *stream_font_destroyer;

	gpointer zlib_stream;
	gint use_compression;
};

#endif /* _RPDF_INTERNAL_H_ */
