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
 * This module defines constants and structures used by most of the C code
 * modules in the library.
 */
#ifndef _RLIB_H_
#define _RLIB_H_

#include <mpfr.h>
#include <glib.h>

#ifndef MAXSTRLEN
#define MAXSTRLEN 1024
#endif

#define RLIB_CONTENT_TYPE_ERROR	-1
#define RLIB_CONTENT_TYPE_PDF 	1
#define RLIB_CONTENT_TYPE_HTML	2
#define RLIB_CONTENT_TYPE_TXT		3
#define RLIB_CONTENT_TYPE_CSV		4

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
	GString *(*rlib_dump_memory_variables)(void);
	gchar *(*rlib_resolve_memory_variable)(char *);
	gint (*rlib_write_output)(char *, gint);
	void (*free)(rlib *);
};

struct rlib_pcode {
	gint count;
	gint line_number;
	struct rlib_pcode_instruction **instructions;
	gchar *infix_string;
};

typedef struct rlib_datetime rlib_datetime;
struct rlib_datetime {
	GDate date;
	gint ltime;
};

#define RLIB_VALUE_ERROR	-1
#define RLIB_VALUE_NONE		0
#define RLIB_VALUE_NUMBER	1
#define RLIB_VALUE_STRING	2
#define RLIB_VALUE_DATE 	3
#define RLIB_VALUE_VECTOR   4
#define RLIB_VALUE_IIF 		100

#define RLIB_VALUE_GET_TYPE(r, a) (rlib_value_get_type(r, a))
#define RLIB_VALUE_IS_NUMBER(r, a)	(RLIB_VALUE_GET_TYPE(r, a) == RLIB_VALUE_NUMBER)
#define RLIB_VALUE_IS_STRING(r, a)	(RLIB_VALUE_GET_TYPE(r, a) == RLIB_VALUE_STRING)
#define RLIB_VALUE_IS_DATE(r, a)	(RLIB_VALUE_GET_TYPE(r, a) == RLIB_VALUE_DATE)
#define RLIB_VALUE_IS_IIF(r, a)		(RLIB_VALUE_GET_TYPE(r, a) == RLIB_VALUE_IIF)
#define RLIB_VALUE_IS_VECTOR(r, a)	(RLIB_VALUE_GET_TYPE(r, a) == RLIB_VALUE_VECTOR)
#define RLIB_VALUE_IS_ERROR(r, a)	(RLIB_VALUE_GET_TYPE(r, a) == RLIB_VALUE_ERROR)
#define RLIB_VALUE_IS_NONE(r, a)	(RLIB_VALUE_GET_TYPE(r, a) == RLIB_VALUE_NONE)
#define RLIB_VALUE_GET_AS_LONG(r, a)	rlib_value_get_as_long(r, a)
#define RLIB_VALUE_GET_AS_INT64(r, a)	rlib_value_get_as_int64(r, a)
#define RLIB_VALUE_GET_AS_DOUBLE(r, a)	rlib_value_get_as_double(r, a)
#define RLIB_VALUE_GET_AS_STRING(r, a)	rlib_value_get_as_string(r, a)
#define RLIB_VALUE_GET_AS_DATE(r, a) (rlib_value_get_as_date(r, a))
#define RLIB_VALUE_GET_AS_VECTOR(r, a) (rlib_value_get_as_vector(r, a))

#define RLIB_DECIMAL_PRECISION	10000000LL
#define RLIB_MPFR_PRECISION_BITS	256

struct rlib_value;
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

/*
 * Initialization, cleanup
 */
rlib *rlib_init(void);
rlib *rlib_init_with_environment(struct environment_filter *environment);
struct environment_filter *rlib_get_environment(rlib *r);
void rlib_free(rlib *r);
const gchar *rlib_version(void);
gchar *rlib_bindtextdomain(rlib *r, gchar *domainname, gchar *dirname);

/*
 * Datasource definition
 * They return TRUE for success, FALSE for error
 */
gint rlib_add_datasource_mysql(rlib *r, const gchar *input_name, const gchar *database_host, const gchar *database_user, const gchar *database_password, const gchar *database_database);
gint rlib_add_datasource_mysql_from_group(rlib *r, const gchar *input_name, const gchar *group);
gint rlib_add_datasource_postgres(rlib *r, const gchar *input_name, const gchar *conn);
gint rlib_add_datasource_odbc(rlib *r, const gchar *input_name, const gchar *source, const gchar *user, const gchar *password);
gint rlib_add_datasource_xml(rlib *r, const gchar *input_name);
gint rlib_add_datasource_csv(rlib *r, const gchar *input_name);
gint rlib_add_datasource_array(rlib *r, const gchar *input_name);

/*
 * Query definition
 * These functions return a positive number for success, -1 on error.
 */
gint rlib_add_query_as(rlib *r, const gchar *input_name, const gchar *sql, const gchar *name);
gint rlib_add_query_pointer_as(rlib *r, const gchar *input_source, gchar *sql, const gchar *name);
gint rlib_add_query_array_as(rlib *r, const gchar *input_source, gpointer array, gint rows, gint cols, const gchar *name);

/* Report XML definition */
gint rlib_add_search_path(rlib *r, const gchar *path);
gint rlib_add_report(rlib *r, const gchar *name);
gint rlib_add_report_from_buffer(rlib *r, gchar *buffer);

/* Output control */
void rlib_set_radix_character(rlib *r, gchar radix_character);
gint rlib_set_output_format(rlib *r, gint format);
void rlib_set_output_format_from_text(rlib *r, gchar * name);
void rlib_set_output_parameter(rlib *r, gchar *parameter, gchar *value);
gboolean rlib_set_locale(rlib *r, gchar *locale);
void rlib_set_output_encoding(rlib *r, const char *encoding);
gint rlib_set_datasource_encoding(rlib *r, gchar *input_name, gchar *encoding);
void rlib_set_query_cache_size(rlib *r, gint cache_size);
gboolean rlib_execute(rlib *r);
gboolean rlib_parse(rlib *);
gchar *rlib_get_content_type_as_text(rlib *r);
gboolean rlib_spool(rlib *r);
gchar *rlib_get_output(rlib *r);
gsize rlib_get_output_length(rlib *r);

/* Report control */
typedef gboolean (*rlib_function)(rlib *, struct rlib_pcode *code, struct rlib_value_stack *, struct rlib_value *this_field_value, gpointer user_data);
gboolean rlib_add_function(rlib *r, gchar *function_name, rlib_function function, gpointer user_data);
gboolean rlib_signal_connect(rlib *r, gint signal_number, gboolean (*signal_function)(rlib *, gpointer), gpointer data);
gboolean rlib_signal_connect_string(rlib *r, gchar *signal_name, gboolean (*signal_function)(rlib *, gpointer), gpointer data);
gint rlib_add_parameter(rlib *r, const gchar *name, const gchar *value);
gboolean rlib_add_resultset_follower_n_to_1(rlib *r, gchar *leader, gchar *leader_field, gchar *follower,gchar *follower_field);
gboolean rlib_add_resultset_follower(rlib *r, gchar *leader, gchar *follower);
gboolean rlib_query_refresh(rlib *r);
gboolean rlib_graph_add_bg_region(rlib *r, gchar *graph_name, gchar *region_label, gchar *color, gdouble start, gdouble end);
gboolean rlib_graph_clear_bg_region(rlib *r, gchar *graph_name);
gboolean rlib_graph_set_x_minor_tick(rlib *r, gchar *graph_name, gchar *x_value);
gboolean rlib_graph_set_x_minor_tick_by_location(rlib *r, gchar *graph_name, gint location);

/* Redirect error messages */
void rlib_setmessagewriter(void(*writer)(rlib *r, const gchar *msg));

/* Value control */
struct rlib_value *rlib_value_alloc(rlib *r);
void rlib_set_numeric_precision_bits(rlib *r, mpfr_prec_t prec);
gint rlib_value_get_type(rlib *r, struct rlib_value *rval);
struct rlib_value *rlib_value_new_none(rlib *r, struct rlib_value *rval);
struct rlib_value *rlib_value_new_number_from_long(rlib *r, struct rlib_value *rval, glong value);
struct rlib_value *rlib_value_new_number_from_int64(rlib *r, struct rlib_value *rval, gint64 value);
struct rlib_value *rlib_value_new_number_from_double(rlib *r, struct rlib_value *rval, gdouble value);
struct rlib_value *rlib_value_new_number_from_mpfr(rlib *r, struct rlib_value *rval, mpfr_ptr value);
struct rlib_value *rlib_value_new_string(rlib *r, struct rlib_value *rval, const char *value);
struct rlib_value *rlib_value_new_date(rlib *r, struct rlib_value *rval, struct rlib_datetime *date);
struct rlib_value *rlib_value_new_vector(rlib *r, struct rlib_value *rval, GSList *vector);
struct rlib_value *rlib_value_new_error(rlib *r, struct rlib_value *rval);
long rlib_value_get_as_long(rlib *r, struct rlib_value *rval);
gint64 rlib_value_get_as_int64(rlib *r, struct rlib_value *rval);
gdouble rlib_value_get_as_double(rlib *r, struct rlib_value *rval);
mpfr_srcptr rlib_value_get_as_mpfr(rlib *r, struct rlib_value *rval);
gchar *rlib_value_get_as_string(rlib *r, struct rlib_value *rval);
struct rlib_datetime *rlib_value_get_as_date(rlib *r, struct rlib_value *rval);
GSList *rlib_value_get_as_vector(rlib *r, struct rlib_value *rval);
void rlib_value_free(rlib *r, struct rlib_value *rval);
gboolean rlib_value_stack_push(rlib *r, struct rlib_value_stack *vs, struct rlib_value *value);
struct rlib_value *rlib_value_stack_pop(rlib *r, struct rlib_value_stack *vs);
struct rlib_pcode *rlib_infix_to_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, gchar *infix, gint line_number, gboolean look_at_metadata);
struct rlib_value *rlib_execute_pcode(rlib *r, struct rlib_value *rval, struct rlib_pcode *code, struct rlib_value *this_field_value);
void rlib_pcode_free(rlib *r, struct rlib_pcode *code);

/* Graphing */
gdouble rlib_graph(rlib *r, struct rlib_part *part, struct rlib_report *report, gdouble left_margin_offset, gdouble *top_margin_offset);
gdouble rlib_chart(rlib *r, struct rlib_part *part, struct rlib_report *report, gdouble left_margin_offset, gdouble *top_margin_offset);

/* Console messages */
void rlogit(rlib *r, const gchar *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void r_debug(rlib *r, const gchar *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void r_info(rlib *r, const gchar *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void r_warning(rlib *r, const gchar *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void r_error(rlib *r, const gchar *fmt, ...) __attribute__ ((format (printf, 2, 3)));

#endif /* RLIB_H_ */
