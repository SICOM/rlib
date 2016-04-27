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
 *
 * This module manages rlib values. It includes a value factory that manages
 * the caching of buffers and a value stack for use by the pcode interpreter.
 * It will eventually be RENAMED to rlib_value.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <value.h>

#include "rlib-internal.h"
#include "config.h"
#include "util.h"


/*--- string functions ---*/
gboolean rlib_var_is_string(rlib_var *v) {
	return (v->type == RLIB_VAR_STRING) || (v->type == RLIB_VAR_REF);
}


const gchar *rlib_var_get_string(rlib_var *v) {
	switch (v->type) {
	case RLIB_VAR_STRING:
		return v->value.ch;
		break;
	case RLIB_VAR_REF:
		return (gchar *)v->value.ref;
		break;
	default:
		r_error(NULL, "rlib_var not a string");
		return "!ERR_STR";
		break;
	}
	return NULL;
}


void rlib_var_set_string(rlib_var *v, const char *str) {
	v->type = RLIB_VAR_STRING;
	g_strlcpy(v->value.ch, str, v->len);
}


gint rlib_var_concat_string(rlib_var *v, const char *str) {
	int len = r_strlen(v->value.ch);
	int tlen = len + r_strlen(str);
	if (tlen >= v->len) return -1;
	
	strcpy(v->value.ch + len, str);
	return tlen;
}


/*--- Number functions ---*/
gint64 rlib_var_get_number(rlib_var *v) {
	if (v->type != RLIB_VAR_NUMBER) {
		r_error(NULL, "rlib_var not a number");
		return (gint64)0LL;
	}
	return v->value.num;
}
                  

void rlib_var_set_number(rlib_var *v, gint64 n) {
	v->value.num = n;
}


void rlib_var_add_number(rlib_var *v, gint64 n) {
	v->value.num += n;
}


void rlib_var_mul_number(rlib_var *v, gint64 n) {
	v->value.num *= n;
}


void rlib_var_divby_number(rlib_var *v, gint64 n) {
	if (n == 0) {
		r_error(NULL, "Divide by 0");
		return;
	}
	v->value.num /= n;
}


void rlib_var_mod_number(rlib_var *v, gint64 n) {
	if (n == 0) {
		r_error(NULL, "Divide by 0");
		return;
	}
	v->value.num %= n;
}


gboolean rlib_var_is_number(rlib_var *v, int type) {
	return v->type == RLIB_VAR_NUMBER;
}


/*--- datetime functions ---*/
void rlib_var_set_datetime(rlib_var *v, rlib_datetime *dt) {
	v->type = RLIB_VAR_DATETIME;
	v->value.dt = *dt;	
}


rlib_datetime *rlib_var_get_datetime(rlib_var *v) {
	return &v->value.dt;
}


/*--- General purpose functions ---*/
gint rlib_var_get_type(rlib_var *v) {
	return v->type;
}


const gchar *rlib_var_get_type_name(rlib_var *v) {
	const gchar *result = "(null)";
	if (v) {
		switch (v->type) {
		case RLIB_VAR_NUMBER:
			result = "number";
			break;
		case RLIB_VAR_STRING:
			result = "string";
			break;
		case RLIB_VAR_REF:
			result = "reference";
			break;
		case RLIB_VAR_IIF:
			result = "iif";
			break;
		case RLIB_VAR_DATETIME:
			result = "datetime";
			break;
		default:
			result = "???????";
			break;
		}
	}
	return result;
}


/*==================================================================
  rlib_var_factory
  ==================================================================*/

/* Creates a new factory */
rlib_var_factory *rlib_var_factory_new(void) {
	rlib_var_factory *f = g_new0(rlib_var_factory, 1);
	return f;
}


/* ALLOCATES a new rlib_val and catalogs it for later destruction */
rlib_var *rlib_var_factory_alloc_new(rlib_var_factory *f, int size) {
	rlib_var *v = g_malloc(sizeof(rlib_var) + size);
	memset(v, 0, sizeof(rlib_var));
	v->len = size + sizeof(union u_rlib_var); /* MAX writeable length of value */
	v->alloclink = f->headalloc; /* So it can be freed */
	f->headalloc = v;
	return v;
}


/**
 * Get a SMALL value to hold a pointer, long long or datetime or small string
 */
rlib_var *rlib_var_factory_get_small(rlib_var_factory *f) {
	rlib_var *v = f->headsm;
	if (v) {
		f->headsm = v->link;
	} else {
		v = rlib_var_factory_alloc_new(f, 0);
	}
	return v;
}


/**
 * Get a value to hold something of the specified size.
 * size must INCLUDE terminating nul for strings
 */
static rlib_var *rlib_var_factory_get(rlib_var_factory *f, int size) {
	rlib_var *v = NULL;
	if (size <= sizeof(union u_rlib_var)) {
		v = rlib_var_factory_get_small(f);
	} else if (size <= RV_MED_SIZE) {
		v = f->headmd;
		if (v) {
			f->headmd = v->link;
		} else {
			v = rlib_var_factory_alloc_new(f, RV_MED_SIZE - sizeof(union u_rlib_var));
		}
	} else {
		rlib_var **vlast = &f->headlg;
		rlib_var *rv = *vlast;
		while (rv) { /* See if we have one that is big enough */
			if (rv->len >= size) {
				v = rv;
				*vlast = rv->link; /* Remove link from the chain */
				break;
			} else {
				vlast = &rv->link;
				rv = *vlast;
			}
		}
		/* Nope, we have to allocate it. */
		if (!v) v = rlib_var_factory_alloc_new(f, size - sizeof(union u_rlib_var));
	}
	return v;
}


/* Gets an appropriately sized rlib_var and copies the string to it. */
rlib_var *rlib_var_factory_new_string(rlib_var_factory *f, const gchar *str) {
	int len = r_strlen(str) + 1;
	rlib_var *v = rlib_var_factory_get(f, len);
	g_strlcpy(v->value.ch, str, len); /* just strcpy maybe?? */
	v->type = RLIB_VAR_STRING;
	return v;
}


/* Gets an rlib_var and sets to this number */
rlib_var *rlib_var_factory_new_number(rlib_var_factory *f, gint64 n) {
	rlib_var *v = rlib_var_factory_get_small(f);
	v->value.num = n;
	v->type = RLIB_VAR_NUMBER;
	return v;
}


/* Gets an rlib_var and sets to this date-time */
rlib_var *rlib_var_factory_new_datetime(rlib_var_factory *f, struct rlib_datetime *dt) {
	rlib_var *v = rlib_var_factory_get_small(f);
	v->value.dt = *dt;
	v->type = RLIB_VAR_DATETIME;
	return v;
}


/*
 * Gets an rlib_var and sets it to a constant string that WILL NOT BE FREED
 * A POINTER is stored. The caller must manage the allocation /deallocation of
 * this. It must persist throughout the lifetime of the rlib_var or else big
 * problems.
 */
rlib_var *rlib_var_factory_new_reference(rlib_var_factory *f, const gchar *str) {
	rlib_var *v = rlib_var_factory_get_small(f);
	v->value.ref = str;
	v->type = RLIB_VAR_REF;
	return v;
}


/* TODO: fix this. IIFs don't belong here. */
rlib_var *rlib_var_factory_new_iif(rlib_var_factory *f, const void *iif) {
	rlib_var *v = rlib_var_factory_get_small(f);
	v->value.iif = iif;
	v->type = RLIB_VAR_IIF;
	return v;
}



/* Add an rlib_val back to the cache for re-use. */
void rlib_var_factory_free_value(rlib_var_factory *f, rlib_var *v) {
	switch (v->len) {
	case sizeof(union u_rlib_var):
		v->link = f->headsm;
		f->headsm = v;
		break;
	case RV_MED_SIZE:
		v->link = f->headmd;
		f->headmd = v;
		break;
	default:
		{ /* Insert into large value list in size increasing order */
			rlib_var **vlast = &f->headlg;
			rlib_var *rv = *vlast;
			gint size = v->len; 
			while (rv) {
				if (rv->len >= size) {
					break;
				}
			}
			v->link = *vlast;
			*vlast = v;
		}
		break;
	}
}


/**
 * Frees all memory allocated for this instance and destroys the reference
 */
void rlib_var_factory_destroy(rlib_var_factory **fptr) {
	rlib_var_factory *f = *fptr;
	*fptr = NULL;
	if (f) {
		rlib_var *rv = f->headalloc;
		rlib_var *rv2;
		while (rv) { /* FREE ALL MEMORY THAT WAS ALLOCATED */
			rv2 = rv->alloclink;
			g_free(rv);
			rv = rv2;
		}
		g_free(f);
	}
}


rlib_var_stack *rlib_var_stack_new(void) {
	rlib_var_stack *s = g_new0(rlib_var_stack, 1);
	s->base = s->cur = s->stack;
	s->max = &s->stack[MAXRLIBVARSTACK];
	return s;
}


void rlib_var_stack_push(rlib_var_stack *s, rlib_var *val) {
	if (s->cur < s->max) *s->cur++ = val;
	else r_error(NULL, "Stack OVERFLOW!!!!!");
}


rlib_var *rlib_var_stack_pop(rlib_var_stack *s) {
	if (s->cur > s->base) return *(--s->cur); 
	r_error(NULL, "Stack UNDERFLOW!!!!");
	return NULL;
}


/* Like _pop, but DOES NOT REMOVE the TOS */
rlib_var *rlib_var_stack_peek(rlib_var_stack *s) {
	if (s->cur > s->base) return *s->cur; 
	r_error(NULL, "Stack UNDERFLOW!!!!");
	return NULL;
}


void rlib_var_stack_destroy(rlib_var_stack **sptr) {
	rlib_var_stack *s = *sptr;
	if (s) {
		g_free(s);
	}
	*sptr = NULL;
}



