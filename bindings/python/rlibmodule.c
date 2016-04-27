/*
 *  Copyright (C) 2003-2016 SICOM Systems, INC.
 *
 *  Authors: William K. Volkman
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
static char *rlib_interface_version="0.1.0";

#include "Python.h"

#include "rlib.h"
#include "rlib_input.h"
#include "pcode.h"

#define INPUT_PRIVATE(input) (((struct _private *)input->private))

static char rlibmodule__doc__[] = "\
RLIB is a report generation library/language.\n\
This is a python interface to the RLIB library.\n\
One of the main advantages to RLIB is that you don't have to be a programmer in order to use\n\
it. The file format for describing reports is XML. RLIB supports full expression evaluation in\n\
human readable format so it is easy to follow the logic behind a report.\n\
";

typedef struct _func_chain {
	struct _func_chain	*next;
	PyObject		*callable;
	int			param_count;
} func_chain;

typedef struct _signal_chain {
	struct _signal_chain	*next;
	PyObject		*callable;
	PyObject		*arg;
} signal_chain;

typedef struct {
	PyObject_HEAD
	rlib 		*rlib_ptr;
	func_chain	*funcs;
	signal_chain	*signals;
	char		*output;
} RLIBObject;

staticforward PyTypeObject RLIBType;

#define check_rlibobject_open(v) if ((v)->rlib_ptr == NULL) \
   { PyErr_SetString(RLIBError, "Rlib object has already been closed."); \
      return NULL; }

static PyObject *RLIBError;
static struct environment_filter *rlib_python_new_environment();

static char rlib_object__doc__[] = "\
This object represents a Rlib report instance.\n";

static PyObject *
newrlibobject()
{
	RLIBObject	*rp;
	
	rp = PyObject_New(RLIBObject, &RLIBType);
	if (rp == NULL)
		return NULL;
	rp->rlib_ptr = rlib_init_with_environment(rlib_python_new_environment());
	if (rp->rlib_ptr == NULL) {
		PyErr_SetString(RLIBError, "rlib_init failed");
		Py_DECREF(rp);
		return NULL;
	}
	rp->funcs = NULL;
	rp->signals = NULL;
	rp->output = NULL;
	return (PyObject *)rp;
}

static void
rlib_dealloc(register RLIBObject *rp)
{
	func_chain	*fp;
	signal_chain	*sp;

	if (rp->rlib_ptr) {
		rlib_free(rp->rlib_ptr);
		rp->rlib_ptr = NULL;
	}
	while ((fp = rp->funcs)) {
		Py_XDECREF(fp->callable);
		rp->funcs = fp->next;
		PyMem_Free(fp);
	}
	while ((sp = rp->signals)) {
		Py_XDECREF(sp->callable);
		Py_XDECREF(sp->arg);
		rp->signals = sp->next;
		PyMem_Free(sp);
	}
	if (rp->output) {
		free(rp->output);
		rp->output = NULL;
	}
	rp->ob_type->tp_free((PyObject*)rp);
}

static int
implement_function_call(rlib *rlib_ptr,  struct rlib_pcode *code, struct rlib_value_stack *vs, struct rlib_value *this_field_value, void *user_data) {
	PyObject 	*arglist;
	PyObject 	*retval;
	func_chain	*fp = user_data;
	int		i;
	struct rlib_value rval_rtn;

	arglist = PyTuple_New(fp->param_count);
	if (arglist == NULL)
		return 0;
	for (i=0; i< fp->param_count; i++) {
		PyObject *arg;
		struct rlib_value *v = rlib_value_stack_pop(vs);
		if (RLIB_VALUE_IS_STRING(v)) {
			arg = PyString_FromString(RLIB_VALUE_GET_AS_STRING(v));
		} else if (RLIB_VALUE_IS_NUMBER(v)) {
			arg = PyFloat_FromDouble((double)RLIB_VALUE_GET_AS_NUMBER(v) / (double)RLIB_DECIMAL_PRECISION);
		} else {
			Py_INCREF(Py_None);
			arg = Py_None;
		}
		rlib_value_free(v);
		if (PyTuple_SetItem(arglist, i, arg) < 0)
			return 0;
	}
	
	/* Time to call the function */
	retval = PyEval_CallObject(fp->callable, arglist);
	Py_DECREF(arglist);
	if (retval == NULL) {
		return 0;
	}
	/* Figure out what to do with result here */

	if (PyString_Check(retval))
		rlib_value_stack_push(rlib_ptr, vs, rlib_value_new_string(&rval_rtn, PyString_AsString(retval)));
	else if (PyInt_Check(retval)) {
		gint64 result = LONG_TO_FXP_NUMBER(PyInt_AsLong(retval));
		rlib_value_stack_push(rlib_ptr, vs, rlib_value_new_number(&rval_rtn, result));
	} else if (PyFloat_Check(retval)) {
		gint64 result = PyFloat_AsDouble(retval)*(gdouble)RLIB_DECIMAL_PRECISION;
                rlib_value_stack_push(rlib_ptr, vs, rlib_value_new_number(&rval_rtn, result));
        } else {
                rlib_value_stack_push(rlib_ptr, vs, rlib_value_new_error(&rval_rtn));
        }
	Py_DECREF(retval);
	return 1;
}


static int
implement_signal_call(rlib *rlib_ptr,  void *user_data) {
	signal_chain	*sp = user_data;
	PyObject	*retval;
	
	retval = PyEval_CallObject(sp->callable, sp->arg);
	Py_XDECREF(retval);
	return 1;
}

static gchar * rlib_python_resolve_memory_variable(gchar *name) {
	PyObject	*moduledict = PyImport_GetModuleDict();
	PyObject	*mainmodule;
	PyObject	*dict;
	PyObject	*gblvar;
	PyObject	*varstr;
	char		*result;

	mainmodule = PyDict_GetItemString(moduledict, "__main__");
	if (!PyModule_Check(mainmodule)) {
		PyErr_SetString(RLIBError, "could not find main module");
		return NULL;
	}
	dict = PyModule_GetDict(mainmodule);
	gblvar = PyDict_GetItemString(dict, name);
	if (gblvar == NULL) {
		char	errstring[255];
		snprintf(errstring, sizeof(errstring)-1, "variable %s does not exist", name);
		errstring[sizeof(errstring)-1] = '\0';
		PyErr_SetString(RLIBError, errstring);
		return NULL;
	}
	varstr = PyObject_Str(gblvar);
	if (varstr == NULL) {
		char	errstring[255];
		snprintf(errstring, sizeof(errstring)-1, "variable %s cannot be converted to a string", name);
		errstring[sizeof(errstring)-1] = '\0';
		PyErr_SetString(RLIBError, errstring);
		return NULL;
	}
	result = PyString_AsString(varstr);
	Py_DECREF(varstr);
        return g_strdup(result);
}

static gint rlib_python_write_output(gchar *data, gint len) {
        return write(1, data, len);
}

static void rlib_python_free(rlib *r) {
        g_free(ENVIRONMENT(r));
}

static struct environment_filter *rlib_python_new_environment() {
	struct environment_filter *ef;
        ef = g_malloc(sizeof(struct environment_filter));
        ef->rlib_resolve_memory_variable = rlib_python_resolve_memory_variable;
        ef->rlib_write_output = rlib_python_write_output;
        ef->free = rlib_python_free;
	return ef;
}

struct rlib_python_array_results {
        char 	*array_name;
        int	cols;
        int	rows;
        int	isdone;
        char	**data;
        int	current_row;
};

struct _private {
	PyObject	*localarray;
	struct rlib_python_array_results *lastresult;
};

static PyObject *
rlib_python_array_locate(char *name)
{
	PyObject	*moduledict = PyImport_GetModuleDict();
	PyObject	*mainmodule;
	PyObject	*dict;
	PyObject	*array;
	
	mainmodule = PyDict_GetItemString(moduledict, "__main__");
	if (!PyModule_Check(mainmodule)) {
		PyErr_SetString(RLIBError, "could not find main module");
		return NULL;
	}
	dict = PyModule_GetDict(mainmodule);
	array = PyDict_GetItemString(dict, name);
	if (array == NULL) {
		char	errstring[255];
		snprintf(errstring, sizeof(errstring)-1, "array %s does not exist", name);
		errstring[sizeof(errstring)-1] = '\0';
		PyErr_SetString(RLIBError, errstring);
		return NULL;
	}
	return PySequence_Fast(array, "array is of incorrect type");
}

void * rlib_python_array_new_result_from_query(gpointer input_ptr, gchar *query) {
	struct input_filter *input = input_ptr;
        struct rlib_python_array_results *result;
	PyObject	*outerlist;
	PyObject	*innerlist;
	PyObject	*element;
        int 		row=0, col=0;
        int 		total_size;
        char 		*data_result;
        char		dstr[64];

	memset(dstr, 0, sizeof(dstr));

	result = PyMem_Malloc(sizeof(struct rlib_python_array_results));
	if (result == NULL)
		return PyErr_NoMemory();
        memset(result, 0, sizeof(struct rlib_python_array_results));
        result->array_name = query;

	outerlist = rlib_python_array_locate(query);
	if (outerlist == NULL) {
		PyMem_Free(result);
		return NULL;
	}

	Py_XDECREF(INPUT_PRIVATE(input)->localarray);
	INPUT_PRIVATE(input)->localarray = outerlist;
        result->rows = PySequence_Fast_GET_SIZE(outerlist);
	
	innerlist = PySequence_Fast_GET_ITEM(outerlist, 0);
	if (!PyList_Check(innerlist) && !PyTuple_Check(innerlist)) {
		Py_DECREF(outerlist);
		INPUT_PRIVATE(input)->localarray = NULL;
		PyMem_Free(result);
		PyErr_SetString(RLIBError, "sub-elements of array must be of type list or tuple"); /* Fixme: use type error exception */
		return NULL;
	}
        result->cols = PySequence_Fast_GET_SIZE(innerlist);

        total_size = result->rows * result->cols * sizeof(char *);
        result->data = PyMem_Malloc(total_size);
	memset(result->data, 0, total_size);
        for (row=0; row < result->rows; row++) {
		innerlist = PySequence_Fast(PySequence_Fast_GET_ITEM(outerlist, row), "sub-elements of array must be of type list or tuple");
		if (innerlist == NULL) {
			Py_DECREF(outerlist);
			INPUT_PRIVATE(input)->localarray = NULL;
			PyMem_Free(result->data);
			PyMem_Free(result);
			return NULL;
		}
                for (col=0; col < result->cols; col++) {
			element = PySequence_Fast_GET_ITEM(innerlist, col);
			if (element == NULL) {
				Py_DECREF(innerlist);
				Py_DECREF(outerlist);
				INPUT_PRIVATE(input)->localarray = NULL;
				PyMem_Free(result->data);
				PyMem_Free(result);
				PyErr_SetString(RLIBError, "array dimensions must be uniform"); /* Fixme: use type error exception */
				return NULL;
			}
                        if( PyString_Check(element) )
                                data_result = strdup(PyString_AsString(element));
                        else if( PyInt_Check(element) ) {
                                sprintf(dstr,"%ld",PyInt_AsLong(element));
                                data_result = strdup(dstr);
                        } else if( PyFloat_Check(element) ) {
                                sprintf(dstr,"%f",PyFloat_AsDouble(element));
                                data_result = strdup(dstr);
                        } else if( element == Py_None ) {
                                data_result = strdup("");
                        } else {
				PyObject	*tp = PyObject_Type(element);
				PyObject	*rp;
				rp = PyObject_Repr(tp);
                                sprintf(dstr,"TYPE %s NOT SUPPORTED",PyString_AsString(rp));
				Py_XDECREF(rp);
				Py_XDECREF(tp);
                                data_result = strdup(dstr);
                        }
                        result->data[(row * result->cols)+col] = data_result;
                }
		Py_DECREF(innerlist);
        }
        return result;
}



static gint rlib_python_array_input_close(gpointer input_ptr) {
        return TRUE;
}
static gint rlib_python_array_first(gpointer input_ptr, gpointer result_ptr) {
        struct rlib_python_array_results *result = result_ptr;
        result->current_row = 1;
        if(result->rows <= 1) {
                result->isdone = TRUE;
                return FALSE;
        }
        result->isdone = FALSE;
        return TRUE;
}

static gint rlib_python_array_next(gpointer input_ptr, gpointer result_ptr) {
        struct rlib_python_array_results *result = result_ptr;
	result->current_row++;
	result->isdone = FALSE;
	if(result->current_row < result->rows)
		return TRUE;
	result->isdone = TRUE;
	result->current_row = result->rows - 1;
	return FALSE;
}

static gint rlib_python_array_isdone(gpointer input_ptr, gpointer result_ptr) {
        struct rlib_python_array_results *result = result_ptr;
        return result->isdone;
}

static gint rlib_python_array_previous(gpointer input_ptr, gpointer result_ptr) {
	struct rlib_python_array_results *result = result_ptr;
	result->current_row--;
	result->isdone = FALSE;
	if(result->current_row >= 1)
		return TRUE;
	result->current_row = 0;
	return FALSE;
}
static gint rlib_python_array_last(gpointer input_ptr, gpointer result_ptr) {
        struct rlib_python_array_results *result = result_ptr;
        result->current_row = result->rows-1;
        return TRUE;
}

static gchar * rlib_python_array_get_field_value_as_string(gpointer input_ptr, gpointer result_ptr, gpointer field_ptr) {
        struct rlib_python_array_results *result = result_ptr;
        int which_field = GPOINTER_TO_INT(field_ptr) - 1;
        if(result->rows <= 1)
                return "";

        return result->data[(result->current_row*result->cols)+which_field];
}

static gpointer rlib_python_array_resolve_field_pointer(gpointer input_ptr, gpointer result_ptr, gchar *name) {
        struct rlib_python_array_results *result = result_ptr;
        int i;
        for(i=0;i<result->cols;i++) {
                if(strcmp(name, result->data[i]) == 0) {
                        i++;
                        return GINT_TO_POINTER(i);
                }
        }
        return NULL;
}

static gint rlib_python_array_free_input_filter(gpointer input_ptr) {
	struct input_filter *input = input_ptr;
	if (input->private) {
		PyMem_Free(input->private);
		input->private = NULL;
	}
	PyMem_Free(input);
        return 0;
}

static void rlib_python_array_free_result(gpointer input_ptr, gpointer result_ptr) {
	struct input_filter *input = input_ptr;
        struct rlib_python_array_results *result = result_ptr;
	int	i, j;
	if (result) {
	    if (result->data)
		    for (i=0;i<result->rows;i++)
			    for (j=0; j<result->cols; j++)
				    free(result->data[(i*result->cols)+j]);
	    PyMem_Free(result->data);
	    result->data = NULL;
	    PyMem_Free(result);
	}
	Py_XDECREF(INPUT_PRIVATE(input)->localarray);
	INPUT_PRIVATE(input)->localarray = NULL;
}


gpointer rlib_python_array_new_input_filter() {
        struct input_filter *input;
        input = PyMem_Malloc(sizeof(struct input_filter));
        input->private = PyMem_Malloc(sizeof(struct _private));
        memset(input->private, 0, sizeof(struct _private));
        input->input_close = rlib_python_array_input_close;
        input->first = rlib_python_array_first;
        input->next = rlib_python_array_next;
        input->previous = rlib_python_array_previous;
        input->last = rlib_python_array_last;
        input->isdone = rlib_python_array_isdone;
        input->new_result_from_query = rlib_python_array_new_result_from_query;
        input->get_field_value_as_string = rlib_python_array_get_field_value_as_string;

        input->resolve_field_pointer = rlib_python_array_resolve_field_pointer;

        input->free = rlib_python_array_free_input_filter;
        input->free_result = rlib_python_array_free_result;
        return input;
}

/*
** --------------------------------------------------------------------------------
** Methods Start here
** --------------------------------------------------------------------------------
*/
static char method_add_datasource_array__doc__[] = "add_datasource_array(datasource_name) -> None";
static PyObject *
method_add_datasource_array(PyObject *self, PyObject *_args) {
	RLIBObject 	*rp = (RLIBObject *)self;
	char		*datasource;

	if (!PyArg_ParseTuple(_args, "s:add_datasource_array", &datasource))
		return NULL;
	check_rlibobject_open(rp);
	(void) rlib_add_datasource(rp->rlib_ptr, datasource, rlib_python_array_new_input_filter());
	Py_INCREF(Py_None);
	return Py_None;
}

static char method_add_datasource_mysql__doc__[] = "add_datasource_mysql(datasource_name, hostname, username, password, database) -> int";
static PyObject *
method_add_datasource_mysql(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char 	*datasource, *hostname, *username, *password, *database;
	int	result;
	
	if(!PyArg_ParseTuple(_args, "sssss:add_datasource_mysql",&datasource,&hostname,&username,&password,&database))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_add_datasource_mysql(rp->rlib_ptr,datasource,hostname,username,password,database);
	return PyInt_FromLong((long)result);
}


static char method_add_datasource_odbc__doc__[] = "add_datasource_odbc(datasource_name,source,username,password) -> int";
static PyObject *
method_add_datasource_odbc(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char 	*datasource, *source, *username, *password;
	int	result;
	
	if(!PyArg_ParseTuple(_args, "ssss:add_datasource_odbc",&datasource,&source,&username,&password))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_add_datasource_odbc(rp->rlib_ptr,datasource,source,username,password);
	return PyInt_FromLong((long)result);
}


static char method_add_datasource_postgres__doc__[] = "add_datasource_postgres(datasource_name, connection_string) -> int";
static PyObject *
method_add_datasource_postgres(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char 	*datasource, *connstr;
	long	result;
	
	if(!PyArg_ParseTuple(_args, "ss:add_datasource_postgres",&datasource,&connstr))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_add_datasource_postgres(rp->rlib_ptr,datasource,connstr);
	return PyInt_FromLong(result);
}


static char method_add_datasource_xml__doc__[] = "add_datasource_xml(datasource_name) -> int";
static PyObject *
method_add_datasource_xml(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*datasource;
	long		result;
	
	if (!PyArg_ParseTuple(_args, "s:add_datasource_xml", &datasource))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_add_datasource_xml(rp->rlib_ptr, datasource);
	return PyInt_FromLong(result);
}

static char method_add_datasource_csv__doc__[] = "add_datasource_csv(datasource_name) -> int";
static PyObject *
method_add_datasource_csv(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*datasource;
	long		result;
	
	if (!PyArg_ParseTuple(_args, "s:add_datasource_csv", &datasource))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_add_datasource_csv(rp->rlib_ptr, datasource);
	return PyInt_FromLong(result);
}


static char method_add_function__doc__[] = "add_function(name, function, param_count) -> None\n\
name - The name of the function\n\
function - the function reference\n\
param_count - The number of parameters the function expects\n\
Note: the types will be evaluated at run time, either string or double.\n\
";
static PyObject *
method_add_function(PyObject *self, PyObject *_args) {
	RLIBObject 	*rp = (RLIBObject *)self;
	char		*name;
	int		param_count;
	func_chain	*nfp;
	
	PyObject *callable;
	if (!PyArg_ParseTuple(_args, "sOi:add_function", &name, &callable, &param_count))
		return NULL;
        if (!PyCallable_Check(callable)) {
            PyErr_SetString(PyExc_TypeError, "the second parameter must be callable");
            return NULL;
        }
	check_rlibobject_open(rp);
	nfp = PyMem_Malloc(sizeof(func_chain));
	if (nfp == NULL)
		return PyErr_NoMemory();
	nfp->callable = callable;
        Py_INCREF(callable);         /* Add a reference to callback */
	nfp->param_count = param_count;
	nfp->next = rp->funcs;
	rp->funcs = nfp;
	rlib_add_function(rp->rlib_ptr, name, implement_function_call, nfp);
	Py_INCREF(Py_None);
	return Py_None;
}

static char method_add_parameter__doc__[] = "add_parameter(name, value) -> None";
static PyObject *
method_add_parameter(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char	*pname, *pvalue;
	
	if (!PyArg_ParseTuple(_args, "ss:add_parameter", &pname, &pvalue))
		return NULL;
	check_rlibobject_open(rp);
	(void)rlib_add_parameter(rp->rlib_ptr, pname, pvalue);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_add_query_as__doc__[] = "add_query_as(datasource_name, query, name) -> query_count";
static PyObject *
method_add_query_as(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char	*src, *query, *name;
	long	result;

	if (!PyArg_ParseTuple(_args, "sss:add_query_as", &src, &query, &name))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_add_query_as(rp->rlib_ptr, src, query, name);
	return PyInt_FromLong(result);
}


static char method_add_report__doc__[] = "add_report(report_file_name) -> report_count";
static PyObject *
method_add_report(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*name;
	long		result;

	if (!PyArg_ParseTuple(_args, "s:add_report", &name))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_add_report(rp->rlib_ptr, name);
	return PyInt_FromLong(result);
}


static char method_add_report_from_buffer__doc__[] = "add_report_from_buffer(report_buffer) -> report_count";
static PyObject *
method_add_report_from_buffer(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*buffer;
	long		result;

	if (!PyArg_ParseTuple(_args, "s:add_report_from_buffer", &buffer))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_add_report_from_buffer(rp->rlib_ptr, buffer);
	return PyInt_FromLong(result);
}


static char method_add_resultset_follower__doc__[] = "add_resultset_follower(leader, follower) -> None";
static PyObject *
method_add_resultset_follower(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*leader, *follower;
	long		result;

	if (!PyArg_ParseTuple(_args, "ss:add_resultset_follower", &leader, &follower))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_add_resultset_follower_n_to_1(rp->rlib_ptr, leader, NULL, follower, NULL);
	if (result < 0) {
		PyErr_SetString(PyExc_RuntimeError,"rlib error: couldn't find leader, follower, or you have a leader following a follower");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_add_resultset_follower_n_to_1__doc__[] = "add_resultset_follower_n_to_1(leader, leader_field, follower, follower_field) -> None";
static PyObject *
method_add_resultset_follower_n_to_1(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*leader, *leader_field, *follower, *follower_field;
	long		result;

	if (!PyArg_ParseTuple(_args, "ssss:add_resultset_follower", &leader, &leader_field, &follower, &follower_field))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_add_resultset_follower_n_to_1(rp->rlib_ptr, leader, leader_field, follower, follower_field);
	if (result < 0) {
		PyErr_SetString(PyExc_RuntimeError,"rlib error: couldn't find leader, follower, or you have a leader following a follower");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_execute__doc__[] = "execute() -> None";
static PyObject *
method_execute(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	long		result;

	check_rlibobject_open(rp);
	result = rlib_execute(rp->rlib_ptr);
	if (result < 0) {
		PyErr_SetString(PyExc_RuntimeError,"rlib error: report execution failed");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_get_content_type_as_text__doc__[] = "get_content_type_as_text() -> string";
static PyObject *
method_get_content_type_as_text(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;

	if (!PyArg_ParseTuple(_args, ":get_content_type_as_text"))
		return NULL;
	check_rlibobject_open(rp);
	return PyString_FromString(rlib_get_content_type_as_text(rp->rlib_ptr));
}


static char method_get_output__doc__[] = "get_output() -> string";
static PyObject *
method_get_output(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	long		size;

	if (!PyArg_ParseTuple(_args, ":get_output"))
		return NULL;
	check_rlibobject_open(rp);
	if (rp->output)
		free(rp->output);
	size = rlib_get_output_length(rp->rlib_ptr);
	rp->output = malloc(size+1);
	rp->output[size] = 0;
	memcpy(rp->output, rlib_get_output(rp->rlib_ptr), size);
	return (size > 0) ? PyString_FromStringAndSize(rp->output, size): PyString_FromString("");
}


static char method_get_output_length__doc__[] = "get_output_length() -> int";
static PyObject *
method_get_output_length(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	if (!PyArg_ParseTuple(_args, ":get_output_length"))
		return NULL;
	check_rlibobject_open(rp);
	return PyInt_FromLong(rlib_get_output_length(rp->rlib_ptr));
}


static char method_graph_add_bg_region__doc__[] = "graph_add_bg_region(graph_name, region_label, color, start, end) -> None";
static PyObject *
method_graph_add_bg_region(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*name, *label, *color;
	double		start, end;
	
	if (!PyArg_ParseTuple(_args, "sssff:graph_add_bg_region", &name, &label, &color, &start, &end))
		return NULL;
	check_rlibobject_open(rp);
	(void)rlib_graph_add_bg_region(rp->rlib_ptr, name, label, color, start, end);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_graph_clear_bg_region__doc__[] = "graph_clear_bg_region(graph_name) -> None";
static PyObject *
method_graph_clear_bg_region(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*name;

	if (!PyArg_ParseTuple(_args, "s:graph_clear_bg_region", &name))
		return NULL;
	check_rlibobject_open(rp);
	(void)rlib_graph_clear_bg_region(rp->rlib_ptr, name);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_graph_set_x_minor_tick__doc__[] = "graph_set_x_minor_tick(graph_name, x_tick_string) -> None";
static PyObject *
method_graph_set_x_minor_tick(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*name, *tick;

	if (!PyArg_ParseTuple(_args, "ss:graph_set_x_minor_tick", &name, &tick))
		return NULL;
	check_rlibobject_open(rp);
	(void)rlib_graph_set_x_minor_tick(rp->rlib_ptr, name, tick);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_graph_set_x_minor_tick_by_location__doc__[] = "graph_set_x_minor_tick_by_location(graph_name, x_location_int) -> None";
static PyObject *
method_graph_set_x_minor_tick_by_location(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*name;
	long		location;

	if (!PyArg_ParseTuple(_args, "si:graph_set_x_minor_tick_by_location", &name, &location))
		return NULL;
	check_rlibobject_open(rp);
	(void)rlib_graph_set_x_minor_tick_by_location(rp->rlib_ptr, name, location);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_query_refresh__doc__[] = "query_refresh() -> None";
static PyObject *
method_query_refresh(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;

	check_rlibobject_open(rp);
	(void)rlib_query_refresh(rp->rlib_ptr);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_set_datasource_encoding__doc__[] = "set_datasource_encoding(datasource_name, encoding) -> None";
static PyObject *
method_set_datasource_encoding(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*name, *encoding;
	long		result;

	if (!PyArg_ParseTuple(_args, "ss:set_datasource_encoding", &name, &encoding))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_set_datasource_encoding(rp->rlib_ptr, name, encoding);
	if (result < 0) {
		PyErr_SetString(RLIBError, "invalid datasource name");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_set_locale__doc__[] = "set_locale(locale) -> None";
static PyObject *
method_set_locale(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*locale;
	long		result;

	if (!PyArg_ParseTuple(_args, "s:set_locale", &locale))
		return NULL;
	check_rlibobject_open(rp);
	result = rlib_set_locale(rp->rlib_ptr, locale);
	if (!result) {
		PyErr_SetString(RLIBError, "Locale not changed");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_set_output_encoding__doc__[] = "set_output_encoding(encoding) -> None";
static PyObject *
method_set_output_encoding(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*encoding;

	if (!PyArg_ParseTuple(_args, "s:set_output_encoding", &encoding))
		return NULL;
	check_rlibobject_open(rp);
	rlib_set_output_encoding(rp->rlib_ptr, encoding);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_set_output_format__doc__[] = "set_output_format(format) -> None";
static PyObject *
method_set_output_format(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	long		format;

	if (!PyArg_ParseTuple(_args, "i:set_output_format", &format))
		return NULL;
	check_rlibobject_open(rp);
	(void)rlib_set_output_format(rp->rlib_ptr, format);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_set_output_format_from_text__doc__[] = "set_output_format_from_text(formatstring) -> None\n\
One of: pdf, html, txt, csv, or xml\n\
";
static PyObject *
method_set_output_format_from_text(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*formatstring;

	if (!PyArg_ParseTuple(_args, "s:set_output_format_from_text", &formatstring))
		return NULL;
	check_rlibobject_open(rp);
	(void)rlib_set_output_format_from_text(rp->rlib_ptr, formatstring);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_set_output_parameter__doc__[] = "set_output_parameter(name, value) -> None";
static PyObject *
method_set_output_parameter(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char	*pname, *pvalue;

	if (!PyArg_ParseTuple(_args, "ss:set_output_parameter", &pname, &pvalue))
		return NULL;
	check_rlibobject_open(rp);
	(void)rlib_set_output_parameter(rp->rlib_ptr, pname, pvalue);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_signal_connect__doc__[] = "signal_connect(signal_num, function, (arg,)) -> None";
static PyObject *
method_signal_connect(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	long		signal_num;
	PyObject	*callable;
	PyObject	*callback_arg = NULL;
	signal_chain	*nsp;
	
	if (!PyArg_ParseTuple(_args, "iO|O:signal_connect", &signal_num, &callable, &callback_arg))
		return NULL;
        if (!PyCallable_Check(callable)) {
            PyErr_SetString(PyExc_TypeError, "the second parameter must be callable");
            return NULL;
        }
	check_rlibobject_open(rp);
	nsp = PyMem_Malloc(sizeof(signal_chain));
	if (nsp == NULL)
		return PyErr_NoMemory();
	nsp->callable = callable;
        Py_INCREF(callable);         	/* Add a reference to callback */
	if (callback_arg == NULL) {
		callback_arg = PyTuple_New(0);
		nsp->arg = callback_arg;
	} else if (PyTuple_Check(callback_arg)) {
		nsp->arg = callback_arg;
		Py_INCREF(callback_arg); 	/* Add reference to argument */
        } else {
		nsp->arg = PyTuple_New(1);
		Py_INCREF(callback_arg); 	/* Add reference to argument */
		PyTuple_SetItem(nsp->arg, 0, callback_arg); /* Then give it to the Tuple */
	}
	nsp->next = rp->signals;
	rp->signals = nsp;
	(void)rlib_signal_connect(rp->rlib_ptr, signal_num, implement_signal_call, nsp);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_signal_connect_string__doc__[] = "signal_connect_string(signal_name, function, (arg,)) -> None\n\
One of: row_change, report_done, report_start, report_iteration, part_iteration\n\
";
static PyObject *
method_signal_connect_string(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	char		*signal_name;
	PyObject	*callable;
	PyObject	*callback_arg = NULL;
	signal_chain	*nsp;
	
	if (!PyArg_ParseTuple(_args, "sO|O:signal_connect_string", &signal_name, &callable, &callback_arg))
		return NULL;
        if (!PyCallable_Check(callable)) {
            PyErr_SetString(PyExc_TypeError, "the second parameter must be callable");
            return NULL;
        }
	check_rlibobject_open(rp);
	nsp = PyMem_Malloc(sizeof(signal_chain));
	if (nsp == NULL)
		return PyErr_NoMemory();
	nsp->callable = callable;
        Py_INCREF(callable);         	/* Add a reference to callback */
	if (callback_arg == NULL) {
		callback_arg = PyTuple_New(0);
		nsp->arg = callback_arg;
	} else if (PyTuple_Check(callback_arg)) {
		nsp->arg = callback_arg;
		Py_INCREF(callback_arg); 	/* Add reference to argument */
        } else {
		nsp->arg = PyTuple_New(1);
		Py_INCREF(callback_arg); 	/* Add reference to argument */
		PyTuple_SetItem(nsp->arg, 0, callback_arg); /* The give it to the Tuple */
	}
	nsp->next = rp->signals;
	rp->signals = nsp;
	(void)rlib_signal_connect_string(rp->rlib_ptr, signal_name, implement_signal_call, nsp);
	Py_INCREF(Py_None);
	return Py_None;
}


static char method_spool__doc__[] = "spool() -> None\n\
Send the report output to stdout, useful for CGI scripts.";
static PyObject *
method_spool(PyObject *self, PyObject *_args) {
	RLIBObject	*rp = (RLIBObject *)self;
	long		result;

	check_rlibobject_open(rp);
	result = rlib_spool(rp->rlib_ptr);
	if (result < 0) {
		PyErr_SetString(PyExc_RuntimeError,"rlib error: report spool failed");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef rlib_methods[] = {
	{"add_datasource_array", 	method_add_datasource_array,		METH_VARARGS, method_add_datasource_array__doc__},
	{"add_datasource_mysql", 	method_add_datasource_mysql,		METH_VARARGS, method_add_datasource_mysql__doc__},
	{"add_datasource_odbc", 	method_add_datasource_odbc,		METH_VARARGS, method_add_datasource_odbc__doc__},
	{"add_datasource_postgres", 	method_add_datasource_postgres,		METH_VARARGS, method_add_datasource_postgres__doc__},
	{"add_datasource_xml", 		method_add_datasource_xml,		METH_VARARGS, method_add_datasource_xml__doc__},
	{"add_datasource_csv", 		method_add_datasource_csv,		METH_VARARGS, method_add_datasource_csv__doc__},
	{"add_function", 		method_add_function,			METH_VARARGS, method_add_function__doc__},
	{"add_parameter", 		method_add_parameter,			METH_VARARGS, method_add_parameter__doc__},
	{"add_query_as", 		method_add_query_as,			METH_VARARGS, method_add_query_as__doc__},
	{"add_report", 			method_add_report,			METH_VARARGS, method_add_report__doc__},
	{"add_report_from_buffer", 	method_add_report_from_buffer,		METH_VARARGS, method_add_report_from_buffer__doc__},
	{"add_resultset_follower", 	method_add_resultset_follower,		METH_VARARGS, method_add_resultset_follower__doc__},
	{"add_resultset_follower_n_to_1", method_add_resultset_follower_n_to_1,	METH_VARARGS, method_add_resultset_follower_n_to_1__doc__},
	{"execute", 			method_execute,				METH_NOARGS,  method_execute__doc__},
	{"get_content_type_as_text", 	method_get_content_type_as_text,	METH_VARARGS, method_get_content_type_as_text__doc__},
	{"get_output", 			method_get_output,			METH_VARARGS, method_get_output__doc__},
	{"get_output_length", 		method_get_output_length,		METH_VARARGS, method_get_output_length__doc__},
	{"graph_add_bg_region",		method_graph_add_bg_region,		METH_VARARGS, method_graph_add_bg_region__doc__},
	{"graph_clear_bg_region", 	method_graph_clear_bg_region,		METH_VARARGS, method_graph_clear_bg_region__doc__},
	{"graph_set_x_minor_tick", 	method_graph_set_x_minor_tick,		METH_VARARGS, method_graph_set_x_minor_tick__doc__},
	{"graph_set_x_minor_tick_by_location", method_graph_set_x_minor_tick_by_location, METH_VARARGS, method_graph_set_x_minor_tick_by_location__doc__},
	{"query_refresh", 		method_query_refresh,			METH_NOARGS,  method_query_refresh__doc__},
	{"set_datasource_encoding", 	method_set_datasource_encoding,		METH_VARARGS, method_set_datasource_encoding__doc__},
	{"set_locale", 			method_set_locale,			METH_VARARGS, method_set_locale__doc__},
	{"set_output_encoding", 	method_set_output_encoding,		METH_VARARGS, method_set_output_encoding__doc__},
	{"set_output_format", 		method_set_output_format,		METH_VARARGS, method_set_output_format__doc__},
	{"set_output_format_from_text", method_set_output_format_from_text,	METH_VARARGS, method_set_output_format_from_text__doc__},
	{"set_output_parameter", 	method_set_output_parameter,		METH_VARARGS, method_set_output_parameter__doc__},
	{"signal_connect", 		method_signal_connect,			METH_VARARGS, method_signal_connect__doc__},
	{"signal_connect_string", 	method_signal_connect_string,		METH_VARARGS, method_signal_connect_string__doc__},
	{"spool", 			method_spool,				METH_NOARGS,  method_spool__doc__},
	{NULL, NULL}	/* sentinel */
};

static PyObject *
rlib_getattr(RLIBObject *rp, char *name)
{
	return Py_FindMethod(rlib_methods, (PyObject *)rp, name);
}

static PyTypeObject RLIBType = {
	PyObject_HEAD_INIT(NULL) 0,
	"rlib.Rlib",
	sizeof(RLIBObject),
	0,
	(destructor)rlib_dealloc,		/* tp_dealloc*/
	0,					/* tp_print*/
	(getattrfunc)rlib_getattr,		/* tp_getattr*/
	0,					/* tp_setattr*/
	0,					/* tp_compare*/
	0,					/* tp_repr*/
	0,					/* tp_as_number*/
	0,					/* tp_as_sequence*/
	0,					/* tp_as_mapping*/
	0,					/* tp_hash*/
	0,					/* tp_call*/
	0,					/* tp_str*/
	0,					/* tp_getattro*/
	0,					/* tp_setattro*/
	0,					/* tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,			/* tp_flags*/
	rlib_object__doc__,			/* tp_doc*/
	0,		               		/* tp_traverse */
	0,		               		/* tp_clear */
	0,		               		/* tp_richcompare */
	0,		               		/* tp_weaklistoffset */
	0,		               		/* tp_iter */
	0,		               		/* tp_iternext */
	rlib_methods,		             	/* tp_methods */
	0,             				/* tp_members */
	0,                         		/* tp_getset */
	0,                         		/* tp_base */
	0,                         		/* tp_dict */
	0,                         		/* tp_descr_get */
	0,                         		/* tp_descr_set */
	0,                         		/* tp_dictoffset */
	(initproc)0,				/* tp_init */
	0,                         		/* tp_alloc */
	newrlibobject,	                 		/* tp_new */
};

/* ----------------------------------------------------------------- */

static char rlibmysql_report__doc__[] = "\
mysql_report(hostname, username, password, database, xmlfilename, sqlquery, outputformat) -> 0\n\
Generate a mysql report and send it to standard out.\n\
";
static PyObject *
rlibmysql_report(PyObject *self, PyObject *_args)
{
	char	*hostname, *username, *password, *database, *xmlfile, *sqlquery, *oformat;
	long	result;

	if (!PyArg_ParseTuple(_args, "sssssss:mysql_report", &hostname, &username, &password, &database, &xmlfile, &sqlquery, &oformat))
		return NULL;
	result = rlib_mysql_report(hostname, username, password, database, xmlfile, sqlquery, oformat);
	return PyInt_FromLong(result);
}

static char rlibopen__doc__[] = "\
open()  -> rlib_object\n\
Create an instance of a Rlib report object.\n\
";

static PyObject *
rlibopen(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":open"))
        return NULL;
    return newrlibobject();
}

static char rlibpostgres_report__doc__[] = "\
postgres_report(connectionstring, xmlfilename, sqlquery, outputformat) -> 0\n\
Generate a PostgreSQL report and send it to standard out.\n\
";
static PyObject *
rlibpostgres_report(PyObject *self, PyObject *_args)
{
	char	*connstr, *xmlfile, *sqlquery, *oformat;
	long	result;

	if (!PyArg_ParseTuple(_args, "ssss:postgres_report", &connstr, &xmlfile, &sqlquery, &oformat))
		return NULL;
	result = rlib_postgres_report(connstr, xmlfile, sqlquery, oformat);
	return PyInt_FromLong(result);
}

static PyMethodDef rlibmodule_methods[] = {
    { "mysql_report",	(PyCFunction)rlibmysql_report,	 	METH_VARARGS, rlibmysql_report__doc__},
    { "open",		(PyCFunction)rlibopen,	 		METH_VARARGS, rlibopen__doc__},
    { "postgres_report",(PyCFunction)rlibpostgres_report,	METH_VARARGS, rlibpostgres_report__doc__},
    { NULL, NULL, 0, NULL },
};
//DL_EXPORT(void)
PyMODINIT_FUNC 
initrlib(void) {
    PyObject *m, *d, *s, *i;

    m = Py_InitModule3("rlib",
		       rlibmodule_methods,
                       rlibmodule__doc__);
    d = PyModule_GetDict(m);
    RLIBError = PyErr_NewException("rlib.Error", NULL, NULL);
    if (RLIBError != NULL) {
        PyDict_SetItemString(d, "Error", RLIBError);

	s = PyString_FromString(rlib_version());
	PyDict_SetItemString(d, "version", s);
	Py_DECREF(s);

	s = PyString_FromString(rlib_interface_version);
	PyDict_SetItemString(d, "__version__", s);
	Py_DECREF(s);

	i = PyInt_FromLong(RLIB_FORMAT_PDF);
	PyDict_SetItemString(d, "FORMAT_PDF", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_FORMAT_HTML);
	PyDict_SetItemString(d, "FORMAT_HTML", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_FORMAT_TXT);
	PyDict_SetItemString(d, "FORMAT_TXT", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_FORMAT_CSV);
	PyDict_SetItemString(d, "FORMAT_CSV", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_CONTENT_TYPE_PDF);
	PyDict_SetItemString(d, "CONTENT_TYPE_PDF", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_CONTENT_TYPE_HTML);
	PyDict_SetItemString(d, "CONTENT_TYPE_HTML", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_CONTENT_TYPE_TXT);
	PyDict_SetItemString(d, "CONTENT_TYPE_TXT", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_CONTENT_TYPE_CSV);
	PyDict_SetItemString(d, "CONTENT_TYPE_CSV", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_ORIENTATION_PORTRAIT);
	PyDict_SetItemString(d, "ORIENTATION_PORTRAIT", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_ORIENTATION_LANDSCAPE);
	PyDict_SetItemString(d, "ORIENTATION_LANDSCAPE", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_SIGNAL_ROW_CHANGE);
	PyDict_SetItemString(d, "SIGNAL_ROW_CHANGE", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_SIGNAL_REPORT_START);
	PyDict_SetItemString(d, "SIGNAL_REPORT_START", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_SIGNAL_REPORT_DONE);
	PyDict_SetItemString(d, "SIGNAL_REPORT_DONE", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_SIGNAL_REPORT_ITERATION);
	PyDict_SetItemString(d, "SIGNAL_REPORT_ITERATION", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_SIGNAL_PART_ITERATION);
	PyDict_SetItemString(d, "SIGNAL_PART_ITERATION", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_LINE_NORMAL);
	PyDict_SetItemString(d, "GRAPH_TYPE_LINE_NORMAL", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_LINE_STACKED);
	PyDict_SetItemString(d, "GRAPH_TYPE_LINE_STACKED", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_LINE_PERCENT);
	PyDict_SetItemString(d, "GRAPH_TYPE_LINE_PERCENT", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_AREA_NORMAL);
	PyDict_SetItemString(d, "GRAPH_TYPE_AREA_NORMAL", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_AREA_STACKED);
	PyDict_SetItemString(d, "GRAPH_TYPE_AREA_STACKED", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_AREA_PERCENT);
	PyDict_SetItemString(d, "GRAPH_TYPE_AREA_PERCENT", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_COLUMN_NORMAL);
	PyDict_SetItemString(d, "GRAPH_TYPE_COLUMN_NORMAL", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_COLUMN_STACKED);
	PyDict_SetItemString(d, "GRAPH_TYPE_COLUMN_STACKED", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_COLUMN_PERCENT);
	PyDict_SetItemString(d, "GRAPH_TYPE_COLUMN_PERCENT", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_ROW_NORMAL);
	PyDict_SetItemString(d, "GRAPH_TYPE_ROW_NORMAL", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_ROW_STACKED);
	PyDict_SetItemString(d, "GRAPH_TYPE_ROW_STACKED", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_ROW_PERCENT);
	PyDict_SetItemString(d, "GRAPH_TYPE_ROW_PERCENT", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_PIE_NORMAL);
	PyDict_SetItemString(d, "GRAPH_TYPE_PIE_NORMAL", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_PIE_RING);
	PyDict_SetItemString(d, "GRAPH_TYPE_PIE_RING", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_PIE_OFFSET);
	PyDict_SetItemString(d, "GRAPH_TYPE_PIE_OFFSET", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_XY_SYMBOLS_ONLY);
	PyDict_SetItemString(d, "GRAPH_TYPE_XY_SYMBOLS_ONLY", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_XY_LINES_WITH_SUMBOLS);
	PyDict_SetItemString(d, "GRAPH_TYPE_XY_LINES_WITH_SUMBOLS", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_XY_LINES_ONLY);
	PyDict_SetItemString(d, "GRAPH_TYPE_XY_LINES_ONLY", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_XY_CUBIC_SPLINE);
	PyDict_SetItemString(d, "GRAPH_TYPE_XY_CUBIC_SPLINE", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_XY_CUBIC_SPLINE_WIHT_SYMBOLS);
	PyDict_SetItemString(d, "GRAPH_TYPE_XY_CUBIC_SPLINE_WIHT_SYMBOLS", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_XY_BSPLINE);
	PyDict_SetItemString(d, "GRAPH_TYPE_XY_BSPLINE", i);
	Py_DECREF(i);

	i = PyInt_FromLong(RLIB_GRAPH_TYPE_XY_BSPLINE_WITH_SYMBOLS);
	PyDict_SetItemString(d, "GRAPH_TYPE_XY_BSPLINE_WITH_SYMBOLS", i);
	Py_DECREF(i);
    }
	if (PyType_Ready(&RLIBType) < 0) {
        
	} else {
    Py_INCREF(&RLIBType);
    PyModule_AddObject(m, "Rlib", (PyObject *)&RLIBType);
	}
}
