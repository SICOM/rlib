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

#define RLIB_FXP_MUL(a, b) rlib_fxp_mul(a, b, RLIB_DECIMAL_PRECISION)
#define RLIB_FXP_DIV(num, denom) rlib_fxp_div(num, denom, RLIB_FXP_PRECISION)

#define RLIB_FXP_TO_NORMAL_LONG_LONG(a) ((gint64)(a/RLIB_DECIMAL_PRECISION))
#define LONG_TO_FXP_NUMBER(a) ((a)*RLIB_DECIMAL_PRECISION)

#define OP_ADD		1
#define OP_SUB 	2
#define OP_MUL 	3
#define OP_DIV 	4
#define OP_MOD 	5
#define OP_POW 	6
#define OP_LT		7
#define OP_LTE 	8
#define OP_GT		9
#define OP_GTE 	10
#define OP_EQL 	11
#define OP_NOTEQL	12
#define OP_AND		13
#define OP_OR		14
#define OP_LPERN	15
#define OP_RPERN	16
#define OP_COMMA	17
#define OP_ABS 	18
#define OP_CEIL	19
#define OP_FLOOR	20
#define OP_ROUND	21
#define OP_SIN 	22
#define OP_COS 	23
#define OP_LN		24
#define OP_EXP 	25
#define OP_ATAN	26
#define OP_SQRT	27
#define OP_FXPVAL	28
#define OP_VAL 	29
#define OP_STR 	30
#define OP_STOD 	31
#define OP_IIF 	32
#define OP_DTOS 	33
#define OP_YEAR 	34
#define OP_MONTH 	35
#define OP_DAY 	36
#define OP_UPPER 	37
#define OP_LOWER 	38
#define OP_PROPER	39
#define OP_STODS 	40
#define OP_ISNULL	41
#define OP_DIM 	42
#define OP_WIY 	43
#define OP_WIYO 	44
#define OP_DATE 	45
#define OP_LEFT	46
#define OP_RIGHT	47
#define OP_MID		48
#define OP_TSTOD	49
#define OP_DTOSF	50
#define OP_DATEOF	41
#define OP_TIMEOF	52
#define OP_CHGDATEOF	53
#define OP_CHGTIMEOF	54
#define OP_GETTIMESECS	55
#define OP_SETTIMESECS	56
#define OP_FORMAT		57
#define OP_STODSQL 	58
#define OP_EVAL 	59
#define OP_LOGICALAND 	60
#define OP_STRLEN 	61
#define OP_LOGICALOR 	62

#define PCODE_PUSH	1
#define PCODE_EXECUTE	2

struct rlib_pcode_instruction {
	gchar instruction;
	void *value;
};

struct rlib_pcode_operator {
	const gchar *tag;		/* What I expect to find in the infix string */
	gint taglen;
	gint precedence;
	gint is_op;
	gint opnum;
	gint is_function;
	gint	(*execute)(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *, struct rlib_value *this_field_value, gpointer user_data);
	gpointer user_data;
};

struct rlib_pcode_if {
	struct rlib_pcode *evaulation;
	struct rlib_pcode *true;
	struct rlib_pcode *false;
	char *str_ptr;
};

#define OPERAND_NUMBER          1
#define OPERAND_STRING          2
#define OPERAND_DATE            3
#define OPERAND_FIELD           4
#define OPERAND_VARIABLE        5
#define OPERAND_MEMORY_VARIABLE 6
#define OPERAND_RLIB_VARIABLE   7
#define OPERAND_METADATA        8
#define OPERAND_IIF             9

#define RLIB_RLIB_VARIABLE_PAGENO    1
#define RLIB_RLIB_VARIABLE_TOTPAGES  2
#define RLIB_RLIB_VARIABLE_VALUE     3
#define RLIB_RLIB_VARIABLE_LINENO    4
#define RLIB_RLIB_VARIABLE_DETAILCNT 5
#define RLIB_RLIB_VARIABLE_FORMAT    6


struct rlib_pcode_operand {
	char type;
	void *value;
};

#define RLIB_FXP_PRECISION 7

int execute_pcode(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gboolean show_stack_errors);
void rlib_pcode_dump(rlib *r, struct rlib_pcode *p, int offset);

int rlib_pcode_operator_multiply(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_add(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_subtract(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_divide(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_mod(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_pow(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_lt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_lte(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_gt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_gte(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_eql(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_noteql(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_and(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_logical_and(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_logical_or(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_or(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_abs(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_ceil(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_floor(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_round(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_sin(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_cos(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_ln(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_exp(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_atan(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_sqrt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_fxpval(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_val(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_str(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_stod(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_tstod(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_iif(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_dtos(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_dtosf(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_year(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_month(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_day(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_upper(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_lower(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_proper(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_stodt(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_isnull(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_dim(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_wiy(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_wiyo(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_date(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_left(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_right(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_substring(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_dateof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_timeof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_chgdateof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_chgtimeof(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_gettimeinsecs(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_settimeinsecs(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_format(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_stodtsql(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_eval(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
int rlib_pcode_operator_strlen(rlib *r, struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, gpointer user_data);
