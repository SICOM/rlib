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

#include <config.h>
 
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"
#include "util.h"
#include "rlib_langinfo.h"

int goodIncs_normal[15] = {1, 2, 3, 4, 5, 8, 10, 15, 20, 25, 30, 40, 50, 60, 75};
int numGoodIncs_normal = 15;
int goodIncs_15[6] = {3, 5, 7, 11, 13, 15};
int numGoodIncs_15 = 6;

#define MAX_COLOR_POOL 20 

const gchar *color_pool[MAX_COLOR_POOL] = { "0x4684ee", "0xdc3912", "0xff9900", "0x008000", 
                                            "0x666666", "0x4942cc", "0xcb4ac5", "0xd6ae00", 
                                            "0x336699", "0xdd4477", "0xaaaa11", "0x66aa00", 
                                            "0x888888", "0x994499", "0xdd5511", "0x22aa99", 
                                            "0x999999", "0x705770", "0x109618", "0xa32929" };

static gboolean is_row_graph(gint graph_type) {
	if(graph_type == RLIB_GRAPH_TYPE_ROW_NORMAL || graph_type == RLIB_GRAPH_TYPE_ROW_PERCENT || graph_type == RLIB_GRAPH_TYPE_ROW_STACKED)
		return TRUE;
	return FALSE;
}

static gboolean is_line_graph(gint graph_type) {
	if(graph_type == RLIB_GRAPH_TYPE_LINE_NORMAL || graph_type == RLIB_GRAPH_TYPE_LINE_PERCENT || graph_type == RLIB_GRAPH_TYPE_LINE_STACKED)
		return TRUE;
	return FALSE;
}

static gboolean is_pie_graph(gint graph_type) {
	if(graph_type == RLIB_GRAPH_TYPE_PIE_NORMAL || graph_type == RLIB_GRAPH_TYPE_PIE_RING || graph_type == RLIB_GRAPH_TYPE_PIE_OFFSET)
		return TRUE;
	return FALSE;
}

static gboolean is_percent_graph(gint graph_type) {
	if(graph_type == RLIB_GRAPH_TYPE_ROW_PERCENT || graph_type == RLIB_GRAPH_TYPE_LINE_PERCENT)
		return TRUE;
	return FALSE;
}

static gboolean is_stacked_graph(gint graph_type) {
	if(graph_type == RLIB_GRAPH_TYPE_ROW_STACKED || graph_type == RLIB_GRAPH_TYPE_LINE_STACKED)
		return TRUE;
	return FALSE;
}

gint determine_graph_type(gchar *type, gchar *subtype) {
	if(strcmp(type, "line") == 0) {
		if(strcmp(subtype, "stacked") == 0)
			return RLIB_GRAPH_TYPE_LINE_STACKED;
		else if(strcmp(subtype, "percent") == 0)
			return RLIB_GRAPH_TYPE_LINE_PERCENT;
		else
			return RLIB_GRAPH_TYPE_LINE_NORMAL;
	} else if(strcmp(type, "area") == 0) {
		if(strcmp(subtype, "stacked") == 0)
			return RLIB_GRAPH_TYPE_AREA_STACKED;
		else if(strcmp(subtype, "percent") == 0)
			return RLIB_GRAPH_TYPE_AREA_PERCENT;
		else
			return RLIB_GRAPH_TYPE_AREA_NORMAL;
	} else if(strcmp(type, "column") == 0) {
		if(strcmp(subtype, "stacked") == 0)
			return RLIB_GRAPH_TYPE_COLUMN_STACKED;
		else if(strcmp(subtype, "percent") == 0)
			return RLIB_GRAPH_TYPE_COLUMN_PERCENT;
		else
			return RLIB_GRAPH_TYPE_COLUMN_NORMAL;
	} else if(strcmp(type, "row") == 0) {
		if(strcmp(subtype, "stacked") == 0)
			return RLIB_GRAPH_TYPE_ROW_STACKED;
		else if(strcmp(subtype, "percent") == 0)
			return RLIB_GRAPH_TYPE_ROW_PERCENT;
		else
			return RLIB_GRAPH_TYPE_ROW_NORMAL;
	} else if(strcmp(type, "pie") == 0) {
		if(strcmp(subtype, "ring") == 0)
			return RLIB_GRAPH_TYPE_PIE_RING;
		else if(strcmp(subtype, "offset") == 0)
			return RLIB_GRAPH_TYPE_PIE_OFFSET;
		else
			return RLIB_GRAPH_TYPE_PIE_NORMAL;	
	} else if(strcmp(type, "xy") == 0) {
		if(strcmp(subtype, "lines with symbols") == 0)
			return RLIB_GRAPH_TYPE_XY_LINES_WITH_SUMBOLS;
		else if(strcmp(subtype, "lines only") == 0)
			return RLIB_GRAPH_TYPE_XY_LINES_ONLY;
		else if(strcmp(subtype, "cubic spline") == 0)
			return RLIB_GRAPH_TYPE_XY_CUBIC_SPLINE;
		else if(strcmp(subtype, "cubic spline with symbols") == 0)
			return RLIB_GRAPH_TYPE_XY_CUBIC_SPLINE_WIHT_SYMBOLS;
		else if(strcmp(subtype, "bspline") == 0)
			return RLIB_GRAPH_TYPE_XY_BSPLINE;
		else if(strcmp(subtype, "bspline with symbols") == 0)
			return RLIB_GRAPH_TYPE_XY_BSPLINE_WITH_SYMBOLS;
		else
			return RLIB_GRAPH_TYPE_XY_SYMBOLS_ONLY;
	}
	return RLIB_GRAPH_TYPE_ROW_NORMAL;
}

/*
	This is beyond evil but it seems to works.  Someone who understands floats better should really do this
*/
static void rlib_graph_label_y_axis(rlib *r, gint side, gboolean for_real, gint y_ticks, gdouble y_min, gdouble y_max, gdouble y_origin, gint decimal_hint) {
	gint i,j,max=0;
	gchar format[20];
	gint max_slen = 0;
	if(decimal_hint < 0) {
		for(j=0;j<6;j++) {
			gboolean bad = FALSE;
			sprintf(format, "%%.0%df", j);
			for(i=0;i<y_ticks;i++) {
				gchar v1[50], v2[50];
				gdouble val = y_min + (((y_max-y_min)/y_ticks)*i);
				gdouble val2 = y_min + (((y_max-y_min)/y_ticks)*(i+1));
				sprintf(v1, format, val);
				sprintf(v2, format, val2);
				if(strcmp(v1, v2) == 0) {
					bad = TRUE;
					break;
				}
			}
			if(bad == FALSE)
				break;
		}
		max = j;
	} else {
		max = decimal_hint;
	}
		
	sprintf(format, "%%.0%df", max);

	for(i=0;i<y_ticks+1;i++) {
		gdouble val = y_min + (((y_max-y_min)/y_ticks)*i);
		gchar label[MAXSTRLEN];		
		sprintf(label, format, val);
		if(strlen(label) > max_slen)
			max_slen = strlen(label);
	}

	sprintf(format, "%%0%d.0%df", max_slen-max, max);

	for(i=0;i<y_ticks+1;i++) {
		gdouble val = y_min + (((y_max-y_min)/y_ticks)*i);
		gchar label[MAXSTRLEN];		
		sprintf(label, format, val);

		if(for_real) 
			OUTPUT(r)->graph_label_y(r, side, i, label);	
		else
			OUTPUT(r)->graph_hint_label_y(r, side, label);	
	}
}

#define POSITIVE 1
#define POSITIVE_AND_NEGATIVE 2
#define NEGATIVE 3
#define MAX_X_TICKS 2000

DLL_EXPORT_SYM gfloat rlib_graph(rlib *r, struct rlib_part *part, struct rlib_report *report, gfloat left_margin_offset, gfloat *top_margin_offset) {
	struct rlib_graph_plot *plot;
	struct rlib_graph *graph = &report->graph;
	GSList *list;
	gchar axis[MAXSTRLEN], x_axis_label[MAXSTRLEN], legend_label[MAXSTRLEN];
	gfloat y_value;
	gfloat stacked_y_value_max[2] = {0,0};
	gfloat stacked_y_value_min[2] = {0,0};
	gfloat y_value_try_max[2];
	gfloat y_value_try_min[2];
	gfloat col_sum = 0;
	gdouble tmi;
	gfloat running_col_sum = 0;
	gdouble y_min[2] = {0,0};
	gdouble y_max[2] = {0,0};
	gfloat *row_sum = NULL, *last_row_values=NULL, *last_row_height=NULL;
	gint data_plot_count = 0;
	gint plot_count = 0;
	gint row_count = 0;
	gint y_ticks = 10, fake_y_ticks;
	gint i = 0;
	gint y_axis_mod = 0;
	gboolean draw_x_line = TRUE, draw_y_line = TRUE, bold_titles = FALSE;
	gboolean have_right_side = FALSE;
	gint side = RLIB_SIDE_LEFT;
	gfloat tmp;
	gfloat graph_width=300, graph_height=300;
	gfloat last_height,last_height_neg,last_height_pos;
	gfloat y_origin[2] = {0,0};
	gchar data_type[2] = {POSITIVE, POSITIVE}; 
	struct rlib_rgb color[MAX_COLOR_POOL];
	struct rlib_rgb plot_color;
	gchar type[MAXSTRLEN], subtype[MAXSTRLEN], title[MAXSTRLEN], legend_bg_color[MAXSTRLEN], legend_orientation[MAXSTRLEN], x_axis_title[MAXSTRLEN];
	gchar y_axis_title[MAXSTRLEN], y_axis_title_right[MAXSTRLEN], side_str[MAXSTRLEN], grid_color[MAXSTRLEN], name[MAXSTRLEN], color_str[MAXSTRLEN];
	gint graph_type;
	gboolean divide_iterations = TRUE;
	gboolean should_label_under_tick = FALSE;
	gfloat value = 0;
	gint did_set[2] = {FALSE, FALSE};
	gint *goodIncs = goodIncs_normal;
	gint numGoodIncs = numGoodIncs_normal;
	gint left_axis_decimal_hint=-1, right_axis_decimal_hint=-1;
	gboolean disabled, tmp_disabled;
	gboolean minor_tick[MAX_X_TICKS];
	gboolean executed_label_pcode = FALSE;

	left_margin_offset += part->left_margin;

	memset(minor_tick, 0, sizeof(gboolean)*MAX_X_TICKS);
		
	if(rlib_execute_as_float(r, graph->width_code, &tmp))
		graph_width = tmp;
	if(rlib_execute_as_float(r, graph->height_code, &tmp))
		graph_height = tmp;
	if(!rlib_execute_as_string(r, graph->type_code, type, MAXSTRLEN))
		type[0] = 0;
	if(!rlib_execute_as_string(r, graph->subtype_code, subtype, MAXSTRLEN))
		subtype[0] = 0;
	if(!rlib_execute_as_string(r, graph->title_code, title, MAXSTRLEN))
		title[0] = 0;
	if(!rlib_execute_as_string(r, graph->name_code, name, MAXSTRLEN))
		name[0] = 0;
	if(!rlib_execute_as_string(r, graph->legend_bg_color_code, legend_bg_color, MAXSTRLEN))
		legend_bg_color[0] = 0;
	if(!rlib_execute_as_string(r, graph->legend_orientation_code, legend_orientation, MAXSTRLEN))
		legend_orientation[0] = 0;
	if(!rlib_execute_as_string(r, graph->grid_color_code, grid_color, MAXSTRLEN))
		grid_color[0] = 0;
	if(!rlib_execute_as_string(r, graph->x_axis_title_code, x_axis_title, MAXSTRLEN))
		x_axis_title[0] = 0;
	if(!rlib_execute_as_string(r, graph->y_axis_title_code, y_axis_title, MAXSTRLEN))
		y_axis_title[0] = 0;
	if(!rlib_execute_as_string(r, graph->y_axis_title_right_code, y_axis_title_right, MAXSTRLEN))
		y_axis_title_right[0] = 0;
	if(rlib_execute_as_int(r, graph->y_axis_mod_code, &i))
		y_axis_mod = i;
	if(rlib_execute_as_int(r, graph->y_axis_decimals_code, &i))
		left_axis_decimal_hint = i;
	if(rlib_execute_as_int(r, graph->y_axis_decimals_code_right, &i))
		right_axis_decimal_hint = i;
	if(rlib_execute_as_int(r, graph->draw_x_line_code, &i))
		draw_x_line = i;
	if(rlib_execute_as_int(r, graph->draw_y_line_code, &i))
		draw_y_line = i;
	if(rlib_execute_as_int(r, graph->bold_titles_code, &i))
		bold_titles = i;


	if(!rlib_will_this_fit(r, part, report, graph_height / RLIB_PDF_DPI, 1)) {
		*top_margin_offset = 0;
		rlib_layout_end_page(r, part, report, TRUE);
		if(report->font_size != -1) {
			r->font_point = report->font_size;
			OUTPUT(r)->set_font_point(r, r->font_point);
		}
	}
	
	graph_type = determine_graph_type(type, subtype);	

	if(is_line_graph(graph_type))
		should_label_under_tick = TRUE;

	if(graph_type == RLIB_GRAPH_TYPE_ROW_STACKED || graph_type == RLIB_GRAPH_TYPE_ROW_PERCENT) 
		divide_iterations = FALSE;
			
	row_count = 0;
	rlib_fetch_first_rows(r);
	if(!INPUT(r, r->current_result)->isdone(INPUT(r, r->current_result), r->results[r->current_result]->result)) {
		while(1) {
			row_count++;
			if(rlib_navigate_next(r, r->current_result) == FALSE) 
				break;
		}
	}

	rlib_fetch_first_rows(r);
	row_count = 0;
	OUTPUT(r)->start_graph(r, part, report, left_margin_offset, rlib_layout_get_next_line_by_font_point(r, part, part->position_top[0]+(*top_margin_offset)+report->top_margin, 0), graph_width, graph_height, should_label_under_tick);

	if(legend_orientation[0] != 0) {
		gint orientation = RLIB_GRAPH_LEGEND_ORIENTATION_RIGHT;
		if(strcmp(legend_orientation, "bottom") == 0)
			orientation = RLIB_GRAPH_LEGEND_ORIENTATION_BOTTOM;
	
		OUTPUT(r)->graph_set_legend_orientation(r, orientation);	
	}

	if(legend_bg_color[0] != 0) {
		struct rlib_rgb rgb;
		rlib_parsecolor(&rgb, legend_bg_color);
		OUTPUT(r)->graph_set_legend_bg_color(r, &rgb);	
	}

	if(grid_color[0] != 0) {
		struct rlib_rgb rgb;
		rlib_parsecolor(&rgb, grid_color);
		OUTPUT(r)->graph_set_grid_color(r, &rgb);	
	}
	
	OUTPUT(r)->graph_set_name(r, name);
	
	OUTPUT(r)->graph_set_draw_x_y(r, draw_x_line, draw_y_line);
	OUTPUT(r)->graph_set_bold_titles(r, bold_titles);
	
	rlib_fetch_first_rows(r);
	if(!INPUT(r, r->current_result)->isdone(INPUT(r, r->current_result), r->results[r->current_result]->result)) {
		while (1) {
			data_plot_count = 0;
			stacked_y_value_max[RLIB_SIDE_LEFT] = 0;;
			stacked_y_value_max[RLIB_SIDE_RIGHT] = 0;;
			stacked_y_value_min[RLIB_SIDE_LEFT] = 0;
			stacked_y_value_min[RLIB_SIDE_RIGHT] = 0;
			row_sum = g_realloc(row_sum, (row_count+1) * sizeof(gfloat));
			row_sum[row_count] = 0;
			for(list=graph->plots;list != NULL; list = g_slist_next(list)) {
				plot = list->data;
				if(rlib_execute_as_string(r, plot->axis_code, axis, MAXSTRLEN)) {
					if(strcmp(axis, "x") == 0 && !is_pie_graph(graph_type)) {
						if(rlib_execute_as_string(r, plot->field_code, x_axis_label, MAXSTRLEN)) {
							GSList *list1;
							gboolean display = TRUE;
							for(list1 = r->graph_minor_x_ticks;list1 != NULL; list1=list1->next) {
								struct rlib_graph_x_minor_tick *gmt = list1->data;
								if(strcmp(name, gmt->graph_name) == 0) {
									if(gmt->by_name == TRUE) {
										if(strcmp(x_axis_label, gmt->x_value) == 0) {
											display = FALSE;
											minor_tick[row_count] = TRUE;
										}
									} else {
										if(row_count == gmt->location) {
											display = FALSE;
											minor_tick[row_count] = TRUE;			
										}
									}
								}								
							}
							if(display)
								OUTPUT(r)->graph_hint_label_x(r, x_axis_label);
						}																
				
					} else if(strcmp(axis, "y") == 0) {
						disabled = FALSE;
						if(rlib_execute_as_boolean(r, plot->disabled_code, &tmp_disabled))
							disabled = tmp_disabled;
						if(!disabled && rlib_execute_as_float(r, plot->field_code, &y_value)) {	
							side = RLIB_SIDE_LEFT;
							if(rlib_execute_as_string(r, plot->side_code, side_str, MAXSTRLEN)) {
								if(strcmp(side_str, "right") == 0) {
									side = RLIB_SIDE_RIGHT;
									have_right_side = TRUE;
								}
							}
							
							if(rlib_execute_as_string(r, plot->label_code, legend_label, MAXSTRLEN)) {
								OUTPUT(r)->graph_hint_legend(r, legend_label);
							}
							if(row_count == 0 && did_set[side] == FALSE) {
								y_min[side] = y_max[side] = y_value;
								did_set[side] = TRUE;
							}
							
							if(is_percent_graph(graph_type) || is_pie_graph(graph_type)) {
								y_value = fabs(y_value);
								row_sum[row_count] += y_value;
							}
							
							if(is_stacked_graph(graph_type)) { 
								if( y_value >= 0 ) {
									if( stacked_y_value_max < 0 ) 
										stacked_y_value_max[side] = y_value;
									else
										stacked_y_value_max[side] += y_value;
									stacked_y_value_min[side] = y_value;
								} else { 
									if( stacked_y_value_min[side] > 0 ) {
										stacked_y_value_min[side] = 0;
									}
									stacked_y_value_min[side] += y_value;
									stacked_y_value_max[side] = y_value;
								}
								y_value_try_max[side] = stacked_y_value_max[side];
								y_value_try_min[side] = stacked_y_value_min[side];
							} else {
								y_value_try_max[side] = y_value;
								y_value_try_min[side] = y_value;
							}
							rlib_parsecolor(&color[data_plot_count%MAX_COLOR_POOL], color_pool[data_plot_count%MAX_COLOR_POOL]);
							data_plot_count++;
							if(y_value_try_min[side] < y_min[side])
								y_min[side] = y_value_try_min[side];
							if(y_value_try_max[side] > y_max[side])
								y_max[side] = y_value_try_max[side];
						}
						if(is_pie_graph(graph_type)) {
							col_sum += fabs(y_value);
							rlib_parsecolor(&color[row_count%MAX_COLOR_POOL], color_pool[row_count%MAX_COLOR_POOL]);			
							if(rlib_execute_as_string(r, plot->label_code, legend_label, MAXSTRLEN)) {
								OUTPUT(r)->graph_hint_legend(r, legend_label);
							}				
						}
					
					}
				}
			}
			row_count++;

			if(rlib_navigate_next(r, r->current_result) == FALSE) 
				break;
		}
	}

	if((y_min[RLIB_SIDE_LEFT]> 1 || y_min[RLIB_SIDE_LEFT] < 1) && y_min[RLIB_SIDE_LEFT] != (gint)y_min[RLIB_SIDE_LEFT])
		y_min[RLIB_SIDE_LEFT] = (long)y_min[RLIB_SIDE_LEFT]-1;
		
	if((y_max[RLIB_SIDE_LEFT] > 1 || y_max[RLIB_SIDE_LEFT] < 1) && y_max[RLIB_SIDE_LEFT] != (gint)y_max[RLIB_SIDE_LEFT])
		y_max[RLIB_SIDE_LEFT] = (long)y_max[RLIB_SIDE_LEFT]+1;

	if(is_percent_graph(graph_type) || is_pie_graph(graph_type)) {
		y_min[RLIB_SIDE_LEFT] = y_min[RLIB_SIDE_RIGHT] = 0;
		y_max[RLIB_SIDE_LEFT] = y_max[RLIB_SIDE_RIGHT] = 100;
		y_ticks = 10;
	} else {
		if(y_axis_mod > 0) {
			if((gint)y_min[RLIB_SIDE_LEFT] % y_axis_mod > 0)
				y_min[RLIB_SIDE_LEFT] = (((gint)y_min[RLIB_SIDE_LEFT] / y_axis_mod)-1)*y_axis_mod;
			if((gint)y_max[RLIB_SIDE_LEFT] % y_axis_mod > 0)
				y_max[RLIB_SIDE_LEFT] = (((gint)y_max[RLIB_SIDE_LEFT] / y_axis_mod)+1)*y_axis_mod;

			if(y_axis_mod % 15 == 0) { /* TIME */
				goodIncs = goodIncs_15;
				numGoodIncs = numGoodIncs_15;
			}

		}

		if(y_min[RLIB_SIDE_LEFT] == y_max[RLIB_SIDE_LEFT]) {
			y_min[RLIB_SIDE_LEFT] = 0;
			y_ticks = 1;
		} else {
			adjust_limits(y_min[RLIB_SIDE_LEFT], y_max[RLIB_SIDE_LEFT], is_row_graph(graph_type), 5, 11, &y_ticks, &tmi, &y_min[RLIB_SIDE_LEFT], &y_max[RLIB_SIDE_LEFT], goodIncs, numGoodIncs);
		}
		if(have_right_side) {
			if(y_min[RLIB_SIDE_RIGHT] == y_max[RLIB_SIDE_RIGHT]) {
				y_min[RLIB_SIDE_RIGHT] = 0;
				y_ticks = 1;
			} else {
				adjust_limits(y_min[RLIB_SIDE_RIGHT], y_max[RLIB_SIDE_RIGHT], is_row_graph(graph_type), 2, y_ticks, &fake_y_ticks, &tmi, &y_min[RLIB_SIDE_RIGHT], &y_max[RLIB_SIDE_RIGHT], goodIncs, numGoodIncs);
			}
		}
	}
	
	if(y_axis_mod > 0) {
		if((gint)y_min[RLIB_SIDE_LEFT] % y_axis_mod > 0)
			y_min[RLIB_SIDE_LEFT] = (((gint)y_min[RLIB_SIDE_LEFT] / y_axis_mod)-1)*y_axis_mod;
		if((gint)y_max[RLIB_SIDE_LEFT] % y_axis_mod > 0)
			y_max[RLIB_SIDE_LEFT] = (((gint)y_max[RLIB_SIDE_LEFT] / y_axis_mod)+1)*y_axis_mod;
	
	}

	rlib_graph_label_y_axis(r, RLIB_SIDE_LEFT, FALSE, y_ticks, y_min[RLIB_SIDE_LEFT], y_max[RLIB_SIDE_LEFT], y_origin[RLIB_SIDE_LEFT], left_axis_decimal_hint);
	if(have_right_side)
		rlib_graph_label_y_axis(r, RLIB_SIDE_RIGHT, FALSE, y_ticks, y_min[RLIB_SIDE_RIGHT], y_max[RLIB_SIDE_RIGHT], y_origin[RLIB_SIDE_RIGHT], right_axis_decimal_hint);

	if(is_pie_graph(graph_type)) {
		OUTPUT(r)->graph_set_data_plot_count(r, row_count * data_plot_count);
		OUTPUT(r)->graph_draw_legend(r);
		i = 1;
	} else {	
		OUTPUT(r)->graph_set_data_plot_count(r, data_plot_count);
		OUTPUT(r)->graph_draw_legend(r);
		i=0;
		for(list=graph->plots;list != NULL; list = g_slist_next(list)) {
			plot = list->data;
			if(rlib_execute_as_string(r, plot->axis_code, axis, MAXSTRLEN)) {
				if(strcmp(axis, "y") == 0) {
					disabled = FALSE;
					if(rlib_execute_as_boolean(r, plot->disabled_code, &tmp_disabled))
						disabled = tmp_disabled;
					if(!disabled) {
						if(rlib_execute_as_string(r, plot->label_code, legend_label, MAXSTRLEN)) {
							if(!rlib_execute_as_string(r, plot->color_code, color_str, MAXSTRLEN)) {
								plot_color = color[i];
							} else {
								rlib_parsecolor(&plot_color, color_str);
							}
							OUTPUT(r)->graph_draw_legend_label(r, i, legend_label, &plot_color, is_line_graph(graph_type));
						}
						i++;
					}
				}
			}
		}
	}
	
	last_row_values = g_malloc(i * sizeof(gfloat));
	last_row_height = g_malloc(i * sizeof(gfloat));

	OUTPUT(r)->graph_set_title(r, title);
	
	OUTPUT(r)->graph_set_minor_ticks(r, minor_tick);
	
	if(!is_pie_graph(graph_type)) {
		OUTPUT(r)->graph_x_axis_title(r, x_axis_title);
		OUTPUT(r)->graph_y_axis_title(r, RLIB_SIDE_LEFT, y_axis_title);
		OUTPUT(r)->graph_y_axis_title(r, RLIB_SIDE_RIGHT, y_axis_title_right);
		OUTPUT(r)->graph_set_x_iterations(r, row_count);
		OUTPUT(r)->graph_do_grid(r, FALSE);
		OUTPUT(r)->graph_tick_y(r, y_ticks);
		OUTPUT(r)->graph_tick_x(r);
	} else {
		OUTPUT(r)->graph_do_grid(r, TRUE);
	}
	
	for(i=0;i<=1;i++) {
		gint use_side;
		if(i == 0)
			use_side = RLIB_SIDE_LEFT;
		else
			use_side = RLIB_SIDE_RIGHT;
		
		if(i == 1 && have_right_side == FALSE)
			break;
			
		if(y_max[use_side] >= 0 && y_min[use_side] >= 0) {
			y_origin[use_side] = y_min[use_side];
			data_type[use_side] = POSITIVE;
		} else if(y_max[use_side] > 0 && y_min[use_side] < 0) {
			y_origin[use_side] = 0;
			data_type[use_side] = POSITIVE_AND_NEGATIVE;
		} else if(y_max[use_side] <= 0 && y_min[use_side] < 0) {
			y_origin[use_side] = y_max[use_side];
			data_type[use_side] = NEGATIVE;
		}
	}
		
	if(!is_pie_graph(graph_type)) {
		OUTPUT(r)->graph_set_limits(r, RLIB_SIDE_LEFT, y_min[RLIB_SIDE_LEFT], y_max[RLIB_SIDE_LEFT], y_origin[RLIB_SIDE_LEFT]);
		rlib_graph_label_y_axis(r, RLIB_SIDE_LEFT, TRUE, y_ticks, y_min[RLIB_SIDE_LEFT], y_max[RLIB_SIDE_LEFT], y_origin[RLIB_SIDE_LEFT], left_axis_decimal_hint);
		if(have_right_side) {
			OUTPUT(r)->graph_set_limits(r, RLIB_SIDE_RIGHT, y_min[RLIB_SIDE_RIGHT], y_max[RLIB_SIDE_RIGHT], y_origin[RLIB_SIDE_RIGHT]);
			rlib_graph_label_y_axis(r, RLIB_SIDE_RIGHT, TRUE, y_ticks, y_min[RLIB_SIDE_RIGHT], y_max[RLIB_SIDE_RIGHT], y_origin[RLIB_SIDE_RIGHT], right_axis_decimal_hint);		
		}
	}

	row_count = 0;
	last_row_values[0] = 0;
	last_row_height[0] = 0;
	rlib_fetch_first_rows(r);
	if(!INPUT(r, r->current_result)->isdone(INPUT(r, r->current_result), r->results[r->current_result]->result)) {
		while (1) {
			data_plot_count = 0;
			last_height_pos=0;
			last_height_neg=0;
			last_height=0;
			i=0;
			for(list=graph->plots;list != NULL; list = g_slist_next(list)) {
				plot = list->data;
				if(rlib_execute_as_string(r, plot->axis_code, axis, MAXSTRLEN)) {
					if(strcmp(axis, "x") == 0) {
						if(minor_tick[row_count] == FALSE) {
							if(rlib_execute_as_string(r, plot->field_code, x_axis_label, MAXSTRLEN)) {
								OUTPUT(r)->graph_label_x(r, row_count, x_axis_label);
							}																
						}
					} else if(strcmp(axis, "y") == 0) {
						disabled = FALSE;
						if(rlib_execute_as_boolean(r, plot->disabled_code, &tmp_disabled))
							disabled = tmp_disabled;
						if(!disabled && rlib_execute_as_float(r, plot->field_code, &y_value)) {
							side = RLIB_SIDE_LEFT;
							if(rlib_execute_as_string(r, plot->side_code, side_str, MAXSTRLEN)) {
								if(strcmp(side_str, "right") == 0) {
									side = RLIB_SIDE_RIGHT;
								}
							}

							executed_label_pcode = rlib_execute_as_string(r, plot->label_code, legend_label, MAXSTRLEN);

							if(!rlib_execute_as_string(r, plot->color_code, color_str, MAXSTRLEN)) {
								plot_color = color[data_plot_count];
							} else {
								rlib_parsecolor(&plot_color, color_str);
							}
							if(is_percent_graph(graph_type) || is_pie_graph(graph_type)) {
								y_value = fabs(y_value);
								if(is_pie_graph(graph_type))
									y_max[RLIB_SIDE_LEFT] = col_sum;
								else
									y_max[RLIB_SIDE_LEFT] = row_sum[row_count];
								y_min[RLIB_SIDE_LEFT] = 0;
							}

							value = 0;
						
							if(data_type[side] == POSITIVE_AND_NEGATIVE) {
								if(value >= 0)
									value = (y_value / y_max[side]) * (fabs(y_max[side])/(fabs(y_min[side])+fabs(y_max[side])));
								else
									value = (y_value / y_min[side]) * (fabs(y_min[side])/(fabs(y_min[side])+fabs(y_max[side])));
							} else if(data_type[side] == NEGATIVE) {
								value = -((y_value - y_origin[side]) / (y_min[side] - y_origin[side]));
							} else {
								value = (y_value - y_origin[side]) / (y_max[side] - y_origin[side]);
							}
							plot_count = data_plot_count;
							if(is_percent_graph(graph_type) || is_stacked_graph(graph_type) || is_pie_graph(graph_type)) { 
								plot_count = 0;
								if( value >= 0 ) 
									last_height = last_height_pos;
								else
									last_height = last_height_neg;
							}
							
							 
							if(is_row_graph(graph_type)) {
								OUTPUT(r)->graph_plot_bar(r, side, row_count, plot_count, value, &plot_color,last_height, divide_iterations, y_value, legend_label);
							} else if(is_line_graph(graph_type)) {
								OUTPUT(r)->graph_plot_line(r, side, row_count, last_row_values[i], last_row_height[i], value, last_height, &plot_color, y_value, legend_label, row_count);
							} else if(is_pie_graph(graph_type) && !isnan(value)) {
								gboolean offset = graph_type == RLIB_GRAPH_TYPE_PIE_OFFSET;
								OUTPUT(r)->graph_plot_pie(r, running_col_sum, value+running_col_sum, offset, &color[row_count + data_plot_count], y_value, legend_label);
								running_col_sum += value;
								if (executed_label_pcode)
									OUTPUT(r)->graph_draw_legend_label(r, row_count+data_plot_count, legend_label, &color[row_count + data_plot_count], is_line_graph(graph_type));
							}
								
							if(is_percent_graph(graph_type) || is_stacked_graph(graph_type) || is_pie_graph(graph_type)) { 
								if(value >= 0 ) 
									last_height_pos += value;
								else
									last_height_neg += value;
							}
							
							if(is_pie_graph(graph_type) && i > 0) {
								data_plot_count++;
								continue;
							}
						
							last_row_values[i] = value;
							last_row_height[i] = last_height;
							i++;
						}
						data_plot_count++;
					} 			
				}
			} 
			row_count++;

			if(rlib_navigate_next(r, r->current_result) == FALSE)
				break;
		}
	}
	report->position_top[0] += graph_height / RLIB_PDF_DPI;
	g_free(row_sum);
	g_free(last_row_values);
	g_free(last_row_height);
	OUTPUT(r)->end_graph(r, part, report);
	return graph_height / RLIB_PDF_DPI;
}
