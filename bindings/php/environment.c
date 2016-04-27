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
 
#include <php.h>

#include "rlib.h"

static char * rlib_php_resolve_memory_variable(char *name) {
	void *temp;
	zval ** data;
	TSRMLS_FETCH();

	if (zend_hash_find(&EG(symbol_table),name,strlen(name)+1, (void *)&temp)==FAILURE) { 
		return NULL;
	} else {
		char *data_result, dstr[1024];
		memset(dstr, 0, 1024);
		data = temp;
		if( Z_TYPE_PP(data) == IS_STRING )	
			data_result = Z_STRVAL_PP(data);
		else if( Z_TYPE_PP(data) == IS_LONG ) {	
			sprintf(dstr,"%ld",Z_LVAL_PP(data));
			data_result = estrdup(dstr);
		} else if( Z_TYPE_PP(data) == IS_DOUBLE ) {	
			sprintf(dstr,"%f",Z_DVAL_PP(data));
			data_result = estrdup(dstr);
		} else if( Z_TYPE_PP(data) == IS_NULL ) {	
			data_result = estrdup("");
		} else {
			sprintf(dstr,"ZEND Z_TYPE %d NOT SUPPORTED",Z_TYPE_PP(data));
			data_result = estrdup(dstr);
		}	
		return data_result;
	}
}

static int rlib_php_write_output(char *data, int len) {
	long wrote = 0;
	TSRMLS_FETCH();
/*
	PHP Has some odd bug on the LO that doesn't allow you to write more then 15785 at a time.
*/
	while(wrote < len) {	
		int size = 4096;
		
		if(size+wrote > len)
			size = len-wrote;
		
		php_write(data+wrote, size TSRMLS_CC);
		wrote += size;
	}
	return TRUE;
}

void rlib_php_free(rlib *r) {
	efree(rlib_get_environment(r));
}


struct environment_filter * rlib_php_new_environment() {
	struct environment_filter *ef;

	ef = emalloc(sizeof(struct environment_filter));

	ef->rlib_resolve_memory_variable = rlib_php_resolve_memory_variable;
	ef->rlib_write_output = rlib_php_write_output;
	ef->free = rlib_php_free;
	return ef;
}
