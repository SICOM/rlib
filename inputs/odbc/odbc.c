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

#include "config.h"
 
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

struct odbc_fields {
	gint col;
	gchar name[512];
};

struct odbc_field_values {
	gint len;
	gchar *value;
};

struct rlib_odbc_results {
	gchar *name;
	SQLHSTMT V_OD_hstmt;
	gint tot_fields;
	gint total_size;
	gint isdone;
	gboolean forward_only;
	GList *data;
	GList *navigator;
	struct odbc_fields *fields;
	struct odbc_field_values *values;
};

struct _private {
	SQLHENV V_OD_Env;
	SQLHDBC V_OD_hdbc;
};

gpointer rlib_odbc_connect(gpointer input_ptr, gchar *source, gchar *user, gchar *password) {
	struct input_filter *input = input_ptr;
	gint V_OD_erg;
	SQLINTEGER	V_OD_err;
	SQLSMALLINT	V_OD_mlen;
	SQLCHAR		V_OD_stat[10]; 
	SQLCHAR		V_OD_msg[200];

	V_OD_erg=SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&INPUT_PRIVATE(input)->V_OD_Env);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		fprintf(stderr, "Error AllocHandle\n");
		return NULL;
	}

	V_OD_erg=SQLSetEnvAttr(INPUT_PRIVATE(input)->V_OD_Env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		fprintf(stderr, "Error SetEnv\n");
		SQLFreeHandle(SQL_HANDLE_ENV, INPUT_PRIVATE(input)->V_OD_Env);
		return NULL;
	}


	V_OD_erg = SQLAllocHandle(SQL_HANDLE_DBC, INPUT_PRIVATE(input)->V_OD_Env, &INPUT_PRIVATE(input)->V_OD_hdbc); 
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		fprintf(stderr, "Error AllocHDB %d\n",V_OD_erg);
		SQLFreeHandle(SQL_HANDLE_ENV, INPUT_PRIVATE(input)->V_OD_Env);
		return NULL;
	}

	SQLSetConnectAttr(INPUT_PRIVATE(input)->V_OD_hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);

	V_OD_erg = SQLConnect(INPUT_PRIVATE(input)->V_OD_hdbc, (SQLCHAR*) source, SQL_NTS, (SQLCHAR*) user, SQL_NTS, (SQLCHAR*) password, SQL_NTS);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		char szState[6];
		unsigned char buffer[256];
      SQLError(INPUT_PRIVATE(input)->V_OD_Env, INPUT_PRIVATE(input)->V_OD_hdbc, NULL, (SQLCHAR*)szState, NULL, buffer, 256, NULL);
      printf("SQLError = %s \n", buffer); 
		fprintf(stderr, "Error SQLConnect %d [%s]\n",V_OD_erg, buffer);
		SQLGetDiagRec(SQL_HANDLE_DBC, INPUT_PRIVATE(input)->V_OD_hdbc,1, V_OD_stat, &V_OD_err,V_OD_msg,100,&V_OD_mlen);
		SQLFreeHandle(SQL_HANDLE_DBC, INPUT_PRIVATE(input)->V_OD_hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, INPUT_PRIVATE(input)->V_OD_Env);
		return NULL;
	}

	return  INPUT_PRIVATE(input)->V_OD_Env;
}

static gint rlib_odbc_input_close(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	SQLFreeHandle(SQL_HANDLE_ENV, INPUT_PRIVATE(input)->V_OD_Env);
	SQLDisconnect(INPUT_PRIVATE(input)->V_OD_hdbc);
	INPUT_PRIVATE(input)->V_OD_Env = NULL;
	return 0;
}

static SQLHSTMT * rlib_odbc_query(gpointer input_ptr, gchar *query) {
	struct input_filter *input = input_ptr;
	SQLHSTMT V_OD_hstmt;
	SQLINTEGER	V_OD_err;
	SQLSMALLINT	V_OD_mlen;
	SQLCHAR		V_OD_stat[10]; 
	SQLCHAR		V_OD_msg[200];
	gint V_OD_erg;

	V_OD_erg=SQLAllocHandle(SQL_HANDLE_STMT, INPUT_PRIVATE(input)->V_OD_hdbc, &V_OD_hstmt);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		fprintf(stderr, "Failed to allocate a connection handle %d\n",V_OD_erg);
		SQLGetDiagRec(SQL_HANDLE_DBC, INPUT_PRIVATE(input)->V_OD_hdbc,1, V_OD_stat,&V_OD_err,V_OD_msg,100,&V_OD_mlen);
		SQLDisconnect(INPUT_PRIVATE(input)->V_OD_hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,INPUT_PRIVATE(input)->V_OD_hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, INPUT_PRIVATE(input)->V_OD_Env);
		return NULL;
	}

	V_OD_erg=SQLExecDirect(V_OD_hstmt, (SQLCHAR *)query, SQL_NTS);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
		fprintf(stderr, "Error Select %d\n",V_OD_erg);
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

static gint odbc_read_first(gpointer result_ptr) {
	struct rlib_odbc_results *results = result_ptr;
	gint V_OD_erg;
	gint i;
	SQLLEN ind;
	if(SQL_SUCCEEDED(( V_OD_erg = SQLFetchScroll (results->V_OD_hstmt, SQL_FETCH_FIRST, 0)))) {
		for (i=0;i<results->tot_fields;i++)
			if (!SQL_SUCCEEDED(( V_OD_erg = SQLGetData(results->V_OD_hstmt, i + 1, SQL_C_CHAR, results->values[i].value, results->values[i].len + 1, &ind))) || ind == SQL_NULL_DATA)
				results->values[i].value = '\0';
		return TRUE;
	}
	return FALSE;
}


static gint rlib_odbc_first(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_odbc_results *result = result_ptr;

	if(result_ptr == NULL)
		return FALSE;

	if(result->forward_only == TRUE) {
		result->navigator = g_list_first(result->data);
		result->isdone = FALSE;
		rlib_odbc_load_from_navigator(result);	
		if(result->navigator == NULL)
			return FALSE;
		else
			return TRUE;
	} else {
		result->isdone = FALSE;
		return odbc_read_first(result_ptr);
	}
}

static gint odbc_read_next(gpointer result_ptr) {
	struct rlib_odbc_results *results = result_ptr;
	gint V_OD_erg;
	gint i;
	SQLLEN ind;
	if(SQL_SUCCEEDED(( V_OD_erg = SQLFetchScroll (results->V_OD_hstmt, SQL_FETCH_NEXT, 0)))) {
		for (i=0;i<results->tot_fields;i++)
			if (!SQL_SUCCEEDED(( V_OD_erg = SQLGetData(results->V_OD_hstmt, i + 1, SQL_C_CHAR, results->values[i].value, results->values[i].len + 1, &ind))) || ind == SQL_NULL_DATA)
				results->values[i].value = '\0';
		return TRUE;
	}
	return FALSE;
}

static gint rlib_odbc_next(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_odbc_results *result = result_ptr;

	if(result->forward_only == TRUE) {
		result->navigator = g_list_next(result->navigator);
		rlib_odbc_load_from_navigator(result);	
		if(result->navigator == NULL) {
			result->isdone = TRUE;
			return FALSE;
		} else {
			result->isdone = FALSE;
			return TRUE;
		}
	} else {
		gint ret;
		ret = odbc_read_next(result_ptr);
		result->isdone = !ret;
		return ret;
	}
}

static gint rlib_odbc_isdone(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_odbc_results *result = result_ptr;
	return result->isdone;
}

static gint odbc_read_prior(gpointer result_ptr) {
	struct rlib_odbc_results *results = result_ptr;
	gint V_OD_erg;
	gint i;
	SQLLEN ind;
	if(SQL_SUCCEEDED(( V_OD_erg = SQLFetchScroll (results->V_OD_hstmt, SQL_FETCH_PRIOR, 0)))) {
		for (i=0;i<results->tot_fields;i++)
			if (!SQL_SUCCEEDED(( V_OD_erg = SQLGetData (results->V_OD_hstmt, i + 1, SQL_C_CHAR, results->values[i].value, results->values[i].len + 1, &ind))) || ind == SQL_NULL_DATA)
				results->values[i].value = '\0';
		return TRUE;
	}
	return FALSE;
}

static gint rlib_odbc_previous(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_odbc_results *result = result_ptr;
	if(result->forward_only == TRUE) {
		result->navigator = g_list_previous(result->navigator);
		rlib_odbc_load_from_navigator(result);	
		if(result->navigator == NULL) {
			result->isdone = TRUE;
			return FALSE;
		} else {
			result->isdone = FALSE;
			return TRUE;
		}
	} else {
		gint ret;
		ret = odbc_read_prior(result_ptr);
		result->isdone = !ret;
		return ret;
	}
}

static gint odbc_read_last(gpointer result_ptr) {
	struct rlib_odbc_results *results = result_ptr;
	gint V_OD_erg;
	gint i;
	SQLLEN ind;
	if(SQL_SUCCEEDED(( V_OD_erg = SQLFetchScroll (results->V_OD_hstmt, SQL_FETCH_LAST, 0)))) {
		for (i=0;i<results->tot_fields;i++)
			if (!SQL_SUCCEEDED(( V_OD_erg = SQLGetData(results->V_OD_hstmt, i + 1, SQL_C_CHAR, results->values[i].value, results->values[i].len + 1, &ind))) || ind == SQL_NULL_DATA)
				results->values[i].value = '\0';
		return TRUE;
	}
	return FALSE;
}

static gint rlib_odbc_last(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_odbc_results *result = result_ptr;

	if(result->forward_only == TRUE) {
		result->navigator = g_list_last(result->navigator);
		rlib_odbc_load_from_navigator(result);	
		result->isdone = TRUE;
		if(result->navigator == NULL) {
			return FALSE;
		} else {
			return TRUE;
		}
	} else {
		gint ret;
		ret = odbc_read_last(result_ptr);
		result->isdone = TRUE;
		return ret;
	}
}

static gchar * rlib_odbc_get_field_value_as_string(gpointer input_ptr, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_odbc_results *results = result_ptr;
	struct odbc_fields *the_field = field_ptr;
	gint field = the_field->col;
	return results->values[field].value;
}

static gpointer rlib_odbc_resolve_field_pointer(gpointer input_ptr, gpointer result_ptr, gchar *name) {
	struct rlib_odbc_results *results = result_ptr;
	gint i=0;

	if(result_ptr == NULL)
		return NULL;

	for (i = 0; i < results->tot_fields; i++) {
		if(!strcmp(results->fields[i].name, name)) {
			return &results->fields[i];
		}
	}
	return NULL;
}

gpointer odbc_new_result_from_query(gpointer input_ptr, gchar *query) {
	struct rlib_odbc_results *results;
	struct input_filter *input = input_ptr;
	SQLHSTMT V_OD_hstmt;
	guint i;
	SQLSMALLINT ncols;
	gint V_OD_erg;
	SQLULEN col_size;
	SQLUINTEGER fFuncs;
	SQLLEN ind;


	V_OD_hstmt = rlib_odbc_query(input_ptr, query);
	if(V_OD_hstmt == NULL)
		return NULL;
	else {
		results = g_malloc(sizeof(struct rlib_odbc_results));
		results->V_OD_hstmt = V_OD_hstmt;
	}

	SQLGetInfo(INPUT_PRIVATE(input)->V_OD_hdbc,SQL_SCROLL_OPTIONS,(SQLPOINTER)&fFuncs,sizeof(fFuncs),NULL);

	if(fFuncs & SQL_SO_FORWARD_ONLY) 
		results->forward_only = TRUE;
	else
		results->forward_only = FALSE;

	SQLNumResultCols (V_OD_hstmt, &ncols);

	results->fields = g_malloc(sizeof(struct odbc_fields) * ncols);
	results->values = g_malloc(sizeof(struct odbc_field_values) * ncols);

	results->total_size = 0;
	for(i=0;i<ncols;i++) {
		SQLCHAR name[ 256 ];
		SQLSMALLINT name_length;
		V_OD_erg = SQLDescribeCol( V_OD_hstmt, i+1,	name, sizeof( name ), &name_length, NULL, &col_size, NULL, NULL );
		results->fields[i].col = i;
		if (SQL_SUCCEEDED(V_OD_erg)) {
			strcpy(results->fields[i].name, (char *)name);
		} else {
			strcpy(results->fields[i].name, "");
			col_size = 0;
		}
		results->values[i].len = col_size;
		results->values[i].value = g_malloc(col_size+1);
		results->total_size += col_size+1;
	}
	
	results->tot_fields = ncols;
	results->data = NULL;
	
	if(results->forward_only) {
		int i;
		results->data = NULL;
		if(SQL_SUCCEEDED(( V_OD_erg = SQLFetchScroll (results->V_OD_hstmt, SQL_FETCH_NEXT, 0)))) {
			do {
				GSList *row_data = NULL;
				for (i=0;i<results->tot_fields;i++) {
					V_OD_erg = SQLGetData (results->V_OD_hstmt, i+1, SQL_C_CHAR, results->values[i].value, results->values[i].len+1, &ind);
					if (SQL_SUCCEEDED(V_OD_erg) && ind != SQL_NULL_DATA)
						row_data = g_slist_append(row_data, g_strdup(results->values[i].value));
					else
						row_data = g_slist_append(row_data, "");
				}

				results->data = g_list_append(results->data, row_data);

			} while(SQL_SUCCEEDED(( V_OD_erg = SQLFetchScroll (results->V_OD_hstmt, SQL_FETCH_NEXT, 0))));
		}
	}
	
	return results;
}

static void rlib_odbc_rlib_free_result(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_odbc_results *results = result_ptr;
	gint i;
	
	if(results->forward_only == TRUE && results->data != NULL) {
		GList *list;
		for(list=results->data;list != NULL; list=list->next) {
			GSList *data;
			for(data = list->data; data != NULL; data = data->next)
				g_free(data->data);
			g_slist_free(list->data);
		}	
		g_list_free(results->data);
	}
	
	SQLFreeHandle(SQL_HANDLE_STMT,results->V_OD_hstmt);
	for(i=0;i<results->tot_fields;i++) {
		g_free(results->values[i].value);
	}
	g_free(results->fields);
	g_free(results->values);
	g_free(results);
}

static gint rlib_odbc_free_input_filter(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	SQLDisconnect(INPUT_PRIVATE(input)->V_OD_hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC,INPUT_PRIVATE(input)->V_OD_hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV,INPUT_PRIVATE(input)-> V_OD_Env);
	g_free(input->private);
	g_free(input);
	return 0;
}

static const gchar * rlib_odbc_get_error(gpointer input_ptr) {
	return "No error information";
}

gpointer rlib_odbc_new_input_filter(void) {
	struct input_filter *input;
	input = g_malloc(sizeof(struct input_filter));
	input->private = g_malloc(sizeof(struct _private));
	memset(input->private, 0, sizeof(struct _private));
	input->input_close = rlib_odbc_input_close;
	input->first = rlib_odbc_first;
	input->next = rlib_odbc_next;
	input->previous = rlib_odbc_previous;
	input->last = rlib_odbc_last;
	input->isdone = rlib_odbc_isdone;
	input->new_result_from_query = odbc_new_result_from_query;
	input->get_field_value_as_string = rlib_odbc_get_field_value_as_string;
	input->get_error = rlib_odbc_get_error;

	input->resolve_field_pointer = rlib_odbc_resolve_field_pointer;

	input->free = rlib_odbc_free_input_filter;
	input->free_result = rlib_odbc_rlib_free_result;
	return input;
}
