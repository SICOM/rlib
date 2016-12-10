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

#include <string.h>
#include <ctype.h>

#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"

/*
	RLIB needs to find direct pointer to result sets and fields ahead of time so
	during executing time things are fast.. The following functions will resolve
	fields and variables into almost direct pointers

	Rlib variables are internal variables to rlib that it exports to you to use in reports
	Right now I only export pageno, value (which is a pointer back to the field value resolved), lineno, detailcnt
	Rlib variables should be referecned as r.whatever
	In this case r.pageno

*/
gint resolve_rlib_variable(gchar *name) {
	if(r_strlen(name) >= 3 && name[0] == 'r' && name[1] == '.') {
		name += 2;
		if(!strcmp(name, "pageno"))
			return RLIB_RLIB_VARIABLE_PAGENO;
		else if(!strcmp(name, "totpages"))
			return RLIB_RLIB_VARIABLE_TOTPAGES;
		else if(!strcmp(name, "value"))
			return RLIB_RLIB_VARIABLE_VALUE;
		else if(!strcmp(name, "lineno"))
			return RLIB_RLIB_VARIABLE_LINENO;
		else if(!strcmp(name, "detailcnt"))
			return RLIB_RLIB_VARIABLE_DETAILCNT;
		else if(!strcmp(name, "format"))
			return RLIB_RLIB_VARIABLE_FORMAT;
	}
	return 0;
}

gchar *rlib_resolve_field_value(rlib *r, struct rlib_resultset_field *rf) {
	struct input_filter *rs = INPUT(r, rf->resultset);
	gsize slen, elen;
	gchar *ptr = NULL;
	const gchar *str;

	if (r->queries[rf->resultset]->navigation_failed == TRUE)
		return NULL;

	if (rf->field != NULL)
		str = rs->get_field_value_as_string(rs, r->queries[rf->resultset]->result, rf->field);
	else
		str = "";
	if (str == NULL)
		return g_strdup("");
	else {
		slen = strlen(str);
		elen = MAXSTRLEN;
		rlib_charencoder_convert(rs->info.encoder, &str, &slen, &ptr, &elen);
		return ptr;
	}
}

gint rlib_lookup_result(rlib *r, gchar *name) {
	gint i;
	for (i = 0; i < r->queries_count; i++) {
		if (r->queries[i]->name != NULL) {
			if (!strcmp(r->queries[i]->name, name))
				return i;
		}
	}
	return -1;
}

gboolean rlib_resolve_resultset_field(rlib *r, char *name, void **rtn_field, gint *rtn_resultset) {
	gint resultset = 0;
	gboolean found = FALSE;
	gchar *right_side = NULL, *result_name = NULL;

	if (r->queries == NULL)
		return FALSE;

	resultset = r->current_result;
	right_side = memchr(name, '.', r_strlen(name));
	if (right_side != NULL) {
		gint t;
		result_name = g_malloc(r_strlen(name) - r_strlen(right_side) + 1);
		memcpy(result_name, name, r_strlen(name) - r_strlen(right_side));
		result_name[r_strlen(name) - r_strlen(right_side)] = '\0';
		right_side++;
		name = right_side;
		t = rlib_lookup_result(r, result_name);
		if (t >= 0) {
			found = TRUE;
			resultset = t;
		} else {
			if(!isdigit((int)*result_name)) {
				r_error(r, "rlib_resolve_namevalue: INVALID RESULT SET %s, name was [%s]\n", result_name, name);
				g_free(result_name);
				return FALSE;
			}
		}
	}
	*rtn_field = INPUT(r, resultset)->resolve_field_pointer(INPUT(r, resultset), r->queries[resultset]->result, name);

	if(*rtn_field != NULL)
		found = TRUE;
	else {
		if(result_name == NULL)
			r_error(r, "The field [%s] does not exist\n", name);
		else
			r_error(r, "The field [%s.%s] does not exist\n", result_name, name);
	}
	*rtn_resultset = resultset;

	g_free(result_name);
	
	return found;
}

static void rlib_field_resolve_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_field *rf) {
	rf->code = rlib_infix_to_pcode(r, part, report, rf->value, rf->value_line_number, TRUE);
	rf->format_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_format.xml, rf->xml_format.line, TRUE);
	rf->link_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_link.xml, rf->xml_link.line, TRUE);
	rf->translate_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_translate.xml, rf->xml_translate.line, TRUE);
	rf->color_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_color.xml, rf->xml_color.line, TRUE);
	rf->bgcolor_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_bgcolor.xml, rf->xml_bgcolor.line, TRUE);
	rf->col_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_col.xml, rf->xml_col.line, TRUE);
	rf->width_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_width.xml, rf->xml_width.line, TRUE);
	rf->bold_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_bold.xml, rf->xml_bold.line, TRUE);
	rf->italics_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_italics.xml, rf->xml_italics.line, TRUE);
	rf->align_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_align.xml, rf->xml_align.line, TRUE);
	rf->memo_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_memo.xml, rf->xml_memo.line, TRUE);
	rf->memo_max_lines_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_memo_max_lines.xml, rf->xml_memo_max_lines.line, TRUE);
	rf->memo_wrap_chars_code = rlib_infix_to_pcode(r, part, report, (gchar *)rf->xml_memo_wrap_chars.xml, rf->xml_memo_wrap_chars.line, TRUE);
	rf->width = -1;
/*	r_error(r, "DUMPING PCODE FOR [%s]\n", rf->value); 
	rlib_pcode_dump(r, rf->code,0);	
	r_error(r, "\n\n");  */
}

static void rlib_literal_resolve_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_literal *rt) {
	rt->color_code = rlib_infix_to_pcode(r, part, report, (gchar *)rt->xml_color.xml, rt->xml_color.line, TRUE);
	rt->bgcolor_code = rlib_infix_to_pcode(r, part, report, (gchar *)rt->xml_bgcolor.xml, rt->xml_bgcolor.line, TRUE);
	rt->link_code = rlib_infix_to_pcode(r, part, report, (gchar *)rt->xml_link.xml, rt->xml_link.line, TRUE);
	rt->translate_code = rlib_infix_to_pcode(r, part, report, (gchar *)rt->xml_translate.xml, rt->xml_translate.line, TRUE);
	rt->col_code = rlib_infix_to_pcode(r, part, report, (gchar *)rt->xml_col.xml, rt->xml_col.line, TRUE);
	rt->width_code = rlib_infix_to_pcode(r, part, report, (gchar *)rt->xml_width.xml, rt->xml_width.line, TRUE);
	rt->bold_code = rlib_infix_to_pcode(r, part, report, (gchar *)rt->xml_bold.xml, rt->xml_bold.line, TRUE);
	rt->italics_code = rlib_infix_to_pcode(r, part, report, (gchar *)rt->xml_italics.xml, rt->xml_italics.line, TRUE);
	rt->align_code = rlib_infix_to_pcode(r, part, report, (gchar *)rt->xml_align.xml, rt->xml_align.line, TRUE);
	rt->width = -1;
/*	rlogit("DUMPING PCODE FOR [%s]\n", rt->value); 
	rlib_pcode_dump(rf->code,0);	
	rlogit("\n\n"); */
}

static void rlib_break_resolve_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_break_fields *bf) {
	if(bf->xml_value.xml == NULL)
		r_error(r, "RLIB ERROR: BREAK FIELD VALUE CAN NOT BE NULL\n");
	bf->code = rlib_infix_to_pcode(r, part, report, (gchar *)bf->xml_value.xml, bf->xml_value.line, TRUE);
}

static void rlib_variable_resolve_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_variable *rv) {
	struct rlib_pcode *code;
	gboolean t;

	rv->code = rlib_infix_to_pcode(r, part, report, (gchar *)rv->xml_value.xml, rv->xml_value.line, TRUE);
	rv->ignore_code = rlib_infix_to_pcode(r, part, report, (gchar *)rv->xml_ignore.xml, rv->xml_ignore.line, TRUE);

	code = rlib_infix_to_pcode(r, part, report, (gchar *)rv->xml_precalculate.xml, rv->xml_precalculate.line, TRUE);
	if (rlib_execute_as_boolean(r, code, &t)) {
		rv->precalculate = t;
	} else {
		rv->precalculate = FALSE;
	}
	rlib_pcode_free(r, code);
}

static void rlib_hr_resolve_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_horizontal_line * rhl) {
	gdouble f;
	rhl->size = 0;

	rhl->indent_code = rlib_infix_to_pcode(r, part, report, (gchar *)rhl->xml_indent.xml, rhl->xml_indent.line, TRUE);
	rhl->length_code = rlib_infix_to_pcode(r, part, report, (gchar *)rhl->xml_length.xml, rhl->xml_length.line, TRUE);
	rhl->bgcolor_code = rlib_infix_to_pcode(r, part, report, (gchar *)rhl->xml_bgcolor.xml, rhl->xml_bgcolor.line, TRUE);
	rhl->suppress_code = rlib_infix_to_pcode(r, part, report, (gchar *)rhl->xml_suppress.xml, rhl->xml_suppress.line, TRUE);
	rhl->size_code = rlib_infix_to_pcode(r, part, report, (gchar *)rhl->xml_size.xml, rhl->xml_size.line, TRUE);

	if (rlib_execute_as_double(r, rhl->size_code, &f))
		rhl->size = f;
	
	rhl->length = 0;
	if (rlib_execute_as_double(r, rhl->length_code, &f))
		rhl->length = f;

	rhl->indent = 0;
	if (rlib_execute_as_double(r, rhl->indent_code, &f))
		rhl->indent = f;
}

static void rlib_image_resolve_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_image * ri) {
	ri->value_code = rlib_infix_to_pcode(r, part, report, (gchar *)ri->xml_value.xml, ri->xml_value.line, TRUE);
	ri->type_code = rlib_infix_to_pcode(r, part, report, (gchar *)ri->xml_type.xml, ri->xml_type.line, TRUE);
	ri->width_code = rlib_infix_to_pcode(r, part, report, (gchar *)ri->xml_width.xml, ri->xml_width.line, TRUE);
	ri->height_code = rlib_infix_to_pcode(r, part, report, (gchar *)ri->xml_height.xml, ri->xml_height.line, TRUE);
	ri->textwidth_code = rlib_infix_to_pcode(r, part, report, (gchar *)ri->xml_textwidth.xml, ri->xml_textwidth.line, TRUE);
}

static void rlib_barcode_resolve_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_barcode * rb) {
	rb->value_code = rlib_infix_to_pcode(r, part, report, (gchar *)rb->xml_value.xml, rb->xml_value.line, TRUE);
	rb->type_code = rlib_infix_to_pcode(r, part, report, (gchar *)rb->xml_type.xml, rb->xml_type.line, TRUE);
	rb->width_code = rlib_infix_to_pcode(r, part, report, (gchar *)rb->xml_width.xml, rb->xml_width.line, TRUE);
	rb->height_code = rlib_infix_to_pcode(r, part, report, (gchar *)rb->xml_height.xml, rb->xml_height.line, TRUE);
}


static void rlib_resolve_fields2(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_output_array *roa) {
	GSList *ptr;
	struct rlib_element *e;
	
	if (roa == NULL)
		return;
		
	if (roa->xml_page.xml != NULL)
		roa->page = atol((char *)roa->xml_page.xml);
	else
		roa->page = -1;

	if (roa->xml_suppress.xml != NULL) {
		struct rlib_pcode *code = rlib_infix_to_pcode(r, part, report, (gchar *)roa->xml_suppress.xml, roa->xml_suppress.line, TRUE);
		roa->suppress = atol((char *)roa->xml_suppress.xml);
		if (rlib_execute_as_boolean(r, code, &roa->suppress) == FALSE)
			roa->suppress = FALSE;
		rlib_pcode_free(r, code);
	} else
		roa->suppress = FALSE;
	
	for (ptr = roa->chain; ptr; ptr = g_slist_next(ptr)) {
		struct rlib_report_output *ro = ptr->data;

		if (ro->type == RLIB_REPORT_PRESENTATION_DATA_LINE) {
			struct rlib_report_lines *rl = ro->data;	
			e = rl->e;
			rl->bgcolor_code = rlib_infix_to_pcode(r, part, report, (gchar *)rl->xml_bgcolor.xml, rl->xml_bgcolor.line, TRUE);
			rl->color_code = rlib_infix_to_pcode(r, part, report, (gchar *)rl->xml_color.xml, rl->xml_color.line, TRUE);
			rl->suppress_code = rlib_infix_to_pcode(r, part, report, (gchar *)rl->xml_suppress.xml, rl->xml_suppress.line, TRUE);
			rl->bold_code = rlib_infix_to_pcode(r, part, report, (gchar *)rl->xml_bold.xml, rl->xml_bold.line, TRUE);
			rl->italics_code = rlib_infix_to_pcode(r, part, report, (gchar *)rl->xml_italics.xml, rl->xml_italics.line, TRUE);

			for (; e != NULL; e = e->next) {
				if (e->type == RLIB_ELEMENT_FIELD) {
					rlib_field_resolve_pcode(r, part, report, ((struct rlib_report_field *)e->data));
				} else if(e->type == RLIB_ELEMENT_LITERAL) {
					rlib_literal_resolve_pcode(r, part, report, ((struct rlib_report_literal *)e->data));
				} else if(e->type == RLIB_ELEMENT_IMAGE) {
					rlib_image_resolve_pcode(r, part, report, ((struct rlib_report_image *)e->data));
				} else if(e->type == RLIB_ELEMENT_BARCODE) {
					rlib_barcode_resolve_pcode(r, part, report, ((struct rlib_report_barcode *)e->data));
				}
			}
		} else if(ro->type == RLIB_REPORT_PRESENTATION_DATA_HR) {
			rlib_hr_resolve_pcode(r, part, report, ((struct rlib_report_horizontal_line *)ro->data));
		} else if(ro->type == RLIB_REPORT_PRESENTATION_DATA_IMAGE) {
			rlib_image_resolve_pcode(r, part, report, ((struct rlib_report_image *)ro->data));
		}
	}
}

static void rlib_resolve_outputs(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_element *e) {
	struct rlib_report_output_array *roa;
	for(; e != NULL; e = e->next) {
		roa = e->data;
		rlib_resolve_fields2(r, part, report, roa);
	}
}

/*
 * Report variables are refereced as v.whatever
 * but when created in the <variables/> section they use there normal name.. ie.. whatever
 */
struct rlib_report_variable *rlib_resolve_variable(rlib *r, struct rlib_part *part UNUSED, struct rlib_report *report, gchar *name) {
	struct rlib_element *e;
	if (report == NULL && part != NULL)
		report = part->only_report;
	if (report == NULL)
		return NULL;
	if (r_strlen(name) >= 3 && name[0] == 'v' && name[1] == '.') {
		name += 2;
		for (e = report->variables; e != NULL; e = e->next) {
			struct rlib_report_variable *rv = e->data;
			if (!strcmp(name, (char *)rv->xml_name.xml))
				return rv;
		}
		rlogit(r, "rlib_resolve_variable: Could not find [%s]\n", name);
	}
	return NULL;
}

int is_true_str(const gchar *str) {
	if (str == NULL) return FALSE;
	return (!strcasecmp(str, "yes") || !strcasecmp(str, "true"))? TRUE : FALSE;
}

void rlib_resolve_graph(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_graph *graph) {
	struct rlib_graph_plot *plot;
	GSList *list;

	if (graph == NULL)
		return;

	graph->name_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_name.xml, graph->xml_name.line, TRUE);
	graph->type_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_type.xml, graph->xml_type.line, TRUE);
	graph->subtype_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_subtype.xml, graph->xml_subtype.line, TRUE);
	graph->width_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_width.xml, graph->xml_width.line, TRUE);
	graph->height_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_height.xml, graph->xml_height.line, TRUE);
	graph->title_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_title.xml, graph->xml_title.line, TRUE);
	graph->bold_titles_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_bold_titles.xml, graph->xml_bold_titles.line, TRUE);
	graph->legend_bg_color_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_legend_bg_color.xml, graph->xml_legend_bg_color.line, TRUE);
	graph->legend_orientation_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_legend_orientation.xml, graph->xml_legend_orientation.line, TRUE);
	graph->draw_x_line_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_draw_x_line.xml, graph->xml_draw_x_line.line, TRUE);
	graph->draw_y_line_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_draw_y_line.xml, graph->xml_draw_y_line.line, TRUE);
	graph->grid_color_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_grid_color.xml, graph->xml_grid_color.line, TRUE);
	graph->x_axis_title_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_x_axis_title.xml, graph->xml_x_axis_title.line, TRUE);
	graph->y_axis_title_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_y_axis_title.xml, graph->xml_y_axis_title.line, TRUE);
	graph->y_axis_mod_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_y_axis_mod.xml, graph->xml_y_axis_mod.line, TRUE);
	graph->y_axis_title_right_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_y_axis_title_right.xml, graph->xml_y_axis_title_right.line, TRUE);
	graph->y_axis_decimals_code = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_y_axis_decimals.xml, graph->xml_y_axis_decimals.line, TRUE);
	graph->y_axis_decimals_code_right = rlib_infix_to_pcode(r, part, report, (gchar *)graph->xml_y_axis_decimals_right.xml, graph->xml_y_axis_decimals_right.line, TRUE);

	for(list=graph->plots;list != NULL; list = g_slist_next(list)) {
		plot = list->data;
		plot->axis_code = rlib_infix_to_pcode(r, part, report, (gchar *)plot->xml_axis.xml, plot->xml_axis.line, TRUE);
		plot->field_code = rlib_infix_to_pcode(r, part, report, (gchar *)plot->xml_field.xml, plot->xml_field.line, TRUE);
		plot->label_code = rlib_infix_to_pcode(r, part, report, (gchar *)plot->xml_label.xml, plot->xml_label.line, TRUE);
		plot->side_code = rlib_infix_to_pcode(r, part, report, (gchar *)plot->xml_side.xml, plot->xml_side.line, TRUE);
		plot->disabled_code = rlib_infix_to_pcode(r, part, report, (gchar *)plot->xml_disabled.xml, plot->xml_disabled.line, TRUE);
		plot->color_code = rlib_infix_to_pcode(r, part, report, (gchar *)plot->xml_color.xml, plot->xml_color.line, TRUE);
	}
}

void rlib_resolve_chart(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_chart *chart) {
	if (chart == NULL)
		return;

	chart->name_code = rlib_infix_to_pcode(r, part, report, (gchar *)chart->xml_name.xml, chart->xml_name.line, TRUE);
	chart->title_code = rlib_infix_to_pcode(r, part, report, (gchar *)chart->xml_title.xml, chart->xml_title.line, TRUE);
	chart->cols_code = rlib_infix_to_pcode(r, part, report, (gchar *)chart->xml_cols.xml, chart->xml_cols.line, TRUE);
	chart->rows_code = rlib_infix_to_pcode(r, part, report, (gchar *)chart->xml_rows.xml, chart->xml_rows.line, TRUE);
	chart->cell_width_code = rlib_infix_to_pcode(r, part, report, (gchar *)chart->xml_cell_width.xml, chart->xml_cell_width.line, TRUE);
	chart->cell_height_code = rlib_infix_to_pcode(r, part, report, (gchar *)chart->xml_cell_height.xml, chart->xml_cell_height.line, TRUE);
	chart->cell_width_padding_code = rlib_infix_to_pcode(r, part, report, (gchar *)chart->xml_cell_width_padding.xml, chart->xml_cell_width_padding.line, TRUE);
	chart->cell_height_padding_code = rlib_infix_to_pcode(r, part, report, (gchar *)chart->xml_cell_height_padding.xml, chart->xml_cell_height_padding.line, TRUE);
	chart->label_width_code = rlib_infix_to_pcode(r, part, report, (gchar *)chart->xml_label_width.xml, chart->xml_label_width.line, TRUE);
	chart->header_row_code = rlib_infix_to_pcode(r, part, report, (gchar *)chart->xml_header_row.xml, chart->xml_header_row.line, TRUE);

	if (chart->header_row) {
		struct rlib_chart_header_row *header_row = chart->header_row;

		header_row->query_code = rlib_infix_to_pcode(r, part, report, (gchar *)header_row->xml_query.xml, header_row->xml_query.line, TRUE);
		header_row->field_code = rlib_infix_to_pcode(r, part, report, (gchar *)header_row->xml_field.xml, header_row->xml_field.line, TRUE);
		header_row->colspan_code = rlib_infix_to_pcode(r, part, report, (gchar *)header_row->xml_colspan.xml, header_row->xml_colspan.line, TRUE);
	}

	if (chart->row) {
		struct rlib_chart_row *row = chart->row;

		row->row_code = rlib_infix_to_pcode(r, part, report, (gchar *)row->xml_row.xml, row->xml_row.line, TRUE);
		row->bar_start_code = rlib_infix_to_pcode(r, part, report, (gchar *)row->xml_bar_start.xml, row->xml_bar_start.line, TRUE);
		row->bar_end_code = rlib_infix_to_pcode(r, part, report, (gchar *)row->xml_bar_end.xml, row->xml_bar_end.line, TRUE);
		row->label_code = rlib_infix_to_pcode(r, part, report, (gchar *)row->xml_label.xml, row->xml_label.line, TRUE);
		row->bar_label_code = rlib_infix_to_pcode(r, part, report, (gchar *)row->xml_bar_label.xml, row->xml_bar_label.line, TRUE);

		row->bar_color_code = rlib_infix_to_pcode(r, part, report, (gchar *)row->xml_bar_color.xml, row->xml_bar_color.line, TRUE);
		row->bar_label_color_code = rlib_infix_to_pcode(r, part, report, (gchar *)row->xml_bar_label_color.xml, row->xml_bar_label_color.line, TRUE);
	}
}

void rlib_resolve_report_fields(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	struct rlib_element *e;
	gdouble f;

	if(report->variables != NULL) {
		for(e = report->variables; e != NULL; e=e->next) {
			struct rlib_report_variable *rv = e->data;
			rlib_variable_resolve_pcode(r, part, report, rv);

		}
	}

	init_variables(r, report);
	rlib_process_expression_variables(r, report);
	
	report->orientation = RLIB_ORIENTATION_PORTRAIT;
	report->orientation_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_orientation.xml, report->xml_orientation.line, TRUE);
	report->font_size = -1;
	report->font_size_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_font_size.xml, report->xml_font_size.line, TRUE);
	report->top_margin = RLIB_DEFAULT_TOP_MARGIN;
	report->top_margin_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_top_margin.xml, report->xml_top_margin.line, TRUE);
	report->left_margin = RLIB_DEFAULT_LEFT_MARGIN;
	report->left_margin_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_left_margin.xml, report->xml_left_margin.line, TRUE);
	report->bottom_margin = RLIB_DEFAULT_BOTTOM_MARGIN;
	report->bottom_margin_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_bottom_margin.xml, report->xml_bottom_margin.line, TRUE);
	report->pages_across = 1;
	report->pages_across_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_pages_across.xml, report->xml_pages_across.line, TRUE);
	report->suppress_page_header_first_page = FALSE;
	report->suppress_page_header_first_page_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_suppress_page_header_first_page.xml, report->xml_suppress_page_header_first_page.line, TRUE);
	report->suppress = FALSE;
	report->suppress_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_suppress.xml, report->xml_suppress.line, TRUE);
	report->height_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_height.xml, report->xml_height.line, TRUE);
	report->iterations = 1;
	report->iterations_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_iterations.xml, report->xml_iterations.line, TRUE);
	report->detail_columns_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_detail_columns.xml, report->xml_detail_columns.line, TRUE);
	report->column_pad_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_column_pad.xml, report->xml_column_pad.line, TRUE);

	report->uniquerow_code = rlib_infix_to_pcode(r, part, report, (gchar *)report->xml_uniquerow.xml, report->xml_uniquerow.line, TRUE);

	if (rlib_execute_as_double(r, report->pages_across_code, &f))
		report->pages_across = f;
	if (rlib_execute_as_double(r, report->iterations_code, &f))
		report->iterations = f;
	
	report->position_top = g_malloc(report->pages_across * sizeof(gdouble));
	report->position_bottom = g_malloc(report->pages_across * sizeof(gdouble));
	report->bottom_size = g_malloc(report->pages_across * sizeof(gdouble));

	rlib_resolve_outputs(r, part, report, report->report_header);
	rlib_resolve_outputs(r, part, report, report->page_header);
	rlib_resolve_outputs(r, part, report, report->page_footer);
	rlib_resolve_outputs(r, part, report, report->report_footer);
	if (report->detail) {
		rlib_resolve_outputs(r, part, report, report->detail->fields);
		rlib_resolve_outputs(r, part, report, report->detail->headers);
	}
	rlib_resolve_outputs(r, part, report, report->alternate);

	rlib_resolve_graph(r, part, report, report->graph);
	rlib_resolve_chart(r, part, report, report->chart);

	if(report->breaks != NULL) {
		for(e = report->breaks; e != NULL; e=e->next) {
			struct rlib_report_break *rb = e->data;
			struct rlib_element *be;
			
			rlib_resolve_outputs(r, part, report, rb->header);
			rlib_resolve_outputs(r, part, report, rb->footer);
			rb->newpage_code = rlib_infix_to_pcode(r, part, report, (gchar *)rb->xml_newpage.xml,rb->xml_newpage.line, TRUE);
			rb->headernewpage_code = rlib_infix_to_pcode(r, part, report, (gchar *)rb->xml_headernewpage.xml, rb->xml_headernewpage.line, TRUE);
			rb->suppressblank_code = rlib_infix_to_pcode(r, part, report, (gchar *)rb->xml_suppressblank.xml, rb->xml_suppressblank.line, TRUE);
			for(be = rb->fields; be != NULL; be=be->next) {
				struct rlib_break_fields *bf = be->data;
				rlib_break_resolve_pcode(r, part, report, bf);
			}		
		}
	}
	
}

void rlib_resolve_part_td(rlib *r, struct rlib_part *part, GSList *part_deviations) {
	GSList *element;
	
	for(element = part_deviations;element != NULL;element = g_slist_next(element)) {
		struct rlib_part_td *td = element->data;
		td->width_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)td->xml_width.xml, td->xml_width.line, TRUE);
		td->height_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)td->xml_height.xml, td->xml_height.line, TRUE);
		td->border_width_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)td->xml_border_width.xml, td->xml_border_width.line, TRUE);
		td->border_color_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)td->xml_border_color.xml, td->xml_border_color.line, TRUE);
	}
}

static void rlib_resolve_part_tr(rlib *r, struct rlib_part *part) {
	GSList *element;
	
	for(element = part->part_rows;element != NULL;element = g_slist_next(element)) {
		struct rlib_part_tr *tr = element->data;
		tr->layout_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)tr->xml_layout.xml, tr->xml_layout.line, TRUE);
		tr->newpage_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)tr->xml_newpage.xml, tr->xml_newpage.line, TRUE);
		rlib_resolve_part_td(r, part, tr->part_deviations);
	}	
}

void rlib_resolve_part_fields(rlib *r, struct rlib_part *part) {
	gdouble f;
	part->orientation = RLIB_ORIENTATION_PORTRAIT;
	part->orientation_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)part->xml_orientation.xml, part->xml_orientation.line, TRUE);
	part->font_size = -1;
	part->font_size_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)part->xml_font_size.xml, part->xml_font_size.line, TRUE);
	part->top_margin = RLIB_DEFAULT_TOP_MARGIN;
	part->top_margin_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)part->xml_top_margin.xml, part->xml_top_margin.line, TRUE);
	part->left_margin = RLIB_DEFAULT_LEFT_MARGIN;
	part->left_margin_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)part->xml_left_margin.xml, part->xml_left_margin.line, TRUE);
	part->bottom_margin = RLIB_DEFAULT_BOTTOM_MARGIN;
	part->bottom_margin_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)part->xml_bottom_margin.xml, part->xml_bottom_margin.line, TRUE);
	part->paper = layout_get_paper(RLIB_PAPER_LETTER);
	part->paper_type_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)part->xml_paper_type.xml, part->xml_paper_type.line, TRUE);
	part->pages_across = 1;
	part->pages_across_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)part->xml_pages_across.xml, part->xml_pages_across.line, TRUE);
	part->iterations = 1;
	part->iterations_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)part->xml_iterations.xml, part->xml_iterations.line, TRUE);
	part->suppress_page_header_first_page = FALSE;
	part->suppress_page_header_first_page_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)part->xml_suppress_page_header_first_page.xml, part->xml_suppress_page_header_first_page.line, TRUE);
	part->suppress = FALSE;
	part->suppress_code = rlib_infix_to_pcode(r, part, NULL, (gchar *)part->xml_suppress.xml, part->xml_suppress.line, TRUE);

	if (rlib_execute_as_double(r, part->pages_across_code, &f))
		part->pages_across = f;
	if (rlib_execute_as_double(r, part->iterations_code, &f))
		part->iterations = f;

	part->position_top = g_malloc(part->pages_across * sizeof(gdouble));
	part->position_bottom = g_malloc(part->pages_across * sizeof(gdouble));
	part->bottom_size = g_malloc(part->pages_across * sizeof(gdouble));

	rlib_resolve_part_tr(r, part);
	rlib_resolve_outputs(r, part, NULL, part->page_header);
	rlib_resolve_outputs(r, part, NULL, part->page_footer);
	rlib_resolve_outputs(r, part, NULL, part->report_header);
}

gchar *rlib_resolve_memory_variable(rlib *r, gchar *name) {
	if(r_strlen(name) >= 3 && name[0] == 'm' && name[1] == '.') {
		gchar *value;
		value = g_hash_table_lookup(r->parameters, name+2);
		if (value != NULL)
			return g_strdup(value);
		return ENVIRONMENT(r)->rlib_resolve_memory_variable(name+2);
	}
	return NULL;
}

void resolve_metadata(gpointer name UNUSED, gpointer value, gpointer user_data) {
	struct rlib_metadata *metadata = value;
	metadata->formula_code = rlib_infix_to_pcode(user_data, NULL, NULL, (gchar *)metadata->xml_formula.xml, metadata->xml_formula.line, FALSE);
	rlib_value_free(metadata->r, &metadata->rval_formula);
}

void rlib_resolve_metadata(rlib *r) {
	g_hash_table_foreach(r->input_metadata, resolve_metadata, r);
}

void process_metadata(gpointer name UNUSED, gpointer value, gpointer user_data) {
	struct rlib_metadata *metadata = value;
	rlib_value_free(metadata->r, &metadata->rval_formula);
	rlib_execute_pcode(user_data, &metadata->rval_formula, metadata->formula_code, NULL);
}

void rlib_process_input_metadata(rlib *r) {
	g_hash_table_foreach(r->input_metadata, process_metadata, r);
}

void rlib_resolve_followers(rlib *r) {
	gint i;
	gboolean reattached;

	/*
	 * Separate the followers list that was easier
	 * to check in rlib_add_resultset_follower_n_to_1() and
	 * rlib_add_resultset_follower() as a tree into two lists:
	 * the 1:1 followers and n:1 followers_n_to_1 lists.
	 */
	for (i = 0; i < r->queries_count; i++) {
		GList *list0, *list;
		GList *new_f = NULL, *new_f_n_to_1 = NULL;

		list0 = r->queries[i]->followers;
		for (list = list0; list; list = list->next) {
			struct rlib_resultset_followers *f = list->data;

			if (f->leader_field && f->follower_field) {
				f->leader_code = rlib_infix_to_pcode(r, NULL, NULL, f->leader_field, -1, FALSE);
				f->follower_code = rlib_infix_to_pcode(r, NULL, NULL, f->follower_field, -1, FALSE);
				new_f_n_to_1 = g_list_append(new_f_n_to_1, f);
			} else
				new_f = g_list_append(new_f, f);
		}

		r->queries[i]->followers = new_f;
		r->queries[i]->followers_n_to_1 = new_f_n_to_1;

		g_list_free(list0);
	}

	/*
	 * Flatten followers as much as possible,
	 * A leader -> 1:1 -> (anything) chain is equivalent to
	 * keeping the leader -> 1:1 chain and move leafs under
	 * the 1:1 child follower to the leader.
	 */
	reattached = TRUE;
	while (reattached) {
		reattached = FALSE;
		for (i = r->queries_count; i; i--) {
			struct rlib_query_internal *leader = r->queries[i - 1]->leader;
			if (leader) {
				GList *list, *list1;
				for (list = leader->followers; list; list = list->next) {
					struct rlib_resultset_followers *f = list->data;
					struct rlib_query_internal *follower = r->queries[f->follower];

					for (list1 = follower->followers; list1; list1 = list1->next) {
						struct rlib_resultset_followers *f1 = list1->data;
						struct rlib_query_internal *follower1 = r->queries[f1->follower];

						f1->leader = leader->query_index;
						follower1->leader = leader;
						reattached = TRUE;
					}
					leader->followers = g_list_concat(leader->followers, follower->followers);
					follower->followers = NULL;

					for (list1 = follower->followers_n_to_1; list1; list1 = list1->next) {
						struct rlib_resultset_followers *f1 = list1->data;
						struct rlib_query_internal *follower1 = r->queries[f1->follower];

						f1->leader = leader->query_index;
						follower1->leader = leader;
						reattached = TRUE;
					}
					leader->followers_n_to_1 = g_list_concat(leader->followers_n_to_1, follower->followers_n_to_1);
					follower->followers_n_to_1 = NULL;
				}
			}
		}
	}
}

void rlib_resolve_breaks(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report) {
	struct rlib_element *e, *f;

	if (report == NULL && part != NULL)
		report = part->only_report;
	if (report == NULL)
		return;

	for (e = report->breaks; e; e = e->next) {
		struct rlib_report_break *rb = e->data;
		for (f = report->variables; f; f = f->next) {
			struct rlib_report_variable *rv = f->data;

			if (strcmp((char *)rb->xml_name.xml, (char *)rv->xml_resetonbreak.xml) == 0) {
				rv->resetonbreak = rb;
				rb->variables = g_slist_append(rb->variables, rv);
			}
		}
	}
}
