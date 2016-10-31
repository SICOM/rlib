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
 * $Id$
 * 
 * This module generates a report from the information stored in the current
 * report object.
 * The main entry point is called once at report generation time for each
 * report defined in the rlib object.
 *
 */
 
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"
#include "rlib_langinfo.h"

void variable_clear(struct rlib_report_variable *rv, gboolean do_expression) {
	if (rv->type == RLIB_REPORT_VARIABLE_EXPRESSION && do_expression) {
		RLIB_VARIABLE_CA(rv)->amount = *rlib_value_new_number(&RLIB_VARIABLE_CA(rv)->amount, 0);
	} else if (rv->type == RLIB_REPORT_VARIABLE_COUNT) {
		RLIB_VARIABLE_CA(rv)->count = *rlib_value_new_number(&RLIB_VARIABLE_CA(rv)->count, 0);
	} else if (rv->type == RLIB_REPORT_VARIABLE_SUM) {
		RLIB_VARIABLE_CA(rv)->amount = *rlib_value_new_number(&RLIB_VARIABLE_CA(rv)->amount, 0);
	} else if (rv->type == RLIB_REPORT_VARIABLE_AVERAGE) {
		RLIB_VARIABLE_CA(rv)->count = *rlib_value_new_number(&RLIB_VARIABLE_CA(rv)->count, 0);
		RLIB_VARIABLE_CA(rv)->amount = *rlib_value_new_number(&RLIB_VARIABLE_CA(rv)->amount, 0);
	} else if (rv->type == RLIB_REPORT_VARIABLE_LOWEST) {
		RLIB_VARIABLE_CA(rv)->amount = *rlib_value_new_number(&RLIB_VARIABLE_CA(rv)->amount, 0);
	} else if (rv->type == RLIB_REPORT_VARIABLE_HIGHEST) {
		RLIB_VARIABLE_CA(rv)->amount = *rlib_value_new_number(&RLIB_VARIABLE_CA(rv)->amount, 0);
	}
} 

void init_variables(struct rlib_report *report) {
	struct rlib_element *e;
	for (e = report->variables; e != NULL; e = e->next) {
		struct rlib_report_variable *rv = e->data;
		variable_clear(rv, TRUE);
	}
}

void rlib_process_variables(rlib *r, struct rlib_report *report) {
	struct rlib_element *e;
	gboolean samerow = FALSE;

	if (report->uniquerow_code != NULL) {
		struct rlib_value tmp_rval;
		rlib_execute_pcode(r, &tmp_rval, report->uniquerow_code, NULL);
		if (rvalcmp(r, &tmp_rval, &report->uniquerow) == 0) {
			samerow = TRUE;
		} else {
			rlib_value_free(&report->uniquerow);
			report->uniquerow = tmp_rval;
		}
	}

	for (e = report->variables; e != NULL; e = e->next) {
		struct rlib_report_variable *rv = e->data;
		struct rlib_value *count = &RLIB_VARIABLE_CA(rv)->count;
		struct rlib_value *amount = &RLIB_VARIABLE_CA(rv)->amount;
		struct rlib_value execute_result, *er = NULL;
		gboolean ignore = FALSE;
		if (rv->code != NULL)
			er = rlib_execute_pcode(r, &execute_result, rv->code, NULL);

		/* rv->code was NULL or rlib_execute_pcode() returned error */
		if (er == NULL)
			continue;

		if (rv->ignore_code != NULL) {
			gboolean test_ignore;
			if (rlib_execute_as_boolean(r, rv->ignore_code, &test_ignore)) {
				ignore = test_ignore;
			}
		}
		
		if (samerow)
			ignore = TRUE;

		if (ignore == FALSE) {
			if (rv->type == RLIB_REPORT_VARIABLE_COUNT) {
				RLIB_VALUE_GET_AS_NUMBER(count) += RLIB_DECIMAL_PRECISION;
			} else if (rv->type == RLIB_REPORT_VARIABLE_EXPRESSION) {
				if (RLIB_VALUE_IS_NUMBER(er)) {
					rlib_value_free(amount);
					rlib_value_new_number(amount, RLIB_VALUE_GET_AS_NUMBER(er));
				} else if (RLIB_VALUE_IS_STRING(er)) {
					rlib_value_free(amount);
					rlib_value_new_string(amount, RLIB_VALUE_GET_AS_STRING(er));
				} else
					r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER OR STRING FOR RLIB_REPORT_VARIABLE_EXPRESSION\n");
			} else if (rv->type == RLIB_REPORT_VARIABLE_SUM) {
				if (RLIB_VALUE_IS_NUMBER(er))
					RLIB_VALUE_GET_AS_NUMBER(amount) += RLIB_VALUE_GET_AS_NUMBER(er);
				else
					r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER FOR RLIB_REPORT_VARIABLE_SUM\n");

			} else if (rv->type == RLIB_REPORT_VARIABLE_AVERAGE) {
				RLIB_VALUE_GET_AS_NUMBER(count) += RLIB_DECIMAL_PRECISION;
				if (RLIB_VALUE_IS_NUMBER(er))
					RLIB_VALUE_GET_AS_NUMBER(amount) += RLIB_VALUE_GET_AS_NUMBER(er);
				else
					r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER FOR RLIB_REPORT_VARIABLE_AVERAGE\n");
			} else if (rv->type == RLIB_REPORT_VARIABLE_LOWEST) {
				if (RLIB_VALUE_IS_NUMBER(er)) {
					if (RLIB_VALUE_GET_AS_NUMBER(er) < RLIB_VALUE_GET_AS_NUMBER(amount) || RLIB_VALUE_GET_AS_NUMBER(amount) == 0) /* TODO: EVIL HACK */
						RLIB_VALUE_GET_AS_NUMBER(amount) = RLIB_VALUE_GET_AS_NUMBER(er);
				} else
					r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER FOR RLIB_REPORT_VARIABLE_LOWEST\n");
			} else if (rv->type == RLIB_REPORT_VARIABLE_HIGHEST) {
				if (RLIB_VALUE_IS_NUMBER(er)) {
					if (RLIB_VALUE_GET_AS_NUMBER(er) > RLIB_VALUE_GET_AS_NUMBER(amount) || RLIB_VALUE_GET_AS_NUMBER(amount) == 0) /* TODO: EVIL HACK */
						RLIB_VALUE_GET_AS_NUMBER(amount) = RLIB_VALUE_GET_AS_NUMBER(er);
				} else
					r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER FOR RLIB_REPORT_VARIABLE_HIGHEST\n");
			}
		}
	}
}

void rlib_process_expression_variables(rlib *r, struct rlib_report *report) {
	struct rlib_element *e;
	for (e = report->variables; e != NULL; e = e->next) {
		struct rlib_report_variable *rv = e->data;
		struct rlib_value *amount = &RLIB_VARIABLE_CA(rv)->amount;
		struct rlib_value execute_result, *er = NULL;
		if (rv->type == RLIB_REPORT_VARIABLE_EXPRESSION) {
			if (rv->code != NULL)
				er = rlib_execute_pcode(r, &execute_result, rv->code, NULL);
			if (er && RLIB_VALUE_IS_NUMBER(er)) {
				rlib_value_free(amount);
				rlib_value_new_number(amount, RLIB_VALUE_GET_AS_NUMBER(er));
			} else if (er && RLIB_VALUE_IS_STRING(er)) {
				rlib_value_free(amount);
				rlib_value_new_string(amount, RLIB_VALUE_GET_AS_STRING(er));
			} else
				r_error(r, "rlib_process_variables EXPECTED TYPE NUMBER OR STRING FOR RLIB_REPORT_VARIABLE_EXPRESSION\n");
		}
	}
}
