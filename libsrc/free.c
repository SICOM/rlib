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
 */

#include <stdlib.h>
#include <gmodule.h>

#include <config.h>
#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"

static void image_free_pcode(rlib *r, struct rlib_report_image * ri) {
	rlib_pcode_free(r, ri->value_code);
	rlib_pcode_free(r, ri->type_code);
	rlib_pcode_free(r, ri->width_code);
	rlib_pcode_free(r, ri->height_code);
	rlib_pcode_free(r, ri->textwidth_code);
	xmlFree(ri->xml_value.xml);
	xmlFree(ri->xml_type.xml);
	xmlFree(ri->xml_width.xml);
	xmlFree(ri->xml_height.xml);
	xmlFree(ri->xml_textwidth.xml);
	g_free(ri);
}

static void barcode_free_pcode(rlib *r, struct rlib_report_barcode * rb) {
	rlib_pcode_free(r, rb->value_code);
	rlib_pcode_free(r, rb->type_code);
	rlib_pcode_free(r, rb->width_code);
	rlib_pcode_free(r, rb->height_code);
	xmlFree(rb->xml_value.xml);
	xmlFree(rb->xml_type.xml);
	xmlFree(rb->xml_width.xml);
	xmlFree(rb->xml_height.xml);
	g_free(rb);
}



static void hr_free_pcode(rlib *r, struct rlib_report_horizontal_line * rhl) {
	rlib_pcode_free(r, rhl->bgcolor_code);
	rlib_pcode_free(r, rhl->suppress_code);
	rlib_pcode_free(r, rhl->indent_code);
	rlib_pcode_free(r, rhl->length_code);
	rlib_pcode_free(r, rhl->font_size_code);
	rlib_pcode_free(r, rhl->size_code);
	xmlFree(rhl->xml_bgcolor.xml);
	xmlFree(rhl->xml_size.xml);
	xmlFree(rhl->xml_indent.xml);
	xmlFree(rhl->xml_length.xml);
	xmlFree(rhl->xml_font_size.xml);
	xmlFree(rhl->xml_suppress.xml);
	g_free(rhl);
}

static void text_free_pcode(rlib *r, struct rlib_report_literal *rt) {
	g_free(rt->value);
	rlib_pcode_free(r, rt->color_code);
	rlib_pcode_free(r, rt->bgcolor_code);
	rlib_pcode_free(r, rt->col_code);
	rlib_pcode_free(r, rt->width_code);
	rlib_pcode_free(r, rt->bold_code);
	rlib_pcode_free(r, rt->italics_code);
	rlib_pcode_free(r, rt->align_code);
	rlib_pcode_free(r, rt->link_code);
	rlib_pcode_free(r, rt->translate_code);
	xmlFree(rt->xml_align.xml);
	xmlFree(rt->xml_bgcolor.xml);
	xmlFree(rt->xml_color.xml);
	xmlFree(rt->xml_width.xml);
	xmlFree(rt->xml_bold.xml);
	xmlFree(rt->xml_italics.xml);
	xmlFree(rt->xml_link.xml);
	xmlFree(rt->xml_translate.xml);
	xmlFree(rt->xml_col.xml);
	g_free(rt);
}

static void field_free_pcode(rlib *r, struct rlib_report_field *rf) {
	g_free(rf->value);
	rlib_pcode_free(r, rf->code);
	rlib_pcode_free(r, rf->format_code);
	rlib_pcode_free(r, rf->link_code);
	rlib_pcode_free(r, rf->translate_code);
	rlib_pcode_free(r, rf->color_code);
	rlib_pcode_free(r, rf->bgcolor_code);
	rlib_pcode_free(r, rf->col_code);
	rlib_pcode_free(r, rf->delayed_code);
	rlib_pcode_free(r, rf->width_code);
	rlib_pcode_free(r, rf->bold_code);
	rlib_pcode_free(r, rf->italics_code);
	rlib_pcode_free(r, rf->align_code);
	rlib_pcode_free(r, rf->memo_code);
	rlib_pcode_free(r, rf->memo_max_lines_code);
	rlib_pcode_free(r, rf->memo_wrap_chars_code);
	xmlFree(rf->xml_align.xml);
	xmlFree(rf->xml_bgcolor.xml);
	xmlFree(rf->xml_color.xml);
	xmlFree(rf->xml_width.xml);
	xmlFree(rf->xml_bold.xml);
	xmlFree(rf->xml_italics.xml);
	xmlFree(rf->xml_format.xml);
	xmlFree(rf->xml_link.xml);
	xmlFree(rf->xml_translate.xml);
	xmlFree(rf->xml_col.xml);
	xmlFree(rf->xml_delayed.xml);
	xmlFree(rf->xml_memo.xml);
	xmlFree(rf->xml_memo_max_lines.xml);
	xmlFree(rf->xml_memo_wrap_chars.xml);

	g_free(rf);
}

void rlib_free_line_elements(rlib *r, struct rlib_element *e) {
	struct rlib_element *next = NULL;

	for (; e != NULL; next = e->next, g_free(e), e = next) {
		switch (e->type) {
		case RLIB_ELEMENT_FIELD:
			field_free_pcode(r, (struct rlib_report_field *)e->data);
			break;
		case RLIB_ELEMENT_LITERAL:
			text_free_pcode(r, (struct rlib_report_literal *)e->data);
			break;
		case RLIB_ELEMENT_IMAGE:
			image_free_pcode(r, (struct rlib_report_image *)e->data);
			break;
		case RLIB_ELEMENT_BARCODE:
			barcode_free_pcode(r, (struct rlib_report_barcode *)e->data);
			break;
		default:
			break;
		}
	}
}

void rlib_free_lines(rlib *r, struct rlib_report_lines *rl) {
	if (rl == NULL)
		return;

	rlib_pcode_free(r, rl->bgcolor_code);
	rlib_pcode_free(r, rl->color_code);
	rlib_pcode_free(r, rl->suppress_code);
	rlib_pcode_free(r, rl->font_size_code);
	rlib_pcode_free(r, rl->bold_code);
	rlib_pcode_free(r, rl->italics_code);

	rlib_free_line_elements(r, rl->e);
	rl->e = NULL;

	xmlFree(rl->xml_bgcolor.xml);
	xmlFree(rl->xml_color.xml);
	xmlFree(rl->xml_bold.xml);
	xmlFree(rl->xml_italics.xml);
	xmlFree(rl->xml_font_size.xml);
	xmlFree(rl->xml_suppress.xml);
	g_free(rl);
}

static void free_fields(rlib *r, struct rlib_report_output_array *roa) {
	GSList *ptr;

	if (roa == NULL)
		return;

	for (ptr = roa->chain; ptr; ptr = g_slist_next(ptr)) {
		struct rlib_report_output *ro = ptr->data;
		if (ro->type == RLIB_REPORT_PRESENTATION_DATA_LINE) {
			rlib_free_lines(r, (struct rlib_report_lines *)ro->data);
		} else if(ro->type == RLIB_REPORT_PRESENTATION_DATA_HR) {
			hr_free_pcode(r, (struct rlib_report_horizontal_line *)ro->data);
		} else if(ro->type == RLIB_REPORT_PRESENTATION_DATA_IMAGE) {
			image_free_pcode(r, (struct rlib_report_image *)ro->data);
		}
		g_free(ro);
	}
	g_slist_free(roa->chain);
	xmlFree(roa->xml_page.xml);
	xmlFree(roa->xml_suppress.xml);
	g_free(roa);
}

static void break_free_pcode(rlib *r, struct rlib_break_fields *bf) {
	rlib_pcode_free(r, bf->code);
	xmlFree(bf->xml_value.xml);
}

void rlib_free_output(rlib *r, struct rlib_element *e) {
	struct rlib_report_output_array *roa;
	struct rlib_element *next;

	for (; e != NULL; next = e->next, g_free(e), e = next) {
		roa = e->data;
		free_fields(r, roa);
	}
}

void rlib_free_graph(rlib *r, struct rlib_graph *graph) {
	struct rlib_graph_plot *plot;
	GSList *list;

	if (graph == NULL)
		return;

	rlib_pcode_free(r, graph->name_code);
	rlib_pcode_free(r, graph->type_code);
	rlib_pcode_free(r, graph->subtype_code);
	rlib_pcode_free(r, graph->width_code);
	rlib_pcode_free(r, graph->height_code);
	rlib_pcode_free(r, graph->bold_titles_code);
	rlib_pcode_free(r, graph->title_code);
	rlib_pcode_free(r, graph->legend_bg_color_code);
	rlib_pcode_free(r, graph->legend_orientation_code);
	rlib_pcode_free(r, graph->draw_x_line_code);
	rlib_pcode_free(r, graph->draw_y_line_code);
	rlib_pcode_free(r, graph->grid_color_code);
	rlib_pcode_free(r, graph->x_axis_title_code);
	rlib_pcode_free(r, graph->y_axis_title_code);
	rlib_pcode_free(r, graph->y_axis_mod_code);
	rlib_pcode_free(r, graph->y_axis_title_right_code);
	rlib_pcode_free(r, graph->y_axis_decimals_code);
	rlib_pcode_free(r, graph->y_axis_decimals_code_right);

	xmlFree(graph->xml_name.xml);
	xmlFree(graph->xml_type.xml);
	xmlFree(graph->xml_subtype.xml);
	xmlFree(graph->xml_width.xml);
	xmlFree(graph->xml_height.xml);
	xmlFree(graph->xml_bold_titles.xml);
	xmlFree(graph->xml_title.xml);
	xmlFree(graph->xml_legend_bg_color.xml);
	xmlFree(graph->xml_legend_orientation.xml);
	xmlFree(graph->xml_draw_x_line.xml);
	xmlFree(graph->xml_draw_y_line.xml);
	xmlFree(graph->xml_grid_color.xml);
	xmlFree(graph->xml_x_axis_title.xml);
	xmlFree(graph->xml_y_axis_title.xml);
	xmlFree(graph->xml_y_axis_mod.xml);
	xmlFree(graph->xml_y_axis_title_right.xml);
	xmlFree(graph->xml_y_axis_decimals.xml);
	xmlFree(graph->xml_y_axis_decimals_right.xml);

	for (list = graph->plots; list != NULL; list = g_slist_next(list)) {
		plot = list->data;
		rlib_pcode_free(r, plot->axis_code);
		rlib_pcode_free(r, plot->field_code);
		rlib_pcode_free(r, plot->label_code);
		rlib_pcode_free(r, plot->side_code);
		rlib_pcode_free(r, plot->disabled_code);
		rlib_pcode_free(r, plot->color_code);
		xmlFree(plot->xml_axis.xml);
		xmlFree(plot->xml_field.xml);
		xmlFree(plot->xml_label.xml);
		xmlFree(plot->xml_side.xml);
		xmlFree(plot->xml_disabled.xml);
		xmlFree(plot->xml_color.xml);
	}

	g_free(graph);
}

void rlib_free_chart_row(rlib *r, struct rlib_chart_row *row) {
	if (row == NULL)
		return;

	rlib_pcode_free(r, row->row_code);
	rlib_pcode_free(r, row->bar_start_code);
	rlib_pcode_free(r, row->bar_end_code);
	rlib_pcode_free(r, row->label_code);
	rlib_pcode_free(r, row->bar_label_code);
	rlib_pcode_free(r, row->bar_color_code);
	rlib_pcode_free(r, row->bar_label_color_code);

	xmlFree(row->xml_row.xml);
	xmlFree(row->xml_bar_start.xml);
	xmlFree(row->xml_bar_end.xml);
	xmlFree(row->xml_label.xml);
	xmlFree(row->xml_bar_label.xml);
	xmlFree(row->xml_bar_color.xml);
	xmlFree(row->xml_bar_label_color.xml);

	g_free(row);
}

void rlib_free_chart_header_row(rlib *r, struct rlib_chart_header_row *header_row) {
	if (header_row == NULL)
		return;

	rlib_pcode_free(r, header_row->query_code);
	rlib_pcode_free(r, header_row->field_code);
	rlib_pcode_free(r, header_row->colspan_code);

	xmlFree(header_row->xml_query.xml);
	xmlFree(header_row->xml_field.xml);
	xmlFree(header_row->xml_colspan.xml);

	g_free(header_row);
}

void rlib_free_chart(rlib *r, struct rlib_chart *chart) {
	if (chart == NULL)
		return;

	rlib_pcode_free(r, chart->name_code);
	rlib_pcode_free(r, chart->title_code);
	rlib_pcode_free(r, chart->cols_code);
	rlib_pcode_free(r, chart->rows_code);
	rlib_pcode_free(r, chart->cell_width_code);
	rlib_pcode_free(r, chart->cell_height_code);
	rlib_pcode_free(r, chart->cell_width_padding_code);
	rlib_pcode_free(r, chart->cell_height_padding_code);
	rlib_pcode_free(r, chart->label_width_code);
	rlib_pcode_free(r, chart->header_row_code);

	xmlFree(chart->xml_name.xml);
	xmlFree(chart->xml_title.xml);
	xmlFree(chart->xml_cols.xml);
	xmlFree(chart->xml_rows.xml);
	xmlFree(chart->xml_cell_width.xml);
	xmlFree(chart->xml_cell_height.xml);
	xmlFree(chart->xml_cell_width_padding.xml);
	xmlFree(chart->xml_cell_height_padding.xml);
	xmlFree(chart->xml_label_width.xml);
	xmlFree(chart->xml_header_row.xml);

	rlib_free_chart_header_row(r, chart->header_row);
	chart->header_row = NULL;

	rlib_free_chart_row(r, chart->row);
	chart->row = NULL;

	g_free(chart);
}

void rlib_free_break_fields(rlib *r, struct rlib_element *be) {
	struct rlib_element *next;

	for (; be; next = be->next, g_free(be), be = next) {
		struct rlib_break_fields *bf = be->data;

		break_free_pcode(r, bf);
		g_free(bf);
	}
}

void rlib_free_breaks(rlib *r, struct rlib_element *e) {
	struct rlib_element *next;

	for (; e != NULL; next = e->next, g_free(e), e = next) {
		struct rlib_report_break *rb;

		rb = e->data;

		rlib_free_output(r, rb->header);
		rb->header = NULL;
		rlib_free_output(r, rb->footer);
		rb->footer = NULL;

		rlib_free_break_fields(r, rb->fields);
		rb->fields = NULL;

		rlib_pcode_free(r, rb->newpage_code);
		rlib_pcode_free(r, rb->headernewpage_code);
		rlib_pcode_free(r, rb->suppressblank_code);
		xmlFree(rb->xml_name.xml);
		xmlFree(rb->xml_newpage.xml);
		xmlFree(rb->xml_headernewpage.xml);
		xmlFree(rb->xml_suppressblank.xml);
		g_free(rb);
	}
}

void rlib_free_variables(rlib *r, struct rlib_element *e) {
	struct rlib_element *save;

	for (; e != NULL; save = e, e = e->next, g_free(save)) {
		struct rlib_report_variable *rv = e->data;

		rlib_pcode_free(r, rv->code);
		rlib_pcode_free(r, rv->ignore_code);

		xmlFree(rv->xml_name.xml);
		xmlFree(rv->xml_str_type.xml);
		xmlFree(rv->xml_value.xml);
		xmlFree(rv->xml_resetonbreak.xml);
		xmlFree(rv->xml_precalculate.xml);
		xmlFree(rv->xml_ignore.xml);

		if(rv->precalculated_values != NULL) {
			g_free(rv->precalculated_values->data);
			rv->precalculated_values = g_slist_remove_link (rv->precalculated_values, rv->precalculated_values);
		}

		g_free(rv);
		e->data = NULL;
	}
}

void rlib_free_detail(rlib *r, struct rlib_report_detail *d) {
	if (d == NULL)
		return;

	rlib_free_output(r, d->fields);
	rlib_free_output(r, d->headers);
	g_free(d);
}

void rlib_free_report(rlib *r, struct rlib_report *report) {
	if (report == NULL)
		return;

	rlib_pcode_free(r, report->font_size_code);
	rlib_pcode_free(r, report->query_code);
	rlib_pcode_free(r, report->orientation_code);
	rlib_pcode_free(r, report->top_margin_code);
	rlib_pcode_free(r, report->left_margin_code);
	rlib_pcode_free(r, report->bottom_margin_code);
	rlib_pcode_free(r, report->pages_across_code);
	rlib_pcode_free(r, report->suppress_page_header_first_page_code);
	rlib_pcode_free(r, report->suppress_code);
	rlib_pcode_free(r, report->detail_columns_code);
	rlib_pcode_free(r, report->column_pad_code);
	rlib_pcode_free(r, report->uniquerow_code);

	rlib_free_output(r, report->report_header);
	report->report_header = NULL;
	rlib_free_output(r, report->page_header);
	report->page_header = NULL;
	rlib_free_output(r, report->page_footer);
	report->page_footer = NULL;
	rlib_free_output(r, report->report_footer);
	report->report_footer = NULL;
	if (report->detail) {
		rlib_free_detail(r, report->detail);
		report->detail = NULL;
	}
	rlib_free_output(r, report->alternate);
	report->alternate = NULL;
	rlib_free_graph(r, report->graph);
	report->graph = NULL;
	rlib_free_chart(r, report->chart);
	report->chart = NULL;

	g_free(report->position_top);
	g_free(report->position_bottom);
	g_free(report->bottom_size);

	rlib_value_free(&report->uniquerow);

	rlib_free_breaks(r, report->breaks);
	report->breaks = NULL;

	rlib_free_variables(r, report->variables);
	report->variables = NULL;

	xmlFree(report->xml_font_size.xml);
	xmlFree(report->xml_query.xml);
	xmlFree(report->xml_orientation.xml);
	xmlFree(report->xml_top_margin.xml);
	xmlFree(report->xml_left_margin.xml);
	xmlFree(report->xml_detail_columns.xml);
	xmlFree(report->xml_column_pad.xml);
	xmlFree(report->xml_bottom_margin.xml);
	xmlFree(report->xml_height.xml);
	xmlFree(report->xml_iterations.xml);
	xmlFree(report->xml_pages_across.xml);
	xmlFree(report->xml_suppress_page_header_first_page.xml);
	xmlFree(report->xml_suppress.xml);
	xmlFree(report->xml_uniquerow.xml);

	g_free(report);
}

void rlib_free_part_td(rlib *r, struct rlib_part_td *td) {
	GSList *reports;

	if (td == NULL)
		return;

	rlib_pcode_free(r, td->width_code);
	rlib_pcode_free(r, td->height_code);
	rlib_pcode_free(r, td->border_width_code);
	rlib_pcode_free(r, td->border_color_code);
	xmlFree(td->xml_width.xml);
	xmlFree(td->xml_height.xml);
	xmlFree(td->xml_border_width.xml);
	xmlFree(td->xml_border_color.xml);

	for (reports = td->reports; reports != NULL; reports = g_slist_next(reports)) {
		rlib_free_report(r, (struct rlib_report *)reports->data);
	}

	g_slist_free(td->reports);
	td->reports = NULL;
	g_free(td);
}

void rlib_free_part_deviations(rlib *r, GSList *part_deviations) {
	GSList *element;
	for (element = part_deviations; element != NULL; element = g_slist_next(element)) {
		struct rlib_part_td *td = element->data;
		rlib_free_part_td(r, td);
	}
	g_slist_free(part_deviations);
}

void rlib_free_part_tr(rlib *r, struct rlib_part_tr *tr) {
	rlib_pcode_free(r, tr->layout_code);
	rlib_pcode_free(r, tr->newpage_code);
	xmlFree(tr->xml_layout.xml);
	xmlFree(tr->xml_newpage.xml);
	rlib_free_part_deviations(r, tr->part_deviations);
	tr->part_deviations = NULL;
	g_free(tr);
}

void rlib_free_part(rlib *r, struct rlib_part *part) {
	GSList *element;

	if (part == NULL)
		return;

	rlib_pcode_free(r, part->orientation_code);
	rlib_pcode_free(r, part->font_size_code);
	rlib_pcode_free(r, part->top_margin_code);
	rlib_pcode_free(r, part->left_margin_code);
	rlib_pcode_free(r, part->bottom_margin_code);
	rlib_pcode_free(r, part->paper_type_code);
	rlib_pcode_free(r, part->pages_across_code);
	rlib_pcode_free(r, part->suppress_page_header_first_page_code);
	rlib_pcode_free(r, part->suppress_code);

	xmlFree(part->xml_name.xml);
	xmlFree(part->xml_pages_across.xml);
	xmlFree(part->xml_font_size.xml);
	xmlFree(part->xml_orientation.xml);
	xmlFree(part->xml_top_margin.xml);
	xmlFree(part->xml_left_margin.xml);
	xmlFree(part->xml_bottom_margin.xml);
	xmlFree(part->xml_paper_type.xml);
	xmlFree(part->xml_iterations.xml);
	xmlFree(part->xml_suppress_page_header_first_page.xml);
	xmlFree(part->xml_suppress.xml);

	g_free(part->position_top);
	g_free(part->position_bottom);
	g_free(part->bottom_size);

	rlib_free_output(r, part->page_header);
	part->page_header = NULL;
	rlib_free_output(r, part->page_footer);
	part->page_footer = NULL;
	rlib_free_output(r, part->report_header);
	part->report_header = NULL;

	for (element = part->part_rows; element != NULL; element = g_slist_next(element)) {
		struct rlib_part_tr *tr = element->data;
		rlib_free_part_tr(r, tr);
	}
	g_slist_free(part->part_rows);
	part->part_rows = NULL;

	g_free(part);
}

void rlib_free_tree(rlib *r) {
	int i;
	for (i = 0; i < r->parts_count; i++) {
		struct rlib_part *part = r->parts[i];
		rlib_free_part(r, part);
		g_free(r->reportstorun[i].name);
		g_free(r->reportstorun[i].dir);
		r->parts[i] = NULL;
	}

	g_slist_free_full(r->search_paths, g_free);
}

/*
 * Only free the rlib_results.result area
 * This is called when re-running queries.
 */
void rlib_free_results(rlib *r) {
	int i;

	for (i = 0; i < r->queries_count; i++) {
		if (r->results[i]->result && INPUT(r, i)->free_result)
			INPUT(r, i)->free_result(INPUT(r, i), r->results[i]->result);
		r->results[i]->result = NULL;
	}
}

/*
 * Free all the data allocated for queries and results.
 * This is called by rlib_free() when destroying everything.
 */
static void rlib_free_results_and_queries(rlib *r) {
	int i;

	for (i = 0; i < r->queries_count; i++) {
		if (r->results[i]->result && INPUT(r, i)->free_result) {
			INPUT(r, i)->free_result(INPUT(r, i), r->results[i]->result);
			r->results[i]->result = NULL;
		}
		g_free(r->results[i]);
		r->results[i] = NULL;
		if (QUERY(r, i) && QUERY(r, i)->input && QUERY(r, i)->input->free_query)
			QUERY(r, i)->input->free_query(QUERY(r, i)->input, QUERY(r, i));
		if (r->queries[i]->sql_allocated)
			g_free(r->queries[i]->sql);
		g_free(r->queries[i]->name);
		g_free(r->queries[i]);
		r->queries[i] = NULL;
	}
	g_free(r->results);
	r->results = NULL;
	g_free(r->queries);
	r->queries = NULL;
	r->queries_count = 0;
}


gint rlib_free_follower(rlib *r) {
	gint i;
	for (i = 0; i < r->resultset_followers_count; i++) {
		g_free(r->followers[i].leader_field);
		g_free(r->followers[i].follower_field);
		rlib_pcode_free(r, r->followers[i].leader_code);
		rlib_pcode_free(r, r->followers[i].follower_code);
	}

	return TRUE;
}

DLL_EXPORT_SYM gint rlib_free(rlib *r) {
	int i;

	rlib_charencoder_free(r->output_encoder);
	g_free(r->output_encoder_name);

	rlib_free_results_and_queries(r);

	rlib_free_tree(r);
	xmlCleanupParser();
	for (i = 0; i < r->inputs_count; i++) {
		rlib_charencoder_free(r->inputs[i].input->info.encoder);
		r->inputs[i].input->input_close(r->inputs[i].input);
		r->inputs[i].input->free(r->inputs[i].input);
		if (r->inputs[i].handle != NULL)
			g_module_close(r->inputs[i].handle);
		g_free(r->inputs[i].name);
	}

	if (r->did_execute && OUTPUT(r)) {
		OUTPUT(r)->free(r);
	}

	ENVIRONMENT(r)->free(r);

	g_hash_table_destroy(r->output_parameters);
	g_hash_table_destroy(r->input_metadata);
	g_hash_table_destroy(r->parameters);
	rlib_free_follower(r);
	g_free(r->special_locale);
	g_free(r->current_locale);

	g_free(r);
	return 0;
}
