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
#include <mysql.h>
#include <glib.h>

#include "util.h"
#include "rlib.h"
#include "rlib_input.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))
#define QUERY_PRIVATE(query) (((struct _query_private *)query->private))

#define UNUSED __attribute__((unused))

struct rlib_mysql_results {
	struct rlib_query *query;
	gchar *name;
	MYSQL_ROW this_row;
	guint current_row;
	guint rows;
	guint cols;
	gint isdone;
	guint *fields;
};

struct _private {
	MYSQL *mysql;
};

struct _query_private {
	MYSQL_RES *result;
};

static gint rlib_mysql_connect_group(gpointer input_ptr, const gchar *group) {
	struct input_filter *input = input_ptr;
	rlib *r = input->r;
	MYSQL *mysql;

	mysql = mysql_init(NULL);

	if (mysql == NULL)
		return -1;

	if (mysql_options(mysql,MYSQL_READ_DEFAULT_GROUP,group)) {
		mysql_close(mysql);
		r_error(r, "Error in mysql_options\n");
		return -1;
	}

	if (mysql_real_connect(mysql, mysql->options.host, mysql->options.user, mysql->options.password, mysql->options.db, mysql->options.port, mysql->options.unix_socket, 0) == NULL) {
		mysql_close(mysql);
		r_error(r, "Error in mysql_real_connect\n");
		return -1;
	}

	mysql_select_db(mysql, mysql->options.db);

	INPUT_PRIVATE(input)->mysql = mysql;
	return 0;
}

static gint rlib_mysql_connect_with_credentials(gpointer input_ptr, const gchar *host, guint port, const gchar *user, const gchar *password, const gchar *database) {
	struct input_filter *input = input_ptr;
	rlib *r = input->r;
	MYSQL *mysql;
	gchar *host_copy = NULL;

	mysql = mysql_init(NULL);

	if (mysql == NULL)
		return -1;

	if (host) {
		char *tmp, *port_s;
		host_copy = g_strdup(host);
		tmp = strchr(host_copy, ':');
		if (tmp) {
			*tmp = '\0';
			port_s = tmp + 1;
			port = atoi(port_s);
		}
	}

	if (mysql_real_connect(mysql, host_copy, user, password, database, port, NULL, 0) == NULL) {
		mysql_close(mysql);
		r_error(r, "Error in mysql_real_connect\n");
		return -1;
	}

	g_free(host_copy);

	mysql_select_db(mysql, database);

	INPUT_PRIVATE(input)->mysql = mysql;
	return 0;
}

static gint rlib_mysql_input_close(gpointer input_ptr) {
	struct input_filter *input = input_ptr;

	mysql_close(INPUT_PRIVATE(input)->mysql);
#if MYSQL_VERSION_ID > 40110
/*	mysql_library_end(); */
#endif
	INPUT_PRIVATE(input)->mysql = NULL;

	return 0;
}

static MYSQL_RES * rlib_mysql_query(MYSQL *mysql, gchar *query) {
	MYSQL_RES *result = NULL;
	gint rtn;
	rtn = mysql_real_query(mysql, query, strlen(query));
	if (rtn == 0) {
		result = mysql_store_result(mysql);
		return result;
	}
	return NULL;
}

static gint rlib_mysql_first(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_mysql_results *result = result_ptr;
	struct rlib_query *query;

	if (!result)
		return FALSE;

	query = result->query;

	result->current_row = 0;
	mysql_data_seek(QUERY_PRIVATE(query)->result, result->current_row);
	result->this_row = mysql_fetch_row(QUERY_PRIVATE(query)->result);
	result->isdone = (result->this_row == NULL);
	return !result->isdone;
}

static gint rlib_mysql_next(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_mysql_results *result = result_ptr;
	struct rlib_query *query;

	if (!result)
		return FALSE;

	if (result->isdone)
		return FALSE;

	query = result->query;

	result->current_row++;
	mysql_data_seek(QUERY_PRIVATE(query)->result, result->current_row);
	result->this_row = mysql_fetch_row(QUERY_PRIVATE(query)->result);
	result->isdone = (result->this_row == NULL);
	return !result->isdone;
}

static gint rlib_mysql_isdone(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_mysql_results *result = result_ptr;

	if (result)
		return result->isdone;
	else
		return TRUE;
}

static gint rlib_mysql_previous(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_mysql_results *result = result_ptr;
	struct rlib_query *query;

	if (!result)
		return FALSE;

	if (result->current_row <= 0)
		return FALSE;

	query = result->query;

	result->current_row--;
	mysql_data_seek(QUERY_PRIVATE(query)->result, result->current_row);
	result->this_row = mysql_fetch_row(QUERY_PRIVATE(query)->result);
	result->isdone = (result->this_row == NULL);
	return !result->isdone;
}

static gint rlib_mysql_last(gpointer input_ptr UNUSED, gpointer result_ptr UNUSED) {
	struct rlib_mysql_results *result = result_ptr;
	struct rlib_query *query;

	if (!result)
		return FALSE;

	if (result->current_row <= 0)
		return FALSE;

	query = result->query;

	result->current_row = result->rows - 1;
	mysql_data_seek(QUERY_PRIVATE(query)->result, result->current_row);
	result->this_row = mysql_fetch_row(QUERY_PRIVATE(query)->result);
	result->isdone = (result->this_row == NULL);
	return !result->isdone;
}

static gchar *rlib_mysql_get_field_value_as_string(gpointer input_ptr UNUSED, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_mysql_results *result = result_ptr;
	gint field = GPOINTER_TO_INT(field_ptr);

	if (result_ptr == NULL)
		return (gchar *)"";

	field -= 1;
	if (result->this_row == NULL)
		return (gchar *)"";

	return result->this_row[field];
}

static gpointer rlib_mysql_resolve_field_pointer(gpointer input_ptr UNUSED, gpointer result_ptr, gchar *name) {
	struct rlib_mysql_results *result = result_ptr;
	struct rlib_query *query;
	gint x = 0;
	MYSQL_FIELD *field;

	if (!result)
		return NULL;

	query = result->query;

	mysql_field_seek(QUERY_PRIVATE(query)->result, 0);

	while ((field = mysql_fetch_field(QUERY_PRIVATE(query)->result))) {
		if (!strcmp(field->name, name)) {
			return GINT_TO_POINTER(result->fields[x]);
		}
		x++;
	}
	return NULL;
}

static void *mysql_new_result_from_query(gpointer input_ptr, gpointer query_ptr) {
	struct input_filter *input = input_ptr;
	struct rlib_query *query = query_ptr;
	struct rlib_mysql_results *results;
	MYSQL_RES *result;
	guint i;

	query->private = (gpointer)g_new0(struct _query_private, 1);

	result = rlib_mysql_query(INPUT_PRIVATE(input)->mysql, query->sql);
	if (result == NULL)
		return NULL;

	QUERY_PRIVATE(query)->result = result;

	results = g_malloc0(sizeof(struct rlib_mysql_results));
	results->query = query;
	results->rows = mysql_num_rows(result);
	results->cols = mysql_field_count(INPUT_PRIVATE(input)->mysql);

	results->fields = g_malloc(sizeof(gint) * results->cols);
	for (i = 0; i < results->cols; i++) {
		results->fields[i] = i + 1;
	}
	return results;
}

static void rlib_mysql_rlib_free_result(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_mysql_results *result = result_ptr;

	if (!result)
		return;

	g_free(result->fields);
	g_free(result);
}

static gint rlib_mysql_free_input_filter(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	g_free(input->private);
	g_free(input);
	return 0;
}

static void rlib_mysql_free_query(gpointer input_ptr UNUSED, gpointer query_ptr) {
	struct rlib_query *query = query_ptr;

	mysql_free_result(QUERY_PRIVATE(query)->result);
	g_free(QUERY_PRIVATE(query));
}

static const gchar* rlib_mysql_get_error(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	return mysql_error(INPUT_PRIVATE(input)->mysql);
}

static guint rlib_mysql_result_rowcount(gpointer result_ptr) {
	struct rlib_mysql_results *results = result_ptr;

	if (results)
		return results->rows;

	return 0;
}

#ifdef HAVE_MYSQL_BUILTIN
gpointer rlib_mysql_new_input_filter(rlib *r) {
#else
DLL_EXPORT_SYM gpointer new_input_filter(rlib *r) {
#endif
	struct input_filter *input;

	input = g_malloc0(sizeof(struct input_filter));
	input->private = g_malloc0(sizeof(struct _private));
	input->r = r;
	input->connect_with_connstr = rlib_mysql_connect_group;
	input->connect_with_credentials = rlib_mysql_connect_with_credentials;
	input->input_close = rlib_mysql_input_close;
	input->first = rlib_mysql_first;
	input->next = rlib_mysql_next;
	input->previous = rlib_mysql_previous;
	input->last = rlib_mysql_last;
	input->isdone = rlib_mysql_isdone;
	input->get_error = rlib_mysql_get_error;
	input->new_result_from_query = mysql_new_result_from_query;
	input->get_field_value_as_string = rlib_mysql_get_field_value_as_string;

	input->resolve_field_pointer = rlib_mysql_resolve_field_pointer;

	input->free = rlib_mysql_free_input_filter;
	input->free_result = rlib_mysql_rlib_free_result;
	input->free_query = rlib_mysql_free_query;

	input->result_rowcount = rlib_mysql_result_rowcount;

	return input;
}
