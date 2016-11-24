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

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "rlib-internal.h"
#include "pcode.h"

#define MAX_COL	100

struct _private {
	gchar col[MAX_COL][MAXSTRLEN];
	gchar rval_type[MAX_COL];
	gchar *top;
	gint top_size;
	gint top_total_size;
	gint length;
	gboolean only_quote_strings;
	gboolean no_quotes;
	gboolean new_line_on_end_of_line;
	gchar csv_delimeter;
};

static void print_text(rlib *r, const gchar *text, gboolean backwards UNUSED, gint col, gint rval_type) {
	gchar *new_text;

	rlib_encode_text(r, text, &new_text);

	if (col < MAX_COL) {
		OUTPUT_PRIVATE(r)->rval_type[col] = rval_type;
		if (OUTPUT_PRIVATE(r)->col[col][0] == 0)
			strcpy(OUTPUT_PRIVATE(r)->col[col], new_text);
		else {
			gchar *tmp;
			tmp = g_strdup_printf("%s %s", OUTPUT_PRIVATE(r)->col[col], new_text);
			if (strlen(tmp) > MAXSTRLEN)
				tmp[MAXSTRLEN] = 0;
			strncpy(OUTPUT_PRIVATE(r)->col[col], tmp, MAXSTRLEN);
			g_free(tmp);
		}
	}
	g_free(new_text);
}

static gdouble csv_get_string_width(rlib *r UNUSED, const gchar *text UNUSED) {
	return 1.0;
}

static void csv_print_text(rlib *r, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, const gchar *text, gboolean backwards, struct rlib_line_extra_data *extra_data) {
	gint rval_type = rlib_value_get_type(r, &extra_data->rval_code);
	if (extra_data->col)
		print_text(r, text, backwards, extra_data->col - 1, rval_type);
}

static void csv_start_new_page(rlib *r UNUSED, struct rlib_part *part) {
	part->position_bottom[0] = 11-part->bottom_margin;
}

static void csv_finalize_private(rlib *r) {
	OUTPUT_PRIVATE(r)->length = OUTPUT_PRIVATE(r)->top_size;
}

static void csv_spool_private(rlib *r) {
	if (OUTPUT_PRIVATE(r)->top != NULL)
		ENVIRONMENT(r)->rlib_write_output(OUTPUT_PRIVATE(r)->top, strlen(OUTPUT_PRIVATE(r)->top));
}

static char csv_get_delimiter(rlib *r) {
	return OUTPUT_PRIVATE(r)->csv_delimeter;
}

static void really_print_text(rlib *r, const gchar *passed_text, gint rval_type, gint field_count) {
	gchar buf[MAXSTRLEN], text[MAXSTRLEN];
	gchar *str_ptr;
	gchar csv_delimeter;
	gint text_size = strlen(passed_text);
	gint *size;
	gint i, spot = 0;

	if (text_size > MAXSTRLEN - 5) {
		text_size = MAXSTRLEN - 5;
	}

	csv_delimeter = csv_get_delimiter(r);

	if (passed_text != NULL && passed_text[0] != '\r') {
		for (i = 0; i < text_size + 1; i++) {
			if (passed_text[i] == '"')
				text[spot++] = '\\';
			text[spot++] = passed_text[i];			
		}
		if (OUTPUT_PRIVATE(r)->no_quotes == TRUE) {
			for (i = spot - 2; i >= 0 && text[i] == ' '; i--)
				/* intentionally empty loop body */;
			text[++i] = '\0';
			text_size = i;
			spot = text_size + 1;
		}

		if (strlen(text) > MAXSTRLEN-4)
			text[MAXSTRLEN-4] = 0;

		if ((OUTPUT_PRIVATE(r)->only_quote_strings == FALSE && OUTPUT_PRIVATE(r)->no_quotes == FALSE) || (OUTPUT_PRIVATE(r)->only_quote_strings == TRUE && rval_type == RLIB_VALUE_STRING)) {
			text_size = spot -1;
			text_size += 2;

			if (field_count == 0) {
				sprintf(buf, "\"%s\"", text);
			} else {
				/* Handle null delimeter */
				if (csv_delimeter) {
					sprintf(buf, "%c\"%s\"", csv_delimeter, text);
					text_size += 1;
				} else {
					sprintf(buf, "\"%s\"", text);					
				}
			}
		} else {
			text_size = spot -1;
			if (field_count == 0) {
				sprintf(buf, "%s", text);
			} else {
				/* Handle null delimeter */
				if (csv_delimeter) {
					sprintf(buf, "%c%s", csv_delimeter, text);
					text_size += 1;
				} else {
					strcat(buf, text);
				}
			}
		}
	} else {
		strcpy(buf, passed_text);
	}

	make_more_space_if_necessary(&OUTPUT_PRIVATE(r)->top, &OUTPUT_PRIVATE(r)->top_size, 
		&OUTPUT_PRIVATE(r)->top_total_size, text_size);
	str_ptr = OUTPUT_PRIVATE(r)->top;	
	size = &OUTPUT_PRIVATE(r)->top_size;
	memcpy(str_ptr + (*size), buf, text_size+1);
	*size = (*size) + text_size;
}

static void print_csv_line(rlib *r) {
	gint i;
	gint biggest = -1;
	for (i = 0; i<MAX_COL; i++)
		if (OUTPUT_PRIVATE(r)->col[i][0] != 0)
			biggest = i;
	
	if (biggest >= 0) {
		for(i=0;i<=biggest;i++)
			if (OUTPUT_PRIVATE(r)->col[i][0] != 0) 
				really_print_text(r, OUTPUT_PRIVATE(r)->col[i], OUTPUT_PRIVATE(r)->rval_type[i], i);
			else
				really_print_text(r, "", RLIB_VALUE_NONE, i);
		
		really_print_text(r, "\r\n", RLIB_VALUE_NONE, i);
	}	
}

static void csv_start_output_section(rlib *r UNUSED, struct rlib_report_output_array *roa UNUSED) {}

static void csv_start_evil_csv(rlib *r) {
	gint i;

	for (i = 0; i < MAX_COL; i++) {
		OUTPUT_PRIVATE(r)->col[i][0] = 0;
	}
}

static void csv_end_output_section(rlib *r UNUSED,  struct rlib_report_output_array *roa UNUSED) {}

static void csv_end_evil_csv(rlib *r) {
	if (OUTPUT_PRIVATE(r)->new_line_on_end_of_line == FALSE)
		print_csv_line(r);
}

static void csv_end_line(rlib *r, gboolean backwards UNUSED) {
	if (OUTPUT_PRIVATE(r)->new_line_on_end_of_line == TRUE) {
		print_csv_line(r);
		csv_start_evil_csv(r);
	}
}


static void csv_end_page(rlib *r, struct rlib_part *part UNUSED) {
	r->current_page_number++;
	r->current_line_number = 1;
}

static char *csv_get_output(rlib *r) {
	return OUTPUT_PRIVATE(r)->top;
}

static gsize csv_get_output_length(rlib *r) {
	return OUTPUT_PRIVATE(r)->top_size;
}

static void csv_set_working_page(rlib *r UNUSED, struct rlib_part *part UNUSED, gint page UNUSED) {}
static void csv_set_fg_color(rlib *r UNUSED, gdouble red UNUSED, gdouble green UNUSED, gdouble blue UNUSED) {}
static void csv_set_bg_color(rlib *r UNUSED, gdouble red UNUSED, gdouble green UNUSED, gdouble blue UNUSED) {}
static void csv_hr(rlib *r UNUSED, gboolean backwards UNUSED, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gdouble how_long UNUSED, gdouble how_tall UNUSED, struct rlib_rgb *color UNUSED, gdouble indent UNUSED, gdouble length UNUSED) {}
static void csv_start_draw_cell_background(rlib *r UNUSED, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gdouble how_long UNUSED, gdouble how_tall UNUSED, struct rlib_rgb *color UNUSED) {}
static void csv_end_draw_cell_background(rlib *r UNUSED) {}
static void csv_start_boxurl(rlib *r UNUSED, struct rlib_part *part UNUSED, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gdouble how_long UNUSED, gdouble how_tall UNUSED, gchar *url UNUSED, gboolean backwards UNUSED) {}
static void csv_end_boxurl(rlib *r UNUSED, gboolean backwards UNUSED) {}
static void csv_background_image(rlib *r UNUSED, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gchar *nname UNUSED, gchar *type UNUSED, gdouble nwidth UNUSED, gdouble nheight UNUSED) {}
static void csv_init_end_page(rlib *r UNUSED) {}
static void csv_start_line(rlib *r UNUSED, gboolean backwards UNUSED) {}
static void csv_start_part(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_start_report(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_end_report(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_start_report_field_headers(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_end_report_field_headers(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_start_report_field_details(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_end_report_field_details(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_start_report_line(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_end_report_line(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_start_report_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_end_report_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_start_report_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_end_report_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_end_part(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_start_rlib_report(rlib *r UNUSED) {}
static void csv_end_rlib_report(rlib *r UNUSED) {}
static void csv_set_font_point(rlib *r UNUSED, gint point UNUSED) {}
static void csv_start_part_table(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_end_part_table(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_start_part_tr(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_end_part_tr(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_start_part_td(rlib *r UNUSED, struct rlib_part *part UNUSED, gdouble width UNUSED, gdouble height UNUSED) {}
static void csv_end_part_td(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_start_part_pages_across(rlib *r UNUSED, struct rlib_part *part UNUSED, gdouble left_margin UNUSED, gdouble top_margin UNUSED, gint width UNUSED, gint height UNUSED, gint border_width UNUSED, struct rlib_rgb *color UNUSED) {}
static void csv_end_part_pages_across(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_set_raw_page(rlib *r UNUSED, struct rlib_part *part UNUSED, gint page UNUSED) {}
static void csv_start_bold(rlib *r UNUSED) {}
static void csv_end_bold(rlib *r UNUSED) {}
static void csv_start_italics(rlib *r UNUSED) {}
static void csv_end_italics(rlib *r UNUSED) {}

static void csv_start_graph(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, gdouble left UNUSED, gdouble top UNUSED, gdouble width UNUSED, gdouble height UNUSED, gboolean x_axis_labels_are_under_tick UNUSED) {}
static void csv_graph_set_limits(rlib *r UNUSED, gchar side UNUSED, gdouble min UNUSED, gdouble max UNUSED, gdouble origin UNUSED) {}
static void csv_graph_set_title(rlib *r UNUSED, gchar *title UNUSED) {}
static void csv_graph_x_axis_title(rlib *r UNUSED, gchar *title UNUSED) {}
static void csv_graph_y_axis_title(rlib *r UNUSED, gchar side UNUSED, gchar *title UNUSED) {}
static void csv_graph_do_grid(rlib *r UNUSED, gboolean just_a_box UNUSED) {}
static void csv_graph_tick_x(rlib *r UNUSED) {}
static void csv_graph_set_x_iterations(rlib *r UNUSED, gint iterations UNUSED) {}
static void csv_graph_hint_label_x(rlib *r UNUSED, gchar *label UNUSED) {}
static void csv_graph_label_x(rlib *r UNUSED, gint iteration UNUSED, gchar *label UNUSED) {}
static void csv_graph_tick_y(rlib *r UNUSED, gint iterations UNUSED) {}
static void csv_graph_label_y(rlib *r UNUSED, gchar side UNUSED, gint iteration UNUSED, gchar *label UNUSED) {}
static void csv_graph_hint_label_y(rlib *r UNUSED, gchar side UNUSED, gchar *label UNUSED) {}
static void csv_graph_set_data_plot_count(rlib *r UNUSED, gint count UNUSED) {}
static void csv_graph_plot_bar(rlib *r UNUSED, gchar side UNUSED, gint iteration UNUSED, gint plot UNUSED, gdouble height_percent UNUSED, struct rlib_rgb *color UNUSED, gdouble last_height UNUSED, gboolean divide_iterations UNUSED, gdouble raw_data UNUSED, gchar *label UNUSED) {}
static void csv_graph_plot_line(rlib *r UNUSED, gchar side UNUSED, gint iteration UNUSED, gdouble p1_height UNUSED, gdouble p1_last_height UNUSED, gdouble p2_height UNUSED, gdouble p2_last_height UNUSED, struct rlib_rgb * color UNUSED, gdouble raw_data UNUSED, gchar *label UNUSED, gint row_count UNUSED) {}
static void csv_graph_plot_pie(rlib *r UNUSED, gdouble start UNUSED, gdouble end UNUSED, gboolean offset UNUSED, struct rlib_rgb *color UNUSED, gdouble raw_data UNUSED, gchar *label UNUSED) {}
static void csv_graph_hint_legend(rlib *r UNUSED, gchar *label UNUSED) {}
static void csv_graph_draw_legend(rlib *r UNUSED) {}
static void csv_graph_draw_legend_label(rlib *r UNUSED, gint iteration UNUSED, gchar *label UNUSED, struct rlib_rgb *color UNUSED, gboolean is_line UNUSED) {}
static void csv_end_graph(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_graph_draw_line(rlib *r UNUSED, gdouble x UNUSED, gdouble y UNUSED, gdouble new_x UNUSED, gdouble new_y UNUSED, struct rlib_rgb *color UNUSED) {}

static void csv_graph_set_name(rlib *r UNUSED, gchar *name UNUSED) {}
static void csv_graph_set_legend_bg_color(rlib *r UNUSED, struct rlib_rgb *rgb UNUSED) {}
static void csv_graph_set_legend_orientation(rlib *r UNUSED, gint orientation UNUSED) {}
static void csv_graph_set_draw_x_y(rlib *r UNUSED, gboolean draw_x UNUSED, gboolean draw_y UNUSED) {}
static void csv_graph_set_bold_titles(rlib *r UNUSED, gboolean bold_titles UNUSED) {}
static void csv_graph_set_grid_color(rlib *r UNUSED, struct rlib_rgb *rgb UNUSED) {}
static void csv_start_part_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_end_part_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_start_part_page_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_end_part_page_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_start_part_page_footer(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_end_part_page_footer(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void csv_start_report_page_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_end_report_page_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_start_report_break_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void csv_end_report_break_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void csv_start_report_break_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void csv_end_report_break_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void csv_start_report_no_data(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void csv_end_report_no_data(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}

static void csv_free(rlib *r) {
	g_free(OUTPUT_PRIVATE(r)->top);
	g_free(OUTPUT_PRIVATE(r));
	g_free(OUTPUT(r));
}

void rlib_csv_new_output_filter(rlib *r) {
	gchar *csv_delimeter = NULL;
	OUTPUT(r) = g_malloc(sizeof(struct output_filter));
	r->o->private = g_malloc0(sizeof(struct _private));
	OUTPUT_PRIVATE(r)->top = NULL;
	OUTPUT_PRIVATE(r)->top_size = 0;
	OUTPUT_PRIVATE(r)->top_total_size = 0;
	OUTPUT_PRIVATE(r)->only_quote_strings = FALSE;
	OUTPUT_PRIVATE(r)->no_quotes = FALSE;
	OUTPUT_PRIVATE(r)->new_line_on_end_of_line = FALSE;
	OUTPUT_PRIVATE(r)->csv_delimeter = ',';

	if (g_hash_table_lookup(r->output_parameters, "only_quote_strings")) {
		OUTPUT_PRIVATE(r)->only_quote_strings = TRUE;
	}
	if (g_hash_table_lookup(r->output_parameters, "no_quotes")) {
		OUTPUT_PRIVATE(r)->no_quotes = TRUE;
	}
	if (g_hash_table_lookup(r->output_parameters, "new_line_on_end_of_line")) {
		OUTPUT_PRIVATE(r)->new_line_on_end_of_line = TRUE;
	}
	csv_delimeter = g_hash_table_lookup(r->output_parameters, "csv_delimeter");
	if (csv_delimeter) 
		OUTPUT_PRIVATE(r)->csv_delimeter = csv_delimeter[0];

	OUTPUT(r)->do_align = FALSE;	
	OUTPUT(r)->do_breaks = FALSE;	
	OUTPUT(r)->do_grouptext = FALSE;
	OUTPUT(r)->paginate = FALSE;
	OUTPUT(r)->trim_links = FALSE;
	OUTPUT(r)->do_graph = FALSE;

	OUTPUT(r)->get_string_width = csv_get_string_width;
	OUTPUT(r)->print_text = csv_print_text;
	/* Fix a memory leak in layout.c */
	OUTPUT(r)->print_text_delayed = NULL;
	OUTPUT(r)->finalize_text_delayed = NULL;
	OUTPUT(r)->set_fg_color = csv_set_fg_color;
	OUTPUT(r)->set_bg_color = csv_set_bg_color;
	OUTPUT(r)->hr = csv_hr;
	OUTPUT(r)->start_draw_cell_background = csv_start_draw_cell_background;
	OUTPUT(r)->end_draw_cell_background = csv_end_draw_cell_background;
	OUTPUT(r)->start_boxurl = csv_start_boxurl;
	OUTPUT(r)->end_boxurl = csv_end_boxurl;
	OUTPUT(r)->background_image = csv_background_image;
	OUTPUT(r)->line_image = csv_background_image;
	OUTPUT(r)->set_font_point = csv_set_font_point;
	OUTPUT(r)->start_new_page = csv_start_new_page;
	OUTPUT(r)->end_page = csv_end_page;   
	OUTPUT(r)->init_end_page = csv_init_end_page;
	OUTPUT(r)->start_rlib_report = csv_start_rlib_report;
	OUTPUT(r)->end_rlib_report = csv_end_rlib_report;
	OUTPUT(r)->start_part = csv_start_part;
	OUTPUT(r)->end_part = csv_end_part;
	OUTPUT(r)->start_report = csv_start_report;
	OUTPUT(r)->end_report = csv_end_report;
	OUTPUT(r)->start_report_field_headers = csv_start_report_field_headers;
	OUTPUT(r)->end_report_field_headers = csv_end_report_field_headers;	
	OUTPUT(r)->start_report_field_details = csv_start_report_field_details;
	OUTPUT(r)->end_report_field_details = csv_end_report_field_details;	
	OUTPUT(r)->start_report_line = csv_start_report_line;
	OUTPUT(r)->end_report_line = csv_end_report_line;	
	OUTPUT(r)->start_report_header = csv_start_report_header;
	OUTPUT(r)->end_report_header = csv_end_report_header;
	OUTPUT(r)->start_report_footer = csv_start_report_footer;
	OUTPUT(r)->end_report_footer = csv_end_report_footer;
	OUTPUT(r)->start_part_header = csv_start_part_header;
	OUTPUT(r)->end_part_header = csv_end_part_header;
	OUTPUT(r)->start_part_page_header = csv_start_part_page_header;
	OUTPUT(r)->end_part_page_header = csv_end_part_page_header;
	OUTPUT(r)->start_part_page_footer = csv_start_part_page_footer;
	OUTPUT(r)->end_part_page_footer = csv_end_part_page_footer;
	OUTPUT(r)->start_report_page_footer = csv_start_report_page_footer;
	OUTPUT(r)->end_report_page_footer = csv_end_report_page_footer;
	OUTPUT(r)->start_report_break_header = csv_start_report_break_header;
	OUTPUT(r)->end_report_break_header = csv_end_report_break_header;
	OUTPUT(r)->start_report_break_footer = csv_start_report_break_footer;
	OUTPUT(r)->end_report_break_footer = csv_end_report_break_footer;
	OUTPUT(r)->start_report_no_data = csv_start_report_no_data;
	OUTPUT(r)->end_report_no_data = csv_end_report_no_data;
		
	OUTPUT(r)->finalize_private = csv_finalize_private;
	OUTPUT(r)->spool_private = csv_spool_private;
	OUTPUT(r)->start_line = csv_start_line;
	OUTPUT(r)->end_line = csv_end_line;
	OUTPUT(r)->start_output_section = csv_start_output_section;   
	OUTPUT(r)->end_output_section = csv_end_output_section; 
	OUTPUT(r)->start_evil_csv = csv_start_evil_csv;   
	OUTPUT(r)->end_evil_csv = csv_end_evil_csv; 
	OUTPUT(r)->get_output = csv_get_output;
	OUTPUT(r)->get_output_length = csv_get_output_length;
	OUTPUT(r)->set_working_page = csv_set_working_page;  
	OUTPUT(r)->set_raw_page = csv_set_raw_page; 
	OUTPUT(r)->start_part_table = csv_start_part_table; 
	OUTPUT(r)->end_part_table = csv_end_part_table; 
	OUTPUT(r)->start_part_tr = csv_start_part_tr; 
	OUTPUT(r)->end_part_tr = csv_end_part_tr; 
	OUTPUT(r)->start_part_td = csv_start_part_td; 
	OUTPUT(r)->end_part_td = csv_end_part_td; 
	OUTPUT(r)->start_part_pages_across = csv_start_part_pages_across; 
	OUTPUT(r)->end_part_pages_across = csv_end_part_pages_across; 
	OUTPUT(r)->start_bold = csv_start_bold;
	OUTPUT(r)->end_bold = csv_end_bold;
	OUTPUT(r)->start_italics = csv_start_italics;
	OUTPUT(r)->end_italics = csv_end_italics;

	OUTPUT(r)->start_graph = csv_start_graph;
	OUTPUT(r)->graph_set_limits = csv_graph_set_limits;
	OUTPUT(r)->graph_set_title = csv_graph_set_title;
	OUTPUT(r)->graph_set_name = csv_graph_set_name;
	OUTPUT(r)->graph_set_legend_bg_color = csv_graph_set_legend_bg_color;
	OUTPUT(r)->graph_set_legend_orientation = csv_graph_set_legend_orientation;
	OUTPUT(r)->graph_set_draw_x_y = csv_graph_set_draw_x_y;
	OUTPUT(r)->graph_set_bold_titles = csv_graph_set_bold_titles;
	OUTPUT(r)->graph_set_grid_color = csv_graph_set_grid_color;	
	OUTPUT(r)->graph_x_axis_title = csv_graph_x_axis_title;
	OUTPUT(r)->graph_y_axis_title = csv_graph_y_axis_title;
	OUTPUT(r)->graph_do_grid = csv_graph_do_grid;
	OUTPUT(r)->graph_tick_x = csv_graph_tick_x;
	OUTPUT(r)->graph_set_x_iterations = csv_graph_set_x_iterations;
	OUTPUT(r)->graph_tick_y = csv_graph_tick_y;
	OUTPUT(r)->graph_hint_label_x = csv_graph_hint_label_x;
	OUTPUT(r)->graph_label_x = csv_graph_label_x;
	OUTPUT(r)->graph_label_y = csv_graph_label_y;
	OUTPUT(r)->graph_draw_line = csv_graph_draw_line;
	OUTPUT(r)->graph_plot_bar = csv_graph_plot_bar;
	OUTPUT(r)->graph_plot_line = csv_graph_plot_line;
	OUTPUT(r)->graph_plot_pie = csv_graph_plot_pie;
	OUTPUT(r)->graph_set_data_plot_count = csv_graph_set_data_plot_count;
	OUTPUT(r)->graph_hint_label_y = csv_graph_hint_label_y;
	OUTPUT(r)->graph_hint_legend = csv_graph_hint_legend;
	OUTPUT(r)->graph_draw_legend = csv_graph_draw_legend;
	OUTPUT(r)->graph_draw_legend_label = csv_graph_draw_legend_label;
	OUTPUT(r)->end_graph = csv_end_graph;

	OUTPUT(r)->free = csv_free;
}
