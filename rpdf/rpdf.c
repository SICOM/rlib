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

static const char *base_fonts[] = {
	"Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique",
	"Helvetica", "Helvetica-Bold", "Helvetica-Oblique", "Helvetica-BoldOblique",
	"Times-Roman", "Times-Bold", "Times-Italic", "Times-BoldItalic",
	"Symbol", "ZapfDingbats",
	NULL
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
		fprintf(stderr, "RPDF:: %s", result);
		g_free(result);
	}
	return;
}

/* HARU error handler */
static void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data UNUSED) {
	rpdf_error("HPDF(libharu) ERROR: error_no=%04X, detail_no=%d\n", (unsigned int) error_no, (int) detail_no);
}

static void rpdf_destroy_gconv(gpointer data) {
	GIConv conv = data;
	g_iconv_close(conv);
}

DLL_EXPORT_SYM struct rpdf *rpdf_new(void) {
	struct rpdf *pdf = g_new0(struct rpdf, 1);
	GIConv conv;

	pdf->pdf = HPDF_New(error_handler, NULL);
	pdf->fonts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	pdf->convs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, rpdf_destroy_gconv);

	conv = g_iconv_open("ISO-8859-1", "UTF-8");
	g_hash_table_insert(pdf->convs, g_strdup("MacRomanEncoding"), conv);

	conv = g_iconv_open("ISO-8859-1", "UTF-8");
	g_hash_table_insert(pdf->convs, g_strdup("WinAnsiEncoding"), conv);

	return pdf;
}

DLL_EXPORT_SYM void rpdf_set_compression(struct rpdf *pdf, gboolean use_compression) {
	HPDF_SetCompressionMode(pdf->pdf, (use_compression ? HPDF_COMP_ALL : 0));
	pdf->use_compression = use_compression;
}

DLL_EXPORT_SYM gboolean rpdf_get_compression(struct rpdf *pdf) {
	return pdf->use_compression;
}

static void rpdf_text_common(struct rpdf *pdf, HPDF_Page page, gdouble x, gdouble y, gdouble angle, const gchar *text);

DLL_EXPORT_SYM void rpdf_set_title(struct rpdf *pdf, const gchar *title) {
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_TITLE, title);
}

DLL_EXPORT_SYM void rpdf_set_subject(struct rpdf *pdf, const gchar *subject) {
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_SUBJECT, subject);
}

DLL_EXPORT_SYM void rpdf_set_author(struct rpdf *pdf, const gchar *author) {
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_AUTHOR, author);
}

DLL_EXPORT_SYM void rpdf_set_keywords(struct rpdf *pdf, const gchar *keywords) {
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_KEYWORDS, keywords);
}

DLL_EXPORT_SYM void rpdf_set_creator(struct rpdf *pdf, const gchar *creator) {
	HPDF_SetInfoAttr(pdf->pdf, HPDF_INFO_CREATOR, creator);
}

DLL_EXPORT_SYM void rpdf_translate(struct rpdf *pdf, gdouble x, gdouble y) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];

	HPDF_Page_Concat(page_info->page, 1.0, 0.0, 0.0, 1.0, x * RPDF_DPI, y * RPDF_DPI); // XXX
}

DLL_EXPORT_SYM void rpdf_rotate(struct rpdf *pdf, gdouble angle) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	gdouble angle1 = M_PI * angle / 180.0;
	gdouble text_sin = sin(angle1);
	gdouble text_cos = cos(angle1);

	HPDF_Page_Concat(page_info->page, text_cos, text_sin, -text_sin, text_cos, 0.0, 0.0);
}

DLL_EXPORT_SYM gint rpdf_new_page(struct rpdf *pdf, gint paper, gint orientation) {
	struct rpdf_page_info *page_info;

	pdf->page_count++;

	pdf->page_info = g_realloc(pdf->page_info, sizeof(gpointer) * pdf->page_count);
	page_info = pdf->page_info[pdf->page_count - 1] = g_new0(struct rpdf_page_info, 1);

	page_info->page = HPDF_AddPage(pdf->pdf);

	/*
	 * Simulate previous RPDF behaviour.
	 * Landscape page is actually a portrait page
	 * with 270 degress rotation.
	 */
	HPDF_Page_SetSize(page_info->page, paper, HPDF_PAGE_PORTRAIT);
	HPDF_Page_SetRotate(page_info->page, (orientation == RPDF_PORTRAIT ? 0 : 270));

	pdf->current_page = pdf->page_count - 1;
#if 0
	/* Inherit the font settings of the previous page */
	if (pdf->current_page > 0) {
		struct rpdf_page_info *prev_page_info = pdf->page_info[pdf->current_page - 1];
		page_info->current_font = prev_page_info->current_font;
		page_info->font_size = prev_page_info->font_size;
		HPDF_Page_SetFontAndSize(page_info->page, page_info->current_font, page_info->font_size);
	}
#endif

	return pdf->current_page;
}

DLL_EXPORT_SYM gboolean rpdf_set_page(struct rpdf *pdf, gint page) {
	struct rpdf_page_info *page_info;

	if (page < 0 || page >= pdf->page_count)
		return FALSE;

	pdf->current_page = page;

	page_info = pdf->page_info[pdf->current_page];

	if (page_info->current_font) {
		HPDF_Page_SetFontAndSize(page_info->page, page_info->current_font, page_info->font_size);
	}

	return TRUE;
}

DLL_EXPORT_SYM void rpdf_get_current_page_size(struct rpdf *pdf, gdouble *x, gdouble *y) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	*x = HPDF_Page_GetWidth(page_info->page);
	*y = HPDF_Page_GetHeight(page_info->page);
}

static const gchar *rpdf_embed_font_fc(struct rpdf *pdf, const char *fontfamily, const char *fontstyle) {
	FcConfig *config;
	FcPattern *pat, *font;
	FcObjectSet *os;
	FcFontSet *fs;
	FcValue val_family;
	FcValue val_style;
	FcChar8 *file;
	gint i;
	gchar *suffix;
	const gchar *font_name = NULL;

	if (fontfamily == NULL)
		return NULL;
	if (fontstyle == NULL)
		fontstyle = RPDF_FONT_STYLE_REGULAR;

	config = FcInitLoadConfigAndFonts();
	os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char *) 0);
	pat = FcPatternCreate();
	val_family.type = FcTypeString;
	val_family.u.s = (FcChar8 *)fontfamily;
	FcPatternAdd(pat, FC_FAMILY, val_family, FcTrue);
	val_style.type = FcTypeString;
	val_style.u.s = (FcChar8 *)fontstyle;
	FcPatternAdd(pat, FC_STYLE, val_style, FcTrue);
	fs = FcFontList(config, pat, os);
	if (fs->nfont == 0) {
		FcFontSetDestroy(fs);
		FcObjectSetDestroy(os);
		FcPatternDestroy(pat);
		FcConfigDestroy(config);
		return NULL;
	}

	for (i = 0; i < fs->nfont; i++) {
		font = fs->fonts[i];

		if (FcPatternGetString(font, FC_FILE, 0, &file) != FcResultMatch)
			continue;

		suffix = (char *)file;
		while (*suffix)
			suffix++;
		while (*suffix != '.' && (suffix > (char *)file))
			suffix--;
		if (strcasecmp(suffix, ".ttf") == 0) {
			font_name = HPDF_LoadTTFontFromFile(pdf->pdf, (char *)file, HPDF_TRUE);
			break;
		}
		if (strcasecmp(suffix, ".pfb") == 0) {
			gchar *afm = g_strdup((char *)file);
			gint pos = (suffix - (char *)file);

			afm[pos + 1] = 'a';
			afm[pos + 2] = 'f';
			afm[pos + 3] = 'm';

			font_name = HPDF_LoadType1FontFromFile(pdf->pdf, afm, (char *)file);
			g_free(afm);
			break;
		}
	}

	FcFontSetDestroy(fs);
	FcObjectSetDestroy(os);
	FcPatternDestroy(pat);
	FcConfigDestroy(config);

	return font_name;
}

DLL_EXPORT_SYM gboolean rpdf_set_font(struct rpdf *pdf, const gchar *font, const gchar *style, const gchar *encoding, gdouble size) {
	gint i = 0;
	const gchar *font_name = NULL;
	HPDF_Font hfont;
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	gchar *three;

	three = g_strconcat(font, style, encoding, NULL);
	hfont = g_hash_table_lookup(pdf->fonts, three);

	if (hfont == NULL) {
		while (base_fonts[i]) {
			if (strcmp(base_fonts[i], font) == 0) {
				font_name = base_fonts[i];
				break;
			}
			i++;
		}

		if (font_name == NULL)
			font_name = rpdf_embed_font_fc(pdf, font, style);

		if (font_name == NULL) {
			rpdf_error("FONT NOT FOUND: font name '%s' font style '%s'\n", font, style);
			g_free(three);
			return FALSE;
		}

		hfont = HPDF_GetFont(pdf->pdf, font_name, encoding);
		g_hash_table_insert(pdf->fonts, three, hfont);

		if (strcmp(font, font_name) != 0) {
			three = g_strconcat(font_name, style, encoding, NULL);
			g_hash_table_insert(pdf->fonts, three, hfont);
		}
	} else {
		g_free(three);
	}

	page_info->current_font = hfont;

	HPDF_Page_SetFontAndSize(page_info->page, hfont, size);

	page_info->font_size = size;

	return TRUE;
}

DLL_EXPORT_SYM void rpdf_set_font_size(struct rpdf *pdf, gdouble size) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];

	page_info->font_size = size;
	HPDF_Page_SetFontAndSize(page_info->page, page_info->current_font, size);
}

static void rpdf_text_common(struct rpdf *pdf, HPDF_Page page, gdouble x, gdouble y, gdouble angle, const gchar *text) {
	HPDF_Font font;
	const gchar *encoding;
	gchar *new_text;
	gsize foo1;
	GIConv conv;
	gdouble angle1 = M_PI * angle / 180.0;
	gdouble text_sin = sin(angle1);
	gdouble text_cos = cos(angle1);

	font = HPDF_Page_GetCurrentFont(page);
	encoding = HPDF_Font_GetEncodingName(font);

	conv = g_hash_table_lookup(pdf->convs, encoding);
	if (conv == NULL) {
		conv = g_iconv_open(encoding, "UTF-8");
		g_hash_table_insert(pdf->convs, g_strdup(encoding), conv);
	}

	new_text = g_convert_with_iconv(text, strlen(text), conv, &foo1, &foo1, NULL);

	HPDF_Page_BeginText(page);

#if 0
	/* TODO - use externally set color */
	HPDF_Page_SetGrayStroke(page, 0.0);
	HPDF_Page_SetGrayFill(page, 0.0);
#endif

	HPDF_Page_SetTextMatrix(page, text_cos, text_sin, -text_sin, text_cos, x * RPDF_DPI, y * RPDF_DPI);
	HPDF_Page_ShowText(page, new_text);

	HPDF_Page_EndText(page);

	g_free(new_text);
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

	if (user_data == NULL)
		return;

	dt = user_data;
	page_info = pdf->page_info[dt->page_number];

	HPDF_Page_SetFontAndSize(page_info->page, dt->font, dt->fontsize);
	HPDF_Page_SetRGBFill(page_info->page, dt->r, dt->g, dt->b);
	HPDF_Page_SetRGBStroke(page_info->page, dt->r, dt->g, dt->b);
	rpdf_text_common(pdf, page_info->page, dt->x, dt->y, dt->angle, text);

	g_free(dt);
}

DLL_EXPORT_SYM void rpdf_image(struct rpdf *pdf, gdouble x, gdouble y, gdouble width, gdouble height, gint image_type, gchar *file_name) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	HPDF_Image image;
	struct stat st;

	if (stat(file_name, &st) != 0)
		return;

	switch (image_type) {
	case RPDF_IMAGE_PNG:
		image = HPDF_LoadPngImageFromFile(pdf->pdf, file_name);
		break;
	case RPDF_IMAGE_JPEG:
		image = HPDF_LoadJpegImageFromFile(pdf->pdf, file_name);
		break;
	default:
		rpdf_error("rpdf_image: Only PNG and JPG images are supported\n");
		return;
	}

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

	HPDF_Page_CreateURILinkAnnot(page_info->page, rect, url);
}

DLL_EXPORT_SYM void rpdf_moveto(struct rpdf *pdf, gdouble x, gdouble y) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];

	if (isnan(x))
		x = 0;
	if (isnan(y))
		y = 0;

	HPDF_Page_MoveTo(page_info->page, x * RPDF_DPI, y * RPDF_DPI);
}

DLL_EXPORT_SYM void rpdf_set_line_width(struct rpdf *pdf, gdouble width) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];

	HPDF_Page_SetLineWidth(page_info->page, width);
}

DLL_EXPORT_SYM void rpdf_lineto(struct rpdf *pdf, gdouble x, gdouble y) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];

	if (isnan(x))
		x = 0;
	if (isnan(y))
		y = 0;

	HPDF_Page_LineTo(page_info->page, x * RPDF_DPI, y * RPDF_DPI);
}

DLL_EXPORT_SYM void rpdf_closepath(struct rpdf *pdf) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	HPDF_Page_ClosePath(page_info->page);
}

DLL_EXPORT_SYM void rpdf_stroke(struct rpdf *pdf) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	HPDF_Page_Stroke(page_info->page);
}

DLL_EXPORT_SYM void rpdf_rect(struct rpdf *pdf, gdouble x, gdouble y, gdouble width, gdouble height) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	HPDF_Page_Rectangle(page_info->page, x * RPDF_DPI, y * RPDF_DPI, width * RPDF_DPI, height * RPDF_DPI);
}

DLL_EXPORT_SYM void rpdf_fill(struct rpdf *pdf) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	HPDF_Page_Fill(page_info->page);
}

DLL_EXPORT_SYM void rpdf_setrgbcolor(struct rpdf *pdf, gdouble r, gdouble g, gdouble b) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	HPDF_Page_SetRGBFill(page_info->page, r, g, b);
	HPDF_Page_SetRGBStroke(page_info->page, r, g, b);
	page_info->r = r;
	page_info->g = g;
	page_info->b = b;
}

DLL_EXPORT_SYM gdouble rpdf_text_width(struct rpdf *pdf, const gchar *text) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	HPDF_TextWidth w;

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
		HPDF_Page_MoveTo(page_info->page, x, y);
		HPDF_Page_LineTo(page_info->page, a0, b0);
	} else {
		HPDF_Page_MoveTo(page_info->page, a0,b0);
	}
	for (i = 1; i <= nsegs; i++) {
		t1 = ((gfloat)i * dt) + start_angle;
		a1 = x + (radius * cos(t1));
		b1 = y + (radius * sin(t1));
		c1 = -radius * sin(t1);
		d1 = radius * cos(t1);
		HPDF_Page_CurveTo(page_info->page, (a0 + (c0 * dtm)), ((b0 + (d0 * dtm))), (a1 - (c1 * dtm)), ((b1 - (d1 * dtm))), a1, b1);
		a0 = a1;
		b0 = b1;
		c0 = c1;
		d0 = d1;
	}
	if (total_angle < DEGREE_2_RAD(360.0)) { /* pizza :) */
		HPDF_Page_LineTo(page_info->page, x, y);
	}
}

DLL_EXPORT_SYM gchar *rpdf_get_buffer(struct rpdf *pdf, guint *length) {
	guchar *buf;

	HPDF_SaveToStream(pdf->pdf);
	*length = HPDF_GetStreamSize(pdf->pdf);

	buf = g_malloc(*length);

	HPDF_ResetStream(pdf->pdf);

	HPDF_ReadFromStream(pdf->pdf, buf, length);

	return (gchar *)buf;
}

DLL_EXPORT_SYM void rpdf_free(struct rpdf *pdf) {
	gint i;

	HPDF_Free(pdf->pdf);

	g_hash_table_destroy(pdf->fonts);
	g_hash_table_destroy(pdf->convs);

	for (i = 0; i < pdf->page_count; i++)
		g_free(pdf->page_info[i]);
	g_free(pdf->page_info);
	g_free(pdf);
}
