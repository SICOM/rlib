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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mysql.h>
#include <glib.h>

#include "util.h"
#include "rlib_input.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))

struct rlib_mysql_results {
	MYSQL_RES *result;
	gchar *name;
	MYSQL_ROW this_row;
	MYSQL_ROW previous_row;
	MYSQL_ROW save_row;
	MYSQL_ROW last_row;
	gint isdone;
	gint didprevious;
	gint *fields;
};

struct _private {
	MYSQL *mysql;
};

gpointer rlib_mysql_real_connect(gpointer input_ptr, gchar *group, gchar *host, gchar *user, gchar *password, gchar *database) {
	struct input_filter *input = input_ptr;
	MYSQL *mysql;
	gchar *host_copy = NULL;
	int port = 3306;

	mysql = mysql_init(NULL);

	if(mysql == NULL)
		return NULL;

	if(group != NULL) {
		if (mysql_options(mysql,MYSQL_READ_DEFAULT_GROUP,group)) {
			mysql_close(mysql);
			return NULL;
		}
	} else if (host) {
		char *tmp, *port_s;
		host_copy = g_strdup(host);
		tmp = strchr(host_copy, ':');
		if (tmp) {
			*tmp = '\0';
			port_s = tmp + 1;
			port = atoi(port_s);
		}
	}

	if (mysql_real_connect(mysql,
		group == NULL ? host_copy : mysql->options.host,
		group == NULL ? user : mysql->options.user,
		group == NULL ? password : mysql->options.password,
		group == NULL ? database : mysql->options.db,
		group == NULL ? port : mysql->options.port,
		group == NULL ? NULL : mysql->options.unix_socket,
		0
	   ) == NULL) {
		mysql_close(mysql);
		return NULL;
	}

	g_free(host_copy);

	mysql_select_db(mysql,database);

	INPUT_PRIVATE(input)->mysql = mysql;
	return mysql;
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
	rtn = mysql_query(mysql, query);
	if(rtn == 0) {
		result = mysql_store_result(mysql);
		return result;
	}
	return NULL;
}

static gint rlib_mysql_first(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_mysql_results *result = result_ptr;
	if (result) {
		mysql_data_seek(result->result,0);
		result->this_row = mysql_fetch_row(result->result);
		result->previous_row = NULL;
		result->last_row = NULL;
		result->didprevious = FALSE;
		result->isdone = FALSE;
		return result->this_row != NULL ? TRUE : FALSE;
	}
	return FALSE;
}

static gint rlib_mysql_next(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_mysql_results *result = result_ptr;
	MYSQL_ROW row;

	if (!result)
		return FALSE;
	
	if(result->didprevious == TRUE) {
		result->didprevious = FALSE;
		result->this_row = result->save_row;
		return TRUE;
	} else {
		row = mysql_fetch_row(result->result);
		if(row != NULL) {
			result->previous_row = result->this_row;
			result->this_row = row;
			result->isdone = FALSE;
			return TRUE;
		} else {
			result->previous_row = result->this_row;
			result->last_row = result->this_row;
			result->isdone = TRUE;
			return FALSE;
		}
	}
}

static gint rlib_mysql_isdone(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_mysql_results *result = result_ptr;
	if (result)
		return result->isdone;
	else
		return TRUE;
}

static gint rlib_mysql_previous(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_mysql_results *result = result_ptr;
	if (result) {
		result->save_row = result->this_row;
		result->this_row = result->previous_row;
		if(result->previous_row == NULL) {
			result->didprevious = FALSE;
			return FALSE;
		} else {
			result->didprevious = TRUE;
			return TRUE;
		}
	}
	return FALSE;
}

static gint rlib_mysql_last(gpointer input_ptr, gpointer result_ptr) {
	return TRUE;
}

static gchar * rlib_mysql_get_field_value_as_string(gpointer input_ptr, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_mysql_results *result = result_ptr;
	gint field = GPOINTER_TO_INT(field_ptr);
	if(result_ptr == NULL)
		return (gchar *)"";
	field -= 1;
	if(result->this_row == NULL)
		return (gchar *)"";
	return result->this_row[field];
}

static gpointer rlib_mysql_resolve_field_pointer(gpointer input_ptr, gpointer result_ptr, gchar *name) {
	struct rlib_mysql_results *results = result_ptr;
	gint x=0;
	MYSQL_FIELD *field;
	if (results) {
		mysql_field_seek(results->result, 0);
		while((field = mysql_fetch_field(results->result))) {
			if(!strcmp(field->name, name)) {
				return GINT_TO_POINTER(results->fields[x]);
			}
			x++;
		}
	}
	return NULL;
}

void * mysql_new_result_from_query(gpointer input_ptr, gchar *query) {
	struct input_filter *input = input_ptr;
	struct rlib_mysql_results *results;
	MYSQL_RES *result;
	guint count,i;
	result = rlib_mysql_query(INPUT_PRIVATE(input)->mysql, query);
	if(result == NULL)
		return NULL;
	else {
		results = g_malloc(sizeof(struct rlib_mysql_results));
		results->result = result;
	}
	count = mysql_field_count(INPUT_PRIVATE(input)->mysql);
	results->fields = g_malloc(sizeof(gint) * count);
	for(i=0;i<count;i++) {
		results->fields[i] = i+1;
	}
	return results;
}

static void rlib_mysql_rlib_free_result(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_mysql_results *results = result_ptr;
	if (results) {
		mysql_free_result(results->result);
		g_free(results->fields);
		g_free(results);
	}
}

static gint rlib_mysql_free_input_filter(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	g_free(input->private);
	g_free(input);
	return 0;
}

static const gchar* rlib_mysql_get_error(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	return mysql_error(INPUT_PRIVATE(input)->mysql);
}

gpointer rlib_mysql_new_input_filter(void) {
	struct input_filter *input;

	input = g_malloc(sizeof(struct input_filter));
	input->private = g_malloc(sizeof(struct _private));
	memset(input->private, 0, sizeof(struct _private));
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
	return input;
}
