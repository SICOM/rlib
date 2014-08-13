/*
 *  Copyright (C) 2003-2014 SICOM Systems, INC.
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

#ifdef RPDF_COMPRESS_STREAM
#include <zlib.h>
#endif

#include "rpdf.h"

#ifndef O_BINARY
#define	O_BINARY	(0)
#endif

struct _rpdf_paper rpdf_paper [] = {
	{RPDF_PAPER_LETTER,612, 792, "LETTER"},
	{RPDF_PAPER_LEGAL, 612, 1008, "LEGAL"},
	{RPDF_PAPER_A4, 595, 842, "A4"},
	{RPDF_PAPER_B5, 499, 708, "B5"},
	{RPDF_PAPER_C5, 459, 649, "C5"},
	{RPDF_PAPER_DL, 312, 624, "DL"},
	{RPDF_PAPER_EXECUTIVE, 522, 756, "EXECUTIVE"},
	{RPDF_PAPER_COMM10, 297, 684, "COMM10"},
	{RPDF_PAPER_MONARCH, 279, 540, "MONARCH"},
	{RPDF_PAPER_FILM35MM, 528, 792, "FILM35MM"},
	{-1, 0, 0, "" },
};

gint rpdf_courier_width[RPDF_MAX_CHARS] = {600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600,600600,600,600,600,600,600};
gint rpdf_helvetica_width[RPDF_MAX_CHARS] = {278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,355,556,556,889,667,191,333,333,389,584,	278,333,278,278,556,556,556,556,556,556,556,556,556,556,278,278,584,584,584,556,1015,667,667,722,722,667,611,778,722,278,500,667,556,833,722,778,667,778,722,667,611,722,667,944,667,667,611,278,278,278,469,556,333,556,556,500,556,556,278,556,556,222,222,500,222,833,556,556,556,556,333,500,278,556,500,722,500,500,500,334,260,334,584,350,556,350,222,556,333,1000,556,556,333,1000,667,333,1000,350,611,350,350,222,222,333,333,350,556,1000,333,1000,500,333,944,350,500,667,278,333,556,556,556,556,260,556,333,737,370,556,584,333,737,333,400,584,333,333,333,556,537,278,333,333,365,556,834,834,834,611,667,667,667,667,667,667,1000,722,667,667,667,667,278,278,278,278,722,722,778,778,778,778,778,584,778,722,722,722,722,667,667,611,556,556,556,556,556,556,889,500,556,556,556,556,278,278,278,278,556,556,556,556,556,556,556,584,611,556,556,556,556,500,556,500};
gint rpdf_helvetica_bold_width[RPDF_MAX_CHARS] = {278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,333,474,556,556,889,722,238,333,333,389,584,278,333,278,278,556,556,556,556,556,556,556,556,556,556,333,333,584,584,584,611,975,722,722,722,722,667,611,778,722,278,556,722,611,833,722,778,667,778,722,667,611,722,667,944,667,667,611,333,278,333,584,556,333,556,611,556,611,556,333,611,611,278,278,556,278,889,611,611,611,611,389,556,333,611,556,778,556,556,500,389,280,389,584,350,556,350,278,556,500,1000,556,556,333,1000,667,333,1000,350,611,350,350,278,278,500,500,350,556,1000,333,1000,556,333,944,350,500,667,278,333,556,556,556,556,280,556,333,737,370,556,584,333,737,333,400,584,333,333,333,611,556,278,333,333,365,556,834,834,834,611,722,722,722,722,722,722,1000,722,667,667,667,667,278,278,278,278,722,722,778,778,778,778,778,584,778,722,722,722,722,667,667,611,556,556,556,556,556,556,889,556,556,556,556,556,278,278,278,278,611,611,611,611,611,611,611,584,611,611,611,611,611,556,611,556};
gint rpdf_helvetica_italics_width[RPDF_MAX_CHARS] = {278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,355,556,556,889,667,191,333,333,389,584,278,333,278,278,556,556,556,556,556,556,556,556,556,556,278,278,584,584,584,556,1015,667,667,722,722,667,611,778,722,278,500,667,556,833,722,778,667,778,722,667,611,722,667,944,667,667,611,278,278,278,469,556,333,556,556,500,556,556,278,556,556,222,222,500,222,833,556,556,556,556,333,500,278,556,500,722,500,500,500,334,260,334,584,350,556,350,222,556,333,1000,556,556,333,1000,667,333,1000,350,611,350,350,222,222,333,333,350,556,1000,333,1000,500,333,944,350,500,667,278,333,556,556,556,556,260,556,333,737,370,556,584,333,737,333,400,584,333,333,333,556,537,278,333,333,365,556,834,834,834,611,667,667,667,667,667,667,1000,722,667,667,667,667,278,278,278,278,722,722,778,778,778,778,778,584,778,722,722,722,722,667,667,611,556,556,556,556,556,556,889,500,556,556,556,556,278,278,278,278,556,556,556,556,556,556,556,584,611,556,556,556,556,500,556,500};
gint rpdf_helvetica_bold_italics_width[RPDF_MAX_CHARS] = {278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,333,474,556,556,889,722,238,333,333,389,584,278,333,278,278,556,556,556,556,556,556,556,556,556,556,333,333,584,584,584,611,975,722,722,722,722,667,611,778,722,278,556,722,611,833,722,778,667,778,722,667,611,722,667,944,667,667,611,333,278,333,584,556,333,556,611,556,611,556,333,611,611,278,278,556,278,889,611,611,611,611,389,556,333,611,556,778,556,556,500,389,280,389,584,350,556,350,278,556,500,1000,556,556,333,1000,667,333,1000,350,611,350,350,278,278,500,500,350,556,1000,333,1000,556,333,944,350,500,667,278,333,556,556,556,556,280,556,333,737,370,556,584,333,737,333,400,584,333,333,333,611,556,278,333,333,365,556,834,834,834,611,722,722,722,722,722,722,1000,722,667,667,667,667,278,278,278,278,722,722,778,778,778,778,778,584,778,722,722,722,722,667,667,611,556,556,556,556,556,556,889,556,556,556,556,556,278,278,278,278,611,611,611,611,611,611,611,584,611,611,611,611,611,556,611,556};
gint rpdf_times_width[RPDF_MAX_CHARS] = {250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,333,408,500,500,833,778,180,333,333,500,564,250,333,250,278,500,500,500,500,500,500,500,500,500,500,278,278,564,564,564,444,921,722,667,667,722,611,556,722,722,333,389,722,611,889,722,722,556,722,667,556,611,722,722,944,722,722,611,333,278,333,469,500,333,444,500,444,500,444,333,500,500,278,278,500,278,778,500,500,500,500,333,389,278,500,500,722,500,500,444,480,200,480,541,350,500,350,333,500,444,1000,500,500,333,1000,556,333,889,350,611,350,350,333,333,444,444,350,500,1000,333,980,389,333,722,350,444,722,250,333,500,500,500,500,200,500,333,760,276,500,564,333,760,333,400,564,300,300,333,500,453,250,333,300,310,500,750,750,750,444,722,722,722,722,722,722,889,667,611,611,611,611,333,333,333,333,722,722,722,722,722,722,722,564,722,722,722,722,722,722,556,500,444,444,444,444,444,444,667,444,444,444,444,444,278,278,278,278,500,500,500,500,500,500,500,564,500,500,500,500,500,500,500,500};
gint rpdf_times_bold_width[RPDF_MAX_CHARS] = {250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,333,555,500,500,1000,833,278,333,333,500,570,250,333,250,278,500,500,500,500,500,500,500,500,500,500,333,333,570,570,570,500,930,722,667,722,722,667,611,778,778,389,500,778,667,944,722,778,611,778,722,556,667,722,722,1000,722,722,667,333,278,333,581,500,333,500,556,444,556,444,333,500,556,278,333,556,278,833,556,500,556,556,444,389,333,556,500,722,500,500,444,394,220,394,520,350,500,350,333,500,500,1000,500,500,333,1000,556,333,1000,350,667,350,350,333,333,500,500,350,500,1000,333,1000,389,333,722,350,444,722,250,333,500,500,500,500,220,500,333,747,300,500,570,333,747,333,400,570,300,300,333,556,540,250,333,300,330,500,750,750,750,500,722,722,722,722,722,722,1000,722,667,667,667,667,389,389,389,389,722,722,778,778,778,778,778,570,778,722,722,722,722,722,611,556,500,500,500,500,500,500,722,444,444,444,444,444,278,278,278,278,500,556,500,500,500,500,500,570,500,556,556,556,556,500,556,500};
gint rpdf_times_italics_width[RPDF_MAX_CHARS] = {250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,333,420,500,500,833,778,214,333,333,500,675,250,333,250,278,500,500,500,500,500,500,500,500,500,500,333,333,675,675,675,500,920,611,611,667,722,611,611,722,722,333,444,667,556,833,667,722,611,722,611,500,556,722,611,833,611,556,556,389,278,389,422,500,333,500,500,444,500,444,278,500,500,278,278,444,278,722,500,500,500,500,389,389,278,500,444,667,444,444,389,400,275,400,541,350,500,350,333,500,556,889,500,500,333,1000,500,333,944,350,556,350,350,333,333,556,556,350,500,889,333,980,389,333,667,350,389,556,250,389,500,500,500,500,275,500,333,760,276,500,675,333,760,333,400,675,300,300,333,500,523,250,333,300,310,500,750,750,750,500,611,611,611,611,611,611,889,667,611,611,611,611,333,333,333,333,722,667,722,722,722,722,722,675,722,722,722,722,722,556,611,500,500,500,500,500,500,500,667,444,444,444,444,444,278,278,278,278,500,500,500,500,500,500,500,675,500,500,500,500,500,444,500,444};
gint rpdf_times_bold_italics_width[RPDF_MAX_CHARS] = {250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,389,555,500,500,833,778,278,333,333,500,570,250,333,250,278,500,500,500,500,500,500,500,500,500,500,333,333,570,570,570,500,832,667,667,667,722,667,667,722,778,389,500,667,611,889,722,722,611,722,667,556,611,722,667,889,667,611,611,333,278,333,570,500,333,500,500,444,500,444,333,500,556,278,278,500,278,778,556,500,500,500,389,389,278,556,444,667,500,444,389,348,220,348,570,350,500,350,333,500,500,1000,500,500,333,1000,556,333,944,350,611,350,350,333,333,500,500,350,500,1000,333,1000,389,333,722,350,389,611,250,389,500,500,500,500,220,500,333,747,266,500,606,333,747,333,400,570,300,300,333,576,500,250,333,300,300,500,750,750,750,500,667,667,667,667,667,667,944,667,667,667,667,667,389,389,389,389,722,722,722,722,722,722,722,570,722,722,722,722,722,611,611,500,500,500,500,500,500,500,722,444,444,444,444,444,278,278,278,278,500,556,500,500,500,500,500,570,500,556,556,556,556,444,500,444};
gint rpdf_symbol_width[RPDF_MAX_CHARS] = {250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,333,713,500,549,833,778,439,333,333,500,549,250,549,250,278,500,500,500,500,500,500,500,500,500,500,278,278,549,549,549,444,549,722,667,722,612,611,763,603,722,333,631,722,686,889,722,722,768,741,556,592,611,690,439,768,645,795,611,333,863,333,658,500,500,631,549,549,494,439,521,411,603,329,603,549,549,576,521,549,549,521,549,603,439,576,713,686,493,686,494,480,200,480,549,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,750,620,247,549,167,713,500,753,753,753,753,1042,987,603,987,603,400,549,411,549,549,713,494,460,549,549,549,549,1000,603,1000,658,823,686,795,987,768,768,823,768,768,713,713,713,713,713,713,713,768,713,790,790,890,823,549,250,713,603,603,1042,987,603,987,603,494,329,790,790,786,713,384,384,384,384,384,384,494,494,494,494,0,329,274,686,686,686,384,384,384,384,384,384,494,494,494,0};
gint rpdf_dingbats_width[RPDF_MAX_CHARS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,278,974,961,974,980,719,789,790,791,690,960,939,549,855,911,933,911,945,974,755,846,762,761,571,677,763,760,759,754,494,552,537,577,692,786,788,788,790,793,794,816,823,789,841,823,833,816,831,923,744,723,749,790,792,695,776,768,792,759,707,708,682,701,826,815,789,789,707,687,696,689,786,787,713,791,785,791,873,761,762,762,759,759,892,892,788,784,438,138,277,415,392,392,668,668,0,390,390,317,317,276,276,509,509,410,410,234,234,334,334,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,732,544,544,910,667,760,760,776,595,694,626,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,788,894,838,1016,458,748,924,748,918,927,928,928,834,873,828,924,924,917,930,931,463,883,836,836,867,867,696,696,874,0,874,760,946,771,865,771,888,967,888,831,873,927,970,918,0};

struct _rpdf_fonts rpdf_fonts[] =  {
	{"Courier", "Type1", rpdf_courier_width},
	{"Courier-Bold", "Type1", rpdf_courier_width},
	{"Courier-Oblique", "Type1", rpdf_courier_width},
	{"Courier-BoldOblique", "Type1", rpdf_courier_width},
	{"Helvetica", "Type1", rpdf_helvetica_width},
	{"Helvetica-Bold", "Type1", rpdf_helvetica_bold_width},
	{"Helvetica-Oblique", "Type1", rpdf_helvetica_italics_width},
	{"Helvetica-BoldOblique", "Type1", rpdf_helvetica_bold_italics_width},
	{"Times-Roman", "Type1", rpdf_times_width},
	{"Times-Bold", "Type1", rpdf_times_bold_width},
	{"Times-Italic", "Type1", rpdf_times_italics_width},
	{"Times-BoldItalic", "Type1", rpdf_times_bold_italics_width},
	{"Symbol", "Type1", rpdf_symbol_width},
	{"ZapfDingbats", "Type1", rpdf_dingbats_width},
	{"", "", NULL }
};

#define M_SOF0  0xC0		/* Start Of Frame N */
#define M_SOF1  0xC1		/* N indicates which compression process */
#define M_SOF2  0xC2		/* Only SOF0-SOF2 are now in common use */
#define M_SOF3  0xC3
#define M_SOF5  0xC5		/* NB: codes C4 and CC are NOT SOF markers */
#define M_SOF6  0xC6
#define M_SOF7  0xC7
#define M_SOF9  0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI   0xD8		/* Start Of Image (beginning of datastream) */
#define M_EOI   0xD9		/* End Of Image (end of datastream) */
#define M_SOS   0xDA		/* Start Of Scan (begins compressed data) */
#define M_APP0	 0xE0		/* Application-specific marker, type N */
#define M_APP12 0xEC		/* (we don't bother to list all 16 APPn's) */
#define M_COM   0xFE		/* COMment */

#define DEGREE_2_RAD(x) (x*M_PI/180.0)

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

static GString * obj_printf(GString *obj, const gchar *fmt, ...) {
	va_list vl;
	gchar *result = NULL;

	if(obj == NULL)
		obj = g_string_new("");

	va_start(vl, fmt);
	result = g_strdup_vprintf(fmt, vl);
	va_end(vl);

	if(result != NULL) {
		g_string_append(obj, result);
		g_free(result);
	}

	return obj;
}

static GString * obj_concat(GString *obj, gchar *data) {
	if(obj == NULL)
		obj = g_string_new("");

	g_string_append(obj, data);
	return obj;
}

static gint rpdf_object_number_skip(struct rpdf *pdf) {
	return ++pdf->object_count;
}

static void rpdf_object_insert(struct rpdf *pdf, gint object_number, gboolean put_wrapper, GString *contents, gchar *stream, gint stream_length) {
	struct rpdf_object *object = g_new0(struct rpdf_object, 1);
	object->object_number = object_number;
	object->put_wrapper = put_wrapper;
	object->contents = contents;
	object->stream = stream;
	object->stream_length = stream_length;
	pdf->objects = g_slist_insert(pdf->objects, object, object_number - 1);
}

static gint rpdf_object_append(struct rpdf *pdf, gboolean put_wrapper, GString *contents, gchar *stream, gint stream_length) {
	struct rpdf_object *object = g_new0(struct rpdf_object, 1);
	object->object_number = ++pdf->object_count;
	object->put_wrapper = put_wrapper;
	object->contents = contents;
	object->stream = stream;
	object->stream_length = stream_length;
	pdf->objects = g_slist_append(pdf->objects, object);
	return object->object_number;
}

static struct rpdf_stream * rpdf_stream_new(gint type, gpointer data) {
	struct rpdf_stream *stream = g_new0(struct rpdf_stream, 1);
	stream->type = type;
	stream->data = data;
	return stream;
}

static void rpdf_stream_append(struct rpdf *pdf, struct rpdf_stream *stream) {
	pdf->page_contents[pdf->current_page] = g_slist_append(pdf->page_contents[pdf->current_page], stream);
}

static gboolean rpdf_out_string(struct rpdf *rpdf, const gchar *output) {
	gint size = strlen(output);
	rpdf->out_buffer = g_realloc(rpdf->out_buffer, rpdf->size+size);
	memcpy(rpdf->out_buffer + rpdf->size, output, size);
	rpdf->size += size;
	return TRUE;
}

static gboolean rpdf_out_gstring(struct rpdf *rpdf, const GString *output) {
	rpdf->out_buffer = g_realloc(rpdf->out_buffer, rpdf->size+output->len);
	memcpy(rpdf->out_buffer + rpdf->size, output->str, output->len);
	rpdf->size += output->len;
	return TRUE;
}


static gboolean rpdf_out_binary(struct rpdf *rpdf, const gchar *output, gint size) {
	rpdf->out_buffer = g_realloc(rpdf->out_buffer, rpdf->size+size);
	memcpy(rpdf->out_buffer + rpdf->size, output, size);
	rpdf->size += size;
	return TRUE;
}

static void rpdf_finalize_objects(gpointer data, gpointer user_data) {
	gchar buf[128];
	struct rpdf_object *object = data;
	struct rpdf *pdf = user_data;

	pdf->xref = g_slist_append(pdf->xref, GINT_TO_POINTER(pdf->size));
	sprintf(buf, "%d 0 obj\n", object->object_number);
	rpdf_out_string(pdf, buf);
	if(object->put_wrapper)
		rpdf_out_string(pdf, "<<\n");
	rpdf_out_gstring(pdf, object->contents);
	if(object->put_wrapper)
		rpdf_out_string(pdf, ">>\n");
	if(object->stream != NULL) {
		rpdf_out_string(pdf, "stream\n");
		rpdf_out_binary(pdf, object->stream, object->stream_length);
		rpdf_out_string(pdf, "\nendstream\n");
		g_free(object->stream);
	}		
	rpdf_out_string(pdf, "endobj\n");
}

static void rpdf_finalize_xref(gpointer data, gpointer user_data) {
	gint spot = GPOINTER_TO_INT(data);
	struct rpdf *pdf = user_data;
	gchar buf[128];
	sprintf(buf, "%010d 00000 n \n", spot);
	rpdf_out_string(pdf, buf);
}

static void rpdf_stream_font_destroyer(gpointer data, gpointer user_data) {
	g_free(data);
}

static void rpdf_object_destroyer(gpointer data, gpointer user_data) {
	struct rpdf_object *object = data;
	g_string_free(object->contents, TRUE);
	g_free(object);
}

static void rpdf_make_page_stream(gpointer data, gpointer user_data) {
	struct rpdf_stream *stream = data;
	gchar *result = NULL, extra[512];
	struct rpdf *pdf = user_data;
	extra[0] = 0;
	
	if(stream->type == RPDF_TYPE_FONT || stream->type == RPDF_TYPE_TEXT) {
		if(pdf->text_on == FALSE) {
			pdf->text_on = TRUE;
			sprintf(extra, "BT\n");
		}
	}
	if(stream->type == RPDF_TYPE_RECT || stream->type == RPDF_TYPE_FILL || stream->type == RPDF_TYPE_IMAGE || stream->type == RPDF_TYPE_LINE
		|| stream->type == RPDF_TYPE_MOVE || stream->type == RPDF_TYPE_CLOSEPATH || stream->type == RPDF_TYPE_STROKE || stream->type == RPDF_TYPE_WIDTH 
		||	stream->type == RPDF_TYPE_ARC) {
		if(pdf->text_on == TRUE) {
			pdf->text_on = FALSE;
			sprintf(extra, "ET\n");
		}
	}

	if(stream->type == RPDF_TYPE_FONT) {
		struct rpdf_stream_font *stream_font = stream->data;
		gchar *font_object;
		result = g_strdup_printf("%s/F%s %.03lf Tf\n", extra, stream_font->font_object->name, stream_font->size);
		font_object = g_hash_table_lookup(pdf->page_fonts, stream_font->font_object->name);
		if(font_object == NULL) {
			g_hash_table_insert(pdf->page_fonts, stream_font->font_object->name, stream_font);
		}
	} else if(stream->type == RPDF_TYPE_TEXT) {
		struct rpdf_stream_text *stream_text = stream->data;
		gdouble angle = M_PI * stream_text->angle / 180.0;
		gdouble text_sin= sin(angle);
		gdouble text_cos = cos(angle);
		char *text = g_strdup(stream_text->text);
		result = g_strdup_printf("%s%.04f %.04f %.04f %.04f %.04f %.04f Tm\n(%s) Tj\n", extra, text_cos, text_sin, -text_sin, text_cos, stream_text->x*RPDF_DPI, stream_text->y*RPDF_DPI, text); 
		g_free(stream_text->text);
		g_free(stream_text);
		g_free(text);
	} else if(stream->type == RPDF_TYPE_RECT) {
		struct rpdf_stream_rect *stream_rect = stream->data;
		result = g_strdup_printf("%s%.03f %.03f %.03f %.03f re\n", extra, stream_rect->x*RPDF_DPI, stream_rect->y*RPDF_DPI, stream_rect->width*RPDF_DPI, stream_rect->height*RPDF_DPI);
		g_free(stream_rect);
	} else if(stream->type == RPDF_TYPE_FILL) {
		result = g_strdup_printf("%sf\n", extra);
	} else if(stream->type == RPDF_TYPE_COLOR) {
		struct rpdf_stream_color *stream_color = stream->data;
		result = g_strdup_printf("%.04f %.04f %.04f rg\n%.04f %.04f %.04f RG\n", stream_color->r, stream_color->g, stream_color->b, stream_color->r, stream_color->g, stream_color->b);
		g_free(stream_color);
	} else if(stream->type == RPDF_TYPE_IMAGE) {
		struct rpdf_images *image = stream->data;
		result = g_strdup_printf("%sq\n1.0000 0.0000 0.0000 1.0000 %.04f %.04f cm\n%.04f 0.0000 0.0000 %.04f 0.0000 0.0000 cm\n/IMrpdf%d Do\nQ\n", 
			extra, image->x*RPDF_DPI, image->y*RPDF_DPI, image->width, image->height, image->number);
	} else if(stream->type == RPDF_TYPE_MOVE) {
		struct rpdf_stream_rect *stream_point = stream->data;
		result = g_strdup_printf("%s%.03f %.03f m\n", extra, stream_point->x*RPDF_DPI, stream_point->y*RPDF_DPI);
		g_free(stream_point);
	} else if(stream->type == RPDF_TYPE_WIDTH) {
		gdouble *width = stream->data;
		result = g_strdup_printf("%s%.04f w\n", extra, *width);
		g_free(width);
	} else if(stream->type == RPDF_TYPE_LINE) {
		struct rpdf_stream_rect *stream_point = stream->data;
		result = g_strdup_printf("%s%.03f %.03f l\n", extra, stream_point->x*RPDF_DPI, stream_point->y*RPDF_DPI);
		g_free(stream_point);
	} else if(stream->type == RPDF_TYPE_CLOSEPATH) {
		result = g_strdup_printf("%sh\n", extra);
	} else if(stream->type == RPDF_TYPE_STROKE) {
		result = g_strdup_printf("%sS\n", extra);
	} else if(stream->type == RPDF_TYPE_ARC) {
		char buf[1024];
		struct rpdf_stream_arc *arc = stream->data;
		gdouble save_x, save_y;
		gdouble x = arc->x;
		gdouble y = arc->y;
		gdouble start_angle = arc->start_angle;
		gdouble end_angle = arc->end_angle;
		gdouble radius = arc->radius;
		gint nsegs = 8,i;
		gdouble total_angle,dt,dtm,t1,a0,b0,c0,d0,a1,b1,c1,d1;

		x *= RPDF_DPI;
		y *= RPDF_DPI;
		radius *= RPDF_DPI;
		start_angle = DEGREE_2_RAD(start_angle);
		end_angle = DEGREE_2_RAD(end_angle);
		total_angle = end_angle - start_angle;
		dt = total_angle / (gfloat)nsegs;
		dtm = dt/3.0;
		t1 = start_angle;
		a0 = x + (radius * cos(t1));
		b0 = y + (radius * sin(t1));
		c0 = -radius * sin(t1);
		d0 = radius * cos(t1);
		save_x = a0;
		save_y = b0;
		if(total_angle < DEGREE_2_RAD(360.0)) {  /* Pie Slices */
			sprintf(buf, "%s%.03f %.03f m\n", extra, x, y);
			sprintf(buf, "%s%.03f %.03f l\n", buf, a0, b0);
		} else {
			sprintf(buf, "%s%.03f %.03f m\n", extra, a0,b0);
		
		}		
		for (i = 1; i <= nsegs; i++) {
			t1 = ((gfloat)i * dt) + start_angle;
			a1 = x + (radius * cos(t1));
			b1 = y + (radius * sin(t1));
			c1 = -radius * sin(t1);
			d1 = radius * cos(t1);
			sprintf(buf, "%s%.02f %.02f %.02f %.02f %.02f %.02f c\n", buf, (a0 + (c0 * dtm)),((b0 + (d0 * dtm))),(a1 - (c1 * dtm)),((b1 - (d1 * dtm))),a1,(b1));
			a0 = a1;
			b0 = b1;
			c0 = c1;
			d0 = d1;
		}
		if(total_angle < DEGREE_2_RAD(360.0)) { /* pizza :) */
			sprintf(buf, "%s%.03f %.03f l\n", buf, x, y);
		}
		result = g_strdup_printf("%s", buf);
		g_free(arc);
	} else if(stream->type == RPDF_TYPE_TEXT_CB) {
		struct rpdf_stream_text_callback *stream_text_callback = stream->data;
		gchar *callback_data;
		gdouble angle = M_PI * stream_text_callback->angle / 180.0;
		gdouble text_sin= sin(angle);
		gdouble text_cos = cos(angle);
		char *text;
		callback_data = g_malloc(stream_text_callback->len+1);
	    stream_text_callback->callback(callback_data, stream_text_callback->len+1, stream_text_callback->user_data);
		text = g_strdup(callback_data);
		result = g_strdup_printf("%s%.04f %.04f %.04f %.04f %.04f %.04f Tm\n(%s) Tj\n", extra, text_cos, text_sin, -text_sin, text_cos, stream_text_callback->x*RPDF_DPI, stream_text_callback->y*RPDF_DPI, text); 
		g_free(text);
		g_free(callback_data);
		g_free(stream_text_callback);
	}

	g_free(stream);
	if(pdf->page_data == NULL) {
		pdf->page_data = g_string_new(result);
		g_free(result);
	} else {
		g_string_append(pdf->page_data, result);
		g_free(result);
	}
}

static void rpdf_make_page_image_obj(gpointer data, gpointer user_data) {
	struct rpdf *pdf = user_data;
	struct rpdf_images *image = data;
	struct rpdf_image_jpeg *jpeg = image->metadata;
	struct rpdf_image_png *png = image->metadata;
	GString *obj = NULL;
	long width, height, bpc, color_space;
	gint object_number;
	gboolean did_color_space = FALSE;
	
	if(image->image_type == RPDF_IMAGE_JPEG) {
		width = jpeg->width;
		height = jpeg->height;
		bpc = jpeg->precision;
		color_space = jpeg->components;
	} else {
		width = png->width;
		height = png->height;	
		bpc = png->bpc;
		color_space = png->ct;
	}

	obj = obj_concat(NULL, "/Type /XObject\n");
	obj = obj_concat(obj, "/Subtype /Image\n");	
	obj = obj_printf(obj, "/Name /IMrpdf%d\n", image->number);
	obj = obj_printf(obj, "/Width %d\n", width);
	obj = obj_printf(obj, "/Height %d\n", height);
	obj = obj_printf(obj, "/BitsPerComponent %d\n", bpc);

	if(image->image_type == RPDF_IMAGE_PNG) {
		if(png->trans_list != NULL) {
			GSList *tmp;
			obj = obj_printf(obj, "/Mask [");
			for(tmp = png->trans_list;tmp != NULL; tmp = tmp->next) {
				obj = obj_printf(obj, "%d %d ", GPOINTER_TO_INT(tmp->data), GPOINTER_TO_INT(tmp->data));
			
			}
			obj = obj_printf(obj, "]\n");
			g_slist_free(png->trans_list);		
		}


		if(png->ct == 3) { //Indexed
			obj = obj_printf(obj, "/ColorSpace [/Indexed /DeviceRGB %d %d 0 R]\n", png->palette->len/3-1, pdf->object_count+2);
			did_color_space = TRUE;
		} 
		obj = obj_concat(obj, "/Filter /FlateDecode\n");	
		obj = obj_concat(obj, png->params);
	} else {
		obj = obj_concat(obj, "/Filter /DCTDecode\n");
	}

	if(!did_color_space) {
		obj = obj_concat(obj, "/ColorSpace /");

		if(color_space == 3)
			obj = obj_concat(obj, "DeviceRGB");
		else if(color_space == 4)
			obj = obj_concat(obj, "DeviceCMYK");
		else 
			obj = obj_concat(obj, "DeviceGray");

		obj = obj_concat(obj, "\n");
	}
	
	if(image->image_type == RPDF_IMAGE_JPEG) {
		obj = obj_printf(obj, "/Length %ld\n", image->length);
		object_number = rpdf_object_append(pdf, TRUE, obj, image->data, image->length);
	} else {
		obj = obj_printf(obj, "/Length %ld\n", png->data->len);
		object_number = rpdf_object_append(pdf, TRUE, obj, png->data->str, png->data->len);
	}

	pdf->working_obj = obj_printf(pdf->working_obj, "/IMrpdf%d %d 0 R\n", image->number, object_number);

	if(image->image_type == RPDF_IMAGE_PNG) {
		if(png->ct == 3) {
			GString *obj = NULL;
			obj = obj_printf(obj, "<</Length %d>>\n", png->palette->len);
			object_number = rpdf_object_append(pdf, FALSE, obj, png->palette->str, png->palette->len);

		}
		g_free(image->data);	
	}
	
	g_free(image->metadata);
	g_free(image);
}

static void rpdf_make_page_annot_obj(gpointer data, gpointer user_data) {
	struct rpdf *pdf = user_data;
	struct rpdf_annots *annot = data;
	gint object_number;
	GString *obj = NULL;
	obj = obj_concat(NULL, "/Type /Annot\n");
	obj = obj_concat(obj, "/Subtype /Link\n");
	obj = obj_printf(obj, "/Rect [%d %d %d %d]\n", (gint)(annot->start_x*RPDF_DPI), (gint)(annot->start_y*RPDF_DPI), (gint)(annot->end_x*RPDF_DPI), (gint)(annot->end_y*RPDF_DPI));
	obj = obj_concat(obj, "/A << /S /URI\n");
	obj = obj_printf(obj, "/URI (%s)\n", annot->url);
	obj = obj_concat(obj, ">>\n");
	obj = obj_concat(obj, "/Border [0 0 0]\n");
	obj = obj_concat(obj, "/C [0.0000 0.0000 1.0000]\n");
	obj = obj_concat(obj, "/F 0\n");
	object_number = rpdf_object_append(pdf, TRUE, obj, NULL, 0);
	pdf->working_obj = obj_printf(pdf->working_obj, "%d 0 R ", object_number);
	g_free(annot->url);
	g_free(annot);
}

static void rpdf_make_page_fonts_stream(gpointer key, gpointer value, gpointer user_data) {
	struct rpdf *pdf = user_data;
	struct rpdf_stream_font *stream_font = value;
	pdf->working_obj = obj_printf(pdf->working_obj, "/F%s %d 0 R\n", stream_font->font_object->name, stream_font->font_object->object_number);
}

static void rpdf_make_fonts_stream(gpointer key, gpointer value, gpointer user_data) {
	struct rpdf *pdf = user_data;
	struct rpdf_font_object *font = value;
	GString *obj;
	obj = obj_concat(NULL, "/Type /Font\n");
	obj = obj_printf(obj, "/Subtype /%s\n", font->font->subtype);
	obj = obj_printf(obj, "/Name /F%s\n", font->name);
	obj = obj_printf(obj, "/BaseFont /%s\n", font->font->name);
	obj = obj_printf(obj, "/Encoding /%s\n", font->encoding);
	font->object_number = rpdf_object_append(pdf, TRUE,  obj, NULL, 0);
}

static void rpdf_number_fonts(gpointer key, gpointer value, gpointer user_data) {
	struct rpdf *pdf = user_data;
	struct rpdf_font_object *font = value;
	font->number = pdf->font_obj_number++;
}

static void rpdf_finalize_page_stream(struct rpdf *pdf) {
	if(pdf->text_on == TRUE) {
		g_string_append(pdf->page_data, "ET\n");
	}
}

static void rpdf_string_destroyer (gpointer data) {
	g_free(data);
}

struct rpdf *rpdf_new(void) {
	struct rpdf *pdf = g_new0(struct rpdf, 1);
#ifdef RPDF_COMPRESS_STREAM
	z_stream *c_stream = g_new(z_stream, 1);
	gint err;
#endif
	pdf->header = g_strdup_printf("%%PDF-1.3\n");
	pdf->fonts = g_hash_table_new_full (g_str_hash, g_str_equal, rpdf_string_destroyer, rpdf_string_destroyer);	
#ifdef RPDF_COMPRESS_STREAM
	if (c_stream != NULL) {
		c_stream->zalloc = (alloc_func)0;
		c_stream->zfree = (free_func)0;
		c_stream->opaque = (voidpf)0;

		err = deflateInit(c_stream, Z_BEST_SPEED);
	 	if (err == Z_OK)
			pdf->zlib_stream = c_stream;
		else
			g_free(c_stream);
	}
#endif
	return pdf;
}

void rpdf_set_compression(struct rpdf *pdf, gboolean use_compression) {
	pdf->use_compression = use_compression;
}

gboolean rpdf_get_compression(struct rpdf *pdf) {
	return pdf->use_compression;
}

static GString *rpdf_add_page_content_uncompressed(struct rpdf *pdf) {
	GString *obj;
	gint len;

	if (pdf == NULL || pdf->page_data == NULL)
		len = 0;
	else
		len = pdf->page_data->len;

	obj = obj_printf(NULL, "<</Length %d>>\n", len);
	obj = obj_concat(obj, "stream\n");
	if (pdf != NULL && pdf->page_data != NULL && pdf->page_data->len > 0) {
		obj = g_string_append(obj,  pdf->page_data->str);
		obj = g_string_append(obj,  "\n");
	}
	obj = obj_concat(obj, "endstream\n");
	return obj;
}

#ifdef RPDF_COMPRESS_STREAM
static GString *rpdf_add_page_content_compressed(struct rpdf *pdf) {
	GString *obj = NULL;
	z_stream *c_stream;
	gchar *compr;
	int err;

	if (pdf == NULL || pdf->page_data == NULL || pdf->page_data->str == NULL || pdf->page_data->len <= 0)
		return NULL;

	c_stream = pdf->zlib_stream;
	compr = g_malloc0(pdf->page_data->len);
	c_stream->next_out = (unsigned char *)compr;
	c_stream->avail_out = pdf->page_data->len;
	c_stream->next_in = (unsigned char *)pdf->page_data->str;
	c_stream->avail_in = pdf->page_data->len;

	err = deflate(c_stream, Z_NO_FLUSH);
	if (err != Z_OK)
		goto out;
	if (c_stream->avail_in != 0)
		goto out;
	err = deflate(c_stream, Z_FINISH);
	if (err != Z_STREAM_END)
		goto out;

	obj = obj_printf(NULL, "<</Filter /FlateDecode /Length %d>>\n", c_stream->total_out);
	obj = obj_concat(obj, "stream\n");
	obj = g_string_append_len(obj,  compr, c_stream->total_out);
	obj = g_string_append(obj,  "\n");
	obj = obj_concat(obj, "endstream\n");

	err = deflateReset(c_stream);
	if (err != Z_OK)
		pdf->use_compression = FALSE;
	g_free(compr);
	return obj;

out:
	pdf->use_compression = FALSE;
	g_free(compr);
	return NULL;
}
#endif

static gint rpdf_add_page_content(struct rpdf *pdf, struct rpdf_page_info *page_info) {
	GString *obj = NULL;

	if(pdf->page_data != NULL) {
		gchar buf1[512], buf2[512];
		if(page_info->has_rotation == TRUE) {
			gdouble angle = M_PI * page_info->rotation_angle / 180.0;
			gdouble text_sin= sin(angle);
			gdouble text_cos = cos(angle);
			sprintf(buf2, "%.04f %.04f %.04f %.04f 0.0 0.0 cm\n", text_cos, text_sin, -text_sin, text_cos);
			g_string_prepend(pdf->page_data, buf2);
		}
		if(page_info->has_translation == TRUE) {
			sprintf(buf1, "1.0000 0.0000 0.0000 1.0000 %.04f %.04f cm\n", page_info->translate_x*RPDF_DPI, page_info->translate_y*RPDF_DPI);
			g_string_prepend(pdf->page_data, buf1);
		}
	}

#ifdef RPDF_COMPRESS_STREAM
	if ((pdf->zlib_stream != NULL) && (pdf->use_compression == TRUE))
		obj = rpdf_add_page_content_compressed(pdf);
	if (obj == NULL)
#endif
		obj = rpdf_add_page_content_uncompressed(pdf);

	return rpdf_object_append(pdf, FALSE,  obj, NULL, 0);
}

gboolean rpdf_finalize(struct rpdf *pdf) {
	gint i, save_size;
	gint pages_ref, page_ref, outlines_ref, procset_ref;
	gint info_ref, catalog_ref;
	GString *obj = NULL;
	char  *saved_locale;
	gchar buf[128];
	struct tm my_tm;
	time_t now;
	
	time(&now);
#ifdef HAVE_LOCALTIME_R
	localtime_r(&now, &my_tm);
#else
	memcpy(&my_tm, localtime(&now), sizeof(struct tm));
#endif

	rpdf_out_string(pdf, pdf->header);
	g_hash_table_foreach(pdf->fonts, rpdf_number_fonts, pdf);
	
	saved_locale = strdup(setlocale(LC_ALL, NULL));
	setlocale(LC_ALL, "C");

	pages_ref = rpdf_object_number_skip(pdf);

	g_hash_table_foreach(pdf->fonts, rpdf_make_fonts_stream, pdf);

	obj = obj_concat(NULL, "/Type /Outlines\n");
	obj = obj_concat(obj, "/Count 0\n"); 
	outlines_ref = rpdf_object_append(pdf, TRUE, obj, NULL, 0);

	buf[0] = 0;
	if(pdf->has_images)
		sprintf(buf, " /ImageC");
	obj = obj_printf(NULL, "[/PDF /Text%s ]\n",buf);
	procset_ref = rpdf_object_append(pdf, FALSE, obj, NULL, 0);

	for(i=0;i<pdf->page_count;i++) {
		struct rpdf_page_info *page_info = pdf->page_info[i];
		GSList *list = pdf->page_contents[i];

		pdf->text_on = FALSE;
		pdf->page_data = NULL;
		pdf->page_fonts = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);

		if(list != NULL)
			g_slist_foreach(list, rpdf_make_page_stream, pdf);

		rpdf_finalize_page_stream(pdf);

		page_ref = rpdf_add_page_content(pdf, page_info);

		obj = obj_concat(NULL, "/Type /Page\n");
		obj = obj_printf(obj, "/Parent %d 0 R\n", pages_ref);
		obj = obj_concat(obj, "/Resources <<\n");
		if(g_hash_table_size(pdf->page_fonts) > 0) {
			obj = obj_concat(obj, "/Font <<\n");
			pdf->working_obj = obj;
			g_hash_table_foreach(pdf->page_fonts, rpdf_make_page_fonts_stream, pdf);
			obj = pdf->working_obj;
			obj = obj_concat(obj, ">>\n");
		}

		if(page_info->images != NULL) {
			obj = obj_concat(obj, "/XObject <<\n");
			pdf->working_obj = obj;
			g_slist_foreach(page_info->images, rpdf_make_page_image_obj, pdf);
			obj = pdf->working_obj;
			obj = obj_concat(obj, ">>\n");
			g_slist_free(page_info->images);
		}

		obj = obj_printf(obj, "/ProcSet %d 0 R >>\n", procset_ref);
		obj = obj_printf(obj, "/MediaBox [0 0 %d %d]\n", page_info->paper->x, page_info->paper->y);
		obj = obj_printf(obj, "/CropBox [0 0 %d %d]\n", page_info->paper->x, page_info->paper->y);
		obj = obj_printf(obj, "/Rotate %d\n", page_info->orientation == RPDF_PORTRAIT ? 0 : 270);
		obj = obj_printf(obj, "/Contents %d 0 R\n", page_ref);
		
		if(page_info->annots != NULL) {
			obj = obj_concat(obj, "/Annots [ ");
			pdf->working_obj = obj;
			g_slist_foreach(page_info->annots, rpdf_make_page_annot_obj, pdf);
			g_slist_free(page_info->annots);
			obj = pdf->working_obj;
			obj = obj_concat(obj, "]\n");
		}
		
		page_info->object_number = rpdf_object_append(pdf, TRUE, obj, NULL, 0);
		
		g_hash_table_destroy(pdf->page_fonts);
		g_string_free(pdf->page_data, TRUE);
	}

	obj = obj_concat(NULL, "/Type /Pages\n");
	obj = obj_printf(obj, "/Count %d\n", pdf->page_count); 
	obj = obj_concat(obj, "/Kids ["); 
	for(i=0;i<pdf->page_count;i++) 
		obj = obj_printf(obj, " %d 0 R ", pdf->page_info[i]->object_number);
	obj = obj_concat(obj, "]\n"); 
	rpdf_object_insert(pdf, pages_ref, TRUE, obj, NULL, 0);

	if(pdf->creator != NULL) 
		obj = obj_printf(NULL, "/Creator %s\n", pdf->creator); 
	else
		obj = obj_concat(NULL, "/Creator (RPDF By SICOM Systems)\n"); 

	obj = obj_printf(obj, "/CreationDate (D:%04ld%02d%02d%02d%02d%02d)\n", my_tm.tm_year+1900, my_tm.tm_mon+1, my_tm.tm_mday, my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec); 
	obj = obj_concat(obj, "/Producer (RPDF 1.0)\n");
	
	if(pdf->author != NULL)
		obj = obj_printf(obj, "/Author %s\n", pdf->author); 
	else		
		obj = obj_concat(obj, "/Author (RPDF)\n"); 
	
	if(pdf->title != NULL)
		obj = obj_printf(obj, "/Title %s\n", pdf->title); 
	else 
		obj = obj_concat(obj, "/Title (No Title)\n"); 

	if(pdf->subject != NULL)
		obj = obj_printf(obj, "/Subject %s\n", pdf->subject); 
	else
		obj = obj_concat(obj, "/Subject (None)\n"); 
				
	if(pdf->keywords != NULL)
		obj = obj_printf(obj, "/Keywords %s\n", pdf->keywords); 
	else
		obj = obj_concat(obj, "/Keywords (RPDF)\n"); 

	info_ref = rpdf_object_append(pdf, TRUE, obj, NULL, 0);

	obj = obj_concat(NULL, "/Type /Catalog\n");
	obj = obj_printf(obj, "/Pages %d 0 R\n", pages_ref); 
	obj = obj_printf(obj, "/Outlines %d 0 R\n", outlines_ref); 
	catalog_ref = rpdf_object_append(pdf, TRUE, obj, NULL, 0);

	g_slist_foreach(pdf->objects, rpdf_finalize_objects, pdf);
	
	save_size = pdf->size;
	rpdf_out_string(pdf, "xref\n");
	sprintf(buf, "0 %d\n", pdf->object_count+1);
	rpdf_out_string(pdf, buf);
	rpdf_out_string(pdf, "0000000000 65535 f \n");
	g_slist_foreach(pdf->xref, rpdf_finalize_xref, pdf);
	g_slist_free(pdf->xref);

	rpdf_out_string(pdf, "trailer\n");
	rpdf_out_string(pdf, "<<\n");
	sprintf(buf, "/Size %d\n", pdf->object_count+1);
	rpdf_out_string(pdf, buf);
	sprintf(buf, "/Root %d 0 R\n", catalog_ref);
	rpdf_out_string(pdf, buf);
	sprintf(buf, "/Info %d 0 R\n", info_ref);
	rpdf_out_string(pdf, buf);
	rpdf_out_string(pdf, ">>\n");
	sprintf(buf, "startxref\n%d\n", save_size);
	rpdf_out_string(pdf, buf);
	rpdf_out_string(pdf, "%%EOF\n");
	setlocale(LC_ALL, saved_locale);
	free(saved_locale);
	
	return TRUE;
}

void rpdf_set_title(struct rpdf *pdf, const gchar *title) {
	pdf->title = g_strconcat("(", title, ")", NULL);
}	

void rpdf_set_subject(struct rpdf *pdf, const gchar *subject) {
	pdf->subject = g_strconcat("(", subject, ")", NULL);
}	

void rpdf_set_author(struct rpdf *pdf, const gchar *author) {
	pdf->author = g_strconcat("(", author, ")", NULL);
}	

void rpdf_set_keywords(struct rpdf *pdf, const gchar *keywords) {
	pdf->keywords = g_strconcat("(", keywords, ")", NULL);
}	

void rpdf_set_creator(struct rpdf *pdf, const gchar *creator) {
	pdf->creator = g_strconcat("(", creator, ")", NULL);
}	

void rpdf_translate(struct rpdf *pdf, gdouble x, gdouble y) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	page_info->has_translation = TRUE;
	page_info->translate_x = x;
	page_info->translate_y = y;
}

void rpdf_rotate(struct rpdf *pdf, gdouble angle) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	page_info->has_rotation = TRUE;
	page_info->rotation_angle = angle;
}

gboolean rpdf_new_page(struct rpdf *pdf, gint paper, gint orientation) {
	struct rpdf_page_info *page_info;
	pdf->page_count++;
	pdf->page_contents = g_realloc(pdf->page_contents, sizeof(gpointer) * pdf->page_count);
	pdf->page_contents[pdf->page_count-1] = NULL;

	pdf->page_info = g_realloc(pdf->page_info, sizeof(gpointer) * pdf->page_count);
	page_info = pdf->page_info[pdf->page_count-1] = g_new0(struct rpdf_page_info, 1);
	page_info->paper = &rpdf_paper[paper-1];
	page_info->orientation = orientation;

	pdf->current_page = pdf->page_count - 1;
	return TRUE;
}

gboolean rpdf_set_page(struct rpdf *pdf, gint page) {
	if(page < 0 || page >= pdf->page_count)
		return FALSE;

	pdf->current_page = page;
	return TRUE;
}

gboolean rpdf_set_font(struct rpdf *pdf, const gchar *font, const gchar *encoding, gdouble size) {
	gint i=0;
	gint found = FALSE;
	struct rpdf_font_object *font_object;
	struct rpdf_stream_font *stream;
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	struct rpdf_stream *real_stream;
	gchar *both;


	while(rpdf_fonts[i].name[0] != 0) {
		if(strcmp(rpdf_fonts[i].name, font) == 0) {
			found = TRUE;
			break;
		}
		i++;
	}

	if(found == FALSE) 
		return FALSE;		
	
	both = g_strconcat(font, encoding, NULL);
	font_object = g_hash_table_lookup(pdf->fonts, both);

	if(font_object == NULL) {
		font_object = g_new0(struct rpdf_font_object, 1);
		font_object->font = &rpdf_fonts[i];
		sprintf(font_object->name, "rpdf%d", pdf->font_count++);
		strcpy(font_object->encoding, encoding);
		g_hash_table_insert(pdf->fonts, both, font_object);			
	} else {
		g_free(both);
	}

	page_info->font_size = size;
	page_info->font = font_object->font;
	page_info->font_object = font_object;

	stream = g_new0(struct rpdf_stream_font, 1);
	stream->font_object = font_object;
	stream->size = size;

	real_stream = rpdf_stream_new(RPDF_TYPE_FONT, stream);
	rpdf_stream_append(pdf, real_stream);
	pdf->stream_font_destroyer = g_slist_prepend(pdf->stream_font_destroyer, stream);
	
	return TRUE;
}

gboolean rpdf_set_font_size(struct rpdf *pdf, gdouble size) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	struct rpdf_stream_font *stream;
	struct rpdf_stream *real_stream;

	page_info->font_size = size;

	if(page_info->font == NULL)
		return FALSE;

	stream = g_new0(struct rpdf_stream_font, 1);
	stream->font_object = page_info->font_object;
	stream->size = size;
	real_stream = rpdf_stream_new(RPDF_TYPE_FONT, stream);
	rpdf_stream_append(pdf, real_stream);
	pdf->stream_font_destroyer = g_slist_append(pdf->stream_font_destroyer, stream);

	return TRUE;
}

gboolean rpdf_text_callback(struct rpdf *pdf, gdouble x, gdouble y, gdouble angle, gint len, CALLBACK, gpointer user_data) {
	struct rpdf_stream_text_callback *stream;
	stream = g_new0(struct rpdf_stream_text_callback, 1);
	stream->x = x;
	stream->y = y;
	stream->angle = angle;
	stream->len = len;
	stream->callback = callback;
	stream->user_data = user_data;
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_TEXT_CB, stream));
	return TRUE;
}



gboolean rpdf_text(struct rpdf *pdf, gdouble x, gdouble y, gdouble angle, const gchar *text) {
	struct rpdf_stream_text *stream;
	gint slen;
	gint count = 0, spot=0, i;
	static GIConv conv = NULL;
	gboolean converted = FALSE;
	gchar *new_text;
	
	if(conv == NULL) {
		conv = g_iconv_open("ISO-8859-1", "UTF-8");	
	}

	if(conv != NULL && text != NULL) {
		gsize foo1, foo2;
		new_text = g_convert_with_iconv(text, strlen(text), conv, &foo1, &foo1, NULL);
		if(new_text != NULL) {
			converted = TRUE;
		} else {
			new_text = (char *)text;	
		}
	}
	
	stream = g_new0(struct rpdf_stream_text, 1);
	stream->x = x;
	stream->y = y;
	stream->angle = angle;
	
	slen = strlen(new_text);
	for(i=0;i<slen;i++) {
		if(new_text[i] == '(' || new_text[i] == ')' || new_text[i] == '\\')
			count++;
	}
	if(count == 0)
		stream->text = g_strdup(new_text); 
	else {
		stream->text = g_malloc(slen + 1 + count);
		for(i=0;i<slen;i++) {
			if(new_text[i] == '(' || new_text[i] == ')' || new_text[i] == '\\') {
				stream->text[spot++] = '\\';
			}
			stream->text[spot++] = new_text[i];
		}
		stream->text[spot++] = 0;
	
	}
	
	if(converted)
		g_free(new_text);
	
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_TEXT, stream));

	return TRUE;
}

static gchar *stream_read_bytes(gchar *stream, gint *spot, gint num_bytes, gint size) {
	if(*spot >= size)
		return NULL;
	*spot = *spot + num_bytes;
	return stream + (*spot) - num_bytes;
}

static guint stream_read_byte(gchar *stream, gint *spot, gint size) {
	guchar data = stream[*spot];
	if(*spot >= size)
		return -1;
	*spot = *spot + 1;
	return data;
}

static guint16 stream_read_two_bytes(gchar *stream, gint *spot, gint size) {
	guint16 data;
	guchar data1 = stream[*spot], data2;
	*spot = *spot + 1;
	data2 = stream[*spot];
	*spot = *spot + 1;

	data = ((guint16)data1 << 8) + data2;

	if(*spot >= size)
		return -1;

	return data;
}

static gint64 stream_read_long(gchar *stream, gint *spot, gint size) {
	gint64 data = 0;
	guchar *string;;
	if(*spot >= size)
		return -1;
	string = (guchar *)&stream[*spot];
	((char *)&data)[0] = string[3];
	((char *)&data)[1] = string[2];
	((char *)&data)[2] = string[1];
	((char *)&data)[3] = string[0];
	*spot = *spot + 4;
	return data;
}

static gint jpeg_get_marker(gchar *stream, gint *spot, gint size) {
	for(;stream_read_byte(stream, spot, size) != 0xFF;);
	return stream_read_byte(stream, spot, size);
}

static void jpeg_skip_section(gchar *stream, gint *spot, gint size) {
 	guint length;

	length = stream_read_two_bytes(stream, spot, size);

	if(length < 2)
		return;

	length -= 2;
	
	*spot = *spot + length;
}

static void jpeg_process_SOFn(gchar *stream, gint *spot, gint size, gint marker, struct rpdf_image_jpeg *info) {
	gint length;
	
	length = stream_read_two_bytes(stream, spot, size);
	info->process = marker;
	info->precision = stream_read_byte(stream, spot, size);
	info->height = stream_read_two_bytes(stream, spot, size);
	info->width = stream_read_two_bytes(stream, spot, size);
	info->components = stream_read_byte(stream, spot, size);
	if(length != (guint)(8 + info->components * 3)) {
		rpdf_error("BAD IMAGE\n");
		return;
	}
	*spot = *spot + (3 * info->components);
}

gboolean rpdf_image(struct rpdf *pdf, gdouble x, gdouble y, gdouble width, gdouble height, gint image_type, gchar *file_name) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	gint fd;
	off_t size, compare_size, pos;
	gint read_spot = 0;
	gboolean loop = TRUE;
	struct rpdf_images *image;
	ssize_t	readbytes;

	if(file_name == NULL)
		return FALSE;
		
	fd = open(file_name, O_RDONLY | O_BINARY, 0);
	
	if(fd <= 0)
		return FALSE;
	
	compare_size = size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	
	image = g_new0(struct rpdf_images, 1);
	image->x = x;
	image->y = y;
	image->width = width;
	image->height = height;
	image->data = g_malloc(size);
	image->image_type = image_type;
	image->length = size;
	pos = 0;
	while (compare_size > 0) {
		readbytes = read(fd, image->data + pos, compare_size);
		if (readbytes > 0) {
			pos += readbytes;
			compare_size -= readbytes;
		} else {
			g_free(image->data);
			g_free(image);
			close(fd);
			return FALSE;
		}
	}
	close(fd);

	if(image_type == RPDF_IMAGE_PNG) {
		struct rpdf_image_png *png_info = g_new0(struct rpdf_image_png, 1);
		gchar header[9];

		sprintf(header, "%cPNG%c%c%c%c", 137,13,10,26,10);			
		if(memcmp(stream_read_bytes(image->data,&read_spot,8,size), header, 8) != 0) {
			g_free(png_info);
			g_free(image->data);
			g_free(image);
			return FALSE;
		}
		stream_read_bytes(image->data,&read_spot,4,size);
		if(memcmp(stream_read_bytes(image->data,&read_spot,4,size), "IHDR", 4) != 0) {
			g_free(png_info);
			g_free(image->data);
			g_free(image);
			return FALSE;
		}
		png_info->width = image->width = stream_read_long(image->data,&read_spot,size);
		png_info->height = image->height = stream_read_long(image->data,&read_spot,size);
		png_info->data = g_string_new("");
		png_info->palette = g_string_new("");
		png_info->trans = g_string_new("");
		
		png_info->bpc = stream_read_byte(image->data, &read_spot, size);

		if(png_info->bpc > 8)
			rpdf_error("16 but depths not supported\n");
		
		png_info->ct = stream_read_byte(image->data, &read_spot, size);
		
		if(png_info->ct == 1 || png_info->ct >= 4)
			rpdf_error("Unsuported Ct (Alpha Channel?)\n");

		if(stream_read_byte(image->data, &read_spot, size) != 0)
			rpdf_error("Unknown compression method\n");
		if(stream_read_byte(image->data, &read_spot, size) != 0)
			rpdf_error("Unknown filter method\n");
		if(stream_read_byte(image->data, &read_spot, size) != 0)
			rpdf_error("Interlacing not supported\n");

		stream_read_long(image->data, &read_spot, size);

		sprintf(png_info->params, "/DecodeParms <</Predictor 15 /Colors %d /BitsPerComponent %d /Columns %d>>\n",
			png_info->ct == 2 ? 3 : 1, png_info->bpc,	png_info->width);

		while(1) {
			long length = stream_read_long(image->data,&read_spot,size);
			
			gchar type[5];
			memcpy(type, stream_read_bytes(image->data,&read_spot,4,size), 4);
			type[4] = '\0';
			if(strcmp(type, "IDAT") == 0) {
				g_string_append_len(png_info->data, stream_read_bytes(image->data,&read_spot,length,size), length);
				stream_read_long(image->data, &read_spot, size);
			} else if(strcmp(type, "PLTE") == 0) {
				g_string_append_len(png_info->palette, stream_read_bytes(image->data,&read_spot,length,size), length);
				stream_read_long(image->data, &read_spot, size);
			} else if(strcmp(type, "tRNS") == 0) {
				gint tmp;
				g_string_append_len(png_info->trans, stream_read_bytes(image->data,&read_spot,length,size), length);
				if(png_info->ct == 0) {
					tmp = png_info->trans->str[1];
					png_info->trans_list = g_slist_append(NULL, GINT_TO_POINTER((tmp)));
				} else if(png_info->ct == 2) {
					tmp = png_info->trans->str[1];
					png_info->trans_list = g_slist_append(NULL, GINT_TO_POINTER((tmp)));
					tmp = png_info->trans->str[3];
					png_info->trans_list = g_slist_append(NULL, GINT_TO_POINTER((tmp)));
					tmp = png_info->trans->str[5];
					png_info->trans_list = g_slist_append(NULL, GINT_TO_POINTER((tmp)));
				} else {
					int i;
					for(i=0;i<png_info->trans->len;i++) {
						if(png_info->trans->str[i] == 0) {
							tmp = png_info->trans->str[i];
							png_info->trans_list = g_slist_append(NULL, GINT_TO_POINTER((tmp)));
							break;
						}					
					}
				}
				g_string_free(png_info->trans, TRUE);
				stream_read_long(image->data, &read_spot, size);
			} else if(strcmp(type, "IEND") == 0) {
				break;
			} else { /*Skip Over Unknown*/
				stream_read_bytes(image->data,&read_spot,length,size);
				stream_read_long(image->data, &read_spot, size);
			}
		}
		image->metadata = png_info;
	} else if(image_type == RPDF_IMAGE_JPEG) {
		struct rpdf_image_jpeg *jpeg_info = g_new0(struct rpdf_image_jpeg, 1);
		guint test1, test2;
		image->metadata = jpeg_info;
		test1 = stream_read_byte(image->data, &read_spot, size);
		test2 = stream_read_byte(image->data, &read_spot, size);
		if(test1 != 0xFF && test2 != M_SOI) {
			g_free(image->data);
			g_free(image);
			return FALSE;
		}
		while( loop) {
			gint marker = jpeg_get_marker(image->data, &read_spot, size);
			switch (marker) {
				case M_SOF0:
				case M_SOF1:
				case M_SOF2:
				case M_SOF3:
				case M_SOF5:
				case M_SOF6:
				case M_SOF7:
				case M_SOF9:
				case M_SOF10:
				case M_SOF11:
				case M_SOF13:
				case M_SOF14:
				case M_SOF15:
			      jpeg_process_SOFn(image->data, &read_spot, size, marker, jpeg_info);
					if(image->width == 0 && image->height == 0) {
						image->width = jpeg_info->width;
						image->height = jpeg_info->height;
					}
			      break;
			    case M_SOS:	
					loop = FALSE;
			    default:
			      jpeg_skip_section(image->data, &read_spot, size);
			      break;
			}
		}	
	}
	image->number = pdf->image_count++;
	page_info->images = g_slist_append(page_info->images, image);
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_IMAGE, image));

	pdf->has_images = TRUE;
	return TRUE;
}

gboolean rpdf_link(struct rpdf *pdf, gdouble start_x, gdouble start_y, gdouble end_x, gdouble end_y, const gchar *url) {
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];
	struct rpdf_annots *annot = g_new0(struct rpdf_annots, 1);
	annot->number = pdf->annot_count++;
	annot->start_x = start_x;
	annot->start_y = start_y;
	annot->end_x = end_x;
	annot->end_y = end_y;
	annot->url = g_strdup(url);
	page_info->annots = g_slist_append(page_info->annots, annot);
	return TRUE;
}

gboolean rpdf_moveto(struct rpdf *pdf, gdouble x, gdouble y) {
	struct rpdf_stream_point *stream;
	stream = g_new0(struct rpdf_stream_point, 1);
	
	if(isnan(x))
		x = 0;
	if(isnan(y))
		y = 0;
	
	stream->x = x;
	stream->y = y;
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_MOVE, stream));
	return TRUE;
}

gboolean rpdf_set_line_width(struct rpdf *pdf, gdouble width) {
	gdouble *new_width = g_malloc(sizeof(gdouble));
	*new_width = width;
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_WIDTH, new_width));
	return TRUE;
}

gboolean rpdf_lineto(struct rpdf *pdf, gdouble x, gdouble y) {
	struct rpdf_stream_point *stream;
	stream = g_new0(struct rpdf_stream_point, 1);

	if(isnan(x))
		x = 0;
	if(isnan(y))
		y = 0;

	stream->x = x;
	stream->y = y;
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_LINE, stream));
	return TRUE;
}

gboolean rpdf_closepath(struct rpdf *pdf) {
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_CLOSEPATH, NULL));
	return TRUE;
}

gboolean rpdf_stroke(struct rpdf *pdf) {
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_STROKE, NULL));
	return TRUE;
}

gboolean rpdf_rect(struct rpdf *pdf, gdouble x, gdouble y, gdouble width, gdouble height) {
	struct rpdf_stream_rect *stream;
	stream = g_new0(struct rpdf_stream_rect, 1);
	stream->x = x;
	stream->y = y;
	stream->width = width;
	stream->height = height;
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_RECT, stream));
	return TRUE;
}

gboolean rpdf_fill(struct rpdf *pdf) {
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_FILL, NULL));
	return TRUE;
}

gboolean rpdf_setrgbcolor(struct rpdf *pdf, gdouble r, gdouble g, gdouble b) {
	struct rpdf_stream_color *stream;
	stream = g_new0(struct rpdf_stream_color, 1);
	stream->r = r;
	stream->g = g;
	stream->b = b;
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_COLOR, stream));
	return TRUE;
}

gdouble rpdf_text_width(struct rpdf *pdf, const gchar *text) {
	gint slen,i;
	gdouble width = 0.0;
	struct rpdf_page_info *page_info = pdf->page_info[pdf->current_page];

	if(text == NULL)
		return 0.0;
	
	slen = strlen(text);
	for(i=0;i<slen;i++) {
		width += page_info->font->widths[(gint)text[i]];
	
	}
	return width*page_info->font_size/1000.0;
}

gint rpdf_arc(struct rpdf *pdf, gdouble x, gdouble y, gdouble radius, gdouble start_angle, gdouble end_angle) {
	struct rpdf_stream_arc *stream = g_new0(struct rpdf_stream_arc, 1);
	stream->x = x;
	stream->y = y;
	stream->radius = radius;
	stream->start_angle = start_angle;
	stream->end_angle = end_angle;
	rpdf_stream_append(pdf, rpdf_stream_new(RPDF_TYPE_ARC, stream));
	return TRUE;
}

gchar *rpdf_get_buffer(struct rpdf *pdf, gint *length) {
	*length = pdf->size;
	return pdf->out_buffer;
}

void rpdf_free(struct rpdf *pdf) {
	gint i;
#ifdef RPDF_COMPRESS_STREAM
	if (pdf->zlib_stream != NULL) {
		z_stream *c_stream = pdf->zlib_stream;
		deflateEnd(c_stream);
		g_free(c_stream);
	}
#endif
	g_free(pdf->header);
	g_free(pdf->title);
	g_free(pdf->subject);
	g_free(pdf->author);
	g_free(pdf->keywords);
	g_free(pdf->creator);
	g_hash_table_destroy(pdf->fonts);
	g_free(pdf->out_buffer);
	for(i=0;i<pdf->page_count;i++) {
		g_free(pdf->page_info[i]);
		g_slist_free(pdf->page_contents[i]);
	}
	g_free(pdf->page_contents);
	g_free(pdf->page_info);
	g_slist_foreach(pdf->stream_font_destroyer, rpdf_stream_font_destroyer, NULL);
	g_slist_free(pdf->stream_font_destroyer);	
	g_slist_foreach(pdf->objects, rpdf_object_destroyer, NULL);
	g_slist_free(pdf->objects);	
	g_free(pdf);
}
