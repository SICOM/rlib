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
 * $Id$
 * 
 * This module generates a report from the information stored in the current
 * report object.
 * The main entry point is called once at report generation time for each
 * report defined in the rlib object.
 *
 */
 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <libintl.h>

#include "config.h"

#include "config.h"
#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"
#include "rlib_langinfo.h"

#define STATE_NONE		0
#define STATE_BGCOLOR	1

#define STATUS_NONE	0
#define STATUS_START 1
#define STATUS_STOP	2

#define TEXT_NORMAL 1
#define TEXT_LEFT 2
#define TEXT_RIGHT 3
 
static const gchar *truefalses[] = {
	"no",
	"yes",
	"false",
	"true",
	NULL
};

static const gchar *aligns[] = {
	"left",
	"right",
	"center",
	NULL
};

static struct rlib_paper paper[] = {
	{RLIB_PAPER_LETTER,612, 792, "LETTER"},
	{RLIB_PAPER_LEGAL, 612, 1008, "LEGAL"},
	{RLIB_PAPER_A4, 595, 842, "A4"},
	{RLIB_PAPER_B5, 499, 708, "B5"},
	{RLIB_PAPER_C5, 459, 649, "C5"},
	{RLIB_PAPER_DL, 312, 624, "DL"},
	{RLIB_PAPER_EXECUTIVE, 522, 756, "EXECUTIVE"},
	{RLIB_PAPER_COMM10, 297, 684, "COMM10"},
	{RLIB_PAPER_MONARCH, 279, 540, "MONARCH"},
	{RLIB_PAPER_FILM35MM, 528, 792, "FILM35MM"},
	{0, 0, 0, ""},
};

struct rlib_paper * rlib_layout_get_paper(rlib *r, gint paper_type) {
	gint i;
	for(i=0;paper[i].type != 0;i++)
		if(paper[i].type == paper_type)
			return &paper[i];
	return NULL;
}

struct rlib_paper * rlib_layout_get_paper_by_name(rlib *r, gchar *paper_name) {
	gint i;
	if(paper_name == NULL)
		return NULL;
		
	for(i=0;paper[i].type != 0;i++)
		if(!strcasecmp(paper[i].name, paper_name))
			return &paper[i];
	return NULL;
}

static gfloat rlib_layout_advance_horizontal_margin(rlib *r, gfloat margin, gint chars) {
	gchar buf[2];
	buf[0] = 'W';
	buf[1] = '\0';
	margin += (OUTPUT(r)->get_string_width(r, buf)*chars);
	return margin;
}

gfloat rlib_layout_estimate_string_width_from_extra_data(rlib *r, struct rlib_line_extra_data *extra_data) {
	gfloat rtn_width;
	OUTPUT(r)->set_font_point(r, extra_data->font_point);
	rtn_width = rlib_layout_advance_horizontal_margin(r, 0, extra_data->width);	
	return rtn_width;
}

static gchar *rlib_encode_text(rlib *r, gchar *text, gchar **result) {
	if (text == NULL) {
		*result = g_strdup("");
	} else {
		gsize len = strlen(text);
		gsize result_len;
		rlib_charencoder_convert(r->output_encoder, &text, &len, result, &result_len);
		if (*result == NULL) {
			r_error(r, "encode returned NULL result input was[%s], len=%d", text, r_strlen(text));
			*result = g_strdup("!ERR_ENC2");
		}
	}
	return *result;
}

gfloat rlib_layout_get_next_line(rlib *r, struct rlib_part *part, gfloat position, struct rlib_report_lines *rl) {
	if(part->landscape)
		return ((part->paper->width/RLIB_PDF_DPI) - (position + rl->max_line_height));
	else
		return ((part->paper->height/RLIB_PDF_DPI) - (position + rl->max_line_height));
}

gfloat rlib_layout_get_next_line_by_font_point(rlib *r, struct rlib_part *part, gfloat position, gfloat point) {
	if(part->landscape)
		return ((part->paper->width/RLIB_PDF_DPI) - (position + RLIB_GET_LINE(point)));
	else
		return ((part->paper->height/RLIB_PDF_DPI) - (position + RLIB_GET_LINE(point)));
}


static gint rlib_check_is_not_suppressed(rlib *r, struct rlib_pcode *code) {
	gint result = FALSE;
	if (rlib_execute_as_int_inlist(r, code, &result, truefalses))
		result &= 1;
	return result? FALSE : TRUE;
}

static gfloat rlib_layout_get_report_width(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	if(report == NULL) {
		if(part->landscape)
			return (part->paper->height/RLIB_PDF_DPI) - (part->left_margin*2);
		else
			return (part->paper->width/RLIB_PDF_DPI) - (part->left_margin*2);
	} else {
		return report->page_width;
	}
}

static gfloat rlib_layout_estimate_string_width(rlib *r, gint len) {
	gchar buf[2];
	buf[0] = 'W';
	buf[1] = '\0';
	return (OUTPUT(r)->get_string_width(r, buf)*len);
}

static gchar *rlib_layout_suppress_non_memo_on_extra_lines(rlib *r, struct rlib_line_extra_data *extra_data, gint memo_line, gchar *spaced_out) {
	if(memo_line <= 1)
		return extra_data->formatted_string;
	else {
		memset(spaced_out, ' ', extra_data->width);
		spaced_out[extra_data->width] = 0;
		return spaced_out;
	}
}

//TODO: This needs to know if the line has any memo fields or not
static gchar *rlib_layout_get_true_text_from_extra_data(rlib *r, struct rlib_line_extra_data *extra_data, gint memo_line, gchar *spaced_out, 
	gboolean *need_free) {
	gchar *text = NULL;
	gchar *align_text = NULL;
	gchar *encoded_text = NULL;

	if(extra_data->is_memo == FALSE && memo_line > 1) {
		memset(spaced_out, ' ', extra_data->width);
		spaced_out[extra_data->width] = 0;
		*need_free = FALSE;
		return spaced_out;
	} else {
		if(extra_data->memo_line_count == 0) {
		 	rlib_encode_text(r, extra_data->formatted_string, &encoded_text);
			*need_free = TRUE;
			return encoded_text;
		} else {
			if(memo_line > extra_data->memo_line_count) {
				memset(spaced_out, ' ', extra_data->width);
				spaced_out[extra_data->width] = 0;
				*need_free = FALSE;
				return spaced_out;
			} else {
				gint i=1;
				GSList *list = extra_data->memo_lines;

				while(i < memo_line) {
					list = list->next;		
					i++;
				}

				text = list->data;
			}
		}
	}

	extra_data->align = extra_data->report_field->align;
	rlib_align_text(r, &align_text, text, extra_data->report_field->align, extra_data->report_field->width);
 	rlib_encode_text(r, align_text, &encoded_text);
	g_free(align_text);
	*need_free = TRUE;
	return encoded_text;
}

static gfloat rlib_layout_output_extras_start(rlib *r, struct rlib_part *part, gint backwards, gfloat left_origin, gfloat bottom_orgin, 
struct rlib_line_extra_data *extra_data, gboolean ignore_links) {
	
	if(extra_data->is_bold)
		OUTPUT(r)->start_bold(r);
	if(extra_data->is_italics)
		OUTPUT(r)->start_italics(r);

	if(!ignore_links) {
		if(extra_data->running_link_status & STATUS_START)
			OUTPUT(r)->start_boxurl(r, part, left_origin, bottom_orgin, extra_data->running_link_total, 
				RLIB_GET_LINE(extra_data->font_point), extra_data->link, backwards);
	}

	if(extra_data->running_bgcolor_status & STATUS_START) 
		OUTPUT(r)->start_draw_cell_background(r, left_origin, bottom_orgin, extra_data->running_bg_total, 
			RLIB_GET_LINE(extra_data->font_point), &extra_data->bgcolor);

	return extra_data->output_width;
}

//BOBD: Extra_data seems to have all the stuff which needs to get passed in JSON
static gfloat rlib_layout_text_from_extra_data(rlib *r, gint backwards, gfloat left_origin, gfloat bottom_orgin, struct rlib_line_extra_data *extra_data, 
gint flag, gint memo_line) {
	gfloat rtn_width;
	gchar *text = extra_data->formatted_string;
	gint i, slen;
	gchar spaced_out[MAXSTRLEN];

	if(OUTPUT(r)->trim_links == FALSE) {
		flag = TEXT_NORMAL;
	}

	if(flag == TEXT_LEFT && extra_data->found_link) {
		text = g_strdup(text);
		slen = r_strlen(text);
		for(i=slen-1;i>0;i--) {
			if(isspace(text[i]))
				text[i] = 0;
			else
				break;
		}
	} else if(flag == TEXT_RIGHT && extra_data->found_link) {
		gint count = 0;
		slen = r_strlen(text);
		for(i=slen-1;i>0;i--) {
			if(isspace(text[i]))
				count++;
			else
				break;
		}
		text = g_malloc(count+1);
		memset(text, ' ', count);
		text[count] = 0;
	}

	if(extra_data->type == RLIB_ELEMENT_IMAGE) {
		gchar *filename;
		gfloat height = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data->rval_image_height));
		gfloat width = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data->rval_image_width));
		gchar *name = RLIB_VALUE_GET_AS_STRING(&extra_data->rval_image_name);
		gchar *type = RLIB_VALUE_GET_AS_STRING(&extra_data->rval_image_type);
		filename = get_filename(r, name, extra_data->report_index, FALSE);
		OUTPUT(r)->line_image(r, left_origin, bottom_orgin, filename, type, width, height);
		g_free(filename);
		rtn_width = extra_data->output_width;
	} else if(extra_data->type == RLIB_ELEMENT_BARCODE) {
		gchar *filename;
		gfloat height = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data->rval_image_height));
		gfloat width = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data->rval_image_width));
		gchar *name = RLIB_VALUE_GET_AS_STRING(&extra_data->rval_image_name);
		gchar *type = RLIB_VALUE_GET_AS_STRING(&extra_data->rval_image_type);
		filename = get_filename(r, name, extra_data->report_index, FALSE);
		OUTPUT(r)->line_image(r, left_origin, bottom_orgin, filename, type, width, height);
		g_free(filename);
		rtn_width = extra_data->output_width;
	} else {
		OUTPUT(r)->set_font_point(r, extra_data->font_point);
		if(extra_data->found_color)
			OUTPUT(r)->set_fg_color(r, extra_data->color.r, extra_data->color.g, extra_data->color.b);
		if(extra_data->is_bold)
			OUTPUT(r)->start_bold(r);
		if(extra_data->is_italics)
			OUTPUT(r)->start_italics(r);

		if(extra_data->delayed == TRUE) {
			struct rlib_delayed_extra_data *delayed_data = g_new0(struct rlib_delayed_extra_data, 1);
			delayed_data->backwards = backwards;
			delayed_data->left_origin = left_origin;
			delayed_data->bottom_orgin = bottom_orgin+(extra_data->font_point/300.0);
			delayed_data->extra_data = *extra_data;
			delayed_data->r = r;
			OUTPUT(r)->print_text_delayed(r, delayed_data, backwards, RLIB_VALUE_GET_TYPE(&extra_data->rval_code));
		} else {
			gboolean need_free;
			gchar *real_text = rlib_layout_get_true_text_from_extra_data(r, extra_data, memo_line, spaced_out, &need_free);
			OUTPUT(r)->print_text(r, left_origin, bottom_orgin+(extra_data->font_point/300.0), real_text, backwards, extra_data);
			if(need_free)
				g_free(real_text);
		}
		rtn_width = extra_data->output_width;
		if(extra_data->found_color)
			OUTPUT(r)->set_fg_color(r, 0, 0, 0);
		if(extra_data->is_bold)
			OUTPUT(r)->end_bold(r);
		if(extra_data->is_italics)
			OUTPUT(r)->end_italics(r);
		OUTPUT(r)->set_font_point(r, r->font_point);
	}
	
	if((flag == TEXT_LEFT || flag == TEXT_RIGHT) && extra_data->found_link) {
		g_free(text);
	}
	return rtn_width;
}

static gfloat rlib_layout_output_extras_end(rlib *r, struct rlib_part *part, gint backwards, gfloat left_origin, gfloat bottom_orgin, 
struct rlib_line_extra_data *extra_data, gint memo_line) {


	if(extra_data->running_bgcolor_status & STATUS_STOP)
		OUTPUT(r)->end_draw_cell_background(r);	

	if(extra_data->running_link_status & STATUS_STOP) {
		OUTPUT(r)->end_boxurl(r, backwards);
		if(OUTPUT(r)->trim_links) {
			rlib_layout_output_extras_start(r, part, backwards, 0, 0, extra_data, TRUE);
			rlib_layout_text_from_extra_data(r, backwards, 0, 0, extra_data, TEXT_RIGHT, memo_line);		
		}
	}

	if(extra_data->is_italics)
		OUTPUT(r)->end_italics(r);
	if(extra_data->is_bold)
		OUTPUT(r)->end_bold(r);
		
	return extra_data->output_width;
}

static gfloat rlib_layout_output_extras(rlib *r, struct rlib_part *part, gint backwards, gfloat left_origin, gfloat bottom_orgin, 
struct rlib_line_extra_data *extra_data) {
	if(extra_data->running_bgcolor_status & STATUS_STOP)
		OUTPUT(r)->end_draw_cell_background(r);	

	if(extra_data->running_link_status & STATUS_STOP)
		OUTPUT(r)->end_boxurl(r, backwards);

	if(extra_data->running_link_status & STATUS_START)
		OUTPUT(r)->start_boxurl(r, part, left_origin, bottom_orgin, extra_data->running_link_total, 
			RLIB_GET_LINE(extra_data->font_point), extra_data->link, backwards);

	if(extra_data->running_bgcolor_status & STATUS_START) {
		OUTPUT(r)->start_draw_cell_background(r, left_origin, bottom_orgin, extra_data->running_bg_total, 
			RLIB_GET_LINE(extra_data->font_point), &extra_data->bgcolor);
	}
		
	return extra_data->output_width;
}

static gfloat rlib_layout_text_string(rlib *r, gint backwards, gfloat left_origin, gfloat bottom_orgin, struct rlib_line_extra_data *extra_data, 
gchar *text, gint memo_line) {
	gfloat rtn_width;
	gchar *encoded_text = NULL;

	OUTPUT(r)->set_font_point(r, extra_data->font_point);
	if(extra_data->found_color)
		OUTPUT(r)->set_fg_color(r, extra_data->color.r, extra_data->color.g, extra_data->color.b);
	if(extra_data->is_bold)
		OUTPUT(r)->start_bold(r);
	if(extra_data->is_italics)
		OUTPUT(r)->start_italics(r);
	OUTPUT(r)->print_text(r, left_origin, bottom_orgin+(extra_data->font_point/300.0), rlib_encode_text(r, text, &encoded_text), backwards, extra_data);
	g_free(encoded_text);
	rtn_width = extra_data->output_width;
	if(extra_data->found_color)
		OUTPUT(r)->set_fg_color(r, 0, 0, 0);
	if(extra_data->is_bold)
		OUTPUT(r)->end_bold(r);
	if(extra_data->is_italics)
		OUTPUT(r)->end_italics(r);
	OUTPUT(r)->set_font_point(r, r->font_point);
	return rtn_width;
}

static void rlib_advance_vertical_position(rlib *r, gfloat *rlib_position, struct rlib_report_lines *rl) {
	*rlib_position += rl->max_line_height;
}

static gint rlib_layout_execute_pcodes_for_line(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_lines *rl, struct rlib_line_extra_data *extra_data, gint *delayed) {
	gint i=0;
	gchar *text;
	gint use_font_point, tmp_int;
	struct rlib_report_field *rf;
	struct rlib_report_literal *rt;
	struct rlib_report_image *ri;
	struct rlib_report_barcode *rb;
	struct rlib_element *e;
	struct rlib_value line_rval_color;
	struct rlib_value line_rval_bgcolor;
	struct rlib_value line_rval_bold;
	struct rlib_value line_rval_italics;
	gint line_has_memo = FALSE;

	RLIB_VALUE_TYPE_NONE(&line_rval_color);
	RLIB_VALUE_TYPE_NONE(&line_rval_bgcolor);
	RLIB_VALUE_TYPE_NONE(&line_rval_bold);
	RLIB_VALUE_TYPE_NONE(&line_rval_italics);
	if(rl->color_code != NULL)
		rlib_execute_pcode(r, &line_rval_color, rl->color_code, NULL);
	if(rl->bgcolor_code != NULL)
		rlib_execute_pcode(r, &line_rval_bgcolor, rl->bgcolor_code, NULL);
	if(rl->bold_code != NULL)
		rlib_execute_pcode(r, &line_rval_bold, rl->bold_code, NULL);
	if(rl->italics_code != NULL)
		rlib_execute_pcode(r, &line_rval_italics, rl->italics_code, NULL);

	use_font_point = get_font_point(r, part, report, rl);

	if(rl->max_line_height < RLIB_GET_LINE(use_font_point))
		rl->max_line_height = RLIB_GET_LINE(use_font_point);
	
	for(e=rl->e; e != NULL; e=e->next) {
		gchar *tmp_align_buf = NULL;
		RLIB_VALUE_TYPE_NONE(&extra_data[i].rval_bgcolor);
		RLIB_VALUE_TYPE_NONE(&extra_data[i].rval_bold);
		RLIB_VALUE_TYPE_NONE(&extra_data[i].rval_italics);
		
		extra_data[i].type = e->type;
		extra_data[i].delayed = FALSE;
		extra_data[i].is_memo = FALSE;
		if (e->type == RLIB_ELEMENT_FIELD) {
			gchar *buf = NULL;
			rf = e->data;
			if (rf == NULL) 
				r_error(r, "report_field is NULL ... will crash");
			else if 
				(rf->code == NULL) r_error(r, "There is no code for field");
			
			rlib_execute_pcode(r, &extra_data[i].rval_code, rf->code, NULL);	
			if(rf->link_code != NULL) {	
				rlib_execute_pcode(r, &extra_data[i].rval_link, rf->link_code, &extra_data[i].rval_code);
			}
			if(rf->color_code != NULL) {
				rlib_execute_pcode(r, &extra_data[i].rval_color, rf->color_code, &extra_data[i].rval_code);
			} else if(rl->color_code != NULL) {
				extra_data[i].rval_color = line_rval_color;
				rlib_value_dup_contents(&extra_data[i].rval_color);
			}
			if(rf->bgcolor_code != NULL) {
				rlib_execute_pcode(r, &extra_data[i].rval_bgcolor, rf->bgcolor_code, &extra_data[i].rval_code);
			} else if(rl->bgcolor_code != NULL) {
				extra_data[i].rval_bgcolor = line_rval_bgcolor;
				rlib_value_dup_contents(&extra_data[i].rval_bgcolor);
			}
			if(rf->bold_code != NULL) {
				rlib_execute_pcode(r, &extra_data[i].rval_bold, rf->bold_code, &extra_data[i].rval_code);
			} else if(rl->bold_code != NULL) {
				extra_data[i].rval_bold = line_rval_bold;
				rlib_value_dup_contents(&extra_data[i].rval_bold);
			}
			if(rf->italics_code != NULL) {
				rlib_execute_pcode(r, &extra_data[i].rval_italics, rf->italics_code, &extra_data[i].rval_code);
			} else if(rl->italics_code != NULL) {
				extra_data[i].rval_italics = line_rval_italics;
				rlib_value_dup_contents(&extra_data[i].rval_italics);
			}

			if (rf->align_code) {
				gint t;
				if (rlib_execute_as_int_inlist(r, rf->align_code, &t, aligns)) {
					if (t < 3) rf->align = t;
				}
			}
			if (rf->width_code) {
				gint t;
				if (rlib_execute_as_int(r, rf->width_code, &t)) { 
					if(t < 0) {
						r_error(r, "Line: %d - Width Can't Be < 0 [%s]\n", rf->xml_width.line, rf->xml_width.xml);
						t = 0;
					}
					rf->width = t;
				}
			}
			
			extra_data[i].translate = FALSE;
			if(rf->translate_code) {
				gboolean t;
				if(rlib_execute_as_boolean(r, rf->translate_code, &t)) 
					extra_data[i].translate = t;
			}
			
			if (rf->memo_code) {
				gint t;
				if (rlib_execute_as_boolean(r, rf->memo_code, &t)) { 
					extra_data[i].is_memo = TRUE;
					line_has_memo = TRUE;
				}
			}
			
			if(extra_data[i].is_memo == FALSE) {
				rlib_format_string(r, &buf, rf, &extra_data[i].rval_code);
				if(extra_data[i].translate && buf != NULL) {
					gchar *tmp_str = gettext(buf);
					if(tmp_str != buf) {
						g_free(buf);
						buf = g_strdup(tmp_str);				
					}
				}
				extra_data[i].align = rf->align;
				rlib_align_text(r, &tmp_align_buf, buf, rf->align, rf->width);
				extra_data[i].formatted_string = tmp_align_buf;
				g_free(buf);

			} else {
				rlib_format_string(r, &buf, rf, &extra_data[i].rval_code);
				if(extra_data[i].translate && buf != NULL) {
					gchar *tmp_str = gettext(buf);
					if(tmp_str != buf) {
						g_free(buf);
						buf = g_strdup(tmp_str);				
					}
				}

				extra_data[i].formatted_string = buf;
				extra_data[i].memo_lines = rlib_format_split_string(r, extra_data[i].formatted_string, rf->width, -1, '\n', ' ', &extra_data[i].memo_line_count);
			}
			

			extra_data[i].width = rf->width;
			extra_data[i].field_code = rf->code;
			extra_data[i].report_field = rf;
			rlib_execute_pcode(r, &extra_data[i].rval_col, rf->col_code, NULL);
			if(rlib_execute_as_int(r, rf->delayed_code, &tmp_int)) {
				extra_data[i].delayed = tmp_int;
				if(tmp_int == TRUE)
					*delayed = TRUE;
			}
		} else if(e->type == RLIB_ELEMENT_LITERAL) {
			gchar *txt_pointer;
			rt = e->data;
			if(rt->color_code != NULL)	
				rlib_execute_pcode(r, &extra_data[i].rval_color, rt->color_code, NULL);
			else if(rl->color_code != NULL) {
				extra_data[i].rval_color = line_rval_color;
				rlib_value_dup_contents(&extra_data[i].rval_color);
			}
			if(rt->link_code != NULL) {	
				rlib_execute_pcode(r, &extra_data[i].rval_link, rt->link_code, NULL);
			}

			if(rt->bgcolor_code != NULL)
				rlib_execute_pcode(r, &extra_data[i].rval_bgcolor, rt->bgcolor_code, NULL);
			else if(rl->bgcolor_code != NULL) {
				extra_data[i].rval_bgcolor = line_rval_bgcolor;
				rlib_value_dup_contents(&extra_data[i].rval_bgcolor);				
			}
			if(rt->bold_code != NULL)	
				rlib_execute_pcode(r, &extra_data[i].rval_bold, rt->bold_code, NULL);
			else if(rl->bold_code != NULL) {
				extra_data[i].rval_bold = line_rval_bold;
				rlib_value_dup_contents(&extra_data[i].rval_bold);
			}
			
			if(rt->italics_code != NULL)	
				rlib_execute_pcode(r, &extra_data[i].rval_italics, rt->italics_code, NULL);
			else if(rl->italics_code != NULL) {
				extra_data[i].rval_italics = line_rval_italics;
				rlib_value_dup_contents(&extra_data[i].rval_italics);
			}

			if (rt->align_code) {
				gint t;
				if (rlib_execute_as_int_inlist(r, rt->align_code, &t, aligns)) {
					if (t <= RLIB_ALIGN_CENTER) rt->align = t;
				}
			}
			if (rt->width_code) {
				gint t;
				if (rlib_execute_as_int(r, rt->width_code, &t)) {
					if(t < 0) {
						r_error(r, "Line: %d - Width Can't Be < 0 [%s]\n", rt->xml_width.line, rt->xml_width.xml);
						t = 0;
					}
					rt->width = t;
				}
			}
			extra_data[i].translate = FALSE;
			if(rt->translate_code) {
				gboolean t;
				if(rlib_execute_as_boolean(r, rt->translate_code, &t)) 
					extra_data[i].translate = t;
			}
			txt_pointer = rt->value;
			if(extra_data[i].translate)
				txt_pointer = gettext(rt->value);

			extra_data[i].align = rt->align;
			rlib_align_text(r, &extra_data[i].formatted_string, txt_pointer, rt->align, rt->width);
				
			extra_data[i].width = rt->width;
			rlib_execute_pcode(r, &extra_data[i].rval_col, rt->col_code, NULL);	
		} else if(e->type == RLIB_ELEMENT_IMAGE) {
			gfloat height, width, text_width ;
			ri = e->data;
			rlib_execute_pcode(r, &extra_data[i].rval_image_name, ri->value_code, NULL);
			rlib_execute_pcode(r, &extra_data[i].rval_image_type, ri->type_code, NULL);
			rlib_execute_pcode(r, &extra_data[i].rval_image_width, ri->width_code, NULL);
			rlib_execute_pcode(r, &extra_data[i].rval_image_height, ri->height_code, NULL);
			rlib_execute_pcode(r, &extra_data[i].rval_image_textwidth, ri->textwidth_code, NULL);
			height = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data[i].rval_image_height));
			width = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data[i].rval_image_width));
			extra_data[i].font_point = rl->font_point;
			text_width = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data[i].rval_image_textwidth));
			if(text_width > 0) 
				extra_data[i].output_width = rlib_layout_advance_horizontal_margin(r, 0, text_width);
			else
				extra_data[i].output_width = RLIB_GET_LINE(width);
			
			if(rl->max_line_height < RLIB_GET_LINE(height))
				rl->max_line_height = RLIB_GET_LINE(height);
		} else if(e->type == RLIB_ELEMENT_BARCODE) {
			gfloat height;
			gchar tmpstr[128] = "RLIB_IMAGE_FILE_XXXXXX";
			int fd;
			gchar filename[128];
			gchar *barcode = "";
			struct rlib_value rval_value;
			rb = e->data;

			rlib_execute_pcode(r, &rval_value, rb->value_code, NULL);
			if(RLIB_VALUE_IS_STRING(&rval_value)) {
				barcode = RLIB_VALUE_GET_AS_STRING(&rval_value);   			
			} else {
				r_error(r, "Barcode values must be strings!");
			}

			rlib_execute_pcode(r, &extra_data[i].rval_image_height, rb->height_code, NULL);			
			fd = mkstemp(tmpstr);
			if (fd >= 0) {
				close(fd);
				unlink(tmpstr);
			} else {
				r_error(r, "mkstemp returned error!");
			}
			sprintf(filename, "%s.png", tmpstr);

			height = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data[i].rval_image_height));

			if (gd_barcode_png_to_file(filename,barcode,height)) {			
				rlib_value_new_string(&extra_data[i].rval_image_name, filename);
				rlib_value_new_string(&extra_data[i].rval_image_type, "png");
			}
			
			if(rl->max_line_height < RLIB_GET_LINE(height))
				rl->max_line_height = RLIB_GET_LINE(height);

			rlib_value_free(&rval_value);
		} else {
			r_error(r, "Line has invalid content");
		}
		if(rl->font_point == -1)
			extra_data[i].font_point = use_font_point;
		else
			extra_data[i].font_point = rl->font_point;
			
		if(rl->max_line_height < RLIB_GET_LINE(extra_data[i].font_point))
			rl->max_line_height = RLIB_GET_LINE(extra_data[i].font_point);
			
		text = extra_data[i].formatted_string;

		if(extra_data[i].is_memo == FALSE) {
			if(text == NULL)
				text = (gchar *)"";
			if(extra_data[i].width == -1)
				extra_data[i].width = r_strlen(text);
			else {
				gint slen = r_strlen(text);
				if(slen > extra_data[i].width)
					*r_ptrfromindex(text, extra_data[i].width) = '\0';
				else if(slen < extra_data[i].width && MAXSTRLEN != slen) {
					gint lim = extra_data[i].width - slen, size = strlen(text);
					gchar *ptr;
					if (size + lim >= extra_data[i].width)
						extra_data[i].formatted_string = text = g_realloc(text, size + lim + 1);
					ptr = r_ptrfromindex(text, slen);
					while (lim-- > 0) {
						*ptr++ = ' ';
					}
					*ptr = '\0';
				}
			}
		}
		extra_data[i].found_bgcolor = FALSE;
		if(!RLIB_VALUE_IS_NONE((&extra_data[i].rval_bgcolor))) {
			gchar *colorstring, *ocolor;
			if(!RLIB_VALUE_IS_STRING((&extra_data[i].rval_bgcolor))) {
				r_error(r, "RLIB ENCOUNTERED AN ERROR PROCESSING THE BGCOLOR FOR THIS VALUE [%s].. BGCOLOR VALUE WAS NOT OF TYPE STRING\n", text);
			} else {
				gchar *idx;
				ocolor = colorstring = g_strdup(RLIB_VALUE_GET_AS_STRING((&extra_data[i].rval_bgcolor)));
				if(!RLIB_VALUE_IS_NONE((&extra_data[i].rval_code)) && RLIB_VALUE_IS_NUMBER((&extra_data[i].rval_code)) && strchr(colorstring, ':')) {
					colorstring = g_strdup(colorstring);
					idx = strchr(colorstring, ':');
					if(RLIB_VALUE_GET_AS_NUMBER((&extra_data[i].rval_code)) >= 0)
						idx[0] = '\0';
					else
						colorstring = idx+1;	
				}
				rlib_parsecolor(&extra_data[i].bgcolor, colorstring);
				extra_data[i].found_bgcolor = TRUE;
				g_free(ocolor);
			}
		}
		extra_data[i].found_color = FALSE;
		if(!RLIB_VALUE_IS_NONE((&extra_data[i].rval_color))) {
			gchar *colorstring, *ocolor;
			if(!RLIB_VALUE_IS_STRING((&extra_data[i].rval_color))) {
				r_error(r, "RLIB ENCOUNTERED AN ERROR PROCESSING THE COLOR FOR THIS VALUE [%s].. COLOR VALUE WAS NOT OF TYPE STRING\n", text);
			} else {
				gchar *idx;
				ocolor = colorstring = g_strdup(RLIB_VALUE_GET_AS_STRING((&extra_data[i].rval_color)));
				if(!RLIB_VALUE_IS_NONE((&extra_data[i].rval_code)) && RLIB_VALUE_IS_NUMBER((&extra_data[i].rval_code)) && strchr(colorstring, ':')) {
					idx = strchr(colorstring, ':');
					if(RLIB_VALUE_GET_AS_NUMBER((&extra_data[i].rval_code)) >= 0)
						idx[0] = '\0';
					else
						colorstring = idx+1;	
				}
				rlib_parsecolor(&extra_data[i].color, colorstring);
				extra_data[i].found_color = TRUE;
				g_free(ocolor);
			}
		}
		extra_data[i].is_bold = FALSE;
		if(!RLIB_VALUE_IS_NONE((&extra_data[i].rval_bold))) {
			if(!RLIB_VALUE_IS_NUMBER((&extra_data[i].rval_bold))) {
				r_error(r, "RLIB ENCOUNTERED AN ERROR PROCESSING BOLD FOR THIS VALUE [%s].. BOLD VALUE WAS NOT OF TYPE NUMBER TYPE=%d\n", text, RLIB_VALUE_GET_TYPE((&extra_data[i].rval_bold)));
			} else {
				if(RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER((&extra_data[i].rval_bold)))) {
					extra_data[i].is_bold = TRUE;
				}
			}
		}
		extra_data[i].is_italics = FALSE;
		if(!RLIB_VALUE_IS_NONE((&extra_data[i].rval_italics))) {
			if(!RLIB_VALUE_IS_NUMBER((&extra_data[i].rval_italics))) {
				r_error(r, "RLIB ENCOUNTERED AN ERROR PROCESSING ITALICS FOR THIS VALUE [%s].. ITALICS VALUE WAS NOT OF TYPE NUMBER\n", text);
			} else {
				if(RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER((&extra_data[i].rval_italics))))
					extra_data[i].is_italics = TRUE;
			}
		}

		extra_data[i].col = 0;
		if(!RLIB_VALUE_IS_NONE((&extra_data[i].rval_col))) {
			if(!RLIB_VALUE_IS_NUMBER((&extra_data[i].rval_col))) {
				r_error(r, "RLIB EXPECTS A expN FOR A COLUMN... text=[%s]\n", text);
			} else {
				extra_data[i].col = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER((&extra_data[i].rval_col)));
			}
		}
		extra_data[i].found_link = FALSE;
		if(!RLIB_VALUE_IS_NONE((&extra_data[i].rval_link))) {
			if(!RLIB_VALUE_IS_STRING((&extra_data[i].rval_link))) {
				r_error(r, "RLIB ENCOUNTERED AN ERROR PROCESSING THE LINK FOR THIS VALUE [%s].. LINK VALUE WAS NOT OF TYPE STRING [%d]\n", text, RLIB_VALUE_GET_TYPE((&extra_data[i].rval_link)));
			} else {
				extra_data[i].link = RLIB_VALUE_GET_AS_STRING((&extra_data[i].rval_link));
				if(extra_data[i].link != NULL && strcmp(extra_data[i].link, "")) {
					extra_data[i].found_link = TRUE;
				}
			}
		}
		if(extra_data[i].type != RLIB_ELEMENT_IMAGE && extra_data[i].type != RLIB_ELEMENT_BARCODE)
			extra_data[i].output_width = rlib_layout_estimate_string_width_from_extra_data(r, &extra_data[i]);		
		i++;
	}
	rlib_value_free(&line_rval_color);
	rlib_value_free(&line_rval_bgcolor);
	rlib_value_free(&line_rval_bold);
	rlib_value_free(&line_rval_italics);
	return line_has_memo;
}	

static void rlib_layout_find_common_properties_in_a_line(rlib *r, struct rlib_line_extra_data *extra_data, gint count, gint delayed) {
	gint i = 0;
	struct rlib_line_extra_data *e_ptr = NULL, *save_ptr = NULL, *previous_ptr = NULL;
	gint state = STATE_NONE;
	
	previous_ptr = &extra_data[i];
	for(i=0;i<count;i++) {
		e_ptr = &extra_data[i];
		if(e_ptr->found_bgcolor) {
			if(state == STATE_NONE) {
				save_ptr = e_ptr;
				state = STATE_BGCOLOR;
				e_ptr->running_bgcolor_status |= STATUS_START;
				e_ptr->running_bg_total = e_ptr->output_width;
			} else {
				if(!memcmp(&save_ptr->bgcolor, &e_ptr->bgcolor, sizeof(struct rlib_rgb))) {
					save_ptr->running_bg_total += e_ptr->output_width;
				} else {
					save_ptr = e_ptr;
					previous_ptr->running_bgcolor_status |= STATUS_STOP;
					e_ptr->running_bgcolor_status |= STATUS_START;
					e_ptr->running_bg_total = e_ptr->output_width;
				}
			}
		} else {
			if(state == STATE_BGCOLOR) {
				previous_ptr->running_bgcolor_status |= STATUS_STOP;
				state = STATE_NONE;
			}
		}
		previous_ptr = e_ptr;
	}
	if(state == STATE_BGCOLOR) {
 		e_ptr->running_bgcolor_status |= STATUS_STOP;
	}

	i = 0;
	e_ptr = NULL;
	save_ptr = NULL;
	previous_ptr = NULL;
	state = STATE_NONE;
	previous_ptr = &extra_data[i];
	for(i=0;i<count;i++) {
		e_ptr = &extra_data[i];
		if(e_ptr->found_link) {
			if(state == STATE_NONE) {
				save_ptr = e_ptr;
				state = STATE_BGCOLOR;
				e_ptr->running_link_status |= STATUS_START;
				e_ptr->running_link_total = e_ptr->output_width;
			} else {
				if(!strcmp(save_ptr->link, e_ptr->link)) {
					save_ptr->running_link_total += e_ptr->output_width;
				} else {
					save_ptr = e_ptr;
					previous_ptr->running_link_status |= STATUS_STOP;
					e_ptr->running_link_status |= STATUS_START;
					e_ptr->running_link_total = e_ptr->output_width;
				}
			}
		} else {
			if(state == STATE_BGCOLOR) {
				if(OUTPUT(r)->trim_links) {
					e_ptr->running_bgcolor_status |= STATUS_START;
					e_ptr->running_bgcolor_status |= STATUS_STOP;					
					previous_ptr->running_bgcolor_status |= STATUS_START;
					previous_ptr->running_bgcolor_status |= STATUS_STOP;
				}
				previous_ptr->running_link_status |= STATUS_STOP;
				state = STATE_NONE;
			}
		}
		previous_ptr = e_ptr;
	}
	if(state == STATE_BGCOLOR) {
 		e_ptr->running_link_status |= STATUS_STOP;
	}
}

/*
	TODO: Don't group text if any of the line output is delayed
	Then dup the extra_data[] in a queue for later and add it as a delayed write thingie!!!	
*/
static gint rlib_layout_report_output_array(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_report_output_array *roa, 
	gint backwards, gint page, gboolean page_header_layout) {
	struct rlib_element *e=NULL;
	gint j=0;
	gfloat margin=0, width=0;
	gfloat *rlib_position;
	struct rlib_line_extra_data *extra_data;
	gint output_count = 0;
	gfloat my_left_margin;
	
	if(roa == NULL || roa->suppress == TRUE)
		return 0;
	
	r->current_line_number++;
	
	OUTPUT(r)->start_report_line(r, part, report);	
	
	if(report != NULL) {
		if(backwards)
			rlib_position = &report->position_bottom[page-1];
		else
			rlib_position = &report->position_top[page-1];	
		my_left_margin = report->left_margin;
	} else {
		if(backwards)
			rlib_position = &part->position_bottom[page-1];
		else
			rlib_position = &part->position_top[page-1];
		my_left_margin = part->left_margin;
	}
	
	if(report != NULL) {
		if(report->detail_columns > 1) {
			gfloat paper_width = (rlib_layout_get_page_width(r, part) - (part->left_margin * 2)) / report->detail_columns;
			my_left_margin += ((r->detail_line_count % report->detail_columns) * paper_width) + ((r->detail_line_count % report->detail_columns) * report->column_pad);		
		}	
	}

	for(j=0;j<roa->count;j++) {
		struct rlib_report_output *ro = roa->data[j];

		if(ro->type == RLIB_REPORT_PRESENTATION_DATA_LINE) {
			struct rlib_report_lines *rl = ro->data;
			gint count=0;
			gint delayed = FALSE;
			gboolean has_memo = TRUE;
			gint max_memo_lines = 0;
			gint i;
			gint total_count = 0;
			
			if(rlib_check_is_not_suppressed(r, rl->suppress_code)) {
				output_count++;
				OUTPUT(r)->start_line(r, backwards);

				for(e = rl->e; e != NULL; e=e->next)
					count++;

				extra_data = g_new0(struct rlib_line_extra_data, count);
				for (i = 0; i < count; i++)
					extra_data[i].report_index = part->report_index;
				has_memo = rlib_layout_execute_pcodes_for_line(r, part, report, rl, extra_data, &delayed);
				rlib_layout_find_common_properties_in_a_line(r, extra_data, count, delayed);
				count = 0;
				if(has_memo) {
					for(e = rl->e; e != NULL; e=e->next) {
						if(extra_data[count].is_memo == TRUE) {
							if(extra_data[count].memo_line_count > max_memo_lines) {
								max_memo_lines = extra_data[count].memo_line_count;
							}
						}
						
						count++;
					}
				}

				if(max_memo_lines < 1)
					max_memo_lines = 1;
				for(i=1; i <= max_memo_lines; i++) {				
					margin = my_left_margin;
					
					if(rlib_will_this_fit(r, part, report, RLIB_GET_LINE(get_font_point(r, part, report, rl)), 1) == FALSE && max_memo_lines > 1) {
						if(page_header_layout == FALSE) {
							if(report != NULL) {
/* We need to let the layout engine know this has nothing to do w/ pages across and then we have a really long memo field in the report header some how */							
								report->raw_page_number = r->current_page_number;
							}
							rlib_layout_end_page(r, part, report, TRUE);
							rlib_force_break_headers(r, part, report, TRUE);

						} else {
							rlib_layout_end_page(r, part, report, FALSE);
						}
					} else if(i>= 2) {
						/* Things like HTML Output need to have the lines ended..For Memo Fields that go over 1 line */
						if(OUTPUT(r)->do_grouptext && !delayed) {
						
						} else {
							OUTPUT(r)->end_line(r, backwards);	
						}
					}
					
					count = 0;
					if(OUTPUT(r)->do_grouptext && !delayed) {
						gchar buf[MAXSTRLEN];
						gchar spaced_out[MAXSTRLEN];
						gfloat fun_width=0;
						gfloat bg_width = 0;
						gfloat bg_margin = margin;
						gint start_count=-1;
						for(e = rl->e; e != NULL; e=e->next) {
							count++;
						}
						total_count = count;
						count=0;
						margin = my_left_margin;				
						buf[0] = 0;
						width = 0;
						start_count = -1;
						for(e = rl->e; e != NULL; e=e->next) {
							gboolean next_field_bg_color_changed = FALSE;
							if(extra_data[count].running_bgcolor_status & STATUS_START)
								next_field_bg_color_changed = TRUE;
							if(count+1 < total_count) {
								if(extra_data[count+1].running_bgcolor_status & STATUS_STOP)
									next_field_bg_color_changed = TRUE;		
							}

/////////////////HERE							
							if(e->type == RLIB_ELEMENT_FIELD) {
								struct rlib_report_field *rf = ((struct rlib_report_field *)e->data);
								rf->rval = &extra_data[count].rval_code;
								bg_width = rlib_layout_output_extras(r, part, backwards, bg_margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), 
										&extra_data[count]);
							} else if(e->type == RLIB_ELEMENT_LITERAL) {
								bg_width = rlib_layout_output_extras(r, part, backwards, bg_margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), 
									&extra_data[count]);
							} else if(e->type == RLIB_ELEMENT_IMAGE) {
								bg_width = extra_data[count].output_width;
							} else {
								bg_width = 0;
							}
							bg_margin += bg_width;
///////////////////END HERE							

							if(extra_data[count].found_color == FALSE && extra_data[count].is_bold == FALSE && extra_data[count].is_italics == FALSE 
							&& extra_data[count].is_memo == FALSE && extra_data[count].type != RLIB_ELEMENT_IMAGE 
							&& extra_data[count].type != RLIB_ELEMENT_BARCODE && next_field_bg_color_changed == FALSE) {
								gchar *tmp_string;
								if(start_count == -1)
									start_count = count;
								tmp_string = rlib_layout_suppress_non_memo_on_extra_lines(r, &extra_data[count], i, spaced_out);
								if(tmp_string != NULL)
									strcat(buf, tmp_string);
								fun_width += extra_data[count].output_width;
							} else {
								if(start_count != -1) {
									rlib_layout_text_string(r, backwards, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), 
										&extra_data[start_count], buf, i);
									start_count = -1;
									margin += fun_width;
									fun_width = 0;
									buf[0] = 0;
								}
								
								width = rlib_layout_text_from_extra_data(r, backwards, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), 
									&extra_data[count], TEXT_NORMAL, i);
								margin += width;

							}
							count++;					
						}
						if(start_count != -1) {
							width += fun_width;
							rlib_layout_text_string(r, backwards, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), &extra_data[start_count], buf, i);
						}
					} else { /* Not Group Next Follows */

						for(e = rl->e; e != NULL; e=e->next) {
							if(e->type == RLIB_ELEMENT_FIELD) {
								struct rlib_report_field *rf = ((struct rlib_report_field *)e->data);
								rf->rval = &extra_data[count].rval_code;
								rlib_layout_output_extras_start(r, part, backwards, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl),
									 &extra_data[count], FALSE);
								width = rlib_layout_text_from_extra_data(r, backwards, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), 
									&extra_data[count], TEXT_LEFT, i);
								rlib_layout_output_extras_end(r, part, backwards, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), 
									&extra_data[count], i);
							} else if(e->type == RLIB_ELEMENT_LITERAL) {
								rlib_layout_output_extras_start(r, part, backwards, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), 
									&extra_data[count], FALSE);
								width = rlib_layout_text_from_extra_data(r, backwards, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), 
									&extra_data[count], TEXT_LEFT, i);
								rlib_layout_output_extras_end(r, part, backwards, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), 
									&extra_data[count], i);
							} else if(e->type == RLIB_ELEMENT_IMAGE) {
								gchar *filename;
								gfloat height1 = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data[count].rval_image_height));
								gfloat width1 = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data[count].rval_image_width));
								gchar *name = RLIB_VALUE_GET_AS_STRING(&extra_data[count].rval_image_name);
								gchar *type = RLIB_VALUE_GET_AS_STRING(&extra_data[count].rval_image_type);
								filename = get_filename(r, name, part->report_index, FALSE);
								OUTPUT(r)->line_image(r, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), filename, type, width1, height1);
								g_free(filename);
								width = RLIB_GET_LINE(width1);
							}  else if(e->type == RLIB_ELEMENT_BARCODE) {
								gchar *filename;
								gfloat height1 = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data[count].rval_image_height));
								gfloat width1 = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(&extra_data[count].rval_image_width));
								gchar *name = RLIB_VALUE_GET_AS_STRING(&extra_data[count].rval_image_name);
								gchar *type = RLIB_VALUE_GET_AS_STRING(&extra_data[count].rval_image_type);
								filename = get_filename(r, name, part->report_index, FALSE);
								OUTPUT(r)->line_image(r, margin, rlib_layout_get_next_line(r, part, *rlib_position, rl), filename, type, width1, height1);
								g_free(filename);
								width = RLIB_GET_LINE(width1);
							}										
							margin += width;
							count++;
						}
					}
					rlib_advance_vertical_position(r, rlib_position, rl);
				}

				OUTPUT(r)->end_line(r, backwards);	

				count=0;
				for(e = rl->e; e != NULL; e=e->next) {
					rlib_value_free(&extra_data[count].rval_code);
					rlib_value_free(&extra_data[count].rval_link);
					rlib_value_free(&extra_data[count].rval_bgcolor);
					rlib_value_free(&extra_data[count].rval_color);
					rlib_value_free(&extra_data[count].rval_col);
					rlib_value_free(&extra_data[count].rval_bold);
					rlib_value_free(&extra_data[count].rval_italics);
					rlib_value_free(&extra_data[count].rval_image_name);
					rlib_value_free(&extra_data[count].rval_image_type);
					rlib_value_free(&extra_data[count].rval_image_width);
					rlib_value_free(&extra_data[count].rval_image_height);
					rlib_value_free(&extra_data[count].rval_image_textwidth);
					g_free(extra_data[count].formatted_string);
					if(extra_data[count].memo_lines != NULL) {
						GSList *list;
						for(list = extra_data[count].memo_lines; list != NULL; list=list->next)
							g_free(list->data);
						g_slist_free(list);
					}
					count++;
				}
				g_free(extra_data);
			}
		} else if(ro->type == RLIB_REPORT_PRESENTATION_DATA_HR) {
			gchar *colorstring;
			struct rlib_value rval2, *rval=&rval2;
			struct rlib_report_horizontal_line *rhl = ro->data;
			if(rlib_check_is_not_suppressed(r, rhl->suppress_code)) {
				rlib_execute_pcode(r, &rval2, rhl->bgcolor_code, NULL);				
				if(!RLIB_VALUE_IS_STRING(rval)) {
					r_error(r, "RLIB ENCOUNTERED AN ERROR PROCESSING THE BGCOLOR FOR A HR.. COLOR VALUE WAS NOT OF TYPE STRING\n");
				} else {
					gfloat font_point;
					gfloat indent;
					gfloat length;
					gfloat tmp_rlib_position;
					gfloat f;					
					struct rlib_rgb bgcolor;
					output_count++;
					colorstring = RLIB_VALUE_GET_AS_STRING(rval);
					rlib_parsecolor(&bgcolor, colorstring);
					if(rhl->font_point == -1)
						font_point = r->font_point;
					else
						font_point = rhl->font_point;
					OUTPUT(r)->set_font_point(r, font_point);
					
					rhl->indent = 0;
					if (rlib_execute_as_float(r, rhl->indent_code, &f))
						rhl->indent = f;					
					
					indent = rlib_layout_estimate_string_width(r, rhl->indent);			
					length = rlib_layout_estimate_string_width(r, rhl->length);			
					OUTPUT(r)->set_font_point(r, r->font_point);

					if(length == 0)
						OUTPUT(r)->hr(r, backwards, my_left_margin+indent, rlib_layout_get_next_line_by_font_point(r, part, *rlib_position, 
							rhl->size),rlib_layout_get_report_width(r, part, report)-indent, rhl->size, &bgcolor, indent, length);
					else
						OUTPUT(r)->hr(r, backwards, my_left_margin+indent, rlib_layout_get_next_line_by_font_point(r, part, *rlib_position, rhl->size),
							length, rhl->size, &bgcolor, indent, length);

					tmp_rlib_position = (rhl->size/RLIB_PDF_DPI);
					*rlib_position += tmp_rlib_position;
					rlib_value_free(rval);
				}
			}
		} else if(ro->type == RLIB_REPORT_PRESENTATION_DATA_IMAGE) {
			struct rlib_value rval2, rval3, rval4, rval5, *rval_value=&rval2, *rval_width=&rval3, *rval_height=&rval4, *rval_type=&rval5;
			struct rlib_report_image *ri = ro->data;
			rlib_execute_pcode(r, &rval2, ri->value_code, NULL);
			rlib_execute_pcode(r, &rval5, ri->type_code, NULL);
			rlib_execute_pcode(r, &rval3, ri->width_code, NULL);
			rlib_execute_pcode(r, &rval4, ri->height_code, NULL);
			if(!RLIB_VALUE_IS_STRING(rval_value) || !RLIB_VALUE_IS_NUMBER(rval_width) || !RLIB_VALUE_IS_NUMBER(rval_height)) {
				r_error(r, "RLIB ENCOUNTERED AN ERROR PROCESSING THE BGCOLOR FOR A IMAGE\n");
			} else {
				gfloat height1 = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(rval_height));
				gfloat width1 = RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(rval_width));
				gchar *name = RLIB_VALUE_GET_AS_STRING(rval_value);
				gchar *type = RLIB_VALUE_GET_AS_STRING(rval_type);
				gchar *filename;
				output_count++;
				filename = get_filename(r, name, part->report_index, FALSE);
				OUTPUT(r)->background_image(r, my_left_margin, rlib_layout_get_next_line_by_font_point(r, part, *rlib_position, height1), filename,
					type, width1, height1);
				g_free(filename);
				rlib_value_free(rval_value);
				rlib_value_free(rval_width);
				rlib_value_free(rval_height);
				rlib_value_free(rval_type);
			}
		}
	}

	OUTPUT(r)->end_report_line(r, part, report);	

	return output_count;
}	

static gint rlib_layout_report_outputs_across_pages(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_element *report_outputs, 
	gint backwards, gboolean page_header_layout) {
	struct rlib_report_output_array *roa;
	gint page;
	gint i;
	gint output_count = 0;
	for(; report_outputs != NULL; report_outputs=report_outputs->next) {
		roa = report_outputs->data;
		page = roa->page;
		for(i=0;i<part->pages_across;i++) {
			OUTPUT(r)->set_working_page(r, part, i);
			OUTPUT(r)->start_output_section(r, roa);
		}
		
		if(page >= 1) {
			OUTPUT(r)->set_working_page(r, part, roa->page-1);
			output_count += rlib_layout_report_output_array(r, part, report, roa, backwards, roa->page, page_header_layout);
		} else {
			if(OUTPUT(r)->do_breaks) {
				for(i=0;i<part->pages_across;i++) {
					OUTPUT(r)->set_working_page(r, part, i);
					output_count = rlib_layout_report_output_array(r, part, report, roa, backwards, i+1, page_header_layout);
				}
			} else { /*Only go once Otherwise CVS Might Look Funny*/
				for(i=0;i<1;i++) {
					OUTPUT(r)->set_working_page(r, part, i);
					output_count = rlib_layout_report_output_array(r, part, report, roa, backwards, i+1, page_header_layout);
				}			
			}
		}
		for(i=0;i<part->pages_across;i++) {
			OUTPUT(r)->set_working_page(r, part, i);
			OUTPUT(r)->end_output_section(r, roa);
		}
	}
	return output_count;
}

gint rlib_layout_report_output(rlib *r, struct rlib_part *part, struct rlib_report *report, struct rlib_element *e, gint backwards, 
gboolean page_header_layout) {
	gint output_count = 0;
	OUTPUT(r)->start_evil_csv(r);
	output_count = rlib_layout_report_outputs_across_pages(r, part, report, e, backwards, page_header_layout);
	OUTPUT(r)->end_evil_csv(r);
	return output_count;
}

gint rlib_layout_report_output_with_break_headers(rlib *r, struct rlib_part *part, struct rlib_report *report, gboolean page_header_layout) {
	struct rlib_element *e;
	gint output_count = 0;
	gint i;
	
	OUTPUT(r)->start_evil_csv(r);

	if(report->breaks != NULL) {
		for(e = report->breaks; e != NULL; e=e->next) {
			struct rlib_report_break *rb = e->data;
			gint blank = TRUE;
			gint suppress = FALSE;
			if(rb->suppressblank) {
				struct rlib_element *be;
				suppress = TRUE;
				for(be = rb->fields; be != NULL; be=be->next) {
					struct rlib_break_fields *bf = be->data;
					if((bf->rval == NULL || (RLIB_VALUE_IS_STRING(bf->rval) && !strcmp(RLIB_VALUE_GET_AS_STRING(bf->rval), ""))) && blank == TRUE)
						blank = TRUE;
					else
						blank = FALSE;
				}		
			}
			if(!suppress || (suppress && !blank)) {
				OUTPUT(r)->start_report_break_header(r, part, report, rb);	
				output_count += rlib_layout_report_outputs_across_pages(r, part, report, rb->header, FALSE, page_header_layout);
				OUTPUT(r)->end_report_break_header(r, part, report, rb);	
			}
		}		
	}
	for(i=0;i<report->pages_across;i++) {
		OUTPUT(r)->set_working_page(r, part, i);
		OUTPUT(r)->start_report_field_details(r, part, report);	
	}

	rlib_layout_report_outputs_across_pages(r, part, report, report->detail.fields, FALSE, FALSE);

	for(i=0;i<report->pages_across;i++) {
		OUTPUT(r)->set_working_page(r, part, i);
		OUTPUT(r)->end_report_field_details(r, part, report);
	}
	OUTPUT(r)->end_evil_csv(r);

	return output_count;
}


gfloat rlib_layout_get_page_width(rlib *r, struct rlib_part *part) {
	if(!part->landscape)
		return (part->paper->width/RLIB_PDF_DPI);
	else
		return (part->paper->height/RLIB_PDF_DPI);
}


void rlib_layout_report_footer(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	gint i;
	if(report->report_footer != NULL) {
		for(i=0;i<report->pages_across;i++)
			rlib_end_page_if_line_wont_fit(r, part, report, report->report_footer);
		OUTPUT(r)->start_report_footer(r, part, report);
		rlib_layout_report_output(r, part, report, report->report_footer, FALSE, FALSE);
		OUTPUT(r)->end_report_footer(r, part, report);
	}
}


void rlib_layout_init_report_page(rlib *r, struct rlib_part *part, struct rlib_report *report) {
	int i;
	
	for(i=0;i<report->pages_across;i++) {
		OUTPUT(r)->set_working_page(r, part, i);
		OUTPUT(r)->start_report_field_headers(r, part, report);
	}

	rlib_layout_report_output(r, part, report, report->detail.headers, FALSE, FALSE);

	for(i=0;i<report->pages_across;i++) {
		OUTPUT(r)->set_working_page(r, part, i);
		OUTPUT(r)->end_report_field_headers(r, part, report);
	}
}

gint rlib_layout_end_page(rlib *r, struct rlib_part *part, struct rlib_report *report, gboolean normal) {
	if(report != NULL) {
		if(report->raw_page_number < r->current_page_number) {
			OUTPUT(r)->end_page_again(r, part, report);
			report->raw_page_number++;
			OUTPUT(r)->set_raw_page(r, part, report->raw_page_number);
		} else {
			OUTPUT(r)->end_page(r, part);
			if(report->font_size != -1)
				r->font_point = report->font_size;		
			rlib_layout_init_part_page(r, part, FALSE, normal);
			report->raw_page_number++;
		}
		rlib_set_report_from_part(r, part, report, 0);
		rlib_layout_init_report_page(r, part, report);
	} else {
		rlib_layout_init_part_page(r, part, FALSE, normal);
	}
	return TRUE;
}

void rlib_layout_init_part_page(rlib *r, struct rlib_part *part, gboolean first, gboolean normal) {
	gint i;
	gint save_font_size = r->font_point;
	if(part->font_size != -1)
		r->font_point = part->font_size;

	for(i=0;i<part->pages_across;i++) {
		part->position_top[i] = part->top_margin;
		part->bottom_size[i] = get_outputs_size(r, part, NULL, part->page_footer, i);
	}		
	r->current_font_point = -1;
	OUTPUT(r)->start_new_page(r, part);
	
	/* It's important that we set the font point for all pages across (RPDF)  Above start_new_page allocates pages_across new pages*/
	for(i=0;i<part->pages_across;i++) {
		r->current_font_point = -1;
		OUTPUT(r)->set_working_page(r, part, i);
		OUTPUT(r)->set_font_point(r, r->font_point);
	}
	
	OUTPUT(r)->set_working_page(r, part, 0);
	
	for(i=0; i<part->pages_across; i++)
		part->position_bottom[i] -= part->bottom_size[i];

	for(i=0; i<part->pages_across; i++) {
		OUTPUT(r)->set_working_page(r, part, i);
		OUTPUT(r)->start_part_page_footer(r, part);
	}
	
	rlib_layout_report_output(r, part, NULL, part->page_footer, TRUE, FALSE);
	
	for(i=0; i<part->pages_across; i++) {
		OUTPUT(r)->set_working_page(r, part, i);
		OUTPUT(r)->end_part_page_footer(r, part);
	}
	
	for(i=0; i<part->pages_across; i++) {
		part->position_bottom[i] -= part->bottom_size[i];

	}

	if(normal) {
		if(first) {
			//bobd
			for(i=0; i<part->pages_across; i++) {
				OUTPUT(r)->set_working_page(r, part, i);
				OUTPUT(r)->start_part_header(r, part);
			}

			rlib_layout_report_output(r, part, NULL, part->report_header, FALSE, TRUE);

			for(i=0; i<part->pages_across; i++) {
				OUTPUT(r)->set_working_page(r, part, i);
				OUTPUT(r)->end_part_header(r, part);
			}
		}
		if(r->current_page_number == 1 && part->suppress_page_header_first_page == TRUE) {
			// We don't print the page header in this case
		} else {
			for(i=0; i<part->pages_across; i++) {
				OUTPUT(r)->set_working_page(r, part, i);
				OUTPUT(r)->start_part_page_header(r, part);
			}

			rlib_layout_report_output(r, part, NULL, part->page_header, FALSE, TRUE);

			for(i=0; i<part->pages_across; i++) {
				OUTPUT(r)->set_working_page(r, part, i);
				OUTPUT(r)->end_part_page_header(r, part);
			}
		}
	}

	OUTPUT(r)->init_end_page(r);
	r->font_point = save_font_size;
}
