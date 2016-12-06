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
 * Built in CSV Input Data Source
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>
#include <unistd.h>

#include "rlib-internal.h"
#include "rlib_input.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))

struct rlib_csv_results {
	gchar *contents;
	gboolean atstart;
	gboolean isdone;
	gint rows;
	gint cols;
	GSList *header;
	GList *detail;
	GList *navigator;
};

struct _private {
	gchar *error;
};

static void rlib_csv_input_close(gpointer input_ptr UNUSED) {}

static const gchar* rlib_csv_get_error(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	return INPUT_PRIVATE(input)->error;
}

static void rlib_csv_start(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_csv_results *result = result_ptr;

	if (result == NULL)
		return;

	result->navigator = NULL;
	result->atstart = TRUE;
	result->isdone = FALSE;
}

static gboolean rlib_csv_next(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_csv_results *result = result_ptr;

	if (result == NULL)
		return FALSE;

	if (result->atstart) {
		result->atstart = FALSE;
		result->navigator = result->detail;
	} else
		result->navigator = g_list_next(result->navigator);

	result->isdone = (result->navigator == NULL);
	return !result->isdone;
}

static gboolean rlib_csv_isdone(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_csv_results *result = result_ptr;

	if (result == NULL)
		return FALSE;

	return result->isdone;
}

static gchar * rlib_csv_get_field_value_as_string(gpointer input_ptr UNUSED, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_csv_results *results = result_ptr;
	gint i = 1;
	GSList *data;

	if (results == NULL)
		return "";

	if (results->navigator == NULL)
		return "";

	for (data = results->navigator->data; data != NULL; data = data->next) {
		if (GPOINTER_TO_INT(field_ptr) == i)
			return (gchar *)data->data;
		i++;
	}

	return "";
}

static gpointer rlib_csv_resolve_field_pointer(gpointer input_ptr UNUSED, gpointer result_ptr, gchar *name) { 
	struct rlib_csv_results *results = result_ptr;
	gint i;
	GSList *data;

	if (results == NULL || results->header == NULL)
		return NULL;

	for (i = 1, data = results->header; data != NULL; data = data->next, i++)
		if (strcmp((gchar *)data->data, name) == 0)
			return GINT_TO_POINTER(i);

	return NULL;
}

static gchar *rlib_csv_get_field_name(gpointer input_ptr UNUSED, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_csv_results *results = result_ptr;
	gint i, field = GPOINTER_TO_INT(field_ptr);
	GSList *data;

	if (results == NULL || results->header == NULL)
		return NULL;

	for (i = 1, data = results->header; data != NULL; data = data->next, i++)
		if (i == field)
			return data->data;

	return NULL;
}

static gint rlib_csv_num_fields(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_csv_results *results = result_ptr;

	if (results == NULL)
		return 0;

	return results->cols;
}

static gboolean parse_line(gchar **ptr, GSList **all_items) {
	gchar *data = *ptr;
	gint spot = 0;
	gchar current = 0, previous = 0;
	gboolean eof = FALSE;
	gchar *start = *ptr;
	GSList *items = NULL;
	gboolean in_quote = FALSE;

	while (1) {
		if(current != 0)
			previous = current;
		current = data[spot];
		if(current == '\0') {
			eof = TRUE;
			break;
		}
		
		if(current == '"' && previous != '\\')
			in_quote = !in_quote;
		
		if((current == ',' && !in_quote) || current == '\n') {
			gint slen;
			data[spot] = '\0';
			slen = strlen(start);
			if(start[0] == '"' && start[slen-1] == '"') {
				start[slen-1] = '\0';
				start++;
			}

			items = g_slist_append(items, start);

			if(current == '\n')
				break;
			
			in_quote = FALSE;
			start = *ptr + spot + 1;
		}
		spot++;
	}
	
	*ptr += spot+1;
	*all_items = items;
	
	return eof;
}

void *csv_new_result_from_query(gpointer input_ptr, gpointer query_ptr) {
	struct rlib_csv_results *results = NULL;
	struct input_filter *input = input_ptr;
	struct rlib_query *query = query_ptr;
	struct stat st;
	gchar *file;
	gint fd;
	gint size;
	gchar *contents;
	gchar *ptr;
	GSList *line_items;
	gint row = 0;

	INPUT_PRIVATE(input)->error = "";

	file = get_filename(input->r, query->sql, -1, FALSE, FALSE);
	if (stat(file, &st) != 0) {
		INPUT_PRIVATE(input)->error = "Error Opening File";
		return NULL;
	}

	size = st.st_size;

	fd = open(file, O_RDONLY, 6);
	g_free(file);
	if (fd < 0) {
		INPUT_PRIVATE(input)->error = "Error Opening File";
		return NULL;
	}

	contents = g_malloc(st.st_size + 1);
	contents[size] = 0;
	if (read(fd, contents, size) != size) {
		g_free(contents);
		INPUT_PRIVATE(input)->error = "Error Reading File";
		return NULL;
	}

	results = g_new0(struct rlib_csv_results, 1);
	results->isdone = FALSE;
	results->contents = contents;
	ptr = contents;
	while (!parse_line(&ptr, &line_items)) {
		if (row == 0)
			results->header = line_items;
		else
			results->detail = g_list_append(results->detail, line_items);
		row++;
	}

	results->navigator = NULL;
	results->rows = row;
	results->cols = g_slist_length(results->header);

	close(fd);

	return results;
}

static void rlib_csv_rlib_free_result(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_csv_results *results = result_ptr;
	GList *data;
	g_slist_free(results->header);
	for(data = results->detail; data != NULL; data = data->next) {
		g_slist_free(data->data);
	}
	g_list_free(results->detail);
	g_free(results->contents);
	g_free(results);
}

static void rlib_csv_free_input_filter(gpointer input_ptr){
	struct input_filter *input = input_ptr;
	g_free(input->private);
	g_free(input);
}

gpointer rlib_csv_new_input_filter(rlib *r) {
	struct input_filter *input;

	input = g_malloc0(sizeof(struct input_filter));
	input->r = r;
	input->private = g_malloc0(sizeof(struct _private));
	input->input_close = rlib_csv_input_close;
	input->num_fields = rlib_csv_num_fields;
	input->start = rlib_csv_start;
	input->next = rlib_csv_next;
	input->isdone = rlib_csv_isdone;
	input->get_error = rlib_csv_get_error;
	input->new_result_from_query = csv_new_result_from_query;
	input->get_field_name = rlib_csv_get_field_name;
	input->get_field_value_as_string = rlib_csv_get_field_value_as_string;
	input->resolve_field_pointer = rlib_csv_resolve_field_pointer;
	input->free = rlib_csv_free_input_filter;
	input->free_result = rlib_csv_rlib_free_result;
	return input;
}
