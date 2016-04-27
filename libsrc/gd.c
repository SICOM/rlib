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
 *
 * $Id$
 * 
 * This module generates a report from the information stored in the current
 * report object.
 * The main entry point is called once at report generation time for each
 * report defined in the rlib object.
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <glib.h>

#include "config.h"
#include "rlib-internal.h"
#include "rlib_gd.h"

#ifdef HAVE_GD
static char *unique_file_name(gchar *buf, gchar *image_directory) {
#ifdef HAVE_SYS_TIME_H
	struct timeval tv;
	gint pid = getpid();
	static gint counter;

	gettimeofday(&tv, NULL);
	if(image_directory != NULL)
		sprintf(buf, "%s/RLIB_IMAGE_FILE_%d_%ld_%ld_%d.png", image_directory, pid, tv.tv_sec, tv.tv_usec, counter++);
	else
		sprintf(buf, "RLIB_IMAGE_FILE_%d_%ld_%ld_%d.png", pid, tv.tv_sec, tv.tv_usec, counter++);
#else
	/* tempnam() accepts NULL as directory name and it's a standard
	 * part of <stdio.h>. Most importantly, it also exists under MingW.
	 */
	sprintf(buf, "%s.png", tempnam(image_directory, "RLIB_IMAGE_FILE_XXXXX"));
#endif
	return buf;
}

int get_color_pool(struct rlib_gd *rgd, struct rlib_rgb *rgb) {
	int i;
	for(i=0;i<gdMaxColors;i++) {
		if(rgd->color_pool[i] != -1) {
			if(memcmp(rgb, &rgd->rlib_color[i], sizeof(struct rlib_rgb)) == 0)
				return rgd->color_pool[i];		
		} else {
			rgd->color_pool[i] = gdImageColorAllocate(rgd->im, rgb->r*255, rgb->g*255, rgb->b*255);
			memcpy(&rgd->rlib_color[i], rgb, sizeof(struct rlib_rgb));
			return rgd->color_pool[i];
		}
	}

	return -1;
}

struct rlib_gd * rlib_gd_new(gint width, gint height, gchar *image_directory) {
	struct rlib_gd *rgd = g_malloc(sizeof(struct rlib_gd));
	char file_name[MAXSTRLEN];
	int fd;
	int i;
	
	memset(rgd, 0, sizeof(struct rlib_gd));
	
	for(i=0;i<gdMaxColors;i++)
		rgd->color_pool[i] = -1;
	
	
	rgd->im =  gdImageCreate(width, height);
	
	while(1) {
		unique_file_name(file_name, image_directory);
		fd = open(file_name, O_RDONLY, 0);
		if(fd < 0) {
			fd = open(file_name, O_CREAT, 0666);
			close(fd);
			break;
		}
		close(fd);
	}

	rgd->file_name = g_strdup(file_name);
	rgd->white = gdImageColorAllocate(rgd->im, 255, 255, 255);	
	rgd->black = gdImageColorAllocate(rgd->im, 0, 0, 0);	
	gdImageFilledRectangle(rgd->im, 0, 0, width, height, rgd->white);
	gdImageRectangle(rgd->im, 0, 0, width-1, height-1, rgd->black);

	return rgd;
}

int rlib_gd_spool(rlib *r, struct rlib_gd *rgd) {
	FILE *out;
	out = fopen(rgd->file_name, "wb");
	if(out != NULL) {
		gdImagePng(rgd->im, out);
		fclose(out);
	} else {
		r_error(r, "GD PROBLEM: Could not write %s\n", rgd->file_name);
	}
	return TRUE;
}

int rlib_gd_text(struct rlib_gd *rgd, char *text, int x, int y, gboolean rotate, gboolean bold) {
	if(bold) {
		if(rotate)
			gdImageStringUp(rgd->im, gdFontMediumBold,	x,	y,	(unsigned char *)text, rgd->black);
		else
			gdImageString(rgd->im, gdFontMediumBold, x,	y,	(unsigned char *)text, rgd->black);
	} else {
		if(rotate)
			gdImageStringUp(rgd->im, gdFontMedium,	x,	y,	(unsigned char *)text, rgd->black);
		else
			gdImageString(rgd->im, gdFontMedium, x,	y,	(unsigned char *)text, rgd->black);	
	}		
	return TRUE;
}

int rlib_gd_color_text(struct rlib_gd *rgd, char *text, int x, int y, gboolean rotate, gboolean bold, struct rlib_rgb *color) {
	gint gd_color = get_color_pool(rgd, color);
	if(bold) {
		if(rotate)
			gdImageStringUp(rgd->im, gdFontMediumBold,	x,	y,	(unsigned char *)text, gd_color);
		else
			gdImageString(rgd->im, gdFontMediumBold, x,	y,	(unsigned char *)text, gd_color);
	} else {
		if(rotate)
			gdImageStringUp(rgd->im, gdFontMedium,	x,	y,	(unsigned char *)text, gd_color);
		else
			gdImageString(rgd->im, gdFontMedium, x,	y,	(unsigned char *)text, gd_color);	
	}		
	return TRUE;
}

int rlib_gd_get_string_width(struct rlib_gd *rgd, const gchar *text, gboolean bold) {
	if(bold)
		return gdFontMediumBold->w * strlen(text);
	else
		return gdFontMedium->w * strlen(text);
}

int rlib_gd_get_string_height(struct rlib_gd *rgd, gboolean bold) {
	if(bold)
		return gdFontMediumBold->h;
	else
		return gdFontMedium->h;
}

int rlib_gd_set_thickness(struct rlib_gd *rgd, int thickness) {
	gdImageSetThickness(rgd->im, thickness);
	return TRUE;
}


int rlib_gd_line(struct rlib_gd *rgd, gint x_1, gint y_1, gint x_2, gint y_2, struct rlib_rgb *color) {
	gint gd_color;
	
	if(color != NULL)
		gd_color = get_color_pool(rgd, color);	
	else
		gd_color = rgd->black;

	gdImageLine(rgd->im, x_1, y_1, x_2, y_2, gd_color);	
	return TRUE;
}


int rlib_gd_rectangle(struct rlib_gd *rgd, gint x, gint y, gint width, gint height, struct rlib_rgb *color) {
	gint gd_color;

	if(height < 0) {
		y -= height;
		height=abs(height);
	};
	
	
	if(color != NULL)
		gd_color = get_color_pool(rgd, color);	
	else
		gd_color = rgd->black;

	gdImageFilledRectangle(rgd->im, x, y-height, x+width, y, gd_color);	
	return TRUE;
}

int rlib_gd_arc(struct rlib_gd *rgd, gint x, gint y, gint radius, gint start_angle, gint end_angle, struct rlib_rgb *color) {
	gint gd_color;

	radius *= 2;
	
	if(color != NULL)
		gd_color = get_color_pool(rgd, color);	
	else
		gd_color = rgd->black;

	gdImageFilledArc(rgd->im, x, y, radius, radius, start_angle, end_angle, gd_color, gdArc);
	return TRUE;
}


int rlib_gd_free(struct rlib_gd *rgd) {
	gdImageDestroy(rgd->im);
	g_free(rgd->file_name);
	g_free(rgd);
	return TRUE;
}

#else

struct rlib_gd * rlib_gd_new(gint width, gint height, gchar *image_directory) {
	return NULL;
}

int rlib_gd_text(struct rlib_gd *rgd, char *text, int x, int y, int rotate, gboolean bold) {
	return TRUE;
}

int rlib_gd_color_text(struct rlib_gd *rgd, char *text, int x, int y, gboolean rotate, gboolean bold, struct rlib_rgb *color) {
	return TRUE;
}

int rlib_gd_get_string_width(struct rlib_gd *rgd, const char *text, gboolean bold) {
	return 0;
}

int rlib_gd_get_string_height(struct rlib_gd *rgd, gboolean bold) {
	return 0;
}

int rlib_gd_set_thickness(struct rlib_gd *rgd, int thickness) {
	return TRUE;
}

int rlib_gd_line(struct rlib_gd *rgd, gint x_1, gint y_1, gint x_2, gint y_2, struct rlib_rgb *color) {
	return TRUE;
}

int rlib_gd_rectangle(struct rlib_gd *rgd, gint x, gint y, gint width, gint height, struct rlib_rgb *color)  {
	return TRUE;
}

int rlib_gd_arc(struct rlib_gd *rgd, gint x, gint y, gint radius, gint start_angle, gint end_angle, struct rlib_rgb *color) {
	return TRUE;
}

int rlib_gd_spool(rlib *r, struct rlib_gd *rgd) {
	return TRUE;
}

int rlib_gd_free(struct rlib_gd *rgd) {
	return TRUE;
}
#endif
