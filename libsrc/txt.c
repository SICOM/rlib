/*
 *  Copyright (C) 2003-2006 SICOM Systems, INC.
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

#include "config.h"
#include "rlib-internal.h"

#define TEXT 1
#define DELAY 2

struct _packet {
	char type;
	gpointer data;
};

struct _private {
	GSList **top;
	GSList **bottom;
	gchar *both;
	gint length;
	gint page_number;
};

static void print_text(rlib *r, const gchar *text, gint backwards) {
	gint current_page = OUTPUT_PRIVATE(r)->page_number;
	struct _packet *packet = NULL;

	if(backwards) {
		if(OUTPUT_PRIVATE(r)->bottom[current_page] != NULL)
			packet = OUTPUT_PRIVATE(r)->bottom[current_page]->data;
		if(packet != NULL && packet->type == TEXT) {
			g_string_append(packet->data, text);
		} else {	
			packet = g_new0(struct _packet, 1);
			packet->type = TEXT;
			packet->data = g_string_new(text);
			OUTPUT_PRIVATE(r)->bottom[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->bottom[current_page], packet);
		}
	} else {
		if(OUTPUT_PRIVATE(r)->top[current_page] != NULL)
			packet = OUTPUT_PRIVATE(r)->top[current_page]->data;
		if(packet != NULL && packet->type == TEXT) {
			g_string_append(packet->data, text);
		} else {	
			packet = g_new0(struct _packet, 1);
			packet->type = TEXT;
			packet->data = g_string_new(text);
			OUTPUT_PRIVATE(r)->top[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->top[current_page], packet);
		}
	}
}

static gfloat txt_get_string_width(rlib *r, const gchar *text) {
	return 1;
}

static void txt_print_text(rlib *r, gfloat left_origin, gfloat bottom_origin, const gchar *text, gint backwards, struct rlib_line_extra_data *extra_data) {
	print_text(r, text, backwards);
}

static void txt_start_new_page(rlib *r, struct rlib_part *part) {
	if(r->current_page_number <= 0) 
		r->current_page_number++;
	part->position_bottom[0] = 11-part->bottom_margin;
}

static void txt_init_end_page(rlib *r) {}
static void txt_start_rlib_report(rlib *r) {}
static void txt_end_rlib_report(rlib *r) {}
static void txt_finalize_private(rlib *r) {}

static void txt_spool_private(rlib *r) {
	ENVIRONMENT(r)->rlib_write_output(OUTPUT_PRIVATE(r)->both, OUTPUT_PRIVATE(r)->length);
}

static void txt_end_line(rlib *r, int backwards) {
	print_text(r, "\n", backwards);
}

static void txt_start_report(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_end_report(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_start_report_field_headers(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_end_report_field_headers(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_start_report_field_details(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_end_report_field_details(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_start_report_line(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_end_report_line(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_start_report_header(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_end_report_header(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_start_report_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_end_report_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}


static void txt_start_part(rlib *r, struct rlib_part *part) {
	gint pages_across = part->pages_across;
	OUTPUT_PRIVATE(r)->top = g_new0(GSList *, pages_across);
	OUTPUT_PRIVATE(r)->bottom = g_new0(GSList *, pages_across);
}

static gchar * txt_callback(struct rlib_delayed_extra_data *delayed_data) {
	struct rlib_line_extra_data *extra_data = &delayed_data->extra_data;
	rlib *r = delayed_data->r;
	gchar *buf = NULL, *buf2 = NULL;
	
	rlib_execute_pcode(r, &extra_data->rval_code, extra_data->field_code, NULL);	
	rlib_format_string(r, &buf, extra_data->report_field, &extra_data->rval_code);
	rlib_align_text(r, &buf2, buf, extra_data->report_field->align, extra_data->report_field->width);
	g_free(buf);
	g_free(delayed_data);
	return buf2;
}

static void txt_print_text_delayed(rlib *r, struct rlib_delayed_extra_data *delayed_data, int backwards, int rval_type) {
	gint current_page = OUTPUT_PRIVATE(r)->page_number;
	struct _packet *packet = g_new0(struct _packet, 1);
	packet->type = DELAY;
	packet->data = delayed_data;
	
	if(backwards)
		OUTPUT_PRIVATE(r)->bottom[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->bottom[current_page], packet);
	else
		OUTPUT_PRIVATE(r)->top[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->top[current_page], packet);
}


static void txt_end_part(rlib *r, struct rlib_part *part) {
	gint i;
	gchar *old;
	for(i=0;i<part->pages_across;i++) {
		GSList *tmp = OUTPUT_PRIVATE(r)->top[i]; 
		GSList *list = NULL;
		while(tmp != NULL) {
			list = g_slist_prepend(list, tmp->data);
			tmp = tmp->next;
		}

		while(list != NULL) {
			struct _packet *packet = list->data;
			gchar *str;	
			if(packet->type == DELAY) {
				str = txt_callback(packet->data);
			} else {
				str = ((GString *)packet->data)->str;
			}
			if(OUTPUT_PRIVATE(r)->both  == NULL) {
				OUTPUT_PRIVATE(r)->both  = str;
			} else {
				old = OUTPUT_PRIVATE(r)->both ;
				OUTPUT_PRIVATE(r)->both  = g_strconcat(OUTPUT_PRIVATE(r)->both , str, NULL);
				g_free(old);
				if(packet->type == TEXT)
					g_string_free(packet->data, TRUE);
			}
			g_free(packet);
			list = list->next;
		}

		g_slist_free(list);
		list = NULL;
		tmp = OUTPUT_PRIVATE(r)->bottom[i]; 
		while(tmp != NULL) {
			list = g_slist_prepend(list, tmp->data);
			tmp = tmp->next;
		}

		while(list != NULL) {
			struct _packet *packet = list->data;
			gchar *str;	
			if(packet->type == DELAY) {
				str = txt_callback(packet->data);
			} else {
				str = ((GString *)packet->data)->str;
			}
			if(OUTPUT_PRIVATE(r)->both  == NULL) {
				OUTPUT_PRIVATE(r)->both  = str;
			} else {
				old = OUTPUT_PRIVATE(r)->both;
				OUTPUT_PRIVATE(r)->both  = g_strconcat(OUTPUT_PRIVATE(r)->both , str, NULL);
				g_free(old);
				if(packet->type == TEXT)
					g_string_free(packet->data, TRUE);

			}
			g_free(packet);
			list = list->next;
		}
		
		g_slist_free(list);
		list = NULL;
		OUTPUT_PRIVATE(r)->bottom[i] = NULL;		
		OUTPUT_PRIVATE(r)->top[i] = NULL;		

	}
	OUTPUT_PRIVATE(r)->length = strlen(OUTPUT_PRIVATE(r)->both);
}

static void txt_end_page(rlib *r, struct rlib_part *part) {
	r->current_page_number++;
	r->current_line_number = 1;
}

static int txt_free(rlib *r) {
	g_free(OUTPUT_PRIVATE(r)->both);
	g_free(OUTPUT_PRIVATE(r));
	g_free(OUTPUT(r));
	return 0;
}

static char *txt_get_output(rlib *r) {
	return OUTPUT_PRIVATE(r)->both;
}

static long txt_get_output_length(rlib *r) {
	return OUTPUT_PRIVATE(r)->length;
}

static void txt_set_working_page(rlib *r, struct rlib_part *part, int page) {
	OUTPUT_PRIVATE(r)->page_number = page;
}

static void txt_set_fg_color(rlib *r, gfloat red, gfloat green, gfloat blue) {}
static void txt_set_bg_color(rlib *r, gfloat red, gfloat green, gfloat blue) {}
static void txt_hr(rlib *r, gint backwards, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall, 
struct rlib_rgb *color, gfloat indent, gfloat length) {}
static void txt_start_draw_cell_background(rlib *r, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall, 
struct rlib_rgb *color) {}
static void txt_end_draw_cell_background(rlib *r) {}
static void txt_start_boxurl(rlib *r, struct rlib_part * part, gfloat
left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall, gchar *url, gint backwards) {}
static void txt_end_boxurl(rlib *r, gint backwards) {}
static void txt_background_image(rlib *r, gfloat left_origin, gfloat bottom_origin, gchar *nname, gchar *type, gfloat nwidth, gfloat nheight) {}
static void txt_set_font_point(rlib *r, gint point) {}
static void txt_start_line(rlib *r, gint backwards) {}
static void txt_start_output_section(rlib *r, struct rlib_report_output_array *roa) {}
static void txt_end_output_section(rlib *r, struct rlib_report_output_array *roa) {}
static void txt_start_evil_csv(rlib *r) {}
static void txt_end_evil_csv(rlib *r) {}
static void txt_start_part_table(rlib *r, struct rlib_part *part) {}
static void txt_end_part_table(rlib *r, struct rlib_part *part) {}
static void txt_start_part_tr(rlib *r, struct rlib_part *part) {}
static void txt_end_part_tr(rlib *r, struct rlib_part *part) {}
static void txt_start_part_td(rlib *r, struct rlib_part *part, gfloat width, gfloat height) {}
static void txt_end_part_td(rlib *r, struct rlib_part *part) {}
static void txt_start_part_pages_across(rlib *r, struct rlib_part *part, gfloat left_margin, gfloat top_margin, int width, int height, int border_width, struct rlib_rgb *color) {}
static void txt_end_part_pages_across(rlib *r, struct rlib_part *part) {}
static void txt_set_raw_page(rlib *r, struct rlib_part *part, gint page) {}
static void txt_start_bold(rlib *r) {}
static void txt_end_bold(rlib *r) {}
static void txt_start_italics(rlib *r) {}
static void txt_end_italics(rlib *r) {}

static void txt_start_graph(rlib *r, struct rlib_part *part, struct rlib_report *report, gfloat left, gfloat top, gfloat width, gfloat height, gboolean x_axis_labels_are_under_tick) {}
static void txt_graph_set_limits(rlib *r, gchar side, gdouble min, gdouble max, gdouble origin) {}
static void txt_graph_set_title(rlib *r, gchar *title) {}
static void txt_graph_x_axis_title(rlib *r, gchar *title) {}
static void txt_graph_y_axis_title(rlib *r, gchar side, gchar *title) {}
static void txt_graph_do_grid(rlib *r, gboolean just_a_box) {}
static void txt_graph_tick_x(rlib *r) {}
static void txt_graph_set_x_iterations(rlib *r, gint iterations) {}
static void txt_graph_hint_label_x(rlib *r, gchar *label) {}
static void txt_graph_label_x(rlib *r, gint iteration, gchar *label) {}
static void txt_graph_tick_y(rlib *r, gint iterations) {}
static void txt_graph_label_y(rlib *r, gchar side, gint iteration, gchar *label) {}
static void txt_graph_hint_label_y(rlib *r, gchar side, gchar *label) {}
static void txt_graph_set_data_plot_count(rlib *r, gint count) {}
static void txt_graph_plot_bar(rlib *r, gchar side, gint iteration, gint plot, gfloat height_percent, struct rlib_rgb *color,gfloat last_height, gboolean divide_iterations, gfloat raw_data, gchar *label) {}
static void txt_graph_plot_line(rlib *r, gchar side, gint iteration, gfloat p1_height, gfloat p1_last_height, gfloat p2_height, gfloat p2_last_height, struct rlib_rgb *color, gfloat raw_data, gchar *label, gint row_count) {}
static void txt_graph_plot_pie(rlib *r, gfloat start, gfloat end, gboolean offset, struct rlib_rgb *color, gfloat raw_data, gchar *label) {}
static void txt_graph_hint_legend(rlib *r, gchar *label) {}
static void txt_graph_draw_legend(rlib *r) {}
static void txt_graph_draw_legend_label(rlib *r, gint iteration, gchar *label, struct rlib_rgb *color, gboolean is_line) {}
static void txt_end_graph(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_graph_draw_line(rlib *r, gfloat x, gfloat y, gfloat new_x, gfloat new_y, struct rlib_rgb *color) {}
static void txt_graph_set_name(rlib *r, gchar *name) {}
static void txt_graph_set_legend_bg_color(rlib *r, struct rlib_rgb *rgb) {}
static void txt_graph_set_legend_orientation(rlib *r, gint orientation) {}
static void txt_graph_set_draw_x_y(rlib *r, gboolean draw_x, gboolean draw_y) {}
static void txt_graph_set_bold_titles(rlib *r, gboolean bold_titles) {}
static void txt_graph_set_grid_color(rlib *r, struct rlib_rgb *rgb) {}
static void txt_start_part_header(rlib *r, struct rlib_part *part) {}
static void txt_end_part_header(rlib *r, struct rlib_part *part) {}
static void txt_start_part_page_header(rlib *r, struct rlib_part *part) {}
static void txt_end_part_page_header(rlib *r, struct rlib_part *part) {}
static void txt_start_part_footer(rlib *r, struct rlib_part *part) {}
static void txt_end_part_footer(rlib *r, struct rlib_part *part) {}
static void txt_start_report_page_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_end_report_page_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_start_report_break_header(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void txt_end_report_break_header(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void txt_start_report_break_footer(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void txt_end_report_break_footer(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void txt_start_report_no_data(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void txt_end_report_no_data(rlib *r, struct rlib_part *part, struct rlib_report *report) {}


void rlib_txt_new_output_filter(rlib *r) {
	OUTPUT(r) = g_malloc(sizeof(struct output_filter));
	r->o->private = g_malloc(sizeof(struct _private));
	memset(OUTPUT_PRIVATE(r), 0, sizeof(struct _private));
	
	OUTPUT(r)->do_align = TRUE;
	OUTPUT(r)->do_breaks = TRUE;
	OUTPUT(r)->do_grouptext = FALSE;	
	OUTPUT(r)->paginate = FALSE;
	OUTPUT(r)->trim_links = FALSE;
	OUTPUT(r)->do_graph = FALSE;
	
	OUTPUT(r)->get_string_width = txt_get_string_width;
	OUTPUT(r)->print_text = txt_print_text;
	OUTPUT(r)->set_fg_color = txt_set_fg_color;
	OUTPUT(r)->set_bg_color = txt_set_bg_color;
	OUTPUT(r)->print_text_delayed = txt_print_text_delayed;	
	OUTPUT(r)->hr = txt_hr;
	OUTPUT(r)->start_draw_cell_background = txt_start_draw_cell_background;
	OUTPUT(r)->end_draw_cell_background = txt_end_draw_cell_background;
	OUTPUT(r)->start_boxurl = txt_start_boxurl;
	OUTPUT(r)->end_boxurl = txt_end_boxurl;
	OUTPUT(r)->background_image = txt_background_image;
	OUTPUT(r)->line_image = txt_background_image;
	OUTPUT(r)->set_font_point = txt_set_font_point;
	OUTPUT(r)->start_new_page = txt_start_new_page;
	OUTPUT(r)->end_page = txt_end_page;   
	OUTPUT(r)->init_end_page = txt_init_end_page;
	OUTPUT(r)->start_rlib_report = txt_start_rlib_report;
	OUTPUT(r)->end_rlib_report = txt_end_rlib_report;
	OUTPUT(r)->start_report = txt_start_report;
	OUTPUT(r)->end_report = txt_end_report;
	OUTPUT(r)->start_report_field_headers = txt_start_report_field_headers;
	OUTPUT(r)->end_report_field_headers = txt_end_report_field_headers;
	OUTPUT(r)->start_report_field_details = txt_start_report_field_details;
	OUTPUT(r)->end_report_field_details = txt_end_report_field_details;
	OUTPUT(r)->start_report_line = txt_start_report_line;
	OUTPUT(r)->end_report_line = txt_end_report_line;
	OUTPUT(r)->start_part = txt_start_part;
	OUTPUT(r)->end_part = txt_end_part;
	OUTPUT(r)->start_report_header = txt_start_report_header;
	OUTPUT(r)->end_report_header = txt_end_report_header;
	OUTPUT(r)->start_report_footer = txt_start_report_footer;
	OUTPUT(r)->end_report_footer = txt_end_report_footer;
	OUTPUT(r)->start_part_header = txt_start_part_header;
	OUTPUT(r)->end_part_header = txt_end_part_header;
	OUTPUT(r)->start_part_page_header = txt_start_part_page_header;
	OUTPUT(r)->end_part_page_header = txt_end_part_page_header;
	OUTPUT(r)->start_part_page_footer = txt_start_part_footer;
	OUTPUT(r)->end_part_page_footer = txt_end_part_footer;
	OUTPUT(r)->start_report_page_footer = txt_start_report_page_footer;
	OUTPUT(r)->end_report_page_footer = txt_end_report_page_footer;
	OUTPUT(r)->start_report_break_header = txt_start_report_break_header;
	OUTPUT(r)->end_report_break_header = txt_end_report_break_header;
	OUTPUT(r)->start_report_break_footer = txt_start_report_break_footer;
	OUTPUT(r)->end_report_break_footer = txt_end_report_break_footer;
	OUTPUT(r)->start_report_no_data = txt_start_report_no_data;
	OUTPUT(r)->end_report_no_data = txt_end_report_no_data;
		
	OUTPUT(r)->finalize_private = txt_finalize_private;
	OUTPUT(r)->spool_private = txt_spool_private;
	OUTPUT(r)->start_line = txt_start_line;
	OUTPUT(r)->end_line = txt_end_line;
	OUTPUT(r)->start_output_section = txt_start_output_section;   
	OUTPUT(r)->end_output_section = txt_end_output_section; 
	OUTPUT(r)->start_evil_csv = txt_start_evil_csv;   
	OUTPUT(r)->end_evil_csv = txt_end_evil_csv; 
	OUTPUT(r)->get_output = txt_get_output;
	OUTPUT(r)->get_output_length = txt_get_output_length;
	OUTPUT(r)->set_working_page = txt_set_working_page;  
	OUTPUT(r)->set_raw_page = txt_set_raw_page; 
	OUTPUT(r)->start_part_table = txt_start_part_table; 
	OUTPUT(r)->end_part_table = txt_end_part_table; 
	OUTPUT(r)->start_part_tr = txt_start_part_tr; 
	OUTPUT(r)->end_part_tr = txt_end_part_tr; 
	OUTPUT(r)->start_part_td = txt_start_part_td; 
	OUTPUT(r)->end_part_td = txt_end_part_td; 
	OUTPUT(r)->start_part_pages_across = txt_start_part_pages_across; 
	OUTPUT(r)->end_part_pages_across = txt_end_part_pages_across; 
	OUTPUT(r)->start_bold = txt_start_bold;
	OUTPUT(r)->end_bold = txt_end_bold;
	OUTPUT(r)->start_italics = txt_start_italics;
	OUTPUT(r)->end_italics = txt_end_italics;

	OUTPUT(r)->start_graph = txt_start_graph;
	OUTPUT(r)->graph_set_limits = txt_graph_set_limits;
	OUTPUT(r)->graph_set_title = txt_graph_set_title;
	OUTPUT(r)->graph_set_name = txt_graph_set_name;
	OUTPUT(r)->graph_set_legend_bg_color = txt_graph_set_legend_bg_color;
	OUTPUT(r)->graph_set_legend_orientation = txt_graph_set_legend_orientation;
	OUTPUT(r)->graph_set_draw_x_y = txt_graph_set_draw_x_y;
	OUTPUT(r)->graph_set_bold_titles = txt_graph_set_bold_titles;
	OUTPUT(r)->graph_set_grid_color = txt_graph_set_grid_color;
	OUTPUT(r)->graph_x_axis_title = txt_graph_x_axis_title;
	OUTPUT(r)->graph_y_axis_title = txt_graph_y_axis_title;
	OUTPUT(r)->graph_do_grid = txt_graph_do_grid;
	OUTPUT(r)->graph_tick_x = txt_graph_tick_x;
	OUTPUT(r)->graph_set_x_iterations = txt_graph_set_x_iterations;
	OUTPUT(r)->graph_tick_y = txt_graph_tick_y;
	OUTPUT(r)->graph_hint_label_x = txt_graph_hint_label_x;
	OUTPUT(r)->graph_label_x = txt_graph_label_x;
	OUTPUT(r)->graph_label_y = txt_graph_label_y;
	OUTPUT(r)->graph_draw_line = txt_graph_draw_line;
	OUTPUT(r)->graph_plot_bar = txt_graph_plot_bar;
	OUTPUT(r)->graph_plot_line = txt_graph_plot_line;
	OUTPUT(r)->graph_plot_pie = txt_graph_plot_pie;
	OUTPUT(r)->graph_set_data_plot_count = txt_graph_set_data_plot_count;
	OUTPUT(r)->graph_hint_label_y = txt_graph_hint_label_y;
	OUTPUT(r)->graph_hint_legend = txt_graph_hint_legend;
	OUTPUT(r)->graph_draw_legend = txt_graph_draw_legend;
	OUTPUT(r)->graph_draw_legend_label = txt_graph_draw_legend_label;
	OUTPUT(r)->end_graph = txt_end_graph;

	OUTPUT(r)->free = txt_free;  
}
