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

#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "rlib.h"
#include "rlib_php.h"

#include <php.h>

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

ZEND_MODULE_STARTUP_D(rlib);

/*WRD: It appears we are thread safe here.. not sure yet*/
static gint le_link;

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
	{NULL, NULL, NULL}
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

static void _close_rlib_link(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	rlib_inout_pass *rip = (rlib_inout_pass *)rsrc->ptr;
	efree(rip);
}

ZEND_MODULE_STARTUP_D(rlib) {
	le_link = zend_register_list_destructors_ex(_close_rlib_link, NULL, LE_RLIB_NAME, module_number);
	return SUCCESS;
};

/* implement standard "stub" routine to introduce ourselves to Zend */
ZEND_GET_MODULE(rlib)

/*
	This will Make the connection to the SQL server and select the right database
*/
ZEND_FUNCTION(rlib_init) {
	rlib_inout_pass *rip;
	gint resource_id;
	
	rip = emalloc(sizeof(rlib_inout_pass));
	memset(rip, 0, sizeof(rlib_inout_pass));
	
	rip->content_type = RLIB_CONTENT_TYPE_ERROR;

	rip->r = rlib_init_with_environment(rlib_php_new_environment());
	
	resource_id = ZEND_REGISTER_RESOURCE(return_value, rip, le_link);
	RETURN_RESOURCE(resource_id);
}

ZEND_FUNCTION(rlib_add_datasource_mysql) {
	zval *z_rip = NULL;
	gint datasource_length, sql_host_length, sql_user_length, sql_password_length, sql_database_length;
	gchar *datasource_name, *database_host, *database_user, *database_password, *database_database;
	rlib_inout_pass *rip;
	gint id = -1;
	gint result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsssss", &z_rip,
		&datasource_name, &datasource_length,
		&database_host, &sql_host_length, 
		&database_user, &sql_user_length, 
		&database_password, &sql_password_length, 
		&database_database, &sql_database_length) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);
	
	result = rlib_add_datasource_mysql(rip->r, datasource_name, database_host, database_user, database_password, database_database);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_mysql_from_group) {
	zval *z_rip = NULL;
	gint datasource_length, sql_group_length;
	gchar *datasource_name, *database_group;
	rlib_inout_pass *rip;
	gint id = -1;
	gint result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip,
		&datasource_name, &datasource_length,
		&database_group, &sql_group_length) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);
	
	result = rlib_add_datasource_mysql_from_group(rip->r, datasource_name, database_group);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_postgres) {
	zval *z_rip = NULL;
	gint datasource_length, conn_length;
	gchar *datasource_name, *conn;
	rlib_inout_pass *rip;
	gint id = -1;
	gint result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip,
		&datasource_name, &datasource_length,
		&conn, &conn_length) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);
	
	result = rlib_add_datasource_postgres(rip->r, datasource_name, conn);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_odbc) {
	zval *z_rip = NULL;
	gint datasource_length, sql_odbc_length, sql_user_length, sql_password_length;
	gchar *datasource_name, *database_odbc, *database_user, *database_password;
	rlib_inout_pass *rip;
	gint id = -1;
	gint result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssss", &z_rip,
		&datasource_name, &datasource_length,
		&database_odbc, &sql_odbc_length, 
		&database_user, &sql_user_length, 
		&database_password, &sql_password_length) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);
	
	result = rlib_add_datasource_odbc(rip->r, datasource_name, database_odbc, database_user, database_password);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_array) {
	zval *z_rip = NULL;
	gint datasource_length;
	gchar *datasource_name;
	rlib_inout_pass *rip;
	gint id = -1;
	gint result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip,
		&datasource_name, &datasource_length) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);
	
	result = rlib_add_datasource_php_array(rip->r, estrdup(datasource_name));
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_xml) {
	zval *z_rip = NULL;
	gint datasource_length;
	gchar *datasource_name;
	rlib_inout_pass *rip;
	gint id = -1;
	gint result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip,
		&datasource_name, &datasource_length) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);
	
	result = rlib_add_datasource_xml(rip->r, estrdup(datasource_name));
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_add_datasource_csv) {
	zval *z_rip = NULL;
	gint datasource_length;
	gchar *datasource_name;
	rlib_inout_pass *rip;
	gint id = -1;
	gint result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip,
		&datasource_name, &datasource_length) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);
	
	result = rlib_add_datasource_csv(rip->r, estrdup(datasource_name));
	RETURN_LONG(result);
}
ZEND_FUNCTION(rlib_add_query_as) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *datasource_name, *sql, *name;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsss", &z_rip, 
		&datasource_name, &whatever, &sql, &whatever, &name, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_add_query_as(rip->r, estrdup(datasource_name), estrdup(sql), estrdup(name));
}

ZEND_FUNCTION(rlib_graph_add_bg_region) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *graph_name, *region_label, *color;
	gdouble start, end;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsssdd", &z_rip, 
		&graph_name, &whatever, &region_label, &whatever, &color, &whatever, &start, &end) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_graph_add_bg_region(rip->r, graph_name, region_label, color, start, end);
}

ZEND_FUNCTION(rlib_graph_set_x_minor_tick) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *graph_name, *x_value;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, 
		&graph_name, &whatever, &x_value, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_graph_set_x_minor_tick(rip->r, graph_name, x_value);
}

ZEND_FUNCTION(rlib_graph_set_x_minor_tick_by_location) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *graph_name;
	gdouble location;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsd", &z_rip, 
		&graph_name, &whatever, &location) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_graph_set_x_minor_tick_by_location(rip->r, graph_name, location);
}

ZEND_FUNCTION(rlib_graph_clear_bg_region) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *graph_name;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, 
		&graph_name, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_graph_clear_bg_region(rip->r, graph_name);
}

ZEND_FUNCTION(rlib_add_resultset_follower) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *leader, *follower;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &leader, &whatever, &follower, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_add_resultset_follower(rip->r, estrdup(leader), estrdup(follower));
}

ZEND_FUNCTION(rlib_add_resultset_follower_n_to_1) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *leader, *follower, *leader_field,*follower_field;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssss", &z_rip, &leader, &whatever,&leader_field,&whatever, &follower, &whatever, &follower_field, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_add_resultset_follower_n_to_1(rip->r, estrdup(leader),estrdup(leader_field), estrdup(follower), estrdup(follower_field) );
}

ZEND_FUNCTION(rlib_add_report) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *name;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &name, &whatever) == FAILURE)
		return;
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_add_report(rip->r, estrdup(name));
		
}

ZEND_FUNCTION(rlib_add_report_from_buffer) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *buffer;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &buffer, &whatever) == FAILURE)
		return;
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_add_report_from_buffer(rip->r, estrdup(buffer));
		
}

ZEND_FUNCTION(rlib_query_refresh) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE)
		return;
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_query_refresh(rip->r);	
}

gboolean default_callback(rlib *r, gpointer data) {
	zval *z_function_name = data;
	zval *retval;
	TSRMLS_FETCH();

	if(call_user_function_ex(CG(function_table), NULL, z_function_name, &retval, 0, NULL, 0, NULL TSRMLS_CC) == FAILURE) {
	   return FALSE;
	}
	
	return TRUE;
}

ZEND_FUNCTION(rlib_signal_connect) {
	zval *z_rip = NULL;
	zval *z_function_name;
	rlib_inout_pass *rip;
	gint id = -1;
	gchar *function, *signal;
	gint whatever;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &signal, &whatever, &function, &whatever) == FAILURE)
		return;
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	


	MAKE_STD_ZVAL(z_function_name);
	ZVAL_STRING(z_function_name, function, 1);
	
	rlib_signal_connect_string(rip->r, signal, default_callback, z_function_name);
}

struct both {
	zval *z_function_name;
	gint params;
};

gboolean default_function(rlib *r, struct rlib_pcode * code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data) {
	struct both *b = user_data;
	zval ***params = emalloc(b->params * sizeof(gpointer));
	int i;
	zval *retval;
	struct rlib_value rval_rtn;
	TSRMLS_FETCH();
	
	for(i=0;i<b->params;i++) {
		struct rlib_value *v = rlib_value_stack_pop(vs);
		int spot = b->params-i-1;
		if(RLIB_VALUE_IS_STRING(v)) {
			params[spot] = emalloc(sizeof(gpointer));
			MAKE_STD_ZVAL(*params[spot]);
			(*params[spot])->type = IS_STRING;
			if(RLIB_VALUE_GET_AS_STRING(v) == NULL) {
				(*params[spot])->value.str.len = 0;
				(*params[spot])->value.str.val = estrdup("");
			} else {
				(*params[spot])->value.str.len = strlen(RLIB_VALUE_GET_AS_STRING(v));
				(*params[spot])->value.str.val = estrdup(RLIB_VALUE_GET_AS_STRING(v));				
			}
			rlib_value_free(v);
		} else if(RLIB_VALUE_IS_NUMBER(v)) {
			params[spot] = emalloc(sizeof(gpointer));
			MAKE_STD_ZVAL(*params[spot]);
			(*params[spot])->type = IS_DOUBLE;
			(*params[spot])->value.dval = (double)RLIB_VALUE_GET_AS_NUMBER(v) / (double)RLIB_DECIMAL_PRECISION;
			rlib_value_free(v);
		}
	
	}

	if(call_user_function_ex(CG(function_table), NULL, b->z_function_name, &retval, b->params, params, 0, NULL TSRMLS_CC) == FAILURE) {
	   return FALSE;
	}

	if( Z_TYPE_P(retval) == IS_STRING )	
		rlib_value_stack_push(r, vs, rlib_value_new_string(&rval_rtn, estrdup(Z_STRVAL_P(retval))));
	else if( Z_TYPE_P(retval) == IS_LONG ) {	
		gint64 result = Z_LVAL_P(retval)*RLIB_DECIMAL_PRECISION;
		rlib_value_stack_push(r, vs, rlib_value_new_number(&rval_rtn, result));
	} else if( Z_TYPE_P(retval) == IS_DOUBLE ) {	
		gint64 result = (gdouble)Z_DVAL_P(retval)*(gdouble)RLIB_DECIMAL_PRECISION;
		rlib_value_stack_push(r, vs, rlib_value_new_number(&rval_rtn, result));
	} else {
		rlib_value_stack_push(r, vs, rlib_value_new_error(&rval_rtn));		
	}

	return TRUE;
}

ZEND_FUNCTION(rlib_add_function) {
	zval *z_rip = NULL;
	zval *z_function_name;
	rlib_inout_pass *rip;
	gint id = -1;
	gchar *function, *name;
	gint whatever;
	gdouble paramaters;
	
	struct both *b;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssd", &z_rip, &name, &whatever, &function, &whatever, &paramaters) == FAILURE)
		return;
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	


	MAKE_STD_ZVAL(z_function_name);
	ZVAL_STRING(z_function_name, function, 1);
	
	b = emalloc(sizeof(struct both));

	b->z_function_name = z_function_name;
	b->params = (int)paramaters;
	
	rlib_add_function(rip->r, name, default_function, b);
}

ZEND_FUNCTION(rlib_set_output_format_from_text) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *name;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &name, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_set_output_format_from_text(rip->r, name);

}

ZEND_FUNCTION(rlib_execute) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	gint id = -1;
	gint result = 0;	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	
	result = rlib_execute(rip->r);
	RETURN_LONG(result);
}

ZEND_FUNCTION(rlib_spool) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	if(rip->r != NULL)
		rlib_spool(rip->r);
	else {
		zend_error(E_ERROR, "Unable to run report with requested data");
	}
	
}

ZEND_FUNCTION(rlib_free) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);
	if(rip->r != NULL) {
		rlib_free(rip->r);
	}
}

ZEND_FUNCTION(rlib_get_content_type) {
	zval *z_rip = NULL;
	rlib_inout_pass *rip;
	gint id = -1;
	static gchar buf[MAXSTRLEN];
	gchar *content_type;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_rip) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	content_type = rlib_get_content_type_as_text(rip->r);

	sprintf(buf, "%s%c", content_type, 10);

	RETURN_STRING(buf, TRUE);
}

ZEND_FUNCTION(rlib_add_parameter) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *name, *value;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &name, &whatever, &value, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_add_parameter(rip->r, name, value);
}

ZEND_FUNCTION(rlib_set_locale) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *locale;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &locale, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_set_locale(rip->r, locale);
}

ZEND_FUNCTION(rlib_bindtextdomain) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *domainname, *dirname;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &domainname, &whatever, &dirname, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_bindtextdomain(rip->r, domainname, dirname);
}

ZEND_FUNCTION(rlib_set_radix_character) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *radix;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &radix, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	if (whatever > 0)	
		rlib_set_radix_character(rip->r, radix[0]);
}


ZEND_FUNCTION(rlib_version) {
	const gchar *ver = rlib_version();
	RETURN_STRING(estrdup(ver), TRUE);
}

ZEND_FUNCTION(rlib_set_datasource_encoding) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *encoding, *datasource;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &datasource, &whatever, &encoding, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_set_datasource_encoding(rip->r, datasource, encoding);
}

ZEND_FUNCTION(rlib_set_output_encoding) {
	zval *z_rip = NULL;
	gint whatever;
	gchar *encoding;
	rlib_inout_pass *rip;
	gint id = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &encoding, &whatever) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	

	rlib_set_output_encoding(rip->r, encoding);
}

ZEND_FUNCTION(rlib_set_output_parameter) {
	gint id = -1;
	gint whatever;
	gchar *d1, *d2;
	zval *z_rip = NULL;
	rlib_inout_pass *rip;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_rip, &d1, &whatever, &d2, &whatever) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);	
	rlib_set_output_parameter(rip->r, d1, d2);
}

GString *error_data;

void compile_error_capture(rlib *r, const gchar *msg) {
	error_data = g_string_append(error_data, msg);
}

ZEND_FUNCTION(rlib_compile_infix) {
	gint size_of_string;
	gchar *infix;
	struct rlib_pcode *code;
	struct rlib_value value;
	error_data = g_string_new("");

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &infix, &size_of_string) == FAILURE) {
		return;
	}

	rlib_setmessagewriter(compile_error_capture);
	code = rlib_infix_to_pcode(NULL, NULL, NULL, infix, -1, FALSE);
	if(code != NULL) {
		rlib_execute_pcode(NULL, &value, code, NULL);
		rlib_pcode_free(code);
		rlib_value_free(&value);
	}
	
	RETURN_STRING(estrdup(error_data->str), TRUE);
	g_string_free(error_data, TRUE);
}

ZEND_FUNCTION(rlib_add_search_path) {
	zval *z_rip = NULL;
	gint sp_len;
	gchar *sp;
	rlib_inout_pass *rip;
	gint id = -1;
	gint result = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_rip, &sp, &sp_len) == FAILURE)
		return;

	ZEND_FETCH_RESOURCE(rip, rlib_inout_pass *, &z_rip, id, LE_RLIB_NAME, le_link);

	result = rlib_add_search_path(rip->r, sp);
	RETURN_LONG(result);
}
