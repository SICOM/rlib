/*
 *  Copyright (C) 2003-2017 SICOM Systems, INC.
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

#ifndef _RLIB_INPUT_H_
#define _RLIB_INPUT_H_

/* #include <iconv.h> */
#include <charencoder.h> 

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif


struct input_info {
	char charset[64];
	GIConv encoder;
};


struct rlib;
struct input_filter;
typedef struct input_filter input_filter;

struct rlib_queries {
	gchar *sql;
	gchar *name;
	struct input_filter *input;
	gpointer private;
};

struct input_filter {
	gpointer private;
	struct rlib *r;
	struct input_info info;
	gint (*input_close)(input_filter *);
	gpointer (*new_result_from_query)(input_filter *, struct rlib_queries *);
	gint (*free)(input_filter *);
	gint (*first)(input_filter *, gpointer);
	gint (*next)(input_filter *, gpointer);
	gint (*previous)(input_filter *, gpointer);
	gint (*last)(input_filter *, gpointer);
	gint (*isdone)(input_filter *, gpointer);
	const gchar * (*get_error)(input_filter *);
	gchar * (*get_field_value_as_string)(input_filter *, gpointer, gpointer);
	gpointer (*resolve_field_pointer)(input_filter *, gpointer, gchar *);
	void (*free_result)(input_filter *, gpointer);
	void (*free_query)(input_filter *, struct rlib_queries *);
	gint (*set_encoding)(gpointer);
};

struct rlib_queries *rlib_alloc_query_space(struct rlib *r);

#endif /* _RLIB_INPUT_H_ */
