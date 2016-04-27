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
 
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <locale.h>

#include "config.h"

#ifdef RLIB_HAVE_MONETARY_H 
#include <monetary.h>
#endif

#include "rlib.h"
#include "pcode.h"
#include "rlib_langinfo.h"


#define MAX_FORMAT_STRING 20

void change_radix_character(rlib *r, char *s) {
	if (r->radix_character == '.')
		return;
	while (*s != '\0') {
		if (*s == '.') {
			*s = r->radix_character;
		} else if (*s == r->radix_character) {
			*s = '.';
		}
		s++;
	}	
}

/*
* Formats numbers in money format using locale parameters and moneyformat codes
*/
gint rlib_format_money(rlib *r, gchar **dest,const gchar *moneyformat, gint64 x) { 
	gint result;
#ifdef HAVE_STRFMON
	gdouble d;
#endif	
	
	*dest = g_malloc(MAXSTRLEN);
#ifndef HAVE_STRFMON
	result = 0;
	(*dest)[0] = 0;
#else
	d = ((gdouble) x) / RLIB_DECIMAL_PRECISION;
	result = strfmon(*dest, MAXSTRLEN - 1, moneyformat, d);
#endif
	change_radix_character(r, *dest);
	return (result >= 0)? strlen(*dest) : 0;
}

gint rlib_format_number(rlib *r, gchar **dest, const gchar *fmt, gint64 x) {
	double d = (((double) x) / (double)RLIB_DECIMAL_PRECISION);
	*dest = g_strdup_printf(fmt, d);
	change_radix_character(r, *dest);
	return TRUE;
}

static gint rlib_string_sprintf(rlib *r, gchar **dest, gchar *fmtstr, struct rlib_value *rval) {
	gchar *value = RLIB_VALUE_GET_AS_STRING(rval);
	*dest = g_strdup_printf(fmtstr, value);
	return TRUE;
}

#define MAX_NUMBER 128

gint rlib_number_sprintf(rlib *r, gchar **woot_dest, gchar *fmtstr, const struct rlib_value *rval, gint special_format, gchar *infix, gint line_number) {
	gint dec=0;
	gint left_padzero=0;
	gint left_pad=0;
	gint right_padzero=1;
	gint right_pad=0;
	gint where=0;
	gint commatize=0;
	gchar *c;
	gchar *radixchar = nl_langinfo(RADIXCHAR);
	GString *dest = g_string_sized_new(MAX_NUMBER);
	gint slen;
	
	for(c=fmtstr;*c && (*c != 'd');c++) {
		if(*c=='$') {
			commatize=1;
		}
		if(*c=='%') {
			where=0;
		} else if(*c=='.') {
			dec=1;
			where=1;
		} else if(isdigit(*c)) {		
			if(where==0) {
				if(left_pad == 0 && (*c-'0') == 0)
					left_padzero = 1;
				left_pad *= 10;
				left_pad += (*c-'0');
			} else {
				right_pad *= 10;
				right_pad += (*c-'0');
			}
		}	
	}
	if (!left_pad)
		left_padzero = 1;
	if (!right_pad)
		right_padzero = 1;
	if(rval != NULL) {
		gchar fleft[MAX_FORMAT_STRING];
		gchar fright[MAX_FORMAT_STRING];
		gchar left_holding[MAX_NUMBER];
		gchar right_holding[MAX_NUMBER];
		gint ptr=0;
		gint64 left=0, right=0;
		
		if(left_pad > MAX_FORMAT_STRING) {
			r_error(r, "LINE %d FORMATTING ERROR: %s\n", line_number, infix);
			r_error(r, "FORMATTING ERROR:  LEFT PAD IS WAY TO BIG! (%d)\n", left_pad);
			left_pad = MAX_FORMAT_STRING;
		} 
		if(right_pad > MAX_FORMAT_STRING) {
			r_error(r, "LINE %d FORMATTING ERROR: %s\n", line_number, infix);
			r_error(r, "FORMATTING ERROR:  LEFT PAD IS WAY TO BIG! (%d)\n", right_pad);			
			right_pad = MAX_FORMAT_STRING;
		} 
		
		left = RLIB_VALUE_GET_AS_NUMBER(rval) / RLIB_DECIMAL_PRECISION;
		if(special_format)
			left = llabs(left);
		fleft[ptr++]='%';
		if(left_padzero)
			fleft[ptr++]='0';
		if(left_pad)
			if(commatize) {
				sprintf(fleft +ptr, "%d'" PRId64, left_pad);
			} else
				sprintf(fleft +ptr, "%d" PRId64, left_pad);
		else {
			char	*int64fmt = PRId64;
			int	i;

			if(commatize)
				fleft[ptr++] = '\'';
			for (i = 0; int64fmt[i]; i++)
				fleft[ptr++] = int64fmt[i];
			fleft[ptr++] = '\0';
		}
		if(left_pad == 0 && left == 0) {
			gint spot=0;
			if(RLIB_VALUE_GET_AS_NUMBER(rval) < 0)
				left_holding[spot++] = '-';
			left_holding[spot++] = '0';
			left_holding[spot++] = 0;			
		} else {
			sprintf(left_holding, fleft, left);
		}
		dest = g_string_append(dest, left_holding);
		if(dec) {
			ptr=0;
			right = llabs(RLIB_VALUE_GET_AS_NUMBER(rval)) % RLIB_DECIMAL_PRECISION;
			fright[ptr++]='%';
			if(right_padzero)
				fright[ptr++]='0';
			if(right_pad)
				sprintf(&fright[ptr], "%d" PRId64, right_pad);
			else {
				char	*int64fmt = PRId64;
				int	i;

				for (i = 0; int64fmt[i]; i++)
					fright[ptr++] = int64fmt[i];
				fright[ptr++] = '\0';
			}
			right /= tentothe(RLIB_FXP_PRECISION-right_pad);
			sprintf(right_holding, fright, right);
			dest = g_string_append(dest, radixchar);
			dest = g_string_append(dest, right_holding);
		}
	}
	*woot_dest = dest->str;
	change_radix_character(r, *woot_dest);
	slen = dest->len;
	g_string_free(dest, FALSE);
	return slen;
}

static gint rlib_format_string_default(rlib *r, struct rlib_report_field *rf, struct rlib_value *rval, gchar **dest) {
	if(RLIB_VALUE_IS_NUMBER(rval)) {
		*dest = g_strdup_printf("%" PRId64, RLIB_FXP_TO_NORMAL_LONG_LONG(RLIB_VALUE_GET_AS_NUMBER(rval)));
	} else if(RLIB_VALUE_IS_STRING(rval)) {
		if(RLIB_VALUE_GET_AS_STRING(rval) == NULL)
			*dest = NULL;
		else
			*dest = g_strdup(RLIB_VALUE_GET_AS_STRING(rval));
	} else if(RLIB_VALUE_IS_DATE(rval))  {
		struct rlib_datetime *dt = &RLIB_VALUE_GET_AS_DATE(rval);
		rlib_datetime_format(r, dest, dt, "%m/%d/%Y");
	} else {
		*dest = g_strdup_printf("!ERR_F");
		return FALSE;
	}
	return TRUE;
}

gint rlib_format_string(rlib *r, gchar **dest, struct rlib_report_field *rf, struct rlib_value *rval) {
	gchar before[MAXSTRLEN], after[MAXSTRLEN];
	gint before_len = 0, after_len = 0;
	gboolean formatted_it = FALSE;	
	
	if(r->special_locale != NULL) 
		setlocale(LC_ALL, r->special_locale);
	if(rf->xml_format.xml == NULL) {
		rlib_format_string_default(r, rf, rval, dest);
	} else {
		gchar *formatstring;
		struct rlib_value rval_fmtstr2, *rval_fmtstr;
		rval_fmtstr = rlib_execute_pcode(r, &rval_fmtstr2, rf->format_code, rval);
		if(!RLIB_VALUE_IS_STRING(rval_fmtstr)) {
			*dest = g_strdup_printf("!ERR_F_F");
			rlib_value_free(rval_fmtstr);
			if(r->special_locale != NULL) 
				setlocale(LC_ALL, r->current_locale);
			return FALSE;
		} else {
			formatstring = RLIB_VALUE_GET_AS_STRING(rval_fmtstr);
			if(formatstring == NULL) {
				rlib_format_string_default(r, rf, rval, dest);
			} else {
				if (*formatstring == '!') {
					gboolean result;
					gchar *tfmt = formatstring + 1;
					gboolean goodfmt = TRUE;
					switch (*tfmt) {
					case '$': /* Format as money */
						if (RLIB_VALUE_IS_NUMBER(rval)) {
							result = rlib_format_money(r, dest, tfmt + 1, RLIB_VALUE_GET_AS_NUMBER(rval));
							if(r->special_locale != NULL) 
								setlocale(LC_ALL, r->current_locale);
							return result;
						}
						++formatstring;
						break;
					case '#': /* Format as number */
						if (RLIB_VALUE_IS_NUMBER(rval)) {
							result = rlib_format_number(r, dest, tfmt + 1, RLIB_VALUE_GET_AS_NUMBER(rval));
							if(r->special_locale != NULL) 
								setlocale(LC_ALL, r->current_locale);
							return result;
						}
						++formatstring;
						break;
					case '@': /* Format as time/date */
						if(RLIB_VALUE_IS_DATE(rval)) {
							struct rlib_datetime *dt = &RLIB_VALUE_GET_AS_DATE(rval);
							rlib_datetime_format(r, dest, dt, tfmt + 1);
							formatted_it = TRUE;
						}
						break;
					default:
						goodfmt = FALSE;
						break;
					}
					if (goodfmt) {
						if(r->special_locale != NULL) 
							setlocale(LC_ALL, r->current_locale);
						return TRUE;
					}
				}
				if(RLIB_VALUE_IS_DATE(rval)) {
					rlib_datetime_format(r, dest, &RLIB_VALUE_GET_AS_DATE(rval), formatstring);
				} else {	
					gint i=0,/*j=0,pos=0,*/fpos=0;
					gchar fmtstr[MAX_FORMAT_STRING];
					gint special_format=0;
					gchar *idx;
					gint len_formatstring;
					idx = strchr(formatstring, ':');
					fmtstr[0] = 0;
					if(idx != NULL && RLIB_VALUE_IS_NUMBER(rval)) {
						formatstring = g_strdup(formatstring);
						idx = strchr(formatstring, ':');
						special_format=1;
						if(RLIB_VALUE_GET_AS_NUMBER(rval) >= 0)
							idx[0] = '\0';
						else
							formatstring = idx+1;				
					}

					len_formatstring = strlen(formatstring);

					for(i=0;i<len_formatstring;i++) {
						if(formatstring[i] == '%' && ((i+1) < len_formatstring && formatstring[i+1] != '%')) {
							int tchar;
							while(formatstring[i] != 's' && formatstring[i] != 'd' && i <=len_formatstring) {
								fmtstr[fpos++] = formatstring[i++];
							}
							fmtstr[fpos++] = formatstring[i];
							fmtstr[fpos] = '\0';
							tchar = fmtstr[fpos - 1];
							if ((tchar == 'd') || (tchar == 'i') || (tchar == 'n')) {
								if(RLIB_VALUE_IS_NUMBER(rval)) {
									rlib_number_sprintf(r, dest, fmtstr, rval, special_format, (gchar *)rf->xml_format.xml, rf->xml_format.line);
									formatted_it = TRUE;
								} else {
									*dest = g_strdup_printf("!ERR_F_D");
									rlib_value_free(rval_fmtstr);
									if(r->special_locale != NULL) 
										setlocale(LC_ALL, r->current_locale);
									return FALSE;
								}
							} else if (tchar == 's') {
								if(RLIB_VALUE_IS_STRING(rval)) {
									rlib_string_sprintf(r, dest, fmtstr, rval);
									formatted_it = TRUE;
								} else {
									*dest = g_strdup_printf("!ERR_F_S");
									rlib_value_free(rval_fmtstr);
									if(r->special_locale != NULL) 
										setlocale(LC_ALL, r->current_locale);
									return FALSE;
								}
							}
						} else {
							if(formatted_it == FALSE) {
								before[before_len++] = formatstring[i];
							} else {
								after[after_len++] = formatstring[i];
							}
							if(formatstring[i] == '%')
								i++;
						}
					}
				}

			}
			rlib_value_free(rval_fmtstr);
		}
	}
	
	if(before_len > 0 || after_len > 0) {
		gchar *new_str;
		before[before_len] = 0;
		after[after_len] = 0;
		new_str = g_strconcat(before, *dest, after, NULL);
		g_free(*dest);
		*dest = new_str;
	}
	if(r->special_locale != NULL) 
		setlocale(LC_ALL, r->current_locale);
	return TRUE;
}

gchar *rlib_align_text(rlib *r, gchar **my_rtn, gchar *src, gint align, gint width) {
	gint len = 0, size = 0, lastidx = 0;
	gchar *rtn;

	if (width == 0) {
		rtn = *my_rtn = g_strdup("");
		return rtn;
	}

	if (src != NULL) {
		len = r_strlen(src);
		size = strlen(src);
		lastidx = width + size - len;
	}

	if (len > width) {
		rtn = *my_rtn = g_strdup(src);
		return rtn;
	} else {
		rtn = *my_rtn  = g_malloc(lastidx + 1);
		if (width >= 1) {
			memset(rtn, ' ', lastidx);
			rtn[lastidx] = 0;
		}  
		if (src != NULL)
			memcpy(rtn, src, size);
	}

	if (!OUTPUT(r)->do_align)
		return rtn;

	if (align == RLIB_ALIGN_LEFT || width == -1) {
		/* intentionally do nothing here */
	} else {
		if (align == RLIB_ALIGN_RIGHT) {        
			gint x = lastidx - size;
			if (x > 0) {
				memset(rtn, ' ', x);
				if (src == NULL)
					rtn[x] = 0;
				else
					g_strlcpy(rtn+x, src, lastidx);
			}
		}
		if (align == RLIB_ALIGN_CENTER) {
			if (!(width > 0 && len > width)) {
				gint x = (width - len)/2;
				if (x > 0) {
					memset(rtn, ' ', x);
					if (src == NULL)
						rtn[x] = 0;
					else
						memcpy(rtn+x, src, size);
				}
			}
		}
	}
	return rtn;
}

GSList * rlib_format_split_string(rlib *r, gchar *data, gint width, gint max_lines, gchar new_line, gchar space, gint *line_count) {
	gint slen;
	gint spot = 0;
	gint end = 0;
	gint i;
	gint line_spot = 0;
	gboolean at_the_end = FALSE;
	GSList *list = NULL;
	
	if(data == NULL) {
		gchar *this_line = g_malloc(width);
		list = g_slist_append(list, this_line);
		memset(this_line, 0, width);
		*line_count = 1;
		return list;
	}
	
	slen = strlen(data);
	while(1) {
		gchar *this_line = g_malloc(width);
		gint space_count = 0;
		memset(this_line, 0, width);
		end = spot + width-1;
		if(end > slen) {
			end = slen;
			at_the_end = TRUE;
		}
	
		line_spot = 0;
		for(i=spot;i<end;i++) {
			spot++;
			if(data[i] == new_line) 
				break;
			this_line[line_spot++] = data[i];
			if(data[i] == space)
				space_count++;
		}
		
		if(spot < slen)
			at_the_end = FALSE;
			
		if(!at_the_end) {
			if(space_count == 0) {
				/* We do nothing here as we have text that just won't fit.. we split it up in "width" chunks */
			} else {
				while(data[i] != space && data[i] != new_line) {
					this_line[--line_spot] = 0;
					i--;
					spot--;
				}
			}
			if(data[spot] == space)
				spot++;
		}

		list = g_slist_append(list, this_line);
		*line_count = *line_count + 1;

		if(at_the_end == TRUE) 
			break;
	}
	
	return list;
}
