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
#ifndef _RLIB_H_
#define _RLIB_H_

#include <glib.h>

#ifndef MAXSTRLEN
#define MAXSTRLEN 1024
#endif

#define RLIB_CONTENT_TYPE_ERROR	-1
#define RLIB_CONTENT_TYPE_PDF 	1
#define RLIB_CONTENT_TYPE_HTML	2
#define RLIB_CONTENT_TYPE_TXT		3
#define RLIB_CONTENT_TYPE_CSV		4

#define RLIB_WEB_CONTENT_TYPE_HTML "Content-Type: text/html; charset=%s\n"
#define RLIB_WEB_CONTENT_TYPE_TEXT "Content-Type: text/plain; charset=%s\n"
#define RLIB_WEB_CONTENT_TYPE_PDF "Content-Type: application/pdf\n"
#define RLIB_WEB_CONTENT_TYPE_CSV "Content-type: application/octet-stream\nContent-Disposition: attachment; filename=report.csv\n"
#define RLIB_WEB_CONTENT_TYPE_CSV_FORMATTED "Content-type: application/octet-stream\nContent-Disposition: attachment; filename=%s\n"

#define RLIB_FORMAT_PDF 	1
#define RLIB_FORMAT_HTML	2
#define RLIB_FORMAT_TXT 	3
#define RLIB_FORMAT_CSV 	4
#define RLIB_FORMAT_XML 	5

#define RLIB_ORIENTATION_PORTRAIT	0
#define RLIB_ORIENTATION_LANDSCAPE	1

#define RLIB_SIGNAL_ROW_CHANGE          0
#define RLIB_SIGNAL_REPORT_START        1
#define RLIB_SIGNAL_REPORT_DONE         2
#define RLIB_SIGNAL_REPORT_ITERATION    3
#define RLIB_SIGNAL_PART_ITERATION      4
#define RLIB_SIGNAL_PRECALCULATION_DONE 5

#define RLIB_SIGNALS 6

struct rlib;
typedef struct rlib rlib;

/* Used by rlib_init_with_environment() */
struct environment_filter {
	gpointer private;
	gchar *(*rlib_resolve_memory_variable)(char *);
	gint (*rlib_write_output)(char *, int);
	void (*free)(rlib *);
};

struct rlib_pcode {
	gint count;
	struct rlib_pcode_instruction *instructions;
	gchar *infix_string;
	gint line_number;
};

typedef struct rlib_datetime rlib_datetime;
struct rlib_datetime {
	GDate date;
	glong ltime;
};

struct rlib_value {
	gint type;
	gint64 number_value;
	struct rlib_datetime date_value;
	gchar *string_value;
	gpointer iif_value;
	gint free;
};

#define RLIB_VALUE_ERROR	-1
#define RLIB_VALUE_NONE		0
#define RLIB_VALUE_NUMBER	1
#define RLIB_VALUE_STRING	2
#define RLIB_VALUE_DATE 	3
#define RLIB_VALUE_IIF 		100

#define RLIB_VALUE_TYPE_NONE(a) ((a)->type = RLIB_VALUE_NONE);((a)->free = FALSE)
#define RLIB_VALUE_GET_TYPE(a) ((a)->type)
#define RLIB_VALUE_IS_NUMBER(a)	(RLIB_VALUE_GET_TYPE(a)==RLIB_VALUE_NUMBER)
#define RLIB_VALUE_IS_STRING(a)	(RLIB_VALUE_GET_TYPE(a)==RLIB_VALUE_STRING)
#define RLIB_VALUE_IS_DATE(a)	(RLIB_VALUE_GET_TYPE(a)==RLIB_VALUE_DATE)
#define RLIB_VALUE_IS_IIF(a)	(RLIB_VALUE_GET_TYPE(a)==RLIB_VALUE_IIF)
#define RLIB_VALUE_IS_ERROR(a)	(RLIB_VALUE_GET_TYPE(a)==RLIB_VALUE_ERROR)
#define RLIB_VALUE_IS_NONE(a)	(RLIB_VALUE_GET_TYPE(a)==RLIB_VALUE_NONE)
#define RLIB_VALUE_GET_AS_NUMBER(a) ((a)->number_value)
#define RLIB_VALUE_GET_AS_NUMBERNP(a) (&((a)->number_value))
#define RLIB_VALUE_GET_AS_STRING(a) ((a)->string_value)
#define RLIB_VALUE_GET_AS_DATE(a) (a->date_value)
#define RLIB_VALUE_GET_AS_IIF(a) ((struct rlib_pcode_if *)a->iif_value)

#define RLIB_DECIMAL_PRECISION	10000000LL

struct rlib_value_stack;
struct rlib_report;
struct rlib_part;

#define RLIB_GRAPH_TYPE_LINE_NORMAL                   1
#define RLIB_GRAPH_TYPE_LINE_STACKED                  2
#define RLIB_GRAPH_TYPE_LINE_PERCENT                  3
#define RLIB_GRAPH_TYPE_AREA_NORMAL                   4
#define RLIB_GRAPH_TYPE_AREA_STACKED                  5
#define RLIB_GRAPH_TYPE_AREA_PERCENT                  6
#define RLIB_GRAPH_TYPE_COLUMN_NORMAL                 7
#define RLIB_GRAPH_TYPE_COLUMN_STACKED                8
#define RLIB_GRAPH_TYPE_COLUMN_PERCENT                9
#define RLIB_GRAPH_TYPE_ROW_NORMAL                   10
#define RLIB_GRAPH_TYPE_ROW_STACKED                  11
#define RLIB_GRAPH_TYPE_ROW_PERCENT                  12
#define RLIB_GRAPH_TYPE_PIE_NORMAL                   13
#define RLIB_GRAPH_TYPE_PIE_RING                     14
#define RLIB_GRAPH_TYPE_PIE_OFFSET                   15
#define RLIB_GRAPH_TYPE_XY_SYMBOLS_ONLY              16
#define RLIB_GRAPH_TYPE_XY_LINES_WITH_SUMBOLS        17
#define RLIB_GRAPH_TYPE_XY_LINES_ONLY                18
#define RLIB_GRAPH_TYPE_XY_CUBIC_SPLINE              19
#define RLIB_GRAPH_TYPE_XY_CUBIC_SPLINE_WIHT_SYMBOLS 20
#define RLIB_GRAPH_TYPE_XY_BSPLINE                   21
#define RLIB_GRAPH_TYPE_XY_BSPLINE_WITH_SYMBOLS      22

/* Initialization, cleanup */
rlib *rlib_init(void);
rlib *rlib_init_with_environment(struct environment_filter *environment);
struct environment_filter *rlib_get_environment(rlib *r);
int rlib_free(rlib *r);
const gchar *rlib_version(void);
gchar *rlib_bindtextdomain(rlib *r, gchar *domainname, gchar *dirname);

/* Datasource definition */
gint rlib_add_datasource_mysql(rlib *r, const gchar *input_name, const gchar *database_host, const gchar *database_user,
	const gchar *database_password, const gchar *database_database);
gint rlib_add_datasource_mysql_from_group(rlib *r, const gchar *input_name, const gchar *group);
gint rlib_add_datasource_postgres(rlib *r, const gchar *input_name, const gchar *conn);
gint rlib_add_datasource_odbc(rlib *r, const gchar *input_name, const gchar *source,
	const gchar *user, const gchar *password);
gint rlib_add_datasource_xml(rlib *r, const gchar *input_name);
gint rlib_add_datasource_csv(rlib *r, const gchar *input_name);

/* Query definition */
gint rlib_add_query_as(rlib *r, const gchar *input_name, const gchar *sql, const gchar *name);
gint rlib_add_query_pointer_as(rlib *r, const gchar *input_source, gchar *sql, const gchar *name);

/* Report XML definition */
gint rlib_add_search_path(rlib *r, const gchar *path);
gint rlib_add_report(rlib *r, const gchar *name);
gint rlib_add_report_from_buffer(rlib *r, gchar *buffer);

/* Output control */
void rlib_set_radix_character(rlib *r, gchar radix_character);
gint rlib_set_output_format(rlib *r, gint format);
gint rlib_set_output_format_from_text(rlib *r, gchar * name);
void rlib_set_output_parameter(rlib *r, gchar *parameter, gchar *value);
gint rlib_set_locale(rlib *r, gchar *locale);
void rlib_set_output_encoding(rlib *r, const char *encoding);
gint rlib_set_datasource_encoding(rlib *r, gchar *input_name, gchar *encoding);
gint rlib_execute(rlib *r);
gchar * rlib_get_content_type_as_text(rlib *r);
gint rlib_spool(rlib *r);
gchar *rlib_get_output(rlib *r);
gint rlib_get_output_length(rlib *r);

/* Report control */
typedef gboolean (*rlib_function)(rlib *, struct rlib_pcode *code, struct rlib_value_stack *, struct rlib_value *this_field_value, gpointer user_data);
gboolean rlib_add_function(rlib *r, gchar *function_name, rlib_function function, gpointer user_data);
gboolean rlib_signal_connect(rlib *r, gint signal_number, gboolean (*signal_function)(rlib *, gpointer), gpointer data);
gboolean rlib_signal_connect_string(rlib *r, gchar *signal_name, gboolean (*signal_function)(rlib *, gpointer), gpointer data);
gint rlib_add_parameter(rlib *r, const gchar *name, const gchar *value);
gint rlib_add_resultset_follower_n_to_1(rlib *r, gchar *leader, gchar *leader_field, gchar *follower,gchar *follower_field);
gint rlib_add_resultset_follower(rlib *r, gchar *leader, gchar *follower);
gboolean rlib_query_refresh(rlib *r);
gint rlib_graph_add_bg_region(rlib *r, gchar *graph_name, gchar *region_label, gchar *color, gfloat start, gfloat end);
gint rlib_graph_clear_bg_region(rlib *r, gchar *graph_name);
gint rlib_graph_set_x_minor_tick(rlib *r, gchar *graph_name, gchar *x_value);
gint rlib_graph_set_x_minor_tick_by_location(rlib *r, gchar *graph_name, gint location);

/* Redirect error messages */
void rlib_setmessagewriter(void(*writer)(rlib *r, const gchar *msg));

/* Value control */
struct rlib_value *rlib_value_stack_pop(struct rlib_value_stack *vs);
int rlib_value_stack_push(rlib *r, struct rlib_value_stack *vs, struct rlib_value *value);
struct rlib_value *rlib_value_new_number(struct rlib_value *rval, gint64 value);
struct rlib_value *rlib_value_new_string(struct rlib_value *rval, const char *value);
struct rlib_value *rlib_value_new_date(struct rlib_value *rval, struct rlib_datetime *date);
struct rlib_value * rlib_value_new_error(struct rlib_value *rval);
gint rlib_value_free(struct rlib_value *rval);
struct rlib_pcode *rlib_infix_to_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, gchar *infix, gint line_number, gboolean look_at_metadata);
struct rlib_value *rlib_execute_pcode(rlib *r, struct rlib_value *rval, struct rlib_pcode *code, struct rlib_value *this_field_value);
void rlib_pcode_free(struct rlib_pcode *code);

/* Graphing */
gfloat rlib_graph(rlib *r, struct rlib_part *part, struct rlib_report *report, gfloat left_margin_offset, gfloat *top_margin_offset);
gfloat rlib_chart(rlib *r, struct rlib_part *part, struct rlib_report *report, gfloat left_margin_offset, gfloat *top_margin_offset);

#endif /* RLIB_H_ */
