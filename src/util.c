#include <Python.h>

PyObject *
util_dict_pop(PyObject *dict, const char *key)
{
    PyObject *result = PyDict_GetItemString(dict, key);
    if (!result)
        return NULL;
    Py_INCREF(result);
    PyDict_DelItemString(dict, key);
    return result;
}

PyObject *
util_dict_pick_off(PyObject *dict, const char *keys[])
{
    PyObject *result = PyDict_New();
    const char *k = keys[0];
    while (k) {
        PyObject *val = util_dict_pop(dict, k);
        if (val) {
            PyDict_SetItemString(dict, k, val);
            Py_DECREF(val);
        }
        ++k;
    }
    return result;
}

void
util_pick_off_keywords(PyObject *kwdict, const char *kwlist[], ...)
{
    va_list vargs;
    va_start(vargs, kwlist);
    for (const char **kwp = kwlist; *kwp; ++kwp) {
        PyObject **target = va_arg(vargs, PyObject **);
        PyObject *val = util_dict_pop(kwdict, *kwp);
        if (val)
            *target = val;
    }
    va_end(vargs);
}
