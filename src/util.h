#ifndef UTIL_H
#define UTIL_H

#include <Python.h>

PyObject *util_dict_pop(PyObject *dict, const char *key);
PyObject *util_pick_off_keywords(PyObject *kwargs, const char *kwlist[], ...);

#endif
