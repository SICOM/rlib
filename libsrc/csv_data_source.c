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
#include <unistd.h>

#include <glib.h>
#include <csv.h>

#include "rlib-internal.h"
#include "rlib_input.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))

/*
 * Make sure there is no padding in this struct
 * as we allocate and array of this.
 */
struct rlib_csv_field {
	gchar *field;
	union {
		gboolean alloc;
		gpointer dummy;
	};
};

struct rlib_csv_results {
	rlib *r;
	gboolean atstart;
	gboolean isdone;
	gint rows;
	gint cols;
	GSList *header;
	GList *detail;
	GList *navigator;
	struct rlib_csv_field *current_row;
	gint current_col;
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
	gint field_idx = GPOINTER_TO_INT(field_ptr) - 1;
	struct rlib_csv_field *fields;

	if (results == NULL)
		return "";

	if (results->navigator == NULL)
		return "";

	if (field_idx < 0 || field_idx > results->cols)
		return "";

	fields = results->navigator->data;
	return fields[field_idx].field;
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

static void rlib_csv_rlib_free_result(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_csv_results *results = result_ptr;
	GSList *header;
	GList *data;

	for (header = results->header; header; header = header->next)
		g_free(header->data);
	g_slist_free(results->header);

	for (data = results->detail; data; data = data->next) {
		struct rlib_csv_field *fields = data->data;
		int i;

		for (i = 0; i < results->cols; i++) {
			if (fields[i].alloc)
				g_free(fields[i].field);
		}

		g_free(fields);
	}
	g_list_free(results->detail);

	g_free(results);
}

static void csv_parser_field_cb(void *field_ptr, size_t field_sz, void *user_data) {
	struct rlib_csv_results *results = user_data;

	if (results->rows == 0) {
		results->header = g_slist_append(results->header, g_strdup((gchar *)field_ptr));
		results->cols++;
	} else {
		if (results->current_col == 0) {
			results->current_row = g_new0(struct rlib_csv_field, results->cols);
		}

		/*
		 * If there are more fields in the record
		 * than in the header, skip these extra fields.
		 */
		if (results->current_col < results->cols) {
			if (field_sz > 0) {
				results->current_row[results->current_col].field = g_strdup((gchar *)field_ptr);
				results->current_row[results->current_col].alloc = TRUE;
			} else
				results->current_row[results->current_col].field = "";
		}
	}

	results->current_col++;
}

static void csv_parser_eol_cb(int eolchar UNUSED, void *user_data) {
	struct rlib_csv_results *results = user_data;

	if (results->rows) {
		int i;

		/*
		 * Don't skip empty rows, treat it as
		 * a field list with all empty fields.
		 */
		if (results->current_row == NULL)
			results->current_row = g_new0(struct rlib_csv_field, results->cols);

		/*
		 * Make sure there is no NULL pointer
		 * in the fields structure. Record rows
		 * may have less fields than the number
		 * of fields in the header row.
		 */
		for (i = 0; i < results->cols; i++)
			if (results->current_row[i].field == NULL)
				results->current_row[i].field = "";

		results->detail = g_list_append(results->detail, results->current_row);
		results->current_row = NULL;
	}
	results->rows++;
	results->current_col = 0;
}

void *csv_new_result_from_query(gpointer input_ptr, gpointer query_ptr) {
	struct rlib_csv_results *results = NULL;
	struct input_filter *input = input_ptr;
	struct rlib_query *query = query_ptr;
	struct stat st;
	gchar *file;
	gint fd;
	size_t parser_retval;
	gchar *contents;
	struct csv_parser csv;
	GSList *header;

	INPUT_PRIVATE(input)->error = "";

	file = get_filename(input->r, query->sql, -1, FALSE, FALSE);
	if (stat(file, &st) != 0) {
		INPUT_PRIVATE(input)->error = "Error Opening File";
		return NULL;
	}

	fd = open(file, O_RDONLY, 6);
	g_free(file);
	if (fd < 0) {
		INPUT_PRIVATE(input)->error = "Error Opening File";
		return NULL;
	}

	contents = g_malloc(st.st_size);

	if (read(fd, contents, st.st_size) != st.st_size) {
		INPUT_PRIVATE(input)->error = "Error Reading File";
		g_free(contents);
		close(fd);
		return NULL;
	}

	close(fd);

	results = g_new0(struct rlib_csv_results, 1);
	results->r = input->r;

	if (csv_init(&csv, CSV_STRICT | CSV_REPALL_NL | CSV_STRICT_FINI | CSV_APPEND_NULL) != 0) {
		INPUT_PRIVATE(input)->error = "Error Initializing CSV Parser";
		rlib_csv_rlib_free_result(input_ptr, results);
		g_free(contents);
		return NULL;
	}

	parser_retval = csv_parse(&csv, contents, st.st_size,
						csv_parser_field_cb,
						csv_parser_eol_cb,
						results);

	g_free(contents);

	if (parser_retval != (size_t)st.st_size) {
		INPUT_PRIVATE(input)->error = "CSV Parser Error";
		rlib_csv_rlib_free_result(input_ptr, results);
		csv_free(&csv);
		return NULL;
	}

	csv_free(&csv);

	for (header = results->header; header; header = header->next) {
		if (strcmp((char *)header->data, "") == 0)
			break;
	}
	if (header) {
		INPUT_PRIVATE(input)->error = "Empty field name in CSV";
		rlib_csv_rlib_free_result(input_ptr, results);
		return NULL;
	}

	results->isdone = FALSE;
	results->navigator = NULL;


	return results;
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
