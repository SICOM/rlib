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
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>
#include "config.h"

#include "rlib.h"
#include "pcode.h"
#include "rlib_langinfo.h"

#ifndef RADIXCHAR
#define RADIXCHAR DECIMAL_POINT
#endif

/*
	This will convert a infix string i.e.: 1 * 3 (3+2) + amount + v.whatever
	and convert it to a math stack... sorta like using postfix

	See the verbs table for all of the operators that are supported

	Operands are 	numbers,
						field names such as amount or someresultsetname.field
						rlib varables reffereced as v.name

*/

struct rlib_pcode_operator rlib_pcode_verbs[] = {
	{"+",		 	1, 4,	TRUE,	OP_ADD,		FALSE,	rlib_pcode_operator_add, NULL},
	{"*",		 	1, 5, TRUE,	OP_MUL,		FALSE,	rlib_pcode_operator_multiply, NULL},
	{"-",		 	1, 4,	TRUE,	OP_SUB,		FALSE,	rlib_pcode_operator_subtract, NULL},
	{"/",		 	1, 5,	TRUE,	OP_DIV,		FALSE,	rlib_pcode_operator_divide, NULL},
	{"%",  	 	1, 5,	TRUE,	OP_MOD,		FALSE,	rlib_pcode_operator_mod, NULL},
	{"^",		 	1, 6,	TRUE,	OP_POW,		FALSE,	rlib_pcode_operator_pow, NULL},
	{"<=",	 	2, 2,	TRUE,	OP_LTE,		FALSE,	rlib_pcode_operator_lte, NULL},
	{"<",		 	1, 2,	TRUE,	OP_LT,		FALSE,	rlib_pcode_operator_lt, NULL},
	{">=",	 	2, 2,	TRUE,	OP_GTE,		FALSE,	rlib_pcode_operator_gte, NULL},
	{">",		 	1, 2,	TRUE,	OP_GT,		FALSE,	rlib_pcode_operator_gt, NULL},
	{"==",	 	2, 2,	TRUE,	OP_EQL,		FALSE,	rlib_pcode_operator_eql, NULL},
	{"!=",	 	2, 2,	TRUE,	OP_NOTEQL,	FALSE,	rlib_pcode_operator_noteql, NULL},
	{"&&",	    	2, 1,	TRUE,	OP_LOGICALAND,		FALSE,	rlib_pcode_operator_logical_and, NULL},
	{"&",	 	1, 1,	TRUE,	OP_AND,		FALSE,	rlib_pcode_operator_and, NULL},
	{"||",	 	2, 1,	TRUE,	OP_LOGICALOR,		FALSE,	rlib_pcode_operator_logical_or, NULL},
	{"|",	 	1, 1,	TRUE,	OP_OR,		FALSE,	rlib_pcode_operator_or, NULL},
	{"(",		 	1, 0,	FALSE,OP_LPERN,	FALSE,	NULL, NULL},
	{")",		 	1, 99,FALSE,OP_RPERN,	FALSE,	NULL, NULL},
	{",",		 	1, 99,FALSE,OP_COMMA,	FALSE,	NULL, NULL},
	{"abs(",	 	4, 0,	TRUE,	OP_ABS,		TRUE,		rlib_pcode_operator_abs, NULL},
	{"atan(", 	5, 0,	TRUE,	OP_ATAN, 	TRUE, 	rlib_pcode_operator_atan, NULL},
	{"ceil(", 	5, 0,	TRUE,	OP_CEIL, 	TRUE, 	rlib_pcode_operator_ceil, NULL},
	{"chgdateof(", 	10, 0,	TRUE,	OP_CHGDATEOF,  	TRUE, 	rlib_pcode_operator_chgdateof, NULL},
	{"chgtimeof(", 	10, 0,	TRUE,	OP_CHGTIMEOF,  	TRUE, 	rlib_pcode_operator_chgtimeof, NULL},
	{"cos(",	 	4, 0,	TRUE,	OP_COS,		TRUE, 	rlib_pcode_operator_cos, NULL},
	{"date(", 	5, 0,	TRUE,	OP_DATE,  	TRUE, 	rlib_pcode_operator_date, NULL},
	{"dateof(", 7, 0,	TRUE,	OP_DATEOF,  	TRUE, 	rlib_pcode_operator_dateof, NULL},
	{"day(", 	4, 0,	TRUE,	OP_DAY,  	TRUE, 	rlib_pcode_operator_day, NULL},
	{"dim(", 	4, 0,	TRUE,	OP_DIM,  	TRUE, 	rlib_pcode_operator_dim, NULL},
	{"dtos(", 	5, 0,	TRUE,	OP_DTOS,  	TRUE, 	rlib_pcode_operator_dtos, NULL},
	{"dtosf(", 	6, 0,	TRUE,	OP_DTOSF,  	TRUE, 	rlib_pcode_operator_dtosf, NULL},
	{"eval(", 	5, 0,	TRUE,	OP_EVAL,  	TRUE, 	rlib_pcode_operator_eval, NULL},
	{"exp(",	 	4, 0,	TRUE,	OP_EXP,		TRUE,		rlib_pcode_operator_exp, NULL},
	{"floor(",	6, 0,	TRUE,	OP_FLOOR,	TRUE, 	rlib_pcode_operator_floor, NULL},
	{"format(", 	7, 0,	TRUE,	OP_FORMAT,  	TRUE, 	rlib_pcode_operator_format, NULL},
	{"fxpval(", 7, 0,	TRUE,	OP_FXPVAL, 	TRUE, 	rlib_pcode_operator_fxpval, NULL},
	{"gettimeinsecs(", 	14, 0,	TRUE,	OP_GETTIMESECS,  	TRUE, 	rlib_pcode_operator_gettimeinsecs, NULL},
	{"iif(",  	4, 0, TRUE,	OP_IIF,		TRUE, 	rlib_pcode_operator_iif, NULL},
	{"isnull(",	7, 0,	TRUE,	OP_ISNULL, 	TRUE, 	rlib_pcode_operator_isnull, NULL},
	{"left(", 	5, 0,	TRUE,	OP_LEFT, 	TRUE, 	rlib_pcode_operator_left, NULL},
	{"ln(", 	 	3, 0,	TRUE,	OP_LN,		TRUE,		rlib_pcode_operator_ln, NULL},
	{"lower(", 	6, 0,	TRUE,	OP_LOWER,  	TRUE, 	rlib_pcode_operator_lower, NULL},
	{"mid(", 	4, 0, TRUE,	OP_LEFT, 	TRUE, 	rlib_pcode_operator_substring, NULL},
	{"month(", 	6, 0,	TRUE,	OP_MONTH,  	TRUE, 	rlib_pcode_operator_month, NULL},
	{"proper(", 7, 0,	TRUE,	OP_PROPER, 	TRUE,		rlib_pcode_operator_proper, NULL},
	{"right(", 	6, 0,	TRUE,	OP_RIGHT, 	TRUE, 	rlib_pcode_operator_right, NULL},
	{"round(",	6, 0,	TRUE,	OP_ROUND,	TRUE, 	rlib_pcode_operator_round, NULL},
	{"settimeinsecs(", 	14, 0,	TRUE,	OP_SETTIMESECS,  	TRUE, 	rlib_pcode_operator_settimeinsecs, NULL},
	{"sin(",	 	4, 0,	TRUE,	OP_SIN,		TRUE, 	rlib_pcode_operator_sin, NULL},
	{"sqrt(", 	5, 0,	TRUE,	OP_SQRT, 	TRUE,		rlib_pcode_operator_sqrt, NULL},
	{"stod(", 	5, 0,	TRUE,	OP_STOD,  	TRUE,		rlib_pcode_operator_stod, NULL},
	{"stodt(", 	6, 0,	TRUE,	OP_STOD,  	TRUE, 	rlib_pcode_operator_stodt, NULL},
	{"stodtsql(", 9, 0,	TRUE,	OP_STODSQL,  	TRUE, 	rlib_pcode_operator_stodtsql, NULL},
	{"str(", 	4, 0,	TRUE,	OP_STR,  	TRUE, 	rlib_pcode_operator_str, NULL},
	{"strlen(", 	7, 0,	TRUE,	OP_STRLEN,  	TRUE, 	rlib_pcode_operator_strlen, NULL},
	{"timeof(", 7, 0,	TRUE,	OP_TIMEOF,  	TRUE, 	rlib_pcode_operator_timeof, NULL},
	{"tstod(", 	6, 0,	TRUE,	OP_TSTOD,  	TRUE, 	rlib_pcode_operator_tstod, NULL},
	{"upper(", 	6, 0,	TRUE,	OP_UPPER,  	TRUE, 	rlib_pcode_operator_upper, NULL},
	{"val(", 	4, 0,	TRUE,	OP_VAL,  	TRUE,		rlib_pcode_operator_val, NULL},
	{"wiy(", 	4, 0,	TRUE,	OP_WIY,  	TRUE, 	rlib_pcode_operator_wiy, NULL},
	{"wiyo(", 	5, 0,	TRUE,	OP_WIYO,  	TRUE, 	rlib_pcode_operator_wiyo, NULL},
	{"year(", 	5, 0,	TRUE,	OP_YEAR,  	TRUE, 	rlib_pcode_operator_year, NULL},

	{ NULL, 	 	0, 0, FALSE,-1,			TRUE,		NULL, NULL}
};

void rlib_pcode_find_index(rlib *r) {
	struct rlib_pcode_operator *op;
	int i=0;
	op = rlib_pcode_verbs;

	r->pcode_alpha_index = -1;
	r->pcode_alpha_m_index = -1;

	while(op && op->tag != NULL) {
		if(r->pcode_alpha_index == -1 && isalpha(op->tag[0]))
			r->pcode_alpha_index = i;
		if(r->pcode_alpha_m_index == -1 && op->tag[0] == 'm')
			r->pcode_alpha_m_index = i;

		op++;
		i++;
	}
}

void rlib_pcode_free(struct rlib_pcode *code) {
	gint i=0;

	if(code == NULL)
		return;

	for(i=0;i<code->count;i++) {
		if(code->instructions[i].instruction == PCODE_PUSH) {
			struct rlib_pcode_operand *o = code->instructions[i].value;
			switch (o->type)
			{
				case OPERAND_STRING:
				case OPERAND_NUMBER:
				case OPERAND_DATE:
				case OPERAND_FIELD:
				case OPERAND_MEMORY_VARIABLE:
					g_free(o->value);
					break;

				case OPERAND_IIF:
				{
					struct rlib_pcode_if *rpif = o->value;
					rlib_pcode_free(rpif->evaulation);
					rlib_pcode_free(rpif->true);
					rlib_pcode_free(rpif->false);
					g_free(rpif);
					break;
				}

				case OPERAND_VARIABLE:
				case OPERAND_RLIB_VARIABLE:
				case OPERAND_METADATA:
				default:
					break;
			}
			g_free(o);
		}
	}
	g_free(code->instructions);
	g_free(code);
}

struct rlib_operator_stack {
	gint count;
	gint pcount;
	struct rlib_pcode_operator *op[200];
};

static struct rlib_pcode_operator * rlib_find_operator(rlib *r, gchar *ptr, struct rlib_operator_stack *os, struct rlib_pcode *p, int have_operand) {
	gint len = strlen(ptr);
	gint result;
	struct rlib_pcode_operator *op;
	GSList *list;
	gboolean alpha = FALSE;

	if(len > 0 && isalpha(ptr[0])) {
		alpha = TRUE;
		if(isupper(ptr[0])) {
			op = NULL;
		} else {
			if(r == NULL) {
				op = rlib_pcode_verbs + 18;
			} else {
				if(ptr[0] >= 'm')
					op = rlib_pcode_verbs + r->pcode_alpha_m_index;
				else
					op = rlib_pcode_verbs + r->pcode_alpha_index;
			}
		}
	} else
		op = rlib_pcode_verbs;

	while(op && op->tag != NULL) {
		if(len >= op->taglen) {
			if((result=strncmp(ptr, op->tag, op->taglen))==0) {
				if(op->opnum == OP_SUB) {
					if(have_operand || p->count > 0)
						return op;
				} else
					return op;
			} else if(alpha && result < 0) {
				break;
			}
		}
		op++;
	}
	if(r != NULL) { 
		for(list=r->pcode_functions;list != NULL; list=list->next) {
			op = list->data;
			if(len >= op->taglen) {
				if(!strncmp(ptr, op->tag, op->taglen)) {
					return op;
				}
			}	
		}
	}
	return NULL;
}

void rlib_pcode_init(struct rlib_pcode *p) {
	p->count = 0;
	p->instructions = NULL;
}

gint rlib_pcode_add(struct rlib_pcode *p, struct rlib_pcode_instruction *i) {
	p->instructions = g_try_realloc(p->instructions, sizeof(struct rlib_pcode_instruction) * (p->count + 1));
	p->instructions[p->count++] = *i;
	return 0;
}

struct rlib_pcode_instruction * rlib_new_pcode_instruction(struct rlib_pcode_instruction *rpi, gint instruction, gpointer value) {
	rpi->instruction = instruction;
	rpi->value = value;
	return rpi;
}

gint64 rlib_str_to_long_long(rlib *r, gchar *str) {
	gint64 foo;
	gchar *other_side;
	gint len=0;
	gint64 left=0, right=0;
	gint sign = 1;
	gchar decimalsep = '.';
	gchar *temp;

	if(str == NULL)
		return 0;
	temp = nl_langinfo(RADIXCHAR);
	if (!temp || r_strlen(temp) != 1) {
		r_warning(r, "nl_langinfo returned %s as DECIMAL_POINT", temp);
	} else {
		decimalsep = *temp;
	}
	left = atoll(str);
	other_side = strchr(str, decimalsep);
	sign = strchr(str, '-')? -1 : 1;
	if (left < 0) {
/*		sign = -1; */
		left = -left;
	}
	if(other_side != NULL) {
		other_side++;
		right = atoll(other_side);
		len = r_strlen(other_side);
	}
	if (len > RLIB_FXP_PRECISION) {
		len = RLIB_FXP_PRECISION;
		r_error(r, "Numerical overflow in str_to_long_long conversion [%s]\n", str);
	}
	foo = ((right * tentothe(RLIB_FXP_PRECISION - len))
				+ (left * RLIB_DECIMAL_PRECISION)) * sign;
	return foo;
}


#if !USE_RLIB_VAL
gint rvalcmp(struct rlib_value *v1, struct rlib_value *v2) {
	if(RLIB_VALUE_IS_NUMBER(v1) && RLIB_VALUE_IS_NUMBER(v2)) {
		if(RLIB_VALUE_GET_AS_NUMBER(v1) == RLIB_VALUE_GET_AS_NUMBER(v2))
			return 0;
		else
			return -1;
	}
	if(RLIB_VALUE_IS_STRING(v1) && RLIB_VALUE_IS_STRING(v2)) {
		if(RLIB_VALUE_GET_AS_STRING(v2) == NULL &&  RLIB_VALUE_GET_AS_STRING(v1) == NULL)
			return 0;
		else if(RLIB_VALUE_GET_AS_STRING(v2) == NULL ||  RLIB_VALUE_GET_AS_STRING(v1) == NULL)
			return -1;
		return r_strcmp(RLIB_VALUE_GET_AS_STRING(v1), RLIB_VALUE_GET_AS_STRING(v2));
	}
	if(RLIB_VALUE_IS_DATE(v1) && RLIB_VALUE_IS_DATE(v2)) {
		return rlib_datetime_compare(&RLIB_VALUE_GET_AS_DATE(v1), &RLIB_VALUE_GET_AS_DATE(v2));
	}
	return -1;
}
#endif

struct rlib_pcode_operand * rlib_new_operand(rlib *r, struct rlib_part *part, struct rlib_report *report, gchar *str, gchar *infix, gint line_number, gboolean look_at_metadata) {
	gint resultset;
	gpointer field=NULL;
	gchar *memresult;
	struct rlib_pcode_operand *o;
	struct rlib_report_variable *rv;
	struct rlib_metadata *metadata;
	gint rvar;
	o = g_new0(struct rlib_pcode_operand, 1);
	if(str[0] == '\'') {
		gint slen;
		gchar *newstr;
		slen = strlen(str);
		if(slen < 2) {
			newstr = g_malloc(2);
			newstr[0] = ' ';
			newstr[1] = 0;
			r_error(r, "Line: %d Invalid String! <rlib_new_operand>:: Bad PCODE [%s]\n", line_number, infix);
		} else {
			newstr = g_malloc(slen-1);
			memcpy(newstr, str+1, slen-1);
			newstr[slen-2] = '\0';
		}
		o->type = OPERAND_STRING;
		o->value = newstr;
	} else if(str[0] == '{') {
		struct rlib_datetime *tm_date = g_malloc(sizeof(struct rlib_datetime));
		str++;
		o->type = OPERAND_DATE;
		o->value = stod(tm_date, str);
	} else if (!strcasecmp(str, "yes") || !strcasecmp(str, "true")) {
		gint64 *newnum = g_malloc(sizeof(gint64));
		o->type = OPERAND_NUMBER;
		*newnum = TRUE * RLIB_DECIMAL_PRECISION;
		o->value = newnum;
	} else if (!strcasecmp(str, "no") || !strcasecmp(str, "false")) {
		gint64 *newnum = g_malloc(sizeof(gint64));
		o->type = OPERAND_NUMBER;
		*newnum = FALSE  * RLIB_DECIMAL_PRECISION;
		o->value = newnum;
	} else if (!g_ascii_strcasecmp(str, "left")) {
		gint64 *newnum = g_malloc(sizeof(gint64));
		o->type = OPERAND_NUMBER;
		*newnum = RLIB_ALIGN_LEFT * RLIB_DECIMAL_PRECISION;
		o->value = newnum;
	} else if (!g_ascii_strcasecmp(str, "right")) {
		gint64 *newnum = g_malloc(sizeof(gint64));
		o->type = OPERAND_NUMBER;
		*newnum = RLIB_ALIGN_RIGHT * RLIB_DECIMAL_PRECISION;
		o->value = newnum;
	} else if (!g_ascii_strcasecmp(str, "center")) {
		gint64 *newnum = g_malloc(sizeof(gint64));
		o->type = OPERAND_NUMBER;
		*newnum = RLIB_ALIGN_CENTER * RLIB_DECIMAL_PRECISION;
		o->value = newnum;
	} else if (!g_ascii_strcasecmp(str, "landscape")) {
		gint64 *newnum = g_malloc(sizeof(gint64));
		o->type = OPERAND_NUMBER;
		*newnum = RLIB_ORIENTATION_LANDSCAPE * RLIB_DECIMAL_PRECISION;
		o->value = newnum;
	} else if (!g_ascii_strcasecmp(str, "portrait")) {
		gint64 *newnum = g_malloc(sizeof(gint64));
		o->type = OPERAND_NUMBER;
		*newnum = RLIB_ORIENTATION_PORTRAIT * RLIB_DECIMAL_PRECISION;
		o->value = newnum;
	} else if (isdigit(*str) || (*str == '-') || (*str == '+') || (*str == '.')) {
		gint64 *newnum = g_malloc(sizeof(gint64));
		o->type = OPERAND_NUMBER;
		*newnum = rlib_str_to_long_long(r, str);
		o->value = newnum;
	} else if((rv = rlib_resolve_variable(r, part, report, str))) {
		o->type = OPERAND_VARIABLE;
		o->value = rv;
	} else if((memresult = rlib_resolve_memory_variable(r, str))) {
		o->type = OPERAND_MEMORY_VARIABLE;
		o->value = g_strdup(memresult);
	} else if((rvar = rlib_resolve_rlib_variable(r, str))) {
		o->type = OPERAND_RLIB_VARIABLE;
		o->value = GINT_TO_POINTER(rvar);
	} else if(r != NULL && (metadata = g_hash_table_lookup(r->input_metadata, str)) != NULL && look_at_metadata == TRUE) {
		o->type = OPERAND_METADATA;
		o->value = metadata;
	} else if(r != NULL && rlib_resolve_resultset_field(r, str, &field, &resultset)) {
		struct rlib_resultset_field *rf = g_malloc(sizeof(struct rlib_resultset_field));
		rf->resultset = resultset;
		rf->field = field;
		o->type = OPERAND_FIELD;
		o->value = rf;
	} else {
		const gchar *err = "BAD_OPERAND";
		gchar *newstr = g_malloc(r_strlen(err)+1);
		strcpy(newstr, err);
		o->type = OPERAND_STRING;
		o->value = newstr;
		r_error(r, "Line: %d Unrecognized operand [%s]\n The Expression Was [%s]\n", line_number, str, infix);
	}
	return o;
}


void rlib_pcode_dump(rlib *r, struct rlib_pcode *p, gint offset) {
	gint i,j;
	rlogit(r, "DUMPING PCODE IT HAS %d ELEMENTS\n", p->count);
	for(i=0;i<p->count;i++) {
		for(j=0;j<offset*5;j++)
			rlogit(r, " ");
		rlogit(r, "DUMP: ");
		if(p->instructions[i].instruction == PCODE_PUSH) {
			struct rlib_pcode_operand *o = p->instructions[i].value;
			rlogit(r, "PUSH: ");
			if(o->type == OPERAND_NUMBER)
				rlogit(r, "%" PRId64, *((gint64 *)o->value));
			else if(o->type == OPERAND_STRING)
				rlogit(r, "'%s'", (char *)o->value);
			else if(o->type == OPERAND_FIELD) {
				struct rlib_resultset_field *rf = o->value;
				rlogit(r, "Result Set = [%d]; Field = [%d]", rf->resultset, rf->field);
			} else if(o->type == OPERAND_METADATA) {
				struct rlib_metadata *metadata = o->value;
				rlogit(r, "METADATA ");
				rlib_pcode_dump(r, metadata->formula_code, offset+1);
			} else if(o->type == OPERAND_MEMORY_VARIABLE) {
				rlogit(r, "Result Memory Variable = [%s]", (char *)o->value);
			} else if(o->type == OPERAND_VARIABLE) {
				struct rlib_report_variable *rv = o->value;
				rlogit(r, "Result Variable = [%s]", rv->xml_name);
			} else if(o->type == OPERAND_RLIB_VARIABLE) {
				rlogit(r, "RLIB Variable\n");
			} else if(o->type == OPERAND_IIF) {
				struct rlib_pcode_if *rpi = o->value;
				rlogit(r, "*IFF EXPRESSION EVAULATION:\n");
				rlib_pcode_dump(r, rpi->evaulation, offset+1);
				rlogit(r, "*IFF EXPRESSION TRUE:\n");
				rlib_pcode_dump(r, rpi->true, offset+1);
				rlogit(r, "*IFF EXPRESSION FALSE:\n");
				rlib_pcode_dump(r, rpi->false, offset+1);
				rlogit(r, "*IFF DONE\n");

			}

		} else if(p->instructions[i].instruction == PCODE_EXECUTE) {
			struct rlib_pcode_operator *o = p->instructions[i].value;
			rlogit(r, "EXECUTE: %s", o->tag);
		}
		rlogit(r, "\n");
	}
}

int operator_stack_push(struct rlib_operator_stack *os, struct rlib_pcode_operator *op) {
	if((op->tag[0] == '(' || op->is_function) && op->opnum != OP_IIF)
		os->pcount++;
	else if(op->tag[0] == ')')
		os->pcount--;

	if(op->tag[0] != ')' && op->tag[0] != ',')
		os->op[os->count++] = op;

//r_error(NULL, "+++++++ operator_stack_push:: [%s]\n", op->tag);
	return 0;
}

struct rlib_pcode_operator * operator_stack_pop(struct rlib_operator_stack *os) {
	if(os->count > 0) {
//r_error(NULL, "------- operator_stack_pop:: [%s]\n", os->op[os->count-1]->tag);
		return os->op[--os->count];
	} else
		return NULL;
}

struct rlib_pcode_operator * operator_stack_peek(struct rlib_operator_stack *os) {
	if(os->count > 0) {
//r_error(NULL, "======== operator_stack_peek:: [%s]\n", os->op[os->count-1]->tag);
		return os->op[os->count-1];
	} else
		return NULL;
}


void operator_stack_init(struct rlib_operator_stack *os) {
	os->count = 0;
	os->pcount = 0;
}

gint operator_stack_is_all_less(struct rlib_operator_stack *os, struct rlib_pcode_operator *op) {
	gint i;
	if(op->tag[0] == ')' || op->tag[0] == ',')
		return FALSE;

	if(op->tag[0] == '(' || op->is_function == TRUE)
		return TRUE;

	if(os->count == 0)
		return TRUE;

	for(i=os->count-1;i>=0;i--) {
		if(os->op[i]->tag[0] == '(' || os->op[i]->is_function == TRUE)
			break;
		if(os->op[i]->precedence >= op->precedence )
			return FALSE;
	}
	return TRUE;
}

void popopstack(struct rlib_pcode *p, struct rlib_operator_stack *os, struct rlib_pcode_operator *op) {
	struct rlib_pcode_operator *o;
	struct rlib_pcode_instruction rpi;
	if(op != NULL && (op->tag[0] == ')' || op->tag[0] == ',')) {
		while((o=operator_stack_peek(os))) {
			if(o->is_op == TRUE) {
				if(op->tag[0] != ',') {
					rlib_pcode_add(p, rlib_new_pcode_instruction(&rpi, PCODE_EXECUTE, o));
				} else {
					if(o->is_function == FALSE) {
						rlib_pcode_add(p, rlib_new_pcode_instruction(&rpi, PCODE_EXECUTE, o));
					}
				}
			}
			if(o->tag[0] == '(' || o->is_function == TRUE) {
				if(op->tag[0] != ',') /* NEED TO PUT THE ( or FUNCTION back on the stack cause it takes more then 1 paramater */
					operator_stack_pop(os);
				break;
			}
			operator_stack_pop(os);
		}
	} else {
//r_error(NULL, "#################### OP IS [%s]\n", op->tag);
		while((o=operator_stack_peek(os))) {
			if(o->tag[0] == '(' || o->is_function == TRUE) {
				break;
			}
			if(o->is_op == TRUE) {
//r_error(NULL, "@@@@@@@@@@@@@@@@@@@ ADDING [%s]\n", o->tag);
				rlib_pcode_add(p, rlib_new_pcode_instruction(&rpi, PCODE_EXECUTE, o));
			}
			operator_stack_pop(os);
		}
	}
}

static void forcepopstack(rlib *r, struct rlib_pcode *p, struct rlib_operator_stack *os) {
	struct rlib_pcode_operator *o;
	struct rlib_pcode_instruction rpi;
	while((o=operator_stack_pop(os))) {
		if(o->is_op == TRUE) {
//r_error(r, "forcepopstack:: Forcing [%s] Off the Stack\n", o->tag);
			rlib_pcode_add(p, rlib_new_pcode_instruction(&rpi, PCODE_EXECUTE, o));
		}
	}


}

void smart_add_pcode(struct rlib_pcode *p, struct rlib_operator_stack *os, struct rlib_pcode_operator *op) {
//r_error(NULL, "smart_add_pcode:: Adding [%s]\n", op->tag);

	if(operator_stack_is_all_less(os, op)) {
//r_error(NULL, "\tsmart_add_pcode:: JUST PUSH [%s]\n", op->tag);
		operator_stack_push(os, op);
	} else {
//r_error(NULL, "\tsmart_add_pcode:: POP AND PUSH [%s]\n", op->tag);
		popopstack(p, os, op);
		operator_stack_push(os, op);
	}
}

static gchar *skip_next_closing_paren(gchar *str) {
	gint ch;

	while ((ch = *str) && (ch != ')'))
		if (ch == '(') str = skip_next_closing_paren(str + 1);
		else ++str;
	return (ch == ')')? str + 1 : str;
}


struct rlib_pcode * rlib_infix_to_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, gchar *infix, gint line_number, gboolean look_at_metadata) {
	gchar *moving_ptr = infix;
	gchar *op_pointer = infix;
	gchar operand[255];
	gint found_op_last = FALSE;
	gint last_op_was_function = FALSE;
	gint move_pointers = TRUE;
	gint instr=0;
	gint indate=0;
	struct rlib_pcode_operator *op;
	struct rlib_pcode *pcodes;
	struct rlib_operator_stack os;
	struct rlib_pcode_instruction rpi;

	if(infix == NULL || infix[0] == '\0' || !strcmp(infix, ""))
		return NULL;

	pcodes = g_malloc(sizeof(struct rlib_pcode));

	rlib_pcode_init(pcodes);
	pcodes->infix_string = infix;
	pcodes->line_number = line_number;

	operator_stack_init(&os);
	rmwhitespacesexceptquoted(infix);

	while(*moving_ptr) {
		if(*moving_ptr == '\'') {
			if(instr) {
				instr = 0;
				moving_ptr++;
				if(!*moving_ptr)
					break;
			}	else
				instr = 1;
		}
		if(*moving_ptr == '{') {
			indate = 1;
		}
		if(*moving_ptr == '}') {
			indate = 0;
			moving_ptr++;
			if(!*moving_ptr)
				break;
		}

		if(!instr && !indate && (op=rlib_find_operator(r, moving_ptr, &os, pcodes, moving_ptr != op_pointer))) {
			if(moving_ptr != op_pointer) {
				memcpy(operand, op_pointer, moving_ptr - op_pointer);
				operand[moving_ptr - op_pointer] = '\0';
				if(operand[0] != ')') {
					rlib_pcode_add(pcodes, rlib_new_pcode_instruction(&rpi, PCODE_PUSH, rlib_new_operand(r, part, report, operand, infix, line_number, look_at_metadata)));
				}
/*				op_pointer += moving_ptr - op_pointer;
            How about just: */
				op_pointer = moving_ptr;
				found_op_last = FALSE;
				last_op_was_function = FALSE;
			}
			if((found_op_last == TRUE  && last_op_was_function == FALSE && op->tag[0] == '-')) {
				move_pointers = FALSE;
			} else {
				found_op_last = TRUE;
/*
				IIF (In Line If's) Are a bit more complicated.  We need to swallow up the whole string like "IIF(1<2,true part, false part)"
				And then idetify all the 3 inner parts, then pass in recursivly to our selfs and populate rlib_pcode_if and smaet_add_pcode that
*/
				if(op->opnum == OP_IIF) {
					gint in_a_string = FALSE;
					gint pcount=1;
					gint ccount=0;
					gchar *save_ptr, *iif, *save_iif;
					gchar *evaulation, *true=NULL, *false=NULL;
					struct rlib_pcode_if *rpif;
					struct rlib_pcode_operand *o;
					gchar in_a_string_in_a_iif = FALSE;
					moving_ptr +=  op->taglen;
					save_ptr = moving_ptr;
					while(*moving_ptr) {
						if(*moving_ptr == '\'')
							in_a_string = !in_a_string;

						if(in_a_string == FALSE) {
							if(*moving_ptr == '(')
								pcount++;
							if(*moving_ptr == ')')
								pcount--;
						}
						moving_ptr++;
						if(pcount == 0)
							break;
					}
					save_iif = iif = g_malloc(moving_ptr - save_ptr + 1);
					iif[moving_ptr-save_ptr] = '\0';
					memcpy(iif, save_ptr, moving_ptr-save_ptr);
					iif[moving_ptr-save_ptr-1] = '\0';
					evaulation = iif;
					while (*iif) {
						if(in_a_string_in_a_iif == FALSE) {
							if (*iif == '(')
								iif = skip_next_closing_paren(iif + 1);
							if (*iif == ')') {
								*iif = '\0';
								break;
							}
						}
						if (*iif == '\'')
							in_a_string_in_a_iif = !in_a_string_in_a_iif;
						if (*iif == ',' && !in_a_string_in_a_iif) {
							*iif='\0';
							if(ccount == 0)
								true = iif + 1;
							else if(ccount == 1)
								false = iif + 1;
							ccount++;
						}
						iif++;
					}
					rpif = g_malloc(sizeof(struct rlib_pcode_if));
					rpif->evaulation = rlib_infix_to_pcode(r, part, report, evaulation, line_number, look_at_metadata);
					rpif->true = rlib_infix_to_pcode(r, part, report, true, line_number, look_at_metadata);
					rpif->false = rlib_infix_to_pcode(r, part, report, false, line_number, look_at_metadata);
					rpif->str_ptr = iif;
					smart_add_pcode(pcodes, &os, op);
					o = g_malloc(sizeof(struct rlib_pcode_operand));
					o->type = OPERAND_IIF;
					o->value = rpif;
					rlib_pcode_add(pcodes, rlib_new_pcode_instruction(&rpi, PCODE_PUSH, o));
					if(1) {
						struct rlib_pcode_operator *ox;
						ox=operator_stack_pop(&os);
						rlib_pcode_add(pcodes, rlib_new_pcode_instruction(&rpi, PCODE_EXECUTE, ox));
					}
					moving_ptr-=1;
					op_pointer = moving_ptr;
					move_pointers = FALSE;
					g_free(save_iif);
				} else {
					smart_add_pcode(pcodes, &os, op);
					last_op_was_function = op->is_function;
					if(op->tag[0] == ')')
						last_op_was_function = TRUE;
					move_pointers = TRUE;
				}
			}
			if(move_pointers)
				moving_ptr = op_pointer = op_pointer + op->taglen + (moving_ptr - op_pointer);
			else
				moving_ptr++;
		} else
			moving_ptr++;
	}
	if((moving_ptr != op_pointer)) {
		memcpy(operand, op_pointer, moving_ptr - op_pointer);
		operand[moving_ptr - op_pointer] = '\0';
		if(operand[0] != ')') {
			rlib_pcode_add(pcodes, rlib_new_pcode_instruction(&rpi, PCODE_PUSH, rlib_new_operand(r, part, report, operand, infix, line_number, look_at_metadata)));
		}
	}
	forcepopstack(r, pcodes, &os);
	if(os.pcount != 0) {
		r_error(r, "Line: %d Compiler Error.  Parenthesis Mismatch [%s]\n", line_number, infix);
	}

	return pcodes;
}

#if !USE_RLIB_VAL
void rlib_value_stack_init(struct rlib_value_stack *vs) {
	vs->count = 0;
}

gint rlib_value_stack_push(rlib *r, struct rlib_value_stack *vs, struct rlib_value *value) {
	if(vs->count == 99)
		return FALSE;
	if(value == NULL) {
		r_error(r, "PCODE EXECUTION ERROR.. TRIED TO *PUSH* A NULL VALUE.. CHECK YOUR EXPRESSION!\n");
		return FALSE;
	}

	vs->values[vs->count++] = *value;
	return TRUE;
}

struct rlib_value * rlib_value_stack_pop(struct rlib_value_stack *vs) {
	if(vs->count <= 0) {
		vs->values[0].type = RLIB_VALUE_NONE;
		return &vs->values[0];
	} else {
		return &vs->values[--vs->count];
	}
}
struct rlib_value * rlib_value_new(struct rlib_value *rval, gint type, gint free_, gpointer value) {
	rval->type = type;
	rval->free = free_;

	if(type == RLIB_VALUE_NUMBER)
		rval->number_value = *(gint64 *)value;
	if(type == RLIB_VALUE_STRING)
		rval->string_value = value;
	if(type == RLIB_VALUE_DATE)
		rval->date_value = *((struct rlib_datetime *) value);
	if(type == RLIB_VALUE_IIF)
		rval->iif_value = value;

	return rval;
}

struct rlib_value * rlib_value_dup(struct rlib_value *orig) {
	struct rlib_value *new;
	new = g_malloc(sizeof(struct rlib_value));
	memcpy(new, orig, sizeof(struct rlib_value));
	if(orig->type == RLIB_VALUE_STRING)
		new->string_value = g_strdup(orig->string_value);
	return new;
}

struct rlib_value * rlib_value_dup_contents(struct rlib_value *rval) {
	if(rval->type == RLIB_VALUE_STRING) {
		rval->string_value = g_strdup(rval->string_value);
		rval->free = TRUE;
	}
	return rval;
}

gint rlib_value_free(struct rlib_value *rval) {
	if(rval == NULL)
		return FALSE;
	else if(rval->free == FALSE)
		return FALSE;
	else if(rval->type == RLIB_VALUE_STRING) {
		g_free(rval->string_value);
		rval->free = FALSE;
		RLIB_VALUE_TYPE_NONE(rval);
		return TRUE;
	}
	return FALSE;
}

struct rlib_value * rlib_value_new_number(struct rlib_value *rval, gint64 value) {
	rval->type = RLIB_VALUE_NUMBER;
	rval->free = FALSE;
	rval->number_value = value;
	return rval;
}

struct rlib_value * rlib_value_new_string(struct rlib_value *rval, const gchar *value) {
	return rlib_value_new(rval, RLIB_VALUE_STRING, TRUE, g_strdup(value));
}

struct rlib_value * rlib_value_new_date(struct rlib_value *rval, struct rlib_datetime *date) {
	return rlib_value_new(rval, RLIB_VALUE_DATE, FALSE, date);
}

struct rlib_value * rlib_value_new_error(struct rlib_value *rval) {
	return rlib_value_new(rval, RLIB_VALUE_ERROR, FALSE, NULL);
}
#endif

/*
	The RLIB SYMBOL TABLE is a bit commplicated because of all the datasources and internal variables
*/
struct rlib_value *rlib_operand_get_value(rlib *r, struct rlib_value *rval, struct rlib_pcode_operand *o,
struct rlib_value *this_field_value) {
	struct rlib_report_variable *rv = NULL;

	if(o->type == OPERAND_NUMBER) {
		return rlib_value_new(rval, RLIB_VALUE_NUMBER, FALSE, o->value);
	} else if(o->type == OPERAND_STRING) {
		return rlib_value_new(rval, RLIB_VALUE_STRING, FALSE, o->value);
	} else if(o->type == OPERAND_DATE) {
		return rlib_value_new(rval, RLIB_VALUE_DATE, FALSE, o->value);
	} else if(o->type == OPERAND_FIELD) {
		return rlib_value_new(rval, RLIB_VALUE_STRING, TRUE, rlib_resolve_field_value(r, o->value));
	} else if(o->type == OPERAND_METADATA) {
		struct rlib_metadata *metadata = o->value;
		*rval = metadata->rval_formula;
		return rval;
	} else if(o->type == OPERAND_MEMORY_VARIABLE) {
		return rlib_value_new(rval, RLIB_VALUE_STRING, FALSE, o->value);
	} else if(o->type == OPERAND_RLIB_VARIABLE) {
		gint type = ((long)o->value);
		if(type == RLIB_RLIB_VARIABLE_PAGENO) {
			gint64 pageno = r->current_page_number*RLIB_DECIMAL_PRECISION;
			return rlib_value_new_number(rval, pageno);
		} else if(type == RLIB_RLIB_VARIABLE_TOTPAGES) {
			gint64 pageno = r->current_page_number*RLIB_DECIMAL_PRECISION;
			return rlib_value_new_number(rval, pageno);
		} else if(type == RLIB_RLIB_VARIABLE_VALUE) {
			return this_field_value;
		} else if(type == RLIB_RLIB_VARIABLE_LINENO) {
			gint64 cln = r->current_line_number*RLIB_DECIMAL_PRECISION;
			return rlib_value_new_number(rval, cln);
		} else if(type == RLIB_RLIB_VARIABLE_DETAILCNT) {
			gint64 dcnt = r->detail_line_count * RLIB_DECIMAL_PRECISION;
			return rlib_value_new_number(rval, dcnt);
		} else if(type == RLIB_RLIB_VARIABLE_FORMAT) {
			return rlib_value_new_string(rval, rlib_format_get_name(r->format));
		}

	} else if(o->type == OPERAND_VARIABLE) {
		gint64 val = 0;
		struct rlib_value *count;
		struct rlib_value *amount;


		rv = o->value;


		count = &RLIB_VARIABLE_CA(rv)->count;
		amount = &RLIB_VARIABLE_CA(rv)->amount;

		if(rv->code == NULL && rv->type != RLIB_REPORT_VARIABLE_COUNT) {
			r_error(r, "Line: %d - Bad Expression in variable [%s] Variable Resolution: Assuming 0 value for variable	\n",rv->xml_name.line,rv->xml_name.xml);
		} else {
			if(rv->type == RLIB_REPORT_VARIABLE_COUNT) {
				val = RLIB_VALUE_GET_AS_NUMBER(count);
			} else if(rv->type == RLIB_REPORT_VARIABLE_EXPRESSION) {
				if(RLIB_VALUE_IS_ERROR(amount) || RLIB_VALUE_IS_NONE(amount)) {
					val = 0;
					r_error(r, "Variable Resolution: Assuming 0 value because rval is ERROR or NONE\n");
				} else  if (RLIB_VALUE_IS_STRING(amount)) {
					gchar *strval = g_strdup(RLIB_VALUE_GET_AS_STRING(amount));
					return rlib_value_new(rval, RLIB_VALUE_STRING, TRUE, strval);
				} else {
					val = RLIB_VALUE_GET_AS_NUMBER(amount);
				}
			} else if(rv->type == RLIB_REPORT_VARIABLE_SUM) {
				val = RLIB_VALUE_GET_AS_NUMBER(amount);
			} else if(rv->type == RLIB_REPORT_VARIABLE_AVERAGE) {
				val = rlib_fxp_div(RLIB_VALUE_GET_AS_NUMBER(amount), RLIB_VALUE_GET_AS_NUMBER(count), RLIB_FXP_PRECISION);
			} else if(rv->type == RLIB_REPORT_VARIABLE_LOWEST) {
				val = RLIB_VALUE_GET_AS_NUMBER(amount);
			} else if(rv->type == RLIB_REPORT_VARIABLE_HIGHEST) {
				val = RLIB_VALUE_GET_AS_NUMBER(amount);
			}
		}
		return rlib_value_new(rval, RLIB_VALUE_NUMBER, TRUE, &val);
	} else if(o->type == OPERAND_IIF) {
		return rlib_value_new(rval, RLIB_VALUE_IIF, FALSE, o->value);
	}
	rlib_value_new(rval, RLIB_VALUE_ERROR, FALSE, NULL);
	return 0;
}

gint execute_pcode(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gboolean show_stack_errors) {
	gint i;
	for(i=0;i<code->count;i++) {
		if(code->instructions[i].instruction == PCODE_PUSH) {
			struct rlib_pcode_operand *o = code->instructions[i].value;
			struct rlib_value rval;
			rlib_value_stack_push(r, vs, rlib_operand_get_value(r, &rval, o, this_field_value));
		} else { /*Execute*/
			struct rlib_pcode_operator *o = code->instructions[i].value;
			if(o->execute != NULL) {
				if(o->execute(r, code, vs, this_field_value, o->user_data) == FALSE) {
					r_error(r, "Line: %d - PCODE Execution Error: [%s] %s Didn't Work \n",code->line_number, code->infix_string,o->tag);
					break;
				}
			}
		}
	}

	if(vs->count != 1 && show_stack_errors) {
		r_error(r, "PCODE Execution Error: Stack Elements %d != 1\n", vs->count);
		r_error(r, "Line: %d - PCODE Execution Error: [%s]\n", code->line_number,code->infix_string );
	}

	return TRUE;
}

struct rlib_value * rlib_execute_pcode(rlib *r, struct rlib_value *rval, struct rlib_pcode *code, struct rlib_value *this_field_value) {
	struct rlib_value_stack value_stack;

	if(code == NULL)
		return NULL;

	rlib_value_stack_init(&value_stack);
	execute_pcode(r, code, &value_stack, this_field_value, TRUE);
	*rval = *rlib_value_stack_pop(&value_stack);
	return rval;
}

gint rlib_execute_as_int(rlib *r, struct rlib_pcode *pcode, gint *result) {
	struct rlib_value val;
	gint isok = FALSE;

	*result = 0;
	if (pcode) {
		rlib_execute_pcode(r, &val, pcode, NULL);
		if (RLIB_VALUE_IS_NUMBER((&val))) {
			*result = RLIB_VALUE_GET_AS_NUMBER((&val)) / RLIB_DECIMAL_PRECISION;
			isok = TRUE;
		} else {
			const gchar *whatgot = "don't know";
			const gchar *gotval = "";
			if (RLIB_VALUE_IS_STRING((&val))) {
				whatgot = "string";
				gotval = RLIB_VALUE_GET_AS_STRING((&val));
			}
			r_error(r, "Expecting numeric value from pcode. Got %s=%s", whatgot, gotval);
			rlib_value_free(&val); /* We only free if it's not a number because numbers don't alloc anything */
		}

	}
	return isok;
}

gint rlib_execute_as_float(rlib *r, struct rlib_pcode *pcode, gfloat *result) {
	struct rlib_value val;
	gint isok = FALSE;

	*result = 0;
	if (pcode) {
		rlib_execute_pcode(r, &val, pcode, NULL);
		if (RLIB_VALUE_IS_NUMBER((&val))) {
			*result = (gdouble)RLIB_VALUE_GET_AS_NUMBER((&val)) / (gdouble)RLIB_DECIMAL_PRECISION;
			isok = TRUE;
		} else {
			const gchar *whatgot = "don't know";
			const gchar *gotval = "";
			if (RLIB_VALUE_IS_STRING((&val))) {
				whatgot = "string";
				gotval = RLIB_VALUE_GET_AS_STRING((&val));
			}
			r_error(r, "Expecting numeric value from pcode. Got %s=%s", whatgot, gotval);
		}
		rlib_value_free(&val);
	}
	return isok;
}


gint rlib_execute_as_boolean(rlib *r, struct rlib_pcode *pcode, gint *result) {
	return rlib_execute_as_int(r, pcode, result)? TRUE : FALSE;
}

gint rlib_execute_as_string(rlib *r, struct rlib_pcode *pcode, gchar *buf, gint buf_len) {
	struct rlib_value val;
	gint isok = FALSE;

	if (pcode) {
		rlib_execute_pcode(r, &val, pcode, NULL);
		if (RLIB_VALUE_IS_STRING((&val))) {
			if (RLIB_VALUE_GET_AS_STRING((&val)) != NULL) 
				strncpy(buf, RLIB_VALUE_GET_AS_STRING((&val)), buf_len);
			isok = TRUE;
		} else {
			r_error(r, "Expecting string value from pcode");
		}
		rlib_value_free(&val);
	}
	return isok;
}

gint rlib_execute_as_int_inlist(rlib *r, struct rlib_pcode *pcode, gint *result, const gchar *list[]) {
	struct rlib_value val;
	gint isok = FALSE;

	*result = 0;
	if (pcode) {
		rlib_execute_pcode(r, &val, pcode, NULL);
		if (RLIB_VALUE_IS_NUMBER((&val))) {
			*result = RLIB_VALUE_GET_AS_NUMBER((&val)) / RLIB_DECIMAL_PRECISION;
			isok = TRUE;
		} else if (RLIB_VALUE_IS_STRING((&val))) {
			gint i;
			gchar * str = RLIB_VALUE_GET_AS_STRING((&val));
			for (i = 0; list[i]; ++i) {
				if (g_ascii_strcasecmp(str, list[i])) {
					*result = i;
					isok = TRUE;
					break;
				}
			}
		} else {
			r_error(r, "Expecting number or specific string from pcode");
		}
		rlib_value_free(&val);
	}
	return isok;
}
