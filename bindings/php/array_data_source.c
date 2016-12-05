/*
 *  Copyright (C) 2003-2016 SICOM Systems, INC.
 *
 *  Authors: Bob Doan <bdoan@sicompos.com>
 *  Updated for PHP 7: Zoltán Böszörményi <zboszormenyi@sicom.com>
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
#include <php.h>
#include <Zend/zend.h>

#include "rlib.h"
#include "rlib_input.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))

#define UNUSED __attribute__((unused))

struct rlib_php_array_results {
	char *array_name;
	zval *zend_value;
	int cols;
	int rows;
	int atstart;
	int isdone;
	char **data;
	int current_row;
};

struct _private {
	int	dummy;
};

static void rlib_php_array_input_close(gpointer input_ptr UNUSED) {}

static const gchar* rlib_php_array_get_error(gpointer input_ptr UNUSED) {
	return "Hard to make a mistake here.. try checking your names/spellings";
}

static void rlib_php_array_start(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_php_array_results *result = result_ptr;

	if (result == NULL)
		return;

	result->current_row = 0;
	result->atstart = TRUE;
	result->isdone = FALSE;
}

static gint rlib_php_array_next(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_php_array_results *result = result_ptr;

	if (result == NULL)
		return FALSE;

	result->current_row++;
	result->atstart = FALSE;
	result->isdone = (result->current_row >= result->rows);
	return !result->isdone;
}

static gint rlib_php_array_isdone(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_php_array_results *result = result_ptr;

	if (result == NULL)
		return TRUE;

	return result->isdone;
}

static gchar * rlib_php_array_get_field_value_as_string(gpointer input_ptr UNUSED, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_php_array_results *result = result_ptr;
	int which_field;

	if (result == NULL || result->atstart || result->isdone)
		return "";

	which_field = GPOINTER_TO_INT(field_ptr) - 1;

	if (result->rows <= 1 || result->current_row >= result->rows)
		return "";

	return result->data[result->current_row * result->cols + which_field];
}

static gpointer rlib_php_array_resolve_field_pointer(gpointer input_ptr UNUSED, gpointer result_ptr, gchar *name) {
	struct rlib_php_array_results *result = result_ptr;
	int i;

	if (result == NULL)
		return NULL;

	for (i = 0; i < result->cols; i++) {
		if (strcmp(name, result->data[i]) == 0) {
			return GINT_TO_POINTER(i + 1);
		}
	}
	return NULL;
}

static gchar *rlib_php_array_get_field_name(gpointer input_ptr UNUSED, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_php_array_results *result = result_ptr;
	int field = GPOINTER_TO_INT(field_ptr) - 1;

	if (result == NULL)
		return NULL;

	return result->data[field];
}

static void *php_array_new_result_from_query(gpointer input_ptr UNUSED, gpointer query_ptr) {
	struct rlib_query *query = query_ptr;
	struct rlib_php_array_results *result = emalloc(sizeof(struct rlib_php_array_results));
#if PHP_MAJOR_VERSION < 7
	void *data, *lookup_data;
#endif
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

#if PHP_MAJOR_VERSION < 7
	if (zend_hash_find(&EG(symbol_table), query->sql, strlen(query->sql) + 1, &data) == FAILURE) {
		efree(result);
		return NULL;
	}
	result->zend_value = *(zval **)data;
#else
	result->zend_value = zend_hash_str_find(&EG(symbol_table), query->sql, strlen(query->sql));
	if (result->zend_value == NULL) {
		efree(result);
		return NULL;
	}

	if (EXPECTED(Z_TYPE_P(result->zend_value) == IS_INDIRECT))
		result->zend_value = Z_INDIRECT_P(result->zend_value);
#endif

	if (UNEXPECTED(Z_TYPE_P(result->zend_value) != IS_ARRAY)) {
		efree(result);
		return NULL;
	}

	ht1 = Z_ARRVAL_P(result->zend_value);
	result->rows = zend_hash_num_elements(ht1);
	zend_hash_internal_pointer_reset_ex(ht1, &pos1);
#if PHP_MAJOR_VERSION < 7
	zend_hash_get_current_data_ex(ht1, &data, &pos1);
	zend_value = *(zval **)data;
#else
	zend_value = zend_hash_get_current_data_ex(ht1, &pos1);
#endif

	ht2 = Z_ARRVAL_P(zend_value);
	result->cols = zend_hash_num_elements(ht2);

	total_size = result->rows * result->cols * sizeof(char *);
	result->data = emalloc(total_size);

	if (result->cols <= 0)
		return NULL;

	zend_hash_internal_pointer_reset_ex(ht1, &pos1);
	while (row < result->rows) {
#if PHP_MAJOR_VERSION < 7
		zend_hash_get_current_data_ex(ht1, &data, &pos1);
		zend_value = *(zval **)data;
#else
		zend_value = zend_hash_get_current_data_ex(ht1, &pos1);
#endif
		ht2 = Z_ARRVAL_P(zend_value);
		zend_hash_internal_pointer_reset_ex(ht2, &pos2);
		col = 0;
		while (col < result->cols) {
#if PHP_MAJOR_VERSION < 7
			int lookup_result = zend_hash_get_current_data_ex(ht2, &lookup_data, &pos2);
			if (lookup_result < 0) {
#else
			lookup_value = zend_hash_get_current_data_ex(ht2, &pos2);
			if (lookup_value == NULL) {
#endif
				result->data[(row * result->cols) + col] = estrdup("RLIB: INVALID ARRAY CELL");
				col++;
				continue;
			}

#if PHP_MAJOR_VERSION < 7
			lookup_value = *(zval **)lookup_data;
#endif

			data_result = NULL;
			memset(dstr, 0, 64);
			if (Z_TYPE_P(lookup_value) == IS_STRING)
				data_result = Z_STRVAL_P(lookup_value);
			else if (Z_TYPE_P(lookup_value) == IS_LONG) {
				sprintf(dstr, "%ld", Z_LVAL_P(lookup_value));
				data_result = estrdup(dstr);
			} else if (Z_TYPE_P(lookup_value) == IS_DOUBLE) {
				sprintf(dstr, "%f", Z_DVAL_P(lookup_value));
				data_result = estrdup(dstr);
			} else if (Z_TYPE_P(lookup_value) == IS_NULL) {
				data_result = estrdup("");
			} else {
				sprintf(dstr, "ZEND Z_TYPE %d NOT SUPPORTED", Z_TYPE_P(lookup_value));
				data_result = estrdup(dstr);
			}

			result->data[(row * result->cols) + col] = data_result;

			col++;
			zend_hash_move_forward_ex(ht2, &pos2);
		}
		row++;
		zend_hash_move_forward_ex(ht1, &pos1);
	}
	
	return result;
}

static gint rlib_php_array_num_fields(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_php_array_results *result = result_ptr;

	if (result == NULL)
		return 0;

	return result->cols;
}

static void rlib_php_array_free_input_filter(gpointer input_ptr UNUSED) {}

static void rlib_php_array_rlib_free_result(gpointer input_ptr UNUSED, gpointer result_ptr UNUSED) {}


static gpointer rlib_php_array_new_input_filter() {
	struct input_filter *input;
	
	input = emalloc(sizeof(struct input_filter));
	memset(input, 0, sizeof(struct input_filter));
	input->private = emalloc(sizeof(struct _private));
	memset(input->private, 0, sizeof(struct _private));
	input->input_close = rlib_php_array_input_close;
	input->num_fields = rlib_php_array_num_fields;
	input->start = rlib_php_array_start;
	input->next = rlib_php_array_next;
	input->get_error = rlib_php_array_get_error;
	input->isdone = rlib_php_array_isdone;
	input->new_result_from_query = php_array_new_result_from_query;
	input->get_field_name = rlib_php_array_get_field_name;
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
