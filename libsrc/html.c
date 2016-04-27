/*
 *  Copyright (C) 2003-2006 SICOM Systems, INC.
 *
 *  Authors:	Bob Doan <bdoan@sicompos.com>
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
 * This module implements the HTML output renderer for generating an HTML
 * formatted report from the rlib object.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "config.h"
#include "rlib-internal.h"
#include "rlib_gd.h"

#define TEXT 1
#define DELAY 2
#define BOTTOM_PAD 8
#define BIGGER_HTML_FONT(a) (a+2)
struct _packet {
	char type;
	gpointer data;
};

struct _graph {
	gdouble y_min;
	gdouble y_max;
	gdouble y_origin;
	gdouble non_standard_x_start;
	gint legend_width;
	gint legend_height;
	gint data_plot_count;
	gint whole_graph_width;
	gint whole_graph_height;
	gint legend_top;
	gint legend_left;
	gint width;
	gint just_a_box;
	gint height;
	gint title_height;
	gint x_label_width;
	gint y_label_width_left;
	gint y_label_width_right;
	gint x_axis_label_height;
	gint y_axis_title_left;
	gint y_axis_title_right;
	gint top;
	gint y_iterations;
	gint intersection;
	gfloat x_tick_width;
	gint x_iterations;
	gint x_start;
	gint y_start;
	gint vertical_x_label;
	gboolean x_axis_labels_are_under_tick;
	gboolean has_legend_bg_color;
	struct rlib_rgb legend_bg_color;
	gint legend_orientation;
	gboolean draw_x;
	gboolean draw_y;
	struct rlib_rgb *grid_color;
	struct rlib_rgb real_grid_color;
	gchar *name;
	gint region_count;
	gint orig_data_plot_count;
	gint current_region;
	gboolean bold_titles;
	gboolean *minor_ticks;
	gint last_left_x_label;
	gint is_chart;
};

struct _private {
	GString *whole_report;
	GSList **top;
	GSList **bottom;

	gint page_number;
	struct rlib_gd *rgd;
	struct _graph graph;
};

static void html_graph_get_x_label_width(rlib *r, gfloat *width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	*width = (gfloat)graph->x_label_width;
}

static void html_graph_set_x_label_width(rlib *r, gfloat width, gint cell_width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if (width == 0) {
		graph->x_label_width = cell_width;
	}
	else
		graph->x_label_width = (gint)width;

	if ((gint)width >= cell_width - 2)
		graph->vertical_x_label = TRUE;
}

static void html_graph_get_y_label_width(rlib *r, gfloat *width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	*width = (gfloat)graph->y_label_width_left;
}

static void html_graph_set_y_label_width(rlib *r, gfloat width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if (width == 0)
		graph->y_label_width_left = rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, "W", FALSE) * 2;
	else
		graph->y_label_width_left = (gint)width;
}

static void html_graph_get_width_offset(rlib *r, gint *width_offset) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint intersection = 5;
	*width_offset = 0;//1;
	*width_offset += graph->y_label_width_left + intersection;
}

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
		if(OUTPUT_PRIVATE(r)->top[current_page] != NULL) {
			packet = OUTPUT_PRIVATE(r)->top[current_page]->data;
		}
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

static gfloat html_get_string_width(rlib *r, const gchar *text) {
	return 1;
}

static gchar *get_html_color(gchar *str, struct rlib_rgb *color) {
	sprintf(str, "#%02x%02x%02x", (gint)(color->r*0xFF),
		(gint)(color->g*0xFF), (gint)(color->b*0xFF));
	return str;
}


static void html_print_text(rlib *r, gfloat left_origin, gfloat bottom_origin, const gchar *text, gint backwards, struct rlib_line_extra_data *extra_data) {
	GString *string = g_string_new("");

	g_string_append_printf(string, "<span data-col=\"%d\" data-width=\"%d\" style=\"font-size: %dpx; ", extra_data->col, extra_data->width, BIGGER_HTML_FONT(extra_data->font_point));

	if(extra_data->found_bgcolor) 
		g_string_append_printf(string, "background-color: #%02x%02x%02x; ", (gint)(extra_data->bgcolor.r*0xFF), (gint)(extra_data->bgcolor.g*0xFF), (gint)(extra_data->bgcolor.b*0xFF));
	if(extra_data->found_color) 
		g_string_append_printf(string, "color:#%02x%02x%02x ", (gint)(extra_data->color.r*0xFF), (gint)(extra_data->color.g*0xFF), (gint)(extra_data->color.b*0xFF));
	if(extra_data->is_bold == TRUE)
		g_string_append(string, "font-weight: bold;");
	if(extra_data->is_italics == TRUE)
		g_string_append(string, "font-style: italic;");

		
	g_string_append(string,"\">");
	gchar *escaped = g_markup_escape_text(text, strlen(text));
	g_string_append(string, escaped);
	g_string_append(string, "</span>");
	g_free(escaped);

	print_text(r, string->str, backwards);
	g_string_free(string, TRUE);
}


static void html_set_fg_color(rlib *r, gfloat red, gfloat green, gfloat blue) {}

static void html_set_bg_color(rlib *r, gfloat red, gfloat green, gfloat blue) {}

static void html_start_draw_cell_background(rlib *r, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall,
struct rlib_rgb *color) {
	OUTPUT(r)->set_bg_color(r, color->r, color->g, color->b);
}

static void html_end_draw_cell_background(rlib *r) {
}

static void html_start_boxurl(rlib *r, struct rlib_part *part, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall, gchar *url, gint backwards) {
	gchar buf[MAXSTRLEN];
	sprintf(buf, "<a href=\"%s\">", url);
	print_text(r, buf, backwards);
}

static void html_end_boxurl(rlib *r, gint backwards) {
	print_text(r, "</a>", backwards);
}



static void html_hr(rlib *r, gint backwards, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall,
struct rlib_rgb *color, gfloat indent, gfloat length) {
	gchar buf[MAXSTRLEN];
	gchar nbsp[MAXSTRLEN];
	gchar color_str[40];
	gchar td[MAXSTRLEN];
	int i;
	get_html_color(color_str, color);


	if( how_tall > 0 ) {
		nbsp[0] = 0;
		td[0] = 0;
		if(indent > 0) {
			for(i=0;i<(int)indent;i++)
				strcpy(nbsp + (i*6), "&nbsp;");
			sprintf(td, "<td style=\"height:%dpx; line-height:%dpx;\">%s</td>", (int)how_tall, (int)how_tall, nbsp);
		}

		print_text(r, "<table cellspacing=\"0\" cellpadding=\"0\" style=\"width:100%;\"><tr>", backwards);
		sprintf(buf,"%s<td style=\"height:%dpx; background-color:%s; width:100%%\"></td>",  td, (int)how_tall,color_str);
		print_text(r, buf, backwards);
		print_text(r, "</tr></table>\n", backwards);

	}
}

static void html_background_image(rlib *r, gfloat left_origin, gfloat bottom_origin, gchar *nname, gchar *type, gfloat nwidth,
gfloat nheight) {
	gchar buf[MAXSTRLEN];

	print_text(r, "<span style=\"float: left; position:absolute;\">", FALSE);
	sprintf(buf, "<img src=\"%s\" width=\"%f\" height=\"%f\" alt=\"background image\"/>", nname, nwidth, nheight);
	print_text(r, buf, FALSE);
	print_text(r, "</span>", FALSE);
}

static void html_line_image(rlib *r, gfloat left_origin, gfloat bottom_origin, gchar *nname, gchar *type, gfloat nwidth,
gfloat nheight) {
	gchar buf[MAXSTRLEN];

	sprintf(buf, "<img src=\"%s\" alt=\"line image\"/>", nname);
	print_text(r, buf, FALSE);
}

static void html_set_font_point(rlib *r, gint point) {
	if(r->current_font_point != point) {
		r->current_font_point = point;
	}
}

static void html_start_new_page(rlib *r, struct rlib_part *part) {
	if(r->current_page_number <= 0)
		r->current_page_number++;
	part->position_bottom[0] = 11-part->bottom_margin;
}

static gchar *html_callback(struct rlib_delayed_extra_data *delayed_data) {
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

static void html_print_text_delayed(rlib *r, struct rlib_delayed_extra_data *delayed_data, int backwards, int rval_type) {
	gint current_page = OUTPUT_PRIVATE(r)->page_number;
	struct _packet *packet = g_new0(struct _packet, 1);
	packet->type = DELAY;
	packet->data = delayed_data;

	if(backwards)
		OUTPUT_PRIVATE(r)->bottom[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->bottom[current_page], packet);
	else
		OUTPUT_PRIVATE(r)->top[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->top[current_page], packet);
}


static void html_start_report(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_end_report(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_start_report_field_headers(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_end_report_field_headers(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_start_report_field_details(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_end_report_field_details(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_start_report_header(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_end_report_header(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_start_report_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_end_report_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}


static void html_start_rlib_report(rlib *r) {
	gchar *meta;
	gchar *link;
	gchar *suppress_head;

	OUTPUT_PRIVATE(r)->whole_report = g_string_new("");

	link = g_hash_table_lookup(r->output_parameters, "trim_links");
	if(link != NULL)
		OUTPUT(r)->trim_links = TRUE;

   	suppress_head = g_hash_table_lookup(r->output_parameters, "suppress_head");
   	if(suppress_head == NULL) {
		char *doctype = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n <html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n";
		int font_size = r->font_point;
		if(font_size <= 0)
			font_size = RLIB_DEFUALT_FONTPOINT;
		
		g_string_append(OUTPUT_PRIVATE(r)->whole_report, doctype);
		g_string_append_printf(OUTPUT_PRIVATE(r)->whole_report, "<head>\n<style type=\"text/css\">\n");


		g_string_append_printf(OUTPUT_PRIVATE(r)->whole_report, "pre { margin:0; padding:0; margin-top:0; margin-bottom:0; font-size:%dpt;}\n", BIGGER_HTML_FONT(font_size));
		g_string_append_printf(OUTPUT_PRIVATE(r)->whole_report, "body { background-color: #ffffff;}\n");
		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "TABLE { border: 0; border-spacing: 0; padding: 0; width:100%; }\n");
		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</style>\n");
		meta = g_hash_table_lookup(r->output_parameters, "meta");
		if(meta != NULL)
			g_string_append(OUTPUT_PRIVATE(r)->whole_report, meta);
		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "<title>RLIB Report</title></head>\n");
		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "<body>\n");

   	}
}


static void html_start_part(rlib *r, struct rlib_part *part) {
	if(OUTPUT_PRIVATE(r)->top == NULL) {
		OUTPUT_PRIVATE(r)->top = g_new0(GSList *, part->pages_across);
		OUTPUT_PRIVATE(r)->bottom = g_new0(GSList *, part->pages_across);
	}
}

static void html_end_part(rlib *r, struct rlib_part *part) {
	gint i;

	//g_string_append(OUTPUT_PRIVATE(r)->whole_report, "<table><!-- START PART-->\n");

	for(i=0;i<part->pages_across;i++) {
		GSList *tmp = OUTPUT_PRIVATE(r)->top[i];
		GSList *list = NULL;

//		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "<!--START PAGE ACROSS--><td valign=\"top\">\n");

		while(tmp != NULL) {
			list = g_slist_prepend(list, tmp->data);
			tmp = tmp->next;
		}

		while(list != NULL) {
			struct _packet *packet = list->data;
			gchar *str;
			if(packet->type == DELAY) {
				str = html_callback(packet->data);
			} else {
				str = ((GString *)packet->data)->str;
			}
			
			g_string_append(OUTPUT_PRIVATE(r)->whole_report, str);
			
			if(packet->type == TEXT)
				g_string_free(packet->data, TRUE);
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
				str = html_callback(packet->data);
			} else {
				str = ((GString *)packet->data)->str;
			}
			
			g_string_append(OUTPUT_PRIVATE(r)->whole_report, str);
			
			if(packet->type == TEXT)
				g_string_free(packet->data, TRUE);

			g_free(packet);
			list = list->next;
		}

		g_slist_free(list);
		list = NULL;

		OUTPUT_PRIVATE(r)->top[i] = NULL;
		OUTPUT_PRIVATE(r)->bottom[i] = NULL;

//		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</td><!--END PAGE ACROSS-->\n\n");


	}

	//g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</table><!-- END PART-->");

}

static void html_spool_private(rlib *r) {
	ENVIRONMENT(r)->rlib_write_output(OUTPUT_PRIVATE(r)->whole_report->str, OUTPUT_PRIVATE(r)->whole_report->len);
}


static void html_end_page(rlib *r, struct rlib_part *part) {
	r->current_line_number = 1;
}

static char *html_get_output(rlib *r) {
	return OUTPUT_PRIVATE(r)->whole_report->str;
}

static long html_get_output_length(rlib *r) {
	return OUTPUT_PRIVATE(r)->whole_report->len;
}

static void html_set_working_page(rlib *r, struct rlib_part *part, gint page) {
	OUTPUT_PRIVATE(r)->page_number = page;
}

static void html_start_part_table(rlib *r, struct rlib_part *part) {
	print_text(r, "<table><!--start from part table-->", FALSE);
}

static void html_end_part_table(rlib *r, struct rlib_part *part) {
	print_text(r, "</table><!--ended from part table-->", FALSE);
}


static void html_start_part_tr(rlib *r, struct rlib_part *part) {
	print_text(r, "<tr><!--start from part tr-->", FALSE);
}

static void html_end_part_tr(rlib *r, struct rlib_part *part) {
	print_text(r, "</tr><!--ended from part tr-->", FALSE);
}

static void html_start_part_td(rlib *r, struct rlib_part *part, gfloat width, gfloat height) {
	print_text(r, "<td><!--started from part td-->", FALSE);
}

static void html_end_part_td(rlib *r, struct rlib_part *part) {
	print_text(r, "</td><!--ended from part td-->", FALSE);
}

static void html_start_report_line(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	
	
}

static void html_end_report_line(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	
}


static void html_start_line(rlib *r, int backwards) {
	print_text(r, "<div><pre style=\"font-size: 1pt\">",  backwards);
}

static void html_end_line(rlib *r, int backwards) {
	print_text(r, "</pre></div>\n", backwards);	
}

static void html_start_part_pages_across(rlib *r, struct rlib_part *part, gfloat left_margin, gfloat top_margin, int width, int height, int border_width, struct rlib_rgb *color) {
	print_text(r, "<!--start pages across-->", FALSE);

}


//SOMETHING NEEDS TO HAPPEN HERE.. probably
static void html_end_part_pages_across(rlib *r, struct rlib_part *part) {
	print_text(r, "<!--end pages across-->", FALSE);
}


static void html_start_bold(rlib *r) {}
static void html_end_bold(rlib *r) {}
static void html_start_italics(rlib *r) {}
static void html_end_italics(rlib *r) {}

static void html_graph_draw_line(rlib *r, gfloat x, gfloat y, gfloat new_x, gfloat new_y, struct rlib_rgb *color) {}

static void html_graph_init(rlib *r) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	memset(graph, 0, sizeof(struct _graph));
}

static void html_graph_get_chart_layout(rlib *r, gfloat top, gfloat bottom, gint cell_height, gint rows, gint *chart_size, gint *chart_height) {
	// don't do anything with chart_size
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint height_offset = 1;
	gint intersection = 5;
	gint title_height = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, graph->bold_titles);

	height_offset += (BOTTOM_PAD + title_height + intersection + graph->x_axis_label_height);

	if (graph->vertical_x_label)
		height_offset += graph->x_label_width + rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE) + BOTTOM_PAD;
	else {
		if (graph->x_label_width != 0) {
			height_offset += rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE) + BOTTOM_PAD;
		}
	}

	*chart_height = height_offset + rows * cell_height;
}

static void html_start_graph(rlib *r, struct rlib_part *part, struct rlib_report *report, gfloat left, gfloat top, gfloat width, gfloat height, gboolean x_axis_labels_are_under_tick) {
	char buf[MAXSTRLEN];
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;

	memset(graph, 0, sizeof(struct _graph));

	OUTPUT_PRIVATE(r)->rgd = rlib_gd_new(width, height,  g_hash_table_lookup(r->output_parameters, "html_image_directory"));

	sprintf(buf, "<img src=\"%s\" width=\"%f\" height=\"%f\" alt=\"graph\"/>", OUTPUT_PRIVATE(r)->rgd->file_name, width, height);
	print_text(r, buf, FALSE);
	graph->width = (gint)width - 1;
	graph->height = (gint)height - 1;
	graph->whole_graph_width = (gint)width;
	graph->whole_graph_height = (gint)height;
	graph->intersection = 5;
	graph->x_axis_labels_are_under_tick = x_axis_labels_are_under_tick;
}

static void html_graph_set_limits(rlib *r, gchar side, gdouble min, gdouble max, gdouble origin) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->y_min = min;
	graph->y_max = max;
	graph->y_origin = origin;
}

static void html_graph_set_title(rlib *r, gchar *title) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat title_width = rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, title, graph->bold_titles);
	graph->title_height = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, graph->bold_titles);
	rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, title, ((graph->width-title_width)/2.0), 0, FALSE, graph->bold_titles);
}

static void html_graph_set_name(rlib *r, gchar *name) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->name = name;
}

static void html_graph_set_legend_bg_color(rlib *r, struct rlib_rgb *rgb) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->legend_bg_color = *rgb;
	graph->has_legend_bg_color = TRUE;
}

static void html_graph_set_legend_orientation(rlib *r, gint orientation) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->legend_orientation = orientation;
}

static void html_graph_set_grid_color(rlib *r, struct rlib_rgb *rgb) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->real_grid_color = *rgb;
	graph->grid_color = &graph->real_grid_color;
}

static void html_graph_set_draw_x_y(rlib *r, gboolean draw_x, gboolean draw_y) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->draw_x = draw_x;
	graph->draw_y = draw_y;
}

static void html_graph_set_is_chart(rlib *r, gint is_chart) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->is_chart = is_chart;
}

static void html_graph_set_bold_titles(rlib *r, gboolean bold_titles) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->bold_titles = bold_titles;
}

static void html_graph_set_minor_ticks(rlib *r, gboolean *minor_ticks) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->minor_ticks = minor_ticks;
}

static void html_graph_x_axis_title(rlib *r, gchar *title) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;

	graph->x_axis_label_height = graph->legend_height;

	if(title[0] == 0)
		graph->x_axis_label_height += 0;
	else {
		gfloat title_width = rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, title, graph->bold_titles);
		graph->x_axis_label_height += rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, graph->bold_titles);
		rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, title,  (graph->width-title_width)/2.0, graph->whole_graph_height-graph->x_axis_label_height, FALSE, graph->bold_titles);
		graph->x_axis_label_height += rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, graph->bold_titles) * 0.25;

	}
}

static void html_graph_y_axis_title(rlib *r, gchar side, gchar *title) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat title_width = rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, title, graph->bold_titles);
	if(title[0] == 0) {

	} else {
		if(side == RLIB_SIDE_LEFT) {
			rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, title,  0, (graph->whole_graph_height-graph->legend_height)  -((graph->whole_graph_height - graph->legend_height - title_width)/2.0), TRUE, graph->bold_titles);
			graph->y_axis_title_left = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, graph->bold_titles);
		} else {
			graph->y_axis_title_right = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, graph->bold_titles);
			rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, title, graph->whole_graph_width-graph->legend_width-graph->y_axis_title_right,  (graph->whole_graph_height-graph->legend_height)-((graph->whole_graph_height - graph->legend_height - title_width)/2.0), TRUE, graph->bold_titles);
		}
	}
}

static void html_draw_regions(gpointer data, gpointer user_data) {
	struct rlib_graph_region *gr = data;
	rlib *r = user_data;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if(graph->name != NULL) {
		if(strcmp(graph->name, gr->graph_name) == 0) {
			rlib_gd_rectangle(OUTPUT_PRIVATE(r)->rgd, graph->x_start + (graph->width*(gr->start/100.0)), graph->y_start, graph->width*((gr->end-gr->start)/100.0), graph->height, &gr->color);
		}
	}
}

static void html_graph_label_x_get_variables(rlib *r, gint iteration, gchar *label, gint *left, gint *y_start, gint string_width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	*left = graph->x_start + (graph->x_tick_width * iteration);
	*y_start = graph->y_start;

	if(string_width == 0)
		string_width = rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, label, FALSE);

	if(graph->vertical_x_label) {
		*y_start += graph->x_label_width;
	} else {
		*left += (graph->x_tick_width - string_width) / 2;
		*y_start += (BOTTOM_PAD/2);
	}

	if (graph->is_chart)
		if (graph->vertical_x_label)
			*left += rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE) / 2;

	if(graph->x_axis_labels_are_under_tick) {
		if(graph->vertical_x_label) {
			*left -= rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE) / 2;
		} else {
			*left -= (graph->x_tick_width / 2);
		}
		*y_start += graph->intersection;
	}

}

static void html_graph_set_x_tick_width(rlib *r) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if(graph->x_axis_labels_are_under_tick)	 {
		if(graph->x_iterations <= 1)
			graph->x_tick_width = 0;
		else
			graph->x_tick_width = (gfloat)graph->width/((gfloat)graph->x_iterations-1.0);
	}
	else
		graph->x_tick_width = (gfloat)graph->width/(gfloat)graph->x_iterations;
}

static void html_graph_do_grid(rlib *r, gboolean just_a_box) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint i;
	gint last_left = 0 - graph->x_label_width;

	graph->width -= (graph->y_axis_title_left + + graph->y_axis_title_right + graph->y_label_width_left + graph->intersection);
	if(graph->y_label_width_right > 0)
		graph->width -= (graph->y_label_width_right + graph->intersection);

	graph->top = graph->title_height*1.1;
	graph->height -= (BOTTOM_PAD + graph->title_height + graph->intersection + graph->x_axis_label_height);

	if(graph->x_iterations != 0) {
		if(graph->x_axis_labels_are_under_tick) {
			if(graph->x_iterations <= 1)
				graph->x_tick_width = 0;
			else
				graph->x_tick_width = (gfloat)graph->width/((gfloat)graph->x_iterations-1.0);
		} else
			graph->x_tick_width = (gfloat)graph->width/(gfloat)graph->x_iterations;
	}

	graph->x_start = graph->y_axis_title_left + graph->intersection;
	graph->y_start = graph->whole_graph_height - graph->x_axis_label_height;


	graph->x_start += graph->y_label_width_left;

	for(i=0;i<graph->x_iterations;i++) {
		gint left,y_start,string_width=graph->x_label_width;
		if(graph->minor_ticks[i] == FALSE) {
			html_graph_label_x_get_variables(r, i, NULL, &left, &y_start, string_width);
			if(left < (last_left+graph->x_label_width+rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, "W", FALSE))) {
				if (!graph->is_chart)
					graph->vertical_x_label = TRUE;
				break;
			}
			if(left + graph->x_label_width > graph->whole_graph_width) {
				if (!graph->is_chart)
					graph->vertical_x_label = TRUE;
				break;
			}

			last_left = left;
		}
	}

	if (graph->is_chart) {
		if (graph->vertical_x_label) {
			graph->height -= graph->x_label_width + rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE)+BOTTOM_PAD;
			graph->y_start -= rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE)+BOTTOM_PAD;
		}
		else {
			if (graph->x_label_width != 0) {
				graph->y_start -= rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE)+BOTTOM_PAD;
				graph->height -= rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE)+BOTTOM_PAD;
			}
		}
	}
	else
	if(graph->vertical_x_label) {
		graph->vertical_x_label = TRUE;
		graph->y_start -= graph->x_label_width;
		graph->height -= graph->x_label_width;
		graph->y_start -= rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, "w", FALSE);
	} else {
		graph->vertical_x_label = FALSE;
		if(graph->x_label_width != 0) {
			graph->y_start -= rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE)+BOTTOM_PAD;
			graph->height -= rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE)+BOTTOM_PAD;
		}
	}

	graph->just_a_box = just_a_box;

	if(!just_a_box) {
		rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, graph->x_start, graph->y_start, graph->x_start + graph->width, graph->y_start, graph->grid_color);
	}

}

static void html_graph_tick_x(rlib *r) {
	gint i;
	gint spot;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint divisor = 1;
	gint iterations = graph->x_iterations;


	if(!graph->x_axis_labels_are_under_tick)
		iterations++;

	for(i=0;i<iterations;i++) {

		if(graph->minor_ticks[i] == TRUE)
			divisor = 2;
		else
			divisor = 1;

		spot = graph->x_start + ((graph->x_tick_width)*i);
		if(graph->draw_x && (graph->minor_ticks[i] == FALSE || i == iterations-1)) {
			if (graph->is_chart)
				rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, spot, graph->y_start, spot, graph->y_start - (graph->intersection/divisor) - graph->height, graph->grid_color);
			else
				rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, spot, graph->y_start+(graph->intersection/divisor), spot, graph->y_start - graph->height, graph->grid_color);
		}
		else
			rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, spot, graph->y_start+(graph->intersection/divisor), spot, graph->y_start, graph->grid_color);
	}

}

static void html_graph_set_x_iterations(rlib *r, gint iterations) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->x_iterations = iterations;
}

static void html_graph_hint_label_x(rlib *r, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint string_width = rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, label, FALSE);

	if(string_width > graph->x_label_width) {
		graph->x_label_width = string_width;
	}
}

static void html_graph_label_x(rlib *r, gint iteration, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint left, y_start;
	gboolean doit = TRUE;
	gint height = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE);

	html_graph_label_x_get_variables(r, iteration, label, &left, &y_start, 0);


	if(graph->vertical_x_label) {
		if(graph->last_left_x_label + height > left)
			doit = FALSE;
		else
			graph->last_left_x_label = left;
	}


	if(doit) {
		if (graph->is_chart) {
			gint text_height;
			if (graph->vertical_x_label) {
				text_height = rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, "w", FALSE);// graph->x_label_width;
				rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label, left, graph->y_start - (graph->height + text_height), graph->vertical_x_label, FALSE);
				//rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label, left, graph->y_start - (graph->height + text_height), graph->vertical_x_label, FALSE);
			}
			else {
				text_height = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE);// / 3.0;
				rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label, left, graph->y_start - (graph->height + text_height), graph->vertical_x_label, FALSE);
			}
			//rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label, left, graph->top + text_height, graph->vertical_x_label, FALSE);
		}
		else
			rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label, left, y_start, graph->vertical_x_label, FALSE);
	}
}

static void html_graph_tick_y(rlib *r, gint iterations) {
	gint i;
	gint y = 0;
	gint extra_width = 0;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->y_iterations = iterations;

	if(graph->y_label_width_right > 0)
		extra_width = graph->intersection;

	if (!graph->is_chart) {
		// this seems like a mistake even for regular graphing since height should already be set
		graph->height = (graph->height/iterations)*iterations;
	}

	g_slist_foreach(r->graph_regions, html_draw_regions, r);

	html_graph_tick_x(r);

	for(i=0;i<iterations+1;i++) {
		y = graph->y_start - ((graph->height/iterations) * i);
		if(graph->draw_y)
			rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, graph->x_start-graph->intersection, y, graph->x_start+graph->width+extra_width, y, graph->grid_color);
		else {
			rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, graph->x_start-graph->intersection, y, graph->x_start, y, graph->grid_color);
			rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, graph->x_start+graph->width, y, graph->x_start+graph->width+extra_width, y, graph->grid_color);
		}
	}
	rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, graph->x_start, graph->y_start, graph->x_start, graph->y_start - graph->height, graph->grid_color);

	if(graph->y_label_width_right > 0) {
		gint xstart;

		if(graph->x_axis_labels_are_under_tick)
			xstart = graph->x_start + ((graph->x_tick_width)*(graph->x_iterations-1));
		else
			xstart = graph->x_start + ((graph->x_tick_width)*(graph->x_iterations));

		rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, xstart, graph->y_start,xstart, graph->y_start - graph->height, graph->grid_color);
	}

}

static void html_graph_label_y(rlib *r, gchar side, gint iteration, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat white_space = graph->height/graph->y_iterations;
	gfloat line_width = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE) / 3.0;
	gfloat top;

	if (graph->is_chart) {
		gfloat l = white_space / 2 + (line_width / 1.5);
		top = graph->y_start - ((white_space * (iteration - 1)) + l);
	}
	else
		top = graph->y_start - (white_space * iteration) - line_width;

	if(side == RLIB_SIDE_LEFT)
		rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label,  1+graph->y_axis_title_left, top, FALSE, FALSE);
	else
		rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label,  1+graph->y_axis_title_left+graph->width+(graph->intersection*2)+graph->y_label_width_left, top, FALSE, FALSE);
}

static void html_graph_hint_label_y(rlib *r, gchar side, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat width =  rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, label, FALSE);
	if(side == RLIB_SIDE_LEFT) {
		if(width > graph->y_label_width_left)
			graph->y_label_width_left = width;
	} else {
		if(width > graph->y_label_width_right)
			graph->y_label_width_right = width;
	}

}

static void html_graph_set_data_plot_count(rlib *r, gint count) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->data_plot_count = count;
}

static void html_graph_draw_bar(rlib *r, gint row, gint start_iteration, gint end_iteration, struct rlib_rgb *color, char *label, struct rlib_rgb *label_color, gint width_pad, gint height_pad) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat bar_length = (end_iteration - start_iteration + 1) * graph->x_tick_width;
	gfloat bar_height = graph->height / graph->y_iterations;
	gfloat x_adjust = width_pad / graph->x_tick_width;
	gfloat y_adjust = height_pad / bar_height;
	gfloat left = graph->x_start + (graph->x_tick_width * (start_iteration - 1)) + (graph->x_tick_width * x_adjust);
	gfloat start = graph->y_start - ((graph->y_iterations - row) * bar_height + bar_height * y_adjust);
	gfloat label_width =  rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, label, FALSE);
	gfloat line_width = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE) / 3.0;
	gchar label_text[MAXSTRLEN];
	gint i;

	strcpy(label_text, label);
	i = strlen(label_text);

	if (width_pad == 0) {
		if (start_iteration == 1)
			bar_length -= 1;
		left += 1;
		bar_length -= 1;
	} else
		bar_length -= (graph->x_tick_width * x_adjust * 2);

	if (height_pad == 0) {
		start -= 1;
		bar_height -= 2;
	} else
		bar_height -= (bar_height * y_adjust * 2);

	//OUTPUT(r)->set_bg_color(r, color->r, color->g, color->b);
	rlib_gd_rectangle(OUTPUT_PRIVATE(r)->rgd, left, start, bar_length, bar_height, color);

	while (i > 0 && label_width > bar_length) {
		label_text[--i] = '\0';
		label_width =  rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, label_text, FALSE);
	}

	if (label_width > 0) {
		gfloat text_left = left + bar_length / 2 - label_width / 2;
		gfloat text_top = start - (bar_height / 2 + line_width * 3 / 2);
		//OUTPUT(r)->set_fg_color(r, label_color->r, label_color->g, label_color->b);
		rlib_gd_color_text(OUTPUT_PRIVATE(r)->rgd, label_text,  text_left, text_top, FALSE, FALSE, label_color);
	}

	//OUTPUT(r)->set_bg_color(r, 0, 0, 0);
}

static void html_graph_plot_bar(rlib *r, gchar side, gint iteration, gint plot, gfloat height_percent, struct rlib_rgb *color, gfloat last_height, gboolean divide_iterations, gfloat raw_data, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat bar_width = graph->x_tick_width *.6;
	gfloat left = graph->x_start + (graph->x_tick_width * iteration) + (graph->x_tick_width *.2);
	gfloat start = graph->y_start;

	if(graph->y_origin != graph->y_min)  {
		gfloat n = fabs(graph->y_max)+fabs(graph->y_origin);
		gfloat d = fabs(graph->y_min)+fabs(graph->y_max);
		gfloat real_height =  1 - (n / d);
		start -= (real_height * graph->height);
	}

	if(divide_iterations)
		bar_width /= graph->data_plot_count;

	left += (bar_width)*plot;

	rlib_gd_rectangle(OUTPUT_PRIVATE(r)->rgd, left, start - last_height*graph->height, bar_width, graph->height*(height_percent), color);

}

static void html_graph_plot_line(rlib *r, gchar side, gint iteration, gfloat p1_height, gfloat p1_last_height, gfloat p2_height, gfloat p2_last_height, struct rlib_rgb * color, gfloat raw_data, gchar *label, gint row_count) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat p1_start = graph->y_start;
	gfloat p2_start = graph->y_start;
	gfloat left = graph->x_start + (graph->x_tick_width * (iteration-1));
	if(row_count <= 0) 
		return;
	p1_height += p1_last_height;
	p2_height += p2_last_height;

	if(graph->y_origin != graph->y_min)  {
		gfloat n = fabs(graph->y_max)+fabs(graph->y_origin);
		gfloat d = fabs(graph->y_min)+fabs(graph->y_max);
		gfloat real_height =  1 - (n / d);
		p1_start -= (real_height * graph->height);
		p2_start -= (real_height * graph->height);
	}
	if(graph->x_iterations < 90)
		rlib_gd_set_thickness(OUTPUT_PRIVATE(r)->rgd, 2);
	else
		rlib_gd_set_thickness(OUTPUT_PRIVATE(r)->rgd, 1);
	rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left, p1_start - (graph->height * p1_height), left+graph->x_tick_width, p2_start - (graph->height * p2_height), color);
	rlib_gd_set_thickness(OUTPUT_PRIVATE(r)->rgd, 1);
}

static void html_graph_plot_pie(rlib *r, gfloat start, gfloat end, gboolean offset, struct rlib_rgb *color, gfloat raw_data, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat start_angle = 360.0 * start;
	gfloat end_angle = 360.0 * end;
	gfloat x = (graph->width / 2);
	gfloat y = graph->top + ((graph->height-graph->legend_height) / 2);
	gfloat radius = 0;
	gfloat offset_factor = 0;
	gfloat rads;

	start_angle += 90;
	end_angle += 90;

	if(graph->width < (graph->height-graph->legend_height))
		radius = graph->width / 2.2;
	else
		radius = (graph->height-graph->legend_height) / 2.2;

	if(offset)
		offset_factor = radius * .1;

	rads = (((start_angle+end_angle)/2.0))*3.1415927/180.0;
	x += offset_factor * cosf(rads);
	y += offset_factor * sinf(rads);
	radius -= offset_factor;

	rlib_gd_arc(OUTPUT_PRIVATE(r)->rgd, x, y, radius, start_angle, end_angle, color);

}

static void html_graph_hint_legend(rlib *r, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat width =  rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, label, FALSE) + rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, "WWW", FALSE);

	if(width > graph->legend_width)
		graph->legend_width = width;
}

static void html_count_regions(gpointer data, gpointer user_data) {
	struct rlib_graph_region *gr = data;
	rlib *r = user_data;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if(graph->name != NULL) {
		if(strcmp(graph->name, gr->graph_name) == 0) {
			gfloat width =  rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, gr->region_label, FALSE) + rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, "WWW", FALSE);

			if(width > graph->legend_width)
				graph->legend_width = width;

			graph->region_count++;
		}
	}
}

static void html_label_regions(gpointer data, gpointer user_data) {
	struct rlib_graph_region *gr = data;
	rlib *r = user_data;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if(graph->name != NULL) {
		if(strcmp(graph->name, gr->graph_name) == 0) {
			gint iteration = graph->orig_data_plot_count + graph->current_region;
			gfloat offset =  (iteration  * rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE));
			gfloat picoffset = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE);
			gfloat textoffset = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE)/4;
			gfloat w_width = rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, "W", FALSE);
			gfloat line_height = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE);
			gfloat left = graph->legend_left + (w_width/2);
			gfloat top = graph->legend_top + offset + picoffset;
			gfloat bottom = top - (line_height*.6);

			rlib_gd_rectangle(OUTPUT_PRIVATE(r)->rgd, graph->legend_left + (w_width/2), graph->legend_top + offset + picoffset , w_width, line_height*.6, &gr->color);
			rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left, bottom, left, top, NULL);
			rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left+w_width, bottom, left+w_width, top, NULL);
			rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left, bottom, left+w_width, bottom, NULL);
			rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left, top, left+w_width, top, NULL);
			rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, gr->region_label,  graph->legend_left + (w_width*2), graph->legend_top + offset + textoffset, FALSE, FALSE);
			graph->current_region++;
		}
	}
}


static void html_graph_draw_legend(rlib *r) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint left, width, height, top, bottom;

	g_slist_foreach(r->graph_regions, html_count_regions, r);
	graph->orig_data_plot_count = graph->data_plot_count;
	graph->data_plot_count += graph->region_count;

	if(graph->legend_orientation == RLIB_GRAPH_LEGEND_ORIENTATION_RIGHT) {
		left = graph->whole_graph_width - graph->legend_width;
		width = graph->legend_width-1;
		height = (rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE) * graph->data_plot_count) + (rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE) / 2);
		top = 0;
		bottom = height;
	} else {
		left = 0;
		width = graph->width;
		height = (rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE) * graph->data_plot_count) + (rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE) / 2);
		top = graph->height - height;
		bottom = graph->height;
	}

	if(graph->has_legend_bg_color) {
		OUTPUT(r)->set_bg_color(r, graph->legend_bg_color.r, graph->legend_bg_color.g, graph->legend_bg_color.b);
		rlib_gd_rectangle(OUTPUT_PRIVATE(r)->rgd, left, bottom, width, bottom-top, &graph->legend_bg_color);
	}


	rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left, bottom, left, top, NULL);
	rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left+width, bottom, left+width, top, NULL);
	rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left, bottom, left+width, bottom, NULL);
	rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left, top, left+width, top, NULL);

	graph->legend_top = top;
	graph->legend_left = left;

	if(graph->legend_orientation == RLIB_GRAPH_LEGEND_ORIENTATION_RIGHT)
		graph->width -= width;
	else {
		graph->legend_width = 0;
		graph->legend_height = height;
	}

	g_slist_foreach(r->graph_regions, html_label_regions, r);

}

static void html_graph_draw_legend_label(rlib *r, gint iteration, gchar *label, struct rlib_rgb *color, gboolean is_line) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat offset =  (iteration  * rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE));
	gfloat picoffset = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE);
	gfloat textoffset = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE)/4;
	gfloat w_width = rlib_gd_get_string_width(OUTPUT_PRIVATE(r)->rgd, "W", FALSE);
	gfloat line_height = rlib_gd_get_string_height(OUTPUT_PRIVATE(r)->rgd, FALSE);
	gfloat left = graph->legend_left + (w_width/2);
	gfloat top = graph->legend_top + offset + picoffset;
	gfloat bottom = top - (line_height*.6);

	if(!is_line) {
		rlib_gd_rectangle(OUTPUT_PRIVATE(r)->rgd, graph->legend_left + (w_width/2), graph->legend_top + offset + picoffset , w_width, line_height*.6, color);
		rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left, bottom, left, top, NULL);
		rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left+w_width, bottom, left+w_width, top, NULL);
		rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left, bottom, left+w_width, bottom, NULL);
		rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, left, top, left+w_width, top, NULL);
	} else {
		rlib_gd_rectangle(OUTPUT_PRIVATE(r)->rgd,  graph->legend_left + (w_width/2), graph->legend_top + offset + textoffset + (line_height/2) , w_width, line_height*.2, color);
	}
	rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label,  graph->legend_left + (w_width*2), graph->legend_top + offset + textoffset, FALSE, FALSE);
}

static void html_end_graph(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	rlib_gd_spool(r, OUTPUT_PRIVATE(r)->rgd);
	rlib_gd_free(OUTPUT_PRIVATE(r)->rgd);
}

static void html_init_end_page(rlib *r) {}
static void html_end_rlib_report(rlib *r) {}

static void html_finalize_private(rlib *r) {
	g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</body></html>");
}

static void html_start_output_section(rlib *r, struct rlib_report_output_array *roa) {}
static void html_end_output_section(rlib *r, struct rlib_report_output_array *roa) {}
static void html_start_evil_csv(rlib *r) {}
static void html_end_evil_csv(rlib *r) {}
static void html_set_raw_page(rlib *r, struct rlib_part *part, gint page) {}
static void html_start_part_header(rlib *r, struct rlib_part *part) {}
static void html_end_part_header(rlib *r, struct rlib_part *part) {}
static void html_start_part_page_header(rlib *r, struct rlib_part *part) {}
static void html_end_part_page_header(rlib *r, struct rlib_part *part) {}
static void html_start_part_page_footer(rlib *r, struct rlib_part *part) {}
static void html_end_part_page_footer(rlib *r, struct rlib_part *part) {}
static void html_start_report_page_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_end_report_page_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_start_report_break_header(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void html_end_report_break_header(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void html_start_report_break_footer(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void html_end_report_break_footer(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void html_start_report_no_data(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void html_end_report_no_data(rlib *r, struct rlib_part *part, struct rlib_report *report) {}

static gint html_free(rlib *r) {
	g_string_free(OUTPUT_PRIVATE(r)->whole_report, TRUE);
	g_free(OUTPUT_PRIVATE(r));
	g_free(OUTPUT(r));
	return 0;
}

void rlib_html_new_output_filter(rlib *r) {
	OUTPUT(r) = g_malloc(sizeof(struct output_filter));
	r->o->private = g_malloc(sizeof(struct _private));
	memset(OUTPUT_PRIVATE(r), 0, sizeof(struct _private));

	OUTPUT_PRIVATE(r)->page_number = 0;
	OUTPUT_PRIVATE(r)->whole_report = NULL;
	OUTPUT(r)->do_align = TRUE;
	OUTPUT(r)->do_breaks = TRUE;
	OUTPUT(r)->do_grouptext = FALSE;
	OUTPUT(r)->paginate = FALSE;
	OUTPUT(r)->trim_links = FALSE;
	OUTPUT(r)->table_around_multiple_detail_columns = TRUE;
	OUTPUT(r)->do_graph = TRUE;

	OUTPUT(r)->get_string_width = html_get_string_width;
	OUTPUT(r)->print_text = html_print_text;
	OUTPUT(r)->print_text_delayed = html_print_text_delayed;
	OUTPUT(r)->set_fg_color = html_set_fg_color;
	OUTPUT(r)->set_bg_color = html_set_bg_color;
	OUTPUT(r)->hr = html_hr;
	OUTPUT(r)->start_draw_cell_background = html_start_draw_cell_background;
	OUTPUT(r)->end_draw_cell_background = html_end_draw_cell_background;
	OUTPUT(r)->start_boxurl = html_start_boxurl;
	OUTPUT(r)->end_boxurl = html_end_boxurl;
	OUTPUT(r)->line_image = html_line_image;
	OUTPUT(r)->background_image = html_background_image;
	OUTPUT(r)->set_font_point = html_set_font_point;
	OUTPUT(r)->start_new_page = html_start_new_page;
	OUTPUT(r)->end_page = html_end_page;
	OUTPUT(r)->init_end_page = html_init_end_page;
	OUTPUT(r)->start_rlib_report = html_start_rlib_report;
	OUTPUT(r)->end_rlib_report = html_end_rlib_report;
	OUTPUT(r)->start_report = html_start_report;
	OUTPUT(r)->end_report = html_end_report;
	OUTPUT(r)->start_report_field_headers = html_start_report_field_headers;
	OUTPUT(r)->end_report_field_headers = html_end_report_field_headers;
	OUTPUT(r)->start_report_field_details = html_start_report_field_details;
	OUTPUT(r)->end_report_field_details = html_end_report_field_details;
	OUTPUT(r)->start_report_line = html_start_report_line;
	OUTPUT(r)->end_report_line = html_end_report_line;
	OUTPUT(r)->start_part = html_start_part;
	OUTPUT(r)->end_part = html_end_part;
	OUTPUT(r)->start_report_header = html_start_report_header;
	OUTPUT(r)->end_report_header = html_end_report_header;
	OUTPUT(r)->start_report_footer = html_start_report_footer;
	OUTPUT(r)->end_report_footer = html_end_report_footer;
	OUTPUT(r)->start_part_header = html_start_part_header;
	OUTPUT(r)->end_part_header = html_end_part_header;
	OUTPUT(r)->start_part_page_header = html_start_part_page_header;
	OUTPUT(r)->end_part_page_header = html_end_part_page_header;
	OUTPUT(r)->start_part_page_footer = html_start_part_page_footer;
	OUTPUT(r)->end_part_page_footer = html_end_part_page_footer;
	OUTPUT(r)->start_report_page_footer = html_start_report_page_footer;
	OUTPUT(r)->end_report_page_footer = html_end_report_page_footer;
	OUTPUT(r)->start_report_break_header = html_start_report_break_header;
	OUTPUT(r)->end_report_break_header = html_end_report_break_header;
	OUTPUT(r)->start_report_break_footer = html_start_report_break_footer;
	OUTPUT(r)->end_report_break_footer = html_end_report_break_footer;
	OUTPUT(r)->start_report_no_data = html_start_report_no_data;
	OUTPUT(r)->end_report_no_data = html_end_report_no_data;
	
	OUTPUT(r)->finalize_private = html_finalize_private;
	OUTPUT(r)->spool_private = html_spool_private;
	OUTPUT(r)->start_line = html_start_line;
	OUTPUT(r)->end_line = html_end_line;
	OUTPUT(r)->start_output_section = html_start_output_section;
	OUTPUT(r)->end_output_section = html_end_output_section;
	OUTPUT(r)->start_evil_csv = html_start_evil_csv;
	OUTPUT(r)->end_evil_csv = html_end_evil_csv;
	OUTPUT(r)->get_output = html_get_output;
	OUTPUT(r)->get_output_length = html_get_output_length;
	OUTPUT(r)->set_working_page = html_set_working_page;
	OUTPUT(r)->set_raw_page = html_set_raw_page;
	OUTPUT(r)->start_part_table = html_start_part_table;
	OUTPUT(r)->end_part_table = html_end_part_table;
	OUTPUT(r)->start_part_tr = html_start_part_tr;
	OUTPUT(r)->end_part_tr = html_end_part_tr;
	OUTPUT(r)->start_part_td = html_start_part_td;
	OUTPUT(r)->end_part_td = html_end_part_td;
	OUTPUT(r)->start_part_pages_across = html_start_part_pages_across;
	OUTPUT(r)->end_part_pages_across = html_end_part_pages_across;
	OUTPUT(r)->start_bold = html_start_bold;
	OUTPUT(r)->end_bold = html_end_bold;
	OUTPUT(r)->start_italics = html_start_italics;
	OUTPUT(r)->end_italics = html_end_italics;

	OUTPUT(r)->graph_init = html_graph_init;
	OUTPUT(r)->graph_get_chart_layout = html_graph_get_chart_layout;
	OUTPUT(r)->start_graph = html_start_graph;
	OUTPUT(r)->graph_set_limits = html_graph_set_limits;
	OUTPUT(r)->graph_set_title = html_graph_set_title;
	OUTPUT(r)->graph_set_name = html_graph_set_name;
	OUTPUT(r)->graph_set_legend_bg_color = html_graph_set_legend_bg_color;
	OUTPUT(r)->graph_set_legend_orientation = html_graph_set_legend_orientation;
	OUTPUT(r)->graph_set_draw_x_y = html_graph_set_draw_x_y;
	OUTPUT(r)->graph_set_is_chart = html_graph_set_is_chart;
	OUTPUT(r)->graph_set_bold_titles = html_graph_set_bold_titles;
	OUTPUT(r)->graph_set_minor_ticks = html_graph_set_minor_ticks;
	OUTPUT(r)->graph_set_grid_color = html_graph_set_grid_color;
	OUTPUT(r)->graph_x_axis_title = html_graph_x_axis_title;
	OUTPUT(r)->graph_y_axis_title = html_graph_y_axis_title;
	OUTPUT(r)->graph_do_grid = html_graph_do_grid;
	OUTPUT(r)->graph_tick_x = html_graph_tick_x;
	OUTPUT(r)->graph_set_x_tick_width = html_graph_set_x_tick_width;
	OUTPUT(r)->graph_set_x_iterations = html_graph_set_x_iterations;
	OUTPUT(r)->graph_tick_y = html_graph_tick_y;
	OUTPUT(r)->graph_hint_label_x = html_graph_hint_label_x;
	OUTPUT(r)->graph_label_x = html_graph_label_x;
	OUTPUT(r)->graph_label_y = html_graph_label_y;
	OUTPUT(r)->graph_draw_line = html_graph_draw_line;
	OUTPUT(r)->graph_draw_bar = html_graph_draw_bar;
	OUTPUT(r)->graph_plot_bar = html_graph_plot_bar;
	OUTPUT(r)->graph_plot_line = html_graph_plot_line;
	OUTPUT(r)->graph_plot_pie = html_graph_plot_pie;
	OUTPUT(r)->graph_set_data_plot_count = html_graph_set_data_plot_count;
	OUTPUT(r)->graph_hint_label_y = html_graph_hint_label_y;
	OUTPUT(r)->graph_hint_legend = html_graph_hint_legend;
	OUTPUT(r)->graph_draw_legend = html_graph_draw_legend;
	OUTPUT(r)->graph_draw_legend_label = html_graph_draw_legend_label;
	OUTPUT(r)->end_graph = html_end_graph;

	OUTPUT(r)->graph_set_x_label_width = html_graph_set_x_label_width;
	OUTPUT(r)->graph_get_x_label_width = html_graph_get_x_label_width;
	OUTPUT(r)->graph_set_y_label_width = html_graph_set_y_label_width;
	OUTPUT(r)->graph_get_y_label_width = html_graph_get_y_label_width;
	OUTPUT(r)->graph_get_width_offset = html_graph_get_width_offset;

	OUTPUT(r)->free = html_free;
}
