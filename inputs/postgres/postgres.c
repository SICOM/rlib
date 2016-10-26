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

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "libpq-fe.h"
#include "rlib_input.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))

#define UNUSED __attribute__((unused))

struct rlib_postgres_results {
	PGresult *result;
	gchar *name;
	gint row;
	gint tot_rows;
	gint tot_fields;
	gint atstart;
	gint isdone;
	gint last_action;
	gint *fields;
};

struct _private {
	PGconn *conn;
};

static gint rlib_postgres_connect(gpointer input_ptr, const gchar *conninfo) {
	struct input_filter *input = input_ptr;
	rlib *r = input->r;
	PGconn *conn;

	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) != CONNECTION_OK) {
		r_error(r, "rlib_postgres_connect: Cannot connect to POSTGRES\n");
		PQfinish(conn);
		return -1;
	}

	INPUT_PRIVATE(input)->conn = conn;	
	return 0;
}

static gint rlib_postgres_input_close(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
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

static void rlib_postgres_start(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;

	if (result == NULL)
		return;

	result->atstart = TRUE;
	result->row = -1;
	result->isdone = FALSE;
}

static gint rlib_postgres_next(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;

	if (result == NULL)
		return FALSE;

	if (result->isdone)
		return FALSE;

	result->atstart = FALSE;
	result->isdone = (result->row >= result->tot_rows);	
	return !result->isdone;
}

static gint rlib_postgres_isdone(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;

	if (result == NULL)
		return TRUE;

	return result->isdone;
}

static gint rlib_postgres_previous(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;

	if (result == NULL)
		return FALSE;

	if (result->atstart)
		return FALSE;

	result->row--;
	result->atstart = (result->row < 0);
	return !result->atstart;
}

static gint rlib_postgres_last(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;
	if (result)
		result->row = result->tot_rows-1;
	return TRUE;
}

static gchar * rlib_postgres_get_field_value_as_string(gpointer input_ptr UNUSED, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_postgres_results *results = result_ptr;
	gint field = GPOINTER_TO_INT(field_ptr);
	field -= 1;
	return PQgetvalue(results->result, results->row, field);
}

static gpointer rlib_postgres_resolve_field_pointer(gpointer input_ptr UNUSED, gpointer result_ptr, gchar *name) {
	struct rlib_postgres_results *result = result_ptr;
	gint i;

	if (result == NULL)
		return "";

	for (i = 0; i < result->tot_fields; i++)
		if (!strcmp(PQfname(result->result, i), name))
			return GINT_TO_POINTER(result->fields[i]);
	return "";
}

static gpointer postgres_new_result_from_query(gpointer input_ptr, gpointer query_ptr) {
	struct input_filter *input = input_ptr;
	struct rlib_query *query = query_ptr;
	struct rlib_postgres_results *results;
	PGresult *result;
	guint count,i;
	
	if(input_ptr == NULL)
		return NULL;

	result = rlib_postgres_query(INPUT_PRIVATE(input)->conn, query->sql);
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

static void rlib_postgres_rlib_free_result(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_postgres_results *results = result_ptr;
	if (results) {
		PQclear(results->result);
		g_free(results->fields);
		g_free(results);
	}
}

static gint rlib_postgres_free_input_filter(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	g_free(input->private);
	g_free(input);
	return 0;
}

static const gchar * rlib_postgres_get_error(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	return PQerrorMessage(INPUT_PRIVATE(input)->conn);
}

#ifdef HAVE_POSTGRES_BUILTIN
gpointer rlib_postgres_new_input_filter(rlib *r) {
#else
DLL_EXPORT_SYM gpointer new_input_filter(rlib *r) {
#endif
	struct input_filter *input;
	
	input = g_malloc0(sizeof(struct input_filter));
	input->private = g_malloc0(sizeof(struct _private));
	input->r = r;
	input->connect_with_connstr = rlib_postgres_connect;
	input->input_close = rlib_postgres_input_close;
	input->start = rlib_postgres_start;
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
