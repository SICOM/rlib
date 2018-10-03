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
 */
 
#include <stdio.h>
#include <rlib.h>
#include <rlib_input.h>

int main(int argc, char **argv) {
	char *hostname, *username, *password, *database;
	char *query;
	rlib *r;

	if (argc != 5) {
		fprintf(stderr, "%s requires 4 arguments hostname username password database\n", argv[0]);
		fprintf(stderr, "You provided %d\n", argc-1);
		return -1;
	}

	hostname = argv[1];
	username = argv[2];
	password = argv[3];
	database = argv[4];

	query = "SELECT store_hierarchy.g0_name, store_hierarchy.g1_name, store_hierarchy.g2_name, store_hierarchy.g3_name, store_hierarchy.g4_name,"
			"stores.name, topline.date, topline.rn as stores_rn, "
			"sum(topline.foodsales_amount) as foodsales_amount, sum(topline.nonfood_amount) as nonfood_amount,  "
			"sum(topline.totaltax_amount) as totaltax_amount, sum(topline.giftsales_amount) as giftsales_amount, "
			"sum(topline.giftcert_amount) as giftcert_amount, "
			"sum(topline.totaldeposits_amount) as totaldeposits_amount, "
			"sum(topline.paidin_amount) as paidin_amount, "
			"sum(topline.paidout_amount) as paidout_amount "
			"FROM store_hierarchy, stores, topline "
			"WHERE topline.date between \"2003-01-01\" AND \"2003-01-07\""
			"AND topline.store = stores.rn "
			"AND store_hierarchy.store = stores.rn "
			"AND store_hierarchy.g0 = 1"
			"AND store_hierarchy.store in (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)"
			"GROUP BY topline.store"
			"ORDER BY g0,g1,g2,g3,g4, stores.name";

	r = rlib_init();
	rlib_add_datasource_mysql(r, "local_mysql", hostname, username, password, database);
	rlib_add_query_as(r, "local_mysql", query, "topline");
	rlib_add_query_as(r, "local_mysql", "select company.*, menu_items.name as report_name from company, menu_items where menu_items.id = 28", "header");
	rlib_add_report(r, "report_financial_summary.xml");
	rlib_set_output_format(r, RLIB_FORMAT_PDF);
	rlib_execute(r);
	rlib_spool(r);
	rlib_free(r);
	return 0;
}
