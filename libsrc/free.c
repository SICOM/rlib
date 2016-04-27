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

#include "config.h"
#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"

static void rlib_image_free_pcode(rlib *r, struct rlib_report_image * ri) {
	rlib_pcode_free(ri->value_code);
	rlib_pcode_free(ri->type_code);
	rlib_pcode_free(ri->width_code);
	rlib_pcode_free(ri->height_code);
	rlib_pcode_free(ri->textwidth_code);
	xmlFree(ri->xml_value.xml);
	xmlFree(ri->xml_type.xml);
	xmlFree(ri->xml_width.xml);
	xmlFree(ri->xml_height.xml);
	xmlFree(ri->xml_textwidth.xml);
	g_free(ri);
}

static void rlib_barcode_free_pcode(rlib *r, struct rlib_report_barcode * rb) {
	rlib_pcode_free(rb->value_code);
	rlib_pcode_free(rb->type_code);
	rlib_pcode_free(rb->width_code);
	rlib_pcode_free(rb->height_code);
	xmlFree(rb->xml_value.xml);
	xmlFree(rb->xml_type.xml);
	xmlFree(rb->xml_width.xml);
	xmlFree(rb->xml_height.xml);
	g_free(rb);
}



static void rlib_hr_free_pcode(rlib *r, struct rlib_report_horizontal_line * rhl) {
	rlib_pcode_free(rhl->bgcolor_code);
	rlib_pcode_free(rhl->suppress_code);
	rlib_pcode_free(rhl->indent_code);
	rlib_pcode_free(rhl->length_code);
	rlib_pcode_free(rhl->font_size_code);
	rlib_pcode_free(rhl->size_code);
	xmlFree(rhl->xml_bgcolor.xml);
	xmlFree(rhl->xml_size.xml);
	xmlFree(rhl->xml_indent.xml);
	xmlFree(rhl->xml_length.xml);
	xmlFree(rhl->xml_font_size.xml);
	xmlFree(rhl->xml_suppress.xml);
	g_free(rhl);
}

static void rlib_text_free_pcode(rlib *r, struct rlib_report_literal *rt) {
	g_free(rt->value);
	rlib_pcode_free(rt->color_code);
	rlib_pcode_free(rt->bgcolor_code);
	rlib_pcode_free(rt->col_code);
	rlib_pcode_free(rt->width_code);
	rlib_pcode_free(rt->bold_code);
	rlib_pcode_free(rt->italics_code);
	rlib_pcode_free(rt->align_code);
	rlib_pcode_free(rt->link_code);
	rlib_pcode_free(rt->translate_code);
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

static void rlib_field_free_pcode(rlib *r, struct rlib_report_field *rf) {
	g_free(rf->value);
	rlib_pcode_free(rf->code);
	rlib_pcode_free(rf->format_code);
	rlib_pcode_free(rf->link_code);
	rlib_pcode_free(rf->translate_code);
	rlib_pcode_free(rf->color_code);
	rlib_pcode_free(rf->bgcolor_code);
	rlib_pcode_free(rf->col_code);
	rlib_pcode_free(rf->delayed_code);
	rlib_pcode_free(rf->width_code);
	rlib_pcode_free(rf->bold_code);
	rlib_pcode_free(rf->italics_code);
	rlib_pcode_free(rf->align_code);
	rlib_pcode_free(rf->memo_code);
	rlib_pcode_free(rf->memo_max_lines_code);
	rlib_pcode_free(rf->memo_wrap_chars_code);
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

static void rlib_free_fields(rlib *r, struct rlib_report_output_array *roa) {
	struct rlib_element *e, *save;
	gint j;

	if(roa == NULL)
		return;
	for(j=0;j<roa->count;j++) {
		struct rlib_report_output *ro = roa->data[j];
		if(ro->type == RLIB_REPORT_PRESENTATION_DATA_LINE) {
			struct rlib_report_lines *rl = ro->data;
			e = rl->e;
			rlib_pcode_free(rl->bgcolor_code);
			rlib_pcode_free(rl->color_code);
			rlib_pcode_free(rl->suppress_code);
			rlib_pcode_free(rl->font_size_code);
			rlib_pcode_free(rl->bold_code);
			rlib_pcode_free(rl->italics_code);
			for(; e != NULL; e=e->next) {
				if(e->type == RLIB_ELEMENT_FIELD) {
					rlib_field_free_pcode(r, ((struct rlib_report_field *)e->data));
				} else if(e->type == RLIB_ELEMENT_LITERAL) {
					rlib_text_free_pcode(r, ((struct rlib_report_literal *)e->data));
				} else if(e->type == RLIB_ELEMENT_IMAGE) {
					rlib_image_free_pcode(r, ((struct rlib_report_image *)e->data));
				} else if(e->type == RLIB_ELEMENT_BARCODE) {
					rlib_barcode_free_pcode(r, ((struct rlib_report_barcode *)e->data));
				}
			}
			for(e=rl->e; e != NULL; ) {
				save = e;
				e=e->next;
				g_free(save);
			}
			xmlFree(rl->xml_bgcolor.xml);
			xmlFree(rl->xml_color.xml);
			xmlFree(rl->xml_bold.xml);
			xmlFree(rl->xml_italics.xml);
			xmlFree(rl->xml_font_size.xml);
			xmlFree(rl->xml_suppress.xml);
			g_free(rl);
		} else if(ro->type == RLIB_REPORT_PRESENTATION_DATA_HR) {
			rlib_hr_free_pcode(r, ((struct rlib_report_horizontal_line *)ro->data));
		} else if(ro->type == RLIB_REPORT_PRESENTATION_DATA_IMAGE) {
			rlib_image_free_pcode(r, ((struct rlib_report_image *)ro->data));
		}
		g_free(ro);
	}
	g_free(roa->data);
	xmlFree(roa->xml_page.xml);
	xmlFree(roa->xml_suppress.xml);
	g_free(roa);
}


static void rlib_break_free_pcode(rlib *r, struct rlib_break_fields *bf) {
	rlib_pcode_free(bf->code);
	xmlFree(bf->xml_value.xml);
}

static void rlib_free_output(rlib *r, struct rlib_element *e) {
	struct rlib_report_output_array *roa;
	struct rlib_element *save;
	while(e != NULL) {
		save = e;
		roa = e->data;
		rlib_free_fields(r, roa);
		e=e->next;
		g_free(save);
	}
}

static void rlib_free_graph(rlib *r, struct rlib_graph *graph) {
	struct rlib_graph_plot *plot;
	GSList *list;

	rlib_pcode_free(graph->name_code);
	rlib_pcode_free(graph->type_code);
	rlib_pcode_free(graph->subtype_code);
	rlib_pcode_free(graph->width_code);
	rlib_pcode_free(graph->height_code);
	rlib_pcode_free(graph->bold_titles_code);
	rlib_pcode_free(graph->title_code);
	rlib_pcode_free(graph->legend_bg_color_code);
	rlib_pcode_free(graph->legend_orientation_code);
	rlib_pcode_free(graph->draw_x_line_code);
	rlib_pcode_free(graph->draw_y_line_code);
	rlib_pcode_free(graph->grid_color_code);
	rlib_pcode_free(graph->x_axis_title_code);
	rlib_pcode_free(graph->y_axis_title_code);
	rlib_pcode_free(graph->y_axis_mod_code);
	rlib_pcode_free(graph->y_axis_title_right_code);
	rlib_pcode_free(graph->y_axis_decimals_code);
	rlib_pcode_free(graph->y_axis_decimals_code_right);

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

	for(list=graph->plots;list != NULL; list = g_slist_next(list)) {
		plot = list->data;
		rlib_pcode_free(plot->axis_code);
		rlib_pcode_free(plot->field_code);
		rlib_pcode_free(plot->label_code);
		rlib_pcode_free(plot->side_code);
		rlib_pcode_free(plot->disabled_code);
		rlib_pcode_free(plot->color_code);
		xmlFree(plot->xml_axis.xml);
		xmlFree(plot->xml_field.xml);
		xmlFree(plot->xml_label.xml);
		xmlFree(plot->xml_side.xml);
		xmlFree(plot->xml_disabled.xml);
		xmlFree(plot->xml_color.xml);
	}
}

static void rlib_free_chart(rlib *r, struct rlib_chart *chart) {
	struct rlib_chart_header_row *header_row;
	struct rlib_chart_row *row;

	rlib_pcode_free(chart->name_code);
	rlib_pcode_free(chart->title_code);
	rlib_pcode_free(chart->cols_code);
	rlib_pcode_free(chart->rows_code);
	rlib_pcode_free(chart->cell_width_code);
	rlib_pcode_free(chart->cell_height_code);
	rlib_pcode_free(chart->cell_width_padding_code);
	rlib_pcode_free(chart->cell_height_padding_code);
	rlib_pcode_free(chart->label_width_code);
	rlib_pcode_free(chart->header_row_code);

	xmlFree(chart->xml_name.xml);
	xmlFree(chart->xml_title.xml);
	xmlFree(chart->xml_cols.xml);
	xmlFree(chart->xml_rows.xml);
	xmlFree(chart->xml_cell_width.xml);
	xmlFree(chart->xml_cell_height.xml);
	xmlFree(chart->xml_label_width.xml);
	xmlFree(chart->xml_header_row.xml);

	header_row = &chart->header_row;

	rlib_pcode_free(header_row->query_code);
	rlib_pcode_free(header_row->field_code);
	rlib_pcode_free(header_row->colspan_code);

	xmlFree(header_row->xml_query.xml);
	xmlFree(header_row->xml_field.xml);
	xmlFree(header_row->xml_colspan.xml);

	row = &chart->row;

	rlib_pcode_free(row->row_code);
	rlib_pcode_free(row->bar_start_code);
	rlib_pcode_free(row->bar_end_code);
	rlib_pcode_free(row->label_code);
	rlib_pcode_free(row->bar_label_code);
	rlib_pcode_free(row->bar_color_code);
	rlib_pcode_free(row->bar_label_color_code);

	xmlFree(row->xml_row.xml);
	xmlFree(row->xml_bar_start.xml);
	xmlFree(row->xml_bar_end.xml);
	xmlFree(row->xml_label.xml);
	xmlFree(row->xml_bar_label.xml);
	xmlFree(row->xml_bar_color.xml);
	xmlFree(row->xml_bar_label_color.xml);
}

void rlib_free_report(rlib *r, struct rlib_report *report) {
	struct rlib_element *e, *prev;

	if(report->doc != NULL) {
/*		xmlFreeDoc(report->doc); */
	} else {
		g_free(report->contents);
	}

	rlib_pcode_free(report->font_size_code);
	rlib_pcode_free(report->query_code);
	rlib_pcode_free(report->orientation_code);
	rlib_pcode_free(report->top_margin_code);
	rlib_pcode_free(report->left_margin_code);
	rlib_pcode_free(report->bottom_margin_code);
	rlib_pcode_free(report->pages_across_code);
	rlib_pcode_free(report->suppress_page_header_first_page_code);
	rlib_pcode_free(report->suppress_code);
	rlib_pcode_free(report->detail_columns_code);
	rlib_pcode_free(report->column_pad_code);
	rlib_pcode_free(report->uniquerow_code);

	rlib_free_output(r, report->report_header);
	rlib_free_output(r, report->page_header);
	rlib_free_output(r, report->page_footer);
	rlib_free_output(r, report->report_footer);
	rlib_free_output(r, report->detail.fields);
	rlib_free_output(r, report->detail.headers);
	rlib_free_output(r, report->alternate.nodata);

	rlib_free_graph(r, &report->graph);
	rlib_free_chart(r, &report->chart);

	g_free(report->position_top);
	g_free(report->position_bottom);
	g_free(report->bottom_size);

	rlib_value_free(&report->uniquerow);


	if(report->breaks != NULL) {
		for(e = report->breaks; e != NULL; e=e->next) {
			struct rlib_report_break *rb = e->data;
			struct rlib_element *be;
			rlib_free_output(r, rb->header);
			rlib_free_output(r, rb->footer);
			for(be = rb->fields; be != NULL; be=be->next) {
				struct rlib_break_fields *bf = be->data;
				rlib_break_free_pcode(r, bf);
				g_free(bf);
			}

			while(rb->fields) {
				prev = NULL;
				for(be = rb->fields; be->next != NULL; be=be->next) {
					prev = be;
				}
				g_free(be);
				if(prev != NULL)
					prev->next = NULL;
				else
					break;
			}
			rlib_pcode_free(rb->newpage_code);
			rlib_pcode_free(rb->headernewpage_code);
			rlib_pcode_free(rb->suppressblank_code);
			xmlFree(rb->xml_name.xml);
			xmlFree(rb->xml_newpage.xml);
			xmlFree(rb->xml_headernewpage.xml);
			xmlFree(rb->xml_suppressblank.xml);
			g_free(rb);
		}

		while(report->breaks) {
			prev = NULL;
			for(e = report->breaks; e->next != NULL; e=e->next) {
				prev = e;
			}
			g_free(e);
			if(prev != NULL)
				prev->next = NULL;
			else
				break;
		}

	}

	if(report->variables != NULL) {
		for(e = report->variables; e != NULL; e=e->next) {
			struct rlib_report_variable *rv = e->data;

			rlib_pcode_free(rv->code);
			rlib_pcode_free(rv->ignore_code);

			xmlFree(rv->xml_name.xml);
			xmlFree(rv->xml_str_type.xml);
			xmlFree(rv->xml_value.xml);
			xmlFree(rv->xml_resetonbreak.xml);
			xmlFree(rv->xml_ignore.xml);

			if(rv->precalculated_values != NULL) {
				g_free(rv->precalculated_values->data);
				rv->precalculated_values = g_slist_remove_link (rv->precalculated_values, rv->precalculated_values);
			}

			g_free(rv);
		}

	}
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
}

void rlib_free_part_td(rlib *r, struct rlib_part *part, GSList *part_deviations) {
	GSList *element;
	GSList *reports;
	for(element = part_deviations;element != NULL;element = g_slist_next(element)) {
		struct rlib_part_td *td = element->data;
		rlib_pcode_free(td->width_code);
		rlib_pcode_free(td->height_code);
		rlib_pcode_free(td->border_width_code);
		rlib_pcode_free(td->border_color_code);
		xmlFree(td->xml_width.xml);
		xmlFree(td->xml_height.xml);
		xmlFree(td->xml_border_width.xml);
		xmlFree(td->xml_border_color.xml);

		for(reports=td->reports;reports != NULL;reports = g_slist_next(reports)) {
			struct rlib_report *report = reports->data;
			if(report != NULL)
				rlib_free_report(r, report);
		}

		g_slist_foreach(td->reports, (GFunc) g_free, NULL);
		g_slist_free(td->reports);
		g_free(td);
	}
}

static void rlib_free_part_tr(rlib *r, struct rlib_part *part) {
	GSList *element;

	for(element = part->part_rows;element != NULL;element = g_slist_next(element)) {
		struct rlib_part_tr *tr = element->data;
		rlib_pcode_free(tr->layout_code);
		rlib_pcode_free(tr->newpage_code);
		xmlFree(tr->xml_layout.xml);
		xmlFree(tr->xml_newpage.xml);
		rlib_free_part_td(r, part, tr->part_deviations);
		g_slist_free(tr->part_deviations);
		g_free(tr);
	}
	g_slist_free(part->part_rows);
}

void rlib_free_part(rlib *r, struct rlib_part *part) {

	if(part == NULL)
		return;

	rlib_pcode_free(part->orientation_code);
	rlib_pcode_free(part->font_size_code);
	rlib_pcode_free(part->top_margin_code);
	rlib_pcode_free(part->left_margin_code);
	rlib_pcode_free(part->bottom_margin_code);
	rlib_pcode_free(part->paper_type_code);
	rlib_pcode_free(part->pages_across_code);
	rlib_pcode_free(part->suppress_page_header_first_page_code);
	rlib_pcode_free(part->suppress_code);

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
	rlib_free_output(r, part->page_footer);
	rlib_free_output(r, part->report_header);

	rlib_free_part_tr(r, part);
}

void rlib_free_tree(rlib *r) {
	int i;
	if (r->queries_count > 0) {
		for (i = 0; i < r->parts_count; i++) {
			struct rlib_part *part = r->parts[i];
			rlib_free_part(r, part);
			g_free(r->reportstorun[i].name);
			g_free(r->reportstorun[i].dir);
			g_free(part);
			r->parts[i] = NULL;
		}
	}

	g_slist_free_full(r->search_paths, g_free);
}

void rlib_free_results(rlib *r) {
	int i;
	for(i=0;i<r->queries_count;i++) {
		INPUT(r, i)->free_result(INPUT(r, i), r->results[i]->result);
	}
}

void rlib_free_results_and_queries(rlib *r) {
	int i;
	for(i=0;i<r->queries_count;i++) {
		if (r->results[i]->result)
			INPUT(r, i)->free_result(INPUT(r, i), r->results[i]->result);
		g_free(r->queries[i]->sql);
		g_free(r->queries[i]->name);
	}
}


gint rlib_free_follower(rlib *r ) {
	gint i;
	for(i=0; i<r->resultset_followers_count; i++) {
		rlib_pcode_free(r->followers[i].leader_code);
		rlib_pcode_free(r->followers[i].follower_code);
	}

	return TRUE;
}

DLL_EXPORT_SYM gint rlib_free(rlib *r) {
	int i;
	rlib_charencoder_free(r->output_encoder);
	g_free(r->output_encoder_name);

	rlib_free_tree(r);
	xmlCleanupParser();
	rlib_free_results_and_queries(r);
	for(i=0;i<r->inputs_count;i++) {
		rlib_charencoder_free(r->inputs[i].input->info.encoder);
		r->inputs[i].input->input_close(r->inputs[i].input);
		r->inputs[i].input->free(r->inputs[i].input);
		if(r->inputs[i].handle != NULL)
			g_module_close(r->inputs[i].handle);
		g_free(r->inputs[i].name);
	}

	if(r->did_execute) {
		OUTPUT(r)->free(r);
		ENVIRONMENT(r)->free(r);
	}
	g_hash_table_destroy(r->output_parameters);
	g_hash_table_destroy(r->input_metadata);
	g_hash_table_destroy(r->parameters);
	rlib_free_follower(r);
	g_free(r->special_locale);
	g_free(r->current_locale);
	for(i=0;i<r->queries_count;i++) {
		g_free(r->queries[i]);
		g_free(r->results[i]);
	}
	g_free(r->queries);
	g_free(r->results);
	g_free(r);
	return 0;
}
