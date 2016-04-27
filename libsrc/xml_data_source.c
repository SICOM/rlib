/*
 *  Copyright (C) 2003-2006 SICOM Systems, INC.
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

#define INPUT_PRIVATE(input) (((struct _private *)input->private))

struct rlib_xml_results {
	xmlNodePtr data;
	xmlNodePtr first_row;
	xmlNodePtr last_row;
	xmlNodePtr this_row;
	xmlNodePtr first_field;
	gint isdone;
};

struct _private {
	xmlDocPtr doc;
};

gpointer rlib_xml_connect(gpointer input_ptr) {
	return NULL;
}

static gint rlib_xml_input_close(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	xmlFreeDoc(INPUT_PRIVATE(input)->doc);
	INPUT_PRIVATE(input)->doc = NULL;

	return 0;
}

static const gchar* rlib_xml_get_error(gpointer input_ptr) {
	return "No error information";
}


static gint rlib_xml_first(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_xml_results *result = result_ptr;

	result->this_row = result->first_row;
	if (result->this_row == NULL) {
		result->isdone = TRUE;
		return FALSE;
	}

	result->isdone = FALSE;
	return TRUE;
}

static gint rlib_xml_next(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_xml_results *result = result_ptr;
	xmlNodePtr row;

	if (result->isdone)
		return FALSE;

	for (row = result->this_row->next; row != NULL && xmlStrcmp(row->name, (const xmlChar *) "row") != 0; row = row->next);

	if (row == NULL) {
		result->isdone = TRUE;
		return FALSE;
	}

	result->this_row = row;
	result->isdone = FALSE;
	return TRUE;
}

static gint rlib_xml_isdone(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_xml_results *result = result_ptr;
	return result->isdone;
}

static gint rlib_xml_previous(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_xml_results *result = result_ptr;

	if (result->this_row == NULL)
		return FALSE;

	if (result->this_row == result->first_row)
		return FALSE;

	result->isdone = FALSE;

	for (result->this_row = result->this_row->prev; xmlStrcmp(result->this_row->name, (const xmlChar *) "row") != 0; result->this_row = result->this_row->prev);

	return TRUE;
}

static gint rlib_xml_last(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_xml_results *result = result_ptr;

	result->this_row = result->last_row;
	if (result->this_row == NULL)
		result->isdone = TRUE;

	return TRUE;
}

static gchar * rlib_xml_get_field_value_as_string(gpointer input_ptr, gpointer result_ptr, gpointer field_ptr) {
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

static gpointer rlib_xml_resolve_field_pointer(gpointer input_ptr, gpointer result_ptr, gchar *name) { 
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

void * xml_new_result_from_query(gpointer input_ptr, gchar *query) {
	struct input_filter *input = input_ptr;
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

	file = get_filename(input->r, query, -1, FALSE);
	doc = xmlReadFile(file, NULL, XML_PARSE_XINCLUDE);
	g_free(file);
	xmlXIncludeProcess(doc);

	if (doc == NULL) {
		r_error(NULL,"xmlParseError\n");
		return NULL;
	}

	INPUT_PRIVATE(input)->doc = doc;

	cur = xmlDocGetRootElement(INPUT_PRIVATE(input)->doc);
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
	for (cur = rows->xmlChildrenNode; cur; cur = cur->next) {
		if (xmlStrcmp(cur->name, (const xmlChar *) "row") == 0) {
			if (first_row == NULL)
				first_row = cur;
			last_row = cur;
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
	return results;
}

static void rlib_xml_rlib_free_result(gpointer input_ptr, gpointer result_ptr) {
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

	input = g_malloc(sizeof(struct input_filter));
	input->private = g_malloc(sizeof(struct _private));
	memset(input->private, 0, sizeof(struct _private));
	input->r = r;
	input->input_close = rlib_xml_input_close;
	input->first = rlib_xml_first;
	input->next = rlib_xml_next;
	input->previous = rlib_xml_previous;
	input->last = rlib_xml_last;
	input->isdone = rlib_xml_isdone;
	input->get_error = rlib_xml_get_error;
	input->new_result_from_query = xml_new_result_from_query;
	input->get_field_value_as_string = rlib_xml_get_field_value_as_string;
	input->resolve_field_pointer = rlib_xml_resolve_field_pointer;
	input->free = rlib_xml_free_input_filter;
	input->free_result = rlib_xml_rlib_free_result;
	return input;
}
