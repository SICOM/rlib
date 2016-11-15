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
	GString *cursor_name;
	GString *fetchstmt;
	gint chunk;
	gint row;
	gint tot_fields;
	gint isdone;
	gint last_action;
	gint *fields;
};

struct _private {
	PGconn *conn;
	gint cache_size;
	gint cursor_number;
};

static gint rlib_postgres_connect(gpointer input_ptr, const gchar *conninfo) {
	struct input_filter *input = input_ptr;
	struct _private *priv = INPUT_PRIVATE(input);
	rlib *r = input->r;
	PGconn *conn;

	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) != CONNECTION_OK) {
		r_error(r, "rlib_postgres_connect: Cannot connect to POSTGRES\n");
		PQfinish(conn);
		return -1;
	}

	priv->conn = conn;
	priv->cursor_number = 0;
	priv->cache_size = 1024;

	return 0;
}

static gint rlib_postgres_input_close(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	PQfinish(INPUT_PRIVATE(input)->conn);
	INPUT_PRIVATE(input)->conn = NULL;
	return 0;
}

static PGresult *rlib_postgres_fetch_next_chunk(struct input_filter *input, struct rlib_postgres_results *results) {
	struct _private *priv = INPUT_PRIVATE(input);
	PGresult *result;

	if (results->result) {
		PQclear(results->result);
		results->result = NULL;
	}

	result = PQexec(priv->conn, results->fetchstmt->str);
	if (PQresultStatus(result) != PGRES_TUPLES_OK) {
		PQclear(result);
		return NULL;
	}

	results->result = result;
	results->chunk++;
	results->row = -1;

	return result;
}

static PGresult *rlib_postgres_query(struct input_filter *input, struct rlib_postgres_results *results, gchar *query) {
	struct _private *priv = INPUT_PRIVATE(input);
	PGresult *result = NULL;
	GString *q = g_string_new(NULL);

	results->cursor_name = g_string_new(NULL);
	g_string_printf(results->cursor_name, "rlib_cursor%d", priv->cursor_number++);
	results->fetchstmt = g_string_new(NULL);
	if (priv->cache_size <= 0)
		g_string_printf(results->fetchstmt, "FETCH ALL FROM %s", results->cursor_name->str);
	else
		g_string_printf(results->fetchstmt, "FETCH %d FROM %s", priv->cache_size, results->cursor_name->str);
	g_string_printf(q, "DECLARE %s CURSOR WITH HOLD FOR %s", results->cursor_name->str, query);
	result = PQexec(priv->conn, q->str);
	g_string_free(q, TRUE);

	if (PQresultStatus(result) != PGRES_COMMAND_OK) {
		PQclear(result);
		g_string_free(results->cursor_name, TRUE);
		g_string_free(results->fetchstmt, TRUE);
		return NULL;
	}

	PQclear(result);

	return rlib_postgres_fetch_next_chunk(input, results);
}

static void rlib_postgres_start(gpointer input_ptr, gpointer result_ptr) {
	struct input_filter *input = input_ptr;
	struct _private *priv = INPUT_PRIVATE(input);
	struct rlib_postgres_results *results = result_ptr;

	if (results == NULL)
		return;

	if (results->chunk > 0) {
		GString *q = g_string_new(NULL);
		PGresult *result;

		g_string_printf(q, "MOVE ABSOLUTE 0 IN %s", results->cursor_name->str);
		result = PQexec(priv->conn, q->str);
		g_string_free(q, TRUE);
		PQclear(result);

		PQclear(results->result);
		results->result = NULL;
		results->chunk = -1;
	}

	results->row = -1;
	results->isdone = FALSE;
}

static gint rlib_postgres_next(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct input_filter *input = input_ptr;
	struct _private *priv = INPUT_PRIVATE(input);
	struct rlib_postgres_results *results = result_ptr;

	if (results == NULL)
		return FALSE;

	if (results->isdone)
		return FALSE;

	if (results->result == NULL || (results->result != NULL && (results->row == priv->cache_size - 1)))
		rlib_postgres_fetch_next_chunk(input, results);

	results->row++;

	results->isdone = (results->row < priv->cache_size && results->row == PQntuples(results->result));
	return !results->isdone;
}

static gint rlib_postgres_isdone(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;

	if (result == NULL)
		return TRUE;

	return result->isdone;
}

static gchar * rlib_postgres_get_field_value_as_string(gpointer input_ptr UNUSED, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_postgres_results *results = result_ptr;
	gint field = GPOINTER_TO_INT(field_ptr);
	field -= 1;
	return (results->row < 0 ? "" : PQgetvalue(results->result, results->row, field));
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
	guint count, i;

	if (input_ptr == NULL)
		return NULL;

	results = g_malloc0(sizeof(struct rlib_postgres_results));
	results->chunk = -1;
	results->row = -1;
	results->isdone = FALSE;

	if (results == NULL)
		return NULL;

	result = rlib_postgres_query(input, results, query->sql);
	if (result == NULL) {
		g_free(results);
		return NULL;
	}

	results->result = result;

	count = PQnfields(result);
	results->fields = g_malloc(sizeof(int) * count);
	for (i = 0; i < count; i++)
		results->fields[i] = i + 1;
	results->tot_fields = count;

	return results;
}

static gint rlib_postgres_num_fields(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_postgres_results *result = result_ptr;

	if (result == NULL)
		return 0;

	return result->tot_fields;
}

static void rlib_postgres_free_result(gpointer input_ptr, gpointer result_ptr) {
	struct input_filter *input = input_ptr;
	struct _private *priv = INPUT_PRIVATE(input);

	struct rlib_postgres_results *results = result_ptr;
	if (results) {
		PGresult *result;
		GString *q = g_string_new(NULL);

		PQclear(results->result);

		g_string_printf(q, "CLOSE %s", results->cursor_name->str);
		result = PQexec(priv->conn, q->str);
		PQclear(result);
		g_string_free(q, TRUE);

		g_free(results->fields);
		g_string_free(results->cursor_name, TRUE);
		g_string_free(results->fetchstmt, TRUE);
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

static void rlib_postgres_set_query_cache_size(gpointer input_ptr, gint cache_size) {
	struct input_filter *input = input_ptr;
	struct _private *priv;

	if (!input)
		return;

	priv = INPUT_PRIVATE(input);
	priv->cache_size = cache_size;
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
	input->num_fields = rlib_postgres_num_fields;
	input->start = rlib_postgres_start;
	input->next = rlib_postgres_next;
	input->isdone = rlib_postgres_isdone;
	input->new_result_from_query = postgres_new_result_from_query;
	input->get_field_value_as_string = rlib_postgres_get_field_value_as_string;
	input->get_error = rlib_postgres_get_error;

	input->resolve_field_pointer = rlib_postgres_resolve_field_pointer;

	input->free = rlib_postgres_free_input_filter;
	input->free_result = rlib_postgres_free_result;

	input->set_query_cache_size = rlib_postgres_set_query_cache_size;

	return input;
}
