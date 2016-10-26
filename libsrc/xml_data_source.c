/*
 *  Copyright (C) 2003-2016 SICOM Systems, INC.
 * 
 *  Authors: Jeremy Lee <jlee@platinumtel.com> 
 *           Warren Smith <wsmith@platinumtel.com>
 *           Bob Doan <bdoan@sicompos.com>
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
 * Built in XML Input Data Source
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/xmlversion.h>
#include <libxml/xmlmemory.h>
#include <libxml/xinclude.h>

#include "glib.h"
#include "rlib-internal.h"
#include "rlib_input.h"

#define QUERY_PRIVATE(query) (((struct _query_private *)query->private))

struct rlib_xml_results {
	xmlNodePtr data;
	xmlNodePtr first_row;
	xmlNodePtr last_row;
	xmlNodePtr this_row;
	xmlNodePtr first_field;
	gint rows;
	gint atstart;
	gint isdone;
};

struct _query_private {
	xmlDocPtr doc;
};

static gint rlib_xml_connect(gpointer input_ptr UNUSED, const gchar *connstr UNUSED) {
	return 0;
}

static gint rlib_xml_input_close(gpointer input_ptr UNUSED) {
	return 0;
}

static const gchar* rlib_xml_get_error(gpointer input_ptr UNUSED) {
	return "No error information";
}


static void rlib_xml_start(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_xml_results *result = result_ptr;

	if (result == NULL)
		return;

	result->this_row = NULL;
	result->atstart = TRUE;
	result->isdone = FALSE;
}

static gint rlib_xml_next(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_xml_results *result = result_ptr;
	xmlNodePtr row;

	if (result == NULL)
		return FALSE;

	if (result->isdone)
		return FALSE;

	if (result->atstart) {
		result->atstart = FALSE;
		row = result->first_row;
	} else
		row = result->this_row->next;

	while (row != NULL && xmlStrcmp(row->name, (const xmlChar *) "row") != 0)
		row = row->next;

	result->this_row = row;
	result->isdone = (row == NULL);
	return !result->isdone;
}

static gint rlib_xml_isdone(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_xml_results *result = result_ptr;

	if (result == NULL)
		return FALSE;

	return result->isdone;
}

static gint rlib_xml_previous(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_xml_results *result = result_ptr;
	xmlNodePtr row;

	if (result == NULL)
		return FALSE;

	if (result->atstart)
		return FALSE;

	if (result->isdone)
		row = result->last_row;
	else
		row = result->this_row->prev;

	while (row != NULL && xmlStrcmp(row->name, (const xmlChar *) "row") != 0)
		row = row->prev;

	result->atstart = (row == NULL);
	result->isdone = FALSE;
	return !result->atstart;
}

static gint rlib_xml_last(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_xml_results *result = result_ptr;

	result->this_row = result->last_row;
	if (result->this_row == NULL)
		result->isdone = TRUE;

	return TRUE;
}

static gchar * rlib_xml_get_field_value_as_string(gpointer input_ptr UNUSED, gpointer result_ptr, gpointer field_ptr) {
	struct rlib_xml_results *result = result_ptr;
	gint field_index = GPOINTER_TO_INT(field_ptr);
	xmlNodePtr col;
	xmlNodePtr field_value;

	if (result_ptr == NULL)
		return (gchar *)"";

	if (result->this_row == NULL)
		return (gchar *)"";

	field_value = NULL;
	for (col = result->this_row->xmlChildrenNode; col != NULL && field_index > 0; col = col->next) {
		if (xmlStrcmp(col->name, (const xmlChar *) "col") == 0) {
			if (field_index-- == 1) {
				field_index = 0;
				field_value = col;
			}
		}
	}

	if (field_value == NULL)
		return (gchar *)"";

	if (field_value->xmlChildrenNode == NULL)
		return (gchar *)"";

	return (gchar *)field_value->xmlChildrenNode->content;
}

static gpointer rlib_xml_resolve_field_pointer(gpointer input_ptr UNUSED, gpointer result_ptr, gchar *name) { 
	struct rlib_xml_results *results = result_ptr;
	xmlNodePtr field;
	gint field_index = 0;

	for (field = results->first_field; field != NULL; field = field->next) {
		if (xmlStrcmp(field->name, (const xmlChar *) "field") == 0) {
			++field_index;

			if (xmlStrcmp(field->xmlChildrenNode->content, (const xmlChar *)name) == 0)
				return GINT_TO_POINTER(field_index);
		}
	}

	return NULL;
}

static void *xml_new_result_from_query(gpointer input_ptr, gpointer query_ptr) {
	struct input_filter *input = input_ptr;
	struct rlib_query *query = query_ptr;
	struct rlib_xml_results *results;
	xmlNodePtr cur;
	xmlNodePtr data;
	xmlNodePtr rows = NULL;
	xmlNodePtr fields = NULL;
	xmlNodePtr first_row;
	xmlNodePtr last_row;
	xmlNodePtr first_field;
	xmlDocPtr doc;
	gchar *file;
	gint nrows = 0;

	file = get_filename(input->r, query->sql, -1, FALSE, FALSE);
	doc = xmlReadFile(file, NULL, XML_PARSE_XINCLUDE);
	g_free(file);

	if (doc == NULL) {
		r_error(NULL,"xmlParseError\n");
		return NULL;
	}

	xmlXIncludeProcess(doc);

	QUERY_PRIVATE(query)->doc = doc;

	cur = xmlDocGetRootElement(QUERY_PRIVATE(query)->doc);
	if (cur == NULL) {
		r_error(NULL,"xmlParseError \n");
		return NULL;
	} else if (xmlStrcmp(cur->name, (const xmlChar *) "data") != 0) {
		r_error(NULL,"document error: 'data' expected, '%s'found\n", cur->name);
		return NULL;
	}

	data = cur;

	for (cur = cur->xmlChildrenNode; cur; cur = cur->next) {
		if (xmlStrcmp(cur->name, (const xmlChar *) "rows") == 0)
			rows  = cur;
		else if (xmlStrcmp(cur->name, (const xmlChar *) "fields") == 0)
			fields = cur;
	}

	if (rows == NULL) {
		r_error(NULL,"document error: 'rows' not found\n");
		return NULL;
	} else if (fields == NULL) {
		r_error(NULL,"document error: 'fields' not found\n");
		return NULL;
	}

	first_row = NULL;
	last_row = NULL;
	nrows = 0;
	for (cur = rows->xmlChildrenNode; cur; cur = cur->next) {
		if (xmlStrcmp(cur->name, (const xmlChar *) "row") == 0) {
			if (first_row == NULL)
				first_row = cur;
			last_row = cur;
			nrows++;
		}
	}

	if (first_row == NULL) {
		r_error(NULL,"'row' count is zero\n");
		return NULL;
	}

	first_field = NULL;
	for (cur = fields->xmlChildrenNode; cur && first_field == NULL; cur = cur->next) {
		if (xmlStrcmp(cur->name, (const xmlChar *) "field") == 0)
			first_field = cur;
	}

	if (first_field == NULL) {
		r_error(NULL,"'field' count is zero\n");
		return NULL;
	}

	results = g_malloc(sizeof(struct rlib_xml_results));
	if (results == NULL)
		return NULL;

	results->data = data;
	results->first_row = first_row;
	results->last_row = last_row;
	results->this_row = first_row;
	results->first_field = first_field;
	results->isdone = FALSE;
	results->rows = nrows;
	return results;
}

static void rlib_xml_free_query(gpointer input_ptr UNUSED, gpointer query_ptr) {
	struct rlib_query *query = query_ptr;

	xmlFreeDoc(QUERY_PRIVATE(query)->doc);
	QUERY_PRIVATE(query)->doc = NULL;
}


static void rlib_xml_rlib_free_result(gpointer input_ptr UNUSED, gpointer result_ptr) {
	struct rlib_xml_results *results = result_ptr;
	g_free(results);
}

static gint rlib_xml_free_input_filter(gpointer input_ptr){
	struct input_filter *input = input_ptr;
	g_free(input->private);
	g_free(input);
	return 0;
}

gpointer rlib_xml_new_input_filter(rlib *r) {
	struct input_filter *input;

	input = g_malloc0(sizeof(struct input_filter));
	input->r = r;
	input->connect_with_connstr = rlib_xml_connect;
	input->input_close = rlib_xml_input_close;
	input->start = rlib_xml_start;
	input->next = rlib_xml_next;
	input->previous = rlib_xml_previous;
	input->last = rlib_xml_last;
	input->isdone = rlib_xml_isdone;
	input->get_error = rlib_xml_get_error;
	input->new_result_from_query = xml_new_result_from_query;
	input->get_field_value_as_string = rlib_xml_get_field_value_as_string;
	input->resolve_field_pointer = rlib_xml_resolve_field_pointer;
	input->free = rlib_xml_free_input_filter;
	input->free_query = rlib_xml_free_query;
	input->free_result = rlib_xml_rlib_free_result;
	return input;
}
