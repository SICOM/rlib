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

 %module rlib
 %{
#include <rlib.h>
 %}
 
rlib * rlib_init();
int rlib_add_datasource_mysql(rlib *r, char *input_name, char *database_host, char *database_user, char *database_password, char *database_database);
int rlib_add_datasource_postgres(rlib *r, char *input_name, char *conn);
int rlib_add_datasource_odbc(rlib *r, char *input_name, char *source, char *user, char *password);
int rlib_add_datasource_xml(rlib *r, char *input_name);
int rlib_add_datasource_csv(rlib *r, char *input_name);
int rlib_add_query_as(rlib *r, char *input_source, char *sql, char *name);
int rlib_add_search_path(rlib *r, const char *path);
int rlib_add_report(rlib *r, char *name);
int rlib_add_report_from_buffer(rlib *r, char *buffer);
int rlib_execute(rlib *r);
char * rlib_get_content_type_as_text(rlib *r);
int rlib_spool(rlib *r);
int rlib_set_output_format(rlib *r, int format);
int rlib_add_resultset_follower_n_to_1(rlib *r, char *leader, char *leader_field, char *follower, char *follower_field);
int rlib_add_resultset_follower(rlib *r, char *leader, char *follower);
int rlib_set_output_format_from_text(rlib *r, char *name);
char *rlib_get_output(rlib *r);
int rlib_get_output_length(rlib *r);
int rlib_signal_connect(rlib *r, int signal_number, int (*signal_function)(rlib *, void *), void * data);
int rlib_signal_connect_string(rlib *r, char *signal_name, int (*signal_function)(rlib *, void *), void * data);
int rlib_query_refresh(rlib *r);
int rlib_add_parameter(rlib *r, const char *name, const char *value);
int rlib_set_locale(rlib *r, char *locale);
char *rlib_bindtextdomain(rlib *r, char *domainname, char *dirname);
void rlib_set_radix_character(rlib *r, gchar radix_character);
void rlib_set_output_parameter(rlib *r, char *parameter, char *value);
void rlib_set_output_encoding(rlib *r, const char *encoding);
int rlib_set_datasource_encoding(rlib *r, char *input_name, char *encoding);
int rlib_free(rlib *r);
char *rlib_version(void);
int rlib_graph_add_bg_region(rlib *r, char *graph_name, char *region_label, char *color, float start, float end);
int rlib_graph_clear_bg_region(rlib *r, char *graph_name);
int rlib_graph_set_x_minor_tick(rlib *r, char *graph_name, char *x_value);
int rlib_graph_set_x_minor_tick_by_location(rlib *r, char *graph_name, int location);
float rlib_graph(rlib *r, struct rlib_part *part, struct rlib_report *report, float left_margin_offset, float *top_margin_offset);
