/*
 *  Copyright (C) 2003 SICOM Systems, INC.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>

#include "rlib.h"
#include "rlib_input.h"


rlib * rlib_init_with_environment(struct environment_filter *environment) {
	rlib *r;
	
	init_signals();

	r = g_new0(rlib, 1);

	if(environment == NULL)
		rlib_new_c_environment(r);
	else
		ENVIRONMENT(r) = environment;
	return r;
}


rlib * rlib_init() {
	return rlib_init_with_environment(NULL);
}

gint rlib_add_query_as(rlib *r, gchar *input_source, gchar *sql, gchar *name) {
	gint i;
	if(r->queries_count > (RLIB_MAXIMUM_QUERIES-1)) {
		return -1;
	}

	r->queries[r->queries_count].sql = sql;
	r->queries[r->queries_count].name = name;
	for(i=0;i<r->inputs_count;i++) {
		if(!strcmp(r->inputs[i].name, input_source)) {
			r->queries[r->queries_count].input = r->inputs[i].input;
		}
	}
	
	r->queries_count++;
	return r->queries_count;
}

gint rlib_add_report(rlib *r, gchar *name, gchar *mainloop) {
	if(r->reports_count > (RLIB_MAXIMUM_REPORTS-1)) {
		return - 1;
	}
	
	r->reportstorun[r->reports_count].name = name;
	r->reportstorun[r->reports_count].query = mainloop;
	r->reports_count++;
	return r->reports_count;
}

gint rlib_execute(rlib *r) {
	gint i,j;
	char newfile[MAXSTRLEN];

	r->now = time(NULL);// snapshot of the current date/time
	for(i=0;i<r->queries_count;i++) {
		r->results[i].input = r->queries[i].input;
		r->results[i].result = INPUT(r,i)->new_result_from_query(INPUT(r,i), r->queries[i].sql);
		if(r->results[i].result == NULL) {
			rlogit("Failed To Run A Query!\n");			
			return -1;
		}
		r->results[i].name =  r->queries[i].name;
	}

	LIBXML_TEST_VERSION

	xmlKeepBlanksDefault(0);
	for(i=0;i<r->reports_count;i++) {
		sprintf(newfile, "%s.rlib", r->reportstorun[i].name);
		if((r->reports[i] = load_report(newfile)) == NULL)
			r->reports[i] = parse_report_file(r->reportstorun[i].name);
		r->reports[i]->mainloop_query = -1;
		if(r->reportstorun[i].query != NULL) {
			for(j=0;j<r->queries_count;j++) {
				if(!strcmp(r->queries[j].name, r->reportstorun[i].query)) {
					r->reports[i]->mainloop_query = j;
					break;
				}					
			}
		}
		xmlCleanupParser();		
		if(r->reports[i] == NULL) {
			//TODO:FREE REPORT AND ALL ABOVE REPORTS
			rlogit("Failed to run a Report\n");
			return -1;
		}
	}
	
	make_report(r);	
	rlib_finalize(r);
	return 0;
}

gchar * rlib_get_content_type_as_text(rlib *r) {
	if(r->format == RLIB_CONTENT_TYPE_PDF)
		return RLIB_WEB_CONTENT_TYPE_PDF;
	else if(r->format == RLIB_CONTENT_TYPE_HTML)
		return RLIB_WEB_CONTENT_TYPE_HTML;
	else if(r->format == RLIB_CONTENT_TYPE_CSV)
		return RLIB_WEB_CONTENT_TYPE_CSV;
	else
		return RLIB_WEB_CONTENT_TYPE_TEXT;
}

gint rlib_spool(rlib *r) {
	OUTPUT(r)->rlib_spool_private(r);
	return 0;
}

gint rlib_set_output_format(rlib *r, int format) {
	r->format = format;
	return 0;
}

gint rlib_add_resultset_follower(rlib *r, gchar *leader, gchar *follower) {
	gint ptr_leader = -1, ptr_follower = -1;
	gint x;

	if(r->resultset_followers_count > (RLIB_MAXIMUM_FOLLOWERS-1)) {
		return -1;
	}

	for(x=0;x<r->queries_count;x++) {
		if(!strcmp(r->queries[x].name, leader))
			ptr_leader = x;
		if(!strcmp(r->queries[x].name, follower))
			ptr_follower = x;
	}
	
	if(ptr_leader == -1) {
		rlogit("rlib_add_resultset_follower: Could not find leader!\n");
		return -1;
	}
	if(ptr_follower == -1) {
		rlogit("rlib_add_resultset_follower: Could not find follower!\n");
		return -1;
	}
	if(ptr_follower == ptr_leader) {
		rlogit("rlib_add_resultset_follower: Followes can't be leaders ;)!\n");
		return -1;
	}
	r->followers[r->resultset_followers_count].leader = ptr_leader;	
	r->followers[r->resultset_followers_count++].follower = ptr_follower;	

	return 0;
}

gint rlib_set_output_format_from_text(rlib *r, gchar *name) {
	if(!strcasecmp(name, "PDF"))
		r->format = RLIB_FORMAT_PDF;
	else if(!strcasecmp(name, "HTML"))
		r->format = RLIB_FORMAT_HTML;
	else if(!strcasecmp(name, "TXT"))
		r->format = RLIB_FORMAT_TXT;
	else if(!strcasecmp(name, "CSV"))
		r->format = RLIB_FORMAT_CSV;
	else if(!strcasecmp(name, "XML"))
		r->format = RLIB_FORMAT_XML;
	else
		r->format = RLIB_FORMAT_TXT;
	return 0;
}

gchar *rlib_get_output(rlib *r) {
	return OUTPUT(r)->rlib_get_output(r);
}

gint rlib_get_output_length(rlib *r) {
	return OUTPUT(r)->rlib_get_output_length(r);
}


/**
 *	Add name/value pair to the memory constants.
 *  Saves copies of the name and value, NOT pointers.
 */
gint rlib_add_parameter(rlib *r, const gchar *name, const gchar *value) {
	gint result = 1;
	rlib_hashtable_ptr ht = r->htParameters;
	
	if (!ht) { //If no hashtable - add one
		ht = r->htParameters = rlib_hashtable_new_copyboth();
	}
	if (ht) {
		rlib_hashtable_insert(ht, (gpointer) name, (gpointer) value);
		result = 0;
	}
	return result;
}


/*
*  Returns TRUE if locale was actually set, otherwise, FALSE
*/
gint rlib_set_locale(rlib *r, gchar *locale) {
	if (strstr(locale, ".utf8")) r->utf8 = TRUE;
	return (setlocale(LC_ALL, locale) == NULL)? FALSE : TRUE;
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
		rlogit("Unable to dup stdout");
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
		rlogit("Could not open memory profile file: %s. Used stderr instead", filename);
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

//These functions are currently a hack so I can test L10n stuff - Chet
void rlib_set_pdf_font_directories(rlib *r, const char *d1, const char *d2) {
	if (d1) strncpy(r->pdf_fontdir1, d1, sizeof(r->pdf_fontdir1) - 1);
	else *r->pdf_fontdir1 = '\0';
	if (d2) strncpy(r->pdf_fontdir2, d2, sizeof(r->pdf_fontdir2) - 1);
	else *r->pdf_fontdir2 = '\0';
	if (d1 && !d2) strcpy(r->pdf_fontdir2, d1);
	if (d2 && !d1) strcpy(r->pdf_fontdir1, d2);
}


void rlib_set_pdf_conversion(rlib *r, int rptnum, const char *encoding) {
	if ((rptnum >= 0) && (rptnum < r->reports_count)) {
		struct rlib_report *rr = r->reports[rptnum];
		if (rr->pdf_conversion != (iconv_t) -1) iconv_close(rr->pdf_conversion);
		rr->pdf_conversion = (iconv_t) -1;
		if (encoding) {
			rr->pdf_conversion = iconv_open(encoding, "UTF-8");
		}
	}	
}


void rlib_set_pdf_font(rlib *r, const char *encoding, const char *fontname) {
	if (encoding) strncpy(r->pdf_encoding, encoding, sizeof(r->pdf_encoding) - 1);
	if (fontname) strncpy(r->pdf_fontname, fontname, sizeof(r->pdf_fontname) - 1);
}


#ifdef VERSION
gchar *rlib_version() {
	return VERSION;
}
#else
gchar *rlib_version() {
	return "Unknown";
}
#endif


#if HAVE_MYSQL
gint rlib_mysql_report(gchar *hostname, gchar *username, gchar *password, gchar *database, gchar *xmlfilename, gchar *sqlquery, 
gchar *outputformat) {
	rlib *r;
	r = rlib_init();
	if(rlib_add_datasource_mysql(r, "mysql", hostname, username, password, database) == -1)
		return -1;
	rlib_add_query_as(r, "mysql", sqlquery, "example");
	rlib_add_report(r, xmlfilename, "example");
	rlib_set_output_format_from_text(r, outputformat);
	if(rlib_execute(r) == -1)
		return -1;
	rlib_spool(r);
	rlib_free(r);
	return 0;
}
#endif

#if HAVE_POSTGRE
gint rlib_postgre_report(gchar *connstr, gchar *xmlfilename, gchar *sqlquery, gchar *outputformat) {
	rlib *r;
	r = rlib_init();
	if(rlib_add_datasource_postgre(r, "postgre", connstr) == -1)
		return -1;
	rlib_add_query_as(r, "postgre", sqlquery, "example");
	rlib_add_report(r, xmlfilename, "example");
	rlib_set_output_format_from_text(r, outputformat);
	if(rlib_execute(r) == -1)
		return -1;
	rlib_spool(r);
	rlib_free(r);
	return 0;
}
#endif
