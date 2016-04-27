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
#include "config.h"

#include <math.h>

#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"
#include "util.h"
#include "rlib_langinfo.h"

#define MAX_X_TICKS 2000

#define MAX_COLS 256
#define MAX_ROWS 300

struct bar {
	gint row;
	gint start;
	gint stop;
	gchar row_label[MAXSTRLEN];
	gchar bar_label[MAXSTRLEN];
	struct rlib_rgb bar_color;
	struct rlib_rgb label_color;
};

static void string_copy(gchar *dest, gchar *src) {
	gint len = strlen(src);
	if (len > MAXSTRLEN - 1)
		len = MAXSTRLEN - 1;
	if (len > 0) {
		strncpy(dest, src, len);
		dest[len] = '\0';
	}
}

static void free_chart(GSList *row_list[]) {
	gint i;
	for (i = 0; i < MAX_ROWS; i++) {
		if (row_list[i] != NULL) {
			GSList *element = row_list[i];
 			while (element != NULL) {
				struct bar *bar = element->data;
				g_free(bar);
				element = g_slist_next(element);
 			}
			g_slist_free(row_list[i]);
		}
	}
}

DLL_EXPORT_SYM gfloat rlib_chart(rlib *r, struct rlib_part *part, struct rlib_report *report, gfloat left_margin_offset, gfloat *top_margin_offset) {
	gint i = 0;
	struct rlib_rgb bar_color;
	struct rlib_rgb label_color;
	gboolean minor_tick[MAX_X_TICKS];
	gchar title[MAXSTRLEN];
	gchar label[MAXSTRLEN];
	gchar header_row[MAXSTRLEN];
	gchar header_row_query[MAXSTRLEN];
	gchar field_header_row[MAXSTRLEN];
	gfloat hint_label_x;
	gfloat hint_label_y;
	gchar color[MAXSTRLEN];
	gint row_count = 0;
	gint col_count = 0;
	gint cols = -1;
	gint rows = -1;
	gint cell_width = -1;
	gint cell_height  = -1;
	gint cell_width_padding = -1;
	gint cell_height_padding = -1;
	gint chart_width = 600;
	gint chart_height = 400;
	struct rlib_chart *chart = &report->chart;
	gint header_row_result_num;
	gint width_offset;
	gint iteration_count = 0;

	gint chart_size;
	gint last_row;

	gfloat top_margin = 0;
	gfloat bottom_margin = 0;

	gchar x_label[MAX_COLS][MAXSTRLEN];
	gboolean already_labeled[MAX_ROWS];
	GSList *row_list[MAX_ROWS];

	for (i = 0; i < MAX_ROWS; i++) {
		row_list[i] = NULL;
		already_labeled[i] = FALSE;
	}

	memset(minor_tick, 0, sizeof(gboolean) * MAX_X_TICKS);
	for (i = 0; i < MAX_X_TICKS; i++)
		minor_tick[i] = FALSE;

	bar_color = (struct rlib_rgb){ 0, 0, 0 };
	label_color = (struct rlib_rgb){ 255, 255, 255 };

	if(!rlib_execute_as_string(r, chart->header_row_code, header_row, MAXSTRLEN))
		header_row[0] = 0;
	if(!rlib_execute_as_string(r, chart->title_code, title, MAXSTRLEN))
		title[0] = 0;

	if (rlib_execute_as_int(r, chart->cols_code, &cols)) {
		if (cols <= 0) {
			r_error(r, "cols must be a positive number\n");
			return FALSE;
		}
	}
	if (rlib_execute_as_int(r, chart->rows_code, &rows)) {
		if (rows <= 0) {
			r_error(r, "rows must be a positive number\n");
			return FALSE;
		}
	}
	if (rlib_execute_as_int(r, chart->cell_width_code, &cell_width)) {
		if (cell_width <= 0) {
			r_error(r, "cell_width must be a positive number\n");
			return FALSE;
		}
	}
	if (rlib_execute_as_int(r, chart->cell_height_code, &cell_height)) {
		if (cell_height <= 0) {
			r_error(r, "cell_height must be a positive number\n");
			return FALSE;
		}
	}
	if (rlib_execute_as_int(r, chart->cell_width_padding_code, &cell_width_padding)) {
		if (cell_width_padding < 0) {
			r_error(r, "cell_width_padding cannot be less than 0\n");
			return FALSE;
		}
	}
	if (rlib_execute_as_int(r, chart->cell_height_padding_code, &cell_height_padding)) {
		if (cell_height_padding < 0) {
			r_error(r, "cell_height_padding cannot be less than 0\n");
			return FALSE;
		}
	}

	if (rows > MAX_ROWS)
		rows = MAX_ROWS;

	top_margin =  rlib_layout_get_next_line_by_font_point(r, part, part->position_top[0] + (*top_margin_offset) + report->top_margin, 0);
	bottom_margin = part->paper->width / RLIB_PDF_DPI - part->position_bottom[0];

	chart_size = rows;

	if (cols > 0 && cell_width > 0)
		chart_width = cell_width * cols;

	if (rows > 0 && cell_height > 0)
		chart_height = cell_height * chart_size;

	OUTPUT(r)->graph_init(r);

	rlib_fetch_first_rows(r);
	if(!INPUT(r, r->current_result)->isdone(INPUT(r, r->current_result), r->results[r->current_result]->result)) {
		while(1) {
			label[0] = 0;
			if (rlib_execute_as_string(r, chart->row.label_code, label, MAXSTRLEN))
				OUTPUT(r)->graph_hint_label_y(r, RLIB_SIDE_LEFT, label);
			if(rlib_navigate_next(r, r->current_result) == FALSE) 
				break;
		}
	}

	OUTPUT(r)->graph_get_width_offset(r, &width_offset);
	chart_width += width_offset;

	col_count = 0;
	if(!rlib_execute_as_string(r, chart->header_row.query_code, header_row_query, MAXSTRLEN))
		header_row_query[0] = 0;
	hint_label_x = 0;
	hint_label_y = 0;
	if (header_row[0] != 0) {
		header_row_result_num = rlib_lookup_result(r, header_row_query);
		if (header_row_result_num < 0) {
			r_error(r, "bad header row query name: %s\n", header_row_query);
			return FALSE;
		}
		rlib_fetch_first_rows(r);
		//rlib_pcode_dump(r, chart->header_row.field_code, 0);
		if(!INPUT(r, header_row_result_num)->isdone(INPUT(r, header_row_result_num), r->results[header_row_result_num]->result)) {
			while(1) {
				if(!rlib_execute_as_string(r, chart->header_row.field_code, field_header_row, MAXSTRLEN))
					field_header_row[0] = 0;
				OUTPUT(r)->graph_hint_label_x(r, field_header_row);
				x_label[col_count][0] = 0;
				string_copy(x_label[col_count], field_header_row);
				col_count++;
				//r_error(r, "I'm IN THE HEADER ROW!!! [%s]\n", field_header_row);
				if(rlib_navigate_next(r, header_row_result_num) == FALSE) 
					break;
			}
		}
	}
	else
		col_count = cols;
	
	OUTPUT(r)->graph_get_x_label_width(r, &hint_label_x);
	OUTPUT(r)->graph_get_y_label_width(r, &hint_label_y);

	OUTPUT(r)->graph_set_x_label_width(r, hint_label_x, cell_width);
	OUTPUT(r)->graph_set_y_label_width(r, hint_label_y);

	if (OUTPUT(r)->paginate) {
		// get initial chart size
		// rows is rows left
		OUTPUT(r)->graph_get_chart_layout(r, top_margin, bottom_margin, cell_height, rows, &chart_size, &chart_height);
		if (chart_size == 0) { // could not put any rows on first page
			rlib_layout_end_page(r, part, report, TRUE);
			OUTPUT(r)->graph_get_chart_layout(r, top_margin, bottom_margin, cell_height, rows, &chart_size, &chart_height);
			if (chart_size == 0) { // error if cannot put out rows on first page???
				r_error(r, "cannot output chart!\n");
				return FALSE;
			}
		}
	} else {
		// already adjusted chart_width for x label width
		// just need to adjust chart_height
		OUTPUT(r)->graph_get_chart_layout(r, top_margin, bottom_margin, cell_height, rows, &chart_size, &chart_height);
	}

	OUTPUT(r)->start_graph(r, part, report, left_margin_offset, top_margin, chart_width, chart_height, FALSE);
	OUTPUT(r)->graph_set_x_label_width(r, hint_label_x, cell_width);
	OUTPUT(r)->graph_set_y_label_width(r, hint_label_y);

	OUTPUT(r)->graph_set_minor_ticks(r, minor_tick);
	if (title[0] != 0)
		OUTPUT(r)->graph_set_title(r, title);
	OUTPUT(r)->graph_set_draw_x_y(r, TRUE, TRUE);
	OUTPUT(r)->graph_set_is_chart(r, TRUE);

	row_count = 1;
	rlib_fetch_first_rows(r);
	if(!INPUT(r, r->current_result)->isdone(INPUT(r, r->current_result), r->results[r->current_result]->result)) {
		while(1) {
			gint t, row = -1, start = 0, stop = 0;
			if (rlib_execute_as_int(r, chart->row.row_code, &t))
				row = t;
			if (row > 0 && row <= MAX_ROWS) {
				// allocate bar and append to list
				struct bar *bar = g_new0(struct bar, 1);
				row_list[row - 1] = g_slist_append(row_list[row - 1], bar);

				bar->row = row;
				label[0] = 0;
				if (rlib_execute_as_string(r, chart->row.label_code, label, MAXSTRLEN))
					OUTPUT(r)->graph_hint_label_y(r, RLIB_SIDE_LEFT, label);
				string_copy(bar->row_label, label);
				label[0] = 0;
				rlib_execute_as_string(r, chart->row.bar_label_code, label, MAXSTRLEN);
				string_copy(bar->bar_label, label);

				bar->start = 0;
				bar->stop = 0;
				if (rlib_execute_as_int(r, chart->row.bar_start_code, &t)) {
					if (t > 0) {
						start = t;
						bar->start = t;
						if (bar->start > cols)
							bar->start = cols;
					}
				}
				if (rlib_execute_as_int(r, chart->row.bar_end_code, &t)) {
					if (t > 0) {
						stop = t;
						bar->stop = t;
						if (bar->stop > cols)
							bar->stop = cols;
					}
				}
				if (bar->start == 0 && bar->stop > 0)
					bar->start = 1;
				if (bar->stop == 0 && bar->start > 0)
					bar->stop = cols;
				if (bar->stop < bar->start) {
					r_error(r, "data index [%d], bar start (%d) greater than stop (%d)\n", row_count, start, stop);
					t = bar->stop;
					bar->stop = bar->start;
					bar->start = t;
				}
				bar_color = (struct rlib_rgb){ 0, 0, 0 };
				label_color = (struct rlib_rgb){ 255, 255, 255 };
				if (!rlib_execute_as_string(r, chart->row.bar_color_code, color, MAXSTRLEN))
					color[0] = 0;
				if (color[0] != 0)
        			rlib_parsecolor(&bar_color, color);
				bar->bar_color = bar_color;
				if (!rlib_execute_as_string(r, chart->row.bar_label_color_code, color, MAXSTRLEN))
					color[0] = 0;
				if (color[0] != 0)
        			rlib_parsecolor(&label_color, color);
				bar->label_color = label_color;

				row_count++; // actual data row
    		}
			
			if(rlib_navigate_next(r, r->current_result) == FALSE) 
				break;
		}
	}

	last_row = 0;

	while (1) {
		OUTPUT(r)->graph_set_x_iterations(r, col_count);
		OUTPUT(r)->graph_do_grid(r, TRUE);
		OUTPUT(r)->graph_set_x_tick_width(r);
		OUTPUT(r)->graph_tick_x(r);

		OUTPUT(r)->graph_tick_y(r, chart_size);

		for (i = 0; i < col_count && header_row[0] != 0; i++) {
			//r_error(r, "I'm IN THE HEADER ROW!!! [%s]\n", x_label[i]);
			OUTPUT(r)->graph_label_x(r, i, x_label[i]);
		}

		for (i = 0; i < chart_size; i++) {
			if (row_list[last_row + i] != NULL) {
				GSList *element = row_list[last_row + i];
				while (element != NULL) {
					struct bar *bar = element->data;
					if (!already_labeled[last_row + i]) {
						OUTPUT(r)->graph_label_y(r, RLIB_SIDE_LEFT, chart_size - i, bar->row_label);
						already_labeled[last_row + i] = TRUE;
					}
					//r_error(r, "We start row %d at %d and end at %d\n", bar->row, bar->start, bar->stop);
					// bar->row should equal i + 1 ???
					if (bar->start > 0 && bar->stop > 0)
						OUTPUT(r)->graph_draw_bar(r, i + 1, bar->start, bar->stop, &bar->bar_color, bar->bar_label, &bar->label_color, cell_width_padding, cell_height_padding);
					//OUTPUT(r)->graph_draw_bar(r, bar->row, bar->start, bar->stop, &bar->bar_color, bar->bar_label, &bar->label_color);
	
					element = g_slist_next(element);
				}
			}
		}

		if (last_row + chart_size >= rows)
			break;

		// HTML should never get here
		// new page
		if (OUTPUT(r)->paginate) {
			rlib_layout_end_page(r, part, report, TRUE);
			if(report->font_size != -1) {
				r->font_point = report->font_size;
				OUTPUT(r)->set_font_point(r, r->font_point);
			}
			top_margin = rlib_layout_get_next_line_by_font_point(r, part, part->position_top[0] + report->top_margin, 0);
			//top_margin = part->paper->width / RLIB_PDF_DPI - part->position_top[0];
			bottom_margin = part->paper->width / RLIB_PDF_DPI - part->position_bottom[0];
		}

		last_row += chart_size;

		// init chart
		// 
		// don't think we need this graph_start()
		//OUTPUT(r)->graph_start(r, left_margin_offset, top_margin, chart_width, chart_height, FALSE);
		// need to always (1) init the x label hint (2) set the vertical x label flag
		OUTPUT(r)->graph_set_x_label_width(r, hint_label_x, cell_width);

		if (OUTPUT(r)->paginate) {
			// get new chart size
			OUTPUT(r)->graph_get_chart_layout(r, top_margin, bottom_margin, cell_height, rows - last_row, &chart_size, &chart_height);
			if (chart_size == 0) {
				// error if cannot put rows out on new page???
				r_error(r, "cannot output chart!\n");
				free_chart(row_list);
				break;
			}
		}

		OUTPUT(r)->start_graph(r, part, report, left_margin_offset, top_margin, chart_width, chart_height, FALSE);
		OUTPUT(r)->graph_set_minor_ticks(r, minor_tick);
		if (title[0] != 0)
			OUTPUT(r)->graph_set_title(r, title);
		OUTPUT(r)->graph_set_draw_x_y(r, TRUE, TRUE);
		OUTPUT(r)->graph_set_is_chart(r, TRUE);
		// set the x y hints
		OUTPUT(r)->graph_set_x_label_width(r, hint_label_x, cell_width);
		OUTPUT(r)->graph_set_y_label_width(r, hint_label_y);

		iteration_count++;
		if (iteration_count > MAX_ROWS) {
			r_error(r, "iteration count exceeded max rows!\n");
			free_chart(row_list);
			break;
		}
	}

	OUTPUT(r)->end_graph(r, part, report); // for HTML this must be called (outputs the graph and frees it)

	free_chart(row_list);

	return TRUE;
}

