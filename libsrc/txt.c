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

#include <stdlib.h>
#include <string.h>

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
	gint pages;
	gint page_number;
};

static void print_text(rlib *r, const gchar *text, gboolean backwards) {
	gint current_page = OUTPUT_PRIVATE(r)->page_number;
	struct _packet *packet = NULL;

	if (backwards) {
		if(OUTPUT_PRIVATE(r)->bottom[current_page] != NULL)
			packet = OUTPUT_PRIVATE(r)->bottom[current_page]->data;
		if (packet != NULL && packet->type == TEXT) {
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
		if (packet != NULL && packet->type == TEXT) {
			g_string_append(packet->data, text);
		} else {
			packet = g_new0(struct _packet, 1);
			packet->type = TEXT;
			packet->data = g_string_new(text);
			OUTPUT_PRIVATE(r)->top[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->top[current_page], packet);
		}
	}
}

static gdouble txt_get_string_width(rlib *r UNUSED, const gchar *text UNUSED) {
	return 1;
}

static void txt_print_text(rlib *r, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, const gchar *text, gboolean backwards, struct rlib_line_extra_data *extra_data UNUSED) {
	gchar *new_text;
	rlib_encode_text(r, text, &new_text);
	print_text(r, new_text, backwards);
	g_free(new_text);
}

static void txt_start_new_page(rlib *r, struct rlib_part *part) {
	if(r->current_page_number <= 0) 
		r->current_page_number++;
	part->position_bottom[0] = 11-part->bottom_margin;
}

static void txt_init_end_page(rlib *r UNUSED) {}
static void txt_start_rlib_report(rlib *r UNUSED) {}
static void txt_end_rlib_report(rlib *r UNUSED) {}
static void txt_finalize_private(rlib *r UNUSED) {}

static void txt_spool_private(rlib *r) {
	ENVIRONMENT(r)->rlib_write_output(OUTPUT_PRIVATE(r)->both, OUTPUT_PRIVATE(r)->length);
}

static void txt_end_line(rlib *r, gboolean backwards) {
	print_text(r, "\n", backwards);
}

static void txt_start_report(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_end_report(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_start_report_field_headers(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_end_report_field_headers(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_start_report_field_details(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_end_report_field_details(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_start_report_line(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_end_report_line(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_start_report_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_end_report_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_start_report_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_end_report_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}

static void txt_start_part(rlib *r, struct rlib_part *part) {
	gint pages_across = part->pages_across;
	OUTPUT_PRIVATE(r)->pages = pages_across;
	OUTPUT_PRIVATE(r)->top = g_new0(GSList *, pages_across);
	OUTPUT_PRIVATE(r)->bottom = g_new0(GSList *, pages_across);
}

static gchar *txt_callback(struct rlib_delayed_extra_data *delayed_data) {
	struct rlib_line_extra_data *extra_data = delayed_data->extra_data;
	rlib *r = delayed_data->r;
	gchar *buf = NULL, *buf2 = NULL;

	if (rlib_pcode_has_variable(r, extra_data->field_code, NULL, NULL, FALSE))
		return NULL;

	rlib_value_free(r, &extra_data->rval_code);
	if (rlib_execute_pcode(r, &extra_data->rval_code, extra_data->field_code, NULL) == NULL)
		return NULL;
	rlib_format_string(r, &buf, extra_data->report_field, &extra_data->rval_code);
	rlib_align_text(r, &buf2, buf, extra_data->report_field->align, extra_data->report_field->width);
	g_free(buf);
	rlib_free_delayed_extra_data(r, delayed_data);
	return buf2;
}

static void txt_print_text_delayed(rlib *r, struct rlib_delayed_extra_data *delayed_data, gboolean backwards, gint rval_type UNUSED) {
	gint current_page = OUTPUT_PRIVATE(r)->page_number;
	struct _packet *packet = g_new0(struct _packet, 1);
	packet->type = DELAY;
	packet->data = delayed_data;

	if (backwards)
		OUTPUT_PRIVATE(r)->bottom[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->bottom[current_page], packet);
	else
		OUTPUT_PRIVATE(r)->top[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->top[current_page], packet);
}

static void txt_finalize_text_delayed(rlib *r, struct rlib_delayed_extra_data *delayed_data, gboolean backwards) {
	int pages, i;

	pages = OUTPUT_PRIVATE(r)->pages;
	for (i = 0; i < pages; i++) {
		GSList *list, *l;

		if (backwards)
			list = OUTPUT_PRIVATE(r)->bottom[i];
		else
			list = OUTPUT_PRIVATE(r)->top[i];

		for (l = list; l; l = l->next) {
			struct _packet *packet = l->data;
			if (packet->type == DELAY && packet->data == delayed_data) {
				gchar *text = txt_callback(packet->data);
				struct _packet *new_packet;

				if (text) {
					gchar *new_text;

					rlib_encode_text(r, text, &new_text);

					new_packet = g_new0(struct _packet, 1);
					new_packet->type = TEXT;
					new_packet->data = g_string_new(new_text);
					l->data = new_packet;

					g_free(text);
					g_free(new_text);
					g_free(packet);
				}
				return;
			}
		}
	}
}

static void txt_end_part(rlib *r, struct rlib_part *part) {
	gint i;
	gchar *old;
	for (i = 0; i < part->pages_across; i++) {
		GSList *tmp = OUTPUT_PRIVATE(r)->top[i]; 
		GSList *list = NULL;
		GSList *list0;

		while(tmp != NULL) {
			list = g_slist_prepend(list, tmp->data);
			tmp = tmp->next;
		}

		list0 = list;
		while(list != NULL) {
			struct _packet *packet = list->data;
			gchar *str;	

			if (packet->type == DELAY) {
				str = txt_callback(packet->data);
			} else {
				str = ((GString *)packet->data)->str;
			}
			if(OUTPUT_PRIVATE(r)->both  == NULL) {
				if (packet->type == TEXT)
					OUTPUT_PRIVATE(r)->both  = g_string_free(packet->data, FALSE);
				else
					OUTPUT_PRIVATE(r)->both  = str;
			} else {
				old = OUTPUT_PRIVATE(r)->both;
				OUTPUT_PRIVATE(r)->both  = g_strconcat(OUTPUT_PRIVATE(r)->both , str, NULL);
				g_free(old);
				if (packet->type == TEXT) {
					g_string_free(packet->data, TRUE);
				} else
					g_free(str);
			}
			g_free(packet);
			list = list->next;
		}
		g_slist_free(list0);

		list = NULL;
		tmp = OUTPUT_PRIVATE(r)->bottom[i]; 
		while(tmp != NULL) {
			list = g_slist_prepend(list, tmp->data);
			tmp = tmp->next;
		}

		list0 = list;
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
				if(packet->type == TEXT) {
					g_string_free(packet->data, TRUE);
				} else
					g_free(str);
			}
			g_free(packet);
			list = list->next;
		}
		g_slist_free(list0);

		g_slist_free(OUTPUT_PRIVATE(r)->bottom[i]);
		OUTPUT_PRIVATE(r)->bottom[i] = NULL;
		g_slist_free(OUTPUT_PRIVATE(r)->top[i]);
		OUTPUT_PRIVATE(r)->top[i] = NULL;		

	}
	OUTPUT_PRIVATE(r)->length = (OUTPUT_PRIVATE(r) && OUTPUT_PRIVATE(r)->both ? strlen(OUTPUT_PRIVATE(r)->both) : 0);
}

static void txt_end_page(rlib *r, struct rlib_part *part UNUSED) {
	r->current_page_number++;
	r->current_line_number = 1;
}

static void txt_free(rlib *r) {
	g_free(OUTPUT_PRIVATE(r)->top);
	g_free(OUTPUT_PRIVATE(r)->bottom);
	g_free(OUTPUT_PRIVATE(r)->both);
	g_free(OUTPUT_PRIVATE(r));
	g_free(OUTPUT(r));
}

static char *txt_get_output(rlib *r) {
	return OUTPUT_PRIVATE(r)->both;
}

static gsize txt_get_output_length(rlib *r) {
	return OUTPUT_PRIVATE(r)->length;
}

static void txt_set_working_page(rlib *r, struct rlib_part *part UNUSED, gint page) {
	OUTPUT_PRIVATE(r)->page_number = page;
}

static void txt_set_fg_color(rlib *r UNUSED, gdouble red UNUSED, gdouble green UNUSED, gdouble blue UNUSED) {}
static void txt_set_bg_color(rlib *r UNUSED, gdouble red UNUSED, gdouble green UNUSED, gdouble blue UNUSED) {}
static void txt_hr(rlib *r UNUSED, gboolean backwards UNUSED, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gdouble how_long UNUSED, gdouble how_tall UNUSED, struct rlib_rgb *color UNUSED, gdouble indent UNUSED, gdouble length UNUSED) {}
static void txt_start_draw_cell_background(rlib *r UNUSED, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gdouble how_long UNUSED, gdouble how_tall UNUSED, struct rlib_rgb *color UNUSED) {}
static void txt_end_draw_cell_background(rlib *r UNUSED) {}
static void txt_start_boxurl(rlib *r UNUSED, struct rlib_part * part UNUSED, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gdouble how_long UNUSED, gdouble how_tall UNUSED, gchar *url UNUSED, gboolean backwards UNUSED) {}
static void txt_end_boxurl(rlib *r UNUSED, gboolean backwards UNUSED) {}
static void txt_background_image(rlib *r UNUSED, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gchar *nname UNUSED, gchar *type UNUSED, gdouble nwidth UNUSED, gdouble nheight UNUSED) {}
static void txt_set_font_point(rlib *r UNUSED, gint point UNUSED) {}
static void txt_start_line(rlib *r UNUSED, gboolean backwards UNUSED) {}
static void txt_start_output_section(rlib *r UNUSED, struct rlib_report_output_array *roa UNUSED) {}
static void txt_end_output_section(rlib *r UNUSED, struct rlib_report_output_array *roa UNUSED) {}
static void txt_start_evil_csv(rlib *r UNUSED) {}
static void txt_end_evil_csv(rlib *r UNUSED) {}
static void txt_start_part_table(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_end_part_table(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_start_part_tr(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_end_part_tr(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_start_part_td(rlib *r UNUSED, struct rlib_part *part UNUSED, gdouble width UNUSED, gdouble height UNUSED) {}
static void txt_end_part_td(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_start_part_pages_across(rlib *r UNUSED, struct rlib_part *part UNUSED, gdouble left_margin UNUSED, gdouble top_margin UNUSED, gint width UNUSED, gint height UNUSED, gint border_width UNUSED, struct rlib_rgb *color UNUSED) {}
static void txt_end_part_pages_across(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_set_raw_page(rlib *r UNUSED, struct rlib_part *part UNUSED, gint page UNUSED) {}
static void txt_start_bold(rlib *r UNUSED) {}
static void txt_end_bold(rlib *r UNUSED) {}
static void txt_start_italics(rlib *r UNUSED) {}
static void txt_end_italics(rlib *r UNUSED) {}

static void txt_start_graph(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, gdouble left UNUSED, gdouble top UNUSED, gdouble width UNUSED, gdouble height UNUSED, gboolean x_axis_labels_are_under_tick UNUSED) {}
static void txt_graph_set_limits(rlib *r UNUSED, gchar side UNUSED, gdouble min UNUSED, gdouble max UNUSED, gdouble origin UNUSED) {}
static void txt_graph_set_title(rlib *r UNUSED, gchar *title UNUSED) {}
static void txt_graph_x_axis_title(rlib *r UNUSED, gchar *title UNUSED) {}
static void txt_graph_y_axis_title(rlib *r UNUSED, gchar side UNUSED, gchar *title UNUSED) {}
static void txt_graph_do_grid(rlib *r UNUSED, gboolean just_a_box UNUSED) {}
static void txt_graph_tick_x(rlib *r UNUSED) {}
static void txt_graph_set_x_iterations(rlib *r UNUSED, gint iterations UNUSED) {}
static void txt_graph_hint_label_x(rlib *r UNUSED, gchar *label UNUSED) {}
static void txt_graph_label_x(rlib *r UNUSED, gint iteration UNUSED, gchar *label UNUSED) {}
static void txt_graph_tick_y(rlib *r UNUSED, gint iterations UNUSED) {}
static void txt_graph_label_y(rlib *r UNUSED, gchar side UNUSED, gint iteration UNUSED, gchar *label UNUSED) {}
static void txt_graph_hint_label_y(rlib *r UNUSED, gchar side UNUSED, gchar *label UNUSED) {}
static void txt_graph_set_data_plot_count(rlib *r UNUSED, gint count UNUSED) {}
static void txt_graph_plot_bar(rlib *r UNUSED, gchar side UNUSED, gint iteration UNUSED, gint plot UNUSED, gdouble height_percent UNUSED, struct rlib_rgb *color UNUSED, gdouble last_height UNUSED, gboolean divide_iterations UNUSED, gdouble raw_data UNUSED, gchar *label UNUSED) {}
static void txt_graph_plot_line(rlib *r UNUSED, gchar side UNUSED, gint iteration UNUSED, gdouble p1_height UNUSED, gdouble p1_last_height UNUSED, gdouble p2_height UNUSED, gdouble p2_last_height UNUSED, struct rlib_rgb *color UNUSED, gdouble raw_data UNUSED, gchar *label UNUSED, gint row_count UNUSED) {}
static void txt_graph_plot_pie(rlib *r UNUSED, gdouble start UNUSED, gdouble end UNUSED, gboolean offset UNUSED, struct rlib_rgb *color UNUSED, gdouble raw_data UNUSED, gchar *label UNUSED) {}
static void txt_graph_hint_legend(rlib *r UNUSED, gchar *label UNUSED) {}
static void txt_graph_draw_legend(rlib *r UNUSED) {}
static void txt_graph_draw_legend_label(rlib *r UNUSED, gint iteration UNUSED, gchar *label UNUSED, struct rlib_rgb *color UNUSED, gboolean is_line UNUSED) {}
static void txt_end_graph(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_graph_draw_line(rlib *r UNUSED, gdouble x UNUSED, gdouble y UNUSED, gdouble new_x UNUSED, gdouble new_y UNUSED, struct rlib_rgb *color UNUSED) {}
static void txt_graph_set_name(rlib *r UNUSED, gchar *name UNUSED) {}
static void txt_graph_set_legend_bg_color(rlib *r UNUSED, struct rlib_rgb *rgb UNUSED) {}
static void txt_graph_set_legend_orientation(rlib *r UNUSED, gint orientation UNUSED) {}
static void txt_graph_set_draw_x_y(rlib *r UNUSED, gboolean draw_x UNUSED, gboolean draw_y UNUSED) {}
static void txt_graph_set_bold_titles(rlib *r UNUSED, gboolean bold_titles UNUSED) {}
static void txt_graph_set_grid_color(rlib *r UNUSED, struct rlib_rgb *rgb UNUSED) {}
static void txt_start_part_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_end_part_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_start_part_page_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_end_part_page_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_start_part_footer(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_end_part_footer(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void txt_start_report_page_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_end_report_page_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_start_report_break_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void txt_end_report_break_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void txt_start_report_break_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void txt_end_report_break_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void txt_start_report_no_data(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void txt_end_report_no_data(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}


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
	OUTPUT(r)->finalize_text_delayed = txt_finalize_text_delayed;
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
