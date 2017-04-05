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
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "libpq-fe.h"
#include "rlib.h"
#include "rlib_input.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))

struct rlib_postgres_results {
	PGresult *result;
	gchar *name;
	gint row;
	gint tot_rows;
	gint tot_fields;
	gint isdone;
	gint last_action;
	gint *fields;
};

struct _private {
	PGconn *conn;
};

gpointer rlib_postgres_connect(input_filter *input, gchar *conninfo) {
	PGconn *conn;

	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) != CONNECTION_OK) {
		PQfinish(conn);
		conn = NULL;
	}

	INPUT_PRIVATE(input)->conn = conn;	
	return conn;
}

static gint rlib_postgres_input_close(input_filter *input) {
	PQfinish(INPUT_PRIVATE(input)->conn);
	INPUT_PRIVATE(input)->conn = NULL;
	return 0;
}

static PGresult * rlib_postgres_query(PGconn *conn, gchar *query) {
	PGresult *result = NULL;

	result = PQexec(conn, query);
	if (PQresultStatus(result) != PGRES_TUPLES_OK) {
		PQclear(result);
		return NULL;
	}
	return result;
}

static gint rlib_postgres_first(input_filter *input, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;
	if (result) {
		result->row = 0;
		result->isdone = FALSE;
	}
	return result != NULL ? TRUE : FALSE;
}

static gint rlib_postgres_next(input_filter *input, gpointer result_ptr) {
	struct rlib_postgres_results *results = result_ptr;
	if (results) {
		if(results->row+1 < results->tot_rows) {
			results->row++;
			results->isdone = FALSE;
			return TRUE;
		}
		results->isdone = TRUE;
	}
	return FALSE;
}

static gint rlib_postgres_isdone(input_filter *input, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;
	if (result)
		return result->isdone;
	else
		return TRUE;
}

static gint rlib_postgres_previous(input_filter *input, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;
	if (result) {
		if(result->row-1 >= 0) {
			result->row--;
			result->isdone = FALSE;
			return TRUE;
		}
		result->isdone = TRUE;
	}
	return FALSE;
}

static gint rlib_postgres_last(input_filter *input, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;
	if (result)
		result->row = result->tot_rows-1;
	return TRUE;
}

static gchar * rlib_postgres_get_field_value_as_string(input_filter *input, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_postgres_results *results = result_ptr;
	gint field = GPOINTER_TO_INT(field_ptr);
	field -= 1;
	return PQgetvalue(results->result, results->row, field);
}

static gpointer rlib_postgres_resolve_field_pointer(input_filter *input, gpointer result_ptr, gchar *name) {
	struct rlib_postgres_results *results = result_ptr;
	gint i=0;

	if (results) {
		for (i = 0; i < results->tot_fields; i++)
			if(!strcmp(PQfname(results->result, i), name)) {
				return GINT_TO_POINTER(results->fields[i]);
			}
	}
	return NULL;
}

gpointer postgres_new_result_from_query(input_filter *input, gchar *query) {
	struct rlib_postgres_results *results;
	PGresult *result;
	guint count,i;
	
	if(input == NULL)
		return NULL;
	
	result = rlib_postgres_query(INPUT_PRIVATE(input)->conn, query);
	if(result == NULL)
		return NULL;
	else {
		results = g_malloc(sizeof(struct rlib_postgres_results));
		results->result = result;
	}
	count = PQnfields(result);
	results->fields = g_malloc(sizeof(int) * count);
	for(i=0;i<count;i++) {
		results->fields[i] = i+1;
	}
	results->tot_fields = count;
	results->tot_rows = PQntuples(result);
	return results;
}

static void rlib_postgres_rlib_free_result(input_filter *input, gpointer result_ptr) {
	struct rlib_postgres_results *results = result_ptr;
	if (results) {
		PQclear(results->result);
		g_free(results->fields);
		g_free(results);
	}
}

static gint rlib_postgres_free_input_filter(input_filter *input) {
	g_free(input->private);
	g_free(input);
	return 0;
}

static const gchar * rlib_postgres_get_error(input_filter *input) {
	return PQerrorMessage(INPUT_PRIVATE(input)->conn);
}

gpointer rlib_postgres_new_input_filter(rlib *r) {
	struct input_filter *input;
	
	input = g_malloc(sizeof(struct input_filter));
	input->private = g_malloc(sizeof(struct _private));
	memset(input->private, 0, sizeof(struct _private));
	input->r = r;
	input->input_close = rlib_postgres_input_close;
	input->first = rlib_postgres_first;
	input->next = rlib_postgres_next;
	input->previous = rlib_postgres_previous;
	input->last = rlib_postgres_last;
	input->isdone = rlib_postgres_isdone;
	input->new_result_from_query = postgres_new_result_from_query;
	input->get_field_value_as_string = rlib_postgres_get_field_value_as_string;
	input->get_error = rlib_postgres_get_error;

	input->resolve_field_pointer = rlib_postgres_resolve_field_pointer;

	input->free = rlib_postgres_free_input_filter;
	input->free_result = rlib_postgres_rlib_free_result;
	return input;
}
