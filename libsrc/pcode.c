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
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>

#include "rlib-internal.h"
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

static void rlib_free_operand(rlib *r, struct rlib_pcode_operand *o);
static struct rlib_pcode *rlib_infix_to_pcode_multi(rlib *r, struct rlib_part *part, struct rlib_report *report, gchar *infix, gchar *delims, gchar **next, gint line_number, gboolean look_at_metadata);

DLL_EXPORT_SYM void rlib_pcode_free(rlib *r, struct rlib_pcode *code) {
	gint64 i = 0;

	if (code == NULL)
		return;

	for (i = 0; i < code->count; i++) {
		struct rlib_pcode_instruction *instruction = code->instructions[i];
		if (instruction->instruction == PCODE_PUSH) {
			if (instruction->value_allocated)
				rlib_free_operand(r, instruction->value);
		}
		g_free(instruction);
	}
	g_free(code->instructions);
	g_free(code);
}

struct rlib_operator_stack {
	gint64 count;
	gint64 pcount;
	struct rlib_pcode_operator *op[200];
};

static struct rlib_pcode_operator *rlib_find_operator(rlib *r, gchar *ptr, struct rlib_pcode *p, gint64 have_operand) {
	gint64 len = strlen(ptr);
	gint64 result;
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

/* This is must be called with the 3rd argument coming from rlib_new_pcode_instruction() */
gint64 rlib_pcode_add(rlib *r, struct rlib_pcode *p, struct rlib_pcode_instruction *i) {
	struct rlib_pcode_instruction **iptr = g_try_realloc(p->instructions, sizeof(struct rlib_pcode_instruction *) * (p->count + 1));

	if (iptr == NULL) {
		r_error(r, "rlib_pcode_add: Out of memory!\n");
		return -1;
	}

	p->instructions = iptr;
	p->instructions[p->count++] = i;
	return 0;
}

static struct rlib_pcode_instruction *rlib_new_pcode_instruction(gint64 instruction, gpointer value, gboolean allocated) {
	struct rlib_pcode_instruction *rpi = g_new0(struct rlib_pcode_instruction, 1);

	if (rpi == NULL)
		return NULL;

	rpi->instruction = instruction;
	rpi->value = value;
	rpi->value_allocated = allocated;
	return rpi;
}

gint rlib_vector_compare(rlib *r, GSList *vec1, GSList *vec2) {
	GSList *v1, *v2;
	gint retval = 0;

	if (g_slist_length(vec1) != g_slist_length(vec2))
		return -1;

	for (v1 = vec1, v2 = vec2; v1; v1 = v1->next, v2 = v2->next) {
		struct rlib_value val1, val2;
		struct rlib_value *pval1, *pval2;
		struct rlib_pcode *code1 = v1->data;
		struct rlib_pcode *code2 = v2->data;

		rlib_value_init(r, &val1);
		rlib_value_init(r, &val2);

		pval1 = rlib_execute_pcode(r, &val1, code1, NULL);
		pval2 = rlib_execute_pcode(r, &val2, code2, NULL);

		retval = rvalcmp(r, pval1, pval2);

		rlib_value_free(r, &val1);
		rlib_value_free(r, &val2);

		if (retval != 0)
			break;
	}

	return retval;
}

gint rvalcmp(rlib *r, struct rlib_value *v1, struct rlib_value *v2) {
	if (v1 == NULL || v2 == NULL)
		return -1;

	if (RLIB_VALUE_IS_NUMBER(r, v1) && RLIB_VALUE_IS_NUMBER(r, v2)) {
		if (mpfr_cmp(v1->mpfr_value, v2->mpfr_value) == 0)
			return 0;
		else
			return -1;
	}
	if (RLIB_VALUE_IS_STRING(r, v1) && RLIB_VALUE_IS_STRING(r, v2)) {
		if (RLIB_VALUE_GET_AS_STRING(r, v2) == NULL &&  RLIB_VALUE_GET_AS_STRING(r, v1) == NULL)
			return 0;
		else if (RLIB_VALUE_GET_AS_STRING(r, v2) == NULL ||  RLIB_VALUE_GET_AS_STRING(r, v1) == NULL)
			return -1;
		return r_strcmp(RLIB_VALUE_GET_AS_STRING(r, v1), RLIB_VALUE_GET_AS_STRING(r, v2));
	}
	if (RLIB_VALUE_IS_DATE(r, v1) && RLIB_VALUE_IS_DATE(r, v2)) {
		return rlib_datetime_compare(RLIB_VALUE_GET_AS_DATE(r, v1), RLIB_VALUE_GET_AS_DATE(r, v2));
	}
	if (RLIB_VALUE_IS_VECTOR(r, v1) && RLIB_VALUE_IS_VECTOR(r, v2)) {
		return rlib_vector_compare(r, RLIB_VALUE_GET_AS_VECTOR(r, v1), RLIB_VALUE_GET_AS_VECTOR(r, v2));
	}
	return -1;
}

struct rlib_pcode_operand *rlib_new_operand(rlib *r, struct rlib_part *part, struct rlib_report *report, gchar *str, gchar *infix, gint line_number, gboolean look_at_metadata) {
	gint64 resultset;
	gpointer field = NULL;
	gchar *memresult;
	struct rlib_pcode_operand *o;
	struct rlib_report_variable *rv;
	struct rlib_metadata *metadata;
	gint64 rvar;
	o = g_new0(struct rlib_pcode_operand, 1);
	if (str[0] == '\'') {
		gint64 slen;
		gchar *newstr;
		slen = strlen(str);
		if (slen < 2) {
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
	} else if (str[0] == '[') {
		GSList *vector = NULL;
		gchar *next = str + 1;
		str++;
		o->type = OPERAND_VECTOR;

		while (next && *next && *next != ']') {
			gchar *str1 = next;
			struct rlib_pcode *p = rlib_infix_to_pcode_multi(r, part, report, str1, ";]", &next, line_number, look_at_metadata);
			vector = g_slist_append(vector, p);
		}
		o->value = vector;
	} else if (!strcasecmp(str, "yes") || !strcasecmp(str, "true")) {
		mpfr_ptr newnum = g_malloc(sizeof(mpfr_t));
		mpfr_init2(newnum, r->numeric_precision_bits);
		o->type = OPERAND_NUMBER;
		mpfr_set_si(newnum, 1, MPFR_RNDN);
		o->value = newnum;
	} else if (!strcasecmp(str, "no") || !strcasecmp(str, "false")) {
		mpfr_ptr newnum = g_malloc(sizeof(mpfr_t));
		mpfr_init2(newnum, r->numeric_precision_bits);
		o->type = OPERAND_NUMBER;
		mpfr_set_si(newnum, 0, MPFR_RNDN);
		o->value = newnum;
	} else if (!g_ascii_strcasecmp(str, "left")) {
		mpfr_ptr newnum = g_malloc(sizeof(mpfr_t));
		mpfr_init2(newnum, r->numeric_precision_bits);
		o->type = OPERAND_NUMBER;
		mpfr_set_si(newnum, RLIB_ALIGN_LEFT, MPFR_RNDN);
		o->value = newnum;
	} else if (!g_ascii_strcasecmp(str, "right")) {
		mpfr_ptr newnum = g_malloc(sizeof(mpfr_t));
		mpfr_init2(newnum, r->numeric_precision_bits);
		o->type = OPERAND_NUMBER;
		mpfr_set_si(newnum, RLIB_ALIGN_RIGHT, MPFR_RNDN);
		o->value = newnum;
	} else if (!g_ascii_strcasecmp(str, "center")) {
		mpfr_ptr newnum = g_malloc(sizeof(mpfr_t));
		mpfr_init2(newnum, r->numeric_precision_bits);
		o->type = OPERAND_NUMBER;
		mpfr_set_si(newnum, RLIB_ALIGN_CENTER, MPFR_RNDN);
		o->value = newnum;
	} else if (!g_ascii_strcasecmp(str, "landscape")) {
		mpfr_ptr newnum = g_malloc(sizeof(mpfr_t));
		mpfr_init2(newnum, r->numeric_precision_bits);
		o->type = OPERAND_NUMBER;
		mpfr_set_si(newnum, RLIB_ORIENTATION_LANDSCAPE, MPFR_RNDN);
		o->value = newnum;
	} else if (!g_ascii_strcasecmp(str, "portrait")) {
		mpfr_ptr newnum = g_malloc(sizeof(mpfr_t));
		mpfr_init2(newnum, r->numeric_precision_bits);
		o->type = OPERAND_NUMBER;
		mpfr_set_si(newnum, RLIB_ORIENTATION_PORTRAIT, MPFR_RNDN);
		o->value = newnum;
	} else if (isdigit(*str) || (*str == '-') || (*str == '+') || (*str == '.')) {
		mpfr_ptr newnum = g_malloc0(sizeof(mpfr_t));
		o->type = OPERAND_NUMBER;
		mpfr_init2(newnum, r->numeric_precision_bits);
		mpfr_set_str(newnum, str, 10, MPFR_RNDN);
		o->value = newnum;
	} else if((rv = rlib_resolve_variable(r, part, report, str))) {
		o->type = OPERAND_VARIABLE;
		o->value = rv;
	} else if((memresult = rlib_resolve_memory_variable(r, str))) {
		o->type = OPERAND_MEMORY_VARIABLE;
		o->value = g_strdup(memresult);
	} else if((rvar = resolve_rlib_variable(str))) {
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
		gchar *newstr = g_malloc(r_strlen(err) + 1);
		strcpy(newstr, err);
		o->type = OPERAND_STRING;
		o->value = newstr;
		r_error(r, "Line: %d Unrecognized operand [%s]\n The Expression Was [%s]\n", line_number, str, infix);
	}
	return o;
}

static void rlib_free_operand(rlib *r, struct rlib_pcode_operand *o) {
	switch (o->type) {
	case OPERAND_STRING:
	case OPERAND_DATE:
	case OPERAND_MEMORY_VARIABLE:
	case OPERAND_FIELD:
		g_free(o->value);
		break;

	case OPERAND_NUMBER:
		mpfr_clear((mpfr_ptr)o->value);
		g_free(o->value);
		break;

	case OPERAND_IIF:
	{
		struct rlib_pcode_if *rpif = o->value;
		rlib_pcode_free(r, rpif->evaluation);
		rlib_pcode_free(r, rpif->true);
		rlib_pcode_free(r, rpif->false);
		g_free(rpif);
		break;
	}

	case OPERAND_VECTOR:
	{
		GSList *v = o->value;
		GSList *ptr;
		for (ptr = v; ptr; ptr = ptr->next) {
			struct rlib_pcode *op = ptr->data;
			rlib_pcode_free(r, op);
		}
		g_slist_free(v);
	}

	case OPERAND_VARIABLE:
	case OPERAND_RLIB_VARIABLE:
	case OPERAND_METADATA:
		break;

	case OPERAND_VALUE:
		rlib_value_free(r, o->value);
		g_free(o->value);
		break;

	default:
		r_error(r, "rlib_free_operand: unknown operand type: %d\n", o->type);
		break;
	}

	g_free(o);
}

static const gchar *rlib_rlib_variable_to_name(gint64 value) {
	switch (value) {
	case RLIB_RLIB_VARIABLE_PAGENO:
		return "pageno";
	case RLIB_RLIB_VARIABLE_TOTPAGES:
		return "totpages";
	case RLIB_RLIB_VARIABLE_VALUE:
		return "value";
	case RLIB_RLIB_VARIABLE_LINENO:
		return "lineno";
	case RLIB_RLIB_VARIABLE_DETAILCNT:
		return "detailcnt";
	case RLIB_RLIB_VARIABLE_FORMAT:
		return "format";
	default:
		return "unknown";
	}
}

void rlib_value_dump(rlib *r, struct rlib_value *rval, gint64 offset, gint64 linefeed) {
	int i;

	for (i = 0; i < offset * 5; i++)
		rlogit(r, " ");

	switch (rval->type) {
	case RLIB_VALUE_NUMBER:
		{
			char *tmp = NULL;
			mpfr_asprintf(&tmp, "%Rf", rval->mpfr_value);
			rlogit(r, "OPERAND_VALUE NUMBER %s", tmp);
			mpfr_free_str(tmp);
			break;
		}
	case RLIB_VALUE_STRING:
		rlogit(r, "OPERAND_VALUE STRING '%s'", rval->string_value);
		break;
	default:
		rlogit(r, "OPERAND_VALUE TYPE [%d]", rval->type);
		break;
	}
	if (linefeed)
		rlogit(r, "\n");
}

void rlib_pcode_dump(rlib *r, struct rlib_pcode *p, gint64 offset) {
	gint64 i,j;
	rlogit(r, "DUMPING PCODE IT HAS %d ELEMENTS\n", p->count);
	for (i = 0; i < p->count; i++) {
		for (j = 0; j < offset * 5; j++)
			rlogit(r, " ");
		rlogit(r, "DUMP: ");
		switch (p->instructions[i]->instruction) {
		case PCODE_PUSH:
		{
			struct rlib_pcode_operand *o = p->instructions[i]->value;
			rlogit(r, "PUSH: ");
			switch (o->type) {
			case OPERAND_NUMBER:
				rlogit(r, "%" PRId64, *((gint64 *)o->value));
				break;
			case OPERAND_STRING:
				rlogit(r, "'%s'", (char *)o->value);
				break;
			case OPERAND_FIELD:
			{
				struct rlib_resultset_field *rf = o->value;
				rlogit(r, "Result Set = [%d]; Field = [%d]", rf->resultset, GPOINTER_TO_INT(rf->field));
				break;
			}
			case OPERAND_METADATA:
			{
				struct rlib_metadata *metadata = o->value;
				rlogit(r, "METADATA ");
				rlib_pcode_dump(r, metadata->formula_code, offset+1);
				break;
			}
			case OPERAND_MEMORY_VARIABLE:
				rlogit(r, "Result Memory Variable = [%s]", (char *)o->value);
				break;
			case OPERAND_VARIABLE:
			{
				struct rlib_report_variable *rv = o->value;
				rlogit(r, "Result Variable = [%s]", rv->xml_name.xml);
				break;
			}
			case OPERAND_RLIB_VARIABLE:
				rlogit(r, "RLIB Variable = [%s]\n", rlib_rlib_variable_to_name(GPOINTER_TO_INT(o->value)));
				break;
			case OPERAND_IIF:
			{
				struct rlib_pcode_if *rpi = o->value;
				rlogit(r, "*IFF EXPRESSION EVALUATION:\n");
				rlib_pcode_dump(r, rpi->evaluation, offset+1);
				rlogit(r, "*IFF EXPRESSION TRUE:\n");
				rlib_pcode_dump(r, rpi->true, offset+1);
				rlogit(r, "*IFF EXPRESSION FALSE:\n");
				rlib_pcode_dump(r, rpi->false, offset+1);
				rlogit(r, "*IFF DONE\n");
				break;
			}
			case OPERAND_VALUE:
			{
				struct rlib_value *rval = o->value;
				rlib_value_dump(r, rval, 0, 0);
				break;
			}
			default:
				rlogit(r, "*UNKOWN EXPRESSION: %d\n", o->type);
			}
			break;
		}
		case PCODE_EXECUTE:
		{
			struct rlib_pcode_operator *o = p->instructions[i]->value;
			rlogit(r, "EXECUTE: %s", o->tag);
			break;
		}
		default:
			rlogit(r, "UNKNOWN INSTRUCTION TYPE %d\n", p->instructions[i]->instruction);
		}
		rlogit(r, "\n");
	}
}

gboolean rlib_pcode_has_variable(rlib *r UNUSED, struct rlib_pcode *p, GSList **varlist, GSList **varlist_nonrb, gboolean include_delayed_rlib_variables) {
	GSList *list = NULL;
	GSList *list_nonrb = NULL;
	struct rlib_report_variable *var;
	int i, count_vars, count_rvars;

	if (varlist)
		*varlist = NULL;

	if (p == NULL)
		return FALSE;

	count_vars = 0;
	count_rvars = 0;
	for (i = 0; i < p->count; i++) {
		switch (p->instructions[i]->instruction) {
		case PCODE_PUSH:
		{
			struct rlib_pcode_operand *o = p->instructions[i]->value;

			if (o == NULL) {
				r_error(r, "rlib_pcode_has_variable OPERAND IS NULL (p->count %d index %d)\n", p->count, i);
				continue;
			}

			switch (o->type) {
			case OPERAND_VARIABLE:
				var = o->value;

				if (var->precalculate) {
					if (var->resetonbreak) {
						if (varlist) {
							GSList *ptr;

							for (ptr = list; ptr; ptr = ptr->next) {
								if (ptr->data == var)
									break;
							}
							if (!ptr)
								list = g_slist_append(list, var);
						}
					} else {
						if (varlist_nonrb) {
							GSList *ptr;

							for (ptr = list_nonrb; ptr; ptr = ptr->next) {
								if (ptr->data == var)
									break;
							}
							if (!ptr)
								list_nonrb = g_slist_append(list_nonrb, var);
						}
					}
					count_vars++;
				}
				break;
			case OPERAND_RLIB_VARIABLE:
				if (include_delayed_rlib_variables) {
					if (GPOINTER_TO_INT(o->value) == RLIB_RLIB_VARIABLE_TOTPAGES)
						count_rvars++;
				}
				break;
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
	}

	if (varlist)
		*varlist = list;

	if (varlist_nonrb)
		*varlist_nonrb = list_nonrb;

	return count_vars + count_rvars;
}

const char *rlib_pcode_operand_name(gint64 type) {
	switch (type) {
	case OPERAND_NUMBER:
		return "OPERAND_NUMBER";
	case OPERAND_STRING:
		return "OPERAND_STRING";
	case OPERAND_DATE:
		return "OPERAND_DATE";
	case OPERAND_FIELD:
		return "OPERAND_FIELD";
	case OPERAND_VARIABLE:
		return "OPERAND_VARIABLE";
	case OPERAND_MEMORY_VARIABLE:
		return "OPERAND_MEMORY_VARIABLE";
	case OPERAND_RLIB_VARIABLE:
		return "OPERAND_RLIB_VARIABLE";
	case OPERAND_METADATA:
		return "OPERAND_METADATA";
	case OPERAND_IIF:
		return "OPERAND_IIF";
	case OPERAND_VECTOR:
		return "OPERAND_VECTOR";
	case OPERAND_VALUE:
		return "OPERAND_VALUE";
	default:
		return "UNKNOWN OPERAND";
	}
}

struct rlib_pcode *rlib_pcode_copy_replace_fields_and_immediates_with_values(rlib *r, struct rlib_pcode *p) {
	struct rlib_pcode *p1;
	int i, quit;

	if (p == NULL)
		return NULL;

	p1 = g_new0(struct rlib_pcode, 1);
	if (p1 == NULL)
		return NULL;

	p1->count = p->count;
	p1->line_number = p->line_number;

	p1->infix_string = p->infix_string;

	p1->instructions = g_new0(struct rlib_pcode_instruction *, p1->count);
	if (p1->instructions == NULL) {
		g_free(p1);
		return NULL;
	}

	for (i = 0, quit = 0; !quit && i < p1->count; i++) {
		p1->instructions[i] = g_new0(struct rlib_pcode_instruction, 1);
		if (p1->instructions[i] == NULL) {
			quit = 1;
			break;
		}

		p1->instructions[i]->instruction = p->instructions[i]->instruction;

		switch (p->instructions[i]->instruction) {
		case PCODE_PUSH:
			{
				struct rlib_pcode_operand *o = p->instructions[i]->value;

				switch (o->type) {
				case OPERAND_FIELD:
					{
						struct rlib_resultset_field *rf = o->value;
						gchar *field_value = rlib_resolve_field_value(r, rf);
						struct rlib_pcode_operand *o1;

						o1 = g_new0(struct rlib_pcode_operand, 1);
						if (o1 == NULL) {
							quit = 1;
							continue;
						}

						o1->type = OPERAND_STRING;
						o1->value = field_value;

						p1->instructions[i]->value = o1;
						p1->instructions[i]->value_allocated = TRUE;

						break;
					}
				case OPERAND_VARIABLE:
					{
						struct rlib_report_variable *rv = o->value;
						struct rlib_pcode_operand *o1;
						struct rlib_value *rval;

						if (rv->precalculate) {
							p1->instructions[i]->value = p->instructions[i]->value;
							break;
						}

						rval = g_new0(struct rlib_value, 1);

						rlib_operand_get_value(r, rval, o, NULL);

						o1 = g_new0(struct rlib_pcode_operand, 1);
						o1->type = OPERAND_VALUE;
						o1->value = rval;

						p1->instructions[i]->value = o1;
						p1->instructions[i]->value_allocated = TRUE;

						break;
					}
				case OPERAND_RLIB_VARIABLE:
					{
						gint64 rlib_vartype = ((long)o->value);

						if (rlib_vartype == RLIB_RLIB_VARIABLE_TOTPAGES) {
							p1->instructions[i]->value = p->instructions[i]->value;
							break;
						}
						/* fall through */
					}
				case OPERAND_MEMORY_VARIABLE:
					{
						struct rlib_pcode_operand *o1;
						struct rlib_value *rval;

						rval = g_new0(struct rlib_value, 1);

						rlib_operand_get_value(r, rval, o, NULL);

						o1 = g_new0(struct rlib_pcode_operand, 1);
						o1->type = OPERAND_VALUE;
						o1->value = rval;

						p1->instructions[i]->value = o1;
						p1->instructions[i]->value_allocated = TRUE;

						break;
					}
				default:
					/* copy the new operand as is, with value_allocated = FALSE */
					p1->instructions[i]->value = p->instructions[i]->value;
					break;
				}

				break;
			}
		case PCODE_EXECUTE:
			/* copy the new instruction as is, with value_allocated = FALSE */
			p1->instructions[i]->value = p->instructions[i]->value;
			break;
		default:
			r_error(r, "rlib_pcode_copy_replace_fields_with_values: Unknown instruction %d\n", p->instructions[i]->instruction);
			quit = 1;
			continue;
		}
	}

	if (quit) {
		p1->count = i;
		rlib_pcode_free(r, p1);
		return NULL;
	}

	return p1;
}

void rlib_pcode_replace_variable_with_value(rlib *r, struct rlib_pcode *p, struct rlib_report_variable *var) {
	int i;

	if (p == NULL || var == NULL)
		return;

	for (i = 0; i < p->count; i++) {
		switch (p->instructions[i]->instruction) {
		case PCODE_PUSH:
			{
				struct rlib_pcode_operand *o = p->instructions[i]->value;

				switch (o->type) {
				case OPERAND_VARIABLE:
					{
						struct rlib_report_variable *v = o->value;

						if (var == v) {
							struct rlib_value *rval = g_new0(struct rlib_value, 1);

							rlib_operand_get_value(r, rval, o, NULL);

							if (p->instructions[i]->value_allocated)
								rlib_free_operand(r, o);

							o = g_new0(struct rlib_pcode_operand, 1);
							o->type = OPERAND_VALUE;
							o->value = rval;
							p->instructions[i]->value = o;
							p->instructions[i]->value_allocated = TRUE;

							break;
						}

						break;
					}
				default:
					break;
				}

				break;
			}
		case PCODE_EXECUTE:
			break;
		default:
			r_error(r, "rlib_pcode_copy_replace_fields_with_values: Unknown instruction %d\n", p->instructions[i]->instruction);
			continue;
		}
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

gint64 operator_stack_is_all_less(struct rlib_operator_stack *os, struct rlib_pcode_operator *op) {
	gint64 i;
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

void popopstack(rlib *r, struct rlib_pcode *p, struct rlib_operator_stack *os, struct rlib_pcode_operator *op) {
	struct rlib_pcode_operator *o;
	if (op != NULL && (op->tag[0] == ')' || op->tag[0] == ',')) {
		while ((o = operator_stack_peek(os))) {
			if (o->is_op == TRUE) {
				if (op->tag[0] != ',') {
					rlib_pcode_add(r, p, rlib_new_pcode_instruction(PCODE_EXECUTE, o, FALSE));
				} else {
					if (o->is_function == FALSE) {
						rlib_pcode_add(r, p, rlib_new_pcode_instruction(PCODE_EXECUTE, o, FALSE));
					}
				}
			}
			if (o->tag[0] == '(' || o->is_function == TRUE) {
				if (op->tag[0] != ',') /* NEED TO PUT THE ( or FUNCTION back on the stack cause it takes more then 1 paramater */
					operator_stack_pop(os);
				break;
			}
			operator_stack_pop(os);
		}
	} else {
//r_error(NULL, "#################### OP IS [%s]\n", op->tag);
		while ((o = operator_stack_peek(os))) {
			if (o->tag[0] == '(' || o->is_function == TRUE) {
				break;
			}
			if (o->is_op == TRUE) {
//r_error(NULL, "@@@@@@@@@@@@@@@@@@@ ADDING [%s]\n", o->tag);
				rlib_pcode_add(r, p, rlib_new_pcode_instruction(PCODE_EXECUTE, o, FALSE));
			}
			operator_stack_pop(os);
		}
	}
}

static void forcepopstack(rlib *r, struct rlib_pcode *p, struct rlib_operator_stack *os) {
	struct rlib_pcode_operator *o;
	while ((o = operator_stack_pop(os))) {
		if (o->is_op == TRUE) {
//r_error(r, "forcepopstack:: Forcing [%s] Off the Stack\n", o->tag);
			rlib_pcode_add(r, p, rlib_new_pcode_instruction(PCODE_EXECUTE, o, FALSE));
		}
	}
}

void smart_add_pcode(rlib *r, struct rlib_pcode *p, struct rlib_operator_stack *os, struct rlib_pcode_operator *op) {
//r_error(NULL, "smart_add_pcode:: Adding [%s]\n", op->tag);

	if(operator_stack_is_all_less(os, op)) {
//r_error(NULL, "\tsmart_add_pcode:: JUST PUSH [%s]\n", op->tag);
		operator_stack_push(os, op);
	} else {
//r_error(NULL, "\tsmart_add_pcode:: POP AND PUSH [%s]\n", op->tag);
		popopstack(r, p, os, op);
		operator_stack_push(os, op);
	}
}

static gchar *skip_next_closing_paren(gchar *str) {
	gint64 ch;

	while ((ch = *str) && (ch != ')'))
		if (ch == '(') str = skip_next_closing_paren(str + 1);
		else ++str;
	return (ch == ')')? str + 1 : str;
}

static struct rlib_pcode *rlib_infix_to_pcode_multi(rlib *r, struct rlib_part *part, struct rlib_report *report, gchar *infix, gchar *delims, gchar **next, gint line_number, gboolean look_at_metadata) {
	gchar *moving_ptr = infix;
	gchar *op_pointer = infix;
	GString *operand;
	gint64 found_op_last = FALSE;
	gint64 last_op_was_function = FALSE;
	gint64 move_pointers = TRUE;
	gint64 instr = 0;
	gint64 indate = 0;
	gint64 invector = 0;
	struct rlib_pcode_operator *op;
	struct rlib_pcode *pcodes;
	struct rlib_operator_stack os;

	if(infix == NULL || infix[0] == '\0' || !strcmp(infix, ""))
		return NULL;

	pcodes = g_new0(struct rlib_pcode, 1);

	rlib_pcode_init(pcodes);
	pcodes->infix_string = infix;
	pcodes->line_number = line_number;

	operator_stack_init(&os);
	rmwhitespacesexceptquoted(infix);

	while(*moving_ptr) {
		/* Skip irrelevant spaces */
		if(!instr && !indate && !invector && isspace(*moving_ptr))
			moving_ptr++;

		if(*moving_ptr == '\'') {
			if(instr) {
				instr = 0;
				moving_ptr++;
				if(!*moving_ptr)
					break;
			} else
				instr = 1;
		}
		if(!instr && *moving_ptr == '{') {
			indate = 1;
		}
		if(indate && *moving_ptr == '}') {
			indate = 0;
			moving_ptr++;
			if(!*moving_ptr)
				break;
		}
		if(!instr && *moving_ptr == '[') {
			invector = 1;
		}
		if(invector && *moving_ptr == ']') {
			invector = 0;
			moving_ptr++;
			if (!*moving_ptr)
				break;
		}

		if (!instr && !indate && !invector && delims) {
			gchar *delim = delims;
			int found = 0;

			for (delim = delims; *delim; delim++) {
				if (*moving_ptr == *delim) {
					found = 1;
					break;
				}
			}

			if (found) {
				if (next)
					*next = moving_ptr + 1;
				break;
			}
		}

		if(!instr && !indate && !invector && (op = rlib_find_operator(r, moving_ptr, pcodes, moving_ptr != op_pointer))) {
			if(moving_ptr != op_pointer) {
				operand = g_string_new_len(op_pointer, moving_ptr - op_pointer + 1);
				operand->str[moving_ptr - op_pointer] = '\0';
				if (operand->str[0] != ')') {
					struct rlib_pcode_operand *op = rlib_new_operand(r, part, report, operand->str, infix, line_number, look_at_metadata);
					rlib_pcode_add(r, pcodes, rlib_new_pcode_instruction(PCODE_PUSH, op, TRUE));
				}
				g_string_free(operand, TRUE);
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
				if (op->opnum == OP_IIF) {
					gint64 in_a_string = FALSE;
					gint64 pcount=1;
					gint64 ccount=0;
					gchar *save_ptr, *iif, *save_iif;
					gchar *evaluation, *true=NULL, *false=NULL;
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
					evaluation = iif;
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
					rpif = g_new0(struct rlib_pcode_if, 1);
					rpif->evaluation = rlib_infix_to_pcode(r, part, report, evaluation, line_number, look_at_metadata);
					rpif->true = rlib_infix_to_pcode(r, part, report, true, line_number, look_at_metadata);
					rpif->false = rlib_infix_to_pcode(r, part, report, false, line_number, look_at_metadata);
					rpif->str_ptr = iif;
					smart_add_pcode(r, pcodes, &os, op);
					o = g_malloc(sizeof(struct rlib_pcode_operand));
					o->type = OPERAND_IIF;
					o->value = rpif;
					rlib_pcode_add(r, pcodes, rlib_new_pcode_instruction(PCODE_PUSH, o, TRUE));
					if (1) {
						struct rlib_pcode_operator *ox;
						ox = operator_stack_pop(&os);
						rlib_pcode_add(r, pcodes, rlib_new_pcode_instruction(PCODE_EXECUTE, ox, FALSE));
					}
					moving_ptr-=1;
					op_pointer = moving_ptr;
					move_pointers = FALSE;
					g_free(save_iif);
				} else {
					smart_add_pcode(r, pcodes, &os, op);
					last_op_was_function = op->is_function;
					if (op->tag[0] == ')')
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
	if ((moving_ptr != op_pointer)) {
		operand = g_string_new_len(op_pointer, moving_ptr - op_pointer + 1);
		operand->str[moving_ptr - op_pointer] = '\0';
		if (operand->str[0] != ')') {
			struct rlib_pcode_operand *op = rlib_new_operand(r, part, report, operand->str, infix, line_number, look_at_metadata);
			struct rlib_pcode_instruction *in = rlib_new_pcode_instruction(PCODE_PUSH, op, TRUE);
			rlib_pcode_add(r, pcodes, in);
		}
		g_string_free(operand, TRUE);
	}
	forcepopstack(r, pcodes, &os);
	if(os.pcount != 0) {
		r_error(r, "Line: %d Compiler Error.  Parenthesis Mismatch [%s]\n", line_number, infix);
	}

	return pcodes;
}

DLL_EXPORT_SYM struct rlib_pcode * rlib_infix_to_pcode(rlib *r, struct rlib_part *part, struct rlib_report *report, gchar *infix, gint line_number, gboolean look_at_metadata) {
	return rlib_infix_to_pcode_multi(r, part, report, infix, NULL, NULL, line_number, look_at_metadata);
}

void rlib_value_stack_init(struct rlib_value_stack *vs) {
	vs->count = 0;
}

DLL_EXPORT_SYM gboolean rlib_value_stack_push(rlib *r, struct rlib_value_stack *vs, struct rlib_value *value) {
	if (vs->count == 99)
		return FALSE;
	if (value == NULL) {
		r_error(r, "PCODE EXECUTION ERROR.. TRIED TO *PUSH* A NULL VALUE.. CHECK YOUR EXPRESSION!\n");
		return FALSE;
	}

	vs->values[vs->count++] = *value;

	return TRUE;
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_stack_pop(rlib *r UNUSED, struct rlib_value_stack *vs) {
	if(vs->count <= 0) {
		vs->values[0].type = RLIB_VALUE_NONE;
		return &vs->values[0];
	} else {
		return &vs->values[--vs->count];
	}
}

struct rlib_value *rlib_value_new(struct rlib_value *rval, gint64 type, gint64 free_, gpointer value) {
	rval->type = type;
	rval->free = free_;

	if (type == RLIB_VALUE_NUMBER) {
		memcpy(&rval->mpfr_value, value, sizeof(mpfr_t));
	} if (type == RLIB_VALUE_STRING)
		rval->string_value = value;
	if (type == RLIB_VALUE_DATE)
		rval->date_value = *((struct rlib_datetime *) value);
	if (type == RLIB_VALUE_IIF)
		rval->iif_value = value;
	if (type == RLIB_VALUE_VECTOR)
		rval->vector_value = value;

	return rval;
}

gboolean rlib_value_is_empty(rlib *r, struct rlib_value *rval) {
	if (rval == NULL)
		return TRUE;

	if (RLIB_VALUE_IS_STRING(r, rval) && strcmp(RLIB_VALUE_GET_AS_STRING(r, rval), "") == 0)
		return TRUE;

	return FALSE;
}

struct rlib_value *rlib_value_dup_contents(rlib *r, struct rlib_value *rval) {
	switch (rval->type) {
	case RLIB_VALUE_STRING:
		rval->string_value = g_strdup(rval->string_value);
		rval->free = TRUE;
		break;
	case RLIB_VALUE_NUMBER:
		{
			mpfr_t newval;
			mpfr_init2(newval, r->numeric_precision_bits);
			mpfr_set(newval, rval->mpfr_value, MPFR_RNDN);
			memcpy(rval->mpfr_value, newval, sizeof(mpfr_t));
		}
		rval->free = TRUE;
		break;
	default:
		break;
	}
	return rval;
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_alloc(rlib *r UNUSED) {
	struct rlib_value *rval = g_new0(struct rlib_value, 1);

	if (rval == NULL)
		return NULL;

	rval->alloc = TRUE;
	return rval;
}

DLL_EXPORT_SYM void rlib_value_init(rlib *r UNUSED, struct rlib_value *rval) {
	rval->alloc = FALSE;
	rval->type = RLIB_VALUE_NONE;
}

DLL_EXPORT_SYM void rlib_value_free(rlib *r UNUSED, struct rlib_value *rval) {
	if (rval == NULL)
		return;
	if (rval->free == FALSE)
		return;

	switch (rval->type) {
	case RLIB_VALUE_NUMBER:
		mpfr_clear(rval->mpfr_value);
		rval->free = FALSE;
		break;
	case RLIB_VALUE_STRING:
		g_free(rval->string_value);
		rval->free = FALSE;
		break;
	case RLIB_VALUE_VECTOR:
		g_slist_free(rval->vector_value);
		rval->free = FALSE;
		break;
	}

	rval->type = RLIB_VALUE_NONE;
	if (rval->alloc)
		g_free(rval);
}

DLL_EXPORT_SYM gint rlib_value_get_type(rlib *r UNUSED, struct rlib_value *rval) {
	return rval->type;
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_new_number_from_mpfr(rlib *r, struct rlib_value *rval, mpfr_ptr value) {
	rval->type = RLIB_VALUE_NUMBER;
	rval->free = TRUE;

	mpfr_init2(rval->mpfr_value, r->numeric_precision_bits);
	mpfr_set(rval->mpfr_value, value, MPFR_RNDN);

	return rval;
}

DLL_EXPORT_SYM mpfr_srcptr rlib_value_get_as_mpfr(rlib *r UNUSED, struct rlib_value *rval) {
	if (rval->type != RLIB_VALUE_NUMBER)
		return NULL;

	return rval->mpfr_value;
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_new_number_from_long(rlib *r, struct rlib_value *rval, glong value) {
	rval->type = RLIB_VALUE_NUMBER;
	rval->free = TRUE;

	mpfr_init2(rval->mpfr_value, r->numeric_precision_bits);
	mpfr_set_si(rval->mpfr_value, value, MPFR_RNDN);

	return rval;
}

DLL_EXPORT_SYM long rlib_value_get_as_long(rlib *r UNUSED, struct rlib_value *rval) {
	if (rval->type != RLIB_VALUE_NUMBER)
		return 0L;

	return mpfr_get_si(rval->mpfr_value, MPFR_RNDN);
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_new_number_from_int64(rlib *r, struct rlib_value *rval, gint64 value) {
	rval->type = RLIB_VALUE_NUMBER;
	rval->free = TRUE;

	mpfr_init2(rval->mpfr_value, r->numeric_precision_bits);
	mpfr_set_sj(rval->mpfr_value, value, MPFR_RNDN);

	return rval;
}

DLL_EXPORT_SYM gint64 rlib_value_get_as_int64(rlib *r UNUSED, struct rlib_value *rval) {
	if (rval->type != RLIB_VALUE_NUMBER)
		return 0LL;

	return mpfr_get_sj(rval->mpfr_value, MPFR_RNDN);
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_new_number_from_double(rlib *r, struct rlib_value *rval, gdouble value) {
	rval->type = RLIB_VALUE_NUMBER;
	rval->free = TRUE;

	mpfr_init2(rval->mpfr_value, r->numeric_precision_bits);
	mpfr_set_d(rval->mpfr_value, value, MPFR_RNDN);

	return rval;
}

DLL_EXPORT_SYM gdouble rlib_value_get_as_double(rlib *r UNUSED, struct rlib_value *rval) {
	if (rval->type != RLIB_VALUE_NUMBER)
		return 0.0;

	return mpfr_get_d(rval->mpfr_value, MPFR_RNDN);
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_new_none(rlib *r UNUSED, struct rlib_value *rval) {
	return rlib_value_new(rval, RLIB_VALUE_NONE, FALSE, NULL);
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_new_string(rlib *r UNUSED, struct rlib_value *rval, const gchar *value) {
	return rlib_value_new(rval, RLIB_VALUE_STRING, TRUE, g_strdup(value));
}

DLL_EXPORT_SYM gchar *rlib_value_get_as_string(rlib *r UNUSED, struct rlib_value *rval) {
	if (rval->type != RLIB_VALUE_STRING)
		return NULL;
	return rval->string_value;
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_new_date(rlib *r UNUSED, struct rlib_value *rval, struct rlib_datetime *date) {
	return rlib_value_new(rval, RLIB_VALUE_DATE, FALSE, date);
}

DLL_EXPORT_SYM struct rlib_datetime *rlib_value_get_as_date(rlib *r UNUSED, struct rlib_value *rval) {
	if (rval->type != RLIB_VALUE_DATE)
		return NULL;
	return &rval->date_value;
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_new_vector(rlib *r UNUSED, struct rlib_value *rval, GSList *vector) {
	return rlib_value_new(rval, RLIB_VALUE_VECTOR, TRUE, vector);
}

DLL_EXPORT_SYM GSList *rlib_value_get_as_vector(rlib *r UNUSED, struct rlib_value *rval) {
	if (rval->type != RLIB_VALUE_VECTOR)
		return NULL;
	return rval->vector_value;
}

DLL_EXPORT_SYM struct rlib_value *rlib_value_new_error(rlib *r UNUSED, struct rlib_value *rval) {
	return rlib_value_new(rval, RLIB_VALUE_ERROR, FALSE, NULL);
}

/*
 * The RLIB SYMBOL TABLE is a bit complicated because of all the datasources and internal variables
 */
struct rlib_value *rlib_operand_get_value(rlib *r, struct rlib_value *rval, struct rlib_pcode_operand *o, struct rlib_value *this_field_value) {
	struct rlib_report_variable *rv = NULL;

	if (o->type == OPERAND_NUMBER) {
		return rlib_value_new(rval, RLIB_VALUE_NUMBER, FALSE, o->value);
	} else if (o->type == OPERAND_STRING) {
		return rlib_value_new(rval, RLIB_VALUE_STRING, FALSE, o->value);
	} else if (o->type == OPERAND_DATE) {
		return rlib_value_new(rval, RLIB_VALUE_DATE, FALSE, o->value);
	} else if (o->type == OPERAND_FIELD) {
		struct rlib_resultset_field *rf = o->value;
		struct rlib_results *rs = r->results[rf->resultset];
		gchar *field_value;

		if (r->use_cached_data) {
			struct rlib_value *rval2 = g_hash_table_lookup(rs->cached_values, rf->field);

			if (rval2) {
				/*
				 * Cached value was found but don't let
				 * the caller free this data.
				 */
				*rval = *rval2;
				rval->free = FALSE;

				return rval;
			}
		}

		field_value = rlib_resolve_field_value(r, rf);
		rval = rlib_value_new(rval, RLIB_VALUE_STRING, TRUE, field_value);

		return rval;
	} else if (o->type == OPERAND_METADATA) {
		struct rlib_metadata *metadata = o->value;
		*rval = metadata->rval_formula;
		return rval;
	} else if (o->type == OPERAND_MEMORY_VARIABLE) {
		return rlib_value_new(rval, RLIB_VALUE_STRING, FALSE, o->value);
	} else if (o->type == OPERAND_RLIB_VARIABLE) {
		gint64 type = GPOINTER_TO_SIZE(o->value);
		switch (type) {
		case RLIB_RLIB_VARIABLE_PAGENO:
			return rlib_value_new_number_from_long(r, rval, r->current_page_number);
		case  RLIB_RLIB_VARIABLE_TOTPAGES:
			return rlib_value_new_number_from_long(r, rval, r->current_page_number);
		case RLIB_RLIB_VARIABLE_VALUE:
			return this_field_value;
		case RLIB_RLIB_VARIABLE_LINENO:
			return rlib_value_new_number_from_long(r, rval, r->current_line_number);
		case RLIB_RLIB_VARIABLE_DETAILCNT:
			return rlib_value_new_number_from_long(r, rval, r->detail_line_count);
		case RLIB_RLIB_VARIABLE_FORMAT:
			return rlib_value_new_string(r, rval, rlib_format_get_name(r->format));
		}
	} else if (o->type == OPERAND_VARIABLE) {
		mpfr_t val;
		struct rlib_value *count;
		struct rlib_value *amount;

		mpfr_init2(val, r->numeric_precision_bits);

		rv = o->value;
		count = &rv->count;
		amount = &rv->amount;

		if (rv->code == NULL && rv->type != RLIB_REPORT_VARIABLE_COUNT)
			r_error(r, "Line: %d - Bad Expression in variable [%s] Variable Resolution: Assuming 0 value for variable\n", rv->xml_name.line, rv->xml_name.xml);
		else {
			switch (rv->type) {
			case RLIB_REPORT_VARIABLE_COUNT:
			case RLIB_REPORT_VARIABLE_SUM:
			case RLIB_REPORT_VARIABLE_LOWEST:
			case RLIB_REPORT_VARIABLE_HIGHEST:
				mpfr_set(val, count->mpfr_value, MPFR_RNDN);
				break;
			case RLIB_REPORT_VARIABLE_AVERAGE:
				mpfr_div(val, amount->mpfr_value, count->mpfr_value, MPFR_RNDN);
				break;
			case RLIB_REPORT_VARIABLE_EXPRESSION:
				if (RLIB_VALUE_IS_ERROR(r, amount) || RLIB_VALUE_IS_NONE(r, amount)) {
					mpfr_set_si(val, 0L, MPFR_RNDN);
					r_error(r, "Variable Resolution: Assuming 0 value because rval is ERROR or NONE\n");
				} else  if (RLIB_VALUE_IS_STRING(r, amount)) {
					gchar *strval = g_strdup(RLIB_VALUE_GET_AS_STRING(r, amount));
					return rlib_value_new(rval, RLIB_VALUE_STRING, TRUE, strval);
				} else {
					mpfr_set(val, amount->mpfr_value, MPFR_RNDN);
				}
				break;
			}
		}
		return rlib_value_new(rval, RLIB_VALUE_NUMBER, TRUE, val);
	} else if (o->type == OPERAND_IIF) {
		return rlib_value_new(rval, RLIB_VALUE_IIF, FALSE, o->value);
	} else if (o->type == OPERAND_VECTOR) {
		return rlib_value_new(rval, RLIB_VALUE_VECTOR, FALSE, o->value);
	} else if (o->type == OPERAND_VALUE) {
		struct rlib_value *oval = o->value;
		*rval = *oval;
		rval->free = FALSE;
		return rval;
	}

	rlib_value_new(rval, RLIB_VALUE_ERROR, FALSE, NULL);
	return NULL;
}

gboolean execute_pcode(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gboolean show_stack_errors) {
	gint64 i;
	for (i = 0; i < code->count; i++) {
		switch (code->instructions[i]->instruction) {
		case PCODE_PUSH:
			{
				struct rlib_pcode_operand *o = code->instructions[i]->value;
				struct rlib_value rval;
				rlib_value_init(r, &rval);
				rlib_value_stack_push(r, vs, rlib_operand_get_value(r, &rval, o, this_field_value));
				break;
			}
		case PCODE_EXECUTE:
			{
				struct rlib_pcode_operator *o = code->instructions[i]->value;
				if (o->execute != NULL) {
					if (o->execute(r, code, vs, this_field_value, o->user_data) == FALSE) {
						r_error(r, "Line: %d - PCODE Execution Error: [%s] %s Didn't Work \n", code->line_number, code->infix_string, o->tag);
						break;
					}
				}
				break;
			}
		default:
			r_error(r, "execute_pcode: Invalid instruction %d\n", code->line_number);
			break;
		}
	}

	if (vs->count != 1 && show_stack_errors) {
		r_error(r, "PCODE Execution Error: Stack Elements %d != 1\n", vs->count);
		r_error(r, "Line: %d - PCODE Execution Error: [%s]\n", code->line_number, code->infix_string);
	}

	return TRUE;
}

DLL_EXPORT_SYM struct rlib_value *rlib_execute_pcode(rlib *r, struct rlib_value *rval, struct rlib_pcode *code, struct rlib_value *this_field_value) {
	struct rlib_value_stack value_stack;

	if (code == NULL)
		return NULL;

	rlib_value_stack_init(&value_stack);
	execute_pcode(r, code, &value_stack, this_field_value, TRUE);
	*rval = *rlib_value_stack_pop(r, &value_stack);

	return rval;
}

gboolean rlib_execute_as_int64(rlib *r, struct rlib_pcode *pcode, gint64 *result) {
	struct rlib_value rval;
	gboolean isok = FALSE;

	*result = 0L;
	if (!pcode)
		return isok;

	rlib_value_init(r, &rval);
	rlib_execute_pcode(r, &rval, pcode, NULL);
	if (RLIB_VALUE_IS_NUMBER(r, (&rval))) {
		*result = mpfr_get_sj(rval.mpfr_value, MPFR_RNDN);
		rlib_value_free(r, &rval);
		isok = TRUE;
	} else {
		const gchar *whatgot = "don't know";
		const gchar *gotval = "";
		if (RLIB_VALUE_IS_STRING(r, (&rval))) {
			whatgot = "string";
			gotval = RLIB_VALUE_GET_AS_STRING(r, &rval);
		}
		r_error(r, "Expecting numeric value from pcode. Got %s=%s", whatgot, gotval);
		rlib_value_free(r, &rval);
	}

	return isok;
}

gboolean rlib_execute_as_double(rlib *r, struct rlib_pcode *pcode, gdouble *result) {
	struct rlib_value rval;
	gint64 isok = FALSE;

	*result = 0;
	if (!pcode)
		return isok;

	rlib_value_init(r, &rval);
	rlib_execute_pcode(r, &rval, pcode, NULL);
	if (RLIB_VALUE_IS_NUMBER(r, (&rval))) {
		*result = mpfr_get_d(rval.mpfr_value, MPFR_RNDN);
		isok = TRUE;
	} else {
		const gchar *whatgot = "don't know";
		const gchar *gotval = "";
		if (RLIB_VALUE_IS_STRING(r, (&rval))) {
			whatgot = "string";
			gotval = RLIB_VALUE_GET_AS_STRING(r, &rval);
		}
		r_error(r, "Expecting numeric value from pcode. Got %s=%s", whatgot, gotval);
	}
	rlib_value_free(r, &rval);

	return isok;
}

gboolean rlib_execute_as_boolean(rlib *r, struct rlib_pcode *pcode, gboolean *result) {
	gint64 res1 = 0;
	gboolean retval = rlib_execute_as_int64(r, pcode, &res1);
	*result = !!res1;
	return retval;
}

gboolean rlib_execute_as_string(rlib *r, struct rlib_pcode *pcode, gchar *buf, gint64 buf_len) {
	struct rlib_value rval;
	gboolean isok = FALSE;

	if (!pcode)
		return isok;

	rlib_value_init(r, &rval);
	rlib_execute_pcode(r, &rval, pcode, NULL);
	if (RLIB_VALUE_IS_STRING(r, (&rval))) {
		if (RLIB_VALUE_GET_AS_STRING(r, &rval) != NULL)
			strncpy(buf, RLIB_VALUE_GET_AS_STRING(r, &rval), buf_len);
		isok = TRUE;
	} else {
		r_error(r, "Expecting string value from pcode");
	}
	rlib_value_free(r, &rval);

	return isok;
}

gboolean rlib_execute_as_int64_inlist(rlib *r, struct rlib_pcode *pcode, gint64 *result, const gchar *list[]) {
	struct rlib_value rval;
	gboolean isok = FALSE;

	*result = 0;
	if (!pcode)
		return isok;

	rlib_value_init(r, &rval);
	rlib_execute_pcode(r, &rval, pcode, NULL);
	if (RLIB_VALUE_IS_NUMBER(r, (&rval))) {
		*result = mpfr_get_sj(rval.mpfr_value, MPFR_RNDN);
		isok = TRUE;
	} else if (RLIB_VALUE_IS_STRING(r, (&rval))) {
		gint64 i;
		gchar * str = RLIB_VALUE_GET_AS_STRING(r, &rval);
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
	rlib_value_free(r, &rval);

	return isok;
}
