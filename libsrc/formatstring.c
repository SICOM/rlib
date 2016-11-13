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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <locale.h>

#ifdef RLIB_HAVE_MONETARY_H 
#include <monetary.h>
#endif

#include "rlib-internal.h"
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

gboolean rlib_format_number(rlib *r, gchar **dest, const gchar *fmt, mpfr_t value) {
	//gchar *d = strstr(fmt, "d");
	/*
	 * TODO TODO TODO
	 *
	 * This function should really look like:
	 * - dissect the format string into pieces
	 * - replace the format specifier character with "Rf" using the same precision
	 * - use mpfr_asprintf() ; *dest = g_strdup() ; mpfr_free_str()
	 */
	double d = mpfr_get_d(value, MPFR_RNDN);
	*dest = g_strdup_printf(fmt, d);
	change_radix_character(r, *dest);
	return TRUE;
}

/*
 * Formats numbers in money format using locale parameters and moneyformat codes
 */
gboolean rlib_format_money(rlib *r, gchar **dest, const gchar *moneyformat, mpfr_t value) {
	gboolean result;
	ssize_t len;

	*dest = g_malloc(MAXSTRLEN);

	len = rlib_mpfr_strfmon(*dest, MAXSTRLEN - 1, moneyformat, value);
	if (len <= 0)
		result = FALSE;
	else
		change_radix_character(r, *dest);
	return result;
}

static gboolean string_sprintf(rlib *r, gchar **dest, gchar *fmtstr, struct rlib_value *rval) {
	gchar *value = RLIB_VALUE_GET_AS_STRING(r, rval);
	*dest = g_strdup_printf(fmtstr, value);
	return TRUE;
}

#define MAX_NUMBER 128

gint64 rlib_number_sprintf(rlib *r, gchar **woot_dest, gchar *fmtstr, const struct rlib_value *rval, gint64 special_format, gchar *infix, gint line_number) {
	gint dec = 0;
	gint left_padzero = 0;
	gint left_pad = 0;
	gint right_padzero = 1;
	gint right_pad = 0;
	gint where = 0;
	gint commatize = 0;
	gchar *c;
	gchar *radixchar = nl_langinfo(RADIXCHAR);
	GString *dest = g_string_sized_new(MAX_NUMBER);
	gint slen;

	for (c = fmtstr; *c && (*c != 'd'); c++) {
		if (*c == '$') {
			commatize = 1;
		}
		if (*c == '%') {
			where = 0;
		} else if (*c == '.') {
			dec = 1;
			where = 1;
		} else if (isdigit(*c)) {
			if (where == 0) {
				if (left_pad == 0 && (*c - '0') == 0)
					left_padzero = 1;
				left_pad *= 10;
				left_pad += (*c - '0');
			} else {
				right_pad *= 10;
				right_pad += (*c - '0');
			}
		}
	}
	if (!left_pad)
		left_padzero = 1;
	if (!right_pad)
		right_padzero = 1;
	if (rval != NULL) {
		mpfr_t tmpvalue;
		char *tmp;
		gchar fleft[MAX_FORMAT_STRING];
		gchar fright[MAX_FORMAT_STRING];
		gchar left_holding[MAX_NUMBER];
		gchar right_holding[MAX_NUMBER];
		gint64 ptr = 0;
		glong left = 0, right = 0;

		mpfr_init2(tmpvalue, r->numeric_precision_bits);

		if (left_pad > MAX_FORMAT_STRING) {
			r_error(r, "LINE %d FORMATTING ERROR: %s\n", line_number, infix);
			r_error(r, "FORMATTING ERROR:  LEFT PAD IS WAY TO BIG! (%d)\n", left_pad);
			left_pad = MAX_FORMAT_STRING;
		}
		if (right_pad > MAX_FORMAT_STRING) {
			r_error(r, "LINE %d FORMATTING ERROR: %s\n", line_number, infix);
			r_error(r, "FORMATTING ERROR:  LEFT PAD IS WAY TO BIG! (%d)\n", right_pad);
			right_pad = MAX_FORMAT_STRING;
		}

		mpfr_trunc(tmpvalue, rval->mpfr_value);
		mpfr_asprintf(&tmp, "%.0Rf", tmpvalue);
		//XXX left = mpfr_get_si(rval->mpfr_value, MPFR_RNDN);
		if (special_format)
			left = llabs(left);
		fleft[ptr++] = '%';
		if (left_padzero)
			fleft[ptr++] = '0';
		if (left_pad)
			if (commatize) {
				sprintf(fleft +ptr, "%d'" PRId64, left_pad);
			} else
				sprintf(fleft +ptr, "%d" PRId64, left_pad);
		else {
			char	*int64fmt = PRId64;
			int	i;

			if (commatize)
				fleft[ptr++] = '\'';
			for (i = 0; int64fmt[i]; i++)
				fleft[ptr++] = int64fmt[i];
			fleft[ptr++] = '\0';
		}
		if (left_pad == 0 && left == 0) {
			gint64 spot = 0;
			if (mpfr_sgn(rval->mpfr_value) < 0)
				left_holding[spot++] = '-';
			left_holding[spot++] = '0';
			left_holding[spot++] = 0;			
		} else {
			sprintf(left_holding, fleft, left);
		}
		mpfr_free_str(tmp);
		dest = g_string_append(dest, left_holding);
		if (dec) {
			ptr = 0;
			mpfr_frac(tmpvalue, rval->mpfr_value, MPFR_RNDN);
			mpfr_asprintf(&tmp, "%Rf", tmpvalue, MPFR_RNDN);
			//XXX right = llabs(RLIB_VALUE_GET_AS_NUMBER(rval)) % RLIB_DECIMAL_PRECISION;
			fright[ptr++] = '%';
			if (right_padzero)
				fright[ptr++] = '0';
			if (right_pad)
				sprintf(&fright[ptr], "%d" PRId64, right_pad);
			else {
				char	*int64fmt = PRId64;
				int	i;

				for (i = 0; int64fmt[i]; i++)
					fright[ptr++] = int64fmt[i];
				fright[ptr++] = '\0';
			}
			right /= tentothe(RLIB_FXP_PRECISION - right_pad);
			sprintf(right_holding, fright, right);
			dest = g_string_append(dest, radixchar);
			dest = g_string_append(dest, right_holding);
			mpfr_free_str(tmp);
		}
		mpfr_clear(tmpvalue);
	}
	*woot_dest = dest->str;
	change_radix_character(r, *woot_dest);
	slen = dest->len;
	g_string_free(dest, FALSE);
	return slen;
}

static gint64 rlib_format_string_default(rlib *r, struct rlib_value *rval, gchar **dest) {
	if (RLIB_VALUE_IS_NUMBER(r, rval)) {
		char *tmp;
		mpfr_asprintf(&tmp, "%.0Rf", rval->mpfr_value);
		*dest = g_strdup(tmp);
		mpfr_free_str(tmp);
	} else if (RLIB_VALUE_IS_STRING(r, rval)) {
		if (RLIB_VALUE_GET_AS_STRING(r, rval) == NULL)
			*dest = NULL;
		else
			*dest = g_strdup(RLIB_VALUE_GET_AS_STRING(r, rval));
	} else if (RLIB_VALUE_IS_DATE(r, rval))  {
		struct rlib_datetime *dt = RLIB_VALUE_GET_AS_DATE(r, rval);
		rlib_datetime_format(r, dest, dt, "%m/%d/%Y");
	} else {
		*dest = g_strdup_printf("!ERR_F");
		return FALSE;
	}
	return TRUE;
}

gint64 rlib_format_string(rlib *r, gchar **dest, struct rlib_report_field *rf, struct rlib_value *rval) {
	gchar before[MAXSTRLEN], after[MAXSTRLEN];
	gint64 before_len = 0, after_len = 0;
	gboolean formatted_it = FALSE;	

	if (r->special_locale != NULL)
		setlocale(LC_ALL, r->special_locale);
	if (rf->xml_format.xml == NULL) {
		rlib_format_string_default(r, rval, dest);
	} else {
		gchar *formatstring;
		struct rlib_value rval_fmtstr2, *rval_fmtstr;
		rlib_value_init(r, &rval_fmtstr2);
		rval_fmtstr = rlib_execute_pcode(r, &rval_fmtstr2, rf->format_code, rval);
		if (!RLIB_VALUE_IS_STRING(r, rval_fmtstr)) {
			*dest = g_strdup_printf("!ERR_F_F");
			rlib_value_free(r, rval_fmtstr);
			if (r->special_locale != NULL) 
				setlocale(LC_ALL, r->current_locale);
			return FALSE;
		} else {
			formatstring = RLIB_VALUE_GET_AS_STRING(r, rval_fmtstr);
			if (formatstring == NULL) {
				rlib_format_string_default(r, rval, dest);
			} else {
				if (*formatstring == '!') {
					gboolean result;
					gchar *tfmt = formatstring + 1;
					gboolean goodfmt = TRUE;
					switch (*tfmt) {
					case '$': /* Format as money */
						if (RLIB_VALUE_IS_NUMBER(r, rval)) {
							result = rlib_format_money(r, dest, tfmt + 1, rval->mpfr_value);
							if (r->special_locale != NULL)
								setlocale(LC_ALL, r->current_locale);
							rlib_value_free(r, rval_fmtstr);
							return result;
						}
						++formatstring;
						break;
					case '#': /* Format as number */
						if (RLIB_VALUE_IS_NUMBER(r, rval)) {
							result = rlib_format_number(r, dest, tfmt + 1, rval->mpfr_value);
							if (r->special_locale != NULL)
								setlocale(LC_ALL, r->current_locale);
							rlib_value_free(r, rval_fmtstr);
							return result;
						}
						++formatstring;
						break;
					case '@': /* Format as time/date */
						if (RLIB_VALUE_IS_DATE(r, rval)) {
							struct rlib_datetime *dt = RLIB_VALUE_GET_AS_DATE(r, rval);
							rlib_datetime_format(r, dest, dt, tfmt + 1);
							formatted_it = TRUE;
						}
						break;
					default:
						goodfmt = FALSE;
						break;
					}
					if (goodfmt) {
						if (r->special_locale != NULL)
							setlocale(LC_ALL, r->current_locale);
						rlib_value_free(r, rval_fmtstr);
						return TRUE;
					}
				}
				if (RLIB_VALUE_IS_DATE(r, rval)) {
					rlib_datetime_format(r, dest, RLIB_VALUE_GET_AS_DATE(r, rval), formatstring);
				} else {	
					gint64 i = 0, /* j = 0, pos = 0, */ fpos = 0;
					gchar fmtstr[MAX_FORMAT_STRING];
					gint64 special_format = 0;
					gchar *idx;
					gint64 len_formatstring;
					idx = strchr(formatstring, ':');
					fmtstr[0] = 0;
					if (idx != NULL && RLIB_VALUE_IS_NUMBER(r, rval)) {
						formatstring = g_strdup(formatstring);
						idx = strchr(formatstring, ':');
						special_format = 1;
						if (mpfr_sgn(rval->mpfr_value) >= 0)
							idx[0] = '\0';
						else
							formatstring = idx + 1;
					}

					len_formatstring = strlen(formatstring);

					for (i = 0 ; i < len_formatstring; i++) {
						if (formatstring[i] == '%' && ((i + 1) < len_formatstring && formatstring[i + 1] != '%')) {
							int tchar;
							while(formatstring[i] != 's' && formatstring[i] != 'd' && i <=len_formatstring) {
								fmtstr[fpos++] = formatstring[i++];
							}
							fmtstr[fpos++] = formatstring[i];
							fmtstr[fpos] = '\0';
							tchar = fmtstr[fpos - 1];
							if ((tchar == 'd') || (tchar == 'i') || (tchar == 'n')) {
								if (RLIB_VALUE_IS_NUMBER(r, rval)) {
									rlib_number_sprintf(r, dest, fmtstr, rval, special_format, (gchar *)rf->xml_format.xml, rf->xml_format.line);
									formatted_it = TRUE;
								} else {
									*dest = g_strdup_printf("!ERR_F_D");
									rlib_value_free(r, rval_fmtstr);
									if (r->special_locale != NULL)
										setlocale(LC_ALL, r->current_locale);
									return FALSE;
								}
							} else if (tchar == 's') {
								if (RLIB_VALUE_IS_STRING(r, rval)) {
									string_sprintf(r, dest, fmtstr, rval);
									formatted_it = TRUE;
								} else {
									*dest = g_strdup_printf("!ERR_F_S");
									rlib_value_free(r, rval_fmtstr);
									if (r->special_locale != NULL)
										setlocale(LC_ALL, r->current_locale);
									return FALSE;
								}
							}
						} else {
							if (formatted_it == FALSE) {
								before[before_len++] = formatstring[i];
							} else {
								after[after_len++] = formatstring[i];
							}
							if (formatstring[i] == '%')
								i++;
						}
					}
				}
			}
			rlib_value_free(r, rval_fmtstr);
		}
	}
	
	if (before_len > 0 || after_len > 0) {
		gchar *new_str;
		before[before_len] = 0;
		after[after_len] = 0;
		new_str = g_strconcat(before, *dest, after, NULL);
		g_free(*dest);
		*dest = new_str;
	}
	if (r->special_locale != NULL)
		setlocale(LC_ALL, r->current_locale);
	return TRUE;
}

gchar *rlib_align_text(rlib *r, gchar **my_rtn, gchar *src, gint64 align, gint64 width) {
	gint64 len = 0, size = 0, lastidx = 0;
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
		rtn = *my_rtn = g_strdup(src ? src : "");
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
			gint64 x = lastidx - size;
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
				gint64 x = (width - len)/2;
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

GSList *format_split_string(gchar *data, gint64 width, gchar new_line, gchar space, gint64 *line_count) {
	gint64 slen;
	gint64 spot = 0;
	gint64 end = 0;
	gint64 i;
	gint64 line_spot = 0;
	gboolean at_the_end = FALSE;
	GSList *list = NULL;
	
	if (data == NULL) {
		gchar *this_line = g_malloc(width);
		list = g_slist_append(list, this_line);
		memset(this_line, 0, width);
		*line_count = 1;
		return list;
	}
	
	slen = strlen(data);
	while(1) {
		gchar *this_line = g_malloc(width);
		gint64 space_count = 0;
		memset(this_line, 0, width);
		end = spot + width-1;
		if (end > slen) {
			end = slen;
			at_the_end = TRUE;
		}
	
		line_spot = 0;
		for (i = spot; i < end; i++) {
			spot++;
			if (data[i] == new_line)
				break;
			this_line[line_spot++] = data[i];
			if (data[i] == space)
				space_count++;
		}
		
		if (spot < slen)
			at_the_end = FALSE;
			
		if (!at_the_end) {
			if (space_count == 0) {
				/* We do nothing here as we have text that just won't fit.. we split it up in "width" chunks */
			} else {
				while (data[i] != space && data[i] != new_line) {
					this_line[--line_spot] = 0;
					i--;
					spot--;
				}
			}
			if (data[spot] == space)
				spot++;
		}

		list = g_slist_append(list, this_line);
		*line_count = *line_count + 1;

		if (at_the_end == TRUE)
			break;
	}
	
	return list;
}
