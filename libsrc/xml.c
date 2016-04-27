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
#include "pcode.h"

#define TEXT 1
#define DELAY 2

struct _private {
	GString *whole_report;
	GString **top_of_page;
	GString **bottom_of_page;
	gint page_number;
};


static gfloat xml_get_string_width(rlib *r, const gchar *text) {
	return 1;
}

const gchar * rlib_xml_value_get_type_as_str(struct rlib_value *v) {
	if(RLIB_VALUE_IS_NUMBER(v))
		return "number";
	if(RLIB_VALUE_IS_STRING(v))
		return "string";
	if(RLIB_VALUE_IS_DATE(v))
		return "date";
	return NULL;
}
 
static void xml_print_text(rlib *r, gfloat left_origin, gfloat bottom_origin, const gchar *text, gint backwards, struct rlib_line_extra_data *extra_data) {
	gchar *escaped = g_markup_escape_text(text, strlen(text));
	const gchar *field_type = NULL;

	if (extra_data->report_field && extra_data->report_field->rval) 
		field_type = rlib_xml_value_get_type_as_str(extra_data->report_field->rval);

	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number],"<data col=\"%d\" width=\"%d\" font_point=\"%d\" bold=\"%d\" italics=\"%d\" align=\"%s\" ", extra_data->col, extra_data->width, extra_data->font_point, extra_data->is_bold, extra_data->is_italics, extra_data->align == RLIB_ALIGN_CENTER ? "center" : extra_data->align == RLIB_ALIGN_RIGHT ? "right" : "left");
	if(extra_data->found_bgcolor) 
		g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number],"bgcolor=\"#%02x%02x%02x\" ", (gint)(extra_data->bgcolor.r*0xFF), (gint)(extra_data->bgcolor.g*0xFF), (gint)(extra_data->bgcolor.b*0xFF));
	if(extra_data->found_color) 
		g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number],"color=\"#%02x%02x%02x\" ", (gint)(extra_data->color.r*0xFF), (gint)(extra_data->color.g*0xFF), (gint)(extra_data->color.b*0xFF));
	if (field_type)
		g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number],"type=\"%s\" ", field_type);
		
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number],">%s</data>\n", escaped);
	g_free(escaped);
}


static void xml_start_new_page(rlib *r, struct rlib_part *part) {
}

static void xml_init_end_page(rlib *r) {}
static void xml_finalize_private(rlib *r) {}

static void xml_spool_private(rlib *r) {
	ENVIRONMENT(r)->rlib_write_output(OUTPUT_PRIVATE(r)->whole_report->str, OUTPUT_PRIVATE(r)->whole_report->len);
}


static void xml_start_rlib_report(rlib *r) {
	OUTPUT_PRIVATE(r)->whole_report = g_string_new("<rlib>\n");
}

static void xml_end_rlib_report(rlib *r) {
	g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</rlib>\n");

}

static void xml_start_part(rlib *r, struct rlib_part *part) {
	int i;
	OUTPUT_PRIVATE(r)->top_of_page = g_new0(GString *, part->pages_across);
	OUTPUT_PRIVATE(r)->bottom_of_page = g_new0(GString *, part->pages_across);
	for(i=0;i<part->pages_across;i++) {
		OUTPUT_PRIVATE(r)->top_of_page[i] = g_string_new("");
		OUTPUT_PRIVATE(r)->bottom_of_page[i] = g_string_new("");
	}
}

static void xml_end_part(rlib *r, struct rlib_part *part) {
	int i;

	g_string_append(OUTPUT_PRIVATE(r)->whole_report, "<part>\n");
	for(i=0;i<part->pages_across;i++) {
		g_string_append_len(OUTPUT_PRIVATE(r)->whole_report, OUTPUT_PRIVATE(r)->top_of_page[i]->str, OUTPUT_PRIVATE(r)->top_of_page[i]->len);
		g_string_append_len(OUTPUT_PRIVATE(r)->whole_report, OUTPUT_PRIVATE(r)->bottom_of_page[i]->str, OUTPUT_PRIVATE(r)->bottom_of_page[i]->len);
		
		g_string_free(OUTPUT_PRIVATE(r)->top_of_page[i], TRUE);
		g_string_free(OUTPUT_PRIVATE(r)->bottom_of_page[i], TRUE);
	}
	g_free(OUTPUT_PRIVATE(r)->top_of_page);
	g_free(OUTPUT_PRIVATE(r)->bottom_of_page);
	g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</part>\n");
	
}


static void xml_start_report(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	//Doesn't work pages across
	//g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<report>");
}

static void xml_end_report(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	//Doesn't work pages across
	//g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</report>");
}

static void xml_start_report_field_headers(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<field_headers>\n");
}

static void xml_end_report_field_headers(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</field_headers>\n");
}

static void xml_start_report_field_details(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<field_details>\n");
}

static void xml_end_report_field_details(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</field_details>\n");
}

static void xml_start_report_line(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void xml_end_report_line(rlib *r, struct rlib_part *part, struct rlib_report *report) {}


static void xml_start_report_header(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<report_header>\n");
}

static void xml_end_report_header(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</report_header>\n");
}

static void xml_start_report_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<report_footer>\n");
}

static void xml_end_report_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</report_footer>\n");
}


static void xml_start_part_tr(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<tr>\n");
}

static void xml_end_part_tr(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</tr>\n");
}

static void xml_start_part_table(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<table>\n");
}

static void xml_end_part_table(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</table>\n");
}


static void xml_start_part_td(rlib *r, struct rlib_part *part, gfloat width, gfloat height) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<td ");
	if (width)
		g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "width=\"%f\" ", width);
	if (height)
		g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "height=\"%f\" ", height);
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], ">");
}

static void xml_end_part_td(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</td>\n");
}


static void xml_start_part_pages_across(rlib *r, struct rlib_part *part, gfloat left_margin, gfloat top_margin, int width, int height, int border_width, struct rlib_rgb *color) {}

static void xml_end_part_pages_across(rlib *r, struct rlib_part *part) {}

static void xml_start_line(rlib *r, gint backwards) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<line>\n");
}

static void xml_end_line(rlib *r, int backwards) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</line>\n");
}

static void xml_start_output_section(rlib *r, struct rlib_report_output_array *roa) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<output>\n");
}

static void xml_end_output_section(rlib *r, struct rlib_report_output_array *roa) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</output>\n");
}

static void xml_start_evil_csv(rlib *r) {}
static void xml_end_evil_csv(rlib *r) {}

static void xml_start_part_header(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<part_header>\n");
}

static void xml_end_part_header(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</part_header>\n");	
}

static void xml_start_part_page_header(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<part_page_header>\n");
}

static void xml_end_part_page_header(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</part_page_header>\n");
}


static void xml_start_part_page_footer(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<part_page_footer>\n");
}

static void xml_end_part_page_footer(rlib *r, struct rlib_part *part) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</part_page_footer>\n");
}

static void xml_start_report_page_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<report_page_footer>\n");
}

static void xml_end_report_page_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</report_page_footer>\n");
}

static void xml_start_report_break_header(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<break_header name=\"%s\">\n", rb->xml_name.xml);
}

static void xml_end_report_break_header(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</break_header>\n");
}

static void xml_start_report_break_footer(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<break_footer name=\"%s\">\n", rb->xml_name.xml);
}

static void xml_end_report_break_footer(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</break_footer>\n");
}

static void xml_start_report_no_data(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<no_data>\n");
}

static void xml_end_report_no_data(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</no_data>\n");
}

static void xml_start_graph(rlib *r, struct rlib_part *part, struct rlib_report *report, gfloat left, gfloat top, gfloat width, gfloat height, gboolean x_axis_labels_are_under_tick) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<graph width=\"%f\" height=\"%f\">\n", width, height);	
}

static void xml_end_graph(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	g_string_append(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "</graph>\n");		
}

static void xml_graph_set_title(rlib *r, gchar *title) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<title>%s</title>\n", title);		
}

static void xml_graph_x_axis_title(rlib *r, gchar *title) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<x_axis_title>%s</x_axis_title>\n", title);		
	
}

static void xml_graph_y_axis_title(rlib *r, gchar side, gchar *title) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<y_axis_title side=\"%s\">%s</y_axis_title>\n", side == RLIB_SIDE_LEFT ? "left" : "right", title);		
}

static void xml_graph_plot_line(rlib *r, gchar side, gint iteration, gfloat p1_height, gfloat p1_last_height, gfloat p2_height, gfloat p2_last_height, struct rlib_rgb * color, gfloat value, gchar *label, gint row_count) {
	gchar *escaped_label = g_markup_escape_text(label, strlen(label));

	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], 
		"<plot_line side=\"%s\" iteration=\"%d\" p1_height=\"%f\" p1_last_height=\"%f\" p2_height=\"%f\" p2_last_height=\"%f\" color=\"#%02x%02x%02x\" value=\"%f\" label=\"%s\"/>\n", 
		side == RLIB_SIDE_LEFT ? "left" : "right", iteration, p1_height, p1_last_height, p2_height, p2_last_height,
		(gint)(color->r*0xFF), (gint)(color->g*0xFF), (gint)(color->b*0xFF), value, escaped_label);			

	g_free(escaped_label);
}

static void xml_graph_plot_pie(rlib *r, gfloat start, gfloat end, gboolean offset, struct rlib_rgb *color, gfloat value, gchar *label) {
	gchar *escaped_label = g_markup_escape_text(label, strlen(label));

	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], 
		"<plot_pie start=\"%f\" end=\"%f\" offset=\"%s\" color=\"#%02x%02x%02x\" value=\"%f\" label=\"%s\"/>", start, end, offset ? "true" : "false", 
		(gint)(color->r*0xFF), (gint)(color->g*0xFF), (gint)(color->b*0xFF), value, escaped_label);

	g_free(escaped_label);
}

static void xml_graph_plot_bar(rlib *r, gchar side, gint iteration, gint plot, gfloat height_percent, struct rlib_rgb *color, gfloat last_height, gboolean divide_iterations, gfloat value, gchar *label) {
	gchar *escaped_label = g_markup_escape_text(label, strlen(label));

	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], 
		"<plot_bar side=\"%s\" iteration=\"%d\" plot=\"%d\" height_percent=\"%f\" color=\"#%02x%02x%02x\" last_height=\"%f\" divide_iterations=\"%s\" value=\"%f\" label=\"%s\"/>", 
		side == RLIB_SIDE_LEFT ? "left" : "right", iteration, plot, height_percent,
		(gint)(color->r*0xFF), (gint)(color->g*0xFF), (gint)(color->b*0xFF), last_height, divide_iterations ? "true" : "false", value, escaped_label);
	
	g_free(escaped_label);
}

static void xml_graph_set_limits(rlib *r, gchar side, gdouble min, gdouble max, gdouble origin) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<limits side=\"%s\" min=\"%f\" max=\"%f\" origin=\"%f\"/>\n", 
		side == RLIB_SIDE_LEFT ? "left" : "right", min, max, origin);		
	
}

static void xml_graph_do_grid(rlib *r, gboolean just_a_box) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<grid draw_lines=\"%s\"/>\n", just_a_box ? "false" : "true");		
	
}

static void xml_graph_set_x_iterations(rlib *r, gint iterations) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<x_iterations>%d</x_iterations>\n", iterations);		
	
}

static void xml_graph_tick_y(rlib *r, gint iterations) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<y_iterations>%d</y_iterations>\n", iterations);		
}

static void xml_graph_label_x(rlib *r, gint iteration, gchar *label) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<x_iterations_label iteration=\"%d\">%s</x_iterations_label>\n", iteration, label);		
}

static void xml_graph_label_y(rlib *r, gchar side, gint iteration, gchar *label) {
	g_string_append_printf(OUTPUT_PRIVATE(r)->top_of_page[OUTPUT_PRIVATE(r)->page_number], "<y_iterations_label side=\"%s\" iteration=\"%d\">%s</y_iterations_label>\n", 
	side == RLIB_SIDE_LEFT ? "left" : "right", iteration, label);		
}



static void xml_print_text_delayed(rlib *r, struct rlib_delayed_extra_data *delayed_data, int backwards, int rval_type) {
}



static void xml_end_page(rlib *r, struct rlib_part *part) {
	r->current_page_number++;
	r->current_line_number = 1;
}

static int xml_free(rlib *r) {
	g_string_free(OUTPUT_PRIVATE(r)->whole_report, TRUE);	
	g_free(OUTPUT_PRIVATE(r));
	g_free(OUTPUT(r));
	return 0;
}

static char *xml_get_output(rlib *r) {
	return OUTPUT_PRIVATE(r)->whole_report->str;
}

static long xml_get_output_length(rlib *r) {
	return OUTPUT_PRIVATE(r)->whole_report->len;
}

static void xml_set_working_page(rlib *r, struct rlib_part *part, int page) {
	OUTPUT_PRIVATE(r)->page_number = page;
}

static void xml_set_fg_color(rlib *r, gfloat red, gfloat green, gfloat blue) {}
static void xml_set_bg_color(rlib *r, gfloat red, gfloat green, gfloat blue) {}
static void xml_hr(rlib *r, gint backwards, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall, 
struct rlib_rgb *color, gfloat indent, gfloat length) {}
static void xml_start_draw_cell_background(rlib *r, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall, 
struct rlib_rgb *color) {}
static void xml_end_draw_cell_background(rlib *r) {}
static void xml_start_boxurl(rlib *r, struct rlib_part * part, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall, gchar *url, gint backwards) {}
static void xml_end_boxurl(rlib *r, gint backwards) {}
static void xml_background_image(rlib *r, gfloat left_origin, gfloat bottom_origin, gchar *nname, gchar *type, gfloat nwidth, gfloat nheight) {}
static void xml_set_font_point(rlib *r, gint point) {}
static void xml_set_raw_page(rlib *r, struct rlib_part *part, gint page) {}
static void xml_start_bold(rlib *r) {}
static void xml_end_bold(rlib *r) {}
static void xml_start_italics(rlib *r) {}
static void xml_end_italics(rlib *r) {}

static void xml_graph_hint_label_x(rlib *r, gchar *label) {}
static void xml_graph_hint_label_y(rlib *r, gchar side, gchar *label) {}
static void xml_graph_tick_x(rlib *r) {}

static void xml_graph_set_data_plot_count(rlib *r, gint count) {}
static void xml_graph_hint_legend(rlib *r, gchar *label) {}
static void xml_graph_draw_legend(rlib *r) {}
static void xml_graph_draw_legend_label(rlib *r, gint iteration, gchar *label, struct rlib_rgb *color, gboolean is_line) {}
static void xml_graph_draw_line(rlib *r, gfloat x, gfloat y, gfloat new_x, gfloat new_y, struct rlib_rgb *color) {}
static void xml_graph_set_name(rlib *r, gchar *name) {}
static void xml_graph_set_legend_bg_color(rlib *r, struct rlib_rgb *rgb) {}
static void xml_graph_set_legend_orientation(rlib *r, gint orientation) {}
static void xml_graph_set_draw_x_y(rlib *r, gboolean draw_x, gboolean draw_y) {}
static void xml_graph_set_bold_titles(rlib *r, gboolean bold_titles) {}
static void xml_graph_set_grid_color(rlib *r, struct rlib_rgb *rgb) {}
static void xml_graph_set_minor_ticks(rlib *r, gboolean *minor_ticks) {}

void rlib_xml_new_output_filter(rlib *r) {
	OUTPUT(r) = g_malloc(sizeof(struct output_filter));
	r->o->private = g_malloc(sizeof(struct _private));
	memset(OUTPUT_PRIVATE(r), 0, sizeof(struct _private));
	
	OUTPUT(r)->do_align = TRUE;
	OUTPUT(r)->do_breaks = TRUE;
	OUTPUT(r)->do_grouptext = FALSE;	
	OUTPUT(r)->paginate = FALSE;
	OUTPUT(r)->trim_links = FALSE;
	OUTPUT(r)->do_graph = TRUE;
	
	OUTPUT(r)->get_string_width = xml_get_string_width;
	OUTPUT(r)->print_text = xml_print_text;
	OUTPUT(r)->set_fg_color = xml_set_fg_color;
	OUTPUT(r)->set_bg_color = xml_set_bg_color;
	OUTPUT(r)->print_text_delayed = xml_print_text_delayed;	
	OUTPUT(r)->hr = xml_hr;
	OUTPUT(r)->start_draw_cell_background = xml_start_draw_cell_background;
	OUTPUT(r)->end_draw_cell_background = xml_end_draw_cell_background;
	OUTPUT(r)->start_boxurl = xml_start_boxurl;
	OUTPUT(r)->end_boxurl = xml_end_boxurl;
	OUTPUT(r)->background_image = xml_background_image;
	OUTPUT(r)->line_image = xml_background_image;
	OUTPUT(r)->set_font_point = xml_set_font_point;
	OUTPUT(r)->start_new_page = xml_start_new_page;
	OUTPUT(r)->end_page = xml_end_page;   
	OUTPUT(r)->init_end_page = xml_init_end_page;
	OUTPUT(r)->start_rlib_report = xml_start_rlib_report;
	OUTPUT(r)->end_rlib_report = xml_end_rlib_report;
	OUTPUT(r)->start_report = xml_start_report;
	OUTPUT(r)->end_report = xml_end_report;
	OUTPUT(r)->start_report_field_headers = xml_start_report_field_headers;
	OUTPUT(r)->end_report_field_headers = xml_end_report_field_headers;
	OUTPUT(r)->start_report_field_details = xml_start_report_field_details;
	OUTPUT(r)->end_report_field_details = xml_end_report_field_details;
	OUTPUT(r)->start_report_line = xml_start_report_line;
	OUTPUT(r)->end_report_line = xml_end_report_line;
	OUTPUT(r)->start_part = xml_start_part;
	OUTPUT(r)->end_part = xml_end_part;
	OUTPUT(r)->start_report_header = xml_start_report_header;
	OUTPUT(r)->end_report_header = xml_end_report_header;
	OUTPUT(r)->start_report_footer = xml_start_report_footer;
	OUTPUT(r)->end_report_footer = xml_end_report_footer;
	OUTPUT(r)->start_part_header = xml_start_part_header;
	OUTPUT(r)->end_part_header = xml_end_part_header;
	OUTPUT(r)->start_part_page_header = xml_start_part_page_header;
	OUTPUT(r)->end_part_page_header = xml_end_part_page_header;
	OUTPUT(r)->start_part_page_footer = xml_start_part_page_footer;
	OUTPUT(r)->end_part_page_footer = xml_end_part_page_footer;
	OUTPUT(r)->start_report_page_footer = xml_start_report_page_footer;
	OUTPUT(r)->end_report_page_footer = xml_end_report_page_footer;
	OUTPUT(r)->start_report_break_header = xml_start_report_break_header;
	OUTPUT(r)->end_report_break_header = xml_end_report_break_header;
	OUTPUT(r)->start_report_break_footer = xml_start_report_break_footer;
	OUTPUT(r)->end_report_break_footer = xml_end_report_break_footer;
	OUTPUT(r)->start_report_no_data = xml_start_report_no_data;
	OUTPUT(r)->end_report_no_data = xml_end_report_no_data;


	OUTPUT(r)->finalize_private = xml_finalize_private;
	OUTPUT(r)->spool_private = xml_spool_private;
	OUTPUT(r)->start_line = xml_start_line;
	OUTPUT(r)->end_line = xml_end_line;
	OUTPUT(r)->start_output_section = xml_start_output_section;   
	OUTPUT(r)->end_output_section = xml_end_output_section; 
	OUTPUT(r)->start_evil_csv = xml_start_evil_csv;   
	OUTPUT(r)->end_evil_csv = xml_end_evil_csv; 
	OUTPUT(r)->get_output = xml_get_output;
	OUTPUT(r)->get_output_length = xml_get_output_length;
	OUTPUT(r)->set_working_page = xml_set_working_page;  
	OUTPUT(r)->set_raw_page = xml_set_raw_page; 
	OUTPUT(r)->start_part_tr = xml_start_part_tr; 
	OUTPUT(r)->end_part_tr = xml_end_part_tr; 
	OUTPUT(r)->start_part_table = xml_start_part_table; 
	OUTPUT(r)->end_part_table = xml_end_part_table; 
	OUTPUT(r)->start_part_td = xml_start_part_td; 
	OUTPUT(r)->end_part_td = xml_end_part_td; 
	OUTPUT(r)->start_part_pages_across = xml_start_part_pages_across; 
	OUTPUT(r)->end_part_pages_across = xml_end_part_pages_across; 
	OUTPUT(r)->start_bold = xml_start_bold;
	OUTPUT(r)->end_bold = xml_end_bold;
	OUTPUT(r)->start_italics = xml_start_italics;
	OUTPUT(r)->end_italics = xml_end_italics;

	OUTPUT(r)->start_graph = xml_start_graph;
	OUTPUT(r)->graph_set_limits = xml_graph_set_limits;
	OUTPUT(r)->graph_set_title = xml_graph_set_title;
	OUTPUT(r)->graph_set_name = xml_graph_set_name;
	OUTPUT(r)->graph_set_legend_bg_color = xml_graph_set_legend_bg_color;
	OUTPUT(r)->graph_set_legend_orientation = xml_graph_set_legend_orientation;
	OUTPUT(r)->graph_set_draw_x_y = xml_graph_set_draw_x_y;
	OUTPUT(r)->graph_set_minor_ticks = xml_graph_set_minor_ticks;
	OUTPUT(r)->graph_set_bold_titles = xml_graph_set_bold_titles;
	OUTPUT(r)->graph_set_grid_color = xml_graph_set_grid_color;
	OUTPUT(r)->graph_x_axis_title = xml_graph_x_axis_title;
	OUTPUT(r)->graph_y_axis_title = xml_graph_y_axis_title;
	OUTPUT(r)->graph_do_grid = xml_graph_do_grid;
	OUTPUT(r)->graph_tick_x = xml_graph_tick_x;
	OUTPUT(r)->graph_set_x_iterations = xml_graph_set_x_iterations;
	OUTPUT(r)->graph_tick_y = xml_graph_tick_y;
	OUTPUT(r)->graph_hint_label_x = xml_graph_hint_label_x;
	OUTPUT(r)->graph_label_x = xml_graph_label_x;
	OUTPUT(r)->graph_label_y = xml_graph_label_y;
	OUTPUT(r)->graph_draw_line = xml_graph_draw_line;
	OUTPUT(r)->graph_plot_bar = xml_graph_plot_bar;
	OUTPUT(r)->graph_plot_line = xml_graph_plot_line;
	OUTPUT(r)->graph_plot_pie = xml_graph_plot_pie;
	OUTPUT(r)->graph_set_data_plot_count = xml_graph_set_data_plot_count;
	OUTPUT(r)->graph_hint_label_y = xml_graph_hint_label_y;
	OUTPUT(r)->graph_hint_legend = xml_graph_hint_legend;
	OUTPUT(r)->graph_draw_legend = xml_graph_draw_legend;
	OUTPUT(r)->graph_draw_legend_label = xml_graph_draw_legend_label;
	OUTPUT(r)->end_graph = xml_end_graph;

	OUTPUT(r)->free = xml_free;  
}
