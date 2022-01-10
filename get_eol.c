#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>

/**
 * this file wrap a c funtion to python package.
 * we compile this file to a so file, and import it in python
 * so we can call this c function in python directly
 * 
 * this file returns a list
 * list[0]: the length of list
 * list[1:len]: eol index(relative index in this buffer)
 */

/*
len: length of fstr
fstr: file content as a string
qc: quote char
xc: escape char
eol: end of line char
 d
return: the index of eol in a list
*/
int * get_eol(int len, char* fstr, int qc, int xc, int eol){
	int 	in_quote = 0;
	int 	last_was_esc = 0;
	int 	ch = 0;
	int 	i = 0;
	int 	res_len = 0;
    int 	*res;
	res = (int*)calloc(len, sizeof(int));

	for (i=0; i < len && fstr[i]!='\0'; i++){
		ch = fstr[i];
		if (in_quote){
			if (!last_was_esc){
				if (ch == qc)
					in_quote = 0;
				else if (ch == xc)
					last_was_esc = 1;
			}
			else
				last_was_esc = 0;
		}
		else if (ch == eol){	
			res[res_len] = i;
			res_len++;
		}
		else if (ch == qc){
			in_quote = 1;
		}
	}

	return res;
}


static PyObject * _get_eol(PyObject *self, PyObject *args){
	int _len;
	char* _fstr;
	char _qc;
	char _xc;
	char _eol;
    int * res;
	res = (int*)calloc(_len, sizeof(int));

	if (!PyArg_ParseTuple(args, "lsccc", &_len, &_fstr, &_qc, &_xc, &_eol))  
        return NULL;
	// l means python int to c long
	// s means python unicode to c char*
    // c means python string(1 char) to c int
    
	res = get_eol(_len, _fstr, _qc, _xc, _eol);
	// we use len of size to record eol index, 
	// for normal data at most has half of eol

	PyObject* mypylist = PyList_New(_len);
	int i = 1;
	for (; res[i-1]; i++){
          // set the i element in mypylist to res[i-1],
          // PyLong_FromLong convert a c int to python int
          PyList_SetItem(mypylist, i, PyLong_FromLong(res[i - 1]));
	}
        // mypylist[0] records the length of this list.
        PyList_SetItem(mypylist, 0, PyLong_FromLong(i));
	return mypylist;
}

static PyMethodDef GetEolMethods[] = {
	{
		"get_eol",
		_get_eol,
		METH_VARARGS,
		"get a list of the index of eol in the str"
	},
	{NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC initget_eol(void) {
    (void) Py_InitModule("get_eol", GetEolMethods);
}