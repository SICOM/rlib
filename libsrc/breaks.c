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

#include <stdlib.h>
#include <string.h>

#include <config.h>
#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"

static void rlib_print_break_header_output(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb, struct rlib_element *e) {
	gint blank = TRUE;
	gint i;

	if (!OUTPUT(r)->do_breaks)
		return;

	if (rb->suppressblank) {
		struct rlib_element *be;
		for (be = rb->fields; be != NULL; be = be->next) {
			struct rlib_break_fields *bf = be->data;
			struct rlib_value rval;

			rlib_execute_pcode(r, &rval, bf->code, NULL);
			if (rlib_value_is_empty(&rval) && blank == TRUE)
				blank = TRUE;
			else
				blank = FALSE;

			rlib_value_free(&rval);
		}
	}

	if (!rb->suppressblank || (rb->suppressblank && !blank)) {
		rb->didheader = TRUE;
		if (e != NULL) {
			for (i = 0; i < report->pages_across; i++) {
				OUTPUT(r)->set_working_page(r, part, i);
				OUTPUT(r)->start_report_break_header(r, part, report, rb);
			}

			rlib_layout_report_output(r, part, report, e, FALSE, TRUE);

			for (i = 0; i < report->pages_across; i++) {
				OUTPUT(r)->set_working_page(r, part, i);
				OUTPUT(r)->end_report_break_header(r, part, report, rb);
			}
		}
	} else {
		rb->didheader = FALSE;
	}
}

static void rlib_print_break_footer_output(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb, struct rlib_element *e) {
	gint i;
	
	if (!OUTPUT(r)->do_breaks)
		return;

	if (rb->didheader) {
		for (i = 0; i < report->pages_across; i++) {
			OUTPUT(r)->set_working_page(r, part, i);
			OUTPUT(r)->start_report_break_footer(r, part, report, rb);
		}

		r->use_cached_data++;
		rlib_layout_report_output(r, part, report, e, FALSE, TRUE);
		r->use_cached_data--;

		for (i = 0; i < report->pages_across; i++) {
			OUTPUT(r)->set_working_page(r, part, i);
			OUTPUT(r)->end_report_break_footer(r, part, report, rb);
		}
	}
}

gboolean rlib_force_break_headers(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	struct rlib_element *e;
	gboolean did_print = FALSE;

	if (!OUTPUT(r)->do_breaks)
		return TRUE;

	if (report->breaks == NULL)
		return TRUE;

	for (e = report->breaks; e != NULL; e = e->next) {
		gint dobreak = 1;
		struct rlib_report_break *rb = e->data;
		struct rlib_element *be;
		for (be = rb->fields; be != NULL; be = be->next) {
			if (dobreak && r->queries[r->current_result]->current_row < 0) {
				dobreak = 1;
			} else {
				dobreak = 0;
			}
		}
	}

	for (e = report->breaks; e != NULL; e = e->next) {
		struct rlib_report_break *rb = e->data;
		if (rb->headernewpage) {
			rlib_print_break_header_output(r, part, report, rb, rb->header);
			did_print = TRUE;
		}
	}
	return did_print;
}

void rlib_handle_break_headers(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	gint page, i;
	gfloat total[RLIB_MAXIMUM_PAGES_ACROSS];
	struct rlib_element *e;
	GSList *breaks = NULL;

	if (report->breaks == NULL)
		return;
	
	for (i = 0; i < RLIB_MAXIMUM_PAGES_ACROSS; i++)
		total[i] = 0;

	for (e = report->breaks; e != NULL; e = e->next) {
		gint dobreak = 1;
		struct rlib_report_break *rb = e->data;
		struct rlib_element *be;
		for (be = rb->fields; be != NULL; be = be->next) {
			struct rlib_break_fields *bf = be->data;
			struct rlib_value rval1, rval2;
			gint retval;

			r->use_cached_data++;
			rlib_execute_pcode(r, &rval1, bf->code, NULL);
			r->use_cached_data--;
			rlib_execute_pcode(r, &rval2, bf->code, NULL);
			retval = rvalcmp(r, &rval1, &rval2);

			if (dobreak && retval) {
				dobreak = 1;
			} else {
				dobreak = 0;
			}
			rlib_value_free(&rval1);
			rlib_value_free(&rval2);
		}

		if (dobreak) {
			if (rb->header != NULL) {
				breaks = g_slist_append(breaks, rb);
			} else {
				rb->didheader = TRUE;
			}
			for (page = 0; page < report->pages_across; page++) {
				total[page] += get_outputs_size(part, report, rb->header, page);
			}
		}
	}

	if (breaks != NULL && OUTPUT(r)->do_breaks) {
		gint allfit = TRUE;
		GSList *list;
		for (page = 0; page < report->pages_across; page++) {
			if (!rlib_will_this_fit(r, part, report, total[page], page + 1))
				allfit = FALSE;
		}
		if (!allfit) {
			rlib_layout_end_page(r, part, report, TRUE);
			if (rlib_force_break_headers(r, part, report) == FALSE) {
				for (list = breaks; list; list = list->next) {
					struct rlib_report_break *rb = list->data;
					rlib_print_break_header_output(r, part, report, rb, rb->header);
				}
			}
		} else {
			for (list = breaks; list; list = list->next) {
				struct rlib_report_break *rb = list->data;
				rlib_print_break_header_output(r, part, report, rb, rb->header);
			}
		}
	}
	g_slist_free(breaks);
}

/* TODO: Variables need to resolve the name into a number or something.. like break numbers for more efficient comparison */
static void reset_variables_on_break(struct rlib_report *report, gchar *name) {
	struct rlib_element *e;

	for (e = report->variables; e != NULL; e = e->next) {
		struct rlib_report_variable *rv = e->data;

		if (rv->xml_resetonbreak.xml != NULL && rv->xml_resetonbreak.xml[0] != '\0' && !strcmp((char *)rv->xml_resetonbreak.xml, name))
			variable_clear(rv, FALSE);
	}
}

static void rlib_break_all_below_in_reverse_order(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_element *e) {
	gint count = 0, i = 0, j = 0;
	gint newpage = FALSE;
	gboolean t;
	struct rlib_report_break *rb;
	struct rlib_element *xxx;
	struct rlib_break_fields *bf = NULL;

	for (xxx = e; xxx != NULL; xxx=xxx->next)
		count++;

	for (i = count; i > 0; i--) {
		xxx = e;
		for (j = 0; j < i - 1; j++)
			xxx = xxx->next;
		rb = xxx->data;

		if (OUTPUT(r)->do_breaks) {
			gint did_end_page = FALSE;

			did_end_page = rlib_end_page_if_line_wont_fit(r, part, report, rb->footer);

			rlib_process_expression_variables(r, report);
			if (bf != NULL && did_end_page) {
				rlib_print_break_header_output(r, part, report, rb, rb->header);
			}

			rlib_print_break_footer_output(r, part, report, rb, rb->footer);

			/*
			 * Finalize delayed data
			 */
			if (rb->delayed_data && OUTPUT(r)->finalize_text_delayed) {
				GSList *list;

				for (list = rb->delayed_data; list; list = list->next) {
					struct rlib_break_delayed_data *dd = list->data;
					GSList *list1;

					for (list1 = rb->variables; list1; list1 = list1->next) {
						struct rlib_report_variable *rv = list1->data;

						rlib_pcode_replace_variable_with_value(r, dd->delayed_data->extra_data->field_code, rv);
					}

					r->use_cached_data++;
					OUTPUT(r)->finalize_text_delayed(r, dd->delayed_data, dd->backwards);
					r->use_cached_data--;

					rlib_free_delayed_extra_data(r, dd->delayed_data);
					g_free(dd);
				}
			}
			g_slist_free(rb->delayed_data);
			rb->delayed_data = NULL;
		}

		reset_variables_on_break(report, (gchar *)rb->xml_name.xml);
		rlib_process_expression_variables(r, report);
		if (rlib_execute_as_boolean(r, rb->newpage_code, &t))
			newpage = t;
	}
	if (newpage && OUTPUT(r)->do_breaks) {
		if (!INPUT(r, r->current_result)->isdone(INPUT(r, r->current_result), r->results[r->current_result]->result)) {
			if (OUTPUT(r)->paginate)
				rlib_layout_end_page(r, part, report, TRUE);
			rlib_force_break_headers(r, part, report);
		}
	}
}

/*
 * Footers are complicated.... I need to go in reverse order for footers... and if I find a match...
 * I need to go back down the list and force breaks for everyone.. in reverse order.. ugh
 */
void rlib_handle_break_footers(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	struct rlib_element *e;
	struct rlib_break_fields *bf = NULL;

	if (report->breaks == NULL)
		return;

	for (e = report->breaks; e; e = e->next) {
		gint dobreak = 1;
		struct rlib_report_break *rb = e->data;
		struct rlib_element *be;
		for (be = rb->fields; be != NULL; be = be->next) {
			struct rlib_value rval_tmp;
			RLIB_VALUE_TYPE_NONE(&rval_tmp);
			bf = be->data;
			if (dobreak) {
				if (INPUT(r, r->current_result)->isdone(INPUT(r, r->current_result), r->results[r->current_result]->result)) {
					dobreak = 1;
				} else {
					struct rlib_value rval1, rval2;

					r->use_cached_data++;
					rlib_execute_pcode(r, &rval1, bf->code, NULL);
					r->use_cached_data--;
					rlib_execute_pcode(r, &rval2, bf->code, NULL);
					if (rvalcmp(r, &rval1, &rval2)) {
						dobreak = 1;
					} else  {
						dobreak = 0;
					}
					rlib_value_free(&rval1);
					rlib_value_free(&rval2);
				}
			} else {
				dobreak = 0;
			}
		}

		if (dobreak) {
			rlib_break_all_below_in_reverse_order(r, part, report, e);
			break;
		}
	}

	/*
	 * Originally the report variables in the break headers
	 * had to be precalculated to show correct values there.
	 * For that, the rlib_variables_precalculate() funtion
	 * was used, that is now gone. The side effect is that
	 * the "precalulation_done" signal would be lost, unless
	 * we emit it here in rlib_handle_break_footers().
	 *
	 * It can be debated whether it should be emitted from
	 * rlib_handle_break_headers(), because historically
	 * it was emitted around that time, but the variable
	 * values would not be correct that way in the current
	 * implementation.
	 */
	rlib_emit_signal(r, RLIB_SIGNAL_PRECALCULATION_DONE);
}

void rlib_break_evaluate_attributes(rlib *r, struct rlib_report *report) {
	struct rlib_report_break *rb;
	struct rlib_element *e;
	gint t;

	for (e = report->breaks; e != NULL; e = e->next) {
		rb = e->data;
		if (rlib_execute_as_boolean(r, rb->headernewpage_code, &t))
			rb->headernewpage = t;
		if (rlib_execute_as_boolean(r, rb->suppressblank_code, &t))
			rb->suppressblank = t;
	}
}
