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

#include <string.h>

#include "config.h"
#include "rlib-internal.h"
#include "pcode.h"
#include "rlib_input.h"

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

static gint rlib_navigate_followers(rlib *r, gint my_leader, gint way) {
	gint i, rtn = TRUE;
	gint found = FALSE;
	for(i=0;i<r->resultset_followers_count;i++) {
		found = FALSE;
		if(r->followers[i].leader == my_leader) {
			if(r->followers[i].leader_code != NULL ) {
				struct rlib_value rval_leader, rval_follower;
				rlib_execute_pcode(r, &rval_leader, r->followers[i].leader_code, NULL);
				rlib_execute_pcode(r, &rval_follower, r->followers[i].follower_code, NULL);
				if( rvalcmp(&rval_leader,&rval_follower) == 0 )  {

				} else {
					rlib_value_free(&rval_follower);
					if(rlib_do_followers(r, i, way) == TRUE) {
						rlib_execute_pcode(r, &rval_follower, r->followers[i].follower_code, NULL);
						if( rvalcmp(&rval_leader,&rval_follower) == 0 )  {
							found = TRUE;
							
						} 
					} 
					if(found == FALSE) {
						r->results[r->followers[i].follower]->navigation_failed = FALSE;
						rlib_do_followers(r, i, RLIB_NAVIGATE_FIRST);
						do {
							rlib_execute_pcode(r, &rval_follower, r->followers[i].follower_code, NULL);
							if(rvalcmp(&rval_leader,&rval_follower) == 0 ) {
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
}

gint rlib_navigate_next(rlib *r, gint resultset_num) {
	gint rtn;
	rtn = INPUT(r, resultset_num)->next(INPUT(r, resultset_num), r->results[resultset_num]->result);
	if(rtn == TRUE)
		return rlib_navigate_followers(r, resultset_num, RLIB_NAVIGATE_NEXT);
	else
		return FALSE;
}

gint rlib_navigate_previous(rlib *r, gint resultset_num) {
	gint rtn;
	rtn = INPUT(r, resultset_num)->previous(INPUT(r, resultset_num), r->results[resultset_num]->result);
	if(rtn == TRUE)
		return rlib_navigate_followers(r, resultset_num, RLIB_NAVIGATE_PREVIOUS);
	else
		return FALSE;
}

gint rlib_navigate_first(rlib *r, gint resultset_num) {
	gint rtn;
	
	if(r->results[resultset_num]->result == NULL) {
		r->results[resultset_num]->navigation_failed = TRUE;
		return FALSE;
	}

	rtn = INPUT(r, resultset_num)->first(INPUT(r, resultset_num), r->results[resultset_num]->result);
	if(rtn == TRUE)
		return rlib_navigate_followers(r, resultset_num, RLIB_NAVIGATE_FIRST);
	else
		return FALSE;
}

gint rlib_navigate_last(rlib *r, gint resultset_num) {
	gint rtn;
	rtn = INPUT(r, resultset_num)->last(INPUT(r, resultset_num), r->results[resultset_num]->result);
	if(rtn == TRUE)
		return rlib_navigate_followers(r, resultset_num, RLIB_NAVIGATE_LAST);
	else
		return FALSE;
}
