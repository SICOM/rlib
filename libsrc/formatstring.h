/*
 *  Copyright (C) 2003-2017 SICOM Systems, INC.
 *
 *  Authors: Bob Doan <bdoan@sicompos.com>
 *           Zoltán Böszörményi <zboszormenyi@sicom.com>
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

#ifndef _FORMATSTRING_H_
#define _FORMATSTRING_H_

#include "rlib.h"

#define RLIB_FORMATSTR_NONE		0
#define RLIB_FORMATSTR_LITERAL	1
#define RLIB_FORMATSTR_NUMBER	2
#define RLIB_FORMATSTR_MONEY	3
#define RLIB_FORMATSTR_DATE		4
#define RLIB_FORMATSTR_STRING	5

gint rlib_number_sprintf(rlib *r, gchar **dest, gchar *fmtstr, const struct rlib_value *rval, gint special_format, gchar *infix, gint line_number);
gint rlib_format_string(rlib *r, gchar **buf,  struct rlib_report_field *rf, struct rlib_value *rval);
gint rlib_format_money(rlib *r, gchar **dest, const gchar *moneyformat, gint64 x);
gint rlib_format_number(rlib *r, gchar **dest, const gchar *moneyformat, gint64 x);
gchar *rlib_align_text(rlib *r, char **rtn, gchar *src, gint align, gint width);
GSList * rlib_format_split_string(rlib *r, gchar *data, gint width, gint max_lines, gchar new_line, gchar space, gint *line_count);
GString *get_next_format_string(rlib *r, const gchar *fmt, gint expected_type, gint *out_type, gint *advance, gboolean *error);

#endif /* _FORMATSTRING_H_ */
