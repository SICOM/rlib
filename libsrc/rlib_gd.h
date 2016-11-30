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

#include "config.h"

#ifdef HAVE_GD
#include <gd.h>
#include <gdfontt.h>
#include <gdfonts.h>
#include <gdfontmb.h>
#include <gdFontMedium.h>
#include <gdfontl.h>
#include <gdfontg.h>
#else
#define gdMaxColors 256
#endif

struct rlib_gd {
#ifdef HAVE_GD
	gdImagePtr im;
#else
	void *im;
#endif
	gint color_pool[gdMaxColors];
	struct rlib_rgb rlib_color[gdMaxColors];
	gchar *file_name;
	gint white;
	gint black;
};

/***** PROTOTYPES: gd.c ********************************************************/
struct rlib_gd * rlib_gd_new(rlib *r, gint width, gint height, gchar *image_directory);
int rlib_gd_free(struct rlib_gd *rgd);
int rlib_gd_spool(rlib *r, struct rlib_gd *rgd);
int rlib_gd_text(struct rlib_gd *rgd, char *text, gint x, gint y, gboolean rotate, gboolean bold);
int rlib_gd_color_text(struct rlib_gd *rgd, char *text, gint x, gint y, gboolean rotate, gboolean bold, struct rlib_rgb *color);
int gd_get_string_width(const char *text, gboolean bold);
int gd_get_string_height(gboolean bold);
int rlib_gd_line(struct rlib_gd *rgd, gint x_1, gint y_1, gint x_2, gint y_2, struct rlib_rgb *color);
int rlib_gd_rectangle(struct rlib_gd *rgd, gint x, gint y, gint width, gint height, struct rlib_rgb *color);
int rlib_gd_arc(struct rlib_gd *rgd, gint x, gint y, gint radius, gint start_angle, gint end_angle, struct rlib_rgb *color);
int rlib_gd_set_thickness(struct rlib_gd *rgd, gint thickness);
