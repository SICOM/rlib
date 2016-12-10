/*
 *  Copyright (C) 2003-2016 SICOM Systems, INC.
 *
 *  Authors: Chet Heilman <cheilman@sicompos.com>
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
 * This module implements a characterencoder that converts a character
 * either to or from UTF8 which is the internal language of RLIB.
 * it is implemented as a simple 'class'.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <errno.h>

#include "rlib-internal.h"
#include "rlib_input.h"
#include "rlib_langinfo.h"

GIConv rlib_charencoder_new(const gchar *to_codeset UNUSED, const gchar *from_codeset UNUSED) {
	if (strcasecmp(to_codeset, from_codeset) == 0)
		return (GIConv)-1;
	return g_iconv_open(to_codeset, from_codeset);
}

void rlib_charencoder_free(GIConv converter UNUSED) {
	if (converter != (GIConv)-1 && converter != (GIConv)0)
		g_iconv_close(converter);
}

gint rlib_charencoder_convert(GIConv converter UNUSED, const gchar **inbuf, gsize *inbytes_left UNUSED, gchar **outbuf, gsize *outbytes_left UNUSED) {
	if((converter == (GIConv) -1) || (converter == (GIConv) 0)) {
		*outbuf = g_strdup(*inbuf);
		return 1;
	} else {
		*outbuf = g_convert_with_iconv(*inbuf, strlen(*inbuf), converter, inbytes_left, outbytes_left, NULL);
		return *outbuf ? 0 : -1;
	}
}
