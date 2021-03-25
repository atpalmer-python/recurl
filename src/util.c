#include <Python.h>

const char *
util_skip_linearwhitespace(const char *p)
{
    while (*p == ' ' || *p == '\t')
        ++p;
    return p;
}

PyObject *
util_Py_None_New(void)
{
    Py_RETURN_NONE;
}

PyObject *
util_or_Py_None(PyObject *o)
{
    return o ? o : util_Py_None_New();
}

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
