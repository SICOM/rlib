/*
 *  Copyright (C) 2003-2006 SICOM Systems, INC.
 *
 *  Authors: Chet Heilman <cheilman@sicompos.com>
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

/* Can change name to rlib_value after conversion is complete.
 * Currently rlib_var to avoid confusion of new/old
 */
/*==================================================================*/
#ifndef RLIB_VAR_H
#define RLIB_VAR_H

#include <glib.h>

#include "rlib-internal.h"
#include "datetime.h"


#define RLIB_VAR_NUMBER		1
#define RLIB_VAR_STRING		2
#define RLIB_VAR_DATETIME	3
#define RLIB_VAR_REF		4
#define RLIB_VAR_IIF		5

#define RV_MED_SIZE	256

#define MAXRLIBVARSTACK 100


#define RLIB_VAR_OK		0


union u_rlib_var {
	struct rlib_datetime dt;
	gint64 num;
	const gchar *ref; /* A pointer to a CONSTANT string - does not get deallocated */
	const void *iif;  /* Does not belong here */
	gchar ch[32]; /* Extended by additional allocation at struct end. */
};


typedef struct rlib_var rlib_var;
struct rlib_var {
	gint type;
	rlib_var *link;
	rlib_var *alloclink;
	gint len;
	union u_rlib_var value;
};


typedef struct rlib_var_factory rlib_var_factory;
struct rlib_var_factory {
	rlib_var *headsm; /* rlib vars for pointer, long long and rlib_datetime values */
	rlib_var *headmd; /* rlib vars for strings < 256 bytes */
	rlib_var *headlg; /* rlib vars for big strings headlg is sorted smest->lgest */
	rlib_var *headalloc; /* Holds a reference to all allocated chunks */
};


typedef struct rlib_var_stack rlib_var_stack;
struct rlib_var_stack {
	rlib_var **base;
	rlib_var **cur;
	rlib_var **max;
	rlib_var *stack[MAXRLIBVARSTACK]; /* This could also be allocated and 'grown'
						to make it dynamic and not size limited */
};

#endif


