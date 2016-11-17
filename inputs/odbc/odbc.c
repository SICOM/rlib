/*
 *  Copyright (C) 2003-2016 SICOM Systems, INC.
 *
 *  Authors: Bob Doan <bdoan@sicompos.com>
 *  Authors: Zoltan Boszormenyi <zboszor@dunaweb.hu>
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

#if NEED_WINDOWS_H
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
 
#include "rlib.h"
#include "rlib_input.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))

#define UNUSED __attribute__((unused))

struct odbc_field_values {
	gint len;
	gchar *value;
};

struct rlib_odbc_results {
	gchar *name;
	gint tot_fields;
	gint total_size;
	gboolean atstart;
	gboolean isdone;
	GList *data;
	GList *navigator;
	gchar **fields;
	struct odbc_field_values *values;
};

struct _private {
	SQLHENV V_OD_Env;
	SQLHDBC V_OD_hdbc;
};

static gint rlib_odbc_connect(gpointer input_ptr, const gchar *source, const gchar *user, const gchar *password) {
	struct input_filter *input = input_ptr;
	rlib *r = input->r;
	gint V_OD_erg;
	SQLINTEGER	V_OD_err;
	SQLSMALLINT	V_OD_mlen;
	SQLCHAR		V_OD_stat[10]; 
	SQLCHAR		V_OD_msg[200];

	V_OD_erg = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &INPUT_PRIVATE(input)->V_OD_Env);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		r_error(r, "Error AllocHandle\n");
		return -1;
	}

	V_OD_erg = SQLSetEnvAttr(INPUT_PRIVATE(input)->V_OD_Env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		r_error(r, "Error SetEnv\n");
		SQLFreeHandle(SQL_HANDLE_ENV, INPUT_PRIVATE(input)->V_OD_Env);
		return -1;
	}


	V_OD_erg = SQLAllocHandle(SQL_HANDLE_DBC, INPUT_PRIVATE(input)->V_OD_Env, &INPUT_PRIVATE(input)->V_OD_hdbc); 
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		r_error(r, "Error AllocHDB %d\n",V_OD_erg);
		SQLFreeHandle(SQL_HANDLE_ENV, INPUT_PRIVATE(input)->V_OD_Env);
		return -1;
	}

	SQLSetConnectAttr(INPUT_PRIVATE(input)->V_OD_hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);

	V_OD_erg = SQLConnect(INPUT_PRIVATE(input)->V_OD_hdbc, (SQLCHAR*) source, SQL_NTS, (SQLCHAR*) user, SQL_NTS, (SQLCHAR*) password, SQL_NTS);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		char szState[6];
		unsigned char buffer[256];

		SQLError(INPUT_PRIVATE(input)->V_OD_Env, INPUT_PRIVATE(input)->V_OD_hdbc, NULL, (SQLCHAR*)szState, NULL, buffer, 256, NULL);
		printf("SQLError = %s \n", buffer); 
		r_error(r, "Error SQLConnect %d [%s]\n",V_OD_erg, buffer);
		SQLGetDiagRec(SQL_HANDLE_DBC, INPUT_PRIVATE(input)->V_OD_hdbc,1, V_OD_stat, &V_OD_err,V_OD_msg,100,&V_OD_mlen);
		SQLFreeHandle(SQL_HANDLE_DBC, INPUT_PRIVATE(input)->V_OD_hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, INPUT_PRIVATE(input)->V_OD_Env);
		return -1;
	}

	return 0;
}

static void rlib_odbc_input_close(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	SQLFreeHandle(SQL_HANDLE_ENV, INPUT_PRIVATE(input)->V_OD_Env);
	SQLDisconnect(INPUT_PRIVATE(input)->V_OD_hdbc);
	INPUT_PRIVATE(input)->V_OD_Env = NULL;
}

static SQLHSTMT * rlib_odbc_query(gpointer input_ptr, gchar *query) {
	struct input_filter *input = input_ptr;
	rlib *r = input->r;
	SQLHSTMT V_OD_hstmt;
	SQLINTEGER	V_OD_err;
	SQLSMALLINT	V_OD_mlen;
	SQLCHAR		V_OD_stat[10]; 
	SQLCHAR		V_OD_msg[200];
	gint V_OD_erg;

	V_OD_erg=SQLAllocHandle(SQL_HANDLE_STMT, INPUT_PRIVATE(input)->V_OD_hdbc, &V_OD_hstmt);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		r_error(r, "Failed to allocate a connection handle %d\n",V_OD_erg);
		SQLGetDiagRec(SQL_HANDLE_DBC, INPUT_PRIVATE(input)->V_OD_hdbc,1, V_OD_stat,&V_OD_err,V_OD_msg,100,&V_OD_mlen);
		SQLDisconnect(INPUT_PRIVATE(input)->V_OD_hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,INPUT_PRIVATE(input)->V_OD_hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, INPUT_PRIVATE(input)->V_OD_Env);
		return NULL;
	}

	V_OD_erg=SQLExecDirect(V_OD_hstmt, (SQLCHAR *)query, SQL_NTS);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		r_error(r, "Error Select %d\n",V_OD_erg);
		SQLGetDiagRec(SQL_HANDLE_DBC, INPUT_PRIVATE(input)->V_OD_hdbc,1, V_OD_stat, &V_OD_err, V_OD_msg,100,&V_OD_mlen);
		SQLFreeHandle(SQL_HANDLE_STMT,V_OD_hstmt);
		SQLDisconnect(INPUT_PRIVATE(input)->V_OD_hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,INPUT_PRIVATE(input)->V_OD_hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV,INPUT_PRIVATE(input)-> V_OD_Env);
		return NULL;
	}

	
	return V_OD_hstmt;
}

static void rlib_odbc_load_from_navigator(struct rlib_odbc_results *result) {
	if(result->navigator != NULL) {
		GSList *data;
		gint i=0;
		for(data = result->navigator->data;data != NULL; data = data->next) {
			strcpy(result->values[i++].value, data->data);
		}
	}
}

static void rlib_odbc_start(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_odbc_results *result = result_ptr;

	if (result == NULL)
		return;

	result->atstart = TRUE;
	result->navigator = NULL;
	result->isdone = FALSE;
}

static gboolean rlib_odbc_next(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_odbc_results *result = result_ptr;

	if (result->atstart) {
		result->atstart = FALSE;
		result->navigator = g_list_first(result->data);
	} else
		result->navigator = g_list_next(result->navigator);
	rlib_odbc_load_from_navigator(result);

	result->isdone = (result->navigator == NULL);
	return !result->isdone;
}

static gboolean rlib_odbc_isdone(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_odbc_results *result = result_ptr;
	return result->isdone;
}

static gchar * rlib_odbc_get_field_value_as_string(gpointer input_ptr UNUSED, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_odbc_results *results = result_ptr;
	gint col = GPOINTER_TO_INT(field_ptr) - 1;

	if (results == NULL || results->atstart || results->isdone)
		return "";

	return results->values[col].value;
}

static gpointer rlib_odbc_resolve_field_pointer(gpointer input_ptr UNUSED, gpointer result_ptr, gchar *name) {
	struct rlib_odbc_results *results = result_ptr;
	gint i;

	if (result_ptr == NULL)
		return NULL;

	for (i = 0; i < results->tot_fields; i++) {
		if (!strcmp(results->fields[i], name)) {
			return GINT_TO_POINTER(i + 1);
		}
	}
	return NULL;
}

gpointer odbc_new_result_from_query(gpointer input_ptr, gpointer query_ptr) {
	struct rlib_odbc_results *results;
	struct rlib_query *query = query_ptr;
	SQLHSTMT V_OD_hstmt;
	SQLSMALLINT i, ncols;
	gint V_OD_erg;
	SQLULEN col_size;
	SQLLEN ind;

	V_OD_hstmt = rlib_odbc_query(input_ptr, query->sql);
	if(V_OD_hstmt == NULL)
		return NULL;

	results = g_malloc(sizeof(struct rlib_odbc_results));
	if (!results)
		return NULL;

	SQLNumResultCols(V_OD_hstmt, &ncols);

	results->fields = g_new(gchar *, ncols);
	results->values = g_new(struct odbc_field_values, ncols);

	results->total_size = 0;
	for (i = 0; i < ncols; i++) {
		SQLCHAR name[256];
		SQLSMALLINT name_length;
		V_OD_erg = SQLDescribeCol( V_OD_hstmt, i + 1, name, sizeof(name), &name_length, NULL, &col_size, NULL, NULL);
		if (!SQL_SUCCEEDED(V_OD_erg)) {
			name[0] = '\0';
			col_size = 0;
		}
		results->fields[i] = g_strdup((char *)name);

		results->values[i].len = col_size;
		results->values[i].value = g_malloc(col_size+1);
		results->total_size += col_size + 1;
	}
	
	results->tot_fields = ncols;
	results->data = NULL;
	
	results->data = NULL;
	if (SQL_SUCCEEDED((V_OD_erg = SQLFetchScroll(V_OD_hstmt, SQL_FETCH_NEXT, 0)))) {
		do {
			GSList *row_data = NULL;
			int i;
			for (i = 0; i < results->tot_fields; i++) {
				V_OD_erg = SQLGetData(V_OD_hstmt, i + 1, SQL_C_CHAR, results->values[i].value, results->values[i].len + 1, &ind);
				if (SQL_SUCCEEDED(V_OD_erg) && ind != SQL_NULL_DATA) {
					if (results->values[i].value[0])
						row_data = g_slist_append(row_data, g_strdup(results->values[i].value));
					else
						row_data = g_slist_append(row_data, "");
				} else
					row_data = g_slist_append(row_data, "");
			}

			results->data = g_list_append(results->data, row_data);

		} while(SQL_SUCCEEDED((V_OD_erg = SQLFetchScroll(V_OD_hstmt, SQL_FETCH_NEXT, 0))));
	}

	SQLFreeHandle(SQL_HANDLE_STMT, V_OD_hstmt);

	return results;
}

static gint rlib_odbc_num_fields(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_odbc_results *result = result_ptr;

	if (result == NULL)
		return 0;

	return result->tot_fields;
}


static void rlib_odbc_rlib_free_result(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_odbc_results *results = result_ptr;
	gint i;
	GList *list;

	for (list = results->data; list != NULL; list = list->next) {
		GSList *data;
		for (data = list->data; data != NULL; data = data->next) {
			gchar *value = data->data;
			if (value[0])
				g_free(value);
		}
		g_slist_free(list->data);
	}
	g_list_free(results->data);
	
	for (i = 0; i < results->tot_fields;i++) {
		g_free(results->fields[i]);
		g_free(results->values[i].value);
	}
	g_free(results->fields);
	g_free(results->values);
	g_free(results);
}

static void rlib_odbc_free_input_filter(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	SQLDisconnect(INPUT_PRIVATE(input)->V_OD_hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC,INPUT_PRIVATE(input)->V_OD_hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV,INPUT_PRIVATE(input)-> V_OD_Env);
	g_free(input->private);
	g_free(input);
}

static const gchar * rlib_odbc_get_error(gpointer input_ptr UNUSED) {
	return "No error information";
}

#ifdef HAVE_ODBC_BUILTIN
gpointer rlib_odbc_new_input_filter(rlib *r) {
#else
DLL_EXPORT_SYM gpointer new_input_filter(rlib *r) {
#endif
	struct input_filter *input;
	input = g_malloc0(sizeof(struct input_filter));
	input->private = g_malloc0(sizeof(struct _private));
	input->r = r;
	input->connect_local_with_credentials = rlib_odbc_connect;
	input->input_close = rlib_odbc_input_close;
	input->num_fields = rlib_odbc_num_fields;
	input->start = rlib_odbc_start;
	input->next = rlib_odbc_next;
	input->isdone = rlib_odbc_isdone;
	input->new_result_from_query = odbc_new_result_from_query;
	input->get_field_value_as_string = rlib_odbc_get_field_value_as_string;
	input->get_error = rlib_odbc_get_error;

	input->resolve_field_pointer = rlib_odbc_resolve_field_pointer;

	input->free = rlib_odbc_free_input_filter;
	input->free_result = rlib_odbc_rlib_free_result;

	return input;
}
