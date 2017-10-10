/*
 *  Copyright (C) 2003-2017 SICOM Systems, INC.
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
 * $Id$s
 *
 * This module implements a characterencoder that converts a character
 * either to or from UTF8 which is the internal language of RLIB.
 * it is implemented as a simple 'class'.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include "config.h"
#include <errno.h>

#include "rlib.h"
#include "rlib_input.h"
#include "rlib_langinfo.h"

GIConv rlib_charencoder_new(const gchar *to_codeset, const gchar *from_codeset) {
	return g_iconv_open(to_codeset, from_codeset);
}

void rlib_charencoder_free(GIConv converter) {
	if(converter > 0)
		g_iconv_close(converter);
}

/*
 * This function expects that the output buffer is
 * pre-allocated and the buffer is at least
 * one byte larger than *outbytes_left.
 */
gsize rlib_charencoder_convert(GIConv converter, gchar **inbuf, gsize *inbytes_left, gchar **outbuf, gsize *outbytes_left, gboolean *error) {
	*error = FALSE;

	if((converter == (GIConv) -1) || (converter == (GIConv) 0)) {
		gsize copy_data = (*outbytes_left < *inbytes_left ? *outbytes_left : *inbytes_left);
		memcpy(*outbuf, *inbuf, copy_data);
		(*outbuf)[copy_data] = '\0';
		return 1;
	} else {
		gsize ret = 0;
		gchar *outbuf_old = *outbuf;
		gsize outbuf_len_old = *outbytes_left;
		while (*inbytes_left > 0) {
			ret = g_iconv(converter, inbuf, inbytes_left, outbuf, outbytes_left);
			if (ret == -1) {
				gchar *old_inbuf = *inbuf;
				gchar *tmp_inbuf = " ";
				gsize tmp_inbytes = 1;

				*inbuf = g_utf8_next_char(old_inbuf);
				*inbytes_left -= (*inbuf - old_inbuf);

				g_iconv(converter, &tmp_inbuf, &tmp_inbytes, outbuf, outbytes_left);

				*error = TRUE;
			}
		}
		outbuf_old[outbuf_len_old - *outbytes_left] = '\0';
		return ret;
	}
}
