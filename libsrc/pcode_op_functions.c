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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>
#include <locale.h>
#include <mpfr.h>

#include "rlib-internal.h"
#include "datetime.h"
#include "pcode.h"

static const gchar *rlib_value_get_type_as_str(rlib *r, struct rlib_value *v) {
	if (v == NULL)
		return "(null)";
	if (RLIB_VALUE_IS_NUMBER(r, v))
		return "number";
	if (RLIB_VALUE_IS_STRING(r, v))
		return "string";
	if (RLIB_VALUE_IS_DATE(r, v))
		return "date";
	if (RLIB_VALUE_IS_IIF(r, v))
		return "iif";
	if (RLIB_VALUE_IS_ERROR(r, v))
		return "ERROR";
	return "UNKNOWN";
}

static void rlib_pcode_operator_fatal_exception(rlib *r, const gchar *operator, struct rlib_pcode *code, gint64 pcount, struct rlib_value *v1, struct rlib_value *v2, struct rlib_value *v3) {
	rlogit(r, "RLIB EXPERIENCED A FATAL MATH ERROR WHILE TRYING TO PERFORM THE FOLLOWING OPERATION: %s\n", operator);
	rlogit(r, "\t* Error on Line %d: The Expression Was [%s]\n", code->line_number, code->infix_string);
	rlogit(r, "\t* DATA TYPES ARE [%s]", rlib_value_get_type_as_str(r, v1));
	if (pcount > 1)
		rlogit(r, " [%s]", rlib_value_get_type_as_str(r, v2));
	if (pcount > 2)
		rlogit(r, " [%s]", rlib_value_get_type_as_str(r, v3));
	rlogit(r, "\n");
}

gboolean rlib_pcode_operator_add(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_add(result, v1->mpfr_value, v2->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
			const gchar *safe1 =  RLIB_VALUE_GET_AS_STRING(r, v1) == NULL ? "" : RLIB_VALUE_GET_AS_STRING(r, v1);
			const gchar *safe2 =  RLIB_VALUE_GET_AS_STRING(r, v2) == NULL ? "" : RLIB_VALUE_GET_AS_STRING(r, v2);
			gchar *newstr = g_malloc(strlen(safe1)+strlen(safe2) + 1);
			strcpy(newstr, safe2);
			strcat(newstr, safe1);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, newstr));
			g_free(newstr);
			return TRUE;
		}
		if ((RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) || (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_DATE(r, v2))) {
			struct rlib_value *number, *date;
			struct rlib_datetime *newday;
			if (RLIB_VALUE_IS_DATE(r, v1)) {
				date = v1;
				number = v2;
			} else {
				date = v2;
				number = v1;
			}
			newday = RLIB_VALUE_GET_AS_DATE(r, date);
			rlib_datetime_addto(newday, mpfr_get_sj(number->mpfr_value, MPFR_RNDN));
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, newday));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r, "ADD", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}


gboolean rlib_pcode_operator_subtract(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_sub(result, v2->mpfr_value, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		} else if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			struct rlib_datetime *dt1, *dt2;
			gint64 result = 0;
			gint64 daysdiff = 0;
			gint64 secsdiff = 0;
			gboolean usedays = FALSE;
			gboolean usesecs = FALSE;
			dt1 = RLIB_VALUE_GET_AS_DATE(r, v1);
			dt2 = RLIB_VALUE_GET_AS_DATE(r, v2);
			if (rlib_datetime_valid_time(dt1) && rlib_datetime_valid_time(dt2)) {
				secsdiff = rlib_datetime_secsdiff(dt1, dt2);
				usesecs = TRUE;
			}
			if (rlib_datetime_valid_date(dt1) && rlib_datetime_valid_date(dt2)) {
				daysdiff = rlib_datetime_daysdiff(dt1, dt2);
				usedays = TRUE;
			}
			if (usedays) result = daysdiff;
			if (usesecs) result	= (daysdiff * RLIB_DATETIME_SECSPERDAY) + secsdiff;
			if (usedays || usesecs) {
				mpfr_t result2;
				mpfr_init2(result2, r->numeric_precision_bits);
				mpfr_set_ui(result2, result, MPFR_RNDN);
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result2));
				mpfr_clear(result2);
				return TRUE;
			}
		} else if (RLIB_VALUE_IS_DATE(r, v2) && RLIB_VALUE_IS_NUMBER(r, v1)) {
			struct rlib_value *number, *date;
			struct rlib_datetime *newday;
			date = v2;
			number = v1;
			newday = RLIB_VALUE_GET_AS_DATE(r, date);
			rlib_datetime_addto(newday, -mpfr_get_sj(number->mpfr_value, MPFR_RNDN));
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, newday));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"SUBTRACT", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_multiply(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_mul(result, v1->mpfr_value, v2->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"MULTIPLY", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_divide(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_div(result, v2->mpfr_value, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"DIVIDE", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_mod(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_remainder(result, v2->mpfr_value, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"MOD", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_pow(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_pow(result, v2->mpfr_value, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"POW", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}


gboolean rlib_pcode_operator_lte(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);

	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			int cmpresult = mpfr_cmp(v2->mpfr_value, v1->mpfr_value);
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_set_ui(result, (cmpresult <= 0 ? 1 : 0), MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
			if (r_strcmp(RLIB_VALUE_GET_AS_STRING(r, v2), RLIB_VALUE_GET_AS_STRING(r, v1)) <= 0) {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, RLIB_DECIMAL_PRECISION));
			} else {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, 0));
			}
			return TRUE;
		}
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			struct rlib_datetime *t1, *t2;
			gint64 val;
			t1 = RLIB_VALUE_GET_AS_DATE(r, v1);
			t2 = RLIB_VALUE_GET_AS_DATE(r, v2);
			val = (rlib_datetime_compare(t2, t1) <= 0)? RLIB_DECIMAL_PRECISION : 0;
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, val));
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"<=", code, 2, v1, v2, NULL);
	return FALSE;
}


gboolean rlib_pcode_operator_lt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			int cmpresult = mpfr_cmp(v2->mpfr_value, v1->mpfr_value);
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_set_ui(result, (cmpresult < 0 ? 1 : 0), MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
			if (r_strcmp(RLIB_VALUE_GET_AS_STRING(r, v2), RLIB_VALUE_GET_AS_STRING(r, v1)) < 0) {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, RLIB_DECIMAL_PRECISION));
			} else {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, 0));
			}
			return TRUE;
		}
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			struct rlib_datetime *t1, *t2;
			gint64 val;
			t1 = RLIB_VALUE_GET_AS_DATE(r, v1);
			t2 = RLIB_VALUE_GET_AS_DATE(r, v2);
			val = (rlib_datetime_compare(t2, t1) < 0) ? RLIB_DECIMAL_PRECISION : 0;
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, val));
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"<", code, 2, v1, v2, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_gte(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			int cmpresult = mpfr_cmp(v2->mpfr_value, v1->mpfr_value);
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_set_ui(result, (cmpresult >= 0 ? 1 : 0), MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
			if (r_strcmp(RLIB_VALUE_GET_AS_STRING(r, v2), RLIB_VALUE_GET_AS_STRING(r, v1)) >= 0) {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, RLIB_DECIMAL_PRECISION));
			} else {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, 0));
			}
			return TRUE;
		}
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			struct rlib_datetime *t1, *t2;
			gint64 val;
			t1 = RLIB_VALUE_GET_AS_DATE(r, v1);
			t2 = RLIB_VALUE_GET_AS_DATE(r, v2);
			val = (rlib_datetime_compare(t2, t1) >= 0) ? RLIB_DECIMAL_PRECISION : 0;
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, val));
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,">=", code, 2, v1, v2, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_gt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			int cmpresult = mpfr_cmp(v2->mpfr_value, v1->mpfr_value);
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_set_ui(result, (cmpresult > 0 ? 1 : 0), MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
			if (r_strcmp(RLIB_VALUE_GET_AS_STRING(r, v2), RLIB_VALUE_GET_AS_STRING(r, v1)) > 0) {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, RLIB_DECIMAL_PRECISION));
			} else {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, 0));
			}
			return TRUE;
		}
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			struct rlib_datetime *t1, *t2;
			gint64 val;
			t1 = RLIB_VALUE_GET_AS_DATE(r, v1);
			t2 = RLIB_VALUE_GET_AS_DATE(r, v2);
			val = (rlib_datetime_compare(t2, t1) > 0) ? RLIB_DECIMAL_PRECISION : 0;
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, val));
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,">", code, 2, v1, v2, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_eql(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			int cmpresult = mpfr_cmp(v2->mpfr_value, v1->mpfr_value);
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_set_ui(result, (cmpresult == 0 ? 1 : 0), MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
			gint64 push;
			if (RLIB_VALUE_GET_AS_STRING(r, v2) == NULL && RLIB_VALUE_GET_AS_STRING(r, v1) == NULL)
				push = RLIB_DECIMAL_PRECISION;
			else if (RLIB_VALUE_GET_AS_STRING(r, v2) == NULL || RLIB_VALUE_GET_AS_STRING(r, v1) == NULL)
				push = 0;
			else {
				if (r_strcmp(RLIB_VALUE_GET_AS_STRING(r, v2), RLIB_VALUE_GET_AS_STRING(r, v1)) == 0) {
					push = RLIB_DECIMAL_PRECISION;
				} else {
					push = 0;
				}
			}
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_new_number_from_long(r, &rval_rtn, push);
			rlib_value_stack_push(r, vs, &rval_rtn);
			return TRUE;
		}
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			struct rlib_datetime *t1, *t2;
			gint64 val;
			t1 = RLIB_VALUE_GET_AS_DATE(r, v1);
			t2 = RLIB_VALUE_GET_AS_DATE(r, v2);
			val = (rlib_datetime_compare(t2, t1) == 0)? RLIB_DECIMAL_PRECISION : 0;
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, val));
			return TRUE;
		}
	}
	if (RLIB_VALUE_IS_VECTOR(r, v1) && RLIB_VALUE_IS_VECTOR(r, v2)) {
		GSList *vec1, *vec2;
		GSList *list1, *list2;
		gint64 val = 0;

		vec1 = RLIB_VALUE_GET_AS_VECTOR(r, v1);
		vec2 = RLIB_VALUE_GET_AS_VECTOR(r, v2);

		if (g_slist_length(vec1) != g_slist_length(vec2)) {
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, val));
			return TRUE;
		}

		for (list1 = vec1, list2 = vec2; list1; list1 = list1->next, list2 = list2->next) {
			struct rlib_pcode *code1 = list1->data, *code2 = list2->data;
			struct rlib_value val1, val2;
			struct rlib_value *pval1, *pval2;
			gint64 retval;

			rlib_value_init(r, &val1);
			rlib_value_init(r, &val2);

			pval1 = rlib_execute_pcode(r, &val1, code1, NULL);
			pval2 = rlib_execute_pcode(r, &val2, code2, NULL);

			retval = rvalcmp(r, pval1, pval2);

			rlib_value_free(r, &val1);
			rlib_value_free(r, &val2);

			if (retval != 0)
				break;
		}

		if (!list1)
			val = RLIB_DECIMAL_PRECISION;

		rlib_value_free(r, v1);
		rlib_value_free(r, v2);
		rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, val));
		return TRUE;
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"==", code, 2, v1, v2, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_noteql(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			int cmpresult = mpfr_cmp(v2->mpfr_value, v1->mpfr_value);
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_set_ui(result, (cmpresult != 0 ? 1 : 0), MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
			gint64 push;
			if (RLIB_VALUE_GET_AS_STRING(r, v2) == NULL && RLIB_VALUE_GET_AS_STRING(r, v1) == NULL)
				push = 0;
			else if (RLIB_VALUE_GET_AS_STRING(r, v2) == NULL || RLIB_VALUE_GET_AS_STRING(r, v1) == NULL)
				push = RLIB_DECIMAL_PRECISION;
			else {
				if (r_strcmp(RLIB_VALUE_GET_AS_STRING(r, v2), RLIB_VALUE_GET_AS_STRING(r, v1)) == 0) {
					push = 0;
				} else {
					push = RLIB_DECIMAL_PRECISION;
				}
			}
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_new_number_from_long(r, &rval_rtn, push);
			rlib_value_stack_push(r, vs, &rval_rtn);
			return TRUE;
		}
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			struct rlib_datetime *t1, *t2;
			gint64 val;
			t1 = RLIB_VALUE_GET_AS_DATE(r, v1);
			t2 = RLIB_VALUE_GET_AS_DATE(r, v2);
			val = (rlib_datetime_compare(t2, t1) != 0)? RLIB_DECIMAL_PRECISION : 0;
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, val));
			return TRUE;
		}
		if (RLIB_VALUE_IS_VECTOR(r, v1) && RLIB_VALUE_IS_VECTOR(r, v2)) {
			GSList *vec1, *vec2;
			GSList *list1, *list2;
			gint64 val = RLIB_DECIMAL_PRECISION;

			vec1 = RLIB_VALUE_GET_AS_VECTOR(r, v1);
			vec2 = RLIB_VALUE_GET_AS_VECTOR(r, v2);

			if (g_slist_length(vec1) != g_slist_length(vec2)) {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, val));
				return TRUE;
			}

			for (list1 = vec1, list2 = vec2; list1; list1 = list1->next, list2 = list2->next) {
				struct rlib_pcode *code1 = list1->data, *code2 = list2->data;
				struct rlib_value val1, val2;
				struct rlib_value *pval1, *pval2;
				gint64 retval = 0;

				rlib_value_init(r, &val1);
				rlib_value_init(r, &val2);

				pval1 = rlib_execute_pcode(r, &val1, code1, NULL);
				pval2 = rlib_execute_pcode(r, &val2, code2, NULL);

				retval = rvalcmp(r, pval1, pval2);

				rlib_value_free(r, &val1);
				rlib_value_free(r, &val2);

				if (retval != 0)
					break;
			}

			if (!list1)
				val = 0;

			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, val));
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"!=", code, 2, v1, v2, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_logical_and(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			gint v1val = mpfr_get_si(v1->mpfr_value, MPFR_RNDN);
			gint v2val = mpfr_get_si(v2->mpfr_value, MPFR_RNDN);
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_set_ui(result, !!(v1val && v2val), MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
			if (RLIB_VALUE_GET_AS_STRING(r, v2) && RLIB_VALUE_GET_AS_STRING(r, v1)) {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, RLIB_DECIMAL_PRECISION));
			} else {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, 0));
			}
			return TRUE;
		}
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, RLIB_DECIMAL_PRECISION));
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"LOGICAL AND", code, 2, v1, v2, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_and(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);

	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			gint64 v1_num = mpfr_get_sj(v1->mpfr_value, MPFR_RNDN);
			gint64 v2_num = mpfr_get_sj(v2->mpfr_value, MPFR_RNDN);
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_set_sj(result, v2_num & v1_num, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"AND", code, 2, v1, v2, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_logical_or(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			gint v1val = mpfr_get_si(v1->mpfr_value, MPFR_RNDN);
			gint v2val = mpfr_get_si(v2->mpfr_value, MPFR_RNDN);
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_set_ui(result, !!(v1val || v2val), MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
			if (RLIB_VALUE_GET_AS_STRING(r, v2) || RLIB_VALUE_GET_AS_STRING(r, v1)) {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, RLIB_DECIMAL_PRECISION));
			} else {
				rlib_value_free(r, v1);
				rlib_value_free(r, v2);
				rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, 0));
			}
			return TRUE;
		}
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, RLIB_DECIMAL_PRECISION));
		}
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"LOGICAL OR", code, 2, v1, v2, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_or(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);

	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			mpfr_t result;
			gint64 v1_num = mpfr_get_sj(v1->mpfr_value, MPFR_RNDN);
			gint64 v2_num = mpfr_get_sj(v2->mpfr_value, MPFR_RNDN);
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_set_sj(result, v2_num | v1_num, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"OR", code, 2, v1, v2, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_abs(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_NUMBER(r, v1)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_abs(result, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"ABS", code, 1, v1, NULL, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_ceil(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_NUMBER(r, v1)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_ceil(result, v1->mpfr_value);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"CEIL", code, 1, v1, NULL, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_floor(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_NUMBER(r, v1)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_floor(result, v1->mpfr_value);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"FLOOR", code, 1, v1, NULL, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_round(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_NUMBER(r, v1)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_round(result, v1->mpfr_value);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"ROUND", code, 1, v1, NULL, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_sin(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_NUMBER(r, v1)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_sin(result, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"sin", code, 1, v1, NULL, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_cos(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_NUMBER(r, v1)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_cos(result, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"cos", code, 1, v1, NULL, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_ln(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_NUMBER(r, v1)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_log(result, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"ln", code, 1, v1, NULL, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_exp(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_NUMBER(r, v1)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_exp(result, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"exp", code, 1, v1, NULL, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_atan(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_NUMBER(r, v1)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_atan(result, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"atan", code, 1, v1, NULL, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_sqrt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_NUMBER(r, v1)) {
			mpfr_t result;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_sqrt(result, v1->mpfr_value, MPFR_RNDN);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"sqrt", code, 1, v1, NULL, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_fxpval(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
			mpfr_t result, powoften;
			mpfr_init2(result, r->numeric_precision_bits);
			mpfr_init2(powoften, r->numeric_precision_bits);
			mpfr_set_str(result, RLIB_VALUE_GET_AS_STRING(r, v2), 10, MPFR_RNDN);
			mpfr_set_si(powoften, 10, MPFR_RNDN);
			mpfr_pow(powoften, powoften, v1->mpfr_value, MPFR_RNDN);
			mpfr_div(result, result, powoften, MPFR_RNDN);
			mpfr_clear(powoften);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
			mpfr_clear(result);
			return TRUE;
		}
	}
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	rlib_pcode_operator_fatal_exception(r,"fxpval", code, 2, v1, v2, NULL);
	return FALSE;
}

gboolean rlib_pcode_operator_val(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (RLIB_VALUE_IS_STRING(r, v1)) {
		mpfr_t result;
		mpfr_init2(result, r->numeric_precision_bits);
		mpfr_set_str(result, RLIB_VALUE_GET_AS_STRING(r, v1), 10, MPFR_RNDN);
		rlib_value_free(r, v1);
		rlib_value_stack_push(r, vs, rlib_value_new_number_from_mpfr(r, &rval_rtn, result));
		mpfr_clear(result);
		return TRUE;
	}
	rlib_pcode_operator_fatal_exception(r,"val", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

/* TODO: REVISIT THIS */
gboolean rlib_pcode_operator_str(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	gchar fmtstring[20];
	gchar *dest;
	struct rlib_value *v1, *v2, *v3, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	v3 = rlib_value_stack_pop(r, vs);
	if (v1 && v2 && v3) {
		if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2) && RLIB_VALUE_IS_NUMBER(r, v3)) {
			gint64 n1 = mpfr_get_sj(v1->mpfr_value, MPFR_RNDN);
			gint64 n2 = mpfr_get_sj(v2->mpfr_value, MPFR_RNDN);
			if (n1 > 0)
				sprintf(fmtstring, "%%%" PRId64 ".%" PRId64 "d", n2, n1);
			else
				sprintf(fmtstring, "%%%" PRId64 "d", n2);
			rlib_number_sprintf(r, &dest, fmtstring, v3, 0, "((NONE))", -1);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_free(r, v3);
			rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, dest));
			g_free(dest);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"str", code, 3, v1, v2, v3);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_free(r, v3);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

static gboolean rlib_pcode_operator_stod_common(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gint64 which) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (RLIB_VALUE_IS_STRING(r, v1)) {
		struct rlib_datetime dt;
		gchar *tstr = RLIB_VALUE_GET_AS_STRING(r, v1);
		int err = FALSE;

		rlib_datetime_clear(&dt);
		if (which) { /* convert time */
			gint hour = 12, minute = 0, second = 0; /* safe time for date only. */
			gchar ampm = 'a';
			if (tstr) {
				if (sscanf(tstr, "%2d:%2d:%2d%c", &hour, &minute, &second, &ampm) != 4) {
					if (sscanf(tstr, "%2d:%2d:%2d", &hour, &minute, &second) != 3) {
						second = 0;
						if (sscanf(tstr, "%2d:%2d%c", &hour, &minute, &ampm) != 3) {
							second = 0;
							ampm = 0;
							if (sscanf(tstr, "%2d:%2d", &hour, &minute) != 2) {
								if (sscanf(tstr, "%2d%2d%2d", &hour, &minute, &second) != 3) {
									second = 0;
									if (sscanf(tstr, "%2d%2d", &hour, &minute) != 2) {
										r_error(r, "Invalid Date format: stod(%s)", tstr);
										err = TRUE;
									}
								}
							}
						}
					}
				}
				if (toupper(ampm) == 'P') hour += 12;
				hour %= 24;
				if (!err) {
					rlib_datetime_set_time(&dt, hour, minute, second);
				}
			} else {
				r_error(r, "Invalid Date format: stod(%s)", tstr);
				err = TRUE;
			}
		} else { /* convert date */
			gint year, month, day;
			if (tstr) {
				if (sscanf(tstr, "%4d-%2d-%2d", &year, &month, &day) != 3) {
					if (sscanf(tstr, "%4d%2d%2d", &year, &month, &day) != 3) {
						r_error(r, "Invalid Date format: stod(%s)", tstr);
						err = TRUE;
					}
				}
				if (!err) {
					rlib_datetime_set_date(&dt, year, month, day);
				}
			} else {
				r_error(r, "Invalid Date format: stod(%s)", tstr);
				err = TRUE;
			}
		}
		if (!err) {
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, &dt));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"stod", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_stod(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data UNUSED) {
	return rlib_pcode_operator_stod_common(r, code, vs, this_field_value, 0);
}

gboolean rlib_pcode_operator_tstod(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data UNUSED) {
	return rlib_pcode_operator_stod_common(r, code, vs, this_field_value, 1);
}

gboolean rlib_pcode_operator_iif(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data UNUSED) {
	gboolean thisresult = FALSE;
	struct rlib_value *v1;
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_IIF(r, v1)) {
			struct rlib_pcode_if *rif = v1->iif_value;
			struct rlib_value *result;

			if (rif->false == NULL || rif->true == NULL) {
				r_error(r, "IIF STATEMENT IS INVALID [%s] @ %d\n", code->infix_string, code->line_number);
			} else {
				execute_pcode(r, rif->evaluation, vs, this_field_value, FALSE);
				result = rlib_value_stack_pop(r, vs);

				if (RLIB_VALUE_IS_NUMBER(r, result)) {
					glong res = mpfr_get_si(result->mpfr_value, MPFR_RNDN);
					if (res == 0) {
						rlib_value_free(r, result);
						rlib_value_free(r, v1);
						thisresult = execute_pcode(r, rif->false, vs, this_field_value, FALSE);
					} else {
						rlib_value_free(r, result);
						rlib_value_free(r, v1);
						thisresult = execute_pcode(r, rif->true, vs, this_field_value, FALSE);
					}
				} else if (RLIB_VALUE_IS_STRING(r, result)) {
					if (RLIB_VALUE_GET_AS_STRING(r, result) == NULL) {
						rlib_value_free(r, result);
						rlib_value_free(r, v1);
						thisresult = execute_pcode(r, rif->false, vs, this_field_value, FALSE);
					} else {
						rlib_value_free(r, result);
						rlib_value_free(r, v1);
						thisresult = execute_pcode(r, rif->true, vs, this_field_value, FALSE);
					}
				} else {
					rlib_value_free(r, result);
					rlib_value_free(r, v1);
					r_error(r, "CAN'T COMPARE IIF VALUE [%d]\n", RLIB_VALUE_GET_TYPE(r, result));
				}
			}
		}
	}
	if (!thisresult)
		rlib_pcode_operator_fatal_exception(r,"iif", code, 1, v1, NULL, NULL);
	return thisresult;
}

static gboolean rlib_pcode_operator_dtos_common(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, const char *format) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	gboolean result = FALSE;
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_DATE(r, v1)) {
			gchar *buf = NULL;
			struct rlib_datetime *tmp = RLIB_VALUE_GET_AS_DATE(r, v1);
			rlib_datetime_format(r, &buf, tmp, format);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, buf));
			g_free(buf);
			result = TRUE;
		}
	}

	rlib_pcode_operator_fatal_exception(r,"dtos", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return result;
}

gboolean rlib_pcode_operator_dateof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value rval_rtn, *v1;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_DATE(r, v1)) {
			struct rlib_datetime *dt0 = RLIB_VALUE_GET_AS_DATE(r, v1);
			struct rlib_datetime dt = *dt0;
			rlib_datetime_clear_time(&dt);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, &dt));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"dateof", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}


gboolean rlib_pcode_operator_timeof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value rval_rtn, *v1;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_DATE(r, v1)) {
			struct rlib_datetime *tm_date0 = RLIB_VALUE_GET_AS_DATE(r, v1);
			struct rlib_datetime tm_date = *tm_date0;
			tm_date.date = *g_date_new();
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, &tm_date));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"dateof", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}


/*
 * function setdateof(date1, date2) - makes date of date1 = date of date2.
 * time of date1 is unchanged.
 */
gboolean rlib_pcode_operator_chgdateof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			struct rlib_datetime *pd1 = RLIB_VALUE_GET_AS_DATE(r, v1);
			struct rlib_datetime *pd2 = RLIB_VALUE_GET_AS_DATE(r, v2);
			struct rlib_datetime d2 = *pd2;
			rlib_datetime_makesamedate(&d2, pd1);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, &d2));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"chgdateof", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_chgtimeof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
			struct rlib_datetime *pd1 = RLIB_VALUE_GET_AS_DATE(r, v1);
			struct rlib_datetime *pd2 = RLIB_VALUE_GET_AS_DATE(r, v2);
			struct rlib_datetime d2 = *pd2;
			rlib_datetime_makesametime(&d2, pd1);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, &d2));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"chgtimeof", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_gettimeinsecs(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value rval_rtn, *v1;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_DATE(r, v1)) {
			struct rlib_datetime *dt = RLIB_VALUE_GET_AS_DATE(r, v1);
			gint64 secs = rlib_datetime_time_as_long(dt);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, secs * RLIB_DECIMAL_PRECISION));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"gettimeinsecs", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_settimeinsecs(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v2 = rlib_value_stack_pop(r, vs);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			struct rlib_datetime dt = *RLIB_VALUE_GET_AS_DATE(r, v1);
			glong secs = mpfr_get_si(v2->mpfr_value, MPFR_RNDN);
			rlib_datetime_set_time_from_long(&dt, secs);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, &dt));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"settimefromsecs", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}


gboolean rlib_pcode_operator_dtos(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data UNUSED) {
	return rlib_pcode_operator_dtos_common(r, code, vs, this_field_value, "%Y-%m-%d");
}


gboolean rlib_pcode_operator_dtosf(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value rval_rtn, *v1;
	gboolean result = FALSE;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_STRING(r, v1)) {
			gchar *fmt = g_strdup(RLIB_VALUE_GET_AS_STRING(r, v1));
			rlib_value_free(r, v1);
			result = rlib_pcode_operator_dtos_common(r, code, vs, this_field_value, fmt);
			g_free(fmt);
			return result;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"dtosf", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	v1 = rlib_value_stack_pop(r, vs); /* clear the stack */
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return result;
}

/*
 * Generic format function format(object, formatstring)
 */
gboolean rlib_pcode_operator_format(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	gchar current_locale[MAXSTRLEN];

	rlib_value_init(r, &rval_rtn);
	v2 = rlib_value_stack_pop(r, vs);
	v1 = rlib_value_stack_pop(r, vs);

	if (r->special_locale) {
		gchar *tmp;
		tmp = setlocale(LC_ALL, NULL);
		if (tmp == NULL)
			current_locale[0] = 0;
		else
			strcpy(current_locale, tmp);
		setlocale(LC_ALL, r->special_locale);
	}

	if (RLIB_VALUE_IS_STRING(r, v2)) {
		gchar buf[MAXSTRLEN];
		gchar *result = "!ERR_F_IV";
		gchar *fmt = RLIB_VALUE_GET_AS_STRING(r, v2);
		gboolean need_free = FALSE;
		if (*fmt == '!') {
			++fmt;
			buf[MAXSTRLEN - 1] = '\0'; /* Assure terminated */
			if (RLIB_VALUE_IS_STRING(r, v1)) {
				gchar *str = RLIB_VALUE_GET_AS_STRING(r, v1);
				if ((*fmt == '#') || (*fmt == '!')) {
					++fmt;
					sprintf(buf, fmt, str);
					result = buf;
				} else r_error(r, "Format type does not match variable type in 'format' function");
			} else if (RLIB_VALUE_IS_NUMBER(r, v1)) {
				if (*fmt == '#') {
					++fmt;
					rlib_format_number(r, &result, fmt, v1->mpfr_value);
					need_free = TRUE;
				} else if (*fmt == '$') {
					++fmt;
					rlib_format_money(r, &result, fmt, v1->mpfr_value);
					need_free = TRUE;
				} else r_error(r, "Format type does not match variable type in 'format' function");
			} else if (RLIB_VALUE_IS_DATE(r, v1)) {
				if (*fmt == '@') {
					struct rlib_datetime *dt = RLIB_VALUE_GET_AS_DATE(r, v1);
					++fmt;
					rlib_datetime_format(r, &result, dt, fmt);
					need_free = TRUE;
				} else {
					r_error(r, "Format type does not match variable type in 'format' function");
				}
			} else {
				r_error(r, "Invalid variable type encountered in format");
			}
		}
		rlib_value_free(r, v1);
		rlib_value_free(r, v2);
		rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, result));
		if (need_free)
			g_free(result);
		setlocale(LC_ALL, current_locale);
		return TRUE;
	}
	rlib_pcode_operator_fatal_exception(r,"format not string in format", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	setlocale(LC_ALL, current_locale);
	return FALSE;
}

gboolean rlib_pcode_operator_year(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_DATE(r, v1)) {
			struct rlib_datetime *d = RLIB_VALUE_GET_AS_DATE(r, v1);
			glong tmp = g_date_get_year(&d->date);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, tmp));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"year", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_month(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_DATE(r, v1)) {
			struct rlib_datetime *d = RLIB_VALUE_GET_AS_DATE(r, v1);
			glong mon = g_date_get_month(&d->date);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, mon));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"month", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_day(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_DATE(r, v1)) {
			struct rlib_datetime *d = RLIB_VALUE_GET_AS_DATE(r, v1);
			glong day = g_date_get_day(&d->date);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, day));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"day", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_upper(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_STRING(r, v1)) {
			gchar *str = RLIB_VALUE_GET_AS_STRING(r, v1);
			gchar *tmp = "";

			if (str)
				tmp = r_strupr(str);

			rlib_value_free(r, v1);

			rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, tmp));
			if (str)
				g_free(tmp);

			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"upper", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_lower(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_STRING(r, v1)) {
			gchar *str = RLIB_VALUE_GET_AS_STRING(r, v1);
			gchar *tmp = "";

			if (str)
				tmp = r_strlwr(str);

			rlib_value_free(r, v1);

			rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, tmp));
			if (str)
				g_free(tmp);

			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"lower", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_strlen(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_STRING(r, v1) ) {
			gint64 slen = 0;
			if ( RLIB_VALUE_GET_AS_STRING(r, v1) )
				slen = strlen(RLIB_VALUE_GET_AS_STRING(r, v1));
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, slen));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"strlen", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_left(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_STRING(r, v2) && RLIB_VALUE_IS_NUMBER(r, v1)) {
			gchar *tmp = g_strdup(RLIB_VALUE_GET_AS_STRING(r, v2));
			glong n = mpfr_get_si(v1->mpfr_value, MPFR_RNDN);
			glong len = r_strlen(tmp);
			if (n >= 0)
				if (len > n)
					*r_ptrfromindex(tmp, n) = '\0';
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, tmp));
			g_free(tmp);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"left", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_right(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	v2 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_STRING(r, v2) && RLIB_VALUE_IS_NUMBER(r, v1)) {
			gchar *tmp = g_strdup(RLIB_VALUE_GET_AS_STRING(r, v2));
			glong n = mpfr_get_si(v1->mpfr_value, MPFR_RNDN);
			glong len = r_strlen(tmp);
			if (n >= 0)
				if (n > len)
					n = len;
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, r_ptrfromindex(tmp, len - n)));
			g_free(tmp);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"right", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_substring(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, *v3, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs); /* the new size */
	v2 = rlib_value_stack_pop(r, vs); /* the start idx */
	v3 = rlib_value_stack_pop(r, vs); /* the string */
	if (v1 && v2 && v3) {
		if (RLIB_VALUE_IS_STRING(r, v3) && RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			gchar *tmp = g_strdup(RLIB_VALUE_GET_AS_STRING(r, v3));
			gint64 st = mpfr_get_sj(v2->mpfr_value, MPFR_RNDN);
			gint64 sz = mpfr_get_sj(v1->mpfr_value, MPFR_RNDN);
			gint64 len = r_strlen(tmp);
			gint64 maxlen;
			if (st < 0)
				st = 0;
			if (st > len)
				st = len;
			maxlen = len - st;
			if (sz < 0)
				sz = maxlen;
			if (sz > maxlen)
				sz = maxlen;
			*r_ptrfromindex(tmp, st + sz) = '\0';
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_free(r, v3);
			rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, r_ptrfromindex(tmp, st)));
			g_free(tmp);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"substring", code, 3, v1, v2, v3);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_free(r, v3);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_proper(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_STRING(r, v1)) {
			if (RLIB_VALUE_GET_AS_STRING(r, v1) == NULL || RLIB_VALUE_GET_AS_STRING(r, v1)[0] == '\0') {
				rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, ""));
				return TRUE;
			} else {
				gchar *tmp = strproper(RLIB_VALUE_GET_AS_STRING(r, v1));
				rlib_value_free(r, v1);
				rlib_value_stack_push(r, vs, rlib_value_new_string(r, &rval_rtn, tmp));
				g_free(tmp);
				return TRUE;
			}
		}
	}
	rlib_pcode_operator_fatal_exception(r,"proper", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_stodt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_STRING(r, v1)) {
			gint year = 2000, month = 1, day = 1, hour = 12, min = 0, sec = 0;
			struct rlib_datetime dt;
			gchar *str = RLIB_VALUE_GET_AS_STRING(r, v1);
			sscanf(str, "%4d%2d%2d%2d%2d%2d", &year, &month, &day, &hour, &min, &sec);
			rlib_datetime_clear(&dt);
			rlib_datetime_set_date(&dt, year, month, day);
			rlib_datetime_set_time(&dt, hour, min, sec);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, &dt));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"stodt", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_stodtsql(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_STRING(r, v1)) {
			gint year = 2000, month = 1, day = 1, hour = 12, min = 0, sec = 0;
			struct rlib_datetime dt;
			gchar *str = RLIB_VALUE_GET_AS_STRING(r, v1);
			sscanf(str, "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec);
			rlib_datetime_clear(&dt);
			rlib_datetime_set_date(&dt, year, month, day);
			rlib_datetime_set_time(&dt, hour, min, sec);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, &dt));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"stodt", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_isnull(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_STRING(r, v1)) {
			gint64 result = RLIB_VALUE_GET_AS_STRING(r, v1) == NULL ? 1 : 0;
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, result));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"isnull", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_dim(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_DATE(r, v1)) {
			struct rlib_datetime *request = RLIB_VALUE_GET_AS_DATE(r, v1);
			glong dim = g_date_get_days_in_month(g_date_get_month(&request->date), g_date_get_year(&request->date));
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, dim));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"dim", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_wiy(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_DATE(r, v1)) {
			struct rlib_datetime *request = RLIB_VALUE_GET_AS_DATE(r, v1);
			glong dim = g_date_get_sunday_week_of_year(&request->date);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, dim));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"wiy", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_wiyo(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_value *v1, *v2, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v2 = rlib_value_stack_pop(r, vs);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1 && v2) {
		if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
			struct rlib_datetime *request = RLIB_VALUE_GET_AS_DATE(r, v1);
			glong dim;
			glong offset;
			offset = g_date_get_weekday(&request->date);
			offset = offset - mpfr_get_si(v2->mpfr_value, MPFR_RNDN);
			if (offset < 0)
				offset += 7;
			g_date_subtract_days(&request->date, offset);
			dim = g_date_get_sunday_week_of_year(&request->date);
			rlib_value_free(r, v1);
			rlib_value_free(r, v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, &rval_rtn, dim));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"wiyo", code, 2, v1, v2, NULL);
	rlib_value_free(r, v1);
	rlib_value_free(r, v2);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}

gboolean rlib_pcode_operator_date(rlib *r, struct rlib_pcode *code UNUSED, struct rlib_value_stack *vs, struct rlib_value *this_field_value UNUSED, gpointer user_data UNUSED) {
	struct rlib_datetime dt;
	struct rlib_value rval_rtn;
	struct tm *tmp = localtime(&r->now);
	rlib_value_init(r, &rval_rtn);
	rlib_datetime_clear(&dt);
	rlib_datetime_set_date(&dt, tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday);
	rlib_datetime_set_time(&dt, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	rlib_value_stack_push(r, vs, rlib_value_new_date(r, &rval_rtn, &dt));
	return TRUE;
}

gboolean rlib_pcode_operator_eval(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data UNUSED) {
	struct rlib_value *v1, rval_rtn;
	rlib_value_init(r, &rval_rtn);
	v1 = rlib_value_stack_pop(r, vs);
	if (v1) {
		if (RLIB_VALUE_IS_STRING(r, v1)) {
			struct rlib_pcode *code = NULL;
			code = rlib_infix_to_pcode(r, NULL, NULL, RLIB_VALUE_GET_AS_STRING(r, v1), -1, TRUE);
			rlib_execute_pcode(r, &rval_rtn, code, this_field_value);
			rlib_pcode_free(r, code);
			rlib_value_free(r, v1);
			rlib_value_stack_push(r, vs,&rval_rtn);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_exception(r,"eval", code, 1, v1, NULL, NULL);
	rlib_value_free(r, v1);
	rlib_value_stack_push(r, vs, rlib_value_new_error(r, &rval_rtn));
	return FALSE;
}
