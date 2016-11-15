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

static void string_destroyer(gpointer data) {
	g_free(data);
}

static void metadata_destroyer(gpointer data) {
	struct rlib_metadata *metadata = data;
	rlib *r = metadata->r;
	rlib_value_free(&metadata->rval_formula);
	rlib_pcode_free(r, metadata->formula_code);
	xmlFree(metadata->xml_formula.xml);
	g_free(data);
}

DLL_EXPORT_SYM rlib *rlib_init_with_environment(struct environment_filter *environment) {
	rlib *r;
	
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

static void rlib_cached_rval_destroy(gpointer data) {
	if (data)
		rlib_value_free(data);
	g_free(data);
}

DLL_EXPORT_SYM struct rlib_query *rlib_alloc_query_space(rlib *r) {
	struct rlib_query_internal *query = NULL;
	struct rlib_results *result = NULL;

	if (r->queries_count == 0) {
		r->queries = g_malloc((r->queries_count + 1) * sizeof(gpointer));
		r->results = g_malloc((r->queries_count + 1) * sizeof(gpointer));		

		if (r->queries == NULL || r->results == NULL) {
			g_free(r->queries);
			g_free(r->results);
			r_error(r, "rlib_alloc_query_space: Out of memory!\n");
			return NULL;
		}
	} else {
		struct rlib_query_internal **queries;
		struct rlib_results **results;

		queries = g_realloc(r->queries, (r->queries_count + 1) * sizeof(void *));
		if (queries == NULL) {
			r_error(r, "rlib_alloc_query_space: Out of memory!\n");
			return NULL;
		}

		results = g_realloc(r->results, (r->queries_count + 1) * sizeof(void *));
		if (results == NULL) {
			r_error(r, "rlib_alloc_query_space: Out of memory!\n");
			return NULL;
		}

		r->queries = queries;
		r->results = results;
	}

	query = g_malloc0(sizeof(struct rlib_query_internal));
	result = g_malloc0(sizeof(struct rlib_results));

	if (query == NULL || result == NULL) {
		g_free(query);
		g_free(result);
		r_error(r, "rlib_alloc_query_space: Out of memory!\n");
		return NULL;
	}

	result->cached_values = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, rlib_cached_rval_destroy);
	if (result->cached_values == NULL) {
		g_free(query);
		g_free(result);
		r_error(r, "rlib_alloc_query_space: Out of memory!\n");
		return NULL;
	}

	r->queries[r->queries_count] = query;
	query->query_index = r->queries_count;
	r->results[r->queries_count] = result;

	r->queries_count++;

	return (struct rlib_query *)query;
}

static struct rlib_query_internal *add_query_pointer_as(rlib *r, const gchar *input_source, gchar *sql, const gchar *name) {
	gint i;

	for (i = 0; i < r->inputs_count; i++) {
		if (!strcmp(r->inputs[i].name, input_source)) {
			gchar *name_copy;
			struct rlib_query_internal *query;

			name_copy = g_strdup(name);
			if (name_copy == NULL) {
				r_error(r, "rlib_add_query_pointer_as: Out of memory!\n");
				return NULL;
			}

			query = (struct rlib_query_internal *)rlib_alloc_query_space(r);
			if (query == NULL) {
				g_free(name_copy);
				return NULL;
			}

			query->sql = sql;
			query->name = name_copy;
			query->input = r->inputs[i].input;

			return query;
		}
	}

	r_error(r, "add_query_pointer_as: Could not find input source [%s]!\n", input_source);
	return NULL;
}

DLL_EXPORT_SYM gint rlib_add_query_pointer_as(rlib *r, const gchar *input_source, gchar *sql, const gchar *name) {
	struct rlib_query_internal *query = add_query_pointer_as(r, input_source, sql, name);

	if (query == NULL)
		return -1;

	return r->queries_count;
}

DLL_EXPORT_SYM gint rlib_add_query_as(rlib *r, const gchar *input_source, const gchar *sql, const gchar *name) {
	gchar *sql_copy = g_strdup(sql);
	struct rlib_query_internal *query;

	if (sql_copy == NULL)
		return -1;

	query = add_query_pointer_as(r, input_source, sql_copy, name);
	if (query == NULL) {
		free(sql_copy);
		return -1;
	}

	query->sql_allocated = 1;

	return r->queries_count;
}

DLL_EXPORT_SYM gint rlib_add_report(rlib *r, const gchar *name) {
	gchar *tmp;
	int i, found_dir_sep = 0, last_dir_sep = 0;

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
		r->reportstorun[r->parts_count].name = g_strdup(tmp + last_dir_sep + 1);
		tmp[last_dir_sep] = '\0';
		r->reportstorun[r->parts_count].dir = tmp;
	} else {
		r->reportstorun[r->parts_count].name = tmp;
		r->reportstorun[r->parts_count].dir = g_strdup("");
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
 * Decision factor for external file types (e.g. images)
 * whether to use their filenames as is for e.g. embedding
 * or as validated full paths.
 */
gboolean use_relative_filename(rlib *r) {
	return (r->format == RLIB_FORMAT_HTML);
}

/*
 * Try to search for a file and return the first one
 * found at the possible locations.
 *
 * report_index:
 *	index to r->reportstorun[] array, or
 *	-1 to try to find relative to every reports
 */
gchar *get_filename(rlib *r, const char *filename, int report_index, gboolean report, gboolean relative_filename) {
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
			return g_strdup(filename);
		}
	}
#endif
	/*
	 * Absolute filename, on the same drive for Windows,
	 * unconditionally for Un*xes.
	 */
	if (filename[0] == '/') {
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
				if (relative_filename) {
					g_free(file);
					return g_strdup(filename);
				}
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
					if (relative_filename) {
						g_free(file);
						return g_strdup(filename);
					}
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
						if (relative_filename) {
							g_free(file);
							return g_strdup_printf("%s/%s", search_path, filename);
						}
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
							if (relative_filename) {
								g_free(file);
								return g_strdup_printf("%s/%s", search_path, filename);
							}
							return file;
						}
						g_free(file);
					}
				}
			}
		}

		file = g_strdup_printf("%s/%s", search_path, filename);

		if (stat(file, &st) == 0) {
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
		if (have_report_dir && !relative_filename)
			file = g_strdup_printf("%s/%s", r->reportstorun[report_index].dir, filename);
		else
			file = g_strdup(filename);
	} else
		file = g_strdup(filename);

	return file;
}

static gint rlib_execute_queries(rlib *r) {
	gint i;
	char *env;
	gint profiling;

	env = getenv("RLIB_PROFILING");
	profiling = !(env == NULL || *env == '\0');

	for (i = 0; i < r->queries_count; i++) {
		r->results[i]->input = NULL;
		r->results[i]->result = NULL;
	}

	for (i = 0; i < r->queries_count; i++) {
		struct timespec ts1, ts2;
		r->results[i]->input = r->queries[i]->input;
		r->results[i]->name =  r->queries[i]->name;
		clock_gettime(CLOCK_MONOTONIC, &ts1);
		if (INPUT(r,i)->set_query_cache_size && r->query_cache_size)
			INPUT(r,i)->set_query_cache_size(INPUT(r,i), r->query_cache_size);
		r->results[i]->result = INPUT(r,i)->new_result_from_query(INPUT(r,i), r->queries[i]);
		clock_gettime(CLOCK_MONOTONIC, &ts2);
		if (profiling) {
			gchar *name = NULL;
			int j;
			long diff = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;

			for (j = 0; j < r->inputs_count; j++) {
				if (r->inputs[j].input == r->results[i]->input) {
					name = r->inputs[j].name;
					break;
				}
			}
			r_warning(r, "rlib_execute_queries: executing query %s:%s took %ld microseconds\n",
						(name ? name : "<unknown>"), r->results[i]->name, diff);
		}
		r->results[i]->next_failed = FALSE;
		r->results[i]->navigation_failed = FALSE;
		if (r->results[i]->result == NULL) {
			r_error(r, "Failed To Run A Query [%s]: %s\n", r->queries[i]->sql, INPUT(r,i)->get_error(INPUT(r,i)));
			return FALSE;
		} else {
			INPUT(r,i)->start(INPUT(r,i), r->results[i]->result);
			INPUT(r,i)->next(INPUT(r,i), r->results[i]->result);
		}
	}
	return TRUE;
}

gint rlib_parse_internal(rlib *r, gboolean allow_fail) {
	gint i;
	char *env;
	gint profiling;
	struct timespec ts1, ts2;

	if (r->did_parse)
		return 0;

	env = getenv("RLIB_PROFILING");
	profiling = !(env == NULL || *env == '\0');

	clock_gettime(CLOCK_MONOTONIC, &ts1);

	r->now = time(NULL);

	if (r->format == RLIB_FORMAT_HTML) {
		gchar *param;
		param = g_hash_table_lookup(r->output_parameters, "debugging");
		if(param != NULL && strcmp(param, "yes") == 0)
			r->html_debugging = TRUE;
	}

	LIBXML_TEST_VERSION

	xmlKeepBlanksDefault(0);
	for (i = 0; i < r->parts_count; i++) {
		r->parts[i] = parse_part_file(r, allow_fail, i);
		xmlCleanupParser();
		if (r->parts[i] == NULL) {
			r_error(r,"Failed to load a report file [%s]\n", r->reportstorun[i].name);
			return -1;
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &ts2);

	if (profiling) {
		long diff = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;

		r_warning(r, "rlib_parse: parsing the report took %ld microseconds\n", diff);
	}

	r->did_parse = TRUE;

	return 0;
}

DLL_EXPORT_SYM gint rlib_parse(rlib *r) {
	return rlib_parse_internal(r, FALSE);
}

DLL_EXPORT_SYM gint rlib_execute(rlib *r) {
	char *env;
	gint profiling;
	struct timespec ts1, ts2;

	if (!r->did_parse) {
		gint parse = rlib_parse_internal(r, TRUE);

		if (parse != 0)
			return -1;
	}

	env = getenv("RLIB_PROFILING");
	profiling = !(env == NULL || *env == '\0');

	r->now = time(NULL);

	if (r->queries_count < 1) {
		r_warning(r,"Warning: No queries added to report\n");
	} else {
		clock_gettime(CLOCK_MONOTONIC, &ts1);

		rlib_execute_queries(r);

		clock_gettime(CLOCK_MONOTONIC, &ts2);

		if (profiling) {
			long diff = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;

			r_warning(r, "rlib_execute: running the queries took %ld microseconds\n", diff);
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &ts1);

	rlib_resolve_metadata(r);
	rlib_resolve_followers(r);

	rlib_make_report(r);

	rlib_finalize(r);
	r->did_execute = TRUE;

	clock_gettime(CLOCK_MONOTONIC, &ts2);

	if (profiling) {
		long diff = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;

		r_warning(r, "rlib_execute: creating report took %ld microseconds\n", diff);
	}

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

static void duplicate_path(rlib *r, struct rlib_query_internal *query, gint *visited) {
	GList *list;

	visited[query->query_index] = 1;

	for (list = query->followers; list; list = list->next) {
		struct rlib_resultset_followers *f = list->data;
		gint follower_idx = f->follower;
		duplicate_path(r, r->queries[follower_idx], visited);
	}
}

static gint rlib_add_resultset_follower_common(rlib *r, gchar *leader, gchar *leader_field, gchar *follower, gchar *follower_field) {
	gint *visited;
	struct rlib_resultset_followers *f;
	gint leader_idx = -1, follower_idx = -1;
	gint x;
	struct rlib_query_internal *top;
	gboolean followers_circular = FALSE;

	if (!r->queries_count)
		return -1;

	for (x = 0; x < r->queries_count; x++) {
		if (!strcmp(r->queries[x]->name, leader))
			leader_idx = x;
		if (!strcmp(r->queries[x]->name, follower))
			follower_idx = x;
	}

	if (leader_idx == -1) {
		r_error(r, "Could not find leader!\n");
		return -1;
	}
	if (follower_idx == -1) {
		r_error(r, "Could not find follower!\n");
		return -1;
	}
	if (follower_idx == leader_idx) {
		r_error(r,"The leader and follower cannot be identical!\n");
		return -1;
	}
	if (follower_idx == 0) {
		r_error(r, "The first added query cannot be a follower!\n");
		return -1;
	}

	/*
	 * Test for straight circularity
	 */
	top = r->queries[leader_idx];
	while (top->leader) {
		if (top->leader == r->queries[follower_idx]) {
			followers_circular = TRUE;
			break;
		}
		top = top->leader;
	}

	if (followers_circular) {
		r_error(r, "Circular follower!\n");
		return -1;
	}

	visited = g_new0(gint, r->queries_count);
	if (!visited) {
		r_error(r, "Out of memory\n");
		return -1;
	}
	duplicate_path(r, top, visited);
	if (visited[follower_idx]) {
		g_free(visited);
		r_error(r, "Follower cannot occur twice in the same follower tree!\n");
		return -1;
	}

	g_free(visited);

	f = g_new0(struct rlib_resultset_followers, 1);
	if (!f) {
		r_error(r, "Out of memory\n");
		return -1;
	}

	r->queries[follower_idx]->leader = r->queries[leader_idx];
	f->leader = leader_idx;
	f->leader_field = g_strdup(leader_field);
	f->follower = follower_idx;
	f->follower_field = g_strdup(follower_field);

	r->queries[leader_idx]->followers = g_list_append(r->queries[leader_idx]->followers, f);

	if (f->leader_field && f->follower_field)
		r->queries[leader_idx]->fcount_n1++;
	else
		r->queries[leader_idx]->fcount++;

	return 0;
}

DLL_EXPORT_SYM gint rlib_add_resultset_follower_n_to_1(rlib *r, gchar *leader, gchar *leader_field, gchar *follower, gchar *follower_field) {
	if (!leader_field || !follower_field) {
		r_error(r, "The leader_field and follower_field both must be valid!\n");
		return -1;
	}

	return rlib_add_resultset_follower_common(r, leader, leader_field, follower, follower_field);
}

DLL_EXPORT_SYM gint rlib_add_resultset_follower(rlib *r, gchar *leader, gchar *follower) {
	return rlib_add_resultset_follower_common(r, leader, NULL, follower, NULL);
}

DLL_EXPORT_SYM gint rlib_set_output_format_from_text(rlib *r, gchar *name) {
	r->format = rlib_format_get_number(name);

	if (r->format == -1)
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
		r_error(r,"Unknown SIGNAL [%s]\n", signal_name);
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

DLL_EXPORT_SYM gchar * rlib_bindtextdomain(rlib *r UNUSED, gchar *domainname, gchar *dirname) {
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

DLL_EXPORT_SYM void rlib_set_query_cache_size(rlib *r, gint cache_size) {
	if (cache_size <= 0)
		cache_size = -1;

	r->query_cache_size = cache_size;
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

DLL_EXPORT_SYM gint rlib_graph_clear_bg_region(rlib *r UNUSED, gchar *graph_name UNUSED) {
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
