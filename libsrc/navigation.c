/*
 *  Copyright (C) 2003-2006 SICOM Systems, INC.
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

#include <string.h>
#include <execinfo.h>

#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"

static gboolean rlib_navigate_n_to_1_check_current(rlib *r, gint resultset_num) {
	GList *fw;

	if (INPUT(r, resultset_num)->isdone(INPUT(r, resultset_num), r->queries[resultset_num]->result))
		return FALSE;

	for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
		struct rlib_resultset_followers *f = fw->data;
		struct rlib_value rval_leader, rval_follower;

		if (INPUT(r, f->follower)->isdone(INPUT(r, f->follower), r->queries[f->follower]->result))
			return FALSE;

		rlib_value_init(r, &rval_leader);
		rlib_value_init(r, &rval_follower);
		rlib_execute_pcode(r, &rval_leader, f->leader_code, NULL);
		rlib_execute_pcode(r, &rval_follower, f->follower_code, NULL);

		if (rvalcmp(r, &rval_leader,&rval_follower) != 0) {
			rlib_value_free(r, &rval_leader);
			rlib_value_free(r, &rval_follower);
			return FALSE;
		}

		rlib_value_free(r, &rval_leader);
		rlib_value_free(r, &rval_follower);
	}

	return TRUE;
}

static gboolean rlib_navigate_n_to_1_check_ended(rlib *r, gint resultset_num) {
	GList *fw;
	gboolean ended = TRUE;

	for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
		struct rlib_resultset_followers *f = fw->data;

		if (!INPUT(r, f->follower)->isdone(INPUT(r, f->follower), r->queries[f->follower]->result)) {
			ended = FALSE;
			break;
		}
	}

	return ended;
}

static gboolean rlib_navigate_next_n_to_1(rlib *r, gint resultset_num) {
	gboolean retval = FALSE;

	if (g_list_length(r->queries[resultset_num]->followers_n_to_1) == 0)
		return retval;

	if (!r->queries[resultset_num]->n_to_1_started) {
		GList *fw;

		/*
		 * If the n_to_1 followers were not started yet,
		 * advance all of them with 1 rows, because
		 * they are at the start, i.e. before the first row.
		 */
		for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
			struct rlib_resultset_followers *f = fw->data;
			rlib_navigate_next(r, f->follower);
			if (INPUT(r, f->follower)->isdone(INPUT(r, f->follower), r->queries[f->follower]->result))
				r->queries[f->follower]->n_to_1_empty = TRUE;
		}
	} else {
		GList *fw;

		/*
		 * If the n_to_1 followers were started in a previous round,
		 * advance only the first non-empty resultset.
		 */
		for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
			struct rlib_resultset_followers *f = fw->data;

			if (r->queries[f->follower]->n_to_1_empty)
				continue;

			rlib_navigate_next(r, f->follower);
			break;
		}
	}

	do {
		GList *fw, *fw1;

		if (rlib_navigate_n_to_1_check_ended(r, resultset_num))
			break;

		retval = rlib_navigate_n_to_1_check_current(r, resultset_num);
		if (retval)
			break;

		for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
			struct rlib_resultset_followers *f = fw->data;

			if (r->queries[f->follower]->n_to_1_empty)
				continue;

			if (INPUT(r, f->follower)->isdone(INPUT(r, f->follower), r->queries[f->follower]->result)) {
				for (fw1 = r->queries[resultset_num]->followers_n_to_1; fw1 && fw1 != fw; fw1 = fw1->next) {
					struct rlib_resultset_followers *f1 = fw1->data;
					rlib_navigate_start(r, f1->follower);
					rlib_navigate_next(r, f1->follower);
				}

				rlib_navigate_start(r, f->follower);
				rlib_navigate_next(r, f->follower);
				continue;
			}

			rlib_navigate_next(r, f->follower);
			break;
		}
	} while (1);

	return retval;
}

gboolean rlib_navigate_next(rlib *r, gint resultset_num) {
	GList *fw;
	gboolean retval, retval11;

	if (resultset_num < 0 || r->queries_count == 0)
		return FALSE;

	if (r->queries[resultset_num]->n_to_1_started) {
		retval = rlib_navigate_next_n_to_1(r, resultset_num);
		r->queries[resultset_num]->n_to_1_matched |= retval;

		if (retval)
			return TRUE;

		if (rlib_navigate_n_to_1_check_ended(r, resultset_num)) {
			r->queries[resultset_num]->n_to_1_started = FALSE;
			if (!r->queries[resultset_num]->n_to_1_matched)
				return TRUE;
		}
	}

	if (!r->queries[resultset_num]->n_to_1_started) {
		struct input_filter *in = INPUT(r, resultset_num);
		struct rlib_query_internal *q = r->queries[resultset_num];
		gint cols = in->num_fields(in, q->result);
		gint i;

		for (i = 1; i <= cols; i++) {
			gpointer col = GINT_TO_POINTER(i);
			gchar *str = in->get_field_value_as_string(in, q->result, col);
			struct rlib_value *rval = g_new0(struct rlib_value, 1);

			if (r->queries[resultset_num]->current_row >= 0)
				rlib_value_new_string(r, rval, (str ? str : ""));
			else
				rlib_value_new_none(r, rval);
			g_hash_table_replace(q->cached_values, col, rval);
		}

		r->queries[resultset_num]->current_row++;
		if (!INPUT(r, resultset_num)->next(INPUT(r, resultset_num), r->queries[resultset_num]->result))
			return FALSE;
	}

	/* 1:1 followers  */
	retval11 = FALSE;
	for (fw = r->queries[resultset_num]->followers; fw; fw = fw->next) {
		struct rlib_resultset_followers *f = fw->data;
		retval = rlib_navigate_next(r, f->follower);
		if (r->queries[f->follower]->followers_n_to_1)
			retval11 = (retval11 || retval);
	}

	/* n:1 followers */
	for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
		struct rlib_resultset_followers *f = fw->data;
		rlib_navigate_start(r, f->follower);
	}

	retval = rlib_navigate_next_n_to_1(r, resultset_num);
	r->queries[resultset_num]->n_to_1_matched |= (retval || retval11);
	r->queries[resultset_num]->n_to_1_started = (retval || retval11);

	return TRUE;
}

void rlib_navigate_start(rlib *r, gint resultset_num) {
	GList *fw;

	if (resultset_num < 0 || r->queries_count == 0)
		return;

	if (r->queries[resultset_num]->result == NULL) {
		r->queries[resultset_num]->navigation_failed = TRUE;
		return;
	}

	INPUT(r, resultset_num)->start(INPUT(r, resultset_num), r->queries[resultset_num]->result);
	r->queries[resultset_num]->current_row = -1;
	r->queries[resultset_num]->n_to_1_empty = FALSE;
	r->queries[resultset_num]->n_to_1_started = FALSE;
	r->queries[resultset_num]->n_to_1_matched = FALSE;

	for (fw = r->queries[resultset_num]->followers; fw; fw = fw->next) {
		struct rlib_resultset_followers *f = fw->data;
		rlib_navigate_start(r, f->follower);
	}

	for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
		struct rlib_resultset_followers *f = fw->data;
		rlib_navigate_start(r, f->follower);
	}
}
