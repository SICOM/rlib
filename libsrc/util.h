/*
 *  Copyright (C) 2003-2006 SICOM Systems, INC.
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
 * $Id$s
 *
 * Definitions and prototypes for utility functions
 */
#ifndef RLIBUTIL_H
#define RLIBUTIL_H

#include <glib.h>

struct rlib_rgb {
	gfloat r;
	gfloat g;
	gfloat b;
};


gchar *strlwrexceptquoted (gchar *s);
gchar *rmwhitespacesexceptquoted(gchar *s);
/* coming soon: void r_fatal(const gchar *fmt, ...); */
gint rutil_enableSignalHandler(gint trueorfalse);
gint64 tentothe(gint n);
gchar hextochar(gchar c);
const gchar *colornames(const gchar *str);
void rlib_parsecolor(struct rlib_rgb *color, const gchar *strx);
struct rlib_datetime * stod(struct rlib_datetime *tm_date, gchar *str);
void bumpday(gint *year, gint *month, gint *day);
void bumpdaybackwords(gint *year, gint *month, gint *day);
gchar *strproper (gchar *s);
gint daysinmonth(gint year, gint month);
void init_signals(void);
void make_more_space_if_necessary(gchar **str, gint *size, gint *total_size, gint len);
gchar *str2hex(const gchar *str);
gint64 rlib_safe_atoll(char *str);

#if DISABLE_UTF8

#define r_strlen(s) (strlen(s))
#define r_strchr(s, len, chr) (strchr(s, chr))
#define r_nextchr(s) (s+1)
#define r_getchr(s) (*s)
#define r_strcmp(s1,s2) (strcmp(s1, s2))
#define r_strupr(s) (g_ascii_strup(s, -1))
#define r_strlwr(s) (g_ascii_strdown(s, -1))
#define r_ptrfromindex(s, idx) (s+idx)

#else

#define r_strlen(s) (g_utf8_strlen(s, -1))
#define r_strchr(t, len, chr) (g_utf8_strchr(t, len, chr))
#define r_nextchr(t) (g_utf8_next_char(t))
#define r_getchr(t) (g_utf8_get_char(t))
#define r_strcmp(a,b) (g_utf8_collate(a, b))
#define r_strupr(a) (g_utf8_strup(a, -1))
#define r_strlwr(a) (g_utf8_strdown(a, -1))
#define r_ptrfromindex(s, idx) (g_utf8_offset_to_pointer(s, idx))

#endif

void make_all_locales_utf8(void);
gchar *make_utf8_locale(const gchar *encoding);

#endif

