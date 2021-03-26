#ifndef UTIL_H
#define UTIL_H

#include <Python.h>

const char *util_skip_linearwhitespace(const char *p);

int util_has_value(PyObject *o);

PyObject *util_Py_None_New(void);
PyObject *util_or_Py_None(PyObject *o);
PyObject *util_incref(PyObject *o);

PyObject *util_dict_pop(PyObject *dict, const char *key);
PyObject *util_pick_off_keywords(PyObject *kwargs, const char *kwlist[], ...);

#endif
