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
 * General Public Liense for more details.
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

/* Not used: static struct rlib_rgb COLOR_BLACK = {0, 0, 0}; */

struct _rlib_format_table {
	gchar name[64];
	gint number;
} rlib_fomat_table[] =  {
	{ "PDF", RLIB_FORMAT_PDF},
	{ "HTML", RLIB_FORMAT_HTML},
	{ "TXT", RLIB_FORMAT_TXT},
	{ "CSV", RLIB_FORMAT_CSV},
	{ "XML", RLIB_FORMAT_XML},
	{ "", -1},
};

gint rlib_format_get_number(const gchar *name) {
	int i = 0;
	while (rlib_fomat_table[i].number != -1) {
		if (strcasecmp(rlib_fomat_table[i].name, name) == 0)
			return rlib_fomat_table[i].number;
		i++;
	}
	return -1;
}

const gchar * rlib_format_get_name(gint number) {
	int i = 0;
	while(rlib_fomat_table[i].number != -1) {
		if (rlib_fomat_table[i].number == number)
			return rlib_fomat_table[i].name;
		i++;
	}
	return "UNKNOWN";
}

static const gchar *orientations[] = {
	"",
	"portrait",
	"landscape",
	NULL
};

gint get_font_point(struct rlib_part *part, struct rlib_report *report, struct rlib_report_lines *rl) {
	gint use_font_point;
	
	if (rl->font_point > 0)
		use_font_point = rl->font_point;
	else if (report != NULL && report->font_size > 0)
		use_font_point = report->font_size;
	else
		use_font_point = part->font_size;

	return use_font_point;
}	


gint rlib_emit_signal(rlib *r, gint signal_number) {
	gboolean (*signal_function)(rlib *, gpointer) = r->signal_functions[signal_number].signal_function;
	gpointer data = r->signal_functions[signal_number].data;
	if (signal_function != NULL)
		return signal_function(r, data);
	else
		return FALSE;
}

void rlib_handle_page_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	gint i;

	for (i = 0; i < report->pages_across; i++) {
		report->bottom_size[i] = get_outputs_size(part, report, report->page_footer, i);
		report->position_bottom[i] -= report->bottom_size[i];
	}

	OUTPUT(r)->start_report_page_footer(r, part, report);
	rlib_layout_report_output(r, part, report, report->page_footer, TRUE, FALSE);
	OUTPUT(r)->end_report_page_footer(r, part, report);

	for (i = 0; i<report->pages_across; i++)
		report->position_bottom[i] -= report->bottom_size[i];
}

gdouble get_output_size(struct rlib_part *part, struct rlib_report *report, struct rlib_report_output_array *roa) {
	GSList *ptr;
	gdouble total = 0;

	if (roa->suppress == TRUE)
		return 0;

	for (ptr = roa->chain; ptr; ptr = g_slist_next(ptr)) {
		struct rlib_report_output *rd = ptr->data;
		if (rd->type == RLIB_REPORT_PRESENTATION_DATA_LINE) {
			struct rlib_report_lines *rl = rd->data;
			total += RLIB_GET_LINE(get_font_point(part, report, rl));
		} else if (rd->type == RLIB_REPORT_PRESENTATION_DATA_HR) {
			struct rlib_report_horizontal_line *rhl = rd->data;
			total += RLIB_GET_LINE(rhl->size);		
		}
	}
	return total;
}

gdouble get_outputs_size(struct rlib_part *part, struct rlib_report *report, struct rlib_element *e, gint page) {
	gdouble total = 0;
	struct rlib_report_output_array *roa;

	for(; e != NULL; e = e->next) {
		roa = e->data;
		if (roa->page == -1 || roa->page == page || roa->page == -1)
			total += get_output_size(part, report, roa);
	}			

	return total;
}

gboolean rlib_will_this_fit(rlib *r, struct rlib_part *part, struct rlib_report *report, gdouble total, gint page) {
	if (OUTPUT(r)->paginate == FALSE)
		return TRUE;

	if (report == NULL)
		return (part->position_top[page - 1] + total <= part->position_bottom[page - 1]);
	else
		return (report->position_top[page - 1] + total <= report->position_bottom[page - 1]);
}

gboolean will_outputs_fit(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_element *e, gint page) {
	gdouble size = 0;
	struct rlib_report_output_array *roa;

	if (OUTPUT(r)->paginate == FALSE)
		return TRUE;
	if (e == NULL)
		return TRUE;
	for (; e != NULL; e=e->next) {
		roa = e->data;
		if (page == -1 || page == roa->page || roa->page == -1) {
			size += get_output_size(part, report, roa);
		}
	}
	return rlib_will_this_fit(r, part, report, size, page);
}

void set_report_from_part(struct rlib_part *part, struct rlib_report *report, gdouble top_margin_offset) {
	gint i;
	for (i = 0; i < report->pages_across; i++) {
		report->position_top[i] = report->top_margin + part->position_top[0] + top_margin_offset;
		report->bottom_size[i] = part->bottom_size[0];
		report->position_bottom[i] = part->position_bottom[0];
	}

}

gboolean rlib_end_page_if_line_wont_fit(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_element *e) {
	gint i;
	gboolean fits = TRUE;

	for (i = 0; i < report->pages_across; i++) {
		if (!will_outputs_fit(r,part, report, e, i + 1))
			fits = FALSE;
	}
	if (!fits)
		rlib_layout_end_page(r, part, report, TRUE);
	return !fits;
}

gboolean rlib_fetch_first_rows(rlib *r) {
	rlib_navigate_start(r, r->current_result);
	return rlib_navigate_next(r, r->current_result);
}

static void rlib_evaluate_report_attributes(rlib *r, struct rlib_report *report) {
	gint64 t;
	gboolean b;
	gdouble f;

	if (rlib_execute_as_int64_inlist(r, report->orientation_code, &t, orientations))
		if ((t == RLIB_ORIENTATION_PORTRAIT) || (t == RLIB_ORIENTATION_LANDSCAPE))
			report->orientation = t;
	if (rlib_execute_as_int64(r, report->font_size_code, &t))
		report->font_size = t;
	if (report->is_the_only_report) {
		report->top_margin = 0;
		report->bottom_margin = 0;
		report->left_margin = 0;	
	} else {
		if (rlib_execute_as_double(r, report->top_margin_code, &f))
			report->top_margin = f;
		if (rlib_execute_as_double(r, report->left_margin_code, &f))
			report->left_margin = f;
		if (rlib_execute_as_double(r, report->bottom_margin_code, &f))
			report->bottom_margin = f;
	}
	if (rlib_execute_as_int64(r, report->pages_across_code, &t))
		report->pages_across = t;
	if (rlib_execute_as_boolean(r, report->suppress_page_header_first_page_code, &b))
		report->suppress_page_header_first_page = b;
	if (rlib_execute_as_boolean(r, report->suppress_code, &b))
		report->suppress = b;
	report->detail_columns = 1;
	if (rlib_execute_as_int64(r, report->detail_columns_code, &t))
		report->detail_columns = t;

	report->column_pad = 0;
	if (rlib_execute_as_double(r, report->column_pad_code, &f))
		report->column_pad = f;
}

static void rlib_evaulate_part_attributes(rlib *r, struct rlib_part *part) {
	gint64 t;
	gdouble f;
	char buf[MAXSTRLEN];
	
	if (rlib_execute_as_int64_inlist(r, part->orientation_code, &t, orientations))
		if ((t == RLIB_ORIENTATION_PORTRAIT) || (t == RLIB_ORIENTATION_LANDSCAPE))
			part->orientation = t;
	if (rlib_execute_as_int64(r, part->font_size_code, &t))
		part->font_size = t;
	if (rlib_execute_as_double(r, part->top_margin_code, &f))
		part->top_margin = f;
	if (rlib_execute_as_double(r, part->left_margin_code, &f))
		part->left_margin = f;
	if (rlib_execute_as_double(r, part->bottom_margin_code, &f))
		part->bottom_margin = f;
	if (rlib_execute_as_double(r, part->pages_across_code, &f))
		part->pages_across = f;
	if (rlib_execute_as_int64(r, part->suppress_page_header_first_page_code, &t))
		part->suppress_page_header_first_page = t;
	if (rlib_execute_as_int64(r, part->suppress_code, &t))
		part->suppress = t;

	if (rlib_execute_as_string(r, part->paper_type_code, buf, MAXSTRLEN)) {
		struct rlib_paper *paper = layout_get_paper_by_name(buf);
		if (paper != NULL)
			part->paper = paper;
	}
}

/// TODO
static void rlib_layout_report_delayed_data(rlib *r, struct rlib_report *report) {
	GSList *ptr;

	for (ptr = report->delayed_data; ptr; ptr = ptr->next) {
		struct rlib_break_delayed_data *dd = ptr->data;
		struct rlib_pcode *p = dd->delayed_data->extra_data->field_code;
		GSList *list = NULL;

		if (rlib_pcode_has_variable(r, p, NULL, &list, FALSE)) {
			GSList *ptr1;

			for (ptr1 = list; ptr1; ptr1 = ptr1->next) {
				struct rlib_report_variable *rv = ptr1->data;
				rlib_pcode_replace_variable_with_value(r, p, rv);
			}
		}

		r->use_cached_data++;
		OUTPUT(r)->finalize_text_delayed(r, dd->delayed_data, dd->backwards);
		r->use_cached_data--;

		g_free(dd);
	}
	g_slist_free(report->delayed_data);
	report->delayed_data = NULL;
}

static gboolean rlib_layout_report(rlib *r, struct rlib_part *part, struct rlib_report *report, gdouble left_margin_offset, gdouble top_margin_offset) {
	gboolean processed_variables;
	gint query_i, i;
	char query[MAXSTRLEN];
	gint64 report_percent;
	gdouble at_least = 0.0, origional_position_top = 0.0, report_header_advance = 0.0;
	gint iterations;

	OUTPUT(r)->start_report(r, part, report);

	report->query_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_query.xml, report->xml_query.line, TRUE);
	r->current_result = 0;
	if (report->query_code != NULL) {
		rlib_execute_as_string(r, report->query_code, query, MAXSTRLEN);
		for (query_i = 0; query_i < r->queries_count; query_i++) {
			if (query != NULL && r->results[query_i]->name != NULL && !strcmp(r->results[query_i]->name, query)) {
				r->current_result = query_i;		
				break;
			}
		}
	} else {
		r->current_result = 0;
	}

	rlib_emit_signal(r, RLIB_SIGNAL_REPORT_START);
	if (!part->has_only_one_report)
		rlib_resolve_report_fields(r, part, report);

	rlib_resolve_breaks(r, part, report);

	for (iterations = 0; iterations < report->iterations; iterations++) {
		if (r->queries_count <= 0) {
			rlib_evaluate_report_attributes(r, report);
			if (report->suppress == TRUE) {
				OUTPUT(r)->end_report(r, part, report);
				return FALSE;
			}
			set_report_from_part(part, report, top_margin_offset);
			report->left_margin += left_margin_offset + part->left_margin;
			OUTPUT(r)->start_report_header(r, part, report);
			rlib_layout_report_output(r, part, report, report->report_header, FALSE, TRUE);
			OUTPUT(r)->end_report_header(r, part, report);
			OUTPUT(r)->start_report_no_data(r, part, report);
			rlib_layout_report_output(r, part, report, report->alternate, FALSE, TRUE);
			OUTPUT(r)->end_report_no_data(r, part, report);
		} else {
			rlib_fetch_first_rows(r);
			if (!part->has_only_one_report) {
				init_variables(r, report);
				rlib_process_variables(r, report);
			}

			processed_variables = TRUE;
			rlib_evaluate_report_attributes(r, report);
			if (report->suppress == TRUE) {
				OUTPUT(r)->end_report(r, part, report);
				return FALSE;
			}
			
			set_report_from_part(part, report, top_margin_offset);
			report->left_margin += left_margin_offset + part->left_margin;
			if (report->font_size != -1) {
				r->font_point = report->font_size;
				OUTPUT(r)->set_font_point(r, r->font_point);
			}
			if (rlib_execute_as_int64(r, report->height_code, &report_percent))
				at_least = (part->position_bottom[0] - part->position_top[0]) * ((gdouble)report_percent/100);
			origional_position_top = report->position_top[0];

			OUTPUT(r)->start_report_header(r, part, report);
			rlib_layout_report_output(r, part, report, report->report_header, FALSE, TRUE);
			OUTPUT(r)->end_report_header(r, part, report);
			

			report_header_advance = (report->position_top[0] - origional_position_top );
			rlib_layout_init_report_page(r, part, report);
			r->detail_line_count = 0;
			if (report->font_size != -1) {
				r->font_point = report->font_size;
				OUTPUT(r)->set_font_point(r, r->font_point);
			}

			if (report->chart) {
				gdouble top;
				if (OUTPUT(r)->do_graph == TRUE) {
					top_margin_offset += report_header_advance;
					top_margin_offset += rlib_chart(r, part, report, left_margin_offset, &top_margin_offset);
					top = report->position_top[0];
					rlib_layout_report_footer(r, part, report);	
					top_margin_offset += report->position_top[0] - top;
				}
			} else if (report->graph) {
				gdouble top;
				if (OUTPUT(r)->do_graph == TRUE) {
					top_margin_offset += report_header_advance;
					top_margin_offset += rlib_graph(r, part, report, left_margin_offset, &top_margin_offset);
					top = report->position_top[0];
					rlib_layout_report_footer(r, part, report);	
					top_margin_offset += report->position_top[0] - top;
				}
			} else {
				rlib_fetch_first_rows(r);
				if (!INPUT(r, r->current_result)->isdone(INPUT(r, r->current_result), r->results[r->current_result]->result)) {
					while (1) {
						struct rlib_element *detail_fields = (report && report->detail ? report->detail->fields : NULL);
						gint output_count = 0;
						gdouble position_top = report->position_top[0];

						if (report->detail_columns > 1 && r->detail_line_count == 0) {
							/*if (OUTPUT(r)->table_around_multiple_detail_columns) {
								OUTPUT(r)->start_tr(r);
							}*/
						}

						if (!processed_variables)
							rlib_process_variables(r, report);

						rlib_break_evaluate_attributes(r, report);
						rlib_handle_break_headers(r, part, report);

						if (rlib_end_page_if_line_wont_fit(r, part, report, detail_fields))
							rlib_force_break_headers(r, part, report);

						if (report->detail_columns > 1) {
							/*if (OUTPUT(r)->table_around_multiple_detail_columns) {
								OUTPUT(r)->start_td(r, part, 0, 0, 0, 0, 0, NULL);
							}*/
						}

						if (OUTPUT(r)->do_breaks) {
							for (i = 0; i < report->pages_across; i++) {
								OUTPUT(r)->set_working_page(r, part, i);
								OUTPUT(r)->start_report_field_details(r, part, report);	
							}

							output_count = rlib_layout_report_output(r, part, report, detail_fields, FALSE, FALSE);

							for (i = 0; i < report->pages_across; i++) {
								OUTPUT(r)->set_working_page(r, part, i);
								OUTPUT(r)->end_report_field_details(r, part, report);
							}
						} else {
							output_count = rlib_layout_report_output_with_break_headers(r, part, report, TRUE);
						}

						if (output_count > 0)
							r->detail_line_count++;

						rlib_emit_signal(r, RLIB_SIGNAL_ROW_CHANGE);

						if (rlib_navigate_next(r, r->current_result) == FALSE) {
							rlib_handle_break_footers(r, part, report);
							break;
						}

						rlib_break_evaluate_attributes(r, report);
						rlib_handle_break_footers(r, part, report);
						processed_variables = FALSE;

						if (report->detail_columns > 1) {
							/*if (OUTPUT(r)->table_around_multiple_detail_columns) {
								OUTPUT(r)->end_td(r);
							}*/
						}

						if (report->detail_columns > 1) {
							if (r->detail_line_count % report->detail_columns != 0) {
								if (report->position_top[0] > position_top)
									report->position_top[0] = position_top;
								else
									report->position_top[0] = position_top = part->position_top[0];
							} else {
								/*if (OUTPUT(r)->table_around_multiple_detail_columns) {
									OUTPUT(r)->end_tr(r);
									OUTPUT(r)->start_tr(r);
								}*/
							}
						}
					}
				}
				rlib_layout_report_footer(r, part, report);
			}
		}

		if (at_least > 0) {
			gdouble used = (report->position_bottom[0]-origional_position_top)-(report->position_bottom[0]-report->position_top[0]);
			if (used < at_least) {
				for (i = 0; i < report->pages_across; i++)
					report->position_top[i] += (at_least-used);
			}
		}
		rlib_emit_signal(r, RLIB_SIGNAL_REPORT_ITERATION);
		OUTPUT(r)->end_report(r, part, report);
		rlib_layout_report_delayed_data(r, report);
	}
	return TRUE;
}

struct rlib_report_position {
	long page;
	gdouble position_top;
};

void rlib_layout_part_td(rlib *r, struct rlib_part *part, GSList *part_deviations, long page_number, gdouble position_top, struct rlib_report_position *rrp) {
	GSList *element;
	gdouble paper_width = layout_get_page_width(part) - (part->left_margin * 2);
	gdouble running_left_margin = 0;
	
	for (element = part_deviations; element != NULL; element = g_slist_next(element)) {
		struct rlib_part_td *td = element->data;
		gdouble running_top_margin = 0;
		gint i;
		gint64 width, height, border_width;
		gchar border_color[MAXSTRLEN];
		struct rlib_rgb bgcolor;
		GSList *report_element;

		if (!rlib_execute_as_int64(r, td->width_code, &width))
			width = 100;

		if (!rlib_execute_as_int64(r, td->height_code, &height))
			height = 0;

		if (!rlib_execute_as_int64(r, td->border_width_code, &border_width))
			border_width = 0;
			
		if (!rlib_execute_as_string(r, td->border_color_code, border_color, MAXSTRLEN))
			border_color[0] = 0;

		rlib_parsecolor(&bgcolor, border_color);
		
		for (i = 0; i < part->pages_across; i++) {
			OUTPUT(r)->set_working_page(r, part, i);
			OUTPUT(r)->start_part_td(r, part, width, height);
			OUTPUT(r)->start_part_pages_across(r, part, running_left_margin+part->left_margin, layout_get_next_line_by_font_point(part, running_top_margin+position_top+part->position_top[0], 0), width,  height, border_width, border_color[0] == 0 ? NULL : &bgcolor);
		}

		for (report_element = td->reports; report_element != NULL; report_element = g_slist_next(report_element)) {
			struct rlib_report *report = report_element->data;
			if (report != NULL) {
				gboolean ran_report;
				report->page_width = (((gdouble)width/100) * paper_width);
				OUTPUT(r)->set_raw_page(r, part, page_number);
				report->raw_page_number = page_number;
				ran_report = rlib_layout_report(r, part, report, running_left_margin, running_top_margin + position_top);
				if (ran_report) {
					running_top_margin = report->position_top[0] - part->position_top[0];
					if (report->raw_page_number > rrp->page) {
						rrp->page = report->raw_page_number;
						rrp->position_top = report->position_top[0];
					} else if (report->raw_page_number == rrp->page) {
						if (report->position_top[0] > rrp->position_top)
							rrp->position_top = report->position_top[0];
					}
				}
			}
		}
		running_left_margin += (((gdouble)width / 100) * paper_width);
		for (i = 0; i < part->pages_across; i++) {
			OUTPUT(r)->set_working_page(r, part, i);
			OUTPUT(r)->end_part_td(r, part);
			OUTPUT(r)->end_part_pages_across(r, part);
		}
	}
}

static void rlib_layout_part_tr(rlib *r, struct rlib_part *part) {
	struct rlib_report_position rrp;
	gint i;
	char buf[MAXSTRLEN];
	GSList *element;
	memset(&rrp, 0, sizeof(rrp));

	for (i = 0; i < part->pages_across; i++) {
		OUTPUT(r)->set_working_page(r, part, i);
		OUTPUT(r)->start_part_table(r, part);
	}
	
	for (element = part->part_rows; element != NULL; element = g_slist_next(element)) {
		struct rlib_part_tr *tr = element->data;
		gdouble save_position_top = 0;
		long save_page_number;
		gboolean newpage;

		for (i = 0; i < part->pages_across; i++) {
			OUTPUT(r)->set_working_page(r, part, i);
			OUTPUT(r)->start_part_tr(r, part);
		}

		if (rlib_execute_as_boolean(r, tr->newpage_code, &newpage)) {
			if (newpage && OUTPUT(r)->paginate) {
				OUTPUT(r)->end_page(r, part);
				rlib_layout_init_part_page(r, part, NULL, FALSE, TRUE);
				memset(&rrp, 0, sizeof(rrp));
			}
		}

		save_page_number = r->current_page_number;

		if (rrp.position_top > 0)
			save_position_top = rrp.position_top - part->position_top[0];

		tr->layout = RLIB_LAYOUT_FLOW;
		if (rlib_execute_as_string(r, tr->layout_code, buf, MAXSTRLEN) && strcmp(buf, "fixed") == 0)
			tr->layout = RLIB_LAYOUT_FIXED;

		memset(&rrp, 0, sizeof(rrp));

		rlib_layout_part_td(r, part, tr->part_deviations, save_page_number, save_position_top, &rrp);

		for (i = 0; i < part->pages_across; i++) {
			OUTPUT(r)->set_working_page(r, part, i);
			OUTPUT(r)->end_part_tr(r, part);
		}
		
	}	

	for (i = 0; i < part->pages_across; i++) {
		OUTPUT(r)->set_working_page(r, part, i);
		OUTPUT(r)->end_part_table(r, part);
	}
}

/*
	This is so single reports that use variable in report header and page header still work (With parts)
*/
gint rlib_evaulate_single_report_variables(rlib *r, struct rlib_part *part) {
	GSList *element, *part_deviations, *element2;

	for (element = part->part_rows; element != NULL; element = g_slist_next(element)) {
		struct rlib_part_tr *tr = element->data;
		part_deviations = tr->part_deviations;
		for (element2 = part_deviations; element2 != NULL; element2 = g_slist_next(element2)) {
			struct rlib_part_td *td = element2->data;
			GSList *report_element;		
			for (report_element = td->reports; report_element != NULL; report_element = g_slist_next(report_element)) {
				struct rlib_report *report = report_element->data;
				char query[MAXSTRLEN];
				gint i;

				report->query_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_query.xml, report->xml_query.line, TRUE);
				rlib_value_init(r, &report->uniquerow);

				r->current_result = 0;
				if (report->query_code != NULL) {
					rlib_execute_as_string(r, report->query_code, query, MAXSTRLEN);
					for (i = 0; i < r->queries_count; i++) {
						if (!strcmp(r->results[i]->name, query)) {
							r->current_result = i;		
							break;
						}
					}
				} else {
					r->current_result = 0;
				}

				rlib_resolve_report_fields(r, part, report);
				rlib_pcode_free(r, report->query_code);

				init_variables(r, report);
				rlib_process_variables(r, report);
				part->only_report = report;
			}
		}
	}
	return TRUE;
}

gint rlib_make_report(rlib *r) {
	gint i = 0;
	gint iterations;

	if (r->format == RLIB_FORMAT_HTML) {
		rlib_html_new_output_filter(r);
	} else if (r->format == RLIB_FORMAT_TXT) {
		rlib_txt_new_output_filter(r);
	} else if (r->format == RLIB_FORMAT_XML) {
		rlib_xml_new_output_filter(r);
	} else if (r->format == RLIB_FORMAT_CSV) {
		gchar *param;
		rlib_csv_new_output_filter(r);
		param = g_hash_table_lookup(r->output_parameters, "do_breaks");
		if (param != NULL && strcmp(param, "yes") == 0)
			OUTPUT(r)->do_breaks = TRUE;
	} else
		rlib_pdf_new_output_filter(r);
	r->current_font_point = -1;

	OUTPUT(r)->set_fg_color(r, -1, -1, -1);
	OUTPUT(r)->set_bg_color(r, -1, -1, -1);

	r->current_page_number = 0;
	r->current_result = 0;
	r->start_of_new_report = TRUE;
	OUTPUT(r)->start_rlib_report(r);

	for (i = 0; i < r->parts_count; i++) {
		struct rlib_part *part = r->parts[i];
		rlib_fetch_first_rows(r);
		if (part->has_only_one_report) 
			rlib_evaulate_single_report_variables(r, part);
		rlib_resolve_part_fields(r, part);

		for (iterations = 0; iterations < part->iterations; iterations++) {
			rlib_fetch_first_rows(r);
			rlib_evaulate_part_attributes(r, part);
			if (part->suppress == FALSE) {
				OUTPUT(r)->start_part(r, part);
				rlib_layout_init_part_page(r, part, NULL, TRUE, TRUE);
				rlib_layout_part_tr(r, part);
				if (part->delayed_data && OUTPUT(r)->finalize_text_delayed) {
					GSList *list;

					for (list = part->delayed_data; list; list = list->next) {
						struct rlib_break_delayed_data *dd = list->data;

						r->use_cached_data++;
						OUTPUT(r)->finalize_text_delayed(r, dd->delayed_data, dd->backwards);
						r->use_cached_data--;

						g_free(dd);
					}
				}
				g_slist_free(part->delayed_data);
				part->delayed_data = NULL;

				OUTPUT(r)->end_part(r, part);
				OUTPUT(r)->end_page(r, part);

				rlib_emit_signal(r, RLIB_SIGNAL_PART_ITERATION);
			}
		}
		rlib_emit_signal(r, RLIB_SIGNAL_REPORT_DONE);
	}
	OUTPUT(r)->end_rlib_report(r);

	return 0;
}

gint rlib_finalize(rlib *r) {
	OUTPUT(r)->finalize_private(r);
	return 0;
}
