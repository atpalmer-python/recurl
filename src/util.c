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

void
util_pick_off_keywords(PyObject *kwdict, const char *kwlist[], ...)
{
    if (!kwdict)
        return;

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
