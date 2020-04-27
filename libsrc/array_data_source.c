/*
 *  Copyright (C) 2016-2017 SICOM Systems, Inc.
 *
 *  Authors: Zoltán Böszörményi <zboszormenyi@sicom.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
 
#include "rlib.h"
#include "rlib_input.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))
#define QUERY_PRIVATE(query) (((struct _query_private *)query->private))

struct rlib_array_results {
	gint cols;
	gint rows;
	gint isdone;
	char **data;
	gint current_row;
};

struct _query_private {
	char **data;
	gint cols;
	gint rows;
};

static gint rlib_array_input_close(input_filter *input_ptr) {
	return TRUE;
}

static const gchar* rlib_array_get_error(input_filter *input) {
	return "Hard to make a mistake here.. try checking your names/spellings";
}

static gint rlib_array_first(input_filter *input, gpointer result_ptr) {
	struct rlib_array_results *result = result_ptr;

	if (result_ptr == NULL) {
		return FALSE;	
	}

	result->current_row = 1;
	result->isdone = FALSE;
	if (result->rows <= 1) {
		result->isdone = TRUE;
		return FALSE;
	}
	return TRUE;
}

static gint rlib_array_next(input_filter *input, gpointer result_ptr) {
	struct rlib_array_results *result = result_ptr;
	result->current_row++;
	result->isdone = FALSE;
	if (result->current_row < result->rows)
		return TRUE;
	result->isdone = TRUE;
	return FALSE;
}

static gint rlib_array_isdone(input_filter *input, gpointer result_ptr) {
	struct rlib_array_results *result = result_ptr;

	if(result == NULL)
		return TRUE;

	return result->isdone;
}

static gint rlib_array_previous(input_filter *input, gpointer result_ptr) {
	struct rlib_array_results *result = result_ptr;
	result->current_row--;
	result->isdone = FALSE;
	if(result->current_row >= 1)
		return TRUE;
	else
		return FALSE;
	return TRUE;
}

static gint rlib_array_last(input_filter *input, gpointer result_ptr) {
	struct rlib_array_results *result = result_ptr;
	result->current_row = result->rows - 1;
	return TRUE;
}

static gchar *rlib_array_get_field_value_as_string(input_filter *input, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_array_results *result = result_ptr;
	int which_field = GPOINTER_TO_INT(field_ptr) - 1;
	if (result->rows <= 1 || result->current_row >= result->rows)
		return "";

	return result->data[(result->current_row * result->cols) + which_field];
}

static gpointer rlib_array_resolve_field_pointer(input_filter *input, gpointer result_ptr, gchar *name) {
	struct rlib_array_results *result = result_ptr;
	int i;

	if (result_ptr == NULL)
		return NULL;

	for (i = 0; i < result->cols; i++) {
		if (strcmp(name, result->data[i]) == 0) {
			i++;
			return GINT_TO_POINTER(i);
		}
	}
	return NULL;
}

static void *rlib_array_new_result_from_query(input_filter *input, struct rlib_queries *query) {
	struct rlib_array_results *result;

	if (query == NULL)
		return NULL;

	result = g_new0(struct rlib_array_results, 1);
	if (result == NULL)
		return NULL;

	result->rows = QUERY_PRIVATE(query)->rows;
	result->cols = QUERY_PRIVATE(query)->cols;
	result->data = QUERY_PRIVATE(query)->data;

	return result;
}

static gint rlib_array_free_input_filter(input_filter *input) {
	g_free(input);
	return 0;
}

static void rlib_array_rlib_free_result(input_filter *input, gpointer result_ptr) {
	g_free(result_ptr);
}

static void rlib_array_rlib_free_query(input_filter *input, struct rlib_queries *query) {
	if (!query->private)
		return

	g_free(QUERY_PRIVATE(query));
	query->private = NULL;
}

static gint rlib_array_num_fields(input_filter *input, gpointer result_ptr) {
	struct rlib_array_results *result = result_ptr;

	if (result == NULL)
		return 0;

	return result->cols;
}

static gpointer rlib_array_new_input_filter(rlib *r) {
	struct input_filter *input;

	input = g_malloc0(sizeof(struct input_filter));
	input->r = r;
	input->private = NULL;
	input->input_close = rlib_array_input_close;
	input->first = rlib_array_first;
	input->next = rlib_array_next;
	input->previous = rlib_array_previous;
	input->last = rlib_array_last;
	input->get_error = rlib_array_get_error;
	input->isdone = rlib_array_isdone;
	input->new_result_from_query = rlib_array_new_result_from_query;
	input->get_field_value_as_string = rlib_array_get_field_value_as_string;

	input->resolve_field_pointer = rlib_array_resolve_field_pointer;

	input->free = rlib_array_free_input_filter;
	input->free_result = rlib_array_rlib_free_result;
	input->free_query = rlib_array_rlib_free_query;

	input->num_fields = rlib_array_num_fields;

	return input;
}

gint rlib_add_datasource_array(rlib *r, const gchar *input_name) {
	rlib_add_datasource(r, input_name, rlib_array_new_input_filter(r));
	return TRUE;
}

gint rlib_add_query_array_as(rlib *r, const gchar *input_source, gpointer array, gint rows, gint cols, const gchar *name) {
	gint i;

	for (i = 0; i < r->inputs_count;i++) {
		if (!strcmp(r->inputs[i].name, input_source)) {
			struct rlib_queries *query;
			struct _query_private *priv;

			priv = g_new0(struct _query_private, 1);
			if (priv == NULL) {
				r_error(r, "rlib_add_query_array_as: Out of memory!\n");
				return -1;
			}

			priv->rows = rows;
			priv->cols = cols;
			priv->data = array;

			query = rlib_alloc_query_space(r);
			if (query == NULL) {
				g_free(priv);
				r_error(r, "rlib_add_query_array_as: Out of memory!\n");
				return -1;
			}

			query->name = g_strdup(name);
			query->input = r->inputs[i].input;

			query->private = (gpointer)priv;

			return r->queries_count;
		}
	}

	r_error(r, "rlib_add_query_as: Could not find input source [%s]!\n", input_source);
	return -1;
}
