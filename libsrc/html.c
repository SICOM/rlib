/*
 *  Copyright (C) 2003-2016 SICOM Systems, INC.
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
 * This module implements the HTML output renderer for generating an HTML
 * formatted report from the rlib object.
 */

#include <config.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "rlib-internal.h"
#include "rlib_gd.h"

#define TEXT 1
#define DELAY 2
#define BOTTOM_PAD 8
#define BIGGER_HTML_FONT(a) (a + 2)

struct _packet {
	char type;
	gpointer data;
};

struct _graph {
	gdouble y_min;
	gdouble y_max;
	gdouble y_origin;
	gdouble non_standard_x_start;
	gint64 legend_width;
	gint64 legend_height;
	gint64 data_plot_count;
	gint64 whole_graph_width;
	gint64 whole_graph_height;
	gint64 legend_top;
	gint64 legend_left;
	gint64 width;
	gint64 just_a_box;
	gint64 height;
	gint64 title_height;
	gint64 x_label_width;
	gint64 y_label_width_left;
	gint64 y_label_width_right;
	gint64 x_axis_label_height;
	gint64 y_axis_title_left;
	gint64 y_axis_title_right;
	gint64 top;
	gint64 y_iterations;
	gint64 intersection;
	gdouble x_tick_width;
	gint64 x_iterations;
	gint64 x_start;
	gint64 y_start;
	gint64 vertical_x_label;
	gboolean x_axis_labels_are_under_tick;
	gboolean has_legend_bg_color;
	struct rlib_rgb legend_bg_color;
	gint64 legend_orientation;
	gboolean draw_x;
	gboolean draw_y;
	struct rlib_rgb *grid_color;
	struct rlib_rgb real_grid_color;
	gchar *name;
	gint64 region_count;
	gint64 orig_data_plot_count;
	gint64 current_region;
	gboolean bold_titles;
	gboolean *minor_ticks;
	gint64 last_left_x_label;
	gint64 is_chart;
};

struct _private {
	GString *whole_report;
	GSList **top;
	GSList **bottom;

	gint64 pages;
	gint64 page_number;
	struct rlib_gd *rgd;
	struct _graph graph;
};

static void html_graph_get_x_label_width(rlib *r, gdouble *width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	*width = (gdouble)graph->x_label_width;
}

static void html_graph_set_x_label_width(rlib *r, gdouble width, gint64 cell_width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if ((gint64)width == 0) {
		graph->x_label_width = cell_width;
	} else
		graph->x_label_width = width;

	if (width >= cell_width - 2)
		graph->vertical_x_label = TRUE;
}

static void html_graph_get_y_label_width(rlib *r, gdouble *width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	*width = (gdouble)graph->y_label_width_left;
}

static void html_graph_set_y_label_width(rlib *r, gdouble width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if (width == 0)
		graph->y_label_width_left = gd_get_string_width("W", FALSE) * 2;
	else
		graph->y_label_width_left = width;
}

static void html_graph_get_width_offset(rlib *r, gint64 *width_offset) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint64 intersection = 5;
	*width_offset = 0;//1;
	*width_offset += graph->y_label_width_left + intersection;
}

static void print_text(rlib *r, const gchar *text, gint64 backwards) {
	gint64 current_page = OUTPUT_PRIVATE(r)->page_number;
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
		if (OUTPUT_PRIVATE(r)->top[current_page] != NULL) {
			packet = OUTPUT_PRIVATE(r)->top[current_page]->data;
		}
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

static gdouble html_get_string_width(rlib *r UNUSED, const gchar *text UNUSED) {
	return 1;
}

static gchar *get_html_color(gchar *str, struct rlib_rgb *color) {
	sprintf(str, "#%02x%02x%02x", color2hex(color->r), color2hex(color->g), color2hex(color->b));
	return str;
}

static GString *html_print_text_common(const gchar *text, struct rlib_line_extra_data *extra_data) {
	GString *string = g_string_new("");
	gchar *escaped;
	gint pos;

	g_string_append_printf(string, "<span data-col=\"%" PRId64 "\" data-width=\"%" PRId64 "\" style=\"font-size: %" PRId64 "px; ", extra_data->col, extra_data->width, BIGGER_HTML_FONT(extra_data->font_point));

	if (extra_data->found_bgcolor)
		g_string_append_printf(string, "background-color: #%02x%02x%02x; ", color2hex(extra_data->bgcolor.r), color2hex(extra_data->bgcolor.g), color2hex(extra_data->bgcolor.b));
	if (extra_data->found_color)
		g_string_append_printf(string, "color:#%02x%02x%02x ", color2hex(extra_data->color.r), color2hex(extra_data->color.g), color2hex(extra_data->color.b));
	if (extra_data->is_bold == TRUE)
		g_string_append(string, "font-weight: bold;");
	if (extra_data->is_italics == TRUE)
		g_string_append(string, "font-style: italic;");

	g_string_append(string,"\">");
	escaped = g_markup_escape_text(text, strlen(text));

#if 0
	gboolean only_spaces = TRUE;
	for (pos = 0; escaped[pos]; pos++) {
		if (!isspace(escaped[pos])) {
			only_spaces = FALSE;
			break;
		}
	}
	if (only_spaces) {
		GString *new_esc = g_string_new(NULL);
		gint i;
		for (i = 0; i < pos; i++) {
			g_string_append(new_esc, "&nbsp;");
		}
		g_free(escaped);
		escaped = g_string_free(new_esc, FALSE);
	}
#else
	{
		GString *new_esc = g_string_new(NULL);
		for (pos = 0; escaped[pos]; pos++) {
			if (isspace(escaped[pos]))
				g_string_append(new_esc, "&nbsp;");
			else
				g_string_append_c(new_esc, escaped[pos]);
		}
		g_free(escaped);
		escaped = g_string_free(new_esc, FALSE);
	}
#endif
	g_string_append(string, escaped);
	g_string_append(string, "</span>");
	g_free(escaped);

	return string;
}

static void html_print_text(rlib *r, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, const gchar *text, gint64 backwards, struct rlib_line_extra_data *extra_data) {
	GString *string = html_print_text_common(text, extra_data);
	print_text(r, string->str, backwards);
	g_string_free(string, TRUE);
}


static void html_set_fg_color(rlib *r UNUSED, gdouble red UNUSED, gdouble green UNUSED, gdouble blue UNUSED) {}

static void html_set_bg_color(rlib *r UNUSED, gdouble red UNUSED, gdouble green UNUSED, gdouble blue UNUSED) {}

static void html_start_draw_cell_background(rlib *r, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gdouble how_long UNUSED, gdouble how_tall UNUSED, struct rlib_rgb *color) {
	OUTPUT(r)->set_bg_color(r, color->r, color->g, color->b);
}

static void html_end_draw_cell_background(rlib *r UNUSED) {}

static void html_start_boxurl(rlib *r, struct rlib_part *part UNUSED, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gdouble how_long UNUSED, gdouble how_tall UNUSED, gchar *url, gint64 backwards) {
	gchar buf[MAXSTRLEN];
	sprintf(buf, "<a href=\"%s\">", url);
	print_text(r, buf, backwards);
}

static void html_end_boxurl(rlib *r, gint64 backwards) {
	print_text(r, "</a>", backwards);
}



static void html_hr(rlib *r, gint64 backwards, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gdouble how_long UNUSED, gdouble how_tall,
struct rlib_rgb *color, gdouble indent, gdouble length UNUSED) {
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

static void html_background_image(rlib *r, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gchar *nname, gchar *type UNUSED, gdouble nwidth,
gdouble nheight) {
	gchar buf[MAXSTRLEN];

	print_text(r, "<span style=\"float: left; position:absolute;\">", FALSE);
	sprintf(buf, "<img src=\"%s\" width=\"%f\" height=\"%f\" alt=\"background image\"/>", nname, nwidth, nheight);
	print_text(r, buf, FALSE);
	print_text(r, "</span>", FALSE);
}

static void html_line_image(rlib *r, gdouble left_origin UNUSED, gdouble bottom_origin UNUSED, gchar *nname, gchar *type UNUSED, gdouble nwidth UNUSED, gdouble nheight UNUSED) {
	gchar buf[MAXSTRLEN];

	sprintf(buf, "<img src=\"%s\" alt=\"line image\"/>", nname);
	print_text(r, buf, FALSE);
}

static void html_set_font_point(rlib *r, gint64 point) {
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
	struct rlib_line_extra_data *extra_data = delayed_data->extra_data;
	rlib *r = delayed_data->r;
	gchar *buf = NULL, *buf2 = NULL;
	GString *string;

	if (rlib_pcode_has_variable(r, extra_data->field_code, NULL, NULL, FALSE))
		return NULL;

	rlib_value_free(r, &extra_data->rval_code);
	if (rlib_execute_pcode(r, &extra_data->rval_code, extra_data->field_code, NULL) == NULL)
		return NULL;
	rlib_format_string(r, &buf, extra_data->report_field, &extra_data->rval_code);
	rlib_align_text(r, &buf2, buf, extra_data->report_field->align, extra_data->report_field->width);
	g_free(buf);

	string = html_print_text_common(buf2, extra_data);
	g_free(buf2);
	rlib_free_delayed_extra_data(r, delayed_data);
	return g_string_free(string, FALSE);
}

static void html_print_text_delayed(rlib *r, struct rlib_delayed_extra_data *delayed_data, gint64 backwards, gint64 rval_type UNUSED) {
	gint64 current_page = OUTPUT_PRIVATE(r)->page_number;
	struct _packet *packet = g_new0(struct _packet, 1);
	packet->type = DELAY;
	packet->data = delayed_data;

	if (backwards)
		OUTPUT_PRIVATE(r)->bottom[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->bottom[current_page], packet);
	else
		OUTPUT_PRIVATE(r)->top[current_page] = g_slist_prepend(OUTPUT_PRIVATE(r)->top[current_page], packet);
}

static void html_finalize_text_delayed(rlib *r, gpointer in_ptr, gint64 backwards) {
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
			if (packet->type == DELAY && packet->data == in_ptr) {
				gchar *text = html_callback(packet->data);
				struct _packet *new_packet;

				if (text) {
					new_packet = g_new0(struct _packet, 1);
					new_packet->type = TEXT;
					new_packet->data = g_string_new(text);
					l->data = new_packet;

					g_free(text);
					g_free(packet);
				}
				return;
			}
		}
	}
}

static void html_start_report(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_end_report(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_start_report_field_headers(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_end_report_field_headers(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_start_report_field_details(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_end_report_field_details(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_start_report_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_end_report_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_start_report_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_end_report_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}

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
		if (font_size <= 0)
			font_size = RLIB_DEFUALT_FONTPOINT;

		g_string_append(OUTPUT_PRIVATE(r)->whole_report, doctype);
		g_string_append_printf(OUTPUT_PRIVATE(r)->whole_report, "<head>\n<style type=\"text/css\">\n");

		g_string_append_printf(OUTPUT_PRIVATE(r)->whole_report, "pre { margin:0; padding:0; margin-top:0; margin-bottom:0; font-size:%dpt;}\n", BIGGER_HTML_FONT(font_size));
		g_string_append_printf(OUTPUT_PRIVATE(r)->whole_report, "body { background-color: #ffffff;}\n");
		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "TABLE { border: 0; border-spacing: 0; padding: 0; width:100%; }\n");
		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</style>\n");
		meta = g_hash_table_lookup(r->output_parameters, "meta");
		if (meta != NULL) {
			int start = 0;
			int len = strlen(meta);
			gboolean closed_meta = FALSE;

			while (isspace(meta[start]))
				start++;
			if (meta[start] == '<') {
				start++;
				while (isspace(meta[start]))
					start++;
				if (strncmp(meta + start, "meta", 4) == 0 && strlen(meta + start) >= 4 && isspace(meta + start + 4)) {
					/* Found "meta" */
					while (len && isspace(meta[len - 1]))
						len--;
					if (meta[len - 1] == '>' && meta[len - 2] == '/')
						closed_meta = TRUE;
					else if (strcmp(meta + len - 7, "</meta>") == 0)
						closed_meta = TRUE;

					g_string_append(OUTPUT_PRIVATE(r)->whole_report, meta);
					if (!closed_meta)
						g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</meta>");
				}
			}
		}
		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "<title>RLIB Report</title></head>\n");
		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "<body>\n");
   	}
}

static void html_start_part(rlib *r, struct rlib_part *part) {
	if (OUTPUT_PRIVATE(r)->top == NULL) {
		OUTPUT_PRIVATE(r)->pages = part->pages_across;
		OUTPUT_PRIVATE(r)->top = g_new0(GSList *, part->pages_across);
		OUTPUT_PRIVATE(r)->bottom = g_new0(GSList *, part->pages_across);
	}
}

static void process_end_part(gpointer data, gpointer user_data) {
	rlib *r = user_data;
	struct _packet *packet = data;
	gchar *str;

	if (packet->type == DELAY) {
		str = html_callback(packet->data);
	} else {
		str = ((GString *)packet->data)->str;
	}

	g_string_append(OUTPUT_PRIVATE(r)->whole_report, str);

	if (packet->type == TEXT)
		g_string_free(packet->data, TRUE);
	else {
		/*
		 * packet->data is struct rlib_delayed_extra_data,
		 * it was already freed by html_callback()
		 */
		g_free(str);
	}

	g_free(packet);
}

static void html_end_part(rlib *r, struct rlib_part *part) {
	gint64 i;

	//g_string_append(OUTPUT_PRIVATE(r)->whole_report, "<table><!-- START PART-->\n");

	for (i = 0; i < part->pages_across; i++) {
		GSList *list;

//		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "<!--START PAGE ACROSS--><td valign=\"top\">\n");

		list = g_slist_reverse(OUTPUT_PRIVATE(r)->top[i]);
		g_slist_foreach(list, process_end_part, r);
		g_slist_free(OUTPUT_PRIVATE(r)->top[i]);
		OUTPUT_PRIVATE(r)->top[i] = NULL;

		list = g_slist_reverse(OUTPUT_PRIVATE(r)->bottom[i]);
		g_slist_foreach(list, process_end_part, r);
		g_slist_free(OUTPUT_PRIVATE(r)->bottom[i]);
		OUTPUT_PRIVATE(r)->bottom[i] = NULL;

//		g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</td><!--END PAGE ACROSS-->\n\n");
	}

	//g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</table><!-- END PART-->");
}

static void html_spool_private(rlib *r) {
	ENVIRONMENT(r)->rlib_write_output(OUTPUT_PRIVATE(r)->whole_report->str, OUTPUT_PRIVATE(r)->whole_report->len);
}


static void html_end_page(rlib *r, struct rlib_part *part UNUSED) {
	r->current_line_number = 1;
}

static char *html_get_output(rlib *r) {
	return OUTPUT_PRIVATE(r)->whole_report->str;
}

static gsize html_get_output_length(rlib *r) {
	return OUTPUT_PRIVATE(r)->whole_report->len;
}

static void html_set_working_page(rlib *r, struct rlib_part *part UNUSED, gint64 page) {
	OUTPUT_PRIVATE(r)->page_number = page;
}

static void html_start_part_table(rlib *r, struct rlib_part *part UNUSED) {
	print_text(r, "<table><!--start from part table-->", FALSE);
}

static void html_end_part_table(rlib *r, struct rlib_part *part UNUSED) {
	print_text(r, "</table><!--ended from part table-->", FALSE);
}


static void html_start_part_tr(rlib *r, struct rlib_part *part UNUSED) {
	print_text(r, "<tr><!--start from part tr-->", FALSE);
}

static void html_end_part_tr(rlib *r, struct rlib_part *part UNUSED) {
	print_text(r, "</tr><!--ended from part tr-->", FALSE);
}

static void html_start_part_td(rlib *r, struct rlib_part *part UNUSED, gdouble width UNUSED, gdouble height UNUSED) {
	print_text(r, "<td><!--started from part td-->", FALSE);
}

static void html_end_part_td(rlib *r, struct rlib_part *part UNUSED) {
	print_text(r, "</td><!--ended from part td-->", FALSE);
}

static void html_start_report_line(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}

static void html_end_report_line(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}

static void html_start_line(rlib *r, gint64 backwards) {
	print_text(r, "<div><pre style=\"font-size: 1pt\">",  backwards);
}

static void html_end_line(rlib *r, gint64 backwards) {
	print_text(r, "</pre></div>\n", backwards);	
}

static void html_start_part_pages_across(rlib *r, struct rlib_part *part UNUSED, gdouble left_margin UNUSED, gdouble top_margin UNUSED, gint64 width UNUSED, gint64 height UNUSED, gint64 border_width UNUSED, struct rlib_rgb *color UNUSED) {
	print_text(r, "<!--start pages across-->", FALSE);
}


//SOMETHING NEEDS TO HAPPEN HERE.. probably
static void html_end_part_pages_across(rlib *r, struct rlib_part *part UNUSED) {
	print_text(r, "<!--end pages across-->", FALSE);
}


static void html_start_bold(rlib *r UNUSED) {}
static void html_end_bold(rlib *r UNUSED) {}
static void html_start_italics(rlib *r UNUSED) {}
static void html_end_italics(rlib *r UNUSED) {}

static void html_graph_draw_line(rlib *r UNUSED, gdouble x UNUSED, gdouble y UNUSED, gdouble new_x UNUSED, gdouble new_y UNUSED, struct rlib_rgb *color UNUSED) {}

static void html_graph_init(rlib *r) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	memset(graph, 0, sizeof(struct _graph));
}

static void html_graph_get_chart_layout(rlib *r, gdouble top UNUSED, gdouble bottom UNUSED, gint64 cell_height, gint64 rows, gint64 *chart_size UNUSED, gint64 *chart_height) {
	// don't do anything with chart_size
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint64 height_offset = 1;
	gint64 intersection = 5;
	gint64 title_height = gd_get_string_height(graph->bold_titles);

	height_offset += (BOTTOM_PAD + title_height + intersection + graph->x_axis_label_height);

	if (graph->vertical_x_label)
		height_offset += graph->x_label_width + gd_get_string_height(FALSE) + BOTTOM_PAD;
	else {
		if (graph->x_label_width != 0) {
			height_offset += gd_get_string_height(FALSE) + BOTTOM_PAD;
		}
	}

	*chart_height = height_offset + rows * cell_height;
}

static void html_start_graph(rlib *r, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, gdouble left UNUSED, gdouble top UNUSED, gdouble width, gdouble height, gboolean x_axis_labels_are_under_tick) {
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

static void html_graph_set_limits(rlib *r, gchar side UNUSED, gdouble min, gdouble max, gdouble origin) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->y_min = min;
	graph->y_max = max;
	graph->y_origin = origin;
}

static void html_graph_set_title(rlib *r, gchar *title) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gdouble title_width = gd_get_string_width(title, graph->bold_titles);
	graph->title_height = gd_get_string_height(graph->bold_titles);
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

static void html_graph_set_legend_orientation(rlib *r, gint64 orientation) {
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

static void html_graph_set_is_chart(rlib *r, gboolean is_chart) {
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
		gdouble title_width = gd_get_string_width(title, graph->bold_titles);
		graph->x_axis_label_height += gd_get_string_height(graph->bold_titles);
		rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, title,  (graph->width-title_width)/2.0, graph->whole_graph_height-graph->x_axis_label_height, FALSE, graph->bold_titles);
		graph->x_axis_label_height += gd_get_string_height(graph->bold_titles) * 0.25;

	}
}

static void html_graph_y_axis_title(rlib *r, gchar side, gchar *title) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gdouble title_width = gd_get_string_width(title, graph->bold_titles);
	if(title[0] == 0) {

	} else {
		if(side == RLIB_SIDE_LEFT) {
			rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, title,  0, (graph->whole_graph_height-graph->legend_height)  -((graph->whole_graph_height - graph->legend_height - title_width)/2.0), TRUE, graph->bold_titles);
			graph->y_axis_title_left = gd_get_string_height(graph->bold_titles);
		} else {
			graph->y_axis_title_right = gd_get_string_height(graph->bold_titles);
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

static void html_graph_label_x_get_variables(rlib *r, gint64 iteration, gchar *label, gint64 *left, gint64 *y_start, gint64 string_width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	*left = graph->x_start + (graph->x_tick_width * iteration);
	*y_start = graph->y_start;

	if(string_width == 0)
		string_width = gd_get_string_width(label, FALSE);

	if(graph->vertical_x_label) {
		*y_start += graph->x_label_width;
	} else {
		*left += (graph->x_tick_width - string_width) / 2;
		*y_start += (BOTTOM_PAD/2);
	}

	if (graph->is_chart)
		if (graph->vertical_x_label)
			*left += gd_get_string_height(FALSE) / 2;

	if(graph->x_axis_labels_are_under_tick) {
		if(graph->vertical_x_label) {
			*left -= gd_get_string_height(FALSE) / 2;
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
			graph->x_tick_width = (gdouble)graph->width/((gdouble)graph->x_iterations-1.0);
	}
	else
		graph->x_tick_width = (gdouble)graph->width/(gdouble)graph->x_iterations;
}

static void html_graph_do_grid(rlib *r, gboolean just_a_box) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint64 i;
	gint64 last_left = 0 - graph->x_label_width;

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
				graph->x_tick_width = (gdouble)graph->width/((gdouble)graph->x_iterations-1.0);
		} else
			graph->x_tick_width = (gdouble)graph->width/(gdouble)graph->x_iterations;
	}

	graph->x_start = graph->y_axis_title_left + graph->intersection;
	graph->y_start = graph->whole_graph_height - graph->x_axis_label_height;


	graph->x_start += graph->y_label_width_left;

	for(i=0;i<graph->x_iterations;i++) {
		gint64 left,y_start,string_width=graph->x_label_width;
		if(graph->minor_ticks[i] == FALSE) {
			html_graph_label_x_get_variables(r, i, NULL, &left, &y_start, string_width);
			if(left < (last_left+graph->x_label_width+gd_get_string_width("W", FALSE))) {
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
			graph->height -= graph->x_label_width + gd_get_string_height(FALSE)+BOTTOM_PAD;
			graph->y_start -= gd_get_string_height(FALSE)+BOTTOM_PAD;
		}
		else {
			if (graph->x_label_width != 0) {
				graph->y_start -= gd_get_string_height(FALSE)+BOTTOM_PAD;
				graph->height -= gd_get_string_height(FALSE)+BOTTOM_PAD;
			}
		}
	}
	else
	if(graph->vertical_x_label) {
		graph->vertical_x_label = TRUE;
		graph->y_start -= graph->x_label_width;
		graph->height -= graph->x_label_width;
		graph->y_start -= gd_get_string_width("w", FALSE);
	} else {
		graph->vertical_x_label = FALSE;
		if(graph->x_label_width != 0) {
			graph->y_start -= gd_get_string_height(FALSE)+BOTTOM_PAD;
			graph->height -= gd_get_string_height(FALSE)+BOTTOM_PAD;
		}
	}

	graph->just_a_box = just_a_box;

	if(!just_a_box) {
		rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, graph->x_start, graph->y_start, graph->x_start + graph->width, graph->y_start, graph->grid_color);
	}

}

static void html_graph_tick_x(rlib *r) {
	gint64 i;
	gint64 spot;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint64 divisor = 1;
	gint64 iterations = graph->x_iterations;


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

static void html_graph_set_x_iterations(rlib *r, gint64 iterations) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->x_iterations = iterations;
}

static void html_graph_hint_label_x(rlib *r, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint64 string_width = gd_get_string_width(label, FALSE);

	if(string_width > graph->x_label_width) {
		graph->x_label_width = string_width;
	}
}

static void html_graph_label_x(rlib *r, gint64 iteration, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint64 left, y_start;
	gboolean doit = TRUE;
	gint64 height = gd_get_string_height(FALSE);

	html_graph_label_x_get_variables(r, iteration, label, &left, &y_start, 0);


	if(graph->vertical_x_label) {
		if(graph->last_left_x_label + height > left)
			doit = FALSE;
		else
			graph->last_left_x_label = left;
	}


	if(doit) {
		if (graph->is_chart) {
			gint64 text_height;
			if (graph->vertical_x_label) {
				text_height = gd_get_string_width("w", FALSE);// graph->x_label_width;
				rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label, left, graph->y_start - (graph->height + text_height), graph->vertical_x_label, FALSE);
				//rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label, left, graph->y_start - (graph->height + text_height), graph->vertical_x_label, FALSE);
			}
			else {
				text_height = gd_get_string_height(FALSE);// / 3.0;
				rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label, left, graph->y_start - (graph->height + text_height), graph->vertical_x_label, FALSE);
			}
			//rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label, left, graph->top + text_height, graph->vertical_x_label, FALSE);
		}
		else
			rlib_gd_text(OUTPUT_PRIVATE(r)->rgd, label, left, y_start, graph->vertical_x_label, FALSE);
	}
}

static void html_graph_tick_y(rlib *r, gint64 iterations) {
	gint64 i;
	gint64 y = 0;
	gint64 extra_width = 0;
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
		gint64 xstart;

		if(graph->x_axis_labels_are_under_tick)
			xstart = graph->x_start + ((graph->x_tick_width)*(graph->x_iterations-1));
		else
			xstart = graph->x_start + ((graph->x_tick_width)*(graph->x_iterations));

		rlib_gd_line(OUTPUT_PRIVATE(r)->rgd, xstart, graph->y_start,xstart, graph->y_start - graph->height, graph->grid_color);
	}

}

static void html_graph_label_y(rlib *r, gchar side, gint64 iteration, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gdouble white_space = graph->height/graph->y_iterations;
	gdouble line_width = gd_get_string_height(FALSE) / 3.0;
	gdouble top;

	if (graph->is_chart) {
		gdouble l = white_space / 2 + (line_width / 1.5);
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
	gdouble width =  gd_get_string_width(label, FALSE);
	if(side == RLIB_SIDE_LEFT) {
		if(width > graph->y_label_width_left)
			graph->y_label_width_left = width;
	} else {
		if(width > graph->y_label_width_right)
			graph->y_label_width_right = width;
	}

}

static void html_graph_set_data_plot_count(rlib *r, gint64 count) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->data_plot_count = count;
}

static void html_graph_draw_bar(rlib *r, gint64 row, gint64 start_iteration, gint64 end_iteration, struct rlib_rgb *color, char *label, struct rlib_rgb *label_color, gint64 width_pad, gint64 height_pad) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gdouble bar_length = (end_iteration - start_iteration + 1) * graph->x_tick_width;
	gdouble bar_height = graph->height / graph->y_iterations;
	gdouble x_adjust = width_pad / graph->x_tick_width;
	gdouble y_adjust = height_pad / bar_height;
	gdouble left = graph->x_start + (graph->x_tick_width * (start_iteration - 1)) + (graph->x_tick_width * x_adjust);
	gdouble start = graph->y_start - ((graph->y_iterations - row) * bar_height + bar_height * y_adjust);
	gdouble label_width =  gd_get_string_width(label, FALSE);
	gdouble line_width = gd_get_string_height(FALSE) / 3.0;
	gchar label_text[MAXSTRLEN];
	gint64 i;

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
		label_width =  gd_get_string_width(label_text, FALSE);
	}

	if (label_width > 0) {
		gdouble text_left = left + bar_length / 2 - label_width / 2;
		gdouble text_top = start - (bar_height / 2 + line_width * 3 / 2);
		//OUTPUT(r)->set_fg_color(r, label_color->r, label_color->g, label_color->b);
		rlib_gd_color_text(OUTPUT_PRIVATE(r)->rgd, label_text,  text_left, text_top, FALSE, FALSE, label_color);
	}

	//OUTPUT(r)->set_bg_color(r, 0, 0, 0);
}

static void html_graph_plot_bar(rlib *r, gchar side UNUSED, gint64 iteration, gint64 plot, gdouble height_percent, struct rlib_rgb *color, gdouble last_height, gboolean divide_iterations, gdouble raw_data UNUSED, gchar *label UNUSED) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gdouble bar_width = graph->x_tick_width *.6;
	gdouble left = graph->x_start + (graph->x_tick_width * iteration) + (graph->x_tick_width *.2);
	gdouble start = graph->y_start;

	if(graph->y_origin != graph->y_min)  {
		gdouble n = fabs(graph->y_max)+fabs(graph->y_origin);
		gdouble d = fabs(graph->y_min)+fabs(graph->y_max);
		gdouble real_height =  1 - (n / d);
		start -= (real_height * graph->height);
	}

	if(divide_iterations)
		bar_width /= graph->data_plot_count;

	left += (bar_width)*plot;

	rlib_gd_rectangle(OUTPUT_PRIVATE(r)->rgd, left, start - last_height*graph->height, bar_width, graph->height*(height_percent), color);

}

static void html_graph_plot_line(rlib *r, gchar side UNUSED, gint64 iteration, gdouble p1_height, gdouble p1_last_height, gdouble p2_height, gdouble p2_last_height, struct rlib_rgb * color, gdouble raw_data UNUSED, gchar *label UNUSED, gint64 row_count) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gdouble p1_start = graph->y_start;
	gdouble p2_start = graph->y_start;
	gdouble left = graph->x_start + (graph->x_tick_width * (iteration-1));
	if(row_count <= 0) 
		return;
	p1_height += p1_last_height;
	p2_height += p2_last_height;

	if(graph->y_origin != graph->y_min)  {
		gdouble n = fabs(graph->y_max)+fabs(graph->y_origin);
		gdouble d = fabs(graph->y_min)+fabs(graph->y_max);
		gdouble real_height =  1 - (n / d);
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

static void html_graph_plot_pie(rlib *r, gdouble start, gdouble end, gboolean offset, struct rlib_rgb *color, gdouble raw_data UNUSED, gchar *label UNUSED) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gdouble start_angle = 360.0 * start;
	gdouble end_angle = 360.0 * end;
	gdouble x = (graph->width / 2);
	gdouble y = graph->top + ((graph->height-graph->legend_height) / 2);
	gdouble radius = 0;
	gdouble offset_factor = 0;
	gdouble rads;

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
	gdouble width =  gd_get_string_width(label, FALSE) + gd_get_string_width("WWW", FALSE);

	if(width > graph->legend_width)
		graph->legend_width = width;
}

static void html_count_regions(gpointer data, gpointer user_data) {
	struct rlib_graph_region *gr = data;
	rlib *r = user_data;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if(graph->name != NULL) {
		if(strcmp(graph->name, gr->graph_name) == 0) {
			gdouble width =  gd_get_string_width(gr->region_label, FALSE) + gd_get_string_width("WWW", FALSE);

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
			gint64 iteration = graph->orig_data_plot_count + graph->current_region;
			gdouble offset =  (iteration  * gd_get_string_height(FALSE));
			gdouble picoffset = gd_get_string_height(FALSE);
			gdouble textoffset = gd_get_string_height(FALSE)/4;
			gdouble w_width = gd_get_string_width("W", FALSE);
			gdouble line_height = gd_get_string_height(FALSE);
			gdouble left = graph->legend_left + (w_width/2);
			gdouble top = graph->legend_top + offset + picoffset;
			gdouble bottom = top - (line_height*.6);

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
	gint64 left, width, height, top, bottom;

	g_slist_foreach(r->graph_regions, html_count_regions, r);
	graph->orig_data_plot_count = graph->data_plot_count;
	graph->data_plot_count += graph->region_count;

	if(graph->legend_orientation == RLIB_GRAPH_LEGEND_ORIENTATION_RIGHT) {
		left = graph->whole_graph_width - graph->legend_width;
		width = graph->legend_width-1;
		height = (gd_get_string_height(FALSE) * graph->data_plot_count) + (gd_get_string_height(FALSE) / 2);
		top = 0;
		bottom = height;
	} else {
		left = 0;
		width = graph->width;
		height = (gd_get_string_height(FALSE) * graph->data_plot_count) + (gd_get_string_height(FALSE) / 2);
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

static void html_graph_draw_legend_label(rlib *r, gint64 iteration, gchar *label, struct rlib_rgb *color, gboolean is_line) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gdouble offset =  (iteration  * gd_get_string_height(FALSE));
	gdouble picoffset = gd_get_string_height(FALSE);
	gdouble textoffset = gd_get_string_height(FALSE)/4;
	gdouble w_width = gd_get_string_width("W", FALSE);
	gdouble line_height = gd_get_string_height(FALSE);
	gdouble left = graph->legend_left + (w_width/2);
	gdouble top = graph->legend_top + offset + picoffset;
	gdouble bottom = top - (line_height*.6);

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

static void html_end_graph(rlib *r, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {
	rlib_gd_spool(r, OUTPUT_PRIVATE(r)->rgd);
	rlib_gd_free(OUTPUT_PRIVATE(r)->rgd);
}

static void html_init_end_page(rlib *r UNUSED) {}
static void html_end_rlib_report(rlib *r UNUSED) {}

static void html_finalize_private(rlib *r) {
	g_string_append(OUTPUT_PRIVATE(r)->whole_report, "</body></html>\n");
}

static void html_start_output_section(rlib *r UNUSED, struct rlib_report_output_array *roa UNUSED) {}
static void html_end_output_section(rlib *r UNUSED, struct rlib_report_output_array *roa UNUSED) {}
static void html_start_evil_csv(rlib *r UNUSED) {}
static void html_end_evil_csv(rlib *r UNUSED) {}
static void html_set_raw_page(rlib *r UNUSED, struct rlib_part *part UNUSED, gint64 page UNUSED) {}
static void html_start_part_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void html_end_part_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void html_start_part_page_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void html_end_part_page_header(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void html_start_part_page_footer(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void html_end_part_page_footer(rlib *r UNUSED, struct rlib_part *part UNUSED) {}
static void html_start_report_page_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_end_report_page_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_start_report_break_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void html_end_report_break_header(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void html_start_report_break_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void html_end_report_break_footer(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED, struct rlib_report_break *rb UNUSED) {}
static void html_start_report_no_data(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}
static void html_end_report_no_data(rlib *r UNUSED, struct rlib_part *part UNUSED, struct rlib_report *report UNUSED) {}

static void html_free(rlib *r) {
	g_string_free(OUTPUT_PRIVATE(r)->whole_report, TRUE);
	g_free(OUTPUT_PRIVATE(r)->top);
	g_free(OUTPUT_PRIVATE(r)->bottom);
	g_free(OUTPUT_PRIVATE(r));
	g_free(OUTPUT(r));
}

void rlib_html_new_output_filter(rlib *r) {
	OUTPUT(r) = g_malloc0(sizeof(struct output_filter));
	r->o->private = g_malloc0(sizeof(struct _private));

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
	OUTPUT(r)->finalize_text_delayed = html_finalize_text_delayed;
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
