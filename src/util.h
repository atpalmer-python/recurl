#ifndef UTIL_H
#define UTIL_H

#include <Python.h>

const char *util_skip_linearwhitespace(const char *p);

int util_has_value(PyObject *o);

PyObject *util_ensure_type(PyObject *o, PyTypeObject *tp, const char *name);
PyObject *util_ensure_mapping(PyObject *o, const char *name);

PyObject *util_or_Py_None(PyObject *o);

int util_obj_BuildAttrString(PyObject *o, const char *name, const char *fmt, ...);

PyObject *util_dict_pop(PyObject *dict, const char *key);
PyObject *util_pick_off_keywords(PyObject *kwargs, const char *kwlist[], ...);

#endif
