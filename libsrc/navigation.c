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

#if 0
static gint rlib_do_followers(rlib *r, gint i, gint way) {
	gint follower;
	gint rtn = TRUE;
	follower = r->followers[i].follower;

	if(r->results[follower]->navigation_failed == TRUE)
		return FALSE;

	if(r->results[follower]->next_failed)
		r->results[follower]->navigation_failed = TRUE;
		

	if(way == RLIB_NAVIGATE_NEXT) {
		if(rlib_navigate_next(r, follower) != TRUE) {
			if(rlib_navigate_last(r, follower) != TRUE) {
				rtn = FALSE;
			}
			r->results[follower]->next_failed = TRUE;
		}	
	} else if(way == RLIB_NAVIGATE_PREVIOUS) {
		if(rlib_navigate_previous(r, follower) != TRUE)
			rtn = FALSE;
	} else if(way == RLIB_NAVIGATE_FIRST) {
		if(rlib_navigate_first(r, follower) != TRUE)
			rtn = FALSE;
		else {
			r->results[follower]->next_failed = FALSE;
			r->results[follower]->navigation_failed = FALSE;
		
		}
	} else if(way == RLIB_NAVIGATE_LAST) {
		if(rlib_navigate_last(r, follower) != TRUE)
			rtn = FALSE;
	}
	return rtn;
}
#endif

static gint rlib_navigate_followers(rlib *r UNUSED, gint my_leader UNUSED, gint way UNUSED) {
#if 0
	gint i, rtn = TRUE;
	gint found = FALSE;
	for(i=0;i<r->resultset_followers_count;i++) {
		found = FALSE;
		if(r->followers[i].leader == my_leader) {
			if(r->followers[i].leader_code != NULL ) {
				struct rlib_value rval_leader, rval_follower;
				rlib_execute_pcode(r, &rval_leader, r->followers[i].leader_code, NULL);
				rlib_execute_pcode(r, &rval_follower, r->followers[i].follower_code, NULL);
				if(rvalcmp(r, &rval_leader,&rval_follower) == 0 )  {

				} else {
					rlib_value_free(&rval_follower);
					if(rlib_do_followers(r, i, way) == TRUE) {
						rlib_execute_pcode(r, &rval_follower, r->followers[i].follower_code, NULL);
						if(rvalcmp(r, &rval_leader,&rval_follower) == 0 )  {
							found = TRUE;
							
						} 
					} 
					if(found == FALSE) {
						r->results[r->followers[i].follower]->navigation_failed = FALSE;
						rlib_do_followers(r, i, RLIB_NAVIGATE_FIRST);
						do {
							rlib_execute_pcode(r, &rval_follower, r->followers[i].follower_code, NULL);
							if(rvalcmp(r, &rval_leader,&rval_follower) == 0 ) {
								found = TRUE;
								break;											
							}
							rlib_value_free(&rval_follower);
						} while(rlib_do_followers(r, i, RLIB_NAVIGATE_NEXT) == TRUE);
					}
					if(!found)  {
						r->results[r->followers[i].follower]->navigation_failed = TRUE;	
					}
				}

				rlib_value_free(&rval_leader);
				rlib_value_free(&rval_follower);
			} else {
				rtn = rlib_do_followers(r, i, way);
			}
		}
	}
	rlib_process_input_metadata(r);
	return rtn;
#else
	return TRUE;
#endif
}

static gint rlib_navigate_n_to_1_check_current(rlib *r, gint resultset_num) {
	GList *fw;

	if (INPUT(r, resultset_num)->isdone(INPUT(r, resultset_num), r->results[resultset_num]->result))
		return FALSE;

	for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
		struct rlib_resultset_followers *f = fw->data;
		struct rlib_value rval_leader, rval_follower;

		if (INPUT(r, f->follower)->isdone(INPUT(r, f->follower), r->results[f->follower]->result))
			return FALSE;

		rlib_execute_pcode(r, &rval_leader, f->leader_code, NULL);
		rlib_execute_pcode(r, &rval_follower, f->follower_code, NULL);

		if (rvalcmp(r, &rval_leader,&rval_follower) != 0) {
			rlib_value_free(&rval_leader);
			rlib_value_free(&rval_follower);
			return FALSE;
		}

		rlib_value_free(&rval_leader);
		rlib_value_free(&rval_follower);
	}

	return TRUE;
}

static gint rlib_navigate_n_to_1_check_ended(rlib *r, gint resultset_num) {
	GList *fw;
	gint ended = TRUE;

	for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
		struct rlib_resultset_followers *f = fw->data;

		if (!INPUT(r, f->follower)->isdone(INPUT(r, f->follower), r->results[f->follower]->result)) {
			ended = FALSE;
			break;
		}
	}

	return ended;
}

static gint rlib_navigate_next_n_to_1(rlib *r, gint resultset_num) {
	gint retval = FALSE;

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
			if (INPUT(r, f->follower)->isdone(INPUT(r, f->follower), r->results[f->follower]->result))
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

			if (INPUT(r, f->follower)->isdone(INPUT(r, f->follower), r->results[f->follower]->result)) {
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

gint rlib_navigate_next(rlib *r, gint resultset_num) {
	GList *fw;

	if (r->queries[resultset_num]->n_to_1_started) {
		int  retval;

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
		/*
		 * This is for tracking the leader -> follower rowcounts
		 * for rlib_navigate_previous().
		 */
		r->queries[resultset_num]->current_row++;

		if (!INPUT(r, resultset_num)->next(INPUT(r, resultset_num), r->results[resultset_num]->result))
			return FALSE;

		/* 1:1 followers  */
		for (fw = r->queries[resultset_num]->followers; fw; fw = fw->next) {
			struct rlib_resultset_followers *f = fw->data;
			rlib_navigate_next(r, f->follower);
		}

		for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
			struct rlib_resultset_followers *f = fw->data;
			rlib_navigate_start(r, f->follower);
		}

		r->queries[resultset_num]->n_to_1_started = rlib_navigate_next_n_to_1(r, resultset_num);
	}

	return TRUE;
}

gint rlib_navigate_previous(rlib *r, gint resultset_num) {
	gint rtn;

	rtn = INPUT(r, resultset_num)->previous(INPUT(r, resultset_num), r->results[resultset_num]->result);
	if(rtn == TRUE)
		return rlib_navigate_followers(r, resultset_num, RLIB_NAVIGATE_PREVIOUS);
	else
		return FALSE;
}

void rlib_navigate_start(rlib *r, gint resultset_num) {
	GList *fw;
	int i;

	if (r->results[resultset_num]->result == NULL) {
		r->results[resultset_num]->navigation_failed = TRUE;
		return;
	}

	INPUT(r, resultset_num)->start(INPUT(r, resultset_num), r->results[resultset_num]->result);
	r->queries[resultset_num]->current_row = -1;
	r->queries[resultset_num]->n_to_1_started = FALSE;
	r->queries[resultset_num]->n_to_1_matched = FALSE;

	for (i = 0, fw = r->queries[resultset_num]->followers; fw && i < 3; fw = fw->next, i++) {
		struct rlib_resultset_followers *f = fw->data;
		rlib_navigate_start(r, f->follower);
	}

	for (fw = r->queries[resultset_num]->followers_n_to_1; fw; fw = fw->next) {
		struct rlib_resultset_followers *f = fw->data;
		rlib_navigate_start(r, f->follower);
	}
}

gint rlib_navigate_last(rlib *r, gint resultset_num) {
	gint rtn;

	rtn = INPUT(r, resultset_num)->last(INPUT(r, resultset_num), r->results[resultset_num]->result);
	if(rtn == TRUE)
		return rlib_navigate_followers(r, resultset_num, RLIB_NAVIGATE_LAST);
	else
		return FALSE;
}
