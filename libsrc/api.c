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
 * This module implements the C language API (Application Programming Interface)
 * for the RLIB library functions.
 */

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libxml/parser.h>

#include "rlib.h"
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

rlib * rlib_init_with_environment(struct environment_filter *environment) {
	rlib *r;
	char *env;
	
	r = g_new0(rlib, 1);

	if(environment == NULL)
		rlib_new_c_environment(r);
	else
		ENVIRONMENT(r) = environment;

	env = ENVIRONMENT(r)->rlib_resolve_memory_variable("RLIB_DEBUGGING");
	r->debug = !(env == NULL || *env == '\0');

	env = ENVIRONMENT(r)->rlib_resolve_memory_variable("RLIB_CREATE_TEST_CASE");
	r->output_testcase = (env && (strcasecmp(env, "yes") == 0 || strcasecmp(env, "true") == 0 || strcasecmp(env, "on") == 0  || strcmp(env, "1") == 0));

	if (r->output_testcase) {
		gchar *test;
		/* This directory must exist beforehand and should be writable by the process */
		r->testcase_dir = ENVIRONMENT(r)->rlib_resolve_memory_variable("RLIB_TEST_CASE_DIR");
		if (!r->testcase_dir && strlen(r->testcase_dir) == 0)
			r->testcase_dir = "/tmp";

		test = g_strdup_printf("%s/testdir", r->testcase_dir);
		if (mkdir(test, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno == EACCES) {
			r_warning(r, "Warning: RLIB_TEST_CASE_DIR ('%s') is not writable\n", r->testcase_dir);
			r->testcase_dir = NULL;
			r->output_testcase = FALSE;
		} else
			rmdir(test);

		g_free(test);
	}

	if (r->output_testcase) {
		/*
		 * 2x16K should be enough for most cases without
		 * implicit realloc() calls.
		 *
		 * r->testcase_code will be automatically constructed
		 * from the XML parts and the datasources. Both the XMLs
		 * and the datasources will be embedded in the code,
		 * this means using array datasources.
		 *
		 * r->testcase_code2 will be used to collect the RLIB API
		 * calls, except:
		 *   rlib_set_output_format*()
		 *   rlib_add_datasource_*()
		 *   rlib_add_query*()
		 * The extra API calls must all be added to the test case
		 * after the XMLs, the datasources and the synthetic
		 * rlib_add_query_as*() calls are transformed and printed
		 * into the test case.
		 */
		r->testcase_code = g_string_sized_new(16384);
		g_string_printf(r->testcase_code, "int main(int argc, char **argv) {\n");
		g_string_append(r->testcase_code, "\trlib *r;\n\n");
		g_string_append(r->testcase_code, "\tif (argc == 1) {\n\t\tprintf(\"usage: %s [ pdf | xml | txt | csv | html ]\\n\", argv[0]);\n\t\treturn 1;\n\t}\n\n");
		g_string_append(r->testcase_code, "\tr = rlib_init();\n");
		g_string_append(r->testcase_code, "\trlib_set_output_format_from_text(r, argv[1]);\n");
		g_string_append(r->testcase_code, "\trlib_add_datasource_array(r, \"local_array\");\n");
		r->testcase_code2 = g_string_sized_new(16384);
	}

	r->output_parameters = g_hash_table_new_full (g_str_hash, g_str_equal, string_destroyer, string_destroyer);
	r->input_metadata = g_hash_table_new_full (g_str_hash, g_str_equal, string_destroyer, metadata_destroyer);
	r->parameters = g_hash_table_new_full (g_str_hash, g_str_equal, string_destroyer, string_destroyer);

	r->radix_character = '.';
	
	make_all_locales_utf8();
	r->did_execute = FALSE;
	r->current_locale = g_strdup(setlocale(LC_ALL, NULL));
	rlib_pcode_find_index(r);
	return r;
}


rlib * rlib_init() {
	return rlib_init_with_environment(NULL);
}

struct rlib_queries *rlib_alloc_query_space(rlib *r) {
	struct rlib_queries *query = NULL;
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
		struct rlib_queries **queries;
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

	query = g_malloc0(sizeof(struct rlib_queries));
	result = g_malloc0(sizeof(struct rlib_results));

	if (query == NULL || result == NULL) {
		g_free(query);
		g_free(result);
		r_error(r, "rlib_alloc_query_space: Out of memory!\n");
		return NULL;
	}

	r->queries[r->queries_count] = query;
	r->results[r->queries_count] = result;

	r->queries_count++;

	return query;
}

gint rlib_add_query_pointer_as(rlib *r, const gchar *input_source, gchar *sql, const gchar *name) {
	struct rlib_queries *query;
	gint i;

	/*
	 * The automatic test case generator converts everything
	 * to a C array datasource, calling this function is not
	 * emitted into the test case as is.
	 */

	query = rlib_alloc_query_space(r);
	if (!query)
		return -1;

	query->sql = sql;
	query->name = g_strdup(name);
	for(i=0;i<r->inputs_count;i++) {
		if(!strcmp(r->inputs[i].name, input_source)) {
			query->input = r->inputs[i].input;
			return r->queries_count;
		}
	}

	r_error(r, "rlib_add_query_as: Could not find input source [%s]!\n", input_source);
	return -1;
}

gint rlib_add_query_as(rlib *r, const gchar *input_source, const gchar *sql, const gchar *name) {
	/*
	 * The automatic test case generator converts everything
	 * to a C array datasource, calling this function is not
	 * emitted into the test case as is.
	 */
	return rlib_add_query_pointer_as(r, input_source, g_strdup(sql), name);
}

gint rlib_add_report(rlib *r, const gchar *name) {
	gchar *tmp;
	int i, found_dir_sep = 0, last_dir_sep;

	/*
	 * The automatic test case generator converts every
	 * report XML to an in-memory string, calling this
	 * function is not emitted into the test case as is.
	 */

	if(r->parts_count > (RLIB_MAXIMUM_REPORTS-1)) {
		return - 1;
	}

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

gint rlib_add_report_from_buffer(rlib *r, gchar *buffer) {
	char cwdpath[PATH_MAX + 1];
	char *cwd;
#ifdef _WIN32
	char *tmp;
#endif

	/*
	 * The automatic test case generator converts every
	 * report XML to an in-memory string, calling this
	 * function is not emitted into the test case as is.
	 */

	if(r->parts_count > (RLIB_MAXIMUM_REPORTS-1)) {
		return - 1;
	}

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
	for (i = 0; i < len; i++)
		if (tmp[i] == '\\')
			tmp[i] = '/';
#endif
	r->reportstorun[r->parts_count].type = RLIB_REPORT_TYPE_BUFFER;
	r->parts_count++;
	return r->parts_count;
}

gint rlib_add_search_path(rlib *r, const gchar *path) {
	gchar *path_copy;
#ifdef _WIN32
	int len;
#endif

	/* Don't add useless search paths */
	if (path == NULL)
		return -1;
	if (strlen(path) == 0)
		return -1;

	if (r->output_testcase) {
		int pos;

		g_string_append(r->testcase_code2, "\trlib_add_search_path(r, \"");
		for (pos = 0; path[pos]; pos++) {
			switch (path[pos]) {
			case '\n':
				g_string_append(r->testcase_code2, "\\n");
				break;
			case '\t':
				g_string_append(r->testcase_code2, "\\t");
				break;
			case '\\':
			case '"':
				g_string_append_c(r->testcase_code2, '\\');
				/* fall through */
			default:
				g_string_append_c(r->testcase_code2, path[pos]);
				break;
			}
		}
		g_string_append(r->testcase_code2, "\");\n");
	}

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
 *     index to r->reportstorun[] array, or
 *     -1 to try to find relative to every reports
 */
gchar *get_filename(rlib *r, const char *filename, int report_index, gboolean report, gboolean relative_filename) {
	int have_report_dir = 0, ri;
	gchar *file;
	struct stat st;
	GSList *elem;

#ifdef _WIN32
	/*
	 * If the filename starts with a drive label,
	 * it is an absolute file name. Return it.
	 */
	if (strlen(filename) >= 2) {
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

	for(i=0;i<r->queries_count;i++) {
		r->results[i]->input = NULL;
		r->results[i]->result = NULL;
	}

	for(i=0;i<r->queries_count;i++) {

		r->results[i]->input = r->queries[i]->input;
		r->results[i]->name =  r->queries[i]->name;
		r->results[i]->result = INPUT(r,i)->new_result_from_query(INPUT(r,i), r->queries[i]);
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

G_LOCK_DEFINE_STATIC(testcase_lock);
static int testcase_count = 0;

gint rlib_execute(rlib *r) {
	gint i;
	int tc_num = 0;

	r->now = time(NULL);

	if(r->format == RLIB_FORMAT_HTML) {
		gchar *param;
		rlib_html_new_output_filter(r);
		param = g_hash_table_lookup(r->output_parameters, "debugging");
		if(param != NULL && strcmp(param, "yes") == 0)
			r->html_debugging = TRUE; 	
	} 

	if (r->queries_count < 1)
		r_warning(r,"Warning: No queries added to report\n");
	else {
		rlib_execute_queries(r);

	}

	LIBXML_TEST_VERSION

	xmlKeepBlanksDefault(0);
	for(i=0;i<r->parts_count;i++) {
		r->parts[i] = parse_part_file(r, i);
		xmlCleanupParser();		
		if(r->parts[i] == NULL) {
			r_error(r,"Failed to load a report file [%s]\n", r->reportstorun[i].name);
			return -1;
		}
	}

	rlib_resolve_metadata(r);
	rlib_resolve_followers(r);

	if (r->output_testcase) {
		int l = 0;
		int i;

		G_LOCK(testcase_lock);
		tc_num = testcase_count;
		testcase_count++;
		G_UNLOCK(testcase_lock);

		/* Worst case, every character must be escaped */
		for (i = 0; i < r->parts_count; i++)
			l += r->parts[i]->xml_dump_len * 2;

		r->testcase = g_string_sized_new(l);
		g_string_printf(r->testcase, "/*\n * Automatic test case created by RLIB %s\n", rlib_version());
		g_string_append(r->testcase, " * Compile it with:\n");
		g_string_append_printf(r->testcase, " * gcc -Wall `pkg-config --cflags --libs rlib glib-2.0` -o testcase-%d-%d testcase-%d-%d.c\n */\n\n", getpid(), tc_num, getpid(), tc_num);
		g_string_append(r->testcase, "#include <stdio.h>\n");
		g_string_append(r->testcase, "#include <rlib/rlib.h>\n\n");

		for (i = 0; i < r->parts_count; i++) {
			int pos;

			g_string_append_printf(r->testcase, "static char *xml%d = \"", i);

			for (pos = 0; pos < r->parts[i]->xml_dump_len; pos++) {
				switch (r->parts[i]->xml_dump[pos]) {
				case '\n':
					g_string_append(r->testcase, "\\n\"\n\t\"");
					break;
				case '\t':
					g_string_append(r->testcase, "\\t");
					break;
				case '"':
				case '\\':
					g_string_append_c(r->testcase, '\\');
					/* fall through */
				default:
					g_string_append_c(r->testcase, r->parts[i]->xml_dump[pos]);
					break;
				}
			}

			g_string_append(r->testcase, "\";\n\n");

			g_string_append_printf(r->testcase_code, "\trlib_add_report_from_buffer(r, xml%d);\n", i);

			xmlFree(r->parts[i]->xml_dump);
			r->parts[i]->xml_dump = NULL;
			r->parts[i]->xml_dump_len = 0;
		}

		{
			GString *filename;
			FILE *tc_file;
			int i;

			filename = g_string_new(NULL);
			if (r->testcase_dir)
				g_string_printf(filename, "%s/testcase-%d-%d.c", r->testcase_dir, getpid(), tc_num);
			else
				g_string_printf(filename, "testcase-%d-%d.c", getpid(), tc_num);

			tc_file = fopen(filename->str, "w+");
			if (tc_file) {
				fprintf(tc_file, "%s", r->testcase->str);
				g_string_free(r->testcase, TRUE);
				r->testcase = NULL;

				for (i = 0; i < r->queries_count; i++) {
					struct input_filter *in = INPUT(r, i);
					struct rlib_results *rs = r->results[i];
					char *qname = r->queries[i]->name;
					int nfields = in->num_fields(in, rs->result);
					int nrows = 0;
					int i;

					/* Print datasources... */
					fprintf(tc_file, "char *%s[][%d] = {\n\t{ ", qname, nfields);

					for (i = 0; i < nfields; i++) {
						if (i)
							fprintf(tc_file, ", ");
						fprintf(tc_file, "\"%s\"", in->get_field_name(in, rs->result, GINT_TO_POINTER(i + 1)));
					}

					fprintf(tc_file, " }\n");

					in->first(in, rs->result);
					while (!in->isdone(in, rs->result)) {
						nrows++;
						fprintf(tc_file, "\t, { ");
						for (i = 0; i < nfields; i++) {
							gchar *value = in->get_field_value_as_string(in, rs->result, GINT_TO_POINTER(i + 1));
							int pos;

							if (i)
								fprintf(tc_file, ", ");

							fprintf(tc_file, "\"");

							if (value) {
								for (pos = 0; value[pos]; pos++) {
									switch (value[pos]) {
									case '\n':
										fprintf(tc_file, "\\n");
										break;
									case '\t':
										fprintf(tc_file, "\\t");
										break;
									case '"':
									case '\\':
										fprintf(tc_file, "\\");
										/* fall through */
									default:
										fprintf(tc_file, "%c", value[pos]);
										break;
									}
								}
							}

							fprintf(tc_file, "\"");
						}

						fprintf(tc_file, " }\n");

						in->next(in, rs->result);
					}

					/*
					 * Reset the datasource for the actual
					 * report execution below.
					 */
					in->first(in, rs->result);
					in->previous(in, rs->result);

					/* ... print contents ... */
					fprintf(tc_file, "};\n\n");
					g_string_append_printf(r->testcase_code, "\trlib_add_query_array_as(r, \"local_array\", %s, %d, %d, \"%s\");\n", qname, nrows + 1, nfields, qname);
				}

				fprintf(tc_file, "%s", r->testcase_code->str);
				g_string_free(r->testcase_code, TRUE);
				r->testcase_code = NULL;

				if (r->testcase_code2->len > 0)
					fprintf(tc_file, "%s", r->testcase_code2->str);
				g_string_free(r->testcase_code2, TRUE);
				r->testcase_code2 = NULL;

				fprintf(tc_file, "\trlib_execute(r);\n");
				fprintf(tc_file, "\trlib_spool(r);\n");
				fprintf(tc_file, "\trlib_free(r);\n");
				fprintf(tc_file, "\treturn 0;\n}\n");
				fclose(tc_file);
			}

			if (r->testcase_dir)
				g_string_printf(filename, "%s/testcase-%d-%d.test", r->testcase_dir, getpid(), tc_num);
			else
				g_string_printf(filename, "testcase-%d-%d.test", getpid(), tc_num);

			tc_file = fopen(filename->str, "w+");
			if (tc_file) {
				GString *vars;

				fprintf(tc_file, "COMMAND=./testcase-%d-%d\n", getpid(), tc_num);
				fprintf(tc_file, "FORMATS=\"pdf xml txt csv html\"\n");
				fprintf(tc_file, "EXPECTED_pdf=\n");
				fprintf(tc_file, "EXPECTED_xml=\n");
				fprintf(tc_file, "EXPECTED_txt=\n");
				fprintf(tc_file, "EXPECTED_csv=\n");
				fprintf(tc_file, "EXPECTED_html=\n");

				vars = ENVIRONMENT(r)->rlib_dump_memory_variables();
				fprintf(tc_file, "%s", vars->str);
				g_string_free(vars, TRUE);

				fclose(tc_file);
			}
		}
	}

	rlib_make_report(r);
	
	rlib_finalize(r);
	r->did_execute = TRUE;
	return 0;
}

gchar * rlib_get_content_type_as_text(rlib *r) {
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
			const char *charset = r->output_encoder_name != NULL ? r->output_encoder_name: "UTF-8";
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

gint rlib_spool(rlib *r) {
	/*
	 * The automatic test case generator emits this call automatically.
	 */
	if(r->did_execute == TRUE) {
		OUTPUT(r)->spool_private(r);
		return 0;
	}
	return -1;
}

gint rlib_set_output_format(rlib *r, int format) {
	/*
	 * The test case adds code for setting the format, don't do it specifically here.
	 */
	r->format = format;
	return 0;
}

gint rlib_add_resultset_follower_n_to_1(rlib *r, gchar *leader, gchar *leader_field, gchar *follower, gchar *follower_field) {
	gint ptr_leader = -1, ptr_follower = -1;
	gint x;

	/*
	 * rlib_add_resultset_follower() also adds code to
	 * the generated test case, so protect this function
	 * to emit a second call with "<nil>" strings.
	 */
	if (r->output_testcase && leader_field && follower_field)
		g_string_append_printf(r->testcase_code2, "\trlib_add_resultset_follower_n_to_1(r, \"%s\", \"%s\", \"%s\", \"%s\");\n", leader, leader_field, follower, follower_field);

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

gint rlib_add_resultset_follower(rlib *r, gchar *leader, gchar *follower) {
	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_add_resultset_follower(r, \"%s\", \"%s\");\n", leader, follower);

	return rlib_add_resultset_follower_n_to_1(r, leader, NULL, follower, NULL);
}

gint rlib_set_output_format_from_text(rlib *r, gchar *name) {
	/*
	 * The generated test case adds code for setting the format.
	 */
	r->format = rlib_format_get_number(name);

	if(r->format == -1)
		r->format = RLIB_FORMAT_TXT;
	return 0;
}

gchar *rlib_get_output(rlib *r) {
	/*
	 * The generated test case uses rlib_spool(),
	 * this function is not emitted into the test case.
	 */
	if(r->did_execute) 
		return OUTPUT(r)->get_output(r);
	else
		return NULL;
}

gint rlib_get_output_length(rlib *r) {
	/*
	 * The generated test case uses rlib_spool(),
	 * this function is not emitted into the test case.
	 */
	if(r->did_execute) 
		return OUTPUT(r)->get_output_length(r);
	else
		return 0;
}

gboolean rlib_signal_connect(rlib *r, gint signal_number, gboolean (*signal_function)(rlib *, gpointer), gpointer data) {	
	if (r->output_testcase) {
		g_string_append_printf(r->testcase_code2,
								"#warning \"Using rlib_signal_connect() needs re-implementing "
								"the original signal handling function in the test case\"\n");
		/*
		 * TODO: there should be a number->symbol name mapping for
		 * signal_number and the test case should use the symbol name.
		 */
		g_string_append_printf(r->testcase_code2, "\t/* rlib_signal_connect(r, %d, signal_function(...), user_data); */\n", signal_number);
	}

	r->signal_functions[signal_number].signal_function = signal_function;
	r->signal_functions[signal_number].data = data;
	return TRUE;
}

gboolean rlib_add_function(rlib *r, gchar *function_name, gboolean (*function)(rlib *, struct rlib_pcode *code, struct rlib_value_stack *, struct rlib_value *this_field_value, gpointer user_data), gpointer user_data) {	
	struct rlib_pcode_operator *rpo = g_new0(struct rlib_pcode_operator, 1);

	if (r->output_testcase) {
		g_string_append_printf(r->testcase_code2,
								"#warning \"Using rlib_add_function() needs re-implementing "
								"the original function in the test case\"\n");
		g_string_append_printf(r->testcase_code2, "\t/* rlib_add_function(r, \"%s\", function(...), user_data); */\n", function_name);
	}

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

gboolean rlib_signal_connect_string(rlib *r, gchar *signal_name, gboolean (*signal_function)(rlib *, gpointer), gpointer data) {
	gint signal = -1;

	/*
	 * TODO: emit this into the test case.
	 * It would need manual adjustments to the test case afterward.
	 */
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

gboolean rlib_query_refresh(rlib *r) {
	rlib_free_results(r);
	rlib_execute_queries(r);
	rlib_fetch_first_rows(r);
	return TRUE;
}

gint rlib_add_parameter(rlib *r, const gchar *name, const gchar *value) {
	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_add_parameter(r, \"%s\", \"%s\");\n", name, value);

	g_hash_table_insert(r->parameters, g_strdup(name), g_strdup(value));
	return TRUE;
}

gint rlib_set_locale(rlib *r, gchar *locale) {
	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_set_locale(r, \"%s\");\n", locale);

	r->special_locale = g_strdup(make_utf8_locale(locale));
	return TRUE;
}

gchar * rlib_bindtextdomain(rlib *r, gchar *domainname, gchar *dirname) {
	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_bindtextdomain(r, \"%s\", \"%s\");\n", domainname, dirname);

	return bindtextdomain(domainname, dirname);
}

void rlib_set_radix_character(rlib *r, gchar radix_character) {
	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_set_radix_character(r, '%c');\n", radix_character);

	r->radix_character = radix_character;
}

void rlib_init_profiler() {
	g_mem_set_vtable(glib_mem_profiler_table);
}


void rlib_dump_profile_stdout(gint profilenum) {
	printf("\nRLIB memory profile #%d:\n", profilenum);
	g_mem_profile();
	fflush(stdout);
}

void rlib_dump_profile(gint profilenum, const gchar *filename) {
	FILE *newout = NULL;
	int fd;
	
	fflush(stdout);
	fd = dup(STDOUT_FILENO); /* get a dup of current stdout */
	if (fd < 0) {
		//r_error(r, "Unable to dup stdout");
		return;
	}
	if (filename) {
		newout = freopen(filename, "ab", stdout);
	}
	if (!newout) dup2(STDERR_FILENO, STDOUT_FILENO); /* Use stderr */
	rlib_dump_profile_stdout(profilenum);
	if (newout) {
		fclose(newout);
	} else {
		//r_error(r,"Could not open memory profile file: %s. Used stderr instead", filename);
	}
	dup2(fd, STDOUT_FILENO); /* restore original stdout and close the dup fd */
	close(fd);
}

/**
 * put calls to this where you want to debug, then just set a breakpoint here.
 */
void rlib_trap() {
	return;
}

void rlib_set_output_parameter(rlib *r, gchar *parameter, gchar *value) {
	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_set_output_parameter(r, \"%s\", \"%s\");\n", parameter, value);

	g_hash_table_insert(r->output_parameters, g_strdup(parameter), g_strdup(value));
}

void rlib_set_output_encoding(rlib *r, const char *encoding) {
	const char *new_encoding = (encoding ? encoding : "UTF-8");

	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_set_output_encoding(r, \"%s\");\n", encoding);

	if (strcasecmp(new_encoding, "UTF-8") == 0 ||
			strcasecmp(new_encoding, "UTF8") == 0)
		r->output_encoder = (GIConv) -1;
	else
		r->output_encoder = rlib_charencoder_new(new_encoding, "UTF-8");
	r->output_encoder_name  = g_strdup(new_encoding);
}

gint rlib_set_datasource_encoding(rlib *r, gchar *input_name, gchar *encoding) {
	int i;
	struct input_filter *tif;

	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_set_datasource_encoding(r, \"%s\", \"%s\");\n", input_name, encoding);

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

gint rlib_graph_set_x_minor_tick(rlib *r, gchar *graph_name, gchar *x_value) {
	struct rlib_graph_x_minor_tick *gmt = g_new0(struct rlib_graph_x_minor_tick, 1);

	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_graph_set_x_minor_tick(r, \"%s\", \"%s\");\n", graph_name, x_value);

	gmt->graph_name = g_strdup(graph_name);
	gmt->x_value = g_strdup(x_value);
	gmt->by_name = TRUE;
	r->graph_minor_x_ticks = g_slist_append(r->graph_minor_x_ticks, gmt);
	return TRUE;
}

gint rlib_graph_set_x_minor_tick_by_location(rlib *r, gchar *graph_name, gint location) {
	struct rlib_graph_x_minor_tick *gmt = g_new0(struct rlib_graph_x_minor_tick, 1);

	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_graph_set_x_minor_tick_by_location(r, \"%s\", %d);\n", graph_name, location);

	gmt->by_name = FALSE;
	gmt->location = location;
	gmt->graph_name = g_strdup(graph_name);
	gmt->x_value = NULL;
	r->graph_minor_x_ticks = g_slist_append(r->graph_minor_x_ticks, gmt);
	return TRUE;
}

gint rlib_graph_add_bg_region(rlib *r, gchar *graph_name, gchar *region_label, gchar *color, gfloat start, gfloat end) {
	struct rlib_graph_region *gr = g_new0(struct rlib_graph_region, 1);

	if (r->output_testcase)
		g_string_append_printf(r->testcase_code2, "\trlib_graph_add_bg_region(r, \"%s\", \"%s\", \"%s\", %lf, %lf);\n", graph_name, region_label, color, start, end);

	gr->graph_name = g_strdup(graph_name);
	gr->region_label = g_strdup(region_label);
	rlib_parsecolor(&gr->color, color);
	gr->start = start;
	gr->end = end;
	r->graph_regions = g_slist_append(r->graph_regions, gr);
	
	return TRUE;
}

gint rlib_graph_clear_bg_region(rlib *r, gchar *graph_name) {

	return TRUE;
}

#ifdef VERSION
const gchar *rpdf_version(void);
const gchar *rlib_version(void) {
#if 0
const gchar *charset="UTF8";
r_debug("rlib_version: version=[%s], CHARSET=%s, RPDF=%s", VERSION, charset, rpdf_version());
#endif
	return VERSION;
}
#else
const gchar *rlib_version(void) {
	return "Unknown";
}
#endif

gint rlib_mysql_report(gchar *hostname, gchar *username, gchar *password, gchar *database, gchar *xmlfilename, gchar *sqlquery, 
gchar *outputformat) {
	rlib *r;
	r = rlib_init();
	if(rlib_add_datasource_mysql(r, "mysql", hostname, username, password, database) == -1)
		return -1;
	rlib_add_query_as(r, "mysql", sqlquery, "example");
	rlib_add_report(r, xmlfilename);
	rlib_set_output_format_from_text(r, outputformat);
	if(rlib_execute(r) == -1)
		return -1;
	rlib_spool(r);
	rlib_free(r);
	return 0;
}

gint rlib_postgres_report(gchar *connstr, gchar *xmlfilename, gchar *sqlquery, gchar *outputformat) {
	rlib *r;
	r = rlib_init();
	if(rlib_add_datasource_postgres(r, "postgres", connstr) == -1)
		return -1;
	rlib_add_query_as(r, "postgres", sqlquery, "example");
	rlib_add_report(r, xmlfilename);
	rlib_set_output_format_from_text(r, outputformat);
	if(rlib_execute(r) == -1)
		return -1;
	rlib_spool(r);
	rlib_free(r);
	return 0;
}

