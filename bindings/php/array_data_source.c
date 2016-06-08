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
#include <config.h>

#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <php.h>
 
#include "rlib.h"
#include "rlib_input.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))

#define UNUSED __attribute__((unused))

struct rlib_php_array_results {
	char *array_name;
	zval *zend_value;
	int cols;
	int rows;
	int isdone;
	char **data;
	int current_row;
};

struct _private {
	int	dummy;
};

static gint rlib_php_array_input_close(gpointer input_ptr UNUSED) {
	return TRUE;
}

static const gchar* rlib_php_array_get_error(gpointer input_ptr UNUSED) {
	return "Hard to make a mistake here.. try checking your names/spellings";
}

static gint rlib_php_array_first(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_php_array_results *result = result_ptr;

	if(result_ptr == NULL) {
		return FALSE;	
	}

	result->current_row = 1;
	result->isdone = FALSE;
	if(result->rows <= 1) {
		result->isdone = TRUE;
		return FALSE;
	}	
	return TRUE;
}

static gint rlib_php_array_next(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_php_array_results *result = result_ptr;
	result->current_row++;
	result->isdone = FALSE;
	if(result->current_row < result->rows)
		return TRUE;
	result->isdone = TRUE;
	return FALSE;
}

static gint rlib_php_array_isdone(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_php_array_results *result = result_ptr;

	if(result == NULL)
		return TRUE;

	return result->isdone;
}

static gint rlib_php_array_previous(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_php_array_results *result = result_ptr;
	result->current_row--;
	result->isdone = FALSE;
	if(result->current_row >= 1)
		return TRUE;
	else
		return FALSE;
	return TRUE;
}

static gint rlib_php_array_last(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_php_array_results *result = result_ptr;
	result->current_row = result->rows-1;
	return TRUE;
}

static gchar * rlib_php_array_get_field_value_as_string(gpointer input_ptr UNUSED, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_php_array_results *result = result_ptr;
	int which_field = GPOINTER_TO_INT(field_ptr) - 1;
	if(result->rows <= 1 || result->current_row >= result->rows)
		return "";

	return result->data[(result->current_row*result->cols)+which_field];
}

static gpointer rlib_php_array_resolve_field_pointer(gpointer input_ptr UNUSED, gpointer result_ptr, gchar *name) {
	struct rlib_php_array_results *result = result_ptr;
	int i;

	if(result_ptr == NULL)
		return NULL;

	for(i=0;i<result->cols;i++) {
		if(strcmp(name, result->data[i]) == 0) {
			i++;
			return GINT_TO_POINTER(i);
		}
	}
	return NULL;
}

static void *php_array_new_result_from_query(gpointer input_ptr UNUSED, gpointer query_ptr) {
	struct rlib_query *query = query_ptr;
	struct rlib_php_array_results *result = emalloc(sizeof(struct rlib_php_array_results));
	long size;
	void *data, *lookup_data;
	char *data_result;
	char dstr[64];
	HashTable *ht1, *ht2;
	HashPosition pos1, pos2;
	zval *zend_value, *lookup_value;
	int row = 0, col = 0;
	int total_size;
	TSRMLS_FETCH();
	
	memset(result, 0, sizeof(struct rlib_php_array_results));
	result->array_name = query->sql;

	if ((size = zend_hash_find(&EG(symbol_table), query->sql, strlen(query->sql) + 1, &data)) == FAILURE) {
		return NULL;
	} else {
		result->zend_value = *(zval **)data;
	}

	ht1 = result->zend_value->value.ht;
	result->rows = ht1->nNumOfElements;
	zend_hash_internal_pointer_reset_ex(ht1, &pos1);
	zend_hash_get_current_data_ex(ht1, &data, &pos1);
	zend_value = *(zval **)data;
	
	ht2 = zend_value->value.ht;
	result->cols = ht2->nNumOfElements;
	
	total_size = result->rows*result->cols*sizeof(char *);
	result->data = emalloc(total_size);

	if(result->cols <= 0)
		return NULL;

	zend_hash_internal_pointer_reset_ex(ht1, &pos1);
	while(1) {
		zend_hash_get_current_data_ex(ht1, &data, &pos1);
		zend_value = *(zval **)data;
		ht2 = zend_value->value.ht;
		zend_hash_internal_pointer_reset_ex(ht2, &pos2);
		col=0;
		while(1) {
			int lookup_result = zend_hash_get_current_data_ex(ht2, &lookup_data, &pos2);
			if(lookup_result < 0) {
				result->data[(row*result->cols)+col] = estrdup("RLIB: INVALID ARRAY CELL");
				col++;
				if(col >= result->cols) {
					break;
				}
			
			} else {

				lookup_value = *(zval **)lookup_data;
				zend_hash_move_forward_ex(ht2, &pos2);

				data_result = NULL;
				memset(dstr,0,64);
				if( Z_TYPE_P(lookup_value) == IS_STRING )	
					data_result = Z_STRVAL_P(lookup_value);
				else if( Z_TYPE_P(lookup_value) == IS_LONG ) {	
					sprintf(dstr,"%ld",Z_LVAL_P(lookup_value));
					data_result = estrdup(dstr);
				} else if( Z_TYPE_P(lookup_value) == IS_DOUBLE ) {	
					sprintf(dstr,"%f",Z_DVAL_P(lookup_value));
					data_result = estrdup(dstr);
				} else if( Z_TYPE_P(lookup_value) == IS_NULL ) {	
					data_result = estrdup("");
				} else {
					sprintf(dstr,"ZEND Z_TYPE %d NOT SUPPORTED",Z_TYPE_P(lookup_value));
					data_result = estrdup(dstr);
				}

				result->data[(row*result->cols)+col] = data_result;
				col++;
				if(col >= result->cols) {
					break;
				}
			}
		}
		row++;
		if(row >= result->rows) {
			break;
		}
		zend_hash_move_forward_ex(ht1, &pos1);
	}
	
	return result;
}

static gint rlib_php_array_free_input_filter(gpointer input_ptr UNUSED) {
	return 0;
}

static void rlib_php_array_rlib_free_result(gpointer input_ptr UNUSED, gpointer result_ptr UNUSED) {
}


static gpointer rlib_php_array_new_input_filter() {
	struct input_filter *input;
	
	input = emalloc(sizeof(struct input_filter));
	memset(input, 0, sizeof(struct input_filter));
	input->private = emalloc(sizeof(struct _private));
	memset(input->private, 0, sizeof(struct _private));
	input->input_close = rlib_php_array_input_close;
	input->first = rlib_php_array_first;
	input->next = rlib_php_array_next;
	input->previous = rlib_php_array_previous;
	input->last = rlib_php_array_last;
	input->get_error = rlib_php_array_get_error;
	input->isdone = rlib_php_array_isdone;
	input->new_result_from_query = php_array_new_result_from_query;
	input->get_field_value_as_string = rlib_php_array_get_field_value_as_string;

	input->resolve_field_pointer = rlib_php_array_resolve_field_pointer;

	input->free = rlib_php_array_free_input_filter;
	input->free_result = rlib_php_array_rlib_free_result;
	return input;
}

gint rlib_add_datasource_php_array(void *r, gchar *input_name) {
	rlib_add_datasource(r, input_name, rlib_php_array_new_input_filter());
	return TRUE;
}
