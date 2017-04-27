/*
 *  Copyright (C) 2003-2017 SICOM Systems, INC.
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <glib.h>
#include <hpdf.h>
#include <fontconfig/fontconfig.h>

#include "rpdf-internal.h"
#include "rpdf.h"

#ifndef O_BINARY
#define	O_BINARY	(0)
#endif

#define UNUSED __attribute__((unused))

#define NUM_PDF_BASE_FONTS 14
static const char *base_fonts[NUM_PDF_BASE_FONTS] = {
	"Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique",
	"Helvetica", "Helvetica-Bold", "Helvetica-Oblique", "Helvetica-BoldOblique",
	"Times-Roman", "Times-Bold", "Times-Italic", "Times-BoldItalic",
	"Symbol", "ZapfDingbats"
};

#define DEGREE_2_RAD(x) ((x) * (M_PI) / 180.0)

static void rpdf_error(const gchar *fmt, ...) __attribute__ ((format(printf, 1, 2)));
static void rpdf_error(const gchar *fmt, ...) {
	va_list vl;
	gchar *result = NULL;

	va_start(vl, fmt);
	result = g_strdup_vprintf(fmt, vl);
	va_end(vl);
	if (result != NULL) {
		fprintf(stderr, "RPDF: %s", result);
		g_free(result);
	}
	return;
}

/* HARU error handler */
static void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data) {
	struct rpdf *pdf = user_data;

	rpdf_error("%s:%d: HPDF(libharu) ERROR: error_no=%04X, detail_no=%d\n", pdf->func, pdf->line, (unsigned int) error_no, (int) detail_no);

	pdf->status = error_no;
	HPDF_ResetError(pdf->pdf);
}

#if RLIB_SENDS_UTF8
static void rpdf_destroy_gconv(gpointer data) {
	GIConv conv = data;
	g_iconv_close(conv);
}
#endif

DLL_EXPORT_SYM struct rpdf *rpdf_new(void) {
	struct rpdf *pdf = g_new0(struct rpdf, 1);
#if RLIB_SENDS_UTF8
	GIConv conv;
#endif

	pdf->pdf = HPDF_New(error_handler, pdf);

	/* Conservative compression, text is not compressed */
	HPDF_SetCompressionMode(pdf->pdf, HPDF_COMP_IMAGE | HPDF_COMP_METADATA);

	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_CREATOR, "RPDF By SICOM Systems");
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_PRODUCER, "RPDF 2.0");

	HPDF_UseUTFEncodings(pdf->pdf);
	HPDF_UseCNSEncodings(pdf->pdf);
	HPDF_UseCNTEncodings(pdf->pdf);
	HPDF_UseJPEncodings(pdf->pdf);
	HPDF_UseKREncodings(pdf->pdf);

	pdf->fonts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	pdf->fontfiles = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

#if RLIB_SENDS_UTF8
	pdf->convs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, rpdf_destroy_gconv);

	conv = g_iconv_open("ISO-8859-1", "UTF-8");
	g_hash_table_insert(pdf->convs, g_strdup("MacRomanEncoding"), conv);

	conv = g_iconv_open("ISO-8859-1", "UTF-8");
	g_hash_table_insert(pdf->convs, g_strdup("WinAnsiEncoding"), conv);
#endif

	return pdf;
}

DLL_EXPORT_SYM void rpdf_set_compression(struct rpdf *pdf, gboolean use_compression) {
	pdf->func = __func__;
	pdf->line = __LINE__;
	HPDF_SetCompressionMode(pdf->pdf, (use_compression ? HPDF_COMP_ALL : (HPDF_COMP_IMAGE | HPDF_COMP_METADATA)));
	pdf->use_compression = use_compression;
}

DLL_EXPORT_SYM gboolean rpdf_get_compression(struct rpdf *pdf) {
	return pdf->use_compression;
}

static void rpdf_text_common(struct rpdf *pdf, HPDF_Page page, gdouble x, gdouble y, gdouble angle, const gchar *text);

DLL_EXPORT_SYM void rpdf_set_title(struct rpdf *pdf, const gchar *title) {
	pdf->func = __func__;
	pdf->line = __LINE__;
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_TITLE, title);
}

DLL_EXPORT_SYM void rpdf_set_subject(struct rpdf *pdf, const gchar *subject) {
	pdf->func = __func__;
	pdf->line = __LINE__;
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_SUBJECT, subject);
}

DLL_EXPORT_SYM void rpdf_set_author(struct rpdf *pdf, const gchar *author) {
	pdf->func = __func__;
	pdf->line = __LINE__;
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_AUTHOR, author);
}

DLL_EXPORT_SYM void rpdf_set_keywords(struct rpdf *pdf, const gchar *keywords) {
	pdf->func = __func__;
	pdf->line = __LINE__;
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_KEYWORDS, keywords);
}

DLL_EXPORT_SYM void rpdf_set_creator(struct rpdf *pdf, const gchar *creator) {
	pdf->func = __func__;
	pdf->line = __LINE__;
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_CREATOR, creator);
}

DLL_EXPORT_SYM void rpdf_translate(struct rpdf *pdf, gdouble x, gdouble y) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_Page_Concat(page_info->page, 1.0, 0.0, 0.0, 1.0, x * RPDF_DPI, y * RPDF_DPI); // XXX
}

DLL_EXPORT_SYM void rpdf_rotate(struct rpdf *pdf, gdouble angle) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	gdouble angle1 = M_PI * angle / 180.0;
	gdouble text_sin = sin(angle1);
	gdouble text_cos = cos(angle1);
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_Page_Concat(page_info->page, text_cos, text_sin, -text_sin, text_cos, 0.0, 0.0);
}

DLL_EXPORT_SYM gint rpdf_new_page(struct rpdf *pdf, gint paper, gint orientation) {
	struct rpdf_page_info **pdf_page_info;
	struct rpdf_page_info *page_info;
	pdf->func = __func__;
	pdf->line = __LINE__;

	pdf_page_info = g_realloc(pdf->page_info, sizeof(gpointer) * (pdf->page_count + 1));
	/* Out of memory, return the last page */
	if (!pdf_page_info)
		return pdf->page_count - 1;

	pdf->page_info = pdf_page_info;
	pdf->page_info[pdf->page_count] = g_new0(struct rpdf_page_info, 1);
	page_info = pdf->page_info[pdf->page_count];
	pdf->current_page = pdf->page_count;
	pdf->page_count++;

	pdf->line = __LINE__;
	page_info->page = HPDF_AddPage(pdf->pdf);

	/*
	 * Simulate the legacy RPDF behaviour.
	 * Landscape page is actually a portrait page
	 * with 270 degress rotation.
	 */
	pdf->line = __LINE__;
	HPDF_Page_SetSize(page_info->page, paper, HPDF_PAGE_PORTRAIT);
	pdf->line = __LINE__;
	HPDF_Page_SetRotate(page_info->page, (orientation == RPDF_PORTRAIT ? 0 : 270));

#if 0
	/* Inherit the font settings of the previous page */
	if (pdf->current_page > 0) {
		struct rpdf_page_info *prev_page_info = pdf->page_info[pdf->current_page - 1];
		page_info->current_font = prev_page_info->current_font;
		page_info->font_size = prev_page_info->font_size;
		pdf->line = __LINE__;
		HPDF_Page_SetFontAndSize(page_info->page, page_info->current_font, page_info->font_size);
	}
#endif

	return pdf->current_page;
}

DLL_EXPORT_SYM gboolean rpdf_set_page(struct rpdf *pdf, gint page) {
	struct rpdf_page_info *page_info;
	pdf->func = __func__;
	pdf->line = __LINE__;

	if (page < 0 || page >= pdf->page_count)
		return FALSE;

	pdf->current_page = page;

	page_info = pdf->page_info[pdf->current_page];

	if (page_info->current_font) {
		pdf->line = __LINE__;
		HPDF_Page_SetFontAndSize(page_info->page, page_info->current_font, page_info->font_size);
	}

	return TRUE;
}

DLL_EXPORT_SYM void rpdf_get_current_page_size(struct rpdf *pdf, gdouble *x, gdouble *y) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;
	*x = HPDF_Page_GetWidth(page_info->page);
	pdf->line = __LINE__;
	*y = HPDF_Page_GetHeight(page_info->page);
}

static const gchar *rpdf_embed_font_fc(struct rpdf *pdf, const char *fontfamily, const char *fontstyle, gboolean utf8, gboolean *error) {
	gboolean check_style = TRUE;
	FcConfig *config;
	FcPattern *pat, *font;
	FcObjectSet *os;
	FcFontSet *fs;
	FcValue val_family;
	gint i, round;
	gchar *suffix;
	const gchar *font_name = NULL;
	pdf->func = __func__;
	pdf->line = __LINE__;

	*error = FALSE;

	if (fontfamily == NULL)
		return NULL;
	if (fontstyle == NULL)
		fontstyle = RPDF_FONT_STYLE_REGULAR;

	config = FcInitLoadConfigAndFonts();
	os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, FC_INDEX, (char *) 0);
	pat = FcPatternCreate();
	val_family.type = FcTypeString;
	val_family.u.s = (FcChar8 *)fontfamily;
	FcPatternAdd(pat, FC_FAMILY, val_family, FcTrue);
	fs = FcFontList(config, pat, os);
	if (fs->nfont == 0) {
		FcFontSetDestroy(fs);
		FcObjectSetDestroy(os);
		FcPatternDestroy(pat);
		FcConfigDestroy(config);
		return NULL;
	}

	for (round = 0; !font_name && round < 2; round++) {
		FcChar8 *file = NULL;

		for (i = 0; i < fs->nfont; i++) {
			font = fs->fonts[i];
			file = NULL;

			if (FcPatternGetString(font, FC_FILE, 0, &file) != FcResultMatch)
				continue;

			if (check_style) {
				FcChar8 *style;
				if (FcPatternGetString(font, FC_STYLE, 0, &style) != FcResultMatch)
					continue;
				if (strcasecmp(fontstyle, (char *)style) != 0)
					continue;
			}

			suffix = (char *)file;
			while (*suffix)
				suffix++;
			while (*suffix != '.' && (suffix > (char *)file))
				suffix--;

			//rpdf_error("FONT EMBEDDING: font family '%s' font style '%s' -> %s\n", fontfamily, fontstyle, file);

			/* Only TTF / TTC fonts can be used with UTF-8 */
			if (strcasecmp(suffix, ".pfb") == 0) {
				if (utf8) {
					*error = TRUE;
					return NULL;
				}
			}

			font_name = g_hash_table_lookup(pdf->fontfiles, file);
			if (font_name) {
				//rpdf_error("FONT EMBEDDING: font found '%s'\n", font_name);
				return font_name;
			}

			if (strcasecmp(suffix, ".ttf") == 0) {
				pdf->line = __LINE__;
				font_name = HPDF_LoadTTFontFromFile(pdf->pdf, (char *)file, HPDF_TRUE);

				if (font_name == NULL) {
					*error = TRUE;
					return FALSE;
				}

				break;
			}
			if (strcasecmp(suffix, ".ttc") == 0) {
				int font_index;

				if (FcPatternGetInteger(font, FC_INDEX, 0, &font_index) != FcResultMatch)
					continue;

				pdf->line = __LINE__;
				font_name = HPDF_LoadTTFontFromFile2(pdf->pdf, (char *)file, font_index, HPDF_TRUE);
				if (font_name == NULL) {
					*error = TRUE;
					return FALSE;
				}

				break;
			}
			if (strcasecmp(suffix, ".pfb") == 0) {
				gchar *afm = g_strdup((char *)file);
				gint pos = (suffix - (char *)file);

				afm[pos + 1] = 'a';
				afm[pos + 2] = 'f';
				afm[pos + 3] = 'm';

				pdf->line = __LINE__;
				font_name = HPDF_LoadType1FontFromFile(pdf->pdf, afm, (char *)file);
				g_free(afm);

				if (font_name == NULL) {
					*error = TRUE;
					return FALSE;
				}

				break;
			}
		}

		if (font_name)
			g_hash_table_insert(pdf->fontfiles, g_strdup((gchar *)file), (gchar *)font_name);

		check_style = FALSE;
	}

	FcFontSetDestroy(fs);
	FcObjectSetDestroy(os);
	FcPatternDestroy(pat);
	FcConfigDestroy(config);

	return font_name;
}

struct encoding_mapping {
	const char *enc;
	const char *hpdf_enc;
};

/*
 * This mapping is needed because HPDF (libharu) only knows
 * LatinX encoding name pattern as ISO8859-X but doesn't
 * handle ISO-8859-X.
 */
static struct encoding_mapping encoding_mapping[] = {
	{ HPDF_ENCODING_FONT_SPECIFIC, HPDF_ENCODING_FONT_SPECIFIC },
	{ HPDF_ENCODING_STANDARD, HPDF_ENCODING_STANDARD },
	{ HPDF_ENCODING_MAC_ROMAN, HPDF_ENCODING_MAC_ROMAN },
	{ HPDF_ENCODING_WIN_ANSI, HPDF_ENCODING_WIN_ANSI },
	{ "ISO-8859-2", HPDF_ENCODING_ISO8859_2 },
	{ HPDF_ENCODING_ISO8859_2, HPDF_ENCODING_ISO8859_2 },
	{ "ISO-8859-3", HPDF_ENCODING_ISO8859_3 },
	{ HPDF_ENCODING_ISO8859_3, HPDF_ENCODING_ISO8859_3 },
	{ "ISO-8859-4", HPDF_ENCODING_ISO8859_4 },
	{ HPDF_ENCODING_ISO8859_4, HPDF_ENCODING_ISO8859_4 },
	{ "ISO-8859-5", HPDF_ENCODING_ISO8859_5 },
	{ HPDF_ENCODING_ISO8859_5, HPDF_ENCODING_ISO8859_5 },
	{ "ISO-8859-6", HPDF_ENCODING_ISO8859_6 },
	{ HPDF_ENCODING_ISO8859_6, HPDF_ENCODING_ISO8859_6 },
	{ "ISO-8859-7", HPDF_ENCODING_ISO8859_7 },
	{ HPDF_ENCODING_ISO8859_7, HPDF_ENCODING_ISO8859_7 },
	{ "ISO-8859-8", HPDF_ENCODING_ISO8859_8 },
	{ HPDF_ENCODING_ISO8859_8, HPDF_ENCODING_ISO8859_8 },
	{ "ISO-8859-9", HPDF_ENCODING_ISO8859_9 },
	{ HPDF_ENCODING_ISO8859_9, HPDF_ENCODING_ISO8859_9 },
	{ "ISO-8859-10", HPDF_ENCODING_ISO8859_10 },
	{ HPDF_ENCODING_ISO8859_10, HPDF_ENCODING_ISO8859_10 },
	{ "ISO-8859-11", HPDF_ENCODING_ISO8859_11 },
	{ HPDF_ENCODING_ISO8859_11, HPDF_ENCODING_ISO8859_11 },
	{ "ISO-8859-13", HPDF_ENCODING_ISO8859_13 },
	{ HPDF_ENCODING_ISO8859_13, HPDF_ENCODING_ISO8859_13 },
	{ "ISO-8859-14", HPDF_ENCODING_ISO8859_14 },
	{ HPDF_ENCODING_ISO8859_14, HPDF_ENCODING_ISO8859_14 },
	{ "ISO-8859-15", HPDF_ENCODING_ISO8859_15 },
	{ HPDF_ENCODING_ISO8859_15, HPDF_ENCODING_ISO8859_15 },
	{ "ISO-8859-16", HPDF_ENCODING_ISO8859_16 },
	{ HPDF_ENCODING_ISO8859_16, HPDF_ENCODING_ISO8859_16 },
	{ HPDF_ENCODING_CP1250, HPDF_ENCODING_CP1250 },
	{ HPDF_ENCODING_CP1251, HPDF_ENCODING_CP1251 },
	{ HPDF_ENCODING_CP1252, HPDF_ENCODING_CP1252 },
	{ HPDF_ENCODING_CP1253, HPDF_ENCODING_CP1253 },
	{ HPDF_ENCODING_CP1254, HPDF_ENCODING_CP1254 },
	{ HPDF_ENCODING_CP1255, HPDF_ENCODING_CP1255 },
	{ HPDF_ENCODING_CP1256, HPDF_ENCODING_CP1256 },
	{ HPDF_ENCODING_CP1257, HPDF_ENCODING_CP1257 },
	{ HPDF_ENCODING_CP1258, HPDF_ENCODING_CP1258 },
	{ HPDF_ENCODING_KOI8_R, HPDF_ENCODING_KOI8_R },
	/* Generic all-encompassing encoding, use with TTF fonts only */
	{ "UTF-8", "UTF-8" },
	/* Simplified Chinese */
	{ "EUC-CN", "GB-EUC-H" },
	{ "CP936", "GBK-EUC-H" },
	/* Traditional Chinese */
	{ "CP950", "ETen-B5-H" },
	/* Japanese */
	{ "EUC-JP", "EUC-H" },
	{ "CP932", "90ms-RKSJ-H" },
#if 0
	/* Japanese, proportional font */
	{ "CP932", "90msp-RKSJ-H" },
#endif
	/* Korean */
	{ "EUC-KR", "KSC-EUC-H" },
	{ "CP949", "KSCms-UHC-HW-H" },
#if 0
	/* Korean, proportional font */
	{ "CP949", "KSCms-UHC-H" },
#endif
	{ NULL, NULL }
};

static const char *get_hpdf_encoding(const char *encoding) {
	int i;

	for (i = 0; encoding_mapping[i].enc; i++) {
		if (strcasecmp(encoding_mapping[i].enc, encoding) == 0)
			return encoding_mapping[i].hpdf_enc;
	}

	return HPDF_ENCODING_WIN_ANSI;
}

#define HASH_STRING g_strconcat(font, style, encoding, NULL)
//#define HASH_STRING g_strconcat(font, style, NULL)

DLL_EXPORT_SYM gboolean rpdf_set_font(struct rpdf *pdf, const gchar *font, const gchar *style, const gchar *encoding, gdouble size) {
	gboolean found = FALSE;
	gint i = 0;
	const gchar *font_name = NULL;
	HPDF_Font hfont;
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	gchar *hash_string;
	pdf->func = __func__;
	pdf->line = __LINE__;

	//rpdf_error("FONT(0): font name '%s' font style '%s' encoding '%s' -> %s\n", font, style, encoding, font_name);
	encoding = get_hpdf_encoding(encoding);

again:
	//rpdf_error("FONT(1): font name '%s' font style '%s' encoding '%s' -> %s\n", font, style, encoding, font_name);
	hash_string = HASH_STRING;
	hfont = g_hash_table_lookup(pdf->fonts, hash_string);
	found = (hfont != NULL);

	if (!found) {
		if (font_name == NULL) {
			gboolean font_error = FALSE;

			font_name = rpdf_embed_font_fc(pdf, font, style, (strcmp(encoding, "UTF-8") == 0), &font_error);
			if (font_error) {
				//rpdf_error("%s:%d: rpdf_embed_font_fc returned with FONT ERROR\n", __func__, __LINE__);
				return FALSE;
			}

			found = (font_name != NULL);
			pdf->func = __func__;

			if (font_name == NULL) {
				for (i = 0; i < NUM_PDF_BASE_FONTS; i++) {
					//fprintf(stderr, "%s:%d font '%s' base_font[%d] '%s'\n", __func__, __LINE__, font, i, base_fonts[i]);
					if (strcasecmp(base_fonts[i], font) == 0) {
						font_name = base_fonts[i];
						found = TRUE;
						break;
					}
				}
			}
		}

		//rpdf_error("%s:%d: '%s' %s %s found %d\n", __func__, __LINE__, font_name, style, encoding, found);

		if (font_name == NULL) {
			g_free(hash_string);
			font = base_fonts[0];
			style = RPDF_FONT_STYLE_REGULAR;
			//rpdf_error("FONT NOT FOUND: font name '%s' font style '%s', using Courier\n", font, style);
			goto again;
		}

		found = TRUE;

		//rpdf_error("FONT: font name '%s' font style '%s' encoding '%s' -> %s\n", font, style, encoding, font_name);
		pdf->line = __LINE__;
		hfont = HPDF_GetFont(pdf->pdf, font_name, encoding);
		if (hfont == NULL) {
			//rpdf_error("%s:%d HPDF_GetFont failed for %s %s %s\n", __func__, __LINE__, font_name, style, encoding);
			return FALSE;
		}
		g_hash_table_insert(pdf->fonts, hash_string, hfont);

		if (strcmp(font, font_name) != 0) {
			hash_string = HASH_STRING;
			g_hash_table_insert(pdf->fonts, hash_string, hfont);
		}
	} else
		g_free(hash_string);

	page_info->current_font = hfont;

	pdf->line = __LINE__;
	HPDF_Page_SetFontAndSize(page_info->page, hfont, size);

	page_info->font_size = size;

	//rpdf_error("%s:%d last line %s %s %s: found %d\n", __func__, __LINE__, font_name, style, encoding, found);
	return found;
}

DLL_EXPORT_SYM void rpdf_set_font_size(struct rpdf *pdf, gdouble size) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;

	page_info->font_size = size;
	pdf->line = __LINE__;
	HPDF_Page_SetFontAndSize(page_info->page, page_info->current_font, size);
}

static void rpdf_text_common(struct rpdf *pdf, HPDF_Page page, gdouble x, gdouble y, gdouble angle, const gchar *text) {
#if RLIB_SENDS_UTF8
	HPDF_Font font;
	const gchar *encoding;
	gchar *new_text;
	gsize foo1;
	GIConv conv;
#endif
	gdouble angle1 = M_PI * angle / 180.0;
	gdouble text_sin = sin(angle1);
	gdouble text_cos = cos(angle1);
	pdf->func = __func__;

#if RLIB_SENDS_UTF8
	pdf->line = __LINE__;
	font = HPDF_Page_GetCurrentFont(page);
	pdf->line = __LINE__;
	encoding = HPDF_Font_GetEncodingName(font);

	conv = g_hash_table_lookup(pdf->convs, encoding);
	if (conv == NULL) {
		conv = g_iconv_open(encoding, "UTF-8");
		g_hash_table_insert(pdf->convs, g_strdup(encoding), conv);
	}

	new_text = g_convert_with_iconv(text, strlen(text), conv, &foo1, &foo1, NULL);
#endif

	pdf->line = __LINE__;
	HPDF_Page_BeginText(page);

#if 0
	/* TODO - use externally set color */
	pdf->line = __LINE__;
	HPDF_Page_SetGrayStroke(page, 0.0);
	pdf->line = __LINE__;
	HPDF_Page_SetGrayFill(page, 0.0);
#endif

	pdf->line = __LINE__;
	HPDF_Page_SetTextMatrix(page, text_cos, text_sin, -text_sin, text_cos, x * RPDF_DPI, y * RPDF_DPI);
	pdf->line = __LINE__;
#if RLIB_SENDS_UTF8
	HPDF_Page_ShowText(page, new_text);
	g_free(new_text);
#else
	HPDF_Page_ShowText(page, text);
#endif

	pdf->line = __LINE__;
	HPDF_Page_EndText(page);
}

DLL_EXPORT_SYM void rpdf_text(struct rpdf *pdf, gdouble x, gdouble y, gdouble angle, const gchar *text) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];

	rpdf_text_common(pdf, page_info->page, x, y, angle, text);
}

DLL_EXPORT_SYM gpointer rpdf_text_callback(struct rpdf *pdf, gdouble x, gdouble y, gdouble angle, gint len) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	struct rpdf_delayed_text *dt = g_new0(struct rpdf_delayed_text, 1);

	if (dt == NULL)
		return NULL;

	dt->page_number = pdf->current_page;
	dt->font = page_info->current_font;
	dt->fontsize = page_info->font_size;
	dt->x = x;
	dt->y = y;
	dt->angle = angle;
	dt->r = page_info->r;
	dt->g = page_info->g;
	dt->b = page_info->b;
	dt->len = len;

	return dt;
}

DLL_EXPORT_SYM void rpdf_finalize_text_callback(struct rpdf *pdf, gpointer user_data, const gchar *text) {
	struct rpdf_delayed_text *dt;
	struct rpdf_page_info *page_info;
	pdf->func = __func__;
	pdf->line = __LINE__;

	if (user_data == NULL)
		return;

	dt = user_data;
	page_info = pdf->page_info[dt->page_number];

	pdf->line = __LINE__;
	HPDF_Page_SetFontAndSize(page_info->page, dt->font, dt->fontsize);
	pdf->line = __LINE__;
	HPDF_Page_SetRGBFill(page_info->page, dt->r, dt->g, dt->b);
	pdf->line = __LINE__;
	HPDF_Page_SetRGBStroke(page_info->page, dt->r, dt->g, dt->b);
	rpdf_text_common(pdf, page_info->page, dt->x, dt->y, dt->angle, text);

	g_free(dt);
}

DLL_EXPORT_SYM void rpdf_image(struct rpdf *pdf, gdouble x, gdouble y, gdouble width, gdouble height, gint image_type, gchar *file_name) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	HPDF_Image image;
	struct stat st;
	pdf->func = __func__;
	pdf->line = __LINE__;

	if (stat(file_name, &st) != 0)
		return;

	switch (image_type) {
	case RPDF_IMAGE_PNG:
		pdf->line = __LINE__;
		image = HPDF_LoadPngImageFromFile(pdf->pdf, file_name);
		break;
	case RPDF_IMAGE_JPEG:
		pdf->line = __LINE__;
		image = HPDF_LoadJpegImageFromFile(pdf->pdf, file_name);
		break;
	default:
		rpdf_error("rpdf_image: Only PNG and JPG images are supported\n");
		return;
	}

	pdf->line = __LINE__;
	HPDF_Page_DrawImage(page_info->page, image, x * RPDF_DPI, y * RPDF_DPI, width, height);
}

DLL_EXPORT_SYM void rpdf_link(struct rpdf *pdf, gdouble start_x, gdouble start_y, gdouble end_x, gdouble end_y, const gchar *url) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	HPDF_Rect rect = {
		.left = start_x * RPDF_DPI,
		.bottom = start_y * RPDF_DPI,
		.right = end_x * RPDF_DPI,
		.top = end_y * RPDF_DPI
	};
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_Page_CreateURILinkAnnot(page_info->page, rect, url);
}

DLL_EXPORT_SYM void rpdf_moveto(struct rpdf *pdf, gdouble x, gdouble y) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;

	if (isnan(x))
		x = 0;
	if (isnan(y))
		y = 0;

	HPDF_Page_MoveTo(page_info->page, x * RPDF_DPI, y * RPDF_DPI);
}

DLL_EXPORT_SYM void rpdf_set_line_width(struct rpdf *pdf, gdouble width) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_Page_SetLineWidth(page_info->page, width);
}

DLL_EXPORT_SYM void rpdf_lineto(struct rpdf *pdf, gdouble x, gdouble y) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;

	if (isnan(x))
		x = 0;
	if (isnan(y))
		y = 0;

	HPDF_Page_LineTo(page_info->page, x * RPDF_DPI, y * RPDF_DPI);
}

DLL_EXPORT_SYM void rpdf_closepath(struct rpdf *pdf) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_Page_ClosePath(page_info->page);
}

DLL_EXPORT_SYM void rpdf_stroke(struct rpdf *pdf) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_Page_Stroke(page_info->page);
}

DLL_EXPORT_SYM void rpdf_rect(struct rpdf *pdf, gdouble x, gdouble y, gdouble width, gdouble height) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_Page_Rectangle(page_info->page, x * RPDF_DPI, y * RPDF_DPI, width * RPDF_DPI, height * RPDF_DPI);
}

DLL_EXPORT_SYM void rpdf_fill(struct rpdf *pdf) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_Page_Fill(page_info->page);
}

DLL_EXPORT_SYM void rpdf_setrgbcolor(struct rpdf *pdf, gdouble r, gdouble g, gdouble b) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_Page_SetRGBFill(page_info->page, r, g, b);
	pdf->line = __LINE__;
	HPDF_Page_SetRGBStroke(page_info->page, r, g, b);
	page_info->r = r;
	page_info->g = g;
	page_info->b = b;
}

DLL_EXPORT_SYM gdouble rpdf_text_width(struct rpdf *pdf, const gchar *text) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	HPDF_TextWidth w;
	pdf->func = __func__;
	pdf->line = __LINE__;

	if (text == NULL)
		return 0.0;

	w = HPDF_Font_TextWidth(page_info->current_font, (const HPDF_BYTE *)text, strlen(text));
	return w.width * page_info->font_size / 1000.0;
}

DLL_EXPORT_SYM void rpdf_arc(struct rpdf *pdf, gdouble x, gdouble y, gdouble radius, gdouble start_angle, gdouble end_angle) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	gint nsegs = 8;
	gint i;
	gdouble total_angle, dt, dtm, t1, a0, b0, c0, d0, a1, b1, c1, d1;
	pdf->func = __func__;
	pdf->line = __LINE__;

	x *= RPDF_DPI;
	y *= RPDF_DPI;
	radius *= RPDF_DPI;
	start_angle = DEGREE_2_RAD(start_angle);
	end_angle = DEGREE_2_RAD(end_angle);
	total_angle = end_angle - start_angle;
	dt = total_angle / (gfloat)nsegs;
	dtm = dt / 3.0;
	t1 = start_angle;
	a0 = x + (radius * cos(t1));
	b0 = y + (radius * sin(t1));
	c0 = -radius * sin(t1);
	d0 = radius * cos(t1);
	if (total_angle < DEGREE_2_RAD(360.0)) {  /* Pie Slices */
		pdf->line = __LINE__;
		HPDF_Page_MoveTo(page_info->page, x, y);
		pdf->line = __LINE__;
		HPDF_Page_LineTo(page_info->page, a0, b0);
	} else {
		pdf->line = __LINE__;
		HPDF_Page_MoveTo(page_info->page, a0,b0);
	}
	for (i = 1; i <= nsegs; i++) {
		t1 = ((gfloat)i * dt) + start_angle;
		a1 = x + (radius * cos(t1));
		b1 = y + (radius * sin(t1));
		c1 = -radius * sin(t1);
		d1 = radius * cos(t1);
		pdf->line = __LINE__;
		HPDF_Page_CurveTo(page_info->page, (a0 + (c0 * dtm)), ((b0 + (d0 * dtm))), (a1 - (c1 * dtm)), ((b1 - (d1 * dtm))), a1, b1);
		a0 = a1;
		b0 = b1;
		c0 = c1;
		d0 = d1;
	}
	if (total_angle < DEGREE_2_RAD(360.0)) { /* pizza :) */
		pdf->line = __LINE__;
		HPDF_Page_LineTo(page_info->page, x, y);
	}
}

DLL_EXPORT_SYM gchar *rpdf_get_buffer(struct rpdf *pdf, guint *length) {
	guchar *buf;
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_SaveToStream(pdf->pdf);
	pdf->line = __LINE__;
	*length = HPDF_GetStreamSize(pdf->pdf);

	buf = g_malloc(*length);

	pdf->line = __LINE__;
	HPDF_ResetStream(pdf->pdf);

	pdf->line = __LINE__;
	HPDF_ReadFromStream(pdf->pdf, buf, length);

	return (gchar *)buf;
}

DLL_EXPORT_SYM void rpdf_free(struct rpdf *pdf) {
	gint i;
	pdf->func = __func__;
	pdf->line = __LINE__;

	HPDF_Free(pdf->pdf);

	g_hash_table_destroy(pdf->fonts);
	g_hash_table_destroy(pdf->fontfiles);
#if RLIB_SENDS_UTF8
	g_hash_table_destroy(pdf->convs);
#endif

	for (i = 0; i < pdf->page_count; i++)
		g_free(pdf->page_info[i]);
	g_free(pdf->page_info);
	g_free(pdf);
}
