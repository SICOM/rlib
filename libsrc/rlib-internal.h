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
 * $Id$s
 *
 * This module defines constants and structures used by most of the C code
 * modules in the library.
 *
 */
#ifndef _RLIB_INTERNAL_H
#define _RLIB_INTERNAL_H

#include <libxml/parser.h>
#include <time.h>
#include <glib.h>

#include "rlib.h"
#include "charencoder.h"
#include "datetime.h"
#include "util.h"

#define RLIB_DEFUALT_FONTPOINT 	10.0

#define USE_RLIB_VAR	0

#define RLIB_WEB_CONTENT_TYPE_HTML "Content-Type: text/html; charset=%s\n"
#define RLIB_WEB_CONTENT_TYPE_TEXT "Content-Type: text/plain; charset=%s\n"
#define RLIB_WEB_CONTENT_TYPE_PDF "Content-Type: application/pdf\n"
#define RLIB_WEB_CONTENT_TYPE_CSV "Content-type: application/octet-stream\nContent-Disposition: attachment; filename=report.csv\n"
#define RLIB_WEB_CONTENT_TYPE_CSV_FORMATTED "Content-type: application/octet-stream\nContent-Disposition: attachment; filename=%s\n"

#define RLIB_NAVIGATE_FIRST 1
#define RLIB_NAVIGATE_NEXT 2
#define RLIB_NAVIGATE_PREVIOUS 3
#define RLIB_NAVIGATE_LAST 4

#define RLIB_ENCODING "UTF-8"

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define RLIB_MAXIMUM_REPORTS	5

#define RLIB_MAXIMUM_PAGES_ACROSS	100

#define RLIB_ELEMENT_LITERAL 1
#define RLIB_ELEMENT_FIELD   2
#define RLIB_ELEMENT_IMAGE   3
#define RLIB_ELEMENT_REPORT  4
#define RLIB_ELEMENT_PART    5
#define RLIB_ELEMENT_TR      6
#define RLIB_ELEMENT_TD      7
#define RLIB_ELEMENT_LOAD    8
#define RLIB_ELEMENT_BARCODE 9

#define RLIB_FORMAT_PDF 	1
#define RLIB_FORMAT_HTML	2
#define RLIB_FORMAT_TXT 	3
#define RLIB_FORMAT_CSV 	4
#define RLIB_FORMAT_XML 	5

#define RLIB_DEFAULT_BOTTOM_MARGIN .2
#define RLIB_DEFAULT_LEFT_MARGIN	.2
#define RLIB_DEFAULT_TOP_MARGIN 	.2

#define RLIB_FILE_XML_STR		400
#define RLIB_FILE_OUTPUTS		500
#define RLIB_FILE_ROA 			600
#define RLIB_FILE_LINE 			700
#define RLIB_FILE_VARIABLES	800
#define RLIB_FILE_VARIABLE		850
#define RLIB_FILE_BREAKS 		900
#define RLIB_FILE_BREAK 		950
#define RLIB_FILE_BREAK_FIELD	975

#define RLIB_PAPER_LETTER     1
#define RLIB_PAPER_LEGAL      2
#define RLIB_PAPER_A4         3
#define RLIB_PAPER_B5         4
#define RLIB_PAPER_C5         5
#define RLIB_PAPER_DL         6
#define RLIB_PAPER_EXECUTIVE  7
#define RLIB_PAPER_COMM10     8
#define RLIB_PAPER_MONARCH    9
#define RLIB_PAPER_FILM35MM   10

#define RLIB_PDF_DPI 72.0f

#define RLIB_LAYOUT_FIXED 1
#define RLIB_LAYOUT_FLOW  2

#define RLIB_GET_LINE(a) ((float)(a/RLIB_PDF_DPI))

#define RLIB_SIDE_LEFT  0
#define RLIB_SIDE_RIGHT 1

#define RLIB_GRAPH_LEGEND_ORIENTATION_RIGHT  0
#define RLIB_GRAPH_LEGEND_ORIENTATION_BOTTOM 1

struct rlib_paper {
	char type;
	long width;
	long height;
	char name[30];
};

struct rlib_value_stack {
	int count;
	struct rlib_value values[100];
};

struct rlib_element {
	gint type;
	gpointer data;
	struct rlib_element *next;
};

#define RLIB_ALIGN_LEFT 	0
#define RLIB_ALIGN_RIGHT	1
#define RLIB_ALIGN_CENTER	2

struct rlib_from_xml {
	xmlChar *xml;
	gint line;
};

struct rlib_report_literal {
	gchar *value;
	struct rlib_from_xml xml_align;
	struct rlib_from_xml xml_color;
	struct rlib_from_xml xml_bgcolor;
	struct rlib_from_xml xml_width;
	struct rlib_from_xml xml_bold;
	struct rlib_from_xml xml_italics;
	struct rlib_from_xml xml_col;
	struct rlib_from_xml xml_link;
	struct rlib_from_xml xml_translate;

	gint width;
	gint align;

	struct rlib_pcode *color_code;
	struct rlib_pcode *bgcolor_code;
	struct rlib_pcode *col_code;
	struct rlib_pcode *width_code;
	struct rlib_pcode *bold_code;
	struct rlib_pcode *italics_code;
	struct rlib_pcode *align_code;
	struct rlib_pcode *link_code;
	struct rlib_pcode *translate_code;
};

struct rlib_resultset_field {
	gint resultset;
	gpointer field;
};

struct rlib_results {
	gchar *name;
	gpointer result;
	gboolean next_failed;
	gboolean navigation_failed;
	struct input_filter *input;
};

struct rlib_line_extra_data {
	gint type;
	struct rlib_value rval_code;
	struct rlib_value rval_link;
	struct rlib_value rval_translate;
	struct rlib_value rval_bgcolor;
	struct rlib_value rval_color;
	struct rlib_value rval_bold;
	struct rlib_value rval_italics;
	struct rlib_value rval_col;

	struct rlib_value rval_image_name;
	struct rlib_value rval_image_type;
	struct rlib_value rval_image_width;
	struct rlib_value rval_image_height;
	struct rlib_value rval_image_textwidth;

	gint font_point;
	gchar* formatted_string;
	gint width;
	gint col;
	gint delayed;
	struct rlib_rgb bgcolor;
	gint found_bgcolor;
	gchar *link;
	gboolean translate;
	gint found_link;
	gint align;
	struct rlib_rgb color;
	gint found_color;
	gfloat output_width;
	gint running_bgcolor_status;
	gfloat running_bg_total;
	gint running_link_status;
	gfloat running_link_total;
	gboolean is_bold;
	gboolean is_italics;
	gboolean is_memo;
	GSList *memo_lines;
	gint memo_line_count;

	struct rlib_pcode *field_code;
	struct rlib_report_field *report_field;

	gint report_index;
};

struct rlib_delayed_extra_data {
	void *r;
	struct rlib_line_extra_data extra_data;
	gint backwards;
	gfloat left_origin;
	gfloat bottom_orgin;
};

struct rlib_report_field {
	gchar *value;
	gint value_line_number;
	struct rlib_from_xml xml_align;
	struct rlib_from_xml xml_bgcolor;
	struct rlib_from_xml xml_color;
	struct rlib_from_xml xml_width;
	struct rlib_from_xml xml_bold;
	struct rlib_from_xml xml_italics;
	struct rlib_from_xml xml_format;
	struct rlib_from_xml xml_link;
	struct rlib_from_xml xml_translate;
	struct rlib_from_xml xml_col;
	struct rlib_from_xml xml_delayed;
	struct rlib_from_xml xml_memo;
	struct rlib_from_xml xml_memo_max_lines;
	struct rlib_from_xml xml_memo_wrap_chars;

	gint width;
	gint align;

	struct rlib_pcode *code;
	struct rlib_pcode *format_code;
	struct rlib_pcode *link_code;
	struct rlib_pcode *translate_code;
	struct rlib_pcode *color_code;
	struct rlib_pcode *bgcolor_code;
	struct rlib_pcode *col_code;
	struct rlib_pcode *delayed_code;
	struct rlib_pcode *width_code;
	struct rlib_pcode *bold_code;
	struct rlib_pcode *italics_code;
	struct rlib_pcode *align_code;
	struct rlib_pcode *memo_code;
	struct rlib_pcode *memo_max_lines_code;
	struct rlib_pcode *memo_wrap_chars_code;

	struct rlib_value *rval;
};

#define RLIB_REPORT_PRESENTATION_DATA_LINE	1
#define RLIB_REPORT_PRESENTATION_DATA_HR 	2
#define RLIB_REPORT_PRESENTATION_DATA_IMAGE	3

struct rlib_report_output {
	gint type;
	gpointer data;
};

struct rlib_report_output_array {
	gint count;
	struct rlib_from_xml xml_page;
	gint page;
	struct rlib_from_xml xml_suppress;
	gboolean suppress;
	struct rlib_report_output **data;
};

struct rlib_report_horizontal_line {
	struct rlib_from_xml xml_bgcolor;
	struct rlib_from_xml xml_size;
	struct rlib_from_xml xml_indent;
	struct rlib_from_xml xml_length;
	struct rlib_from_xml xml_font_size;
	struct rlib_from_xml xml_suppress;

	gint font_point;
	gfloat size;
	gint indent;
	gint length;

	struct rlib_pcode *bgcolor_code;
	struct rlib_pcode *size_code;
	struct rlib_pcode *indent_code;
	struct rlib_pcode *length_code;
	struct rlib_pcode *font_size_code;
	struct rlib_pcode *suppress_code;
};

struct rlib_report_image {
	struct rlib_from_xml xml_value;
	struct rlib_from_xml xml_type;
	struct rlib_from_xml xml_width;
	struct rlib_from_xml xml_height;
	struct rlib_from_xml xml_textwidth;

	struct rlib_pcode *value_code;
	struct rlib_pcode *type_code;
	struct rlib_pcode *width_code;
	struct rlib_pcode *height_code;
	struct rlib_pcode *textwidth_code;
};

struct rlib_report_barcode {
	struct rlib_from_xml xml_value;
	struct rlib_from_xml xml_type;
	struct rlib_from_xml xml_width;
	struct rlib_from_xml xml_height;

	struct rlib_pcode *value_code;
	struct rlib_pcode *type_code;
	struct rlib_pcode *width_code;
	struct rlib_pcode *height_code;
};

struct rlib_report_lines {
	struct rlib_from_xml xml_bgcolor;
	struct rlib_from_xml xml_color;
	struct rlib_from_xml xml_bold;
	struct rlib_from_xml xml_italics;
	struct rlib_from_xml xml_font_size;
	struct rlib_from_xml xml_suppress;

	gint font_point;

	struct rlib_pcode *bgcolor_code;
	struct rlib_pcode *color_code;
	struct rlib_pcode *suppress_code;
	struct rlib_pcode *font_size_code;
	struct rlib_pcode *bold_code;
	struct rlib_pcode *italics_code;

	struct rlib_element *e;

	gfloat max_line_height;
};

struct rlib_break_fields {
	struct rlib_from_xml xml_value;
	struct rlib_pcode *code;
	struct rlib_value rval2;
	struct rlib_value *rval;
};

struct rlib_report_break {
	struct rlib_from_xml xml_name;
	struct rlib_from_xml xml_newpage;
	struct rlib_from_xml xml_headernewpage;
	struct rlib_from_xml xml_suppressblank;

	gint didheader;
	gint headernewpage;
	gint suppressblank;

	struct rlib_element *header;
	struct rlib_element *fields;
	struct rlib_element *footer;

	struct rlib_pcode *newpage_code;
	struct rlib_pcode *headernewpage_code;
	struct rlib_pcode *suppressblank_code;
};

struct rlib_report_detail {
	struct rlib_element *headers;
	struct rlib_element *fields;
};

struct rlib_report_alternate {
	struct rlib_element *nodata;
};

struct rlib_count_amount {
	struct rlib_value count;
	struct rlib_value amount;
};

#define RLIB_REPORT_VARIABLE_UNDEFINED	-1
#define RLIB_REPORT_VARIABLE_EXPRESSION	1
#define RLIB_REPORT_VARIABLE_COUNT 		2
#define RLIB_REPORT_VARIABLE_SUM	 		3
#define RLIB_REPORT_VARIABLE_AVERAGE 		4
#define RLIB_REPORT_VARIABLE_LOWEST		5
#define RLIB_REPORT_VARIABLE_HIGHEST		6

#define RLIB_VARIABLE_CA(a)	(&(a->data))

struct rlib_report_variable {
	struct rlib_from_xml xml_name;
	struct rlib_from_xml xml_str_type;
	struct rlib_from_xml xml_value;
	struct rlib_from_xml xml_resetonbreak;
	struct rlib_from_xml xml_precalculate;
	struct rlib_from_xml xml_ignore;

	gchar type;
	gchar precalculate;
	struct rlib_pcode *code;
	struct rlib_pcode *ignore_code;
	struct rlib_count_amount data;

	GSList *precalculated_values;
};

struct rlib_part_load {
	struct rlib_from_xml xml_name;
	struct rlib_pcode *name_code;
};

struct rlib_part_td {
	struct rlib_from_xml xml_width;
	struct rlib_from_xml xml_height;
	struct rlib_from_xml xml_border_width;
	struct rlib_from_xml xml_border_color;
	struct rlib_pcode *width_code;
	struct rlib_pcode *height_code;
	struct rlib_pcode *border_width_code;
	struct rlib_pcode *border_color_code;
	GSList *reports;
};

struct rlib_part_tr {
	struct rlib_from_xml xml_layout;
	struct rlib_from_xml xml_newpage;

	struct rlib_pcode *layout_code;
	struct rlib_pcode *newpage_code;
	gchar layout;

	GSList *part_deviations;
};

struct rlib_part {
	struct rlib_from_xml xml_name;
	struct rlib_from_xml xml_pages_across;
	struct rlib_from_xml xml_orientation;
	struct rlib_from_xml xml_top_margin;
	struct rlib_from_xml xml_left_margin;
	struct rlib_from_xml xml_bottom_margin;
	struct rlib_from_xml xml_paper_type;
	struct rlib_from_xml xml_font_size;
	struct rlib_from_xml xml_iterations;
	struct rlib_from_xml xml_suppress_page_header_first_page;
	struct rlib_from_xml xml_suppress;

	GSList *part_rows;
	struct rlib_element *report_header;
	struct rlib_element *page_header;
	struct rlib_element *page_footer;

	struct rlib_pcode *name_code;
	struct rlib_pcode *pages_across_code;
	struct rlib_pcode *orientation_code;
	struct rlib_pcode *top_margin_code;
	struct rlib_pcode *left_margin_code;
	struct rlib_pcode *bottom_margin_code;
	struct rlib_pcode *paper_type_code;
	struct rlib_pcode *font_size_code;
	struct rlib_pcode *iterations_code;
	struct rlib_pcode *suppress_page_header_first_page_code;
	struct rlib_pcode *suppress_code;

	struct rlib_paper *paper;
	gint orientation;
	gint font_size;
	gint pages_across;
	gint iterations;
	gboolean suppress;
	gboolean has_only_one_report;
	struct rlib_report *only_report;
	gfloat *position_top;
	gfloat *position_bottom;
	gfloat *bottom_size;
	gfloat top_margin;
	gfloat bottom_margin;
	gfloat left_margin;
	gint landscape;
	gint suppress_page_header_first_page;
	gint report_index;
};

struct rlib_graph_x_minor_tick {
	gboolean by_name;
	gchar *graph_name;
	gchar *x_value;
	gint location;
};


struct rlib_graph_region {
	gchar *graph_name;
	gchar *region_label;
	struct rlib_rgb color;
	gfloat start;
	gfloat end;
};

struct rlib_graph_plot {
	struct rlib_from_xml xml_axis;
	struct rlib_from_xml xml_field;
	struct rlib_from_xml xml_label;
	struct rlib_from_xml xml_side;
	struct rlib_from_xml xml_disabled;
	struct rlib_from_xml xml_color;
	struct rlib_value rval_axis;
	struct rlib_value rval_field;
	struct rlib_value rval_label;
	struct rlib_value rval_side;
	struct rlib_value rval_disabled;
	struct rlib_value rval_color;
	struct rlib_pcode *axis_code;
	struct rlib_pcode *field_code;
	struct rlib_pcode *label_code;
	struct rlib_pcode *side_code;
	struct rlib_pcode *disabled_code;
	struct rlib_pcode *color_code;
};

struct rlib_graph {
	struct rlib_from_xml xml_name;
	struct rlib_from_xml xml_type;
	struct rlib_from_xml xml_subtype;
	struct rlib_from_xml xml_width;
	struct rlib_from_xml xml_height;
	struct rlib_from_xml xml_title;
	struct rlib_from_xml xml_bold_titles;
	struct rlib_from_xml xml_legend_bg_color;
	struct rlib_from_xml xml_legend_orientation;
	struct rlib_from_xml xml_draw_x_line;
	struct rlib_from_xml xml_draw_y_line;
	struct rlib_from_xml xml_grid_color;
	struct rlib_from_xml xml_x_axis_title;
	struct rlib_from_xml xml_y_axis_title;
	struct rlib_from_xml xml_y_axis_mod;
	struct rlib_from_xml xml_y_axis_title_right;
	struct rlib_from_xml xml_y_axis_decimals;
	struct rlib_from_xml xml_y_axis_decimals_right;
	struct rlib_pcode *name_code;
	struct rlib_pcode *type_code;
	struct rlib_pcode *subtype_code;
	struct rlib_pcode *width_code;
	struct rlib_pcode *height_code;
	struct rlib_pcode *title_code;
	struct rlib_pcode *bold_titles_code;
	struct rlib_pcode *legend_bg_color_code;
	struct rlib_pcode *legend_orientation_code;
	struct rlib_pcode *draw_x_line_code;
	struct rlib_pcode *draw_y_line_code;
	struct rlib_pcode *grid_color_code;
	struct rlib_pcode *x_axis_title_code;
	struct rlib_pcode *y_axis_title_code;
	struct rlib_pcode *y_axis_mod_code;
	struct rlib_pcode *y_axis_title_right_code;
	struct rlib_pcode *y_axis_decimals_code;
	struct rlib_pcode *y_axis_decimals_code_right;
	GSList *plots;
};

struct rlib_chart_row {
	struct rlib_from_xml xml_row;
	struct rlib_from_xml xml_bar_start;
	struct rlib_from_xml xml_bar_end;
	struct rlib_from_xml xml_label;
	struct rlib_from_xml xml_bar_label;
	struct rlib_from_xml xml_bar_color;
	struct rlib_from_xml xml_bar_label_color;

	struct rlib_pcode *row_code;
	struct rlib_pcode *bar_start_code;
	struct rlib_pcode *bar_end_code;
	struct rlib_pcode *label_code;
	struct rlib_pcode *bar_label_code;
	struct rlib_pcode *bar_color_code;
	struct rlib_pcode *bar_label_color_code;
};

struct rlib_chart_header_row {
	struct rlib_from_xml xml_query;
	struct rlib_from_xml xml_field;
	struct rlib_from_xml xml_colspan;

	struct rlib_pcode *query_code;
	struct rlib_pcode *field_code;
	struct rlib_pcode *colspan_code;
};

struct rlib_chart {
	gboolean have_chart;
	struct rlib_from_xml xml_name;
	struct rlib_from_xml xml_title;
	struct rlib_from_xml xml_cols;
	struct rlib_from_xml xml_rows;
	struct rlib_from_xml xml_cell_width;
	struct rlib_from_xml xml_cell_height;
	struct rlib_from_xml xml_cell_width_padding;
	struct rlib_from_xml xml_cell_height_padding;
	struct rlib_from_xml xml_label_width;
	struct rlib_from_xml xml_header_row;

	struct rlib_pcode *name_code;
	struct rlib_pcode *title_code;
	struct rlib_pcode *cols_code;
	struct rlib_pcode *rows_code;
	struct rlib_pcode *cell_width_code;
	struct rlib_pcode *cell_height_code;
	struct rlib_pcode *cell_width_padding_code;
	struct rlib_pcode *cell_height_padding_code;
	struct rlib_pcode *label_width_code;
	struct rlib_pcode *header_row_code;

	struct rlib_chart_header_row header_row;
	struct rlib_chart_row row;
};

struct rlib_report {
	xmlDocPtr doc;
	gchar *contents;
	struct rlib_from_xml xml_font_size;
	struct rlib_from_xml xml_query;
	struct rlib_from_xml xml_orientation;
	struct rlib_from_xml xml_top_margin;
	struct rlib_from_xml xml_left_margin;
	struct rlib_from_xml xml_bottom_margin;
	struct rlib_from_xml xml_pages_across;
	struct rlib_from_xml xml_detail_columns;
	struct rlib_from_xml xml_column_pad;
	struct rlib_from_xml xml_suppress_page_header_first_page;
	struct rlib_from_xml xml_suppress;
	struct rlib_from_xml xml_height;
	struct rlib_from_xml xml_iterations;
	struct rlib_from_xml xml_uniquerow;

	gfloat *position_top;
	gfloat *position_bottom;
	gfloat *bottom_size;

	gint main_loop_query;
	gint raw_page_number;

	gint orientation;
	gint font_size;
	gint detail_columns;
	gfloat column_pad;
	gfloat top_margin;
	gfloat bottom_margin;
	gfloat left_margin;
	gfloat page_width;
	gint iterations;
	gint pages_across;
	gboolean suppress_page_header_first_page;
	gboolean suppress;
	gboolean is_the_only_report;
	struct rlib_pcode *iterations_code;
	struct rlib_value uniquerow;

	struct rlib_element *report_header;
	struct rlib_element *page_header;
	struct rlib_report_detail detail;
	struct rlib_element *page_footer;
	struct rlib_element *report_footer;
	struct rlib_element *variables;

	struct rlib_element *breaks;
	struct rlib_report_alternate alternate;
	struct rlib_graph graph;
	struct rlib_chart chart;
	gint mainloop_query;

	struct rlib_pcode *font_size_code;
	struct rlib_pcode *query_code;
	struct rlib_pcode *orientation_code;
	struct rlib_pcode *detail_columns_code;
	struct rlib_pcode *column_pad_code;
	struct rlib_pcode *height_code;
	struct rlib_pcode *top_margin_code;
	struct rlib_pcode *left_margin_code;
	struct rlib_pcode *bottom_margin_code;
	struct rlib_pcode *pages_across_code;
	struct rlib_pcode *suppress_page_header_first_page_code;
	struct rlib_pcode *suppress_code;
	struct rlib_pcode *uniquerow_code;

};

struct rlib_queries {
	gchar *sql;
	gchar *name;
	struct input_filter *input;
};

#define RLIB_REPORT_TYPE_FILE 1
#define RLIB_REPORT_TYPE_BUFFER 2

struct rlib_rip_reports {
	gchar *name;
	gchar *dir;
	gchar type;
};

#define MAX_INPUT_FILTERS	10

struct input_filters {
	gchar *name;
	gpointer handle;
	struct input_filter *input;
};

#define RLIB_MAXIMUM_FOLLOWERS	10
struct rlib_resultset_followers {
	gint leader;
	gint follower;
	gchar *leader_field;
	gchar *follower_field;
	struct rlib_pcode *leader_code;
	struct rlib_pcode *follower_code;
};

struct rlib;
typedef struct rlib rlib;
struct rlib_signal_functions {
	gboolean (*signal_function)(rlib *, gpointer);
	gpointer data;
};

struct rlib_metadata {
	struct rlib_from_xml xml_formula;
	struct rlib_value rval_formula;
	struct rlib_pcode *formula_code;
};

struct rlib {
	gint current_page_number;
	gint current_line_number;
	gint detail_line_count;
	gint start_of_new_report;

	gint font_point;

	gint current_font_point;

	GHashTable *parameters;
	GHashTable *output_parameters;
	GHashTable *input_metadata;
	GSList *pcode_functions;

	GIConv output_encoder;
	gchar *output_encoder_name;

	time_t now; /* set when rlib starts now will then be a constant over the time of the report */

	struct rlib_signal_functions signal_functions[RLIB_SIGNALS];

	struct rlib_queries **queries;

	gint queries_count;
	struct rlib_rip_reports reportstorun[RLIB_MAXIMUM_REPORTS];
	GSList *search_paths;
	struct rlib_results **results;

	struct rlib_part *parts[RLIB_MAXIMUM_REPORTS];
	gint parts_count;

	gint current_result;

	gint resultset_followers_count;
	struct rlib_resultset_followers followers[RLIB_MAXIMUM_FOLLOWERS];

	gint format;
	gint inputs_count;
	gboolean did_execute;

	gchar *special_locale;
	gchar *current_locale;
	gchar radix_character;

	gint html_debugging;

	struct output_filter *o;
	struct input_filters inputs[MAX_INPUT_FILTERS];
	struct environment_filter *environment;
	GSList *graph_regions;
	GSList *graph_minor_x_ticks;

	gint pcode_alpha_index;
	gint pcode_alpha_m_index;

	GIConv xml_encoder;
};

#define INPUT(r, i) (r->results[i]->input)
#define ENVIRONMENT(r) (r->environment)
#define ENVIRONMENT_PRIVATE(r) (((struct _private *)r->evnironment->private))

#define OUTPUT(r) (r->o)
#define OUTPUT_PRIVATE(r) (((struct _private *)r->o->private))

struct output_filter {
	gpointer *private;
	gboolean do_align;
	gboolean do_breaks;
	gboolean do_grouptext;
	gboolean trim_links;
	gboolean table_around_multiple_detail_columns;
	gboolean do_graph;
	gint paginate;
	gfloat (*get_string_width)(rlib *, const char *);
	void (*print_text)(rlib *, float, float, const char *, int, struct rlib_line_extra_data *);
	void (*print_text_delayed)(rlib *, struct rlib_delayed_extra_data *, int, int);
	void (*set_fg_color)(rlib *, float, float, float);
	void (*set_bg_color)(rlib *, float, float, float);
	void (*hr)(rlib *, int, float, float, float, float, struct rlib_rgb *, float, float);
	void (*start_draw_cell_background)(rlib *, float, float, float, float, struct rlib_rgb *);
	void (*end_draw_cell_background)(rlib *);
	void (*start_boxurl)(rlib *, struct rlib_part *part, float, float, float, float, char *, int);
	void (*end_boxurl)(rlib *, int);
	void (*start_bold)(rlib *);
	void (*end_bold)(rlib *);
	void (*start_italics)(rlib *);
	void (*end_italics)(rlib *);
	void (*background_image)(rlib *, float, float, char *, char *, float, float);
	void (*line_image)(rlib *, float, float, char *, char *, float, float);
	void (*set_font_point)(rlib *, int);
	void (*start_new_page)(rlib *, struct rlib_part *);
	void (*end_page)(rlib *, struct rlib_part *);
	void (*end_page_again)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*init_end_page)(rlib *);
	void (*set_working_page)(rlib *, struct rlib_part *, int);
	void (*set_raw_page)(rlib *, struct rlib_part *, int);
	void (*start_rlib_report)(rlib *);
	void (*end_rlib_report)(rlib *);
	void (*start_part)(rlib *, struct rlib_part *);
	void (*end_part)(rlib *, struct rlib_part *);
	void (*start_part_table)(rlib *, struct rlib_part *);
	void (*end_part_table)(rlib *, struct rlib_part *);
	void (*start_part_tr)(rlib *, struct rlib_part *);
	void (*end_part_tr)(rlib *, struct rlib_part *);
	void (*start_part_td)(rlib *, struct rlib_part *, gfloat width, gfloat height);
	void (*end_part_td)(rlib *, struct rlib_part *);


	void (*start_report)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*end_report)(rlib *, struct rlib_part *part, struct rlib_report *);
	void (*start_report_header)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*end_report_header)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*start_report_footer)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*end_report_footer)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*start_report_field_headers)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*end_report_field_headers)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*start_report_field_details)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*end_report_field_details)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*start_report_line)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*end_report_line)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*start_part_header)(rlib *, struct rlib_part *);
	void (*end_part_header)(rlib *, struct rlib_part *);
	void (*start_part_page_header)(rlib *, struct rlib_part *);
	void (*end_part_page_header)(rlib *, struct rlib_part *);
	void (*start_part_page_footer)(rlib *, struct rlib_part *);
	void (*end_part_page_footer)(rlib *, struct rlib_part *);
	void (*start_report_page_footer)(rlib *, struct rlib_part *, struct rlib_report *report);
	void (*end_report_page_footer)(rlib *, struct rlib_part *, struct rlib_report *report);
	void (*start_report_break_header)(rlib *, struct rlib_part *, struct rlib_report *, struct rlib_report_break *);
	void (*end_report_break_header)(rlib *, struct rlib_part *, struct rlib_report *, struct rlib_report_break *);
	void (*start_report_break_footer)(rlib *, struct rlib_part *, struct rlib_report *, struct rlib_report_break *);
	void (*end_report_break_footer)(rlib *, struct rlib_part *, struct rlib_report *, struct rlib_report_break *);
	void (*start_report_no_data)(rlib *, struct rlib_part *, struct rlib_report *);
	void (*end_report_no_data)(rlib *, struct rlib_part *, struct rlib_report *);


	void (*finalize_private)(rlib *);
	void (*spool_private)(rlib *);
	void (*start_line)(rlib *, int);
	void (*end_line)(rlib *, int);

	void (*start_output_section)(rlib *, struct rlib_report_output_array *);
	void (*end_output_section)(rlib *, struct rlib_report_output_array *);

	void (*start_evil_csv)(rlib *);
	void (*end_evil_csv)(rlib *);

	char *(*get_output)(rlib *);
	long (*get_output_length)(rlib *);

	void (*start_part_pages_across)(rlib *, struct rlib_part *part, gfloat left_margin, gfloat bottom_margin, int width, int height, gint border_width, struct rlib_rgb *color);
	void (*end_part_pages_across)(rlib *, struct rlib_part *part);

	void (*graph_get_x_label_width)(rlib *r, gfloat *width);
	void (*graph_get_y_label_width)(rlib *r, gfloat *width);
	void (*graph_set_x_label_width)(rlib *r, gfloat width, gint cell_width);
	void (*graph_set_y_label_width)(rlib *r, gfloat width);
	void (*graph_get_width_offset)(rlib *r, gint *width_offset);

	void (*graph_init)(rlib *r);
	void (*graph_get_chart_layout)(rlib *r, gfloat top, gfloat bottom, gint cell_height, gint rows, gint *chart_size, gint *chart_height);
	void (*start_graph)(rlib *r, struct rlib_part *, struct rlib_report *, float, float, float, float, gboolean x_axis_labels_are_under_tick);
	void (*end_graph)(rlib *r, struct rlib_part *, struct rlib_report *);
	void (*graph_set_title)(rlib *r, gchar *title);
	void (*graph_set_name)(rlib *r, gchar *name);
	void (*graph_set_legend_bg_color)(rlib *r, struct rlib_rgb *);
	void (*graph_set_legend_orientation)(rlib *r, gint orientation);
	void (*graph_set_draw_x_y)(rlib *r, gboolean draw_x, gboolean draw_y);
	void (*graph_set_is_chart)(rlib *r, gboolean is_chart);
	void (*graph_set_bold_titles)(rlib *r, gboolean bold_titles);
	void (*graph_set_minor_ticks)(rlib *r, gboolean *ticks);
	void (*graph_set_grid_color)(rlib *r, struct rlib_rgb *);
	void (*graph_x_axis_title)(rlib *r, gchar *title);
	void (*graph_y_axis_title)(rlib *r, gchar side, gchar *title);
	void (*graph_set_limits)(rlib *r, gchar side, gdouble min, gdouble max, gdouble origin);
	void (*graph_do_grid)(rlib *r, gboolean just_a_box);
	void (*graph_draw_line)(rlib *, float, float, float, float, struct rlib_rgb *);
	void (*graph_set_x_iterations)(rlib *, int iterations);
	void (*graph_set_x_tick_width)(rlib *);
	void (*graph_tick_x)(rlib *);
	void (*graph_tick_y)(rlib *, int iterations);
	void (*graph_set_data_plot_count)(rlib *r, int count);
	void (*graph_hint_label_x)(rlib *r, gchar *label);
	void (*graph_label_x)(rlib *r, int iteration, gchar *label);
	void (*graph_label_y)(rlib *r, gchar side, int iteration, gchar *label);
	void (*graph_draw_bar)(rlib *r, gint row, gint start_iteration, gint end_iteration, struct rlib_rgb *color, char *label, struct rlib_rgb *label_color, gint width_pad, gint height_pad);
	void (*graph_plot_bar)(rlib *r, gchar side, gint iteration, int plot, gfloat height, struct rlib_rgb * color,gfloat last_height, gboolean divide_iterations, gfloat raw_data, char*label);
	void (*graph_plot_pie)(rlib *r, gfloat start, gfloat end, gboolean offset, struct rlib_rgb *color, gfloat raw_data, gchar *label);
	void (*graph_plot_line)(rlib *r, gchar side, gint iteration, gfloat p1_height, gfloat p1_last_height, gfloat p2_height, gfloat p2_last_height, struct rlib_rgb * color, gfloat raw_data, gchar *label, gint row_count);
	void (*graph_hint_label_y)(rlib *r, gchar side, gchar *string);
	void (*graph_hint_legend)(rlib *r, gchar *string);
	void (*graph_draw_legend)(rlib *r);
	void (*graph_draw_legend_label)(rlib *r, gint iteration, gchar *string, struct rlib_rgb *, gboolean);
	int (*free)(rlib *r);
};

/***** PROTOTYPES: breaks.c ***************************************************/
gboolean rlib_force_break_headers(rlib *r, struct rlib_part *part, struct rlib_report *report, gboolean precalculate);
void rlib_handle_break_headers(rlib *r, struct rlib_part *part, struct rlib_report *report, gboolean precalculate);
void rlib_handle_break_footers(rlib *r, struct rlib_part *part, struct rlib_report *report, gboolean precalculate);
void rlib_break_evaluate_attributes(rlib *r, struct rlib_report *report);
void rlib_breaks_clear(rlib *r, struct rlib_part *part, struct rlib_report *report);

/***** PROTOTYPES: formatstring.c *********************************************/
gint rlib_number_sprintf(rlib *r, gchar **dest, gchar *fmtstr, const struct rlib_value *rval, gint special_format, gchar *infix, gint line_number);
gint rlib_format_string(rlib *r, gchar **buf,  struct rlib_report_field *rf, struct rlib_value *rval);
gint rlib_format_money(rlib *r, gchar **dest, const gchar *moneyformat, gint64 x);
gint rlib_format_number(rlib *r, gchar **dest, const gchar *moneyformat, gint64 x);
gchar *rlib_align_text(rlib *r, char **rtn, gchar *src, gint align, gint width);
GSList * rlib_format_split_string(rlib *r, gchar *data, gint width, gint max_lines, gchar new_line, gchar space, gint *line_count);

/***** PROTOTYPES: fxp.c ******************************************************/
gint64 rlib_fxp_mul(gint64 a, gint64 b, gint64 factor);
gint64 rlib_fxp_div(gint64 num, gint64 denom, gint places);

/***** PROTOTYPES: api.c ******************************************************/
void rlib_trap(void); /* For internals debugging only */
gchar * get_filename(rlib *r, const char *filename, int report_index, gboolean report); /* not an exported API, no rlib_ prefix */

/***** PROTOTYPES: parsexml.c *************************************************/
struct rlib_part * parse_part_file(rlib *r, gint report_index);
struct rlib_report_output * report_output_new(gint type, gpointer data);

/***** PROTOTYPES: pcode.c ****************************************************/
gint64 rlib_str_to_long_long(rlib *r, gchar *str);
gint rvalcmp(struct rlib_value *v1, struct rlib_value *v2);
struct rlib_value * rlib_value_dup(struct rlib_value *orig);
struct rlib_value * rlib_value_dup_contents(struct rlib_value *rval);
struct rlib_value * rlib_value_new_error(struct rlib_value *rval);
gint rlib_execute_as_int(rlib *r, struct rlib_pcode *pcode, gint *result);
gint rlib_execute_as_boolean(rlib *r, struct rlib_pcode *pcode, gint *result);
gint rlib_execute_as_string(rlib *r, struct rlib_pcode *pcode, gchar *buf, gint buf_len);
gint rlib_execute_as_int_inlist(rlib *r, struct rlib_pcode *pcode, gint *result, const gchar *list[]);
gint rlib_execute_as_float(rlib *r, struct rlib_pcode *pcode, gfloat *result);
void rlib_pcode_find_index(rlib *r);

/***** PROTOTYPES: reportgen.c ****************************************************/
void rlib_set_report_from_part(rlib *r, struct rlib_part *part, struct rlib_report *report, gfloat top_margin_offset);
gint will_outputs_fit(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_element *e, gint page);
gint rlib_will_this_fit(rlib *r, struct rlib_part *part, struct rlib_report *report, gfloat total, gint page);
gint get_font_point(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_lines *rl);
gfloat get_output_size(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_output_array *roa);
gint rlib_fetch_first_rows(rlib *r);
gint rlib_end_page_if_line_wont_fit(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_element *e) ;
gfloat get_outputs_size(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_element *e, gint page);
void rlib_init_page(rlib *r, struct rlib_part *part, struct rlib_report *report, gchar report_header);
gint rlib_make_report(rlib *r);
gint rlib_finalize(rlib *r);
gint rlib_format_get_number(const gchar *name);
const gchar * rlib_format_get_name(gint number);
gint rlib_emit_signal(rlib *r, gint signal_number);

/***** PROTOTYPES: resolution.c ***********************************************/
gint rlib_resolve_rlib_variable(rlib *r, gchar *name);
gchar * rlib_resolve_memory_variable(rlib *r, gchar *name);
gchar * rlib_resolve_field_value(rlib *r, struct rlib_resultset_field *rf);
gint rlib_lookup_result(rlib *r, gchar *name);
gint rlib_resolve_resultset_field(rlib *r, gchar *name, void **rtn_field, gint *rtn_resultset);
struct rlib_report_variable *rlib_resolve_variable(rlib *r, struct rlib_part *part, struct rlib_report *report, gchar *name);
void rlib_resolve_report_fields(rlib *r, struct rlib_part *part, struct rlib_report *report);
void rlib_resolve_part_fields(rlib *r, struct rlib_part *part);
void rlib_resolve_metadata(rlib *r);
void rlib_resolve_followers(rlib *r);
void rlib_process_input_metadata(rlib *r);

/***** PROTOTYPES: navigation.c ***********************************************/
gint rlib_navigate_next(rlib *r, gint resultset_num);
gint rlib_navigate_first(rlib *r, gint resultset_num);
gint rlib_navigate_previous(rlib *r, gint resultset_num);
gint rlib_navigate_last(rlib *r, gint resultset_num);

/***** PROTOTYPES: environment.c **********************************************/
void rlib_new_c_environment(rlib *r);

/***** PROTOTYPES: free.c *****************************************************/
void rlib_free_results(rlib *r);

/***** PROTOTYPES: pdf.c ******************************************************/
void rlib_pdf_new_output_filter(rlib *r);

/***** PROTOTYPES: html.c *****************************************************/
void rlib_html_new_output_filter(rlib *r);

/***** PROTOTYPES: txt.c ******************************************************/
void rlib_txt_new_output_filter(rlib *r);

/***** PROTOTYPES: xml.c ******************************************************/
void rlib_xml_new_output_filter(rlib *r);

/***** PROTOTYPES: csv.c ******************************************************/
void rlib_csv_new_output_filter(rlib *r);

/***** PROTOTYPES: mysql.c ****************************************************/
gpointer rlib_mysql_new_input_filter(rlib *r);
gpointer rlib_mysql_real_connect(gpointer input_ptr, gchar *group, gchar *host, gchar *user, gchar *password, gchar *database);

/***** PROTOTYPES: postgres.c **************************************************/
gpointer rlib_postgres_new_input_filter(rlib *r);
gpointer rlib_postgres_connect(gpointer input_ptr, gchar *conn);

/***** PROTOTYPES: layout.c ***************************************************/
gfloat rlib_layout_get_page_width(rlib *r, struct rlib_part *part);
void rlib_layout_init_part_page(rlib *r, struct rlib_part *part, gboolean first, gboolean normal);
gint rlib_layout_report_output(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_element *e, gint backwards, gboolean page_header_layout);
struct rlib_paper * rlib_layout_get_paper(rlib *r, gint paper_type);
struct rlib_paper * rlib_layout_get_paper_by_name(rlib *r, gchar *paper_name);
gint rlib_layout_report_output_with_break_headers(rlib *r, struct rlib_part *part, struct rlib_report *report, gboolean page_header_layout);
void rlib_layout_init_report_page(rlib *r, struct rlib_part *part, struct rlib_report *report);
void rlib_layout_report_footer(rlib *r, struct rlib_part *part, struct rlib_report *report);
gfloat rlib_layout_get_next_line(rlib *r, struct rlib_part *part, gfloat position, struct rlib_report_lines *rl);
gfloat rlib_layout_get_next_line_by_font_point(rlib *r, struct rlib_part *part, gfloat position, gfloat point);
gint rlib_layout_end_page(rlib *r, struct rlib_part *part, struct rlib_report *report, gboolean normal);

/***** PROTOTYPES: axis.c ******************************************************/
/* void rlib_graph_find_y_range(rlib *r, gdouble a, gdouble b, gdouble *y_min, gdouble *y_max, gint graph_type); */
/* gint rlib_graph_num_ticks(rlib *r, gdouble a, gdouble b); */
int adjust_limits(gdouble  dataMin, gdouble dataMax, gint denyMinEqualsAdjMin, gint minTMs, gint maxTMs,
	gint* numTms, gdouble* tmi, gdouble* adjMin, gdouble* adjMax, gint *goodIncs, gint numGoodIncs);

/***** PROTOTYPES: xml_data_source.c ******************************************************/
gpointer rlib_xml_new_input_filter(rlib *r);
gpointer rlib_xml_connect(gpointer input_ptr);

/***** PROTOTYPES: csv_data_source.c ******************************************************/
gpointer rlib_csv_new_input_filter(rlib *r);
gpointer rlib_csv_connect(gpointer input_ptr);

/***** PROTOTYPES: util.c ******************************************************/
void rlogit(rlib *r, const gchar *fmt, ...);
void r_debug(rlib *r, const gchar *fmt, ...);
void r_info(rlib *r, const gchar *fmt, ...);
void r_warning(rlib *r, const gchar *fmt, ...);
void r_error(rlib *r, const gchar *fmt, ...);

/***** PROTOTYPES: variables.c ******************************************************/
void rlib_init_variables(rlib *r, struct rlib_report *report);
void rlib_process_variables(rlib *r, struct rlib_report *report, gboolean precalculate);
void rlib_process_expression_variables(rlib *r, struct rlib_report *report);
gboolean rlib_variabls_needs_precalculate(rlib *r, struct rlib_part *part, struct rlib_report *report);
void rlib_variables_precalculate(rlib *r, struct rlib_part *part, struct rlib_report *report);
void rlib_variable_clear(rlib *r, struct rlib_report_variable *rv, gboolean do_expression);

/***** PROTOTYPES: datetime.c ******************************************************/
void rlib_datetime_format(rlib *r, gchar **dest, struct rlib_datetime *dt, const gchar *fmt);

/***** PROTOTYPES: barcode.c ********************************************************/
int gd_barcode_png_to_file(char *filename, char *barcode, int height);

#endif /* _RLIB_INTERNAL_H */
