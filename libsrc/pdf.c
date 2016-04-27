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
#include <locale.h>
#include <string.h>
#include <math.h>


#include "config.h"
#include "rlib.h"
#include "rpdf.h"

#define MAX_PDF_PAGES 500

#define BOLD 1
#define ITALICS 2

#define PDF_PIXEL 0.002

#define PLOT_LINE_WIDTH 1.2
#define GRID_LINE_WIDTH 0.5

const gchar *font_names[4] = { "Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique" };

struct _graph {
	gdouble y_min[2];
	gdouble y_max[2];
	gdouble y_origin[2];
	gdouble non_standard_x_start;
	gboolean has_legend_bg_color;
	struct rlib_rgb legend_bg_color;
	gboolean has_grid_color;
	struct rlib_rgb grid_color;

	gint legend_orientation;
	gfloat y_label_space_left;
	gfloat y_label_space_right;
	gfloat y_label_width_left;
	gfloat y_label_width_right;
	gfloat x_label_width;
	gfloat x_tick_width;
	gfloat legend_width;
	gfloat legend_height;
	gfloat top;
	gfloat bottom;
	gfloat left;
	gfloat width_before_legend;
	gfloat width;
	gfloat height;
	gfloat x_start;
	gfloat x_width;
	gfloat y_start;
	gfloat y_height;
	gfloat intersection;
	gfloat height_offset;
	gfloat width_offset;
	gfloat title_height;
	gint x_iterations;
	gint y_iterations;
	gint data_plot_count;
	gint orig_data_plot_count;
	gboolean x_axis_labels_are_under_tick;
	gboolean is_chart;
	
	gfloat legend_top;
	gfloat legend_left;
	gboolean draw_x;
	gboolean draw_y;
	gchar *name;
	gint region_count;
	gint current_region;
	gboolean bold_titles;
	gboolean *minor_ticks;
	gboolean vertical_x_label;
	gfloat last_left_x_label;
};

struct _private {
	struct rlib_rgb current_color;
	struct rpdf *pdf;
	gchar text_on[MAX_PDF_PAGES];
	gchar *buffer;
	gint length;
	gint page_diff;
	gint current_page;
	gboolean is_bold;
	gboolean is_italics;
	struct _graph graph;
};

static void pdf_graph_get_x_label_width(rlib *r, gfloat *width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	*width = graph->x_label_width;
}

static void pdf_graph_set_x_label_width(rlib *r, gfloat width, gint cell_width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if (width == 0) {
		graph->x_label_width = cell_width / RLIB_PDF_DPI;
	}
	else
		graph->x_label_width = width;
	
	if ((float)RLIB_PDF_DPI * width >= cell_width - 2)
		graph->vertical_x_label = TRUE;
}

static void pdf_graph_get_y_label_width(rlib *r, gfloat *width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	*width = graph->y_label_width_left;
}

static void pdf_graph_set_y_label_width(rlib *r, gfloat width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if (width == 0)
		graph->y_label_width_left = rpdf_text_width(OUTPUT_PRIVATE(r)->pdf, "W")/(RLIB_PDF_DPI);
	else	
		graph->y_label_width_left = width;
}

static void pdf_graph_get_width_offset(rlib *r, gint *width_offset) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	*width_offset = graph->y_label_width_left * RLIB_PDF_DPI + .1 * RLIB_PDF_DPI;
}

static gfloat pdf_get_string_width(rlib *r, const gchar *text) {
	return rpdf_text_width(OUTPUT_PRIVATE(r)->pdf, text)/(RLIB_PDF_DPI);
}

static void pdf_print_text(rlib *r, gfloat left_origin, gfloat bottom_origin, const gchar *text, gfloat orientation) {
	struct rpdf *pdf = OUTPUT_PRIVATE(r)->pdf;

	rpdf_text(pdf, left_origin, bottom_origin, orientation, text);
}

static void pdf_rpdf_callback(gchar *data, gint len, void *user_data) {
	struct rlib_delayed_extra_data *delayed_data = user_data;
	struct rlib_line_extra_data *extra_data = &delayed_data->extra_data;
	rlib *r = delayed_data->r;
	gchar *buf = NULL, *buf2 = NULL;
	
	rlib_execute_pcode(r, &extra_data->rval_code, extra_data->field_code, NULL);	
	rlib_format_string(r, &buf, extra_data->report_field, &extra_data->rval_code);
	rlib_align_text(r, &buf2, buf, extra_data->report_field->align, extra_data->report_field->width);
	memcpy(data, buf2, len);
	g_free(buf);
	g_free(buf2);
	data[len-1] = 0;
	g_free(user_data);
}

static void pdf_print_text_delayed(rlib *r, struct rlib_delayed_extra_data *delayed_data, int backwards, int rval_type) {
	struct rpdf *pdf = OUTPUT_PRIVATE(r)->pdf;
	rpdf_text_callback(pdf, delayed_data->left_origin, delayed_data->bottom_orgin, 0, delayed_data->extra_data.width, pdf_rpdf_callback, delayed_data);
}

static void pdf_print_text_API(rlib *r, gfloat left_origin, gfloat bottom_origin, const gchar *text, gint backwards, struct rlib_line_extra_data *extra_data) {
	pdf_print_text(r, left_origin, bottom_origin, text, 0); 
}

static void pdf_set_fg_color(rlib *r, gfloat red, gfloat green, gfloat blue) {
	if(OUTPUT_PRIVATE(r)->current_color.r != red || OUTPUT_PRIVATE(r)->current_color.g != green 
	|| OUTPUT_PRIVATE(r)->current_color.b != blue) {
		if(red != -1 && green != -1 && blue != -1 )
			rpdf_setrgbcolor(OUTPUT_PRIVATE(r)->pdf, red, green, blue);
		OUTPUT_PRIVATE(r)->current_color.r = red;
		OUTPUT_PRIVATE(r)->current_color.g = green;
		OUTPUT_PRIVATE(r)->current_color.b = blue;	
	}
}

static void pdf_drawbox(rlib *r, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall, struct rlib_rgb *color) {
	if(!(color->r == 1.0 && color->g == 1.0 && color->b == 1.0)) {
		/* the - PDF_PIXEL seems to get around decimal percision problems.. but should investigate this a bit further */
		OUTPUT(r)->set_bg_color(r, color->r, color->g, color->b);
		rpdf_rect(OUTPUT_PRIVATE(r)->pdf, left_origin, bottom_origin, how_long, how_tall-PDF_PIXEL);
		rpdf_fill(OUTPUT_PRIVATE(r)->pdf);
		OUTPUT(r)->set_bg_color(r, 0, 0, 0);
	}
}

static void pdf_hr(rlib *r, gint backwards, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall, 
struct rlib_rgb *color, gfloat indent, gfloat length) {
	how_tall = how_tall / RLIB_PDF_DPI;
	pdf_drawbox(r, left_origin, bottom_origin, how_long, how_tall, color);
}

static void pdf_start_boxurl(rlib *r, struct rlib_part *part, gfloat left_origin, gfloat bottom_origin, gfloat how_long, gfloat how_tall, gchar *url, gint backwards) {
	if(part->landscape) {
		gfloat new_left = rlib_layout_get_page_width(r, part)-left_origin-how_long;
		gfloat new_bottom = bottom_origin;
		rpdf_link(OUTPUT_PRIVATE(r)->pdf, new_bottom, new_left, new_bottom+how_tall, new_left+how_long, url);
	} else  {
		gfloat new_left = left_origin;
		gfloat new_bottom = bottom_origin;
		rpdf_link(OUTPUT_PRIVATE(r)->pdf, new_left, new_bottom, new_left+how_long, new_bottom+how_tall, url);
	}
}

static void pdf_line_image(rlib *r, gfloat left_origin, gfloat bottom_origin, gchar *nname, gchar *type, gfloat nwidth, 
gfloat nheight) {
	gchar	*type_lower;
	gint realtype = RPDF_IMAGE_JPEG;

	type_lower = g_ascii_strdown(type, -1);
	
	if(strstr(type_lower, "png"))
		realtype = RPDF_IMAGE_PNG;

	g_free(type_lower);

	rpdf_image(OUTPUT_PRIVATE(r)->pdf, left_origin, bottom_origin, nwidth, nheight, realtype, nname);

	OUTPUT(r)->set_bg_color(r, 0, 0, 0);
}

static void pdf_set_font_point_actual(rlib *r, gint point) {
	const char *fontname;
	int which_font = 0;
	gchar *pdffontname;

	pdffontname = g_hash_table_lookup(r->output_parameters, "pdf_fontname");

	if(OUTPUT_PRIVATE(r)->is_bold)
		which_font += BOLD;

	if(OUTPUT_PRIVATE(r)->is_italics)
		which_font += ITALICS;

	fontname = pdffontname ? pdffontname : font_names[which_font];
	
	rpdf_set_font(OUTPUT_PRIVATE(r)->pdf, fontname, "WinAnsiEncoding", point);
}

static void pdf_set_font_point(rlib *r, gint point) {
	if(point == 0)
		point = 8;

	if(r->current_font_point != point) {
		pdf_set_font_point_actual(r, point);
		r->current_font_point = point;
	}
}
	
static void pdf_start_new_page(rlib *r, struct rlib_part *part) {
	gint i=0;
	gint pages_across = part->pages_across;
	gchar paper_type[40];
	r->current_page_number++;
	
	sprintf(paper_type, "0 0 %ld %ld", part->paper->width, part->paper->height);
	for(i=0;i<pages_across;i++) {
		if(part->orientation == RLIB_ORIENTATION_LANDSCAPE) {
			part->position_bottom[i] = (part->paper->width/RLIB_PDF_DPI)-part->bottom_margin;
			rpdf_new_page(OUTPUT_PRIVATE(r)->pdf, part->paper->type, RPDF_LANDSCAPE); 
			rpdf_translate(OUTPUT_PRIVATE(r)->pdf, 0.0, (part->paper->height/RLIB_PDF_DPI));	
		   rpdf_rotate(OUTPUT_PRIVATE(r)->pdf, -90.0);
			part->landscape = 1;
		} else {
			part->position_bottom[i] = (part->paper->height/RLIB_PDF_DPI)-part->bottom_margin;
			rpdf_new_page(OUTPUT_PRIVATE(r)->pdf, part->paper->type, RPDF_PORTRAIT); 
			part->landscape = 0;
		}
	}
}

static void pdf_set_working_page(rlib *r, struct rlib_part *part, gint page) {
	gint pages_across = part->pages_across;
	gint page_number = (r->current_page_number-1) * pages_across;
	
	rpdf_set_page(OUTPUT_PRIVATE(r)->pdf, page_number + page - OUTPUT_PRIVATE(r)->page_diff);

	OUTPUT_PRIVATE(r)->current_page = page_number + page - OUTPUT_PRIVATE(r)->page_diff;
}

static void pdf_set_raw_page(rlib *r, struct rlib_part *part, gint page) {
	OUTPUT_PRIVATE(r)->page_diff = r->current_page_number - page;
}


static void pdf_init_end_page(rlib *r) {
	if(r->start_of_new_report == TRUE) {
		r->start_of_new_report = FALSE;
	}
}

static void pdf_start_rlib_report(rlib *r) {
	struct rpdf *pdf;
	gchar *compress;

	pdf = rpdf_new();
	rpdf_set_title(pdf, "RLIB Report");
	compress = g_hash_table_lookup(r->output_parameters, "compress");
	if(compress != NULL) {
		rpdf_set_compression(pdf, TRUE);
	}

	
	OUTPUT_PRIVATE(r)->pdf = pdf;
}

static void pdf_end_rlib_report(rlib *r) {}

static void pdf_finalize_private(rlib *r) {
	int length;
	rpdf_finalize(OUTPUT_PRIVATE(r)->pdf);
	OUTPUT_PRIVATE(r)->buffer = rpdf_get_buffer(OUTPUT_PRIVATE(r)->pdf, &length);
	OUTPUT_PRIVATE(r)->length = length;
}

static void pdf_spool_private(rlib *r) {
	ENVIRONMENT(r)->rlib_write_output(OUTPUT_PRIVATE(r)->buffer, OUTPUT_PRIVATE(r)->length);
}

static void pdf_end_page(rlib *r, struct rlib_part *part) {
	int i;
	for(i=0;i<part->pages_across;i++)
		pdf_set_working_page(r, part, i);
/*	r->current_page_number++; */
	r->current_line_number = 1;
}

static void pdf_end_page_again(rlib *r, struct rlib_part *part, struct rlib_report *report) {
}

static int pdf_free(rlib *r) {
	rpdf_free(OUTPUT_PRIVATE(r)->pdf);
	g_free(OUTPUT_PRIVATE(r));
	g_free(OUTPUT(r));
	return 0;
}

static char *pdf_get_output(rlib *r) {
	return OUTPUT_PRIVATE(r)->buffer;
}

static long pdf_get_output_length(rlib *r) {
	return OUTPUT_PRIVATE(r)->length;
}

static void pdf_start_part_pages_across(rlib *r, struct rlib_part *part, gfloat left_margin, gfloat bottom_margin, int width, int height, int border_width, struct rlib_rgb *border_color) {
	struct rlib_rgb color;
	gfloat real_width;
	gfloat real_height;
	gfloat box_width = width / RLIB_PDF_DPI / 100;
	gfloat space = part->position_bottom[0] - part->position_top[0];

	if(part->orientation == RLIB_ORIENTATION_LANDSCAPE) {
		real_height = space * ((gfloat)height/100);
	} else {
		real_height = space * ((gfloat)height/100);
	}


	if(part->orientation == RLIB_ORIENTATION_LANDSCAPE) {
		real_width = ((part->paper->height/RLIB_PDF_DPI) - (part->left_margin*2)) * ((gfloat)width/100);
	} else {
		real_width = ((part->paper->width/RLIB_PDF_DPI) - (part->left_margin*2)) * ((gfloat)width/100);
	}
	
	memset(&color, 0, sizeof(struct rlib_rgb));
	
	if(border_color == NULL)
		border_color = &color;

	if(border_width > 0) {
		pdf_drawbox(r, left_margin, bottom_margin - real_height, box_width, real_height, border_color);
		pdf_drawbox(r, left_margin + real_width - (box_width * 2), bottom_margin - real_height, box_width, real_height, border_color);

		pdf_drawbox(r, left_margin, bottom_margin - real_height, real_width, box_width, border_color);
		pdf_drawbox(r, left_margin, bottom_margin, real_width, box_width, border_color);
	}
}

static void pdf_start_bold(rlib *r) {
	OUTPUT_PRIVATE(r)->is_bold = TRUE;
	pdf_set_font_point_actual(r, r->current_font_point);
}

static void pdf_end_bold(rlib *r) {
	OUTPUT_PRIVATE(r)->is_bold = FALSE;
	pdf_set_font_point_actual(r, r->current_font_point);
}

static void pdf_start_italics(rlib *r) {
	OUTPUT_PRIVATE(r)->is_italics = TRUE;
	pdf_set_font_point_actual(r, r->current_font_point);
}

static void pdf_end_italics(rlib *r) {
	OUTPUT_PRIVATE(r)->is_italics = FALSE;
	pdf_set_font_point_actual(r, r->current_font_point);
}

static void pdf_graph_draw_line(rlib *r, gfloat x, gfloat y, gfloat new_x, gfloat new_y, struct rlib_rgb *color) {
	if(isnan(x) || isnan(y) || isnan(new_x) || isnan(new_y)) {
	
	} else {
		rpdf_moveto(OUTPUT_PRIVATE(r)->pdf, x, y);
		rpdf_lineto(OUTPUT_PRIVATE(r)->pdf, new_x, new_y);
		rpdf_closepath(OUTPUT_PRIVATE(r)->pdf); 
		rpdf_stroke(OUTPUT_PRIVATE(r)->pdf); 
	}
}

static void pdf_graph_get_chart_layout(rlib *r, gfloat top, gfloat bottom, gint cell_height, gint rows, gint *chart_size, gint *chart_height) {
	gint rows_available;
	gfloat height = top - bottom;
	gfloat intersection = .1;
	gfloat title_height = RLIB_GET_LINE(r->current_font_point);
	gfloat height_offset = 0;
	gfloat y_height = 0;

	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;

	height -= title_height * 1.1;
	height_offset += intersection;

	if (graph->vertical_x_label == TRUE)
		height -= graph->x_label_width;
	else
		height -= RLIB_GET_LINE(r->current_font_point);
	
	y_height = height - height_offset - intersection;
	rows_available = (y_height * (float)RLIB_PDF_DPI) / cell_height;

	if (rows > rows_available)
		*chart_size = rows_available;
	else
		*chart_size = rows;

	*chart_height = ((top - bottom - y_height) * (float)RLIB_PDF_DPI) + *chart_size * cell_height;
}

static void pdf_graph_init(rlib *r) {
	memset(&OUTPUT_PRIVATE(r)->graph, 0, sizeof(struct _graph));
}

static void pdf_start_graph(rlib *r, struct rlib_part *part, struct rlib_report *report, gfloat left, gfloat top, gfloat width, gfloat height, gboolean x_axis_labels_are_under_tick) {
	memset(&OUTPUT_PRIVATE(r)->graph, 0, sizeof(struct _graph));
	
	width /= RLIB_PDF_DPI;
	height /= RLIB_PDF_DPI;
	
	OUTPUT_PRIVATE(r)->graph.top = top;
	OUTPUT_PRIVATE(r)->graph.bottom = top - height;
	OUTPUT_PRIVATE(r)->graph.left = left;
	OUTPUT_PRIVATE(r)->graph.width = width;
	OUTPUT_PRIVATE(r)->graph.width_before_legend = width;
	OUTPUT_PRIVATE(r)->graph.height = height;
	OUTPUT_PRIVATE(r)->graph.intersection = .1;
	OUTPUT_PRIVATE(r)->graph.x_axis_labels_are_under_tick = x_axis_labels_are_under_tick;
}

static void pdf_graph_set_limits(rlib *r, gchar side, gdouble min, gdouble max, gdouble origin) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->y_min[(gint)side] = min;
	graph->y_max[(gint)side] = max;
	graph->y_origin[(gint)side] = origin;
}

static void pdf_graph_set_title(rlib *r, gchar *title) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat title_width = pdf_get_string_width(r, title);
	graph->title_height = RLIB_GET_LINE(r->current_font_point);
	if(graph->bold_titles)
		rpdf_set_font(OUTPUT_PRIVATE(r)->pdf, font_names[1], "WinAnsiEncoding", r->current_font_point);
	pdf_print_text(r, graph->left + ((graph->width-title_width)/2.0), graph->top-graph->title_height, title, 0);
	if(graph->bold_titles)
		rpdf_set_font(OUTPUT_PRIVATE(r)->pdf, font_names[0], "WinAnsiEncoding", r->current_font_point);
}

static void pdf_graph_set_name(rlib *r, gchar *name) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->name = name;
}

static void pdf_graph_set_legend_bg_color(rlib *r, struct rlib_rgb *rgb) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->legend_bg_color = *rgb;
	graph->has_legend_bg_color = TRUE;
}

static void pdf_graph_set_legend_orientation(rlib *r, gint orientation) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->legend_orientation = orientation;
}

static void pdf_graph_set_grid_color(rlib *r, struct rlib_rgb *rgb) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->grid_color = *rgb;
	graph->has_grid_color = TRUE;
}

static void pdf_graph_set_draw_x_y(rlib *r, gboolean draw_x, gboolean draw_y) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->draw_x = draw_x;
	graph->draw_y = draw_y;
}

static void pdf_graph_set_is_chart(rlib *r, gboolean is_chart) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->is_chart = is_chart;
}

static void pdf_graph_set_bold_titles(rlib *r, gboolean bold_titles) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->bold_titles = bold_titles;
}

static void pdf_graph_set_minor_ticks(rlib *r, gboolean *minor_ticks) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->minor_ticks = minor_ticks;
}

static void pdf_graph_x_axis_title(rlib *r, gchar *title) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->height_offset = 0;
	

	if(graph->bold_titles)
		rpdf_set_font(OUTPUT_PRIVATE(r)->pdf, font_names[1], "WinAnsiEncoding", r->current_font_point);

	if(graph->legend_orientation == RLIB_GRAPH_LEGEND_ORIENTATION_BOTTOM)
		graph->height_offset += graph->legend_height;	
	
	if(title[0] == 0)
		graph->height_offset += RLIB_GET_LINE(r->current_font_point) / 2.0;
	else {
		gfloat title_width = pdf_get_string_width(r, title);
		graph->height_offset += (RLIB_GET_LINE(r->current_font_point) * 2);
		pdf_print_text(r, graph->left + ((graph->width-title_width)/2.0), graph->bottom+graph->height_offset - (RLIB_GET_LINE(r->current_font_point)*1.3), title, 0);
	}

	if(graph->bold_titles)
		rpdf_set_font(OUTPUT_PRIVATE(r)->pdf, font_names[0], "WinAnsiEncoding", r->current_font_point);

}

static void pdf_graph_y_axis_title(rlib *r, gchar side, gchar *title) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat title_width;
	
	if(graph->bold_titles)
		rpdf_set_font(OUTPUT_PRIVATE(r)->pdf, font_names[1], "WinAnsiEncoding", r->current_font_point);

	title_width = pdf_get_string_width(r, title);
	if(title[0] == 0) {
	
	} else {
		if(side == RLIB_SIDE_LEFT) {
			pdf_print_text(r, graph->left+(pdf_get_string_width(r, "W")*1.25), graph->bottom+graph->legend_height+((graph->height - graph->legend_height - title_width)/2.0), title,  90);
			graph->y_label_space_left = pdf_get_string_width(r, "W") * 1.5;
		} else {
			pdf_print_text(r, graph->left+graph->width - (pdf_get_string_width(r, "W")*.3), graph->bottom+graph->legend_height+((graph->height -graph->legend_height- title_width)/2.0), title,  90);
			graph->y_label_space_right = pdf_get_string_width(r, "W") * 1.6;
		}
	}

	if(graph->bold_titles)
		rpdf_set_font(OUTPUT_PRIVATE(r)->pdf, font_names[0], "WinAnsiEncoding", r->current_font_point);

}

static void pdf_draw_regions(gpointer data, gpointer user_data) {
	struct rlib_graph_region *gr = data;
	rlib *r = user_data;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if(graph->name != NULL) {
		if(strcmp(graph->name, gr->graph_name) == 0) {
			OUTPUT(r)->set_bg_color(r, gr->color.r, gr->color.g, gr->color.b);
			rpdf_rect(OUTPUT_PRIVATE(r)->pdf, graph->x_start + (graph->x_width*(gr->start/100.0)), graph->y_start, graph->x_width*((gr->end-gr->start)/100.0), graph->y_height);
			rpdf_fill(OUTPUT_PRIVATE(r)->pdf);
			OUTPUT(r)->set_bg_color(r, 0, 0, 0);	
		}
	}	
}

static void pdf_graph_label_x_get_variables(rlib *r, gint iteration, gchar *label, gfloat *left, gfloat *y_offset, gfloat *rotation, gfloat *string_width) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat white_space = graph->x_tick_width;
	gfloat w_width = pdf_get_string_width(r, "W");

	if(*string_width == 0)
		*string_width = pdf_get_string_width(r, label);
	*rotation = 0;
	*left = graph->x_start + (white_space * iteration);
	*y_offset = RLIB_GET_LINE(r->current_font_point);

	if(graph->vertical_x_label) {
		*y_offset = 0;
		*rotation = -90;
		*left += (white_space - w_width) / 2;
		if(graph->x_axis_labels_are_under_tick)
			*y_offset = *y_offset + graph->intersection / 2;
	} else {
		if(*string_width < white_space)
			*left += (white_space - (*string_width)) / 2;
	}

	if (graph->is_chart)
		if (graph->vertical_x_label)
			*left += RLIB_GET_LINE(r->current_font_point) / 2.0;
	
	if(graph->x_axis_labels_are_under_tick) {
		if(graph->vertical_x_label)
			*left -= white_space / 2.0;
		else {
			if(*string_width > white_space)
				*left -= (*string_width) / 2.0;
			else
				*left -= (white_space) / 2.0;
		}
		*y_offset += graph->intersection / 2;
	} else
		*y_offset /= 1.2;

}

static void pdf_graph_set_x_tick_width(rlib *r) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if(graph->x_axis_labels_are_under_tick)	 {
		if(graph->x_iterations <= 1)
			graph->x_tick_width = 0;
		else
			graph->x_tick_width = graph->x_width/(graph->x_iterations-1);
	}
	else
		graph->x_tick_width = graph->x_width/graph->x_iterations;
}

static void pdf_graph_do_grid(rlib *r, gboolean just_a_box) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint i;
	gfloat last_left = graph->left-graph->x_label_width;
	
	graph->width_offset = graph->y_label_space_left + graph->y_label_width_left +  graph->intersection ;

	pdf_graph_draw_line(r, graph->left, graph->bottom, graph->left, graph->top, NULL);
	pdf_graph_draw_line(r, graph->left+graph->width_before_legend, graph->bottom, graph->left+graph->width_before_legend, graph->top, NULL);
	pdf_graph_draw_line(r, graph->left, graph->bottom, graph->left+graph->width_before_legend, graph->bottom, NULL);
	pdf_graph_draw_line(r, graph->left, graph->top, graph->left+graph->width_before_legend, graph->top, NULL);

	graph->top -= graph->title_height*1.1;
	graph->height -= graph->title_height*1.1;

	graph->height_offset += graph->intersection;
	graph->width -= (graph->y_label_width_right + graph->y_label_space_right);
	graph->x_width = graph->width - graph->width_offset - graph->intersection;

	if(graph->x_axis_labels_are_under_tick)	 {
		if(graph->x_iterations <= 1)
			graph->x_tick_width = 0;
		else
			graph->x_tick_width = graph->x_width/(graph->x_iterations-1);
	}
	else
		graph->x_tick_width = graph->x_width/graph->x_iterations;

	graph->x_start = graph->left+graph->width_offset;

	for(i=0;i<graph->x_iterations;i++) {
		gfloat left,y_offset, rotation, string_width=graph->x_label_width;
		if(graph->minor_ticks[i] == FALSE) {
			pdf_graph_label_x_get_variables(r, i, NULL, &left, &y_offset, &rotation, &string_width); 
			if(left < (last_left+graph->x_label_width+pdf_get_string_width(r, "W"))) {
				if (!graph->is_chart)
					graph->vertical_x_label = TRUE;
				break;
			}
			if(left + graph->x_label_width > graph->left + graph->width_before_legend) {
				if (!graph->is_chart)
					graph->vertical_x_label = TRUE;
				break;	
			}
			last_left = left;
		}
	}

	if (graph->is_chart) {
		//gfloat line_width = RLIB_GET_LINE(r->current_font_point) / 3.0;
		if (graph->vertical_x_label == TRUE)
			graph->height -= graph->x_label_width;
		else
			graph->height -= RLIB_GET_LINE(r->current_font_point);
	}
	else
	/* Make more room for the x axis label is we need to rotate the text */
/*	if(graph->x_label_width > (graph->x_tick_width)) { */
	if(graph->vertical_x_label == TRUE) {
		graph->height_offset += graph->x_label_width - 	RLIB_GET_LINE(r->current_font_point);
	}
	
	if(graph->x_axis_labels_are_under_tick)
		graph->height_offset += graph->intersection;

	graph->y_start = graph->bottom + graph->height_offset;
	graph->y_height = graph->height - graph->height_offset - graph->intersection;

	g_slist_foreach(r->graph_regions, pdf_draw_regions, r);

	if(!just_a_box) {
		if(graph->has_grid_color) 
			OUTPUT(r)->set_bg_color(r, graph->grid_color.r, graph->grid_color.g, graph->grid_color.b);

		pdf_graph_draw_line(r, graph->left+graph->width_offset, graph->bottom+graph->height_offset-graph->intersection, graph->left+graph->width_offset, graph->top-graph->intersection, NULL);
		pdf_graph_draw_line(r, graph->left+graph->width_offset-graph->intersection, graph->bottom+graph->height_offset, graph->left+graph->width-graph->intersection, graph->bottom+graph->height_offset, NULL);

		if(graph->y_label_width_right > 0)
			pdf_graph_draw_line(r, graph->x_start+graph->x_width, graph->bottom+graph->height_offset-graph->intersection, graph->x_start+graph->x_width, graph->top-graph->intersection, NULL);

		if(graph->has_grid_color) 
			OUTPUT(r)->set_bg_color(r, 0, 0, 0);
	}
}

static void pdf_graph_tick_x(rlib *r) {
	gint i;
	gfloat spot;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gint iterations = graph->x_iterations;

	graph->height_offset	+= graph->intersection;
	
	if(graph->has_grid_color) 
		OUTPUT(r)->set_bg_color(r, graph->grid_color.r, graph->grid_color.g, graph->grid_color.b);

	rpdf_set_line_width(OUTPUT_PRIVATE(r)->pdf, GRID_LINE_WIDTH);

	if(!graph->x_axis_labels_are_under_tick)
		iterations++;

	for(i=0;i<iterations;i++) {
		spot = graph->x_start + ((graph->x_tick_width)*i);
		if(graph->minor_ticks[i] == TRUE && i != (iterations-1)) {
			pdf_graph_draw_line(r, spot, graph->y_start-(graph->intersection/2), spot, graph->y_start, NULL);
		} else {
			if(graph->draw_x) {
				if (graph->is_chart)
					pdf_graph_draw_line(r, spot, graph->y_start, spot, graph->y_start + graph->y_height + graph->intersection, NULL);
				else
					pdf_graph_draw_line(r, spot, graph->y_start-(graph->intersection), spot, graph->y_start + graph->y_height, NULL);
			}
			else
				pdf_graph_draw_line(r, spot, graph->y_start-(graph->intersection), spot, graph->y_start, NULL);
		}
	}

	rpdf_set_line_width(OUTPUT_PRIVATE(r)->pdf, 1);

	if(graph->has_grid_color) 
		OUTPUT(r)->set_bg_color(r, 0, 0, 0);

}

static void pdf_graph_set_x_iterations(rlib *r, gint iterations) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->x_iterations = iterations;
}

static void pdf_graph_hint_label_x(rlib *r, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat string_width = pdf_get_string_width(r, label);
	if(string_width > graph->x_label_width)
		graph->x_label_width = string_width;
}

static void pdf_graph_label_x(rlib *r, gint iteration, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat rotation = 0;
	gfloat left = 0;
	gfloat y_offset = 0;
	gfloat string_width = 0;
	gfloat height = RLIB_GET_LINE(r->current_font_point);
	gboolean doit = TRUE;

	pdf_graph_label_x_get_variables(r, iteration, label, &left, &y_offset, &rotation, &string_width);

	if(graph->vertical_x_label) {
		if(graph->last_left_x_label + height > left)
			doit = FALSE;
		else
			graph->last_left_x_label = left;	
	}
	
	if(doit) {
		if (graph->is_chart) {
			float text_height;
			if (graph->vertical_x_label) {
				text_height = RLIB_GET_LINE(r->current_font_point) / 3.0;
				//text_height = graph->x_label_width + RLIB_GET_LINE(r->current_font_point) / 3.0;
				rotation = 90;//rotation;
			}
			else
				text_height = RLIB_GET_LINE(r->current_font_point) / 3.0;
			pdf_print_text(r, left, graph->y_start + graph->y_height + text_height, label, rotation);
			//pdf_print_text(r, left, graph->y_start + ((graph->y_height / graph->y_iterations) * graph->y_iterations) + y_offset - .05, label, rotation);
		}
		else
			pdf_print_text(r, left, graph->y_start - y_offset, label, rotation);
	}
}

static void pdf_graph_tick_y(rlib *r, gint iterations) {
	gfloat i;
	gfloat extra = 0;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->y_iterations = iterations;

	if(graph->y_label_width_right > 0)
		extra += graph->intersection;	

	if(graph->has_grid_color) 
		OUTPUT(r)->set_bg_color(r, graph->grid_color.r, graph->grid_color.g, graph->grid_color.b);

	rpdf_set_line_width(OUTPUT_PRIVATE(r)->pdf, GRID_LINE_WIDTH);
	
	for(i=0;i<iterations+1;i++) {
		gfloat y = graph->y_start + ((graph->y_height/iterations) * i);
		if(graph->draw_y) {
			pdf_graph_draw_line(r, graph->left+graph->width_offset-graph->intersection, y, graph->left+graph->width-graph->intersection+extra, y, NULL);
		} else {
			pdf_graph_draw_line(r, graph->left+graph->width_offset-graph->intersection, y, graph->left+graph->width_offset, y, NULL);
			pdf_graph_draw_line(r, graph->left+graph->width-graph->intersection, y, graph->left+graph->width-graph->intersection+extra, y, NULL);
		}
	}

	rpdf_set_line_width(OUTPUT_PRIVATE(r)->pdf, 1);

	if(graph->has_grid_color) 
		OUTPUT(r)->set_bg_color(r, 0, 0, 0);

}

static void pdf_graph_label_y(rlib *r, gchar side, gint iteration, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat white_space = graph->y_height/graph->y_iterations;
	gfloat line_width = RLIB_GET_LINE(r->current_font_point) / 3.0;

	gfloat top;// = graph->y_start + (white_space * iteration) - line_width;
	if (graph->is_chart) {
		gfloat l = white_space / 2 - (line_width / 1.5);
		top = graph->y_start + (white_space * (iteration - 1)) + l;
	}
	else
		top = graph->y_start + (white_space * iteration) - line_width;

	if(side == RLIB_SIDE_LEFT) {
		pdf_print_text(r, graph->left+graph->y_label_space_left, top, label, 0);
	} else {
		pdf_print_text(r, graph->left+graph->width, top, label, 0);
	}
}

static void pdf_graph_hint_label_y(rlib *r, gchar side, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat width =  pdf_get_string_width(r, label);
	if(side == RLIB_SIDE_LEFT) {
		if(width > graph->y_label_width_left)
			graph->y_label_width_left = width;
	}	else {
		if(width > graph->y_label_width_right)
			graph->y_label_width_right = width;
	}
}

static void pdf_graph_set_data_plot_count(rlib *r, gint count) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	graph->data_plot_count = count;
}

static void pdf_graph_draw_bar(rlib *r, gint row, gint start_iteration, gint end_iteration, struct rlib_rgb *color, char *label, struct rlib_rgb *label_color, gint width_pad, gint height_pad) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;	
	gfloat bar_length = (end_iteration - start_iteration + 1) * graph->x_tick_width;
	gfloat bar_height = graph->y_height / graph->y_iterations;
	gfloat x_adjust = width_pad / RLIB_PDF_DPI / graph->x_tick_width;
	gfloat y_adjust = height_pad / RLIB_PDF_DPI / bar_height;
	gfloat left = graph->x_start + (graph->x_tick_width * (start_iteration - 1)) + (graph->x_tick_width * x_adjust);
	gfloat start = graph->y_start + (graph->y_iterations - row) * bar_height + bar_height * y_adjust;
	gfloat label_width =  pdf_get_string_width(r, label);
	gfloat line_width = RLIB_GET_LINE(r->current_font_point) / 3.0;

	gchar label_text[MAXSTRLEN];
	gint i;

	strcpy(label_text, label);
	i = strlen(label_text);

	bar_length -= (graph->x_tick_width * x_adjust * 2);
	bar_height -= (bar_height * y_adjust * 2);

	OUTPUT(r)->set_bg_color(r, color->r, color->g, color->b);
	rpdf_rect(OUTPUT_PRIVATE(r)->pdf, left, start, bar_length, bar_height);
	rpdf_fill(OUTPUT_PRIVATE(r)->pdf);

	if (width_pad == 0 && height_pad == 0) {
		OUTPUT(r)->set_bg_color(r, 0, 0, 0);
		rpdf_rect(OUTPUT_PRIVATE(r)->pdf, left, start, bar_length, bar_height);
		rpdf_stroke(OUTPUT_PRIVATE(r)->pdf); 
	}

	while (i > 0 && label_width > bar_length) {
		label_text[--i] = '\0';
		label_width = pdf_get_string_width(r, label_text);
	}

	if (label_width > 0) {
		gfloat text_left = left + bar_length / 2 - label_width / 2;
		gfloat text_top = start + bar_height / 2 - line_width * 2 / 3;
		OUTPUT(r)->set_fg_color(r, label_color->r, label_color->g, label_color->b);
		pdf_print_text(r, text_left, text_top, label_text, 0);
	}

	OUTPUT(r)->set_bg_color(r, 0, 0, 0);
}

static void pdf_graph_plot_bar(rlib *r, gchar side, gint iteration, gint plot, gfloat height_percent, struct rlib_rgb *color,gfloat last_height, gboolean divide_iterations, gfloat raw_data, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;	
	gfloat bar_width = graph->x_tick_width *.6;
	gfloat left = graph->x_start + (graph->x_tick_width * iteration) + (graph->x_tick_width *.2);
	gfloat start = graph->y_start;

	if(graph->y_origin != graph->y_min)  {
		gfloat n = fabs(graph->y_max[(gint)side])+fabs(graph->y_origin[(gint)side]);
		gfloat d = fabs(graph->y_min[(gint)side])+fabs(graph->y_max[(gint)side]);
		gfloat real_height =  1 - (n / d);				
		start += (real_height * graph->y_height);
	}
	
	if(divide_iterations)
		bar_width /= graph->data_plot_count;

	left += (bar_width)*plot;	
	bar_width -= (PDF_PIXEL * 4);
	OUTPUT(r)->set_bg_color(r, color->r, color->g, color->b);
	rpdf_rect(OUTPUT_PRIVATE(r)->pdf, left, start + last_height*graph->y_height, bar_width, graph->y_height*(height_percent));
	rpdf_fill(OUTPUT_PRIVATE(r)->pdf);
	OUTPUT(r)->set_bg_color(r, 0, 0, 0);
}

void pdf_graph_plot_line(rlib *r, gchar side, gint iteration, gfloat p1_height, gfloat p1_last_height, gfloat p2_height, gfloat p2_last_height, struct rlib_rgb *color, gfloat raw_data, gchar *label, gint row_count) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat p1_start = graph->y_start;
	gfloat p2_start = graph->y_start;
	gfloat left = graph->x_start + (graph->x_tick_width * (iteration-1));
	gfloat x_tick_width = graph->x_tick_width;
	if(row_count <= 0) 
		return;
	p1_height += p1_last_height;
	p2_height += p2_last_height;

	if(graph->y_origin != graph->y_min)  {
		gfloat n = fabs(graph->y_max[(gint)side])+fabs(graph->y_origin[(gint)side]);
		gfloat d = fabs(graph->y_min[(gint)side])+fabs(graph->y_max[(gint)side]);
		gfloat real_height =  1 - (n / d);				
		p1_start += (real_height * graph->y_height);
		p2_start += (real_height * graph->y_height);
	}
	OUTPUT(r)->set_bg_color(r, color->r, color->g, color->b);
	rpdf_set_line_width(OUTPUT_PRIVATE(r)->pdf, PLOT_LINE_WIDTH);
	pdf_graph_draw_line(r, left, p1_start + (graph->y_height * p1_height), left+x_tick_width, p2_start + (graph->y_height * p2_height), NULL);
	rpdf_set_line_width(OUTPUT_PRIVATE(r)->pdf, 1);
	OUTPUT(r)->set_bg_color(r, 0, 0, 0);
}

static void pdf_graph_plot_pie(rlib *r, gfloat start, gfloat end, gboolean offset, struct rlib_rgb *color, gfloat raw_data, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat start_angle = 360.0 * start;
	gfloat end_angle = 360.0 * end;
	gfloat x = graph->left + (graph->width / 2);
	gfloat y = graph->top - ((graph->height-graph->legend_height) / 2);
	gfloat radius = 0;
	gfloat offset_factor = 0;
	gfloat rads;
	
	if(start == end)
		return;
	
	if(graph->width < (graph->height-graph->legend_height))
		radius = graph->width / 2.2;
	else
		radius = (graph->height-graph->legend_height) / 2.2 ;

	if(offset)
		offset_factor = radius * .1;
	else	
		offset_factor = radius *.01;

	rads = (((start_angle+end_angle)/2.0))*3.1415927/180.0;
	x += offset_factor * cosf(rads);
	y += offset_factor * sinf(rads);
	radius -= offset_factor;

	OUTPUT(r)->set_bg_color(r, color->r, color->g, color->b);
	rpdf_arc(OUTPUT_PRIVATE(r)->pdf, x, y, radius, start_angle, end_angle);
	rpdf_fill(OUTPUT_PRIVATE(r)->pdf);
	rpdf_stroke(OUTPUT_PRIVATE(r)->pdf); 
	OUTPUT(r)->set_bg_color(r, 0, 0, 0);
}

static void pdf_graph_hint_legend(rlib *r, gchar *label) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat width =  pdf_get_string_width(r, label) + pdf_get_string_width(r, "WWW");

	if(width > graph->legend_width)
		graph->legend_width = width;
}

static void pdf_count_regions(gpointer data, gpointer user_data) {
	struct rlib_graph_region *gr = data;
	rlib *r = user_data;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if(graph->name != NULL) {
		if(strcmp(graph->name, gr->graph_name) == 0) {
			gfloat width =  pdf_get_string_width(r, gr->region_label) + pdf_get_string_width(r, "WWW");
		
			if(width > graph->legend_width)
				graph->legend_width = width;

			graph->region_count++;
		}
	}	
}

static void pdf_label_regions(gpointer data, gpointer user_data) {
	struct rlib_graph_region *gr = data;
	rlib *r = user_data;
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	if(graph->name != NULL) {
		if(strcmp(graph->name, gr->graph_name) == 0) {
			gint iteration = graph->orig_data_plot_count + graph->current_region;
			gfloat offset =  ((iteration + 1) * RLIB_GET_LINE(r->current_font_point));
			gfloat w_width = pdf_get_string_width(r, "W");
			gfloat line_height = RLIB_GET_LINE(r->current_font_point);
			gfloat left = graph->legend_left + (w_width/2);
			gfloat top =  graph->legend_top - offset;
			gfloat bottom =  top + (line_height *.6);

			OUTPUT(r)->set_bg_color(r, gr->color.r, gr->color.g, gr->color.b);
			rpdf_rect(OUTPUT_PRIVATE(r)->pdf, graph->legend_left + (w_width/2), graph->legend_top - offset , w_width, line_height*.6);
			rpdf_fill(OUTPUT_PRIVATE(r)->pdf);
			OUTPUT(r)->set_bg_color(r, 0, 0, 0);

			pdf_graph_draw_line(r, left, bottom, left, top, NULL);
			pdf_graph_draw_line(r, left+w_width, bottom, left+w_width, top, NULL);
			pdf_graph_draw_line(r, left, bottom, left+w_width, bottom, NULL);
			pdf_graph_draw_line(r, left, top, left+w_width, top, NULL);
			pdf_print_text(r, graph->legend_left + (w_width*2), graph->legend_top - offset, gr->region_label, 0);

			graph->current_region++;
		}
	}	
}

static void pdf_graph_draw_legend(rlib *r) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat left, width, height, top, bottom;
	
	g_slist_foreach(r->graph_regions, pdf_count_regions, r);
	graph->orig_data_plot_count = graph->data_plot_count;
	graph->data_plot_count += graph->region_count;

	if(graph->legend_orientation == RLIB_GRAPH_LEGEND_ORIENTATION_RIGHT) {
		left = graph->left+graph->width - graph->legend_width;
		width = graph->legend_width;
		height = (RLIB_GET_LINE(r->current_font_point) * graph->data_plot_count) + (RLIB_GET_LINE(r->current_font_point) / 2);
		top = graph->top;
		bottom = graph->top - height;
	} else {
		left = graph->left;
		width = graph->width;
		height = (RLIB_GET_LINE(r->current_font_point) * graph->data_plot_count) + (RLIB_GET_LINE(r->current_font_point) / 2);
		top = graph->bottom + height;
		bottom = graph->bottom;
	}

	if(graph->has_legend_bg_color) {
		OUTPUT(r)->set_bg_color(r, graph->legend_bg_color.r, graph->legend_bg_color.g, graph->legend_bg_color.b);
		rpdf_rect(OUTPUT_PRIVATE(r)->pdf, left, bottom, width, top-bottom);
		rpdf_fill(OUTPUT_PRIVATE(r)->pdf);		
		OUTPUT(r)->set_bg_color(r, 0, 0, 0);
	}

	pdf_graph_draw_line(r, left, bottom, left, top, NULL);
	pdf_graph_draw_line(r, left+width, bottom, left+width, top, NULL);
	pdf_graph_draw_line(r, left, bottom, left+width, bottom, NULL);
	pdf_graph_draw_line(r, left, top, left+width, top, NULL);
	
	graph->legend_top = top;
	graph->legend_left = left;

	if(graph->legend_orientation == RLIB_GRAPH_LEGEND_ORIENTATION_RIGHT)
		graph->width -= width;
	else
		graph->legend_height = height;
	
	g_slist_foreach(r->graph_regions, pdf_label_regions, r);

}

static void pdf_graph_draw_legend_label(rlib *r, gint iteration, gchar *label, struct rlib_rgb *color, gboolean line) {
	struct _graph *graph = &OUTPUT_PRIVATE(r)->graph;
	gfloat offset =  ((iteration + 1) * RLIB_GET_LINE(r->current_font_point) );
	gfloat w_width = pdf_get_string_width(r, "W");
	gfloat line_height = RLIB_GET_LINE(r->current_font_point);
	gfloat left = graph->legend_left + (w_width/2);
	gfloat top =  graph->legend_top - offset;
	gfloat bottom =  top + (line_height *.6);

	if(!line) {
		OUTPUT(r)->set_bg_color(r, color->r, color->g, color->b);
		rpdf_rect(OUTPUT_PRIVATE(r)->pdf, graph->legend_left + (w_width/2), graph->legend_top - offset , w_width, line_height*.6);
		rpdf_fill(OUTPUT_PRIVATE(r)->pdf);
		OUTPUT(r)->set_bg_color(r, 0, 0, 0);

		pdf_graph_draw_line(r, left, bottom, left, top, NULL);
		pdf_graph_draw_line(r, left+w_width, bottom, left+w_width, top, NULL);
		pdf_graph_draw_line(r, left, bottom, left+w_width, bottom, NULL);
		pdf_graph_draw_line(r, left, top, left+w_width, top, NULL);
	} else {
		OUTPUT(r)->set_bg_color(r, color->r, color->g, color->b);
		rpdf_rect(OUTPUT_PRIVATE(r)->pdf, graph->legend_left + (w_width/2), graph->legend_top - offset + (line_height * .2) , w_width, line_height*.2);
		rpdf_fill(OUTPUT_PRIVATE(r)->pdf);
		OUTPUT(r)->set_bg_color(r, 0, 0, 0);	
	}	
	pdf_print_text(r, graph->legend_left + (w_width*2), graph->legend_top - offset, label, 0);
}

static void pdf_end_graph(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_end_part_pages_across(rlib *r, struct rlib_part *part) {}
static void pdf_stub_line(rlib *r, int backwards) {}
static void pdf_start_output_section(rlib *r, struct rlib_report_output_array *roa) {}
static void pdf_end_output_section(rlib *r, struct rlib_report_output_array *roa) {}
static void pdf_start_evil_csv(rlib *r) {}
static void pdf_end_evil_csv(rlib *r) {}
static void pdf_end_boxurl(rlib *r, gint backwards) {}
static void pdf_end_draw_cell_background(rlib *r) {}
static void pdf_start_report(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_end_report(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_start_report_field_headers(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_end_report_field_headers(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_start_report_field_details(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_end_report_field_details(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_start_report_line(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_end_report_line(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_start_report_header(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_end_report_header(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_start_report_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_end_report_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_start_part_header(rlib *r, struct rlib_part *part) {}
static void pdf_end_part_header(rlib *r, struct rlib_part *part) {}
static void pdf_start_part_page_header(rlib *r, struct rlib_part *part) {}
static void pdf_end_part_page_header(rlib *r, struct rlib_part *part) {}
static void pdf_start_part_page_footer(rlib *r, struct rlib_part *part) {}
static void pdf_end_part_page_footer(rlib *r, struct rlib_part *part) {}
static void pdf_start_report_page_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_end_report_page_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_start_report_break_header(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void pdf_end_report_break_header(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void pdf_start_report_break_footer(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void pdf_end_report_break_footer(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_break *rb) {}
static void pdf_start_report_no_data(rlib *r, struct rlib_part *part, struct rlib_report *report) {}
static void pdf_end_report_no_data(rlib *r, struct rlib_part *part, struct rlib_report *report) {}


static void pdf_start_part(rlib *r, struct rlib_part *part) {}
static void pdf_end_part(rlib *r, struct rlib_part *part) {}
static void pdf_start_part_table(rlib *r, struct rlib_part *part) {}
static void pdf_end_part_table(rlib *r, struct rlib_part *part) {}
static void pdf_start_part_tr(rlib *r, struct rlib_part *part) {}
static void pdf_end_part_tr(rlib *r, struct rlib_part *part) {}
static void pdf_start_part_td(rlib *r, struct rlib_part *part, gfloat width, gfloat height) {}
static void pdf_end_part_td(rlib *r, struct rlib_part *part) {}

void rlib_pdf_new_output_filter(rlib *r) {
	OUTPUT(r) = g_malloc(sizeof(struct output_filter));
	r->o->private = g_malloc(sizeof(struct _private));
	memset(OUTPUT_PRIVATE(r), 0, sizeof(struct _private));
	OUTPUT(r)->do_graph = TRUE;
	OUTPUT(r)->do_align = TRUE;
	OUTPUT(r)->do_breaks = TRUE;
	OUTPUT(r)->do_grouptext = TRUE;	
	OUTPUT(r)->paginate = TRUE;
	OUTPUT(r)->trim_links = FALSE;
	OUTPUT(r)->get_string_width = pdf_get_string_width;
	OUTPUT(r)->print_text = pdf_print_text_API;
	OUTPUT(r)->print_text_delayed = pdf_print_text_delayed;
	OUTPUT(r)->set_fg_color = pdf_set_fg_color;
	OUTPUT(r)->set_bg_color = pdf_set_fg_color;
	OUTPUT(r)->start_draw_cell_background = pdf_drawbox;
	OUTPUT(r)->end_draw_cell_background = pdf_end_draw_cell_background;
	OUTPUT(r)->hr = pdf_hr;
	OUTPUT(r)->start_boxurl = pdf_start_boxurl;
	OUTPUT(r)->end_boxurl = pdf_end_boxurl;

	OUTPUT(r)->line_image = pdf_line_image;
	OUTPUT(r)->background_image = pdf_line_image;
	OUTPUT(r)->set_font_point = pdf_set_font_point;
	OUTPUT(r)->start_new_page = pdf_start_new_page;
	OUTPUT(r)->end_page = pdf_end_page;
	OUTPUT(r)->end_page_again = pdf_end_page_again;
	OUTPUT(r)->init_end_page = pdf_init_end_page;
	OUTPUT(r)->start_rlib_report = pdf_start_rlib_report;
	OUTPUT(r)->end_rlib_report = pdf_end_rlib_report;
	OUTPUT(r)->start_report = pdf_start_report;
	OUTPUT(r)->end_report = pdf_end_report;
	OUTPUT(r)->start_report_field_headers = pdf_start_report_field_headers;
	OUTPUT(r)->end_report_field_headers = pdf_end_report_field_headers;
	OUTPUT(r)->start_report_field_details = pdf_start_report_field_details;
	OUTPUT(r)->end_report_field_details = pdf_end_report_field_details;
	OUTPUT(r)->start_report_line = pdf_start_report_line;
	OUTPUT(r)->end_report_line = pdf_end_report_line;
	OUTPUT(r)->start_part = pdf_start_part;
	OUTPUT(r)->end_part = pdf_end_part;	
	OUTPUT(r)->start_report_header = pdf_start_report_header;
	OUTPUT(r)->end_report_header = pdf_end_report_header;
	OUTPUT(r)->start_report_footer = pdf_start_report_footer;
	OUTPUT(r)->end_report_footer = pdf_end_report_footer;
	OUTPUT(r)->start_part_header = pdf_start_part_header;
	OUTPUT(r)->end_part_header = pdf_end_part_header;
	OUTPUT(r)->start_part_page_header = pdf_start_part_page_header;
	OUTPUT(r)->end_part_page_header = pdf_end_part_page_header;
	OUTPUT(r)->start_part_page_footer = pdf_start_part_page_footer;
	OUTPUT(r)->end_part_page_footer = pdf_end_part_page_footer;
	OUTPUT(r)->start_report_page_footer = pdf_start_report_page_footer;
	OUTPUT(r)->end_report_page_footer = pdf_end_report_page_footer;
	OUTPUT(r)->start_report_break_header = pdf_start_report_break_header;
	OUTPUT(r)->end_report_break_header = pdf_end_report_break_header;
	OUTPUT(r)->start_report_break_footer = pdf_start_report_break_footer;
	OUTPUT(r)->end_report_break_footer = pdf_end_report_break_footer;
	OUTPUT(r)->start_report_no_data = pdf_start_report_no_data;
	OUTPUT(r)->end_report_no_data = pdf_end_report_no_data;
	
	OUTPUT(r)->finalize_private = pdf_finalize_private;
	OUTPUT(r)->spool_private = pdf_spool_private;
	OUTPUT(r)->start_line = pdf_stub_line;
	OUTPUT(r)->end_line = pdf_stub_line;
	OUTPUT(r)->set_working_page = pdf_set_working_page;
	OUTPUT(r)->set_raw_page = pdf_set_raw_page;
	OUTPUT(r)->start_output_section = pdf_start_output_section;
	OUTPUT(r)->end_output_section = pdf_end_output_section;
	OUTPUT(r)->start_evil_csv = pdf_start_evil_csv;
	OUTPUT(r)->end_evil_csv = pdf_end_evil_csv;
	OUTPUT(r)->get_output = pdf_get_output;
	OUTPUT(r)->get_output_length = pdf_get_output_length;
	OUTPUT(r)->start_part_table = pdf_start_part_table;
	OUTPUT(r)->end_part_table = pdf_end_part_table;
	OUTPUT(r)->start_part_tr = pdf_start_part_tr;
	OUTPUT(r)->end_part_tr = pdf_end_part_tr;
	OUTPUT(r)->start_part_td = pdf_start_part_td;
	OUTPUT(r)->end_part_td = pdf_end_part_td;
	OUTPUT(r)->start_part_pages_across = pdf_start_part_pages_across;
	OUTPUT(r)->end_part_pages_across = pdf_end_part_pages_across;
	OUTPUT(r)->start_bold = pdf_start_bold;
	OUTPUT(r)->end_bold = pdf_end_bold;
	OUTPUT(r)->start_italics = pdf_start_italics;
	OUTPUT(r)->end_italics = pdf_end_italics;
	
	OUTPUT(r)->graph_init = pdf_graph_init;
	OUTPUT(r)->graph_get_chart_layout = pdf_graph_get_chart_layout;
	OUTPUT(r)->start_graph = pdf_start_graph;
	OUTPUT(r)->graph_set_limits = pdf_graph_set_limits;
	OUTPUT(r)->graph_set_title = pdf_graph_set_title;
	OUTPUT(r)->graph_set_name = pdf_graph_set_name;
	OUTPUT(r)->graph_set_legend_bg_color = pdf_graph_set_legend_bg_color;
	OUTPUT(r)->graph_set_legend_orientation = pdf_graph_set_legend_orientation;
	OUTPUT(r)->graph_set_draw_x_y = pdf_graph_set_draw_x_y;
	OUTPUT(r)->graph_set_is_chart = pdf_graph_set_is_chart;
	OUTPUT(r)->graph_set_bold_titles = pdf_graph_set_bold_titles;
	OUTPUT(r)->graph_set_minor_ticks = pdf_graph_set_minor_ticks;
	OUTPUT(r)->graph_set_grid_color = pdf_graph_set_grid_color;
	OUTPUT(r)->graph_x_axis_title = pdf_graph_x_axis_title;
	OUTPUT(r)->graph_y_axis_title = pdf_graph_y_axis_title;
	OUTPUT(r)->graph_do_grid = pdf_graph_do_grid;
	OUTPUT(r)->graph_tick_x = pdf_graph_tick_x;
	OUTPUT(r)->graph_set_x_iterations = pdf_graph_set_x_iterations;
	OUTPUT(r)->graph_set_x_tick_width = pdf_graph_set_x_tick_width;
	OUTPUT(r)->graph_tick_y = pdf_graph_tick_y;
	OUTPUT(r)->graph_hint_label_x = pdf_graph_hint_label_x;
	OUTPUT(r)->graph_label_x = pdf_graph_label_x;
	OUTPUT(r)->graph_label_y = pdf_graph_label_y;
	OUTPUT(r)->graph_draw_line = pdf_graph_draw_line;
	OUTPUT(r)->graph_draw_bar = pdf_graph_draw_bar;
	OUTPUT(r)->graph_plot_bar = pdf_graph_plot_bar;
	OUTPUT(r)->graph_plot_line = pdf_graph_plot_line;
	OUTPUT(r)->graph_plot_pie = pdf_graph_plot_pie;
	OUTPUT(r)->graph_set_data_plot_count = pdf_graph_set_data_plot_count;
	OUTPUT(r)->graph_hint_label_y = pdf_graph_hint_label_y;
	OUTPUT(r)->graph_hint_legend = pdf_graph_hint_legend;
	OUTPUT(r)->graph_draw_legend = pdf_graph_draw_legend;
	OUTPUT(r)->graph_draw_legend_label = pdf_graph_draw_legend_label;
	OUTPUT(r)->end_graph = pdf_end_graph;

	OUTPUT(r)->graph_set_x_label_width = pdf_graph_set_x_label_width;
	OUTPUT(r)->graph_get_x_label_width = pdf_graph_get_x_label_width;
	OUTPUT(r)->graph_set_y_label_width = pdf_graph_set_y_label_width;
	OUTPUT(r)->graph_get_y_label_width = pdf_graph_get_y_label_width;
	OUTPUT(r)->graph_get_width_offset = pdf_graph_get_width_offset;
	
	OUTPUT(r)->free = pdf_free;
}
