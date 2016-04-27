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

#include <unistd.h>
#include <glib.h>

#include "rlib-internal.h"

static gchar * rlib_c_resolve_memory_variable(gchar *name) {
	return (gchar *)g_getenv(name);
}

static gint rlib_c_write_output(gchar *data, gint len) {
	return write(1, data, len);
}

void rlib_c_free(rlib *r) {
	g_free(ENVIRONMENT(r));
}


void rlib_new_c_environment(rlib *r) {
	ENVIRONMENT(r) = g_malloc(sizeof(struct environment_filter));

	ENVIRONMENT(r)->rlib_resolve_memory_variable = rlib_c_resolve_memory_variable;
	ENVIRONMENT(r)->rlib_write_output = rlib_c_write_output;
	ENVIRONMENT(r)->free = rlib_c_free;
}

DLL_EXPORT_SYM struct environment_filter *rlib_get_environment(rlib *r) {
	return ENVIRONMENT(r);
}
