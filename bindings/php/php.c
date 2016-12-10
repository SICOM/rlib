/*
 *  Copyright (C) 2003-2016 SICOM Systems, INC.
 *
 *  Authors: Bob Doan <bdoan@sicompos.com>
 *  Updated for PHP 7: Zoltán Böszörményi <zboszormenyi@sicom.com>
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

#include <php.h>

#include <rlib.h>
#include "rlib_php.h"

#define UNUSED __attribute__((unused))

/*
	here we define the PHP interface to rlib.  always assume no access to this source when making methods
	If you want to hack this read the "Extending PHP" section of the PHP Manual from php.net
*/


/* declaration of functions to be exported */
ZEND_FUNCTION(rlib_init);
ZEND_FUNCTION(rlib_add_datasource_mysql);
ZEND_FUNCTION(rlib_add_datasource_mysql_from_group);
ZEND_FUNCTION(rlib_add_datasource_postgres);
ZEND_FUNCTION(rlib_add_datasource_odbc);
ZEND_FUNCTION(rlib_add_datasource_array);
ZEND_FUNCTION(rlib_add_datasource_xml);
ZEND_FUNCTION(rlib_add_datasource_csv);
ZEND_FUNCTION(rlib_add_query_as);
ZEND_FUNCTION(rlib_graph_add_bg_region);
ZEND_FUNCTION(rlib_graph_clear_bg_region);
ZEND_FUNCTION(rlib_graph_set_x_minor_tick);
ZEND_FUNCTION(rlib_graph_set_x_minor_tick_by_location);
ZEND_FUNCTION(rlib_add_resultset_follower);
ZEND_FUNCTION(rlib_add_resultset_follower_n_to_1);
ZEND_FUNCTION(rlib_add_report);
ZEND_FUNCTION(rlib_add_report_from_buffer);
ZEND_FUNCTION(rlib_query_refresh);
ZEND_FUNCTION(rlib_signal_connect);
ZEND_FUNCTION(rlib_add_function);
ZEND_FUNCTION(rlib_set_output_format_from_text);
ZEND_FUNCTION(rlib_execute);
ZEND_FUNCTION(rlib_spool);
ZEND_FUNCTION(rlib_free);
ZEND_FUNCTION(rlib_get_content_type);
ZEND_FUNCTION(rlib_add_parameter);
ZEND_FUNCTION(rlib_set_locale);
ZEND_FUNCTION(rlib_bindtextdomain);
ZEND_FUNCTION(rlib_set_radix_character);
ZEND_FUNCTION(rlib_version);
ZEND_FUNCTION(rlib_set_output_parameter);
ZEND_FUNCTION(rlib_set_datasource_encoding);
ZEND_FUNCTION(rlib_set_output_encoding);
ZEND_FUNCTION(rlib_compile_infix);
ZEND_FUNCTION(rlib_add_search_path);
ZEND_FUNCTION(rlib_parse);
ZEND_FUNCTION(rlib_set_query_cache_size);
ZEND_FUNCTION(rlib_set_numeric_precision_bits);

PHP_MINIT_FUNCTION(rlib);

static int le_link;

/* compiled function list so Zend knows what's in this module */
zend_function_entry rlib_functions[] =
{
	ZEND_FE(rlib_init, NULL)
	ZEND_FE(rlib_add_datasource_mysql, NULL)
	ZEND_FE(rlib_add_datasource_mysql_from_group, NULL)
	ZEND_FE(rlib_add_datasource_postgres, NULL)
	ZEND_FE(rlib_add_datasource_odbc, NULL)
	ZEND_FE(rlib_add_datasource_array, NULL)
	ZEND_FE(rlib_add_datasource_xml, NULL)
	ZEND_FE(rlib_add_datasource_csv, NULL)
	ZEND_FE(rlib_add_query_as, NULL)
	ZEND_FE(rlib_graph_add_bg_region, NULL)
	ZEND_FE(rlib_graph_clear_bg_region, NULL)
	ZEND_FE(rlib_graph_set_x_minor_tick, NULL)
	ZEND_FE(rlib_graph_set_x_minor_tick_by_location, NULL)
	ZEND_FE(rlib_add_resultset_follower, NULL)
	ZEND_FE(rlib_add_resultset_follower_n_to_1, NULL)
	ZEND_FE(rlib_add_report, NULL)
	ZEND_FE(rlib_add_report_from_buffer, NULL)
	ZEND_FE(rlib_query_refresh, NULL)
	ZEND_FE(rlib_signal_connect, NULL)
	ZEND_FE(rlib_add_function, NULL)
	ZEND_FE(rlib_set_output_format_from_text, NULL)
	ZEND_FE(rlib_execute, NULL)
	ZEND_FE(rlib_spool, NULL)
	ZEND_FE(rlib_free, NULL)
	ZEND_FE(rlib_get_content_type, NULL)
	ZEND_FE(rlib_add_parameter, NULL)
	ZEND_FE(rlib_set_locale, NULL)
	ZEND_FE(rlib_bindtextdomain, NULL)
	ZEND_FE(rlib_set_radix_character, NULL)
	ZEND_FE(rlib_version, NULL)
	ZEND_FE(rlib_set_output_parameter, NULL)
	ZEND_FE(rlib_set_datasource_encoding, NULL)
	ZEND_FE(rlib_set_output_encoding, NULL)
	ZEND_FE(rlib_compile_infix, NULL)
	ZEND_FE(rlib_add_search_path, NULL)
	ZEND_FE(rlib_parse, NULL)
	ZEND_FE(rlib_set_query_cache_size, NULL)
	ZEND_FE(rlib_set_numeric_precision_bits, NULL)
	{ .fname = NULL }
};

/* compiled module information */
zend_module_entry rlib_module_entry =
{
    STANDARD_MODULE_HEADER,
    "rlib",
    rlib_functions,
    ZEND_MODULE_STARTUP_N(rlib), 
    NULL, 
    NULL, 
    NULL, 
    NULL,
    NO_VERSION_YET,
    STANDARD_MODULE_PROPERTIES
};

ZEND_RSRC_DTOR_FUNC(_close_rlib_link)
{
#if PHP_MAJOR_VERSION < 7
	rlib_inout_pass *rip = (rlib_inout_pass *)rsrc->ptr;
#else
	rlib_inout_pass *rip = (rlib_inout_pass *)res->ptr;
#endif
	efree(rip);
}

PHP_MINIT_FUNCTION(rlib) {
	le_link = zend_register_list_destructors_ex(_close_rlib_link, NULL, LE_RLIB_NAME, module_number);
	return SUCCESS;
};

/* implement standard "stub" routine to introduce ourselves to Zend */
ZEND_GET_MODULE(rlib)

/*
 * This will create the RLIB struct
 */
ZEND_FUNCTION(rlib_init) {
	rlib_inout_pass *rip;
#if PHP_MAJOR_VERSION < 7
	gint resource_id;
#else
	zend_resource *res;
#endif

	rip = emalloc(sizeof(rlib_inout_pass));
	memset(rip, 0, sizeof(rlib_inout_pass));

	rip->r = rlib_init_with_environment(rlib_php_new_environment());

#if PHP_MAJOR_VERSION < 7
	resource_id = zend_register_resource(return_value, rip, le_link);
	RETURN_RESOURCE(resource_id);
#else
	res = zend_register_resource(rip, le_link);
	RETURN_RES(res);
#endif
}

ZEND_FUNCTION(rlib_add_datasource_mysql) {
	zval *z_rip = NULL;
	z_str_len_t datasource_length, sql_host_length, sql_user_length, sql_password_length, sql_database_length;
	char *datasource_name, *database_host, *database_user, *database_password, *database_database;
	rlib_inout_pass *rip;
	gint result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsssss", &z_rip,
		&datasource_name, &datasource_length,
		&database_host, &sql_host_length, 
		&database_user, &sql_user_length, 
		&database_password, &sql_password_length, 
		&database_database, &sql_database_length) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif
	
	result = rlib_add_datasource_mysql(rip->r, datasource_name, database_host, database_user, database_password, database_database);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_mysql_from_group) {
	zval *z_rip = NULL;
	z_str_len_t datasource_length, sql_group_length;
	char *datasource_name, *database_group;
	rlib_inout_pass *rip;
	gint result = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip,
		&datasource_name, &datasource_length,
		&database_group, &sql_group_length) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif
	
	result = rlib_add_datasource_mysql_from_group(rip->r, datasource_name, database_group);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_postgres) {
	zval *z_rip = NULL;
	z_str_len_t datasource_length, conn_length;
	char *datasource_name, *conn;
	rlib_inout_pass *rip;
	gint result = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip,
		&datasource_name, &datasource_length,
		&conn, &conn_length) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif
	
	result = rlib_add_datasource_postgres(rip->r, datasource_name, conn);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_odbc) {
	zval *z_rip = NULL;
	z_str_len_t datasource_length, sql_odbc_length, sql_user_length, sql_password_length;
	gchar *datasource_name, *database_odbc, *database_user, *database_password;
	rlib_inout_pass *rip;
	gint result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssss", &z_rip,
		&datasource_name, &datasource_length,
		&database_odbc, &sql_odbc_length, 
		&database_user, &sql_user_length, 
		&database_password, &sql_password_length) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif
	
	result = rlib_add_datasource_odbc(rip->r, datasource_name, database_odbc, database_user, database_password);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_array) {
	zval *z_rip = NULL;
	z_str_len_t datasource_length;
	char *datasource_name;
	rlib_inout_pass *rip;
	gint result = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &datasource_name, &datasource_length) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
#endif
	
	result = rlib_add_datasource_php_array(rip->r, estrdup(datasource_name));
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_xml) {
	zval *z_rip = NULL;
	z_str_len_t datasource_length;
	char *datasource_name;
	rlib_inout_pass *rip;
	gint result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &datasource_name, &datasource_length) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
#endif
	
	result = rlib_add_datasource_xml(rip->r, estrdup(datasource_name));
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_csv) {
	zval *z_rip = NULL;
	z_str_len_t datasource_length;
	char *datasource_name;
	rlib_inout_pass *rip;
	gint result = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &datasource_name, &datasource_length) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif
	
	result = rlib_add_datasource_csv(rip->r, estrdup(datasource_name));
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_query_as) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *datasource_name, *sql, *name;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsss", &z_rip,
		&datasource_name, &whatever, &sql, &whatever, &name, &whatever) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_add_query_as(rip->r, estrdup(datasource_name), estrdup(sql), estrdup(name));
}

ZEND_FUNCTION(rlib_graph_add_bg_region) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *graph_name, *region_label, *color;
	gdouble start, end;
	rlib_inout_pass *rip;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsssdd", &z_rip, 
		&graph_name, &whatever, &region_label, &whatever, &color, &whatever, &start, &end) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_graph_add_bg_region(rip->r, graph_name, region_label, color, start, end);
}

ZEND_FUNCTION(rlib_graph_set_x_minor_tick) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *graph_name, *x_value;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, 
		&graph_name, &whatever, &x_value, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_graph_set_x_minor_tick(rip->r, graph_name, x_value);
}

ZEND_FUNCTION(rlib_graph_set_x_minor_tick_by_location) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *graph_name;
	gdouble location;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsd", &z_rip, 
		&graph_name, &whatever, &location) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_graph_set_x_minor_tick_by_location(rip->r, graph_name, location);
}

ZEND_FUNCTION(rlib_graph_clear_bg_region) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *graph_name;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, 
		&graph_name, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_graph_clear_bg_region(rip->r, graph_name);
}

ZEND_FUNCTION(rlib_add_resultset_follower) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *leader, *follower;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &leader, &whatever, &follower, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_add_resultset_follower(rip->r, estrdup(leader), estrdup(follower));
}

ZEND_FUNCTION(rlib_add_resultset_follower_n_to_1) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *leader, *follower, *leader_field,*follower_field;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssss", &z_rip,
			&leader, &whatever, &leader_field, &whatever,
			&follower, &whatever, &follower_field, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_add_resultset_follower_n_to_1(rip->r, estrdup(leader),estrdup(leader_field), estrdup(follower), estrdup(follower_field) );
}

ZEND_FUNCTION(rlib_add_report) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *name;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &name, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_add_report(rip->r, estrdup(name));
		
}

ZEND_FUNCTION(rlib_add_report_from_buffer) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *buffer;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &buffer, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_add_report_from_buffer(rip->r, estrdup(buffer));
		
}

ZEND_FUNCTION(rlib_query_refresh) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_query_refresh(rip->r);	
}

gboolean default_callback(rlib *r UNUSED, gpointer data) {
	zval *z_function_name = data;
#if PHP_MAJOR_VERSION < 7
	zval *retval;
#else
	zval retval;
#endif

	TSRMLS_FETCH();

	if (call_user_function_ex(CG(function_table), NULL, z_function_name, &retval, 0, NULL, 0, NULL TSRMLS_CC) == FAILURE)
		return FALSE;

	return TRUE;
}

ZEND_FUNCTION(rlib_signal_connect) {
	zval *z_rip = NULL;
	zval *z_function_name;
	rlib_inout_pass *rip;
	char *function, *signal;
	z_str_len_t whatever;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &signal, &whatever, &function, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

#if PHP_MAJOR_VERSION < 7
	MAKE_STD_ZVAL(z_function_name);
	ZVAL_STRING(z_function_name, function, 1);
#else
	z_function_name = emalloc(sizeof(zval));
	ZVAL_STRING(z_function_name, function);
#endif
	
	rlib_signal_connect_string(rip->r, signal, default_callback, z_function_name);
}

struct both {
	zval *z_function_name;
	gint params;
};

gboolean default_function(rlib *r, struct rlib_pcode * code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct both *b = user_data;
	int i;
#if PHP_MAJOR_VERSION < 7
	zval ***params = emalloc(b->params * sizeof(gpointer));
#else
	zval *params = emalloc(b->params * sizeof(zval));
	zval value;
#endif
	zval *retval;
	struct rlib_value *rval_rtn;
	TSRMLS_FETCH();

	for (i = 0; i < b->params; i++) {
		struct rlib_value *v = rlib_value_stack_pop(r, vs);
		int spot = b->params-i-1;
		if (RLIB_VALUE_IS_STRING(r, v)) {
			if (RLIB_VALUE_GET_AS_STRING(r, v) == NULL) {
#if PHP_MAJOR_VERSION < 7
				params[spot] = emalloc(sizeof(gpointer));
				MAKE_STD_ZVAL(*params[spot]);
				ZVAL_EMPTY_STRING(*params[spot]);
#else
				ZVAL_EMPTY_STRING(&params[spot]);
#endif
			} else {
#if PHP_MAJOR_VERSION < 7
				params[spot] = emalloc(sizeof(gpointer));
				MAKE_STD_ZVAL(*params[spot]);
				ZVAL_STRING(*params[spot], RLIB_VALUE_GET_AS_STRING(r, v), 1);
#else
				ZVAL_STRING(&params[spot], estrdup(RLIB_VALUE_GET_AS_STRING(r, v)));
#endif
			}
			rlib_value_free(r, v);
		} else if (RLIB_VALUE_IS_NUMBER(r, v)) {
#if PHP_MAJOR_VERSION < 7
			params[spot] = emalloc(sizeof(gpointer));
			MAKE_STD_ZVAL(*params[spot]);
			ZVAL_DOUBLE(*params[spot], rlib_value_get_as_double(r, v));
#else
			ZVAL_DOUBLE(&params[spot], rlib_value_get_as_double(r, v));
#endif
			rlib_value_free(r, v);
		}
	}

#if PHP_MAJOR_VERSION < 7
	if (call_user_function_ex(CG(function_table), NULL, b->z_function_name, &retval, b->params, params, 0, NULL TSRMLS_CC) == FAILURE)
#else
	retval = &value;
	if (call_user_function_ex(CG(function_table), NULL, b->z_function_name, &value, b->params, params, 0, NULL TSRMLS_CC) == FAILURE)
#endif
		return FALSE;

	rval_rtn = rlib_value_alloc(r);

	if (Z_TYPE_P(retval) == IS_STRING)
		rlib_value_stack_push(r, vs, rlib_value_new_string(r, rval_rtn, estrdup(Z_STRVAL_P(retval))));
	else if (Z_TYPE_P(retval) == IS_LONG) {
		rlib_value_stack_push(r, vs, rlib_value_new_number_from_long(r, rval_rtn, Z_LVAL_P(retval)));
	} else if (Z_TYPE_P(retval) == IS_DOUBLE) {
		rlib_value_stack_push(r, vs, rlib_value_new_number_from_double(r, rval_rtn, Z_DVAL_P(retval)));
	} else {
		rlib_value_stack_push(r, vs, rlib_value_new_error(r, rval_rtn));
	}

	return TRUE;
}

ZEND_FUNCTION(rlib_add_function) {
	zval *z_rip = NULL;
	zval *z_function_name;
	rlib_inout_pass *rip;
	char *function, *name;
	z_str_len_t whatever;
	gdouble paramaters;
	struct both *b;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssd", &z_rip, &name, &whatever, &function, &whatever, &paramaters) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

#if PHP_MAJOR_VERSION < 7
	MAKE_STD_ZVAL(z_function_name);
	ZVAL_STRING(z_function_name, function, 1);
#else
	z_function_name = emalloc(sizeof(zval));
	ZVAL_STRING(z_function_name, function);
#endif

	b = emalloc(sizeof(struct both));

	b->z_function_name = z_function_name;
	b->params = (int)paramaters;
	
	rlib_add_function(rip->r, name, default_function, b);
}

ZEND_FUNCTION(rlib_set_output_format_from_text) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *name;
	rlib_inout_pass *rip;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &name, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_set_output_format_from_text(rip->r, name);
}

ZEND_FUNCTION(rlib_execute) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	gint result = 0;	

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	result = rlib_execute(rip->r);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_spool) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	if (rip->r != NULL)
		rlib_spool(rip->r);
	else {
		zend_error(E_ERROR, "Unable to run report with requested data");
	}
}

ZEND_FUNCTION(rlib_free) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	if (rip->r != NULL) {
		rlib_free(rip->r);
	}
}

ZEND_FUNCTION(rlib_get_content_type) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	gchar *content_type;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	content_type = rlib_get_content_type_as_text(rip->r);

#if PHP_MAJOR_VERSION < 7
	RETURN_STRING(content_type, TRUE);
#else
	RETURN_STRING(content_type)
#endif
}

ZEND_FUNCTION(rlib_add_parameter) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *name, *value;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &name, &whatever, &value, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_add_parameter(rip->r, name, value);
}

ZEND_FUNCTION(rlib_set_locale) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *locale;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &locale, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_set_locale(rip->r, locale);
}

ZEND_FUNCTION(rlib_bindtextdomain) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *domainname, *dirname;
	rlib_inout_pass *rip;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &domainname, &whatever, &dirname, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_bindtextdomain(rip->r, domainname, dirname);
}

ZEND_FUNCTION(rlib_set_radix_character) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *radix;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &radix, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	if (whatever > 0)	
		rlib_set_radix_character(rip->r, radix[0]);
}


ZEND_FUNCTION(rlib_version) {
	const gchar *ver = rlib_version();

#if PHP_MAJOR_VERSION < 7
	RETURN_STRING(estrdup(ver), TRUE);
#else
	RETURN_STRING(estrdup(ver));
#endif
}

ZEND_FUNCTION(rlib_set_datasource_encoding) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *encoding, *datasource;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &datasource, &whatever, &encoding, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_set_datasource_encoding(rip->r, datasource, encoding);
}

ZEND_FUNCTION(rlib_set_output_encoding) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *encoding;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &encoding, &whatever) == FAILURE)
		return;
	
#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_set_output_encoding(rip->r, encoding);
}

ZEND_FUNCTION(rlib_set_output_parameter) {
	zval *z_rip = NULL;
	z_str_len_t whatever;
	char *d1, *d2;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &d1, &whatever, &d2, &whatever) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_set_output_parameter(rip->r, d1, d2);
}

GString *error_data;

static void compile_error_capture(rlib *r UNUSED, const gchar *msg) {
	error_data = g_string_append(error_data, msg);
}

ZEND_FUNCTION(rlib_compile_infix) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	z_str_len_t size_of_string;
	char *infix;
	struct rlib_pcode *code;
	struct rlib_value *value = NULL;
	char *ret_str;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &infix, &size_of_string) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	error_data = g_string_new("");

	rlib_setmessagewriter(compile_error_capture);
	code = rlib_infix_to_pcode(rip->r, NULL, NULL, infix, -1, FALSE);
	if (code != NULL) {
		value = rlib_value_alloc(rip->r);
		rlib_execute_pcode(rip->r, value, code, NULL);
		rlib_pcode_free(rip->r, code);
		rlib_value_free(rip->r, value);
	}

	ret_str = estrdup(error_data->str);
	g_string_free(error_data, TRUE);
#if PHP_MAJOR_VERSION < 7
	RETURN_STRING(ret_str, TRUE);
#else
	RETURN_STRING(ret_str);
#endif
}

ZEND_FUNCTION(rlib_add_search_path) {
	zval *z_rip = NULL;
	z_str_len_t sp_len;
	char *sp;
	rlib_inout_pass *rip;
	gint result = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &sp, &sp_len) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	result = rlib_add_search_path(rip->r, sp);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_parse) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	gint result = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	result = rlib_parse(rip->r);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_set_query_cache_size) {
	zval *z_rip = NULL;
	long cache_size;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &z_rip, &cache_size) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_set_query_cache_size(rip->r, cache_size);
	RETURN_LONG(0);
}

ZEND_FUNCTION(rlib_set_numeric_precision_bits) {
	zval *z_rip = NULL;
	long prec;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &z_rip, &prec) == FAILURE)
		return;

#if PHP_MAJOR_VERSION < 7
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, -1, LE_RLIB_NAME, le_link);
#else
	rip = (rlib_inout_pass *)zend_fetch_resource(Z_RES_P(z_rip), LE_RLIB_NAME, le_link);
	if (rip == NULL)
		RETURN_FALSE;
#endif

	rlib_set_numeric_precision_bits(rip->r, prec);

	RETURN_LONG(0);
}
