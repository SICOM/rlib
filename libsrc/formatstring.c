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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <locale.h>

#ifdef RLIB_HAVE_MONETARY_H 
#include <monetary.h>
#endif

#include "rlib.h"
#include "pcode.h"
#include "rlib_langinfo.h"
#include "formatstring.h"

#define MAX_FORMAT_STRING 20

void change_radix_character(rlib *r, char *s) {
	if (r->radix_character == '.')
		return;
	while (*s != '\0') {
		if (*s == '.') {
			*s = r->radix_character;
		} else if (*s == r->radix_character) {
			*s = '.';
		}
		s++;
	}
}

/* Number formatstring flags */
#define RLIB_FMTSTR_FLAG_ALTERNATE			(0x01)	/* '#' */
#define RLIB_FMTSTR_FLAG_0PADDED			(0x02)	/* '0' */
#define RLIB_FMTSTR_FLAG_LEFTALIGN			(0x04)	/* '-' */
#define RLIB_FMTSTR_FLAG_SIGN_BLANK			(0x08)	/* ' ' */
#define	RLIB_FMTSTR_FLAG_SIGN				(0x10)	/* '+' */
#define RLIB_FMTSTR_FLAG_GROUPING			(0x20)	/* '\'' or '$' (legacy format) */
#define RLIB_FMTSTR_FLAG_ALTERNATE_DIGITS	(0x40)	/* 'I' */

/* Monetary formatstring flags */
#define RLIB_FMTSTR_MFLAG_FILLCHAR			(0x01)	/* '=f' */
#define RLIB_FMTSTR_MFLAG_NOGROUPING		(0x02)	/* '^' */
#define RLIB_FMTSTR_MFLAG_NEG_PAR			(0x04)	/* '(' */
#define RLIB_FMTSTR_MFLAG_NEG_SIGN			(0x08)	/* '+' */
#define RLIB_FMTSTR_MFLAG_OMIT_CURRENCY		(0x10)	/* '!' */
#define RLIB_FMTSTR_MFLAG_LEFTALIGN			(0x20)	/* '-' */

struct rlib_format_string_element_t {
	gint type;
	gint flags;
	gint length;
	gint prec;
	gchar conv;
	GString *string;
};

static gboolean good_conv(gchar c) {
	switch (c) {
	case 'd': /* legacy number */
	case 'a': /* various floating point conversions for the C format string */
	case 'A':
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
		return TRUE;
	default:
		return FALSE;
	}
}

static gboolean good_money_conv(gchar c) {
	switch (c) {
	case 'i': /* international monetary format */
	case 'n': /* national monetary format */
		return TRUE;
	default:
		return FALSE;
	}
}

static gint formatstring_flags(const gchar *fmt, gint *advance) {
	gint flags = 0;
	gint pos, quit = 0;

	for (pos = 0; !quit && fmt[pos]; pos++) {
		switch (fmt[pos]) {
		case '#':
			flags |= RLIB_FMTSTR_FLAG_ALTERNATE;
			break;
		case '0':
			flags |= RLIB_FMTSTR_FLAG_0PADDED;
			break;
		case '-':
			flags |= RLIB_FMTSTR_FLAG_LEFTALIGN;
			break;
		case ' ':
			flags |= RLIB_FMTSTR_FLAG_SIGN_BLANK;
			break;
		case '+':
			flags |= RLIB_FMTSTR_FLAG_SIGN;
		case '\'':
			flags |= RLIB_FMTSTR_FLAG_GROUPING;
			break;
		case 'I':
			flags |= RLIB_FMTSTR_FLAG_ALTERNATE_DIGITS;
			break;
		default:
			quit = 1;
			break;
		}
		if (quit)
			break;
	}
	*advance = pos;
	return flags;
}

static gint formatstring_flags_money(const gchar *fmt, gchar *filler, gint *advance) {
	gint flags = 0;
	gchar fill = '\0';
	gint pos, quit = 0;

	for (pos = 0; !quit && fmt[pos]; pos++) {
		switch (fmt[pos]) {
		case '=':
			if (!fmt[pos + 1]) {
				quit = 1;
				break;
			}
			pos++;
			flags |= RLIB_FMTSTR_MFLAG_FILLCHAR;
			fill = fmt[pos];
			break;
		case '^':
			flags |= RLIB_FMTSTR_MFLAG_NOGROUPING;
			break;
		case '(':
			flags |= RLIB_FMTSTR_MFLAG_NEG_PAR;
			break;
		case '+':
			flags |= RLIB_FMTSTR_MFLAG_NEG_SIGN;
			break;
		case '!':
			flags |= RLIB_FMTSTR_MFLAG_OMIT_CURRENCY;
		case '-':
			flags |= RLIB_FMTSTR_MFLAG_LEFTALIGN;
			break;
		default:
			quit = 1;
			break;
		}
		if (quit)
			break;
	}

	*filler = fill;
	*advance = pos;
	return flags;
}


static void formatstring_length_prec(const gchar *fmt, gint *length, gint *prec, gint *advance) {
	gint pos = 0;
	GString *tmp = g_string_new("");

	*length = 0;
	*prec = 0;

	for (pos = 0; fmt[pos] && isdigit(fmt[pos]); pos++)
		g_string_append_c(tmp, fmt[pos]);
	g_string_append_c(tmp, '\0');
	if (strlen(tmp->str) > 0)
		*length = atoi(tmp->str);

	if (fmt[pos] != '.') {
		g_string_free(tmp, TRUE);
		*advance = pos;
		return;
	}

	g_string_set_size(tmp, 0);
	for (pos++; fmt[pos] && isdigit(fmt[pos]); pos++)
		g_string_append_c(tmp, fmt[pos]);
	g_string_append_c(tmp, '\0');

	if (strlen(tmp->str) > 0)
		*prec = atoi(tmp->str);

	g_string_free(tmp, TRUE);
	*advance = pos;
	return;
}

static void formatstring_money_length_prec(const gchar *fmt, gint *length, gint *lprec, gint *prec, gint *advance) {
	gint pos = 0;
	GString *tmp = g_string_new("");

	*length = 0;
	*lprec = 0;
	*prec = 0;

	for (pos = 0; fmt[pos] && isdigit(fmt[pos]); pos++)
		g_string_append_c(tmp, fmt[pos]);
	g_string_append_c(tmp, '\0');
	if (strlen(tmp->str) > 0)
		*length = atoi(tmp->str);

	if (fmt[pos] == '#') {
		g_string_set_size(tmp, 0);
		for (pos++; fmt[pos] && isdigit(fmt[pos]); pos++)
			g_string_append_c(tmp, fmt[pos]);
		g_string_append_c(tmp, '\0');

		if (strlen(tmp->str) > 0)
			*lprec = atoi(tmp->str);
	}

	if (fmt[pos] != '.') {
		g_string_free(tmp, TRUE);
		*advance = pos;
		return;
	}

	g_string_set_size(tmp, 0);
	for (pos++; fmt[pos] && isdigit(fmt[pos]); pos++)
		g_string_append_c(tmp, fmt[pos]);
	g_string_append_c(tmp, '\0');

	if (strlen(tmp->str) > 0)
		*prec = atoi(tmp->str);

	g_string_free(tmp, TRUE);
	*advance = pos;
	return;
}

GString *get_next_format_string(rlib *r, const gchar *fmt, gint expected_type, gint *out_type, gint *advance, gboolean *error) {
	GString *str;
	gint type = RLIB_FORMATSTR_NONE;
	gint adv = 0;

	*error = FALSE;

	if (fmt == NULL) {
		*out_type = type;
		*advance = 0;
		return NULL;
	}

	if (fmt[adv] == '\0') {
		*out_type = type;
		*advance = 0;
		return NULL;
	}

	str = g_string_new("");

	while (fmt[adv]) {
		if (expected_type == RLIB_FORMATSTR_LITERAL) {
			/* Shortcut when a literal is expected. */
			while (fmt[adv]) {
				/* Convert %% to %% but don't bother with anything else. */
				if (fmt[adv] == '%' && fmt[adv + 1] == '%') {
					g_string_append_c(str, '%');
					adv += 2;
				} else {
					g_string_append_c(str, fmt[adv]);
					adv++;
				}
			}
			*out_type = RLIB_FORMATSTR_LITERAL;
			*advance = adv;
			return str;
		}

		if (fmt[adv] == '!') {
			/* Possibly a new style format string */
			switch (fmt[adv + 1]) {
			case '@':
				/* Possibly a new style date format string for strfmon style formatting. */
				if (expected_type != RLIB_FORMATSTR_DATE) {
					switch (type) {
					case RLIB_FORMATSTR_NONE:
						type = RLIB_FORMATSTR_LITERAL;
						/* fall through */
					case RLIB_FORMATSTR_LITERAL:
						g_string_append_len(str, fmt + adv, 2);
						adv += 2;
						break;
					default:
						*error = TRUE;
						g_string_printf(str, "!ERR_F_F");
						return str;
					}
				} else {
					switch (type) {
					case RLIB_FORMATSTR_LITERAL:
						/*
						 * Date format specifier found but there's already
						 * a literal before it. Let's return it now.
						 */
						g_string_append_c(str, '\0');
						*out_type = type;
						*advance = adv;
						return str;
					case RLIB_FORMATSTR_NONE:
						/* Date... */
						type = RLIB_FORMATSTR_DATE;
						if (fmt[adv + 2] == '{') {
							/*
							 * Process until '}' or break with an error
							 * if '}' is not found.
							 */
							int i;
							for (i = adv; fmt[i] && fmt[i] != '}'; i++)
								;

							if (fmt[i] == '}') {
								g_string_append_len(str, fmt + adv + 3, i - adv - 3);
								g_string_append_c(str, '\0');
								*out_type = type;
								*advance = i - adv + 1;
								return str;
							}

							g_string_printf(str, "!ERR_DT_NO");
							*out_type = type;
							*error = TRUE;
							return str;
						} else {
							/* Process until the end of the string */
							g_string_append(str, fmt + adv + 2);
							adv += strlen(fmt + adv);
							*out_type = type;
							*advance = adv;
							return str;
						}
						break;
					default:
						*error = TRUE;
						g_string_printf(str, "!ERR_F_F");
						return str;
					}
				}
				break;
			case '$':
				/* Possibly a new style monetary number for strfmon style formatting */
				if (expected_type != RLIB_FORMATSTR_NUMBER) {
					switch (type) {
					case RLIB_FORMATSTR_NONE:
						type = RLIB_FORMATSTR_LITERAL;
						/* fall through */
					case RLIB_FORMATSTR_LITERAL:
						g_string_append_len(str, fmt + adv, 2);
						adv += 2;
						break;
					default:
						*error = TRUE;
						g_string_printf(str, "!ERR_F_F");
						return str;
					}
				} else {
					switch (type) {
					case RLIB_FORMATSTR_LITERAL:
						/*
						 * Date format specifier found but there's already
						 * a literal before it. Let's return it now.
						 */
						g_string_append_c(str, '\0');
						*out_type = type;
						*advance = adv;
						return str;
					case RLIB_FORMATSTR_NONE:
						/* Monetary number... */
						type = RLIB_FORMATSTR_MONEY;
						if (fmt[adv + 2] == '{') {
							/*
							 * Process until '}' or break with an error
							 * if '}' is not found.
							 */
							int i;
							for (i = adv; fmt[i] && fmt[i] != '}'; i++)
								;

							if (fmt[i] == '}') {
								gchar filler, conv;
								gint adv1, adv2;
								gint flags, length, lprec, prec;
								g_string_append_len(str, fmt + adv + 3, i - adv - 3);
								g_string_append_c(str, '\0');

								if (str->str[0] != '%') {
									*out_type = type;
									*error = TRUE;
									g_string_printf(str, "!ERR_F_F");
									return str;
								}
								flags = formatstring_flags_money(str->str + 1, &filler, &adv1);
								formatstring_money_length_prec(str->str + 1 + adv1, &length, &lprec, &prec, &adv2);
								if (prec > 0) {
									g_string_printf(str, "!ERR_F_F");
									*out_type = type;
									*error = TRUE;
									return str;
								}

								conv = str->str[adv1 + adv2 + 1];
								if (!good_money_conv(conv)) {
									g_string_printf(str, "!ERR_F_F");
									*out_type = type;
									*error = TRUE;
									return str;
								}

								g_string_printf(str, "%%");
								if ((flags & RLIB_FMTSTR_MFLAG_NOGROUPING))
									g_string_append_c(str, '^');
								if ((flags & RLIB_FMTSTR_MFLAG_NEG_PAR))
									g_string_append_c(str, '(');
								if ((flags & RLIB_FMTSTR_MFLAG_NEG_SIGN))
									g_string_append_c(str, '+');
								if ((flags & RLIB_FMTSTR_MFLAG_OMIT_CURRENCY))
									g_string_append_c(str, '!');
								if ((flags & RLIB_FMTSTR_MFLAG_LEFTALIGN))
									g_string_append_c(str, '-');
								if ((flags & RLIB_FMTSTR_MFLAG_FILLCHAR)) {
									g_string_append_printf(str, "=%c", filler);
								}
								if (length > 0)
									g_string_append_printf(str, "%d", length);
								if (lprec > 0)
									g_string_append_printf(str, "#%d", lprec);
								if (prec > 0)
									g_string_append_printf(str, ".%d", prec);
								g_string_append_c(str, conv);
								g_string_append_c(str, '\0');

								*out_type = type;
								*advance = i - adv + 1;
								return str;
							}

							g_string_printf(str, "!ERR_F_F");
							*out_type = type;
							*error = TRUE;
							return str;
						} else {
							gchar filler = '\0', conv;
							gint adv1, adv2;
							gint flags, length, lprec, prec;

							/*
							 * Accept any filler char as ornament
							 * before the actual format string
							 */
							while (fmt[adv + 2] != '\0' && fmt[adv + 2] != '%') {
								g_string_append_c(str, fmt[adv + 2]);
								adv++;
							}

							if (fmt[adv + 2] == '\0') {
								g_string_printf(str, "!ERR_F_F");
								*out_type = type;
								*error = TRUE;
								return str;
							}

							flags = formatstring_flags_money(fmt + adv + 3, &filler, &adv1);
							formatstring_money_length_prec(fmt + adv + 3 + adv1, &length, &lprec, &prec, &adv2);
							if (prec > 0) {
								g_string_printf(str, "!ERR_F_F");
								*out_type = type;
								*error = TRUE;
								return str;
							}

							conv = fmt[adv + 3 + adv1 + adv2];
							if (!good_money_conv(conv)) {
								g_string_printf(str, "!ERR_F_F");
								*out_type = type;
								*error = TRUE;
								return str;
							}

							g_string_append_c(str, '%');
							if ((flags & RLIB_FMTSTR_MFLAG_NOGROUPING))
								g_string_append_c(str, '^');
							if ((flags & RLIB_FMTSTR_MFLAG_NEG_PAR))
								g_string_append_c(str, '(');
							if ((flags & RLIB_FMTSTR_MFLAG_NEG_SIGN))
								g_string_append_c(str, '+');
							if ((flags & RLIB_FMTSTR_MFLAG_OMIT_CURRENCY))
								g_string_append_c(str, '!');
							if ((flags & RLIB_FMTSTR_MFLAG_LEFTALIGN))
								g_string_append_c(str, '-');
							if ((flags & RLIB_FMTSTR_MFLAG_FILLCHAR)) {
								g_string_append_printf(str, "=%c", filler);
							}
							if (length > 0)
								g_string_append_printf(str, "%d", length);
							if (lprec > 0)
								g_string_append_printf(str, "#%d", lprec);
							if (prec > 0)
								g_string_append_printf(str, ".%d", prec);
							g_string_append_c(str, conv);
							g_string_append_c(str, '\0');

							*out_type = type;
							*advance = adv + adv1 + adv2 + 4;
							return str;
						}
						break;
					default:
						*error = TRUE;
						g_string_printf(str, "!ERR_F_F");
						return str;
					}
				}
				break;
			case '#':
				/* Possibly a new style number for printf style formatting */
				if (expected_type != RLIB_FORMATSTR_NUMBER) {
					if (expected_type != RLIB_FORMATSTR_LITERAL && (fmt[adv + 2] == '%' || (fmt[adv + 2] == '{' && fmt[adv + 3] == '%'))) {
						switch (type) {
						case RLIB_FORMATSTR_LITERAL:
							/*
							 * Number format specifier found but there's already
							 * a literal before it. Let's return it now.
							 */
							g_string_append_c(str, '\0');
							*out_type = type;
							*advance = adv;
							return str;
						default:
							*error = TRUE;
							g_string_printf(str, "!ERR_F_F");
							return str;
						}
					}
					switch (type) {
					case RLIB_FORMATSTR_NONE:
						type = RLIB_FORMATSTR_LITERAL;
						/* fall through */
					case RLIB_FORMATSTR_LITERAL:
						g_string_append_len(str, fmt + adv, 2);
						adv += 2;
						break;
					default:
						*error = TRUE;
						g_string_printf(str, "!ERR_F_F");
						return str;
					}
				} else {
					switch (type) {
					case RLIB_FORMATSTR_LITERAL:
						/*
						 * Number format specifier found but there's already
						 * a literal before it. Let's return it now.
						 */
						g_string_append_c(str, '\0');
						*out_type = type;
						*advance = adv;
						return str;
					case RLIB_FORMATSTR_NONE:
						/* Number... */
						type = RLIB_FORMATSTR_NUMBER;
						if (fmt[adv + 2] == '{') {
							/*
							 * Process until '}' or break with an error
							 * if '}' is not found.
							 */
							int i;
							for (i = adv; fmt[i] && fmt[i] != '}'; i++)
								;

							if (fmt[i] == '}') {
								GString *str1;
								gint type1, adv1;
								gboolean error1;

								g_string_append_len(str, fmt + adv + 3, i - adv - 3);
								g_string_append_c(str, '\0');
								*out_type = type;
								*advance = i - adv + 1;
								str1 = get_next_format_string(r, str->str, RLIB_FORMATSTR_NUMBER, &type1, &adv1, &error1);
								g_string_free(str, TRUE);
								return str1;
							}

							g_string_printf(str, "!ERR_F_F");
							*out_type = type;
							*error = TRUE;
							return str;
						} else {
							gint adv2, adv3;
							gint flags;
							gboolean legacy = FALSE;
							gint length, prec;
							gchar c;

							/*
							 * Accept any filler char as ornament
							 * before the actual format string.
							 */
							while (fmt[adv + 2] != '\0' && fmt[adv + 2] != '%') {
								g_string_append_c(str, fmt[adv + 2]);
								adv++;
							}

							if (fmt[adv + 2] == '\0') {
								g_string_printf(str, "!ERR_F_F");
								*out_type = type;
								*error = TRUE;
								return str;
							}

							flags = formatstring_flags(fmt + adv + 3, &adv2);
							formatstring_length_prec(fmt + adv + 3 + adv2, &length, &prec, &adv3);
							c = fmt[adv + 3 + adv2 + adv3];
							if ((good_conv(c) && expected_type == RLIB_FORMATSTR_NUMBER) || (c == 's' && expected_type == RLIB_FORMATSTR_STRING) ) {
								type = expected_type;
								if (c == 'd') {
									/* Legacy format regarding length/precision count */
									if (prec)
										length += prec + 1;
									c = 'f';
									legacy = TRUE;
								}
								g_string_append_c(str, '%');
								if ((flags & RLIB_FMTSTR_FLAG_ALTERNATE))
									g_string_append_c(str, '#');
								if ((flags & RLIB_FMTSTR_FLAG_0PADDED))
									g_string_append_c(str, '0');
								if ((flags & RLIB_FMTSTR_FLAG_LEFTALIGN))
									g_string_append_c(str, '-');
								if ((flags & RLIB_FMTSTR_FLAG_SIGN_BLANK))
									g_string_append_c(str, ' ');
								if ((flags & RLIB_FMTSTR_FLAG_SIGN))
									g_string_append_c(str, '+');
								if ((flags & RLIB_FMTSTR_FLAG_GROUPING))
									g_string_append_c(str, '\'');
								if ((flags & RLIB_FMTSTR_FLAG_ALTERNATE_DIGITS))
									g_string_append_c(str, 'I');
								if (expected_type == RLIB_FORMATSTR_STRING) {
									if (length > 0 && prec == 0) {
										prec = length;
										length = 0;
									}
								}
								if (length)
									g_string_append_printf(str, "%d", length);
								if (prec)
									g_string_append_printf(str, ".%d", prec);
								else if (legacy)
									g_string_append(str, ".0");
								if (expected_type == RLIB_FORMATSTR_NUMBER)
									g_string_append_printf(str, "R%c", c);
								else
									g_string_append_c(str, c);
								*advance = adv + adv2 + adv3 + 4;
								*out_type = type;
								return str;
							} else {
								g_string_printf(str, "!ERR_F_F");
								*error = TRUE;
								*out_type = type;
								return str;
							}
						}

						break;
					default:
						*error = TRUE;
						g_string_printf(str, "!ERR_F_F");
						return str;
					}
				}
				break;
			default:
				switch (type) {
				case RLIB_FORMATSTR_NONE:
					type = RLIB_FORMATSTR_LITERAL;
					/* fall through */
				case RLIB_FORMATSTR_LITERAL:
					g_string_append_len(str, fmt + adv, 2);
					if (fmt[adv + 1])
						adv += 2;
					else {
						adv++;
						*advance = adv;
						*out_type = type;
						return str;
					}
					break;
				default:
					*error = TRUE;
					g_string_printf(str, "!ERR_F_F");
					return str;
				}
			}
		} else if (fmt[adv] == '%') {
			/*
			 * Legacy format string handling
			 */
			if (!fmt[adv + 1] || fmt[adv + 1] == '%') {
				switch (type) {
				case RLIB_FORMATSTR_NONE:
					type = RLIB_FORMATSTR_LITERAL;
					/* fall through */
				case RLIB_FORMATSTR_LITERAL:
					g_string_append_c(str, fmt[adv]);
					if (fmt[adv + 1] == '%')
						adv += 2;
					else
						adv++;
					break;
				default:
					*error = TRUE;
					g_string_printf(str, "!ERR_F_F");
					return str;
				}
			} else if (strlen(fmt + adv + 1) > 1 && fmt[adv + 1] == '$') {
				/* Legacy thousand grouping in numbers */
				gint adv2, adv3;
				gint flags;
				gboolean legacy = FALSE;
				gint length, prec;
				gchar c;

				type = RLIB_FORMATSTR_NUMBER;
				flags = formatstring_flags(fmt + adv + 2, &adv2);
				flags |= RLIB_FMTSTR_FLAG_GROUPING;
				formatstring_length_prec(fmt + adv + 2 + adv2, &length, &prec, &adv3);
				c = fmt[adv + 2 + adv2 + adv3];
				if (good_conv(c)) {
					if (c == 'd') {
						/* Legacy format regarding length/precision count */
						if (prec)
							length += prec + 1;
						c = 'f';
						legacy = TRUE;
					}
					g_string_printf(str, "%%");
					if ((flags & RLIB_FMTSTR_FLAG_ALTERNATE))
						g_string_append_c(str, '#');
					if ((flags & RLIB_FMTSTR_FLAG_0PADDED))
						g_string_append_c(str, '0');
					if ((flags & RLIB_FMTSTR_FLAG_LEFTALIGN))
						g_string_append_c(str, '-');
					if ((flags & RLIB_FMTSTR_FLAG_SIGN_BLANK))
						g_string_append_c(str, ' ');
					if ((flags & RLIB_FMTSTR_FLAG_SIGN))
						g_string_append_c(str, '+');
					if ((flags & RLIB_FMTSTR_FLAG_GROUPING))
						g_string_append_c(str, '\'');
					if ((flags & RLIB_FMTSTR_FLAG_ALTERNATE_DIGITS))
						g_string_append_c(str, 'I');
					if (length)
						g_string_append_printf(str, "%d", length);
					if (prec)
						g_string_append_printf(str, ".%d", prec);
					else if (legacy)
						g_string_append(str, ".0");
					g_string_append_printf(str, "R%c", c);
					*advance = adv + adv2 + adv3 + 3;
					*out_type = type;
					return str;
				} else {
					g_string_printf(str, "!ERR_F_F");
					*error = TRUE;
					*out_type = type;
					return str;
				}
			} else {
				switch (expected_type) {
				case RLIB_FORMATSTR_DATE:
					switch (type) {
					case RLIB_FORMATSTR_LITERAL:
						/*
						 * Date format specifier found but there's already
						 * a literal before it. Let's return it now.
						 */
						g_string_append_c(str, '\0');
						*out_type = type;
						*advance = adv;
						return str;
					case RLIB_FORMATSTR_NONE:
						/* Process until the end of the string */
						g_string_append(str, fmt + adv);
						adv += strlen(fmt + adv);
						*out_type = RLIB_FORMATSTR_DATE;
						*advance = adv;
						return str;
					default:
						*error = TRUE;
						g_string_printf(str, "!ERR_F_F");
						return str;
					}
				case RLIB_FORMATSTR_NUMBER:
				case RLIB_FORMATSTR_STRING:
					switch (type) {
					case RLIB_FORMATSTR_LITERAL:
						/*
						 * Date format specifier found but there's already
						 * a literal before it. Let's return it now.
						 */
						g_string_append_c(str, '\0');
						*out_type = type;
						*advance = adv;
						return str;
					case RLIB_FORMATSTR_NONE:
						break;
					default:
						*error = TRUE;
						g_string_printf(str, "!ERR_F_F");
						return str;
					}
					{
						gint adv2, adv3;
						gint flags;
						gboolean legacy = FALSE;
						gint length, prec;
						gchar c;

						flags = formatstring_flags(fmt + adv + 1, &adv2);
						formatstring_length_prec(fmt + adv + 1 + adv2, &length, &prec, &adv3);
						c = fmt[adv + 1 + adv2 + adv3];
						if ((good_conv(c) && expected_type == RLIB_FORMATSTR_NUMBER) || (c == 's' && expected_type == RLIB_FORMATSTR_STRING) ) {
							type = expected_type;
							if (c == 'd') {
								/* Legacy format regarding length/precision count */
								if (prec)
									length += prec + 1;
								c = 'f';
								legacy = TRUE;
							}
							g_string_printf(str, "%%");
							if ((flags & RLIB_FMTSTR_FLAG_ALTERNATE))
								g_string_append_c(str, '#');
							if ((flags & RLIB_FMTSTR_FLAG_0PADDED))
								g_string_append_c(str, '0');
							if ((flags & RLIB_FMTSTR_FLAG_LEFTALIGN))
								g_string_append_c(str, '-');
							if ((flags & RLIB_FMTSTR_FLAG_SIGN_BLANK))
								g_string_append_c(str, ' ');
							if ((flags & RLIB_FMTSTR_FLAG_SIGN))
								g_string_append_c(str, '+');
							if ((flags & RLIB_FMTSTR_FLAG_GROUPING))
								g_string_append_c(str, '\'');
							if ((flags & RLIB_FMTSTR_FLAG_ALTERNATE_DIGITS))
								g_string_append_c(str, 'I');
							if (expected_type == RLIB_FORMATSTR_STRING) {
								if (length > 0 && prec == 0) {
									prec = length;
									length = 0;
								}
							}
							if (length)
								g_string_append_printf(str, "%d", length);
							if (prec)
								g_string_append_printf(str, ".%d", prec);
							else if (legacy)
								g_string_append(str, ".0");
							if (expected_type == RLIB_FORMATSTR_NUMBER)
								g_string_append_printf(str, "R%c", c);
							else
								g_string_append_c(str, c);
							*advance = adv + adv2 + adv3 + 2;
							*out_type = type;
							return str;
						} else {
							g_string_printf(str, "!ERR_F_F");
							*error = TRUE;
							*out_type = type;
							return str;
						}
					}
				}
			}
		} else {
			switch (type) {
			case RLIB_FORMATSTR_NONE:
				type = RLIB_FORMATSTR_LITERAL;
				/* fall through */
			case RLIB_FORMATSTR_LITERAL:
				g_string_append_c(str, fmt[adv]);
				adv++;
				break;
			default:
				*error = TRUE;
				g_string_printf(str, "!ERR_F_F");
				return str;
			}
		}
	}

	*advance = adv;
	*out_type = type;
	return str;
}


gboolean rlib_format_number(rlib *r, gchar **dest, const gchar *fmt, gint64 value) {
	GString *str, *tmp;
	gint types[2] = { RLIB_FORMATSTR_NUMBER, RLIB_FORMATSTR_LITERAL };
	gint type_idx, type, advance, adv;
	gboolean error;
	char *num;

	if (dest == NULL || fmt == NULL)
		return FALSE;

	str = g_string_new("");
	advance = 0;
	type_idx = 0;
	while (fmt[advance]) {
		tmp = get_next_format_string(r, fmt + advance, types[type_idx], &type, &adv, &error);
		if (error) {
			g_string_free(str, TRUE);
			*dest = g_string_free(tmp, FALSE);
			return FALSE;
		}

		advance += adv;

		switch (type) {
		case RLIB_FORMATSTR_LITERAL:
			g_string_append(str, tmp->str);
			g_string_free(tmp, TRUE);
			break;
		case RLIB_FORMATSTR_NUMBER:
			num = NULL;
			if (asprintf(&num, tmp->str, (double)value / (double)RLIB_DECIMAL_PRECISION) >= 0) {
				change_radix_character(r, num);
				g_string_append(str, num);
				free(num);
			}
			/*
			 * There is a possible leak of "num" in case of an error
			 * but it can't be avoided, as asprintf() may leave
			 * it undefined in this case.
			 */
			g_string_free(tmp, TRUE);
			break;
		}

		if (type == types[type_idx])
			type_idx++;
	}

	*dest = g_string_free(str, FALSE);
	return TRUE;
}

/*
 * Formats numbers in money format using locale parameters and moneyformat codes
 */
gboolean rlib_format_money(rlib *r, gchar **dest, const gchar *moneyformat, gint64 value) {
	gboolean result;
	ssize_t len;

	*dest = g_malloc(MAXSTRLEN);

	len = strfmon(*dest, MAXSTRLEN - 1, moneyformat, (double)value / (double)RLIB_DECIMAL_PRECISION);
	if (len <= 0)
		result = FALSE;
	else
		change_radix_character(r, *dest);
	return result;
}

static gboolean string_sprintf(rlib *r, gchar **dest, gchar *fmtstr, struct rlib_value *rval) {
	gchar *value = RLIB_VALUE_GET_AS_STRING(rval);
	*dest = g_strdup_printf(fmtstr, value);
	return TRUE;
}

#define MAX_NUMBER 128

gint rlib_number_sprintf(rlib *r, gchar **woot_dest, gchar *fmtstr, const struct rlib_value *rval, gint special_format, gchar *infix, gint line_number) {
	GString *str;
	gint types[2] = { RLIB_FORMATSTR_NUMBER, RLIB_FORMATSTR_LITERAL };
	gint type_idx, type;
	gint advance, slen;

	str = g_string_new("");
	advance = 0;
	type_idx = 0;
	while (fmtstr[advance]) {
		gint adv;
		gboolean error;
		GString *str1;

		str1 = get_next_format_string(r, fmtstr + advance, types[type_idx], &type, &adv, &error);

		if (error) {
			g_string_free(str, TRUE);
			slen = str1->len;
			*woot_dest = g_string_free(str1, FALSE);
			return slen;
		}

		switch (type) {
		case RLIB_FORMATSTR_LITERAL:
			g_string_append(str, str1->str);
			break;
		case RLIB_FORMATSTR_NUMBER:
			{
				char *formatted;

				if (asprintf(&formatted, str1->str, (double)RLIB_VALUE_GET_AS_NUMBER(rval) / (double)RLIB_DECIMAL_PRECISION) >= 0) {
					g_string_append(str, formatted);
					free(formatted);
				}
			}
			break;
		case RLIB_FORMATSTR_MONEY:
			{
				gchar *money = g_malloc(MAXSTRLEN);
				if (strfmon(money, MAXSTRLEN, str1->str, (double)RLIB_VALUE_GET_AS_NUMBER(rval) / (double)RLIB_DECIMAL_PRECISION) >= 0)
					g_string_append(str, money);
				g_free(money);
			}
			break;
		}

		g_string_free(str1, TRUE);
		advance += adv;
	}

	slen = str->len;
	*woot_dest = g_string_free(str, FALSE);
	return slen;
}

static gboolean rlib_format_string_default(rlib *r, struct rlib_value *rval, gchar **dest) {
	if (RLIB_VALUE_IS_NUMBER(rval)) {
		char *tmp;
		asprintf(&tmp, "%.0lf", (double)RLIB_VALUE_GET_AS_NUMBER(rval) / (double)RLIB_DECIMAL_PRECISION);
		*dest = g_strdup(tmp);
		free(tmp);
	} else if (RLIB_VALUE_IS_STRING(rval)) {
		if (RLIB_VALUE_GET_AS_STRING(rval) == NULL)
			*dest = NULL;
		else
			*dest = g_strdup(RLIB_VALUE_GET_AS_STRING(rval));
	} else if (RLIB_VALUE_IS_DATE(rval))  {
		struct rlib_datetime dt = RLIB_VALUE_GET_AS_DATE(rval);
		rlib_datetime_format(r, dest, &dt, "%m/%d/%Y");
	} else {
		*dest = g_strdup_printf("!ERR_F");
		return FALSE;
	}
	return TRUE;
}

gboolean rlib_format_string(rlib *r, gchar **dest, struct rlib_report_field *rf, struct rlib_value *rval) {
	if (r->special_locale != NULL)
		setlocale(LC_ALL, r->special_locale);
	if (rf->xml_format.xml == NULL) {
		rlib_format_string_default(r, rval, dest);
	} else {
		gchar *formatstring;
		struct rlib_value rval_fmtstr2, *rval_fmtstr;
		rval_fmtstr = rlib_execute_pcode(r, &rval_fmtstr2, rf->format_code, rval);
		if (!RLIB_VALUE_IS_STRING(rval_fmtstr)) {
			*dest = g_strdup_printf("!ERR_F_F");
			rlib_value_free(rval_fmtstr);
			if (r->special_locale != NULL)
				setlocale(LC_ALL, r->current_locale);
			return FALSE;
		} else {
			formatstring = RLIB_VALUE_GET_AS_STRING(rval_fmtstr);

			if (formatstring == NULL) {
				rlib_format_string_default(r, rval, dest);
			} else {
				GString *str;
				gint types[2] = { RLIB_FORMATSTR_NONE, RLIB_FORMATSTR_LITERAL };
				gint advance, type_idx;

				if (RLIB_VALUE_IS_NUMBER(rval))
					types[0] = RLIB_FORMATSTR_NUMBER;
				else if (RLIB_VALUE_IS_DATE(rval))
					types[0] = RLIB_FORMATSTR_DATE;
				else if (RLIB_VALUE_IS_STRING(rval))
					types[0] = RLIB_FORMATSTR_STRING;

				str = g_string_new("");
				advance = 0;
				type_idx = 0;
				while (formatstring[advance]) {
					GString *tmp;
					gchar *result;
					gint type, adv;
					gboolean error;

					tmp = get_next_format_string(r, formatstring + advance, types[type_idx], &type, &adv, &error);

					if (error) {
						g_string_free(str, TRUE);
						*dest = g_string_free(tmp, FALSE);
						return FALSE;
					}

					if (types[type_idx] == type || (types[type_idx] == RLIB_FORMATSTR_NUMBER && type == RLIB_FORMATSTR_MONEY))
						type_idx++;

					switch (type) {
					case RLIB_FORMATSTR_LITERAL:
						g_string_append(str, tmp->str);
						break;
					case RLIB_FORMATSTR_NUMBER:
						if (asprintf(&result, tmp->str, (double)RLIB_VALUE_GET_AS_NUMBER(rval) / (double)RLIB_DECIMAL_PRECISION) >= 0) {
							g_string_append(str, result);
							free(result);
						}
						break;
					case RLIB_FORMATSTR_MONEY:
						result = g_malloc(MAXSTRLEN);
						if (strfmon(result, MAXSTRLEN, tmp->str, (double)RLIB_VALUE_GET_AS_NUMBER(rval) / (double)RLIB_DECIMAL_PRECISION) >= 0) {
							g_string_append(str, result);
							g_free(result);
						}
						break;
					case RLIB_FORMATSTR_DATE:
						{
							struct rlib_datetime dt = RLIB_VALUE_GET_AS_DATE(rval);
							rlib_datetime_format(r, &result, &dt, tmp->str);
						}
						g_string_append(str, result);
						g_free(result);
						break;
					case RLIB_FORMATSTR_STRING:
						string_sprintf(r, &result, tmp->str, rval);
						g_string_append(str, result);
						g_free(result);
						break;
					}

					if (r->special_locale != NULL)
						setlocale(LC_ALL, r->current_locale);
					rlib_value_free(rval_fmtstr);

					advance += adv;

					g_string_free(tmp, TRUE);
				}
				*dest = g_string_free(str, FALSE);
			}
			rlib_value_free(rval_fmtstr);
		}
	}

	if (r->special_locale != NULL)
		setlocale(LC_ALL, r->current_locale);
	return TRUE;
}

gchar *rlib_align_text(rlib *r, gchar **my_rtn, gchar *src, gint align, gint width) {
	gint len = 0, size = 0, lastidx = 0;
	gchar *rtn;

	if (width == 0) {
		rtn = *my_rtn = g_strdup("");
		return rtn;
	}

	if (src != NULL) {
		len = r_strlen(src);
		size = strlen(src);
		lastidx = width + size - len;
	}

	if (len > width) {
		rtn = *my_rtn = g_strdup(src ? src : "");
		return rtn;
	} else {
		rtn = *my_rtn  = g_malloc(lastidx + 1);
		if (width >= 1) {
			memset(rtn, ' ', lastidx);
			rtn[lastidx] = 0;
		}  
		if (src != NULL)
			memcpy(rtn, src, size);
	}

	if (!OUTPUT(r)->do_align)
		return rtn;

	if (align == RLIB_ALIGN_LEFT || width == -1) {
		/* intentionally do nothing here */
	} else {
		if (align == RLIB_ALIGN_RIGHT) {        
			gint x = lastidx - size;
			if (x > 0) {
				memset(rtn, ' ', x);
				if (src == NULL)
					rtn[x] = 0;
				else
					g_strlcpy(rtn+x, src, lastidx);
			}
		}
		if (align == RLIB_ALIGN_CENTER) {
			if (!(width > 0 && len > width)) {
				gint x = (width - len)/2;
				if (x > 0) {
					memset(rtn, ' ', x);
					if (src == NULL)
						rtn[x] = 0;
					else
						memcpy(rtn+x, src, size);
				}
			}
		}
	}
	return rtn;
}

GSList * rlib_format_split_string(rlib *r, gchar *data, gint width, gint max_lines, gchar new_line, gchar space, gint *line_count) {
	gint slen;
	gint spot = 0;
	gint end = 0;
	gint i;
	gint line_spot = 0;
	gboolean at_the_end = FALSE;
	GSList *list = NULL;
	
	if (data == NULL) {
		gchar *this_line = g_malloc(width);
		list = g_slist_append(list, this_line);
		memset(this_line, 0, width);
		*line_count = 1;
		return list;
	}
	
	slen = strlen(data);
	while(1) {
		gchar *this_line = g_malloc(width);
		gint space_count = 0;
		memset(this_line, 0, width);
		end = spot + width-1;
		if (end > slen) {
			end = slen;
			at_the_end = TRUE;
		}
	
		line_spot = 0;
		for (i = spot; i < end; i++) {
			spot++;
			if (data[i] == new_line)
				break;
			this_line[line_spot++] = data[i];
			if (data[i] == space)
				space_count++;
		}
		
		if (spot < slen)
			at_the_end = FALSE;
			
		if (!at_the_end) {
			if (space_count == 0) {
				/* We do nothing here as we have text that just won't fit.. we split it up in "width" chunks */
			} else {
				while (data[i] != space && data[i] != new_line) {
					this_line[--line_spot] = 0;
					i--;
					spot--;
				}
			}
			if (data[spot] == space)
				spot++;
		}

		list = g_slist_append(list, this_line);
		*line_count = *line_count + 1;

		if (at_the_end == TRUE)
			break;
	}
	
	return list;
}
