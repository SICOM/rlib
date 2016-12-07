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

#include <string.h>
#include <gmodule.h>

#include "rlib-internal.h"
#include "rlib_input.h"

DLL_EXPORT_SYM gint rlib_add_datasource(rlib *r, const gchar *input_name, struct input_filter *input) {
	if (r->inputs_count == MAX_INPUT_FILTERS)
		return -1;

	r->inputs[r->inputs_count].input = input;
	r->inputs[r->inputs_count].name = g_strdup(input_name);
	r->inputs[r->inputs_count].handle = NULL;
	r->inputs[r->inputs_count].input->info.encoder = NULL;
	r->inputs_count++;
	return 0;
}

/*
 * For historical reasons, the port is omitted.
 * Now we support "host:port" notation in the host string.
 */
DLL_EXPORT_SYM gint rlib_add_datasource_mysql(rlib *r UNUSED, const gchar *input_name UNUSED, const gchar *database_host, const gchar *database_user UNUSED, const gchar *database_password UNUSED, const gchar *database_database UNUSED) {
#ifndef HAVE_MYSQL
	return -1;
#else
#ifndef HAVE_MYSQL_BUILTIN
	GModule* handle;
	gpointer (*new_input_filter)(struct rlib *);
#endif
	struct input_filter *input;
	gchar *name_copy;
	guint port = 3306;
	gchar *host_copy;

	if (r->inputs_count == MAX_INPUT_FILTERS)
		return -1;

	name_copy = g_strdup(input_name);
	if (name_copy == NULL) {
		r_error(r, "rlib_add_datasource_mysql_private: Out of memory!\n");
		return -1;
	}

#ifndef HAVE_MYSQL_BUILTIN
	handle = g_module_open("libr-mysql", 2);
	if (!handle) {
		g_free(name_copy);
		r_error(r,"Could Not Load MYSQL Input [%s]\n", g_module_error());
		return -1;
	}

	if (!g_module_symbol(handle, "new_input_filter", (gpointer)&new_input_filter)) {
		g_free(name_copy);
		g_module_close(handle);
		r_error(r, "Could Not Load MYSQL Input [%s]\n", g_module_error());
		return -1;
	}

	input = new_input_filter(r);
#else
	input = rlib_mysql_new_input_filter(r);
#endif
	if (input == NULL) {
#ifndef HAVE_MYSQL_BUILTIN
		g_module_close(handle);
#endif
		g_free(name_copy);
		return -1;
	}

	if (database_host) {
		char *tmp, *port_s;
		host_copy = g_strdup(database_host);
		tmp = strchr(host_copy, ':');
		if (tmp) {
			*tmp = '\0';
			port_s = tmp + 1;
			port = atoi(port_s);
		}
	} else
		host_copy = NULL;

	if (!input->connect_with_credentials(input, host_copy, port, database_user, database_password, database_database)) {
		g_free(host_copy);
		g_free(name_copy);
		input->free(input);
#ifndef HAVE_MYSQL_BUILTIN
		g_module_close(handle);
#endif
		r_error(r,"ERROR: Could not connect to MYSQL\n");
		return -1;
	}

	g_free(host_copy);

	r->inputs[r->inputs_count].name = name_copy;
	r->inputs[r->inputs_count].input = input;
#ifndef HAVE_MYSQL_BUILTIN
	r->inputs[r->inputs_count].handle = handle;
#else
	r->inputs[r->inputs_count].handle = NULL;
#endif
	r->inputs[r->inputs_count].input->info.encoder = NULL;
	r->inputs_count++;

	return 0;
#endif
}

DLL_EXPORT_SYM gboolean rlib_add_datasource_mysql_from_group(rlib *r UNUSED, const gchar *input_name UNUSED, const gchar *group UNUSED) {
#ifndef HAVE_MYSQL
	return -1;
#else
#ifndef HAVE_MYSQL_BUILTIN
	GModule* handle;
	gpointer (*new_input_filter)(struct rlib *);
#endif
	struct input_filter *input;
	gchar *name_copy;

	if (r->inputs_count == MAX_INPUT_FILTERS)
		return -1;

	name_copy = g_strdup(input_name);
	if (name_copy == NULL) {
		r_error(r, "rlib_add_datasource_mysql_private: Out of memory!\n");
		return -1;
	}

#ifndef HAVE_MYSQL_BUILTIN
	handle = g_module_open("libr-mysql", 2);
	if (!handle) {
		g_free(name_copy);
		r_error(r,"Could Not Load MYSQL Input [%s]\n", g_module_error());
		return -1;
	}

	if (!g_module_symbol(handle, "new_input_filter", (gpointer)&new_input_filter)) {
		g_module_close(handle);
		g_free(name_copy);
		r_error(r, "Could Not Load MYSQL Input [%s]\n", g_module_error());
		return -1;
	}

	input = new_input_filter(r);
#else
	input = rlib_mysql_new_input_filter(r);
#endif
	if (input == NULL) {
#ifndef HAVE_MYSQL_BUILTIN
		g_module_close(handle);
#endif
		g_free(name_copy);
		return -1;
	}

	if (!input->connect_with_connstr(input, group)) {
		g_free(name_copy);
		input->free(input);
#ifndef HAVE_MYSQL_BUILTIN
		g_module_close(handle);
#endif
		r_error(r,"ERROR: Could not connect to MYSQL\n");
		return -1;
	}

	r->inputs[r->inputs_count].name = name_copy;
	r->inputs[r->inputs_count].input = input;
#ifndef HAVE_MYSQL_BUILTIN
	r->inputs[r->inputs_count].handle = handle;
#else
	r->inputs[r->inputs_count].handle = NULL;
#endif
	r->inputs[r->inputs_count].input->info.encoder = NULL;
	r->inputs_count++;

	return 0;
#endif
}

DLL_EXPORT_SYM gboolean rlib_add_datasource_postgres(rlib *r UNUSED, const gchar *input_name UNUSED, const gchar *conn UNUSED) {
#ifndef HAVE_POSTGRES
	return -1;
#else
#ifndef HAVE_POSTGRES_BUILTIN
	GModule* handle;
	gpointer (*new_input_filter)(struct rlib *);
#endif
	gchar *name_copy;
	struct input_filter *input;

	if (r->inputs_count == MAX_INPUT_FILTERS)
		return -1;

	name_copy = g_strdup(input_name);
	if (name_copy == NULL) {
		r_error(r, "rlib_add_datasource_postgres: Out of memory!\n");
		return -1;
	}

#ifndef HAVE_POSTGRES_BUILTIN
	handle = g_module_open("libr-postgres", 2);
	if (!handle) {
		g_free(name_copy);
		r_error(r,"Could Not Load POSTGRES Input [%s]\n", g_module_error());
		return -1;
	}

	if (!g_module_symbol(handle, "new_input_filter", (gpointer)&new_input_filter)) {
		g_module_close(handle);
		g_free(name_copy);
		r_error(r, "Could Not Load POSTGRES Input [%s]\n", g_module_error());
		return -1;
	}

	input = new_input_filter(r);
#else
	input = rlib_postgres_new_input_filter(r);
#endif
	if (input == NULL) {
#ifndef HAVE_POSTGRES_BUILTIN
		g_module_close(handle);
#endif
		g_free(name_copy);
		return -1;
	}

	if (!input->connect_with_connstr(input, conn)) {
		g_free(name_copy);
		input->free(input);
#ifndef HAVE_POSTGRES_BUILTIN
		g_module_close(handle);
#endif
		r_error(r,"ERROR: Could not connect to POSTGRES\n");
		return -1;
	}

	r->inputs[r->inputs_count].name = name_copy;
	r->inputs[r->inputs_count].input = input;
#ifndef HAVE_POSTGRES_BUILTIN
	r->inputs[r->inputs_count].handle = handle;
#else
	r->inputs[r->inputs_count].handle = NULL;
#endif
	r->inputs[r->inputs_count].input->info.encoder = NULL;
	r->inputs_count++;

	return 0;
#endif
}

DLL_EXPORT_SYM gboolean rlib_add_datasource_odbc(rlib *r UNUSED, const gchar *input_name UNUSED, const gchar *source UNUSED, const gchar *user UNUSED, const gchar *password UNUSED) {
#ifndef HAVE_ODBC
	return -1;
#else
#ifndef HAVE_ODBC_BUILTIN
	GModule* handle;
	gpointer (*new_input_filter)(struct rlib *);
#endif
	gchar *name_copy;
	struct input_filter *input;

	if (r->inputs_count == MAX_INPUT_FILTERS)
		return -1;

	name_copy = g_strdup(input_name);
	if (name_copy == NULL) {
		r_error(r, "rlib_add_datasource_odbc: Out of memory!\n");
		return -1;
	}

#ifndef HAVE_ODBC_BUILTIN
	handle = g_module_open("libr-odbc", 2);
	if (!handle) {
		g_free(name_copy);
		r_error(r,"Could Not Load ODBC Input [%s]\n", g_module_error());
		return -1;
	}

	if (!g_module_symbol(handle, "new_input_filter", (gpointer)&new_input_filter)) {
		g_module_close(handle);
		g_free(name_copy);
		r_error(r, "Could Not Load ODBC Input [%s]\n", g_module_error());
		return -1;
	}

	input = new_input_filter(r);
#else
	input = rlib_odbc_new_input_filter(r);
#endif
	if (input == NULL) {
#ifndef HAVE_ODBC_BUILTIN
		g_module_close(handle);
#endif
		g_free(name_copy);
		return -1;
	}

	if (!input->connect_local_with_credentials(input, source, user, password)) {
		g_free(name_copy);
		input->free(input);
#ifndef HAVE_ODBC_BUILTIN
		g_module_close(handle);
#endif
		r_error(r,"ERROR: Could not connect to ODBC\n");
		return -1;
	}

	r->inputs[r->inputs_count].name = name_copy;
	r->inputs[r->inputs_count].input = input;
#ifndef HAVE_ODBC_BUILTIN
	r->inputs[r->inputs_count].handle = handle;
#else
	r->inputs[r->inputs_count].handle = NULL;
#endif
	r->inputs[r->inputs_count].input->info.encoder = NULL;
	r->inputs_count++;

	return 0;
#endif
}

DLL_EXPORT_SYM gboolean rlib_add_datasource_xml(rlib *r, const gchar *input_name) {
	struct input_filter *input;
	gchar *name_copy;

	if (r->inputs_count == MAX_INPUT_FILTERS)
		return -1;

	name_copy = g_strdup(input_name);
	if (name_copy == NULL) {
		r_error(r, "rlib_add_datasource_xml: Out of memory!\n");
		return -1;
	}

	input = rlib_xml_new_input_filter(r);
	if (input == NULL) {
		g_free(name_copy);
		return -1;
	}

	/* No need to connect the datasource */

	r->inputs[r->inputs_count].name = g_strdup(input_name);
	r->inputs[r->inputs_count].input = input;
	r->inputs[r->inputs_count].handle = NULL;
	r->inputs[r->inputs_count].input->info.encoder = NULL;
	r->inputs_count++;
	return 0;
}

DLL_EXPORT_SYM gboolean rlib_add_datasource_csv(rlib *r, const gchar *input_name) {
	struct input_filter *input;
	gchar *name_copy;

	if (r->inputs_count == MAX_INPUT_FILTERS)
		return -1;

	name_copy = g_strdup(input_name);
	if (name_copy == NULL) {
		r_error(r, "rlib_add_datasource_xml: Out of memory!\n");
		return -1;
	}

	input = rlib_csv_new_input_filter(r);
	if (input == NULL) {
		g_free(name_copy);
		return -1;
	}

	/* No need to connect the datasource */

	r->inputs[r->inputs_count].input = input;
	r->inputs[r->inputs_count].name = name_copy;
	r->inputs[r->inputs_count].handle = NULL;
	r->inputs[r->inputs_count].input->info.encoder = NULL;
	r->inputs_count++;
	return 0;
}
