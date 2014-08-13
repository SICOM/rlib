/*
 *  Copyright (C) 2003-2014 SICOM Systems, INC.
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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>
#include <locale.h>

#include "config.h"
#include "rlib.h"
#include "datetime.h"
#include "pcode.h"

const gchar * rlib_value_get_type_as_str(struct rlib_value *v) {
	if(v == NULL)
		return "(null)";
	if(RLIB_VALUE_IS_NUMBER(v))
		return "number";
	if(RLIB_VALUE_IS_STRING(v))
		return "string";
	if(RLIB_VALUE_IS_DATE(v))
		return "date";
	if(RLIB_VALUE_IS_IIF(v))
		return "iif";
	if(RLIB_VALUE_IS_ERROR(v))
		return "ERROR";
	return "UNKNOWN";
}

static void rlib_pcode_operator_fatal_execption(rlib *r, const gchar *operator, struct rlib_pcode *code, gint pcount, struct rlib_value *v1,
	struct rlib_value *v2, struct rlib_value *v3) {
	rlogit(r, "RLIB EXPERIENCED A FATAL MATH ERROR WHILE TRYING TO PERFORM THE FOLLOWING OPERATION: %s\n", operator);
	rlogit(r, "\t* Error on Line %d: The Expression Was [%s]\n", code->line_number, code->infix_string);
	rlogit(r, "\t* DATA TYPES ARE [%s]", rlib_value_get_type_as_str(v1));
	if(pcount > 1)
		rlogit(r, " [%s]", rlib_value_get_type_as_str(v2));
	if(pcount > 2)
		rlogit(r, " [%s]", rlib_value_get_type_as_str(v3));
	rlogit(r, "\n");
}

#if USE_RLIB_VAR
gint rlib_pcode_add(rlib_var_stack *rvs) {
	rlib_var *v1, *v2;
	int tv1, tv2;
	v1 = rlib_var_stack_pop(rvs);
	v2 = rlib_var_stack_peek(rvs);
	tv1 = rlib_var_get_type(v1);
	tv2 = rlib_var_get_type(v2);
	switch (tv2) {
	case RLIB_VAR_NUMERIC:
		if (tv1 != RLIB_VAR_NUMERIC) r_error("Wrong type");
		else {
			rlib_var_add_number(v2, rlib_var_get_number(v1));
		}
		break;
	}
}

gint rlib_pcode_operator_add(rlib *r, struct rlib_pcode *code, struct rlib_var_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2;
	struct rlib_var_stack *vs = r->stack;
	v1 = rlib_var_stack_pop(vs);
	v2 = rlib_var_stack_pop(vs);
	if(v1 != NULL && v2 != NULL) {
		if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
			gint64 result = RLIB_VALUE_GET_AS_NUMBER(v1) + RLIB_VALUE_GET_AS_NUMBER(v2);
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
			return TRUE;
		}
		if(RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
			gchar *newstr = g_malloc(r_strlen(RLIB_VALUE_GET_AS_STRING(v1))+r_strlen(RLIB_VALUE_GET_AS_STRING(v2))+1);
			strcpy(newstr, RLIB_VALUE_GET_AS_STRING(v2));
			strcat(newstr, RLIB_VALUE_GET_AS_STRING(v1));
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, newstr));
			g_free(newstr);
			return TRUE;
		}
		if((RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_NUMBER(v2)) || (RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_DATE(v2))) {
			struct rlib_value *number, *date;
			struct rlib_datetime newday;
			if(RLIB_VALUE_IS_DATE(v1)) {
				date = v1;
				number = v2;
			} else {
				date = v2;
				number = v1;
			}
			newday = RLIB_VALUE_GET_AS_DATE(date);
			rlib_datetime_addto(&newday, RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(number)));
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &newday));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_execption(r,"ADD", 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}





#endif


gint rlib_pcode_operator_add(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(v1 != NULL && v2 != NULL) {
		if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
			gint64 result = RLIB_VALUE_GET_AS_NUMBER(v1) + RLIB_VALUE_GET_AS_NUMBER(v2);
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r, vs, rlib_value_new_number(&rval_rtn, result));
			return TRUE;
		}
		if(RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
			const gchar *safe1 =  RLIB_VALUE_GET_AS_STRING(v1) == NULL ? "" : RLIB_VALUE_GET_AS_STRING(v1);
			const gchar *safe2 =  RLIB_VALUE_GET_AS_STRING(v2) == NULL ? "" : RLIB_VALUE_GET_AS_STRING(v2);
			gchar *newstr = g_malloc(strlen(safe1)+strlen(safe2)+1);
			strcpy(newstr, safe2);
			strcat(newstr, safe1);
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r, vs, rlib_value_new_string(&rval_rtn, newstr));
			g_free(newstr);
			return TRUE;
		}
		if((RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_NUMBER(v2)) || (RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_DATE(v2))) {
			struct rlib_value *number, *date;
			struct rlib_datetime newday;
			if(RLIB_VALUE_IS_DATE(v1)) {
				date = v1;
				number = v2;
			} else {
				date = v2;
				number = v1;
			}
			newday = RLIB_VALUE_GET_AS_DATE(date);
			rlib_datetime_addto(&newday, RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(number)));
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &newday));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_execption(r, "ADD", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}


gint rlib_pcode_operator_subtract(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(v1 != NULL && v2 != NULL) {
		if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
			gint64 result = RLIB_VALUE_GET_AS_NUMBER(v2) - RLIB_VALUE_GET_AS_NUMBER(v1);
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
			return TRUE;
		} else if (RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
			struct rlib_datetime *dt1, *dt2;
			gint64 result = 0;
			gint daysdiff = 0;
			gint secsdiff = 0;
			gboolean usedays = FALSE;
			gboolean usesecs = FALSE;
			dt1 = &RLIB_VALUE_GET_AS_DATE(v1);
			dt2 = &RLIB_VALUE_GET_AS_DATE(v2);
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
				rlib_value_free(v1);
				rlib_value_free(v2);
				rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, LONG_TO_FXP_NUMBER(result)));
				return TRUE;
			}
		} else if (RLIB_VALUE_IS_DATE(v2) && RLIB_VALUE_IS_NUMBER(v1)) {
			struct rlib_value *number, *date;
			struct rlib_datetime newday;
			date = v2;
			number = v1;
			newday = RLIB_VALUE_GET_AS_DATE(date);
			rlib_datetime_addto(&newday, -RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(number)));
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &newday));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_execption(r,"SUBTRACT", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_multiply(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(v1 != NULL && v2 != NULL) {
		if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
			gint64 result = RLIB_FXP_MUL(RLIB_VALUE_GET_AS_NUMBER(v1), RLIB_VALUE_GET_AS_NUMBER(v2));
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_execption(r,"MULTIPLY", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_divide(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(v1 != NULL && v2 != NULL) {
		if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
			gint64 result = RLIB_FXP_DIV(RLIB_VALUE_GET_AS_NUMBER(v2), RLIB_VALUE_GET_AS_NUMBER(v1));
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_execption(r,"DIVIDE", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_mod(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(v1 != NULL && v2 != NULL) {
		if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
			gint64 result = RLIB_VALUE_GET_AS_NUMBER(v2) % RLIB_VALUE_GET_AS_NUMBER(v1);
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_execption(r,"MOD", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_pow(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(v1 != NULL && v2 != NULL) {
		if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
			gint64 result = pow(RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(v2)), RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(v1)));
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result*RLIB_DECIMAL_PRECISION));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_execption(r,"POW", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}


gint rlib_pcode_operator_lte(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);

	if (v1 != NULL && v2 != NULL) {
		if (RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
			if (RLIB_VALUE_GET_AS_NUMBER(v2) <= RLIB_VALUE_GET_AS_NUMBER(v1))	{
				rlib_value_free(v1);
				rlib_value_free(v2);
				rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
			} else {
				rlib_value_free(v1);
				rlib_value_free(v2);
				rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
			}
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
			if (r_strcmp(RLIB_VALUE_GET_AS_STRING(v2), RLIB_VALUE_GET_AS_STRING(v1)) <= 0) {
				rlib_value_free(v1);
				rlib_value_free(v2);
				rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
			} else {
				rlib_value_free(v1);
				rlib_value_free(v2);
				rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
			}
			return TRUE;
		}
		if (RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
			struct rlib_datetime *t1, *t2;
			gint64 val;
			t1 = &RLIB_VALUE_GET_AS_DATE(v1);
			t2 = &RLIB_VALUE_GET_AS_DATE(v2);
			val = (rlib_datetime_compare(t2, t1) <= 0)? RLIB_DECIMAL_PRECISION : 0;
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, val));
			return TRUE;
		}
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"<=", code, 2, v1, v2, NULL);
	return FALSE;
}


gint rlib_pcode_operator_lt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if (v1 != NULL && v2 != NULL) {
		if (RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
			if (RLIB_VALUE_GET_AS_NUMBER(v2) < RLIB_VALUE_GET_AS_NUMBER(v1))	{
				rlib_value_free(v1);
				rlib_value_free(v2);
				rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
			} else {
				rlib_value_free(v1);
				rlib_value_free(v2);
				rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
			}
			return TRUE;
		}
		if (RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
			if (r_strcmp(RLIB_VALUE_GET_AS_STRING(v2), RLIB_VALUE_GET_AS_STRING(v1)) < 0) {
				rlib_value_free(v1);
				rlib_value_free(v2);
				rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
			} else {
				rlib_value_free(v1);
				rlib_value_free(v2);
				rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
			}
			return TRUE;
		}
		if (RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
			struct rlib_datetime *t1, *t2;
			gint64 val;
			t1 = &RLIB_VALUE_GET_AS_DATE(v1);
			t2 = &RLIB_VALUE_GET_AS_DATE(v2);
			val = (rlib_datetime_compare(t2, t1) < 0)? RLIB_DECIMAL_PRECISION : 0;
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, val));
			return TRUE;
		}
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"<", code, 2, v1, v2, NULL);
	return FALSE;
}

gint rlib_pcode_operator_gte(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		if(RLIB_VALUE_GET_AS_NUMBER(v2) >= RLIB_VALUE_GET_AS_NUMBER(v1))	{
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	if(RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
		if(r_strcmp(RLIB_VALUE_GET_AS_STRING(v2), RLIB_VALUE_GET_AS_STRING(v1)) >= 0) {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
		struct rlib_datetime *t1, *t2;
		gint64 val;
		t1 = &RLIB_VALUE_GET_AS_DATE(v1);
		t2 = &RLIB_VALUE_GET_AS_DATE(v2);
		val = (rlib_datetime_compare(t2, t1) >= 0)? RLIB_DECIMAL_PRECISION : 0;
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, val));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,">=", code, 2, v1, v2, NULL);
	return FALSE;
}

gint rlib_pcode_operator_gt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		if(RLIB_VALUE_GET_AS_NUMBER(v2) > RLIB_VALUE_GET_AS_NUMBER(v1))	{
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	if (RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
		if (r_strcmp(RLIB_VALUE_GET_AS_STRING(v2), RLIB_VALUE_GET_AS_STRING(v1)) > 0) {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
		struct rlib_datetime *t1, *t2;
		gint64 val;
		t1 = &RLIB_VALUE_GET_AS_DATE(v1);
		t2 = &RLIB_VALUE_GET_AS_DATE(v2);
		val = (rlib_datetime_compare(t2, t1) > 0)? RLIB_DECIMAL_PRECISION : 0;
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, val));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,">", code, 2, v1, v2, NULL);
	return FALSE;
}

gint rlib_pcode_operator_eql(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		if(RLIB_VALUE_GET_AS_NUMBER(v2) == RLIB_VALUE_GET_AS_NUMBER(v1))	{
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	if(RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
		gint64 push;
		if(RLIB_VALUE_GET_AS_STRING(v2) == NULL && RLIB_VALUE_GET_AS_STRING(v1) == NULL)
			push = RLIB_DECIMAL_PRECISION;
		else if(RLIB_VALUE_GET_AS_STRING(v2) == NULL || RLIB_VALUE_GET_AS_STRING(v1) == NULL)
			push = 0;
		else {
			if(r_strcmp(RLIB_VALUE_GET_AS_STRING(v2), RLIB_VALUE_GET_AS_STRING(v1)) == 0) {
				push = RLIB_DECIMAL_PRECISION;
			} else {
				push = 0;
			}
		}
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_new_number(&rval_rtn, push);
		rlib_value_stack_push(r,vs, &rval_rtn);
		return TRUE;
	}
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
		struct rlib_datetime *t1, *t2;
		gint64 val;
		t1 = &RLIB_VALUE_GET_AS_DATE(v1);
		t2 = &RLIB_VALUE_GET_AS_DATE(v2);
		val = (rlib_datetime_compare(t2, t1) == 0)? RLIB_DECIMAL_PRECISION : 0;
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, val));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"==", code, 2, v1, v2, NULL);
	return FALSE;
}

gint rlib_pcode_operator_noteql(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		if(RLIB_VALUE_GET_AS_NUMBER(v2) != RLIB_VALUE_GET_AS_NUMBER(v1))	{
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	if(RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
		gint64 push;
		if(RLIB_VALUE_GET_AS_STRING(v2) == NULL && RLIB_VALUE_GET_AS_STRING(v1) == NULL)
			push = 0;
		else if(RLIB_VALUE_GET_AS_STRING(v2) == NULL || RLIB_VALUE_GET_AS_STRING(v1) == NULL)
			push = RLIB_DECIMAL_PRECISION;
		else {
			if(r_strcmp(RLIB_VALUE_GET_AS_STRING(v2), RLIB_VALUE_GET_AS_STRING(v1)) == 0) {
				push = 0;
			} else {
				push = RLIB_DECIMAL_PRECISION;
			}
		}
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_new_number(&rval_rtn, push);
		rlib_value_stack_push(r,vs, &rval_rtn);
		return TRUE;
	}
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
		struct rlib_datetime *t1, *t2;
		gint64 val;
		t1 = &RLIB_VALUE_GET_AS_DATE(v1);
		t2 = &RLIB_VALUE_GET_AS_DATE(v2);
		val = (rlib_datetime_compare(t2, t1) != 0)? RLIB_DECIMAL_PRECISION : 0;
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, val));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"!=", code, 2, v1, v2, NULL);
	return FALSE;
}

gint rlib_pcode_operator_logical_and(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		if(RLIB_VALUE_GET_AS_NUMBER(v2) && RLIB_VALUE_GET_AS_NUMBER(v1))	{
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		rlib_value_free(v1);
		rlib_value_free(v2);
		return TRUE;
	}
	if(RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
		if(RLIB_VALUE_GET_AS_STRING(v2) && RLIB_VALUE_GET_AS_STRING(v1)) {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"LOGICAL AND", code, 2, v1, v2, NULL);
	return FALSE;
}

gint rlib_pcode_operator_and(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);

	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		long v1_num = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(v1));
		long v2_num = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(v2));

		if(v2_num & v1_num)	{
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, (v2_num & v1_num)*RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"AND", code, 2, v1, v2, NULL);
	return FALSE;
}

gint rlib_pcode_operator_logical_or(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		if(RLIB_VALUE_GET_AS_NUMBER(v2) || RLIB_VALUE_GET_AS_NUMBER(v1))	{
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	if(RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
		if(RLIB_VALUE_GET_AS_STRING(v2) || RLIB_VALUE_GET_AS_STRING(v1)) {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, RLIB_DECIMAL_PRECISION));
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"LOGICAL OR", code, 2, v1, v2, NULL);
	return FALSE;
}

gint rlib_pcode_operator_or(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);

	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		long v1_num = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(v1));
		long v2_num = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(v2));

		if(v2_num | v1_num)	{
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, (v2_num | v1_num)*RLIB_DECIMAL_PRECISION));
		} else {
			rlib_value_free(v1);
			rlib_value_free(v2);
			rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, 0));
		}
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"OR", code, 2, v1, v2, NULL);
	return FALSE;
}

gint rlib_pcode_operator_abs(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
 	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1)) {
		gint64 result = abs(RLIB_VALUE_GET_AS_NUMBER(v1));
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"ABS", code, 1, v1, NULL, NULL);
	return FALSE;
}

gint rlib_pcode_operator_ceil(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
 	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1)) {
		gint64 dec = RLIB_VALUE_GET_AS_NUMBER(v1) % RLIB_DECIMAL_PRECISION;
		gint64 result = RLIB_VALUE_GET_AS_NUMBER(v1) - dec ;
		if(dec != 0)
			result += RLIB_DECIMAL_PRECISION;
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"CEIL", code, 1, v1, NULL, NULL);
	return FALSE;
}

gint rlib_pcode_operator_floor(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
 	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1)) {
		gint64 dec = RLIB_VALUE_GET_AS_NUMBER(v1) % RLIB_DECIMAL_PRECISION;
		gint64 result = RLIB_VALUE_GET_AS_NUMBER(v1) - dec;
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"FLOOR", code, 1, v1, NULL, NULL);
	return FALSE;
}

gint rlib_pcode_operator_round(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
 	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1)) {
		gint64 dec = RLIB_VALUE_GET_AS_NUMBER(v1) % RLIB_DECIMAL_PRECISION;
		gint64 result = RLIB_VALUE_GET_AS_NUMBER(v1);
		if(dec > 0) {
			result -= dec;
			if(dec >= (5 * RLIB_DECIMAL_PRECISION / 10))
				result += RLIB_DECIMAL_PRECISION;
		} else if(dec < 0) {
			result -= dec;
			if(dec <= (-5 * RLIB_DECIMAL_PRECISION / 10))
				result -= RLIB_DECIMAL_PRECISION;
		}

		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"ROUND", code, 1, v1, NULL, NULL);
	return FALSE;
}

gint rlib_pcode_operator_sin(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
 	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1)) {
		gdouble num = (double)RLIB_VALUE_GET_AS_NUMBER(v1) / (double)RLIB_DECIMAL_PRECISION;
		gint64 result = sin(num)*RLIB_DECIMAL_PRECISION;
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"sin", code, 1, v1, NULL, NULL);
	return FALSE;
}

gint rlib_pcode_operator_cos(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
 	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1)) {
		gdouble num = (double)RLIB_VALUE_GET_AS_NUMBER(v1) / (double)RLIB_DECIMAL_PRECISION;
		gint64 result = cos(num)*RLIB_DECIMAL_PRECISION;
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"cos", code, 1, v1, NULL, NULL);
	return FALSE;
}

gint rlib_pcode_operator_ln(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
 	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1)) {
		gdouble num = (double)RLIB_VALUE_GET_AS_NUMBER(v1) / (double)RLIB_DECIMAL_PRECISION;
		gint64 result = log(num)*RLIB_DECIMAL_PRECISION;
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"ln", code, 1, v1, NULL, NULL);
	return FALSE;
}

gint rlib_pcode_operator_exp(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
 	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1)) {
		gdouble num = (double)RLIB_VALUE_GET_AS_NUMBER(v1) / (double)RLIB_DECIMAL_PRECISION;
		gint64 result = exp(num)*RLIB_DECIMAL_PRECISION;
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"exp", code, 1, v1, NULL, NULL);
	return FALSE;
}

gint rlib_pcode_operator_atan(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
 	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1)) {
		gdouble num = (double)RLIB_VALUE_GET_AS_NUMBER(v1) / (double)RLIB_DECIMAL_PRECISION;
		gint64 result = atan(num)*RLIB_DECIMAL_PRECISION;
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"atan", code, 1, v1, NULL, NULL);
	return FALSE;
}

gint rlib_pcode_operator_sqrt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
 	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1)) {
		gdouble num = (double)RLIB_VALUE_GET_AS_NUMBER(v1) / (double)RLIB_DECIMAL_PRECISION;
		gint64 result = sqrt(num)*RLIB_DECIMAL_PRECISION;
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"sqrt", code, 1, v1, NULL, NULL);
	return FALSE;
}

gint rlib_pcode_operator_fxpval(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_STRING(v2)) {
		gint64 result = rlib_safe_atoll(RLIB_VALUE_GET_AS_STRING(v2));
		gint64 decplaces = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(v1));
		gint64 to_be_pushed_result;
		rlib_value_free(v1);
		rlib_value_free(v2);
		to_be_pushed_result = rlib_fxp_div(result, tentothe(decplaces), RLIB_FXP_PRECISION);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, to_be_pushed_result));
		return TRUE;
	}
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	rlib_pcode_operator_fatal_execption(r,"fxpval", code, 2, v1, v2, NULL);
	return FALSE;
}

gint rlib_pcode_operator_val(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_STRING(v1)) {
		gint64 val = rlib_str_to_long_long(r, RLIB_VALUE_GET_AS_STRING(v1));
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, val));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"val", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

/* TODO: REVISIT THIS */
gint rlib_pcode_operator_str(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	gchar fmtstring[20];
	gchar *dest;
	gint64 n1, n2;
	struct rlib_value *v1, *v2, *v3, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	v3 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2) && RLIB_VALUE_IS_NUMBER(v3)) {
		n1 = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(v1));
		n2 = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(v2));
		if(RLIB_VALUE_GET_AS_NUMBER(v1) > 0)
			sprintf(fmtstring, "%%%" PRId64 ".%" PRId64, n2, n1);
		else
			sprintf(fmtstring, "%%%" PRId64, n2);
		rlib_number_sprintf(r, &dest, fmtstring, v3, 0, "((NONE))", -1);
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_free(v3);
		rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, dest));
		g_free(dest);
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"str", code, 3, v1, v2, v3);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_free(v3);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

static gint rlib_pcode_operator_stod_common(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, int which) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_STRING(v1)) {
		struct rlib_datetime dt;
		gchar *tstr = RLIB_VALUE_GET_AS_STRING(v1);
		int err = FALSE;

		rlib_datetime_clear(&dt);
		if (which) { /* convert time */
			gint hour = 12, minute = 0, second = 0; /* safe time for date only. */
			gchar ampm = 'a';
			if(tstr != NULL) {
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
			if(tstr != NULL) {
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
			rlib_value_free(v1);
			rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &dt));
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_execption(r,"stod", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_stod(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	return rlib_pcode_operator_stod_common(r, code, vs, this_field_value, 0);
}

gint rlib_pcode_operator_tstod(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	return rlib_pcode_operator_stod_common(r, code, vs, this_field_value, 1);
}

gboolean rlib_pcode_operator_iif(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	gboolean thisresult = FALSE;
	struct rlib_value *v1;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_IIF(v1)) {
		struct rlib_pcode_if *rif = RLIB_VALUE_GET_AS_IIF(v1);
		struct rlib_value *result;

		if(rif->false == NULL || rif->true == NULL) {
			r_error(r, "IIF STATEMENT IS INVALID [%s] @ %d\n", code->infix_string, code->line_number);
		} else {
			execute_pcode(r, rif->evaulation, vs, this_field_value, FALSE);
			result = rlib_value_stack_pop(vs);

			if(RLIB_VALUE_IS_NUMBER(result)) {
				if(RLIB_VALUE_GET_AS_NUMBER(result) == 0) {
					rlib_value_free(result);
					rlib_value_free(v1);
					thisresult = execute_pcode(r, rif->false, vs, this_field_value, FALSE);

				} else {
					rlib_value_free(result);
					rlib_value_free(v1);
					thisresult = execute_pcode(r, rif->true, vs, this_field_value, FALSE);
				}
			} else if(RLIB_VALUE_IS_STRING(result)) {
				if(RLIB_VALUE_GET_AS_STRING(result) == NULL) {
					rlib_value_free(result);
					rlib_value_free(v1);
					thisresult = execute_pcode(r, rif->false, vs, this_field_value, FALSE);
				} else {
					rlib_value_free(result);
					rlib_value_free(v1);
					thisresult = execute_pcode(r, rif->true, vs, this_field_value, FALSE);
				}
			} else {
				rlib_value_free(result);
				rlib_value_free(v1);
				r_error(r, "CAN'T COMPARE IIF VALUE [%d]\n", RLIB_VALUE_GET_TYPE(result));
			}
		}
	}
	if (!thisresult)
		rlib_pcode_operator_fatal_execption(r,"iif", code, 1, v1, NULL, NULL);
	return thisresult;
}


static gboolean rlib_pcode_operator_dtos_common(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, const char *format) {
	struct rlib_value *v1, rval_rtn;
	gboolean result = FALSE;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_DATE(v1)) {
		gchar *buf = NULL;
		struct rlib_datetime *tmp = &RLIB_VALUE_GET_AS_DATE(v1);
		rlib_datetime_format(r, &buf, tmp, format);
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, buf));
		g_free(buf);
		result = TRUE;
	} else {
		rlib_pcode_operator_fatal_execption(r,"dtos", code, 1, v1, NULL, NULL);
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	}
	return result;
}


gboolean rlib_pcode_operator_dateof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value rval_rtn, *v1;

	v1 = rlib_value_stack_pop(vs);
	if (RLIB_VALUE_IS_DATE(v1)) {
		struct rlib_datetime dt = RLIB_VALUE_GET_AS_DATE(v1);
		rlib_datetime_clear_time(&dt);
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &dt));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"dateof", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}


gboolean rlib_pcode_operator_timeof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value rval_rtn, *v1;

	v1 = rlib_value_stack_pop(vs);
	if (RLIB_VALUE_IS_DATE(v1)) {
		struct rlib_datetime tm_date = RLIB_VALUE_GET_AS_DATE(v1);
		tm_date.date = *g_date_new();
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &tm_date));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"dateof", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}


/*
* function setdateof(date1, date2) - makes date of date1 = date of date2.
* 	time of date1 is unchanged.
*/
gint rlib_pcode_operator_chgdateof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
		struct rlib_datetime *pd1 = &RLIB_VALUE_GET_AS_DATE(v1);
		struct rlib_datetime d2 = RLIB_VALUE_GET_AS_DATE(v2);
		rlib_datetime_makesamedate(&d2, pd1);
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &d2));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"chgdateof", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}


gint rlib_pcode_operator_chgtimeof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
		struct rlib_datetime *pd1 = &RLIB_VALUE_GET_AS_DATE(v1);
		struct rlib_datetime d2 = RLIB_VALUE_GET_AS_DATE(v2);
		rlib_datetime_makesametime(&d2, pd1);
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &d2));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"chgtimeof", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}


gboolean rlib_pcode_operator_gettimeinsecs(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value rval_rtn, *v1;

	v1 = rlib_value_stack_pop(vs);
	if (RLIB_VALUE_IS_DATE(v1)) {
		struct rlib_datetime dt = RLIB_VALUE_GET_AS_DATE(v1);
		glong secs = rlib_datetime_time_as_long(&dt);
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, secs * RLIB_DECIMAL_PRECISION));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"gettimeinsecs", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}


gint rlib_pcode_operator_settimeinsecs(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v2 = rlib_value_stack_pop(vs);
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		struct rlib_datetime dt = RLIB_VALUE_GET_AS_DATE(v1);
		gint64 secs = RLIB_VALUE_GET_AS_NUMBER(v2);
		rlib_datetime_set_time_from_long(&dt, secs);
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &dt));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"settimefromsecs", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}


gboolean rlib_pcode_operator_dtos(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	return rlib_pcode_operator_dtos_common(r, code, vs, this_field_value, "%Y-%m-%d");
}


gboolean rlib_pcode_operator_dtosf(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value rval_rtn, *v1 = rlib_value_stack_pop(vs);
	gboolean result = FALSE;
	if (RLIB_VALUE_IS_STRING(v1)) {
		gchar *fmt = g_strdup(RLIB_VALUE_GET_AS_STRING(v1));
		rlib_value_free(v1);
		result = rlib_pcode_operator_dtos_common(r, code, vs, this_field_value, fmt);
		g_free(fmt);
	} else {
		rlib_pcode_operator_fatal_execption(r,"dtosf", code, 1, v1, NULL, NULL);
		rlib_value_free(v1);
		v1 = rlib_value_stack_pop(vs); /* clear the stack */
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	}
	return result;
}


/*
* Generic format function format(object, formatstring)
*/
gboolean rlib_pcode_operator_format(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	gchar current_locale[MAXSTRLEN];

	v2 = rlib_value_stack_pop(vs);
	v1 = rlib_value_stack_pop(vs);

	if(r->special_locale != NULL) {
		gchar *tmp;
		tmp = setlocale(LC_ALL, NULL);
		if(tmp == NULL)
			current_locale[0] = 0;
		else
			strcpy(current_locale, tmp);
		setlocale(LC_ALL, r->special_locale);
	}

	if (RLIB_VALUE_IS_STRING(v2)) {
		gchar buf[MAXSTRLEN];
		gchar *result = "!ERR_F_IV";
		gchar *fmt = RLIB_VALUE_GET_AS_STRING(v2);
		gboolean need_free = FALSE;
		if (*fmt == '!') {
			++fmt;
			buf[MAXSTRLEN - 1] = '\0'; /* Assure terminated */
			if(RLIB_VALUE_IS_STRING(v1)) {
				gchar *str = RLIB_VALUE_GET_AS_STRING(v1);
				if ((*fmt == '#') || (*fmt == '!')) {
					++fmt;
					sprintf(buf, fmt, str);
					result = buf;
				} else r_error(r, "Format type does not match variable type in 'format' function");
			} else if(RLIB_VALUE_IS_NUMBER(v1)) {
				gint64 num = RLIB_VALUE_GET_AS_NUMBER(v1);
				if (*fmt == '#') {
					++fmt;
					rlib_format_number(r, &result, fmt, num);
					need_free = TRUE;
				} else if(*fmt == '$') {
					++fmt;
					rlib_format_money(r, &result, fmt, num);
					need_free = TRUE;
				} else r_error(r, "Format type does not match variable type in 'format' function");
			} else if(RLIB_VALUE_IS_DATE(v1)) {
				if (*fmt == '@') {
					struct rlib_datetime dt = RLIB_VALUE_GET_AS_DATE(v1);
					++fmt;
					rlib_datetime_format(r, &result, &dt, fmt);
					need_free = TRUE;
				} else {
					r_error(r, "Format type does not match variable type in 'format' function");
				}
			} else {
				r_error(r, "Invalid variable type encountered in format");
			}
		}
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, result));
		if(need_free)
			g_free(result);
		setlocale(LC_ALL, current_locale);
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"format not string in format", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	setlocale(LC_ALL, current_locale);
	return FALSE;
}


gint rlib_pcode_operator_year(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_DATE(v1)) {
		gint64 tmp = g_date_get_year(&RLIB_VALUE_GET_AS_DATE(v1).date);
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, LONG_TO_FXP_NUMBER(tmp)));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"year", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}


gint rlib_pcode_operator_month(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_DATE(v1)) {
		int mon = g_date_get_month(&RLIB_VALUE_GET_AS_DATE(v1).date);
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, LONG_TO_FXP_NUMBER(mon)));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"month", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_day(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_DATE(v1)) {
		int day = g_date_get_day(&RLIB_VALUE_GET_AS_DATE(v1).date);
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, LONG_TO_FXP_NUMBER(day)));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"day", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_upper(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_STRING(v1)) {
		gchar *str = RLIB_VALUE_GET_AS_STRING(v1);
		gchar *tmp = "";

		if (str != NULL)
			tmp = r_strupr(str);

		rlib_value_free(v1);

		rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, tmp));
		if (str != NULL)
			g_free(tmp);

		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"upper", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_lower(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_STRING(v1)) {
		gchar *str = RLIB_VALUE_GET_AS_STRING(v1);
		gchar *tmp = "";

		if (str != NULL)
			tmp = r_strlwr(str);

		rlib_value_free(v1);

		rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, tmp));
		if (str != NULL)
			g_free(tmp);

		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"lower", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_strlen(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_STRING(v1) ) {
		gint64 slen=0;
		if( RLIB_VALUE_GET_AS_STRING(v1) != NULL )
			slen = strlen(RLIB_VALUE_GET_AS_STRING(v1));
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, LONG_TO_FXP_NUMBER(slen) ));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"strlen", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_left(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);

	if(RLIB_VALUE_IS_STRING(v2) && RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_GET_AS_STRING(v2) != NULL) {
		gchar *tmp = g_strdup(RLIB_VALUE_GET_AS_STRING(v2));
		gint n = RLIB_VALUE_GET_AS_NUMBER(v1)/RLIB_DECIMAL_PRECISION;
		if (n >= 0) {
			if (r_strlen(tmp) > n) *r_ptrfromindex(tmp, n) = '\0';
		}
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, tmp));
		g_free(tmp);
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"left", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_right(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	v2 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_STRING(v2) && RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_GET_AS_STRING(v2) != NULL) {
		gchar *tmp = g_strdup(RLIB_VALUE_GET_AS_STRING(v2));
		gint n = RLIB_VALUE_GET_AS_NUMBER(v1)/RLIB_DECIMAL_PRECISION;
		gint len = r_strlen(tmp);
		if (n >= 0) {
			if (n > len) n = len;
		}
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, r_ptrfromindex(tmp, len - n)));
		g_free(tmp);
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"right", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_substring(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, *v3, rval_rtn;
	v1 = rlib_value_stack_pop(vs); /* the new size */
	v2 = rlib_value_stack_pop(vs); /* the start idx */
	v3 = rlib_value_stack_pop(vs); /* the string */
	if(RLIB_VALUE_IS_STRING(v3) && RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		gchar *tmp = g_strdup(RLIB_VALUE_GET_AS_STRING(v3));
		gint st = RLIB_VALUE_GET_AS_NUMBER(v2)/RLIB_DECIMAL_PRECISION;
		gint sz = RLIB_VALUE_GET_AS_NUMBER(v1)/RLIB_DECIMAL_PRECISION;
		gint len = r_strlen(tmp);
		gint maxlen;
		if (st < 0) st = 0;
		if (st > len) st = len;
		maxlen = len - st;
		if (sz < 0) sz = maxlen;
		if (sz > maxlen) sz = maxlen;
		*r_ptrfromindex(tmp, st + sz) = '\0';
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_free(v3);
		rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, r_ptrfromindex(tmp, st)));
		g_free(tmp);
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"substring", code, 3, v1, v2, v3);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_free(v3);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_proper(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_STRING(v1)) {
		if(RLIB_VALUE_GET_AS_STRING(v1) == NULL || RLIB_VALUE_GET_AS_STRING(v1)[0] == '\0') {
			rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, ""));
			return TRUE;
		} else {
			gchar *tmp = strproper(RLIB_VALUE_GET_AS_STRING(v1));
			rlib_value_free(v1);
			rlib_value_stack_push(r,vs, rlib_value_new_string(&rval_rtn, tmp));
			g_free(tmp);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_execption(r,"proper", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_stodt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_STRING(v1)) {
		gint year=2000, month=1, day=1, hour=12, min=0, sec=0;
		struct rlib_datetime dt;
		gchar *str = RLIB_VALUE_GET_AS_STRING(v1);
		gint result;
		result = sscanf(str, "%4d%2d%2d%2d%2d%2d", &year, &month, &day, &hour, &min, &sec);
		rlib_datetime_clear(&dt);
		rlib_datetime_set_date(&dt, year, month, day);
		rlib_datetime_set_time(&dt, hour, min, sec);
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &dt));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"stodt", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_stodtsql(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_STRING(v1)) {
		gint year=2000, month=1, day=1, hour=12, min=0, sec=0;
		struct rlib_datetime dt;
		gchar *str = RLIB_VALUE_GET_AS_STRING(v1);
		gint result;
		result = sscanf(str, "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec);
		rlib_datetime_clear(&dt);
		rlib_datetime_set_date(&dt, year, month, day);
		rlib_datetime_set_time(&dt, hour, min, sec);
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &dt));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"stodt", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_isnull(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_STRING(v1)) {
		gint64 result = RLIB_VALUE_GET_AS_STRING(v1) == NULL ? 1 : 0;
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, result));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"isnull", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_dim(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_DATE(v1)) {
		struct rlib_datetime *request = &RLIB_VALUE_GET_AS_DATE(v1);
		gint dim = g_date_get_days_in_month(g_date_get_month(&request->date), g_date_get_year(&request->date));
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, LONG_TO_FXP_NUMBER(dim)));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"dim", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_wiy(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_DATE(v1)) {
		struct rlib_datetime *request = &RLIB_VALUE_GET_AS_DATE(v1);
		gint dim = g_date_get_sunday_week_of_year(&request->date);
		rlib_value_free(v1);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, LONG_TO_FXP_NUMBER(dim)));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"wiy", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_wiyo(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, *v2, rval_rtn;
	v2 = rlib_value_stack_pop(vs);
	v1 = rlib_value_stack_pop(vs);
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		struct rlib_datetime request = RLIB_VALUE_GET_AS_DATE(v1);
		gint dim;
		gint offset;
		offset = g_date_get_weekday(&request.date);
		offset = offset - RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(v2));
		if(offset < 0)
			offset += 7;
		g_date_subtract_days(&request.date, offset);
		dim = g_date_get_sunday_week_of_year(&request.date);
		rlib_value_free(v1);
		rlib_value_free(v2);
		rlib_value_stack_push(r,vs, rlib_value_new_number(&rval_rtn, LONG_TO_FXP_NUMBER(dim)));
		return TRUE;
	}
	rlib_pcode_operator_fatal_execption(r,"wiyo", code, 2, v1, v2, NULL);
	rlib_value_free(v1);
	rlib_value_free(v2);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}

gint rlib_pcode_operator_date(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_datetime dt;
	struct rlib_value rval_rtn;
	struct tm *tmp = localtime(&r->now);
	rlib_datetime_clear(&dt);
	rlib_datetime_set_date(&dt, tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday);
	rlib_datetime_set_time(&dt, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	rlib_value_stack_push(r,vs, rlib_value_new_date(&rval_rtn, &dt));
	return TRUE;
}

gint rlib_pcode_operator_eval(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct rlib_value *v1, rval_rtn;
	v1 = rlib_value_stack_pop(vs);
	if(v1 != NULL) {
		if(RLIB_VALUE_IS_STRING(v1)) {
			struct rlib_pcode *code = NULL;
			code = rlib_infix_to_pcode(r, NULL, NULL, RLIB_VALUE_GET_AS_STRING(v1), -1, TRUE);
			rlib_execute_pcode(r, &rval_rtn, code, this_field_value);
			rlib_pcode_free(code);
			rlib_value_free(v1);
			rlib_value_stack_push(r,vs,&rval_rtn);
			return TRUE;
		}
	}
	rlib_pcode_operator_fatal_execption(r,"eval", code, 1, v1, NULL, NULL);
	rlib_value_free(v1);
	rlib_value_stack_push(r,vs, rlib_value_new_error(&rval_rtn));
	return FALSE;
}
