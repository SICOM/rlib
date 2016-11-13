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
 *
 * This module generates a report from the information stored in the current
 * report object.
 * The main entry point is called once at report generation time for each
 * report defined in the rlib object.
 */

#include <config.h>
 
#include <stdlib.h>
#include <string.h>
#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"
#include "rlib_langinfo.h"

void variable_clear(rlib *r, struct rlib_report_variable *rv, gboolean do_expression) {
	switch (rv->type) {
	case RLIB_REPORT_VARIABLE_EXPRESSION:
		if (do_expression) {
			if (RLIB_VALUE_IS_NUMBER(r, &rv->amount))
				mpfr_set_ui(rv->amount.mpfr_value, 0, MPFR_RNDN);
			else {
				rlib_value_free(r, &rv->amount);
				rlib_value_new_number_from_long(r, &rv->amount, 0);
			}
		}
		break;
	case RLIB_REPORT_VARIABLE_COUNT:
		mpfr_set_ui(rv->count.mpfr_value, 0, MPFR_RNDN);
		break;
	case RLIB_REPORT_VARIABLE_SUM:
		if (RLIB_VALUE_IS_NUMBER(r, &rv->amount))
			mpfr_set_ui(rv->amount.mpfr_value, 0, MPFR_RNDN);
		else {
			rlib_value_free(r, &rv->amount);
			rlib_value_new_number_from_long(r, &rv->amount, 0);
		}
		break;
	case RLIB_REPORT_VARIABLE_AVERAGE:
		mpfr_set_ui(rv->count.mpfr_value, 0, MPFR_RNDN);
		if (RLIB_VALUE_IS_NUMBER(r, &rv->amount))
			mpfr_set_ui(rv->amount.mpfr_value, 0, MPFR_RNDN);
		else {
			rlib_value_free(r, &rv->amount);
			rlib_value_new_number_from_long(r, &rv->amount, 0);
		}
		break;
	case RLIB_REPORT_VARIABLE_LOWEST:
	case RLIB_REPORT_VARIABLE_HIGHEST:
		rlib_value_free(r, &rv->amount);
		break;
	}
}

void init_variables(rlib *r, struct rlib_report *report) {
	struct rlib_element *e;
	for (e = report->variables; e != NULL; e = e->next) {
		struct rlib_report_variable *rv = e->data;
		variable_clear(r, rv, TRUE);
	}
}

void rlib_process_variables(rlib *r, struct rlib_report *report) {
	struct rlib_element *e;
	gboolean samerow = FALSE;

	if (report->uniquerow_code != NULL) {
		struct rlib_value tmp_rval;
		rlib_value_init(r, &tmp_rval);
		rlib_execute_pcode(r, &tmp_rval, report->uniquerow_code, NULL);
		if (rvalcmp(r, &tmp_rval, &report->uniquerow) == 0) {
			rlib_value_free(r, &tmp_rval);
			samerow = TRUE;
		} else {
			rlib_value_free(r, &report->uniquerow);
			report->uniquerow = tmp_rval;
		}
	}

	for (e = report->variables; e != NULL; e = e->next) {
		struct rlib_report_variable *rv = e->data;
		struct rlib_value *count = &rv->count;
		struct rlib_value *amount = &rv->amount;
		struct rlib_value execute_result, *er = NULL;
		gboolean ignore = FALSE;

		rlib_value_init(r, &execute_result);

		if (rv->code != NULL)
			er = rlib_execute_pcode(r, &execute_result, rv->code, NULL);

		/* rv->code was NULL or rlib_execute_pcode() returned error */
		if (er == NULL) {
			rlib_value_init(r, &execute_result);
			continue;
		}

		if (rv->ignore_code != NULL) {
			gboolean test_ignore;
			if (rlib_execute_as_boolean(r, rv->ignore_code, &test_ignore)) {
				ignore = test_ignore;
			}
		}

		if (samerow)
			ignore = TRUE;

		if (ignore) {
			rlib_value_free(r, er);
			continue;
		}

		switch (rv->type) {
		case RLIB_REPORT_VARIABLE_COUNT:
			mpfr_add_ui(count->mpfr_value, count->mpfr_value, 1, MPFR_RNDN);
			break;
		case RLIB_REPORT_VARIABLE_EXPRESSION:
			if (RLIB_VALUE_IS_NUMBER(r, er)) {
				rlib_value_free(r, amount);
				rlib_value_new_number_from_mpfr(r, amount, er->mpfr_value);
			} else if (RLIB_VALUE_IS_STRING(r, er)) {
				rlib_value_free(r, amount);
				rlib_value_new_string(r, amount, RLIB_VALUE_GET_AS_STRING(r, er));
			} else
				r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER OR STRING FOR RLIB_REPORT_VARIABLE_EXPRESSION\n");
			break;
		case RLIB_REPORT_VARIABLE_SUM:
			if (RLIB_VALUE_IS_NUMBER(r, er)) {
				if (RLIB_VALUE_IS_NONE(r, amount)) {
					rlib_value_new_number_from_mpfr(r, amount, er->mpfr_value);
					fprintf(stderr, "rlib_process_variables SUM RLIB_VALUE_IS_NONE\n");
				} else
					mpfr_add(amount->mpfr_value, amount->mpfr_value, er->mpfr_value, MPFR_RNDN);
				mpfr_fprintf(stderr, "rlib_process_variables SUM '%s' %Rf\n", rv->xml_name.xml, amount->mpfr_value);
			} else
				r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER FOR RLIB_REPORT_VARIABLE_SUM\n");
			break;
		case RLIB_REPORT_VARIABLE_AVERAGE:
			mpfr_add_ui(count->mpfr_value, count->mpfr_value, 1, MPFR_RNDN);
			if (RLIB_VALUE_IS_NUMBER(r, er)) {
				if (RLIB_VALUE_IS_NONE(r, amount))
					rlib_value_new_number_from_mpfr(r, amount, er->mpfr_value);
				else
					mpfr_add(amount->mpfr_value, amount->mpfr_value, er->mpfr_value, MPFR_RNDN);
			} else
				r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER FOR RLIB_REPORT_VARIABLE_AVERAGE\n");
			break;
		case RLIB_REPORT_VARIABLE_LOWEST:
			if (RLIB_VALUE_IS_NUMBER(r, er)) {
				if (RLIB_VALUE_IS_NONE(r, amount))
					rlib_value_new_number_from_mpfr(r, amount, er->mpfr_value);
				else if (mpfr_cmp(er->mpfr_value, amount->mpfr_value) < 0)
					mpfr_set(amount->mpfr_value, er->mpfr_value, MPFR_RNDN);
			} else
				r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER FOR RLIB_REPORT_VARIABLE_LOWEST\n");
			break;
		case RLIB_REPORT_VARIABLE_HIGHEST:
			if (RLIB_VALUE_IS_NUMBER(r, er)) {
				if (RLIB_VALUE_IS_NONE(r, amount))
					rlib_value_new_number_from_mpfr(r, amount, er->mpfr_value);
				else if (mpfr_cmp(er->mpfr_value, amount->mpfr_value) > 0)
					mpfr_set(amount->mpfr_value, er->mpfr_value, MPFR_RNDN);
			} else
				r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER FOR RLIB_REPORT_VARIABLE_HIGHEST\n");
			break;
		default:
			r_error(r, "rlib_process_variables UNKNOWN REPORT VARIABLE TYPE: %d\n", rv->type);
			break;
		}
		rlib_value_free(r, er);
	}
}

void rlib_process_expression_variables(rlib *r, struct rlib_report *report) {
	struct rlib_element *e;
	for (e = report->variables; e != NULL; e = e->next) {
		struct rlib_report_variable *rv = e->data;
		struct rlib_value *amount = &rv->amount;
		struct rlib_value execute_result, *er = NULL;
		if (rv->type == RLIB_REPORT_VARIABLE_EXPRESSION) {
			rlib_value_init(r, &execute_result);
			if (rv->code != NULL)
				er = rlib_execute_pcode(r, &execute_result, rv->code, NULL);
			if (!er) {
				rlib_value_free(r, &execute_result);
				continue;
			}

			if (RLIB_VALUE_IS_NUMBER(r, er)) {
				if (RLIB_VALUE_IS_NUMBER(r, amount))
					mpfr_set(amount->mpfr_value, er->mpfr_value, MPFR_RNDN);
				else {
					rlib_value_free(r, amount);
					rlib_value_new_number_from_mpfr(r, amount, er->mpfr_value);
				}
			} else if (RLIB_VALUE_IS_STRING(r, er)) {
				rlib_value_free(r, amount);
				rlib_value_new_string(r, amount, RLIB_VALUE_GET_AS_STRING(r, er));
			} else
				r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER OR STRING FOR RLIB_REPORT_VARIABLE_EXPRESSION\n");
			rlib_value_free(r, er);
		}
	}
}
