#include <Python.h>

const char *
util_skip_linearwhitespace(const char *p)
{
    while (*p == ' ' || *p == '\t')
        ++p;
    return p;
}

int
util_has_value(PyObject *o)
{
    if (!o)
        return 0;
    if (o == Py_None)
        return 0;
    return 1;
}

PyObject *
util_ensure_type(PyObject *o, PyTypeObject *tp, const char *name)
{
    if (PyObject_IsInstance(o, (PyObject *)tp))
        return o;

    PyErr_Format(PyExc_TypeError,
        "%s must be of type '%s', not '%s'\n",
        name,
        tp->tp_name,
        Py_TYPE(o)->tp_name);

    return NULL;
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
util_incref(PyObject *o)
{
    Py_INCREF(o);
    return o;
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
