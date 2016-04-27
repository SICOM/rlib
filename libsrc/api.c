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
 * This module implements the C language API (Application Programming Interface)
 * for the RLIB library functions.
 */

#include <config.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"
#include "rlib_langinfo.h"

#ifndef CODESET
#define CODESET _NL_CTYPE_CODESET_NAME
#endif

static void string_destroyer (gpointer data) {
	g_free(data);
}

static void metadata_destroyer (gpointer data) {
	struct rlib_metadata *metadata = data;
	rlib_value_free(&metadata->rval_formula);
	rlib_pcode_free(metadata->formula_code);
	g_free(data);
}

DLL_EXPORT_SYM rlib * rlib_init_with_environment(struct environment_filter *environment) {
	rlib *r;
	
	init_signals();

	r = g_new0(rlib, 1);

	if(environment == NULL)
		rlib_new_c_environment(r);
	else
		ENVIRONMENT(r) = environment;
	
	r->output_parameters = g_hash_table_new_full (g_str_hash, g_str_equal, string_destroyer, string_destroyer);
	r->input_metadata = g_hash_table_new_full (g_str_hash, g_str_equal, string_destroyer, metadata_destroyer);
	r->parameters = g_hash_table_new_full (g_str_hash, g_str_equal, string_destroyer, string_destroyer);

	r->radix_character = '.';
	
#if !DISABLE_UTF8
	make_all_locales_utf8();
#endif
/*	strcpy(r->pdf_encoding, "WinAnsiEncoding"); */
	r->did_execute = FALSE;
	r->current_locale = g_strdup(setlocale(LC_ALL, NULL));
	rlib_pcode_find_index(r);
	return r;
}


DLL_EXPORT_SYM rlib *rlib_init(void) {
	return rlib_init_with_environment(NULL);
}

static void rlib_alloc_query_space(rlib *r) {
	if(r->queries_count == 0) {
		r->queries = g_malloc((r->queries_count + 1) * sizeof(gpointer));
		r->results = g_malloc((r->queries_count + 1) * sizeof(gpointer));		
	} else {
		r->queries = g_realloc(r->queries, (r->queries_count + 1) * sizeof(void *));
		r->results = g_realloc(r->results, (r->queries_count + 1) * sizeof(void *));
	}
	r->queries[r->queries_count] = g_malloc0(sizeof(struct rlib_queries));
	r->results[r->queries_count] = g_malloc0(sizeof(struct rlib_results));
}

DLL_EXPORT_SYM gint rlib_add_query_pointer_as(rlib *r, const gchar *input_source, gchar *sql, const gchar *name) {
	gint i;

	rlib_alloc_query_space(r);
	r->queries[r->queries_count]->sql = sql;
	r->queries[r->queries_count]->name = g_strdup(name);
	for(i=0;i<r->inputs_count;i++) {
		if(!strcmp(r->inputs[i].name, input_source)) {
			r->queries[r->queries_count]->input = r->inputs[i].input;
			r->queries_count++;
			return r->queries_count;
		}
	}

	r_error(r, "rlib_add_query_as: Could not find input source [%s]!\n", input_source);
	return -1;
}

DLL_EXPORT_SYM gint rlib_add_query_as(rlib *r, const gchar *input_source, const gchar *sql, const gchar *name) {
	return rlib_add_query_pointer_as(r, input_source, g_strdup(sql), name);
}

DLL_EXPORT_SYM gint rlib_add_report(rlib *r, const gchar *name) {
	gchar *tmp;
	int i, found_dir_sep = 0, last_dir_sep;

	if (r->parts_count > RLIB_MAXIMUM_REPORTS - 1)
		return -1;

	tmp = g_strdup(name);
#ifdef _WIN32
	/* Sanitize the file path to look like UNIX */
	for (i = 0; tmp[i]; i++)
		if (tmp[i] == '\\')
			tmp[i] = '/';
#endif
	for (i = 0; tmp[i]; i++) {
		if (name[i] == '/') {
			found_dir_sep = 1;
			last_dir_sep = i;
		}
	}
	if (found_dir_sep) {
		r->reportstorun[r->parts_count].name = strdup(tmp + last_dir_sep + 1);
		tmp[last_dir_sep] = '\0';
		r->reportstorun[r->parts_count].dir = tmp;
	} else {
		r->reportstorun[r->parts_count].name = tmp;
		r->reportstorun[r->parts_count].dir = strdup("");
	}
	r->reportstorun[r->parts_count].type = RLIB_REPORT_TYPE_FILE;
	r->parts_count++;
	return r->parts_count;
}

DLL_EXPORT_SYM gint rlib_add_report_from_buffer(rlib *r, gchar *buffer) {
	char cwdpath[PATH_MAX + 1];
	char *cwd;
#ifdef _WIN32
	int i;
	char *tmp __attribute__((unused));
#endif

	if (r->parts_count > RLIB_MAXIMUM_REPORTS - 1)
		return -1;
	/*
	 * When we add a toplevel report part from buffer,
	 * we can't rely on file path, we can only rely
	 * on the application's current working directory.
	 */
	cwd = getcwd(cwdpath, sizeof(cwdpath));
	/*
	 * The errors when getcwd() returns NULL are pretty serious.
	 * Let's not do any work in this case.
	 */
	if (cwd == NULL)
		return -1;
	r->reportstorun[r->parts_count].name = g_strdup(buffer);
	r->reportstorun[r->parts_count].dir = g_strdup(cwdpath);
#ifdef _WIN32
	tmp = r->reportstorun[r->parts_count].dir;
	/* Sanitize the file path to look like UNIX */
	for (i = 0; i < tmp[i]; i++)
		if (tmp[i] == '\\')
			tmp[i] = '/';
#endif
	r->reportstorun[r->parts_count].type = RLIB_REPORT_TYPE_BUFFER;
	r->parts_count++;
	return r->parts_count;
}

DLL_EXPORT_SYM gint rlib_add_search_path(rlib *r, const gchar *path) {
	gchar *path_copy;
#ifdef _WIN32
	int i;
	int len;
#endif

	/* Don't add useless search paths */
	if (path == NULL)
		return -1;
	if (strlen(path) == 0)
		return -1;

	path_copy = g_strdup(path);
	if (path_copy == NULL)
		return -1;

#ifdef _WIN32
	len = strlen(path_copy);
	/* Sanitize the file path to look like UNIX */
	for (i = 0; i < len; i++)
		if (path_copy[i] == '\\')
			path_copy[i] = '/';
#endif
	r->search_paths = g_slist_append(r->search_paths, path_copy);
	return 0;
}

/*
 * Try to search for a file and return the first one
 * found at the possible locations.
 *
 * report_index:
 *	index to r->reportstorun[] array, or
 *	-1 to try to find relative to every reports
 */
gchar *get_filename(rlib *r, const char *filename, int report_index, gboolean report) {
	int have_report_dir = 0, ri;
	gchar *file;
	struct stat st;
	GSList *elem;
#ifdef _WIN32
	int len = strlen(filename);
#endif

#ifdef _WIN32
	/*
	 * If the filename starts with a drive label,
	 * it is an absolute file name. Return it.
	 */
	if (len >= 2) {
		if (tolower(filename[0]) >= 'a' && tolower(filename[0]) <= 'z' &&
				filename[1] == ':') {
			r_info(r, "get_filename: Absolute file name with drive label: %s\n", filename);
			return g_strdup(filename);
		}
	}
#endif
	/*
	 * Absolute filename, on the same drive for Windows,
	 * unconditionally for Un*xes.
	 */
	if (filename[0] == '/') {
		r_info(r, "get_filename: Absolute file name: %s\n", filename);
		return g_strdup(filename);
	}

	/*
	 * Try to find the file in report's subdirectory
	 */
	if (report_index >= 0) {
		have_report_dir = (r->reportstorun[report_index].dir[0] != 0);
		if (have_report_dir) {
			file = g_strdup_printf("%s/%s", r->reportstorun[report_index].dir, filename);
			if (stat(file, &st) == 0) {
				r_info(r, "get_filename: File found relative to the report: %s, full path: %s\n", filename, file);
				return file;
			}
			g_free(file);
		}
	} else {
		for (ri = 0; ri < r->parts_count; ri++) {
			have_report_dir = (r->reportstorun[ri].dir[0] != 0);
			if (have_report_dir) {
				file = g_strdup_printf("%s/%s", r->reportstorun[ri].dir, filename);
				if (stat(file, &st) == 0) {
					r_info(r, "get_filename: File found relative to the report: %s, full path: %s\n", filename, file);
					return file;
				}
				g_free(file);
			}
		}
	}

	/*
	 * Try to find the file in the search path
	 */
	for (elem = r->search_paths; elem; elem = elem->next) {
		gchar *search_path = elem->data;
		gboolean absolute_search_path = FALSE;

#ifdef _WIN32
		/*
		 * If the filename starts with a drive label,
		 * it is an absolute file name. Use it.
		 */
		if (len >= 2) {
			if (tolower(search_path[0]) >= 'a' && tolower(search_path[0]) <= 'z' &&
					search_path[1] == ':')
				absolute_search_path = TRUE;
		}
#endif
		if (!absolute_search_path) {
			if (search_path[0] == '/')
				absolute_search_path = TRUE;
		}

		if (!absolute_search_path) {
			if (report_index >= 0) {
				have_report_dir = (r->reportstorun[report_index].dir[0] != 0);
				if (have_report_dir) {
					file = g_strdup_printf("%s/%s/%s", r->reportstorun[report_index].dir, search_path, filename);
					if (stat(file, &st) == 0) {
						r_info(r, "get_filename: File found in search path relative to report XML: %s, full path: %s\n", search_path, file);
						return file;
					}
					g_free(file);
				}
			} else {
				for (ri = 0; ri < r->parts_count; ri++) {
					have_report_dir = (r->reportstorun[ri].dir[0] != 0);
					if (have_report_dir) {
						file = g_strdup_printf("%s/%s/%s", r->reportstorun[ri].dir, search_path, filename);
						if (stat(file, &st) == 0) {
							r_info(r, "get_filename: File found in search path relative to report XML: %s, full path: %s\n", search_path, file);
							return file;
						}
						g_free(file);
					}
				}
			}
		}

		file = g_strdup_printf("%s/%s", search_path, filename);

		if (stat(file, &st) == 0) {
			r_info(r, "get_filename: File found in search path: %s, full path: %s\n", search_path, file);
			return file;
		}

		g_free(file);
	}

	/*
	 * Last resort, return the file as is,
	 * relative to the work directory of the
	 * current process, distinguishing between
	 * report file names and other implicit ones.
	 * Let the caller fail because we already know
	 * it doesn't exist at any known location.
	 */
	if (report && report_index >= 0) {
		have_report_dir = (r->reportstorun[report_index].dir[0] != 0);
		if (have_report_dir)
			file = g_strdup_printf("%s/%s", r->reportstorun[report_index].dir, filename);
		else
			file = g_strdup(filename);
	} else
		file = g_strdup(filename);

	if (stat(file, &st) == 0)
		r_info(r, "get_filename: File found: %s\n", file);
	else
		r_info(r, "get_filename: File not found, expect errors. Filename: %s\n", file);

	return file;
}

static gint rlib_execute_queries(rlib *r) {
	gint i;

	for(i=0;i<r->queries_count;i++) {
		r->results[i]->input = NULL;
		r->results[i]->result = NULL;
	}

	for(i=0;i<r->queries_count;i++) {

		r->results[i]->input = r->queries[i]->input;
		r->results[i]->name =  r->queries[i]->name;
		r->results[i]->result = INPUT(r,i)->new_result_from_query(INPUT(r,i), r->queries[i]->sql);
		r->results[i]->next_failed = FALSE;
		r->results[i]->navigation_failed = FALSE;
		if(r->results[i]->result == NULL) {
			r_error(r, "Failed To Run A Query [%s]: %s\n", r->queries[i]->sql, INPUT(r,i)->get_error(INPUT(r,i)));
			return FALSE;
		} else {
			INPUT(r,i)->first(INPUT(r,i), r->results[i]->result);
		}
	}
	return TRUE;
}

DLL_EXPORT_SYM gint rlib_execute(rlib *r) {
	gint i;
	r->now = time(NULL);

	if(r->format == RLIB_FORMAT_HTML) {
		gchar *param;
		rlib_html_new_output_filter(r);
		param = g_hash_table_lookup(r->output_parameters, "debugging");
		if(param != NULL && strcmp(param, "yes") == 0)
			r->html_debugging = TRUE; 	
	} 

	if(r->queries_count < 1) {
		r_warning(r,"Warning: No queries added to report\n");
	} else
		rlib_execute_queries(r);

	LIBXML_TEST_VERSION

	xmlKeepBlanksDefault(0);
	for (i = 0; i < r->parts_count; i++) {
		r->parts[i] = parse_part_file(r, i);
		xmlCleanupParser();
		if (r->parts[i] == NULL) {
			r_error(r,"Failed to load a report file [%s]\n", r->reportstorun[i].name);
			return -1;
		}
	}

	rlib_resolve_metadata(r);
	rlib_resolve_followers(r);

	rlib_make_report(r);
	
	rlib_finalize(r);
	r->did_execute = TRUE;
	return 0;
}

DLL_EXPORT_SYM gchar *rlib_get_content_type_as_text(rlib *r) {
	static char buf[256];
	gchar *filename = g_hash_table_lookup(r->output_parameters, "csv_file_name");
	
	if(r->did_execute == TRUE) {
		if(r->format == RLIB_CONTENT_TYPE_PDF) {
			sprintf(buf, "Content-Type: application/pdf\nContent-Length: %ld%c", OUTPUT(r)->get_output_length(r), 10);
			return buf;
		}
		if(r->format == RLIB_CONTENT_TYPE_CSV) {
			
			if(filename == NULL)
				return (gchar *)RLIB_WEB_CONTENT_TYPE_CSV;
			else {
				sprintf(buf, RLIB_WEB_CONTENT_TYPE_CSV_FORMATTED, filename);
				return buf;
			}
		} else {
#if DISABLE_UTF8		
			const char *charset = "ISO-8859-1";
#else
			const char *charset = r->output_encoder_name != NULL ? r->output_encoder_name: "UTF-8";
#endif
			if(r->format == RLIB_CONTENT_TYPE_HTML) {
				g_snprintf(buf, sizeof(buf), RLIB_WEB_CONTENT_TYPE_HTML, charset);
				return buf;
			} else {
				g_snprintf(buf, sizeof(buf), RLIB_WEB_CONTENT_TYPE_TEXT, charset);
				return buf;
			}
		}
	}
	r_error(r,"Content type code unknown");
	return (gchar *)"UNKNOWN";
}

DLL_EXPORT_SYM gint rlib_spool(rlib *r) {
	if(r->did_execute == TRUE) {
		OUTPUT(r)->spool_private(r);
		return 0;
	}
	return -1;
}

DLL_EXPORT_SYM gint rlib_set_output_format(rlib *r, int format) {
	r->format = format;
	return 0;
}

DLL_EXPORT_SYM gint rlib_add_resultset_follower_n_to_1(rlib *r, gchar *leader, gchar *leader_field, gchar *follower, gchar *follower_field) {
	gint ptr_leader = -1, ptr_follower = -1;
	gint x;

	if(r->resultset_followers_count > (RLIB_MAXIMUM_FOLLOWERS-1)) {
		return -1;
	}

	for(x=0;x<r->queries_count;x++) {
		if(!strcmp(r->queries[x]->name, leader))
			ptr_leader = x;
		if(!strcmp(r->queries[x]->name, follower))
			ptr_follower = x;
	}
	
	if(ptr_leader == -1) {
		r_error(r,"rlib_add_resultset_follower: Could not find leader!\n");
		return -1;
	}
	if(ptr_follower == -1) {
		r_error(r,"rlib_add_resultset_follower: Could not find follower!\n");
		return -1;
	}
	if(ptr_follower == ptr_leader) {
		r_error(r,"rlib_add_resultset_follower: Followes can't be leaders ;)!\n");
		return -1;
	}
	r->followers[r->resultset_followers_count].leader = ptr_leader;
	r->followers[r->resultset_followers_count].leader_field = g_strdup(leader_field);
	r->followers[r->resultset_followers_count].follower = ptr_follower;
	r->followers[r->resultset_followers_count++].follower_field = g_strdup(follower_field);

	return 0;
}

DLL_EXPORT_SYM gint rlib_add_resultset_follower(rlib *r, gchar *leader, gchar *follower) {
	return rlib_add_resultset_follower_n_to_1(r, leader, NULL, follower, NULL);
}

DLL_EXPORT_SYM gint rlib_set_output_format_from_text(rlib *r, gchar *name) {
	r->format = rlib_format_get_number(name);

	if(r->format == -1)
		r->format = RLIB_FORMAT_TXT;
	return 0;
}

DLL_EXPORT_SYM gchar *rlib_get_output(rlib *r) {
	if(r->did_execute) 
		return OUTPUT(r)->get_output(r);
	else
		return NULL;
}

DLL_EXPORT_SYM gint rlib_get_output_length(rlib *r) {
	if(r->did_execute) 
		return OUTPUT(r)->get_output_length(r);
	else
		return 0;
}

DLL_EXPORT_SYM gboolean rlib_signal_connect(rlib *r, gint signal_number, gboolean (*signal_function)(rlib *, gpointer), gpointer data) {
	r->signal_functions[signal_number].signal_function = signal_function;
	r->signal_functions[signal_number].data = data;
	return TRUE;
}

DLL_EXPORT_SYM gboolean rlib_add_function(rlib *r, gchar *function_name, gboolean (*function)(rlib *, struct rlib_pcode *code, struct rlib_value_stack *, struct rlib_value *this_field_value, gpointer user_data), gpointer user_data) {
	struct rlib_pcode_operator *rpo = g_new0(struct rlib_pcode_operator, 1);
	rpo->tag = g_strconcat(function_name, "(", NULL);	
	rpo->taglen = strlen(rpo->tag);
	rpo->precedence = 0;
	rpo->is_op = TRUE;
	rpo->opnum = 999999;
	rpo->is_function = TRUE;
	rpo->execute = function;
	rpo->user_data = user_data;
	r->pcode_functions = g_slist_append(r->pcode_functions, rpo);
	return TRUE;
}

DLL_EXPORT_SYM gboolean rlib_signal_connect_string(rlib *r, gchar *signal_name, gboolean (*signal_function)(rlib *, gpointer), gpointer data) {
	gint signal = -1;
	if(!strcasecmp(signal_name, "row_change"))
		signal = RLIB_SIGNAL_ROW_CHANGE;
	else if(!strcasecmp(signal_name, "report_done"))
		signal = RLIB_SIGNAL_REPORT_DONE;
	else if(!strcasecmp(signal_name, "report_start"))
		signal = RLIB_SIGNAL_REPORT_START;
	else if(!strcasecmp(signal_name, "report_iteration"))
		signal = RLIB_SIGNAL_REPORT_ITERATION;
	else if(!strcasecmp(signal_name, "part_iteration"))
		signal = RLIB_SIGNAL_PART_ITERATION;
	else if(!strcasecmp(signal_name, "precalculation_done"))
		signal = RLIB_SIGNAL_PRECALCULATION_DONE;
	else {
		r_error(r,"Unknowm SIGNAL [%s]\n", signal_name);
		return FALSE;
	}
	return rlib_signal_connect(r, signal, signal_function, data);
}

DLL_EXPORT_SYM gboolean rlib_query_refresh(rlib *r) {
	rlib_free_results(r);
	rlib_execute_queries(r);
	rlib_fetch_first_rows(r);
	return TRUE;
}

DLL_EXPORT_SYM gint rlib_add_parameter(rlib *r, const gchar *name, const gchar *value) {
	g_hash_table_insert(r->parameters, g_strdup(name), g_strdup(value));
	return TRUE;
}

/*
*  Returns TRUE if locale was actually set, otherwise, FALSE
*/
DLL_EXPORT_SYM gint rlib_set_locale(rlib *r, gchar *locale) {

#if DISABLE_UTF8
	r->special_locale = g_strdup(locale);
#else
	r->special_locale = g_strdup(make_utf8_locale(locale));
#endif
	return TRUE;
}

DLL_EXPORT_SYM gchar * rlib_bindtextdomain(rlib *r, gchar *domainname, gchar *dirname) {
	return bindtextdomain(domainname, dirname);
}

DLL_EXPORT_SYM void rlib_set_radix_character(rlib *r, gchar radix_character) {
	r->radix_character = radix_character;
}

/**
 * put calls to this where you want to debug, then just set a breakpoint here.
 */
void rlib_trap() {
	return;
}

DLL_EXPORT_SYM void rlib_set_output_parameter(rlib *r, gchar *parameter, gchar *value) {
	g_hash_table_insert(r->output_parameters, g_strdup(parameter), g_strdup(value));
}

DLL_EXPORT_SYM void rlib_set_output_encoding(rlib *r, const char *encoding) {
	const char *new_encoding = (encoding ? encoding : "UTF-8");

	if (strcasecmp(new_encoding, "UTF-8") == 0 ||
			strcasecmp(new_encoding, "UTF8") == 0)
		r->output_encoder = (GIConv) -1;
	else
		r->output_encoder = rlib_charencoder_new(new_encoding, "UTF-8");
	r->output_encoder_name  = g_strdup(new_encoding);
}

DLL_EXPORT_SYM gint rlib_set_datasource_encoding(rlib *r, gchar *input_name, gchar *encoding) {
	int i;
	struct input_filter *tif;
	
	for (i=0;i<r->inputs_count;i++) {
		tif = r->inputs[i].input;
		if (strcmp(r->inputs[i].name, input_name) == 0) {
			rlib_charencoder_free(tif->info.encoder);
			tif->info.encoder = rlib_charencoder_new("UTF-8", encoding);
			return 0;
		}
	}
	r_error(r,"Error.. datasource [%s] does not exist\n", input_name);
	return -1;
}

DLL_EXPORT_SYM gint rlib_graph_set_x_minor_tick(rlib *r, gchar *graph_name, gchar *x_value) {
	struct rlib_graph_x_minor_tick *gmt = g_new0(struct rlib_graph_x_minor_tick, 1);
	gmt->graph_name = g_strdup(graph_name);
	gmt->x_value = g_strdup(x_value);
	gmt->by_name = TRUE;
	r->graph_minor_x_ticks = g_slist_append(r->graph_minor_x_ticks, gmt);
	return TRUE;
}

DLL_EXPORT_SYM gint rlib_graph_set_x_minor_tick_by_location(rlib *r, gchar *graph_name, gint location) {
	struct rlib_graph_x_minor_tick *gmt = g_new0(struct rlib_graph_x_minor_tick, 1);
	gmt->by_name = FALSE;
	gmt->location = location;
	gmt->graph_name = g_strdup(graph_name);
	gmt->x_value = NULL;
	r->graph_minor_x_ticks = g_slist_append(r->graph_minor_x_ticks, gmt);
	return TRUE;
}

DLL_EXPORT_SYM gint rlib_graph_add_bg_region(rlib *r, gchar *graph_name, gchar *region_label, gchar *color, gfloat start, gfloat end) {
	struct rlib_graph_region *gr = g_new0(struct rlib_graph_region, 1);
	gr->graph_name = g_strdup(graph_name);
	gr->region_label = g_strdup(region_label);
	rlib_parsecolor(&gr->color, color);
	gr->start = start;
	gr->end = end;
	r->graph_regions = g_slist_append(r->graph_regions, gr);
	
	return TRUE;
}

DLL_EXPORT_SYM gint rlib_graph_clear_bg_region(rlib *r, gchar *graph_name) {

	return TRUE;
}

#ifdef VERSION
const gchar *rpdf_version(void);
DLL_EXPORT_SYM const gchar *rlib_version(void) {
#if 0
#if DISABLE_UTF8
const gchar *charset="8859-1";
#else
const gchar *charset="UTF8";
#endif
r_debug("rlib_version: version=[%s], CHARSET=%s, RPDF=%s", VERSION, charset, rpdf_version());
#endif
	return VERSION;
}
#else
DLL_EXPORT_SYM const gchar *rlib_version(void) {
	return "Unknown";
}
#endif
