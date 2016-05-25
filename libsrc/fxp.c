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
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include <config.h>

#define DIV0(a,b) ((b) == 0 ? 0 : ((a)/(b)))
#define MOD0(a,b) ((b) == 0 ? 0 : ((a)%(b)))

gint64 rlib_fxp_mul(gint64 a, gint64 b, gint64 factor) {
	gint64 wholea;
	gint64 wholeb;
	gint64 parta;
	gint64 partb;

	wholea = DIV0(a,factor);
	wholeb = DIV0(b,factor);
	parta = a-(wholea*factor);
	partb = b-(wholeb*factor);
	return( (wholea*wholeb)*factor+(parta*wholeb+partb*wholea)+DIV0(parta*partb,factor));
}

gint64 rlib_fxp_div(gint64 num, gint64 denom, gint places) {
	gint64 result = 0l;

	if( denom != 0L) {
		if( (llabs(denom)&((gint64)0xFC00000000000000LL)) ) {
			num >>= 4;
			denom >>= 4;
		}
		while( places >= 0) {
			result = result*10 + num/denom;
			num = 10*(num%denom);
			--places;
		}
	}
	return( result);
}
