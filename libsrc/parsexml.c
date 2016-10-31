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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/xmlversion.h>
#include <libxml/xmlmemory.h>
#include <libxml/xinclude.h>

#include <glib.h>

#include "rlib-internal.h"

#define UNUSED __attribute__((unused))

void dump_part(struct rlib_part *part);

static struct rlib_report *parse_report_file(rlib *r, gboolean allow_fail, int report_index, gchar *filename, gchar *query);

void safestrncpy(gchar *dest, gchar *str, int n) {
	if (!dest) return;
	*dest = '\0';
	if (str) g_strlcpy(dest, str, n);
}


#if DISABLE_UTF8
static void utf8_to_8813(rlib *r, gchar *dest, gchar *str) {
	size_t len;
	size_t slen;
	gchar *olddest = dest;
	if(str != NULL && str[0] != 0) {
		if(r->xml_encoder != NULL && r->xml_encoder != (void *)-1) {
			slen = strlen(str);
			len = slen + sizeof(gchar);
			memset(dest, 0, len);
			g_iconv(r->xml_encoder, (char **)&str, &slen, &olddest, &len);
		} else {
			strcpy(dest, str);
		}
	} else {
		dest[0] = 0;
	}
}
#endif

static int ignoreElement(const char *elname) {
	const xmlChar	*xmlname = (xmlChar *)elname;
	int result = FALSE;
	if (!xmlStrcmp(xmlname, (xmlChar *)"comment")
		|| !xmlStrcmp(xmlname, (xmlChar *)"text")
		|| !xmlStrcmp(xmlname, (xmlChar *)"include")) {
		result = TRUE;
	}
	return result;
}
static void get_both(struct rlib_from_xml *data, xmlNodePtr cur, gchar *name) {
	xmlFree(data->xml);
	data->xml = xmlGetProp(cur, (const xmlChar *) name);
	data->line = xmlGetLineNo(cur);
}

static struct rlib_report_image * parse_image(xmlNodePtr cur) {
	struct rlib_report_image *ri = g_new0(struct rlib_report_image, 1);
	get_both(&ri->xml_value, cur, "value");
	get_both(&ri->xml_type, cur, "type");
	get_both(&ri->xml_width, cur, "width");
	get_both(&ri->xml_height, cur, "height");
	get_both(&ri->xml_textwidth, cur, "textwidth");
	return ri;
}

static struct rlib_report_barcode * parse_barcode(xmlNodePtr cur) {
	struct rlib_report_barcode *rb = g_new0(struct rlib_report_barcode, 1);
	get_both(&rb->xml_value, cur, "value");
	get_both(&rb->xml_type, cur, "type");
	get_both(&rb->xml_width, cur, "width");
	get_both(&rb->xml_height, cur, "height");
	return rb;
}

static struct rlib_element *parse_line_array(rlib *r, gboolean allow_fail, xmlDocPtr doc, xmlNodePtr cur, gint *error) __attribute__((nonnull(1, 3, 4, 5)));
static struct rlib_element *parse_line_array(rlib *r, gboolean allow_fail, xmlDocPtr doc, xmlNodePtr cur, gint *error) {
	struct rlib_element *e = NULL;
	struct rlib_element *last = NULL;
	xmlChar *sp;
	gint count = 0;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		struct rlib_element *current = NULL;
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "field"))) {
			struct rlib_report_field *f = g_new0(struct rlib_report_field, 1);
			current = g_new0(struct rlib_element, 1);

			if (f == NULL || current == NULL) {
				r_error(r, "Out of memory\n");
				g_free(f);
				g_free(current);
				rlib_free_line_elements(r, e);
				*error = 1;
				return NULL;
			}

			sp = xmlGetProp(cur, (const xmlChar *) "value");
			if (sp == NULL) {
				r_error(r, "Line: %ld - <field> is missing 'value' attribute. (name %s)\n", xmlGetLineNo(cur), cur->name);
				g_free(f);
				g_free(current);
				rlib_free_line_elements(r, e);
				*error = 1;
				return NULL;
			}
			f->value = g_malloc0(strlen((char *)sp) + sizeof(gchar));
#if DISABLE_UTF8
			utf8_to_8813(r, f->value, (gchar *)sp);
#else
			safestrncpy(f->value, (gchar *)sp, strlen((char *)sp)+1);
#endif
			xmlFree(sp);
			/* TODO: we need to utf to 8813 all string values in single quotes */
			f->value_line_number = xmlGetLineNo(cur);
			get_both(&f->xml_align, cur, "align");
			get_both(&f->xml_bgcolor, cur, "bgcolor");
			get_both(&f->xml_color, cur, "color");
			get_both(&f->xml_width, cur, "width");
			get_both(&f->xml_bold, cur, "bold");
			get_both(&f->xml_italics, cur, "italics");
			get_both(&f->xml_format, cur, "format");
			get_both(&f->xml_link, cur, "link");
			get_both(&f->xml_translate, cur, "translate");
			get_both(&f->xml_col, cur, "col");
			get_both(&f->xml_memo, cur, "memo");
			get_both(&f->xml_memo_max_lines, cur, "memo_max_lines");
			get_both(&f->xml_memo_wrap_chars, cur, "memo_wrap_chars");

			current->data = f;
			current->type = RLIB_ELEMENT_FIELD;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "literal"))) {
			struct rlib_report_literal *t = g_new0(struct rlib_report_literal, 1);
			current = g_new0(struct rlib_element, 1);

			if (t == NULL || current == NULL) {
				r_error(r, "Out of memory\n");
				g_free(t);
				g_free(current);
				rlib_free_line_elements(r, e);
				*error = 1;
				return NULL;
			}

			sp = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (sp != NULL)
				t->value = g_malloc0(strlen((char *)sp) + sizeof(gchar));
			else
				t->value = g_malloc0(sizeof(gchar));
#if DISABLE_UTF8
			utf8_to_8813(r, t->value, (char *)sp);
#else
			if (sp != NULL)
				safestrncpy(t->value, (gchar *)sp, strlen((char *)sp)+1);
#endif
			xmlFree(sp);
			get_both(&t->xml_align, cur, "align");
			get_both(&t->xml_bgcolor, cur, "bgcolor");
			get_both(&t->xml_color, cur, "color");
			get_both(&t->xml_width, cur, "width");
			get_both(&t->xml_bold, cur, "bold");
			get_both(&t->xml_italics, cur, "italics");
			get_both(&t->xml_link, cur, "link");
			get_both(&t->xml_translate, cur, "translate");
			get_both(&t->xml_col, cur, "col");

			current->data = t;
			current->type = RLIB_ELEMENT_LITERAL;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Image"))) {
			struct rlib_report_image *ri = parse_image(cur);
			current = g_new0(struct rlib_element, 1);

			if (ri == NULL || current == NULL) {
				r_error(r, "Out of memory\n");
				g_free(ri);
				g_free(current);
				rlib_free_line_elements(r, e);
				*error = 1;
				return NULL;
			}

			current->data = ri;
			current->type = RLIB_ELEMENT_IMAGE;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Barcode"))) {
			struct rlib_report_barcode *rb = parse_barcode(cur);
			current = g_new0(struct rlib_element, 1);

			if (rb == NULL || current == NULL) {
				r_error(r, "Out of memory\n");
				g_free(rb);
				g_free(current);
				rlib_free_line_elements(r, e);
				*error = 1;
				return NULL;
			}

			current->data = rb;
			current->type = RLIB_ELEMENT_BARCODE;
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <Line> \n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_line_elements(r, e);
				*error = 1;
				return NULL;
			}
		}

		if (current) {
			count++;
			if (e == NULL) {
				e = current;
				last = e;
			} else {
				last->next = current;
				last = current;
			}
		}
		cur = cur->next;
	}
	*error = 0;
	return e;
}

struct rlib_report_output * report_output_new(gint type, gpointer data) {
	struct rlib_report_output *ro = g_malloc0(sizeof(struct rlib_report_output));
	ro->type = type;
	ro->data = data;
	return ro;
}

static struct rlib_element *parse_report_output(rlib *r, gboolean allow_fail, xmlDocPtr doc, xmlNodePtr cur, gint *error) {
	struct rlib_element *e = g_new0(struct rlib_element, 1);
	struct rlib_report_output_array *roa = g_new0(struct rlib_report_output_array, 1);
	e->next = NULL;
	e->data = roa;
	roa->xml_page.xml = NULL;
	if (cur != NULL && (!xmlStrcmp(cur->name, (const xmlChar *) "Output"))) {
		get_both(&roa->xml_page, cur, "page");
		get_both(&roa->xml_suppress, cur, "suppress");
		cur = cur->xmlChildrenNode;
	}
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "Line"))) {
			struct rlib_report_lines *rl = g_new0(struct rlib_report_lines, 1);
			struct rlib_element *ptr;
			gint error1;

			get_both(&rl->xml_bgcolor, cur, "bgcolor");
			get_both(&rl->xml_color, cur, "color");
			get_both(&rl->xml_bold, cur, "bold");
			get_both(&rl->xml_italics, cur, "italics");
			get_both(&rl->xml_font_size, cur, "fontSize");
			get_both(&rl->xml_suppress, cur, "suppress");

			if (rl->xml_font_size.xml == NULL)
				rl->font_point = -1;
			else
				rl->font_point = atoi((const char *)rl->xml_font_size.xml);

			ptr = parse_line_array(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_output(r, e);
				rlib_free_lines(r, rl);
				*error = 1;
				return NULL;
			}

			rl->e = ptr;
			roa->chain = g_slist_append(roa->chain, report_output_new(RLIB_REPORT_PRESENTATION_DATA_LINE, rl));
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "HorizontalLine"))) {
			struct rlib_report_horizontal_line *rhl = g_new0(struct rlib_report_horizontal_line, 1);

			get_both(&rhl->xml_bgcolor, cur, "bgcolor");
			get_both(&rhl->xml_size, cur, "size");
			get_both(&rhl->xml_indent, cur, "indent");
			get_both(&rhl->xml_length, cur, "length");
			get_both(&rhl->xml_font_size, cur, "fontSize");
			get_both(&rhl->xml_suppress, cur, "suppress");

			if(rhl->xml_font_size.xml == NULL)
				rhl->font_point = -1;
			else
				rhl->font_point = atoi((const char *)rhl->xml_font_size.xml);
			roa->chain = g_slist_append(roa->chain, report_output_new(RLIB_REPORT_PRESENTATION_DATA_HR, rhl));
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Image"))) {
			struct rlib_report_image *ri = parse_image(cur);
			roa->chain = g_slist_append(roa->chain, report_output_new(RLIB_REPORT_PRESENTATION_DATA_IMAGE, ri));
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <Output>. Expected: Line, HorizontalLine or Image On Line \n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_output(r, e);
				*error = 1;
				return NULL;
			}
		}
		cur = cur->next;
	}
	*error = 0;
	return e;
}

static struct rlib_element *parse_report_outputs(rlib *r, gboolean allow_fail, xmlDocPtr doc, xmlNodePtr cur, gint *error) {
	struct rlib_element *e = NULL, *last = NULL;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "Output"))) {
			gint error1;
			struct rlib_element *ptr = parse_report_output(r, allow_fail, doc, cur, &error1);

			if (error1) {
				rlib_free_output(r, e);
				*error = 1;
				return NULL;
			}

			if (last == NULL)
				e = last = ptr;
			else {
				last->next = ptr;
				last = ptr;
			}
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Invalid element: [%s] Expected: <Output> \n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_output(r, e);
				*error = 1;
				return NULL;
			}
		}
		cur = cur->next;
	}
	*error = 0;
	return e;
}

static struct rlib_element *parse_break_field(rlib *r, gboolean allow_fail, xmlNodePtr cur, gint *error) {
	struct rlib_element *e = g_malloc(sizeof(struct rlib_element));
	struct rlib_break_fields *bf = g_new0(struct rlib_break_fields, 1);
	e->next = NULL;
	e->data = bf;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "BreakField"))) {
			get_both(&bf->xml_value, cur, "value");
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <Break>. Expected BreakField \n", xmlGetLineNo(cur), cur->name );
			if (!allow_fail) {
				rlib_free_break_fields(r, e);
				*error = 1;
				return NULL;
			}
		}
		cur = cur->next;
	}
	*error = 0;
	return e;
}

static struct rlib_element *parse_report_break(rlib *r, gboolean allow_fail, struct rlib_report *report UNUSED, xmlDocPtr doc, xmlNsPtr ns UNUSED, xmlNodePtr cur, gint *error) {
	struct rlib_element *e = g_malloc0(sizeof(struct rlib_element));
	struct rlib_report_break *rb = g_new0(struct rlib_report_break, 1);
	struct rlib_element *last_field = NULL;
	gint error1;

	e->data = rb;
	get_both(&rb->xml_name, cur, "name");
	get_both(&rb->xml_newpage, cur, "newpage");
	get_both(&rb->xml_headernewpage, cur, "headernewpage");
	get_both(&rb->xml_suppressblank, cur, "suppressblank");

	cur = cur->xmlChildrenNode;
	rb->fields = NULL;
	rb->header = NULL;
	rb->footer = NULL;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "BreakHeader"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_breaks(r, e);
				*error = 1;
				return NULL;
			}
			if (rb->header) {
				r_error(r, "Line: %ld - Duplicate BreakHeader\n", xmlGetLineNo(cur));
				rlib_free_output(r, ptr);
				rlib_free_breaks(r, e);
				*error = 1;
				return NULL;
			}
			rb->header = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "BreakFooter"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_breaks(r, e);
				*error = 1;
				return NULL;
			}
			if (rb->footer) {
				r_error(r, "Line: %ld - Duplicate BreakFooter\n", xmlGetLineNo(cur));
				rlib_free_output(r, ptr);
				rlib_free_breaks(r, e);
				*error = 1;
				return NULL;
			}
			rb->footer = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "BreakFields"))) {
			struct rlib_element *ptr = parse_break_field(r, allow_fail, cur, &error1);
			if (error1) {
				rlib_free_breaks(r, e);
				*error = 1;
				return NULL;
			}
			if (last_field == NULL)
				rb->fields = last_field = ptr;
			else {
				last_field->next = ptr;
				last_field = ptr;
			}
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <ReportBreak>. Expected BreakHeader, BreakFooter, BreakFieldsn \n", xmlGetLineNo(cur), cur->name );
			if (!allow_fail) {
				rlib_free_breaks(r, e);
				*error = 1;
				return NULL;
			}
		}
		cur = cur->next;
	}
	*error = 0;
	return e;
}

static struct rlib_element *parse_report_breaks(rlib *r, gboolean allow_fail, struct rlib_report *report, xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, gint *error) __attribute__((warn_unused_result));
static struct rlib_element *parse_report_breaks(rlib *r, gboolean allow_fail, struct rlib_report *report, xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, gint *error) {
	struct rlib_element *e = NULL, *last = NULL;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "Break"))) {
			gint error1;
			struct rlib_element *ptr = parse_report_break(r, allow_fail, report, doc, ns, cur, &error1);

			if (error1) {
				rlib_free_breaks(r, e);
				*error = 1;
				return NULL;
			}

			if (last == NULL)
				e = last = ptr;
			else {
				last->next = ptr;
				last = ptr;
			}
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <Breaks>. Expected Break.\n", xmlGetLineNo(cur), cur->name );
			if (!allow_fail) {
				rlib_free_breaks(r, e);
				*error = 1;
				return NULL;
			}
		}
		cur = cur->next;
	}
	*error = 0;
	return e;
}

static struct rlib_report_detail *parse_detail(rlib *r, gboolean allow_fail, xmlDocPtr doc, xmlNodePtr cur) __attribute__((warn_unused_result));
static struct rlib_report_detail *parse_detail(rlib *r, gboolean allow_fail, xmlDocPtr doc, xmlNodePtr cur) {
	struct rlib_report_detail *detail = g_new0(struct rlib_report_detail, 1);
	gint error1;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "FieldHeaders"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_output(r, ptr);
				rlib_free_output(r, detail->headers);
				rlib_free_output(r, detail->fields);
				g_free(detail);
				return NULL;
			}
			if (detail->headers) {
				r_error(r, "Line: %ld - Duplicate FieldHeaders in <Detail>\n", xmlGetLineNo(cur));
				rlib_free_output(r, ptr);
				rlib_free_output(r, detail->headers);
				rlib_free_output(r, detail->fields);
				g_free(detail);
				return NULL;
			}
			detail->headers = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "FieldDetails"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_output(r, detail->headers);
				rlib_free_output(r, detail->fields);
				rlib_free_output(r, ptr);
				g_free(detail);
				return NULL;
			}
			if (detail->fields) {
				r_error(r, "Line: %ld - Duplicate FieldDetails in <Detail>\n", xmlGetLineNo(cur));
				rlib_free_output(r, detail->headers);
				rlib_free_output(r, detail->fields);
				rlib_free_output(r, ptr);
				g_free(detail);
				return NULL;
			}
			detail->fields = ptr;
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <Detail>. Expected FieldHeaders or FieldDetails\n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_output(r, detail->headers);
				rlib_free_output(r, detail->fields);
				g_free(detail);
				return NULL;
			}
		}
		cur = cur->next;
	}
	return detail;
}

static struct rlib_element *parse_alternate(rlib *r, gboolean allow_fail, xmlDocPtr doc, xmlNodePtr cur, gint *error) __attribute__((warn_unused_result));
static struct rlib_element *parse_alternate(rlib *r, gboolean allow_fail, xmlDocPtr doc, xmlNodePtr cur, gint *error) {
	struct rlib_element *nodata = NULL;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "NoData"))) {
			gint error1;
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_output(r, nodata);
				*error = 1;
				return NULL;
			}
			if (nodata) {
				r_error(r, "Line: %ld - Duplicate NoData in <Alternate>\n", xmlGetLineNo(cur));
				rlib_free_output(r, nodata);
				rlib_free_output(r, ptr);
				*error = 1;
				return NULL;
			}
			nodata = ptr;
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <Alternate>. Expected NoData\n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_output(r, nodata);
				*error = 1;
				return NULL;
			}
		}
		cur = cur->next;
	}
	*error = 0;
	return nodata;
}

static struct rlib_graph_plot * parse_graph_plots(xmlNodePtr cur) __attribute__((warn_unused_result));
static struct rlib_graph_plot * parse_graph_plots(xmlNodePtr cur) {
	struct rlib_graph_plot *gp = g_new0(struct rlib_graph_plot, 1);

	get_both(&gp->xml_axis, cur, "axis");
	get_both(&gp->xml_field, cur, "field");
	get_both(&gp->xml_label, cur, "label");
	get_both(&gp->xml_side, cur, "side");
	get_both(&gp->xml_disabled, cur, "disabled");
	get_both(&gp->xml_color, cur, "color");

	return gp;
}

static struct rlib_graph *parse_graph(rlib *r, gboolean allow_fail, xmlNodePtr cur) __attribute__((warn_unused_result));
static struct rlib_graph *parse_graph(rlib *r, gboolean allow_fail, xmlNodePtr cur) {
	struct rlib_graph *graph = g_new0(struct rlib_graph, 1);

	get_both(&graph->xml_name, cur, "name");
	get_both(&graph->xml_type, cur, "type");
	get_both(&graph->xml_subtype, cur, "subtype");
	get_both(&graph->xml_width, cur, "width");
	get_both(&graph->xml_height, cur, "height");
	get_both(&graph->xml_bold_titles, cur, "bold_titles");
	get_both(&graph->xml_title, cur, "title");
	get_both(&graph->xml_legend_bg_color, cur, "legend_bg_color");
	get_both(&graph->xml_legend_orientation, cur, "legend_orientation");
	get_both(&graph->xml_draw_x_line, cur, "draw_x_line");
	get_both(&graph->xml_draw_y_line, cur, "draw_y_line");
	get_both(&graph->xml_grid_color, cur, "grid_color");
	get_both(&graph->xml_x_axis_title, cur, "x_axis_title");
	get_both(&graph->xml_y_axis_title, cur, "y_axis_title");
	get_both(&graph->xml_y_axis_mod, cur, "y_axis_mod");
	get_both(&graph->xml_y_axis_title_right, cur, "y_axis_title_right");
	get_both(&graph->xml_y_axis_decimals, cur, "y_axis_decimals");
	get_both(&graph->xml_y_axis_decimals_right, cur, "y_axis_decimals_right");

	graph->plots = NULL;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "Plot"))) {
			graph->plots = g_slist_append(graph->plots, parse_graph_plots(cur));
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in  <Graph>. Expected Plot\n", xmlGetLineNo(cur), cur->name );
			if (!allow_fail) {
				rlib_free_graph(r, graph);
				return NULL;
			}
		}
		cur = cur->next;
	}

	return graph;
}

static struct rlib_chart_row *parse_chart_row(xmlNodePtr cur) {
	struct rlib_chart_row *cr = g_new0(struct rlib_chart_row, 1);

	get_both(&cr->xml_row, cur, "row");
	get_both(&cr->xml_bar_start, cur, "bar_start");
	get_both(&cr->xml_bar_end, cur, "bar_end");
	get_both(&cr->xml_label, cur, "label");
	get_both(&cr->xml_bar_label, cur, "bar_label");
	get_both(&cr->xml_bar_color, cur, "bar_color");
	get_both(&cr->xml_bar_label_color, cur, "bar_label_color");

	return cr;
}

static struct rlib_chart_header_row *parse_chart_header_row(xmlNodePtr cur) {
	struct rlib_chart_header_row *chr = g_new0(struct rlib_chart_header_row, 1);

	get_both(&chr->xml_query, cur, "query");
	get_both(&chr->xml_field, cur, "field");
	get_both(&chr->xml_colspan, cur, "colspan");

	return chr;
}

static struct rlib_chart *parse_chart(rlib *r, gboolean allow_fail, xmlNodePtr cur, gint *error) __attribute__((warn_unused_result));
static struct rlib_chart *parse_chart(rlib *r, gboolean allow_fail, xmlNodePtr cur, gint *error) {
	struct rlib_chart *chart = g_new0(struct rlib_chart, 1);
	gboolean have_row = FALSE;

	get_both(&chart->xml_name, cur, "name");
	get_both(&chart->xml_title, cur, "title");
	get_both(&chart->xml_cols, cur, "cols");
	get_both(&chart->xml_rows, cur, "rows");
	get_both(&chart->xml_cell_width, cur, "cell_width");
	get_both(&chart->xml_cell_height, cur, "cell_height");
	get_both(&chart->xml_cell_width_padding, cur, "cell_width_padding");
	get_both(&chart->xml_cell_height_padding, cur, "cell_height_padding");
	get_both(&chart->xml_label_width, cur, "label_width");
	get_both(&chart->xml_header_row, cur, "header_row");

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "HeaderRow"))) {
			struct rlib_chart_header_row *ptr = parse_chart_header_row(cur);
			if (chart->header_row) {
				r_error(r, "Line: %ld - Duplicate HeaderRow in <Chart>\n", xmlGetLineNo(cur));
				rlib_free_chart_header_row(r, ptr);
				rlib_free_chart(r, chart);
				*error = 1;
				return NULL;
			}
			chart->header_row = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Row"))) {
			struct rlib_chart_row *ptr = parse_chart_row(cur);
			if (chart->row) {
				r_error(r, "Line: %ld - Duplicate Row in <Chart>\n", xmlGetLineNo(cur));
				rlib_free_chart_row(r, ptr);
				rlib_free_chart(r, chart);
				*error = 1;
				return NULL;
			}
			chart->row = ptr;
			have_row = TRUE;
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in  <Chart>. Expected Row or HeaderRow\n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_chart(r, chart);
				*error = 1;
				return NULL;
			}
		}
		cur = cur->next;
	}

	if (!have_row) {
		r_error(r, "Line: %ld - No Row in <Chart>\n", xmlGetLineNo(cur));
		if (!allow_fail) {
			rlib_free_chart(r, chart);
			*error = 1;
			return NULL;
		}
	}

	*error = 0;
	return chart;
}

static struct rlib_element *parse_report_variable(rlib *r, gboolean allow_fail, xmlNodePtr cur) {
	struct rlib_element *e = g_malloc0(sizeof(struct rlib_report));
	struct rlib_report_variable *rv = g_new0(struct rlib_report_variable, 1);
	e->next = NULL;
	e->data = rv;

	get_both(&rv->xml_name, cur, "name");
	get_both(&rv->xml_str_type, cur, "type");
	get_both(&rv->xml_value, cur, "value");
	get_both(&rv->xml_resetonbreak, cur, "resetonbreak");
	get_both(&rv->xml_ignore, cur, "ignore");

	rv->type = RLIB_REPORT_VARIABLE_UNDEFINED;
	if(rv->xml_str_type.xml != NULL && rv->xml_str_type.xml[0] != '\0') {
		if(!strcmp((char *)rv->xml_str_type.xml, "expression") || !strcmp((char *)rv->xml_str_type.xml, "static"))
			rv->type = RLIB_REPORT_VARIABLE_EXPRESSION;
		else if(!strcmp((char *)rv->xml_str_type.xml, "count"))
			rv->type = RLIB_REPORT_VARIABLE_COUNT;
		else if(!strcmp((char *)rv->xml_str_type.xml, "sum"))
			rv->type = RLIB_REPORT_VARIABLE_SUM;
		else if(!strcmp((char *)rv->xml_str_type.xml, "average"))
			rv->type = RLIB_REPORT_VARIABLE_AVERAGE;
		else if(!strcmp((char *)rv->xml_str_type.xml, "lowest"))
			rv->type = RLIB_REPORT_VARIABLE_LOWEST;
		else if(!strcmp((char *)rv->xml_str_type.xml, "highest"))
			rv->type = RLIB_REPORT_VARIABLE_HIGHEST;
		else {
			r_error(r, "Line: %ld - Unknown report variable type [%s] in <Variable>\n", xmlGetLineNo(cur), rv->xml_str_type.xml);
			if (!allow_fail) {
				rlib_free_variables(r, e);
				return NULL;
			}
		}
	}

	return e;
}

static struct rlib_element *parse_report_variables(rlib *r, gboolean allow_fail, xmlNodePtr cur, gint *error) {
	struct rlib_element *e = NULL, *last = NULL;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "Variable"))) {
			struct rlib_element *ptr = parse_report_variable(r, allow_fail, cur);

			if (ptr == NULL) {
				rlib_free_variables(r, e);
				*error = 1;
				return NULL;
			}

			if (last == NULL)
				e = last = ptr;
			else {
				last->next = ptr;
				last = ptr;
			}
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <Variables>. Expected Variable.\n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_variables(r, e);
				*error = 1;
				return NULL;
			}
		}
		cur = cur->next;
	}
	*error = 0;
	return e;
}

static void parse_metadata(xmlNodePtr cur, GHashTable *ht) {
	struct rlib_metadata *metadata = g_new0(struct rlib_metadata, 1);
	gchar *name;

	name = (gchar *)xmlGetProp(cur, (const xmlChar *) "name");
	get_both(&metadata->xml_formula, cur, "value");
	if (name != NULL)
		g_hash_table_insert(ht, g_strdup(name), metadata);
	else {
		xmlFree(metadata->xml_formula.xml);
		g_free(metadata);
	}
	xmlFree(name);
	return;
}

static gint parse_metadata_item(rlib *r, gboolean allow_fail, xmlNodePtr cur, GHashTable *ht) {
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "MetaData"))) {
			parse_metadata(cur, ht);
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <MetaData>. Expected MetaData.\n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail)
				return -1;
		}
		cur = cur->next;
	}
	return 0;
}

static struct rlib_report *parse_report(rlib *r, gboolean allow_fail, struct rlib_part *part, xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, gchar *query) __attribute__((warn_unused_result));
static struct rlib_report *parse_report(rlib *r, gboolean allow_fail, struct rlib_part *part, xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, gchar *query) {
	struct rlib_report *report;
	gint error1;
/*	if (doc->encoding)
		g_strlcpy(report->xml_encoding_name, doc->encoding, sizeof(report->xml_encoding_name)); */

	report = g_new0(struct rlib_report, 1);
	if (report == NULL) {
		r_error(r, "Out of memory\n");
		return NULL;
	}

	while (cur && xmlIsBlankNode (cur))
		cur = cur -> next;

	if (cur == NULL) {
		r_error(r, "Empty report\n");
		if (allow_fail)
			return report;
		rlib_free_report(r, report);
		return NULL;
	}

	get_both(&report->xml_font_size, cur, "fontSize");
	if (query == NULL) {
		get_both(&report->xml_query, cur, "query");
	} else {
		report->xml_query.xml = xmlStrdup((xmlChar *)query);
		report->xml_query.line = -1;
	}

	get_both(&report->xml_orientation, cur, "orientation");
	get_both(&report->xml_top_margin, cur, "topMargin");
	get_both(&report->xml_left_margin, cur, "leftMargin");
	get_both(&report->xml_detail_columns, cur, "detail_columns");
	get_both(&report->xml_column_pad, cur, "column_pad");
	get_both(&report->xml_bottom_margin, cur, "bottomMargin");
	get_both(&report->xml_height, cur, "height");
	get_both(&report->xml_iterations, cur, "iterations");

	if (part) {
		if (xmlHasProp(cur, (const xmlChar *) "paperType") && part->xml_paper_type.xml == NULL)
			get_both(&part->xml_paper_type, cur, "paperType");
	}

	get_both(&report->xml_pages_across, cur, "pagesAcross");
	get_both(&report->xml_suppress_page_header_first_page, cur, "suppressPageHeaderFirstPage");
	get_both(&report->xml_suppress, cur, "suppress");
	get_both(&report->xml_uniquerow, cur, "uniquerow");

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "ReportHeader"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_report(r, report);
				return NULL;
			}
			if (report->report_header) {
				r_error(r, "Line: %ld - Duplicate ReportHeader in <Report>\n", xmlGetLineNo(cur));
				rlib_free_output(r, ptr);
				rlib_free_report(r, report);
				return NULL;
			}
			report->report_header = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "PageHeader"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_report(r, report);
				return NULL;
			}
			if (report->page_header) {
				r_error(r, "Line: %ld - Duplicate PageHeader in <Report>\n", xmlGetLineNo(cur));
				rlib_free_output(r, ptr);
				rlib_free_report(r, report);
				return NULL;
			}
			report->page_header = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "PageFooter"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_report(r, report);
				return NULL;
			}
			if (report->page_footer) {
				r_error(r, "Line: %ld - Duplicate PageFooter in <Report>\n", xmlGetLineNo(cur));
				rlib_free_output(r, ptr);
				rlib_free_report(r, report);
				return NULL;
			}
			report->page_footer = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "ReportFooter"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_report(r, report);
				return NULL;
			}
			if (report->report_footer) {
				r_error(r, "Line: %ld - Duplicate ReportFooter in <Report>\n", xmlGetLineNo(cur));
				rlib_free_output(r, ptr);
				rlib_free_report(r, report);
				return NULL;
			}
			report->report_footer = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Detail"))) {
			struct rlib_report_detail *ptr = parse_detail(r, allow_fail, doc, cur);
			if (ptr == NULL) {
				rlib_free_report(r, report);
				return NULL;
			}
			if (report->detail) {
				r_error(r, "Line: %ld - Duplicate Detail in <Report>\n", xmlGetLineNo(cur));
				rlib_free_detail(r, ptr);
				rlib_free_report(r, report);
				return NULL;
			}
			report->detail = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Alternate"))) {
			struct rlib_element *ptr = parse_alternate(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_report(r, report);
				return NULL;
			}
			if (report->alternate) {
				r_error(r, "Line: %ld - Duplicate Alternate in <Report>\n", xmlGetLineNo(cur));
				rlib_free_output(r, ptr);
				rlib_free_report(r, report);
				return NULL;
			}
			report->alternate = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Graph"))) {
			struct rlib_graph *ptr = parse_graph(r, allow_fail, cur);
			if (ptr == NULL) {
				rlib_free_report(r, report);
				return NULL;
			}
			if (report->graph) {
				rlib_free_graph(r, ptr);
				rlib_free_report(r, report);
				r_error(r, "Line: %ld - Duplicate Graph in <Report>\n", xmlGetLineNo(cur));
				return NULL;
			}
			report->graph = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Chart"))) {
			struct rlib_chart *ptr = parse_chart(r, allow_fail, cur, &error1);
			if (error1) {
				rlib_free_report(r, report);
				return NULL;
			}
			if (report->chart) {
				rlib_free_chart(r, ptr);
				rlib_free_report(r, report);
				r_error(r, "Line: %ld - Duplicate Chart in <Report>\n", xmlGetLineNo(cur));
				return NULL;
			}
			report->chart = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Breaks"))) {
			gint error;
			struct rlib_element *ptr = parse_report_breaks(r, allow_fail, report, doc, ns, cur, &error);
			if (error) {
				rlib_free_report(r, report);
				return NULL;
			}
			if (report->breaks) {
				r_error(r, "Line: %ld - Duplicate Breaks in <Report>\n", xmlGetLineNo(cur));
				rlib_free_breaks(r, ptr);
				rlib_free_report(r, report);
				return NULL;
			}
			report->breaks = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Variables"))) {
			struct rlib_element *ptr = parse_report_variables(r, allow_fail, cur, &error1);
			if (error1) {
				rlib_free_report(r, report);
				return NULL;
			}
			if (report->variables) {
				r_error(r, "Line: %ld - Duplicate Variables in <Report>\n", xmlGetLineNo(cur));
				rlib_free_variables(r, ptr);
				rlib_free_report(r, report);
				return NULL;
			}
			report->variables = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "MetaData"))) {
			if (parse_metadata_item(r, allow_fail, cur, r->input_metadata) == -1) {
				rlib_free_report(r, report);
				return NULL;
			}
		} else if (!ignoreElement((const char *)cur->name)) { /* must be last */
			r_error(r, "Line: %ld - Unknown element [%s] in <Report>\n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_report(r, report);
				return NULL;
			}
		}
		cur = cur->next;
	}

	return report;
}

static struct rlib_report *parse_part_load(rlib *r, gboolean allow_fail, struct rlib_part *part, xmlNodePtr cur) {
	struct rlib_report *report;
	gchar *name, *query;
	struct rlib_pcode *name_code, *query_code;
	gchar real_name[MAXSTRLEN], real_query[MAXSTRLEN];
	gboolean result_name;

	name =  (gchar *)xmlGetProp(cur, (const xmlChar *) "name");
	query =  (gchar *)xmlGetProp(cur, (const xmlChar *) "query");

	name_code = rlib_infix_to_pcode(r, part, NULL, name, xmlGetLineNo(cur), TRUE);
	query_code = rlib_infix_to_pcode(r, part, NULL, query, xmlGetLineNo(cur), TRUE);

	result_name = rlib_execute_as_string(r, name_code, real_name, MAXSTRLEN - 1);
	rlib_execute_as_string(r, query_code, real_query, MAXSTRLEN - 1);

	if (result_name && result_name) {
		report = parse_report_file(r, allow_fail, part->report_index, real_name, query);
	} else {
		r_error(r, "parse_part_load - Query or Name Is Invalid\n");
		report = NULL;
	}
	rlib_pcode_free(r, name_code);
	rlib_pcode_free(r, query_code);

	xmlFree(name);

	return report;
}

static struct rlib_part_td *parse_part_td(rlib *r, gboolean allow_fail, struct rlib_part *part, xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur) {
	struct rlib_part_td *td = g_new0(struct rlib_part_td, 1);

	if (td == NULL)
		return NULL;

	get_both(&td->xml_width, cur, "width");
	get_both(&td->xml_height, cur, "height");
	get_both(&td->xml_border_width, cur, "border_width");
	get_both(&td->xml_border_color, cur, "border_color");

	td->reports = NULL;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "load"))) {
			struct rlib_report *report = parse_part_load(r, allow_fail, part, cur);
			if (report == NULL) {
				rlib_free_part_td(r, td);
				return NULL;
			}
			td->reports = g_slist_append(td->reports, report);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "Report"))) {
			struct rlib_report *report = parse_report(r, allow_fail, part, doc, ns, cur, NULL);
			if (report == NULL) {
				rlib_free_part_td(r, td);
				return NULL;
			}
			td->reports = g_slist_append(td->reports, report);
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <tr>. Expected td.\n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_part_td(r, td);
				return NULL;
			}
		}
		cur = cur->next;
	}
	return td;
}

static struct rlib_part_tr *parse_part_tr(rlib *r, gboolean allow_fail, struct rlib_part *part, xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur) {
	struct rlib_part_tr *tr = g_new0(struct rlib_part_tr, 1);
	get_both(&tr->xml_layout, cur, "layout");
	get_both(&tr->xml_newpage, cur, "newpage");

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "pd"))) {
			struct rlib_part_td *ptr = parse_part_td(r, allow_fail, part, doc, ns, cur);
			if (ptr == NULL) {
				rlib_free_part_tr(r, tr);
				return NULL;
			}
			tr->part_deviations = g_slist_append(tr->part_deviations, ptr);
		} else if (!ignoreElement((const char *)cur->name)) {
			r_error(r, "Line: %ld - Unknown element [%s] in <tr>. Expected td.\n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_part_tr(r, tr);
				return NULL;
			}
		}
		cur = cur->next;
	}
	return tr;
}

static struct rlib_part *parse_part(rlib *r, gboolean allow_fail, xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur) __attribute__((warn_unused_result));
static struct rlib_part *parse_part(rlib *r, gboolean allow_fail, xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur) {
	struct rlib_part *part = g_new0(struct rlib_part, 1);
	gint error1;

	if (part == NULL) {
		r_error(r, "Out of memory\n");
		return NULL;
	}

	while (cur && xmlIsBlankNode (cur))
		cur = cur -> next;

	if (cur == NULL) {
		r_error(r, "Empty part\n");
		rlib_free_part(r, part);
		return NULL;
	}

	get_both(&part->xml_name, cur, "name");
	get_both(&part->xml_pages_across, cur, "pages_across");
	get_both(&part->xml_font_size, cur, "fontSize");
	get_both(&part->xml_orientation, cur, "orientation");
	get_both(&part->xml_top_margin, cur, "top_margin");
	get_both(&part->xml_left_margin, cur, "left_margin");
	get_both(&part->xml_bottom_margin, cur, "bottom_margin");
	get_both(&part->xml_paper_type, cur, "paper_type");
	get_both(&part->xml_iterations, cur, "iterations");
	get_both(&part->xml_suppress_page_header_first_page, cur, "suppressPageHeaderFirstPage");
	if (xmlHasProp(cur, (const xmlChar *) "paperType") && part->xml_paper_type.xml == NULL) {
		get_both(&part->xml_paper_type, cur, "paperType");
	}

	part->part_rows = NULL;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *) "pr"))) {
			struct rlib_part_tr *ptr = parse_part_tr(r, allow_fail, part, doc, ns, cur);
			if (ptr == NULL) {
				rlib_free_part(r, part);
				return NULL;
			}
			part->part_rows = g_slist_append(part->part_rows, ptr);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "PageHeader"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_part(r, part);
				return NULL;
			}
			if (part->page_header) {
				rlib_free_output(r, ptr);
				rlib_free_part(r, part);
				r_error(r, "Line: %ld - Duplicate PageHeader in <Part>\n", xmlGetLineNo(cur));
				return NULL;
			}
			part->page_header = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "ReportHeader"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_part(r, part);
				return NULL;
			}
			if (part->report_header) {
				rlib_free_output(r, ptr);
				rlib_free_part(r, part);
				r_error(r, "Line: %ld - Duplicate ReportHeader in <Part>\n", xmlGetLineNo(cur));
				return NULL;
			}
			part->report_header = ptr;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *) "PageFooter"))) {
			struct rlib_element *ptr = parse_report_outputs(r, allow_fail, doc, cur, &error1);
			if (error1) {
				rlib_free_part(r, part);
				return NULL;
			}
			if (part->page_footer) {
				rlib_free_output(r, ptr);
				rlib_free_part(r, part);
				r_error(r, "Line: %ld - Duplicate PageFooter in <Part>\n", xmlGetLineNo(cur));
				return NULL;
			}
			part->page_footer = ptr;
		} else if (!ignoreElement((const char *)cur->name)) { /* must be last */
			/* ignore comments, etc */
			r_error(r, "Line: %ld - Unknown element [%s] in <Part>\n", xmlGetLineNo(cur), cur->name);
			if (!allow_fail) {
				rlib_free_part(r, part);
				return NULL;
			}
		}
		cur = cur->next;
	}

	return part;
}

void dump_part_td(gpointer data UNUSED, gpointer user_data UNUSED) {
}

void dump_part_tr(gpointer data, gpointer user_data UNUSED) {
	struct rlib_part_tr *tr_data = data;
	g_slist_foreach(tr_data->part_deviations, dump_part_td, NULL);
}

void dump_part(struct rlib_part *part) {
	g_slist_foreach(part->part_rows, dump_part_tr, NULL);
}

struct rlib_part *parse_part_file(rlib *r, gboolean allow_fail, int report_index) {
	gchar *filename = r->reportstorun[report_index].name;
	gchar type = r->reportstorun[report_index].type;
	xmlDocPtr doc;
	struct rlib_part *part = NULL;
	xmlNsPtr ns = NULL;
	xmlNodePtr cur;
	int found = FALSE;

	xmlLineNumbersDefault(1);

#if DISABLE_UTF8
	r->xml_encoder = g_iconv_open(ICONV_ISO, "UTF-8");
#endif

	if(type == RLIB_REPORT_TYPE_BUFFER)
		doc = xmlReadMemory(filename, strlen(filename), NULL, NULL, XML_PARSE_XINCLUDE);
	else {
		gchar *file = get_filename(r, filename, report_index, TRUE, FALSE);
		doc = xmlReadFile(file, NULL, XML_PARSE_XINCLUDE);
		g_free(file);
	}

	xmlXIncludeProcess(doc);

	if (doc == NULL)  {
		r_error(r, "xmlParseError \n");
		return NULL;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		r_error(r, "xmlParseError \n");
		xmlFreeDoc(doc);
		return NULL;
	}

	if ((xmlStrcmp(cur->name, (const xmlChar *) "Part")) == 0) {
		part = parse_part(r, allow_fail, doc, ns, cur);
		if (part == NULL) {
			xmlFreeDoc(doc);
			return NULL;
		}
		part->report_index = report_index;
		found = TRUE;
	} else if ((xmlStrcmp(cur->name, (const xmlChar *) "Report")) == 0) {
		/*
		 * If a report appears by it's self put it in a table w/ 1 row and 1 col 100%
		 */
		struct rlib_report *report;
		struct rlib_part_tr *tr;
		struct rlib_part_td *td;

		part = g_new0(struct rlib_part, 1);
		tr = g_new0(struct rlib_part_tr, 1);
		td = g_new0(struct rlib_part_td, 1);
		if (part == NULL || tr == NULL || td == NULL) {
			rlib_free_part(r, part);
			rlib_free_part_tr(r, tr);
			rlib_free_part_td(r, td);
			xmlFreeDoc(doc);
			r_error(r, "Out of memory\n");
			return NULL;
		}

		report = parse_report(r, allow_fail, part, doc, ns, cur, NULL);
		if (report == NULL) {
			rlib_free_part(r, part);
			rlib_free_part_tr(r, tr);
			rlib_free_part_td(r, td);
			xmlFreeDoc(doc);
			return NULL;
		}
		part->report_index = report_index;
		part->part_rows = g_slist_append(part->part_rows, tr);
		tr->part_deviations = g_slist_append(tr->part_deviations, td);
		td->reports = g_slist_append(td->reports, report);
		rlib_free_output(r, part->page_header);
		part->page_header = report->page_header;
		rlib_free_output(r, part->report_header);
		part->report_header = report->report_header;
		rlib_free_output(r, part->page_footer);
		part->page_footer = report->page_footer;
		report->page_header = NULL;
		report->report_header = NULL;
		report->page_footer = NULL;

		part->xml_left_margin.xml = xmlStrdup(report->xml_left_margin.xml);
		part->xml_top_margin.xml = xmlStrdup(report->xml_top_margin.xml);
		part->xml_bottom_margin.xml = xmlStrdup(report->xml_bottom_margin.xml);
		part->xml_font_size.xml = xmlStrdup(report->xml_font_size.xml);
		part->xml_orientation.xml = xmlStrdup(report->xml_orientation.xml);
		part->xml_suppress_page_header_first_page.xml = xmlStrdup(report->xml_suppress_page_header_first_page.xml);
		part->xml_suppress.xml = xmlStrdup(report->xml_suppress.xml);

		report->is_the_only_report = TRUE;
		part->has_only_one_report = TRUE;

		part->xml_pages_across.xml = xmlStrdup(report->xml_pages_across.xml);
		found = TRUE;
	}

	if (!found) {
		r_error(r, "document of the wrong type, was '%s', Report or Part expected", cur->name);
		r_error(r, "xmlDocDump follows\n");
		xmlDocDump(stderr, doc);
		xmlFreeDoc(doc);
		rlib_free_part(r, part);
		return NULL;
	}

	xmlFreeDoc(doc);

#if DISABLE_UTF8
	if ((long)r->xml_encoder != -1)
		g_iconv_close(r->xml_encoder);
#endif

	return part;
}

static struct rlib_report *parse_report_file(rlib *r, gboolean allow_fail, int report_index, gchar *filename, gchar *query) {
	xmlDocPtr doc;
	gchar *file;
	struct rlib_report *report = NULL;
	xmlNsPtr ns = NULL;
	xmlNodePtr cur;

	xmlLineNumbersDefault(1);

	file = get_filename(r, filename, report_index, FALSE, FALSE);
	doc = xmlReadFile(file, NULL, XML_PARSE_XINCLUDE);
	g_free(file);

	if (doc == NULL)  {
		r_error(r, "xmlParseError \n");
		return NULL;
	}

	xmlXIncludeProcess(doc);

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		r_error(r, "xmlParseError \n");
		xmlFreeDoc(doc);
		return NULL;
	}

	if ((xmlStrcmp(cur->name, (const xmlChar *) "Report")) == 0) {
		report = parse_report(r, allow_fail, NULL, doc, ns, cur, query);
		if (report == NULL) {
			xmlFreeDoc(doc);
			return NULL;
		}
	} else {
		r_error(r, "Document of the wrong type, was '%s'\n", cur->name);
		r_error(r, "Report xmlDocDump follows\n\n");
		xmlDocDump(stderr, doc);
		xmlFreeDoc(doc);
		return NULL;
	}

	xmlFreeDoc(doc);

	return report;
}
