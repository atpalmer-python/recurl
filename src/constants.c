#include <Python.h>
#include "constants.h"

PyObject *ConstantUnicodeHTTP = NULL;
PyObject *ConstantUnicodeHTTPS = NULL;
PyObject *ConstantUnicode_mount = NULL;

int
Constants_Init(void)
{
    ConstantUnicodeHTTP = PyUnicode_FromString("http://");
    if (!ConstantUnicodeHTTP) {
        Constants_Free();
        return -1;
    }

    ConstantUnicodeHTTPS = PyUnicode_FromString("https://");
    if (!ConstantUnicodeHTTPS) {
        Constants_Free();
        return -1;
    }

    ConstantUnicode_mount = PyUnicode_FromString("mount");
    if (!ConstantUnicode_mount) {
        Constants_Free();
        return -1;
    }

    return 0;
}

void
Constants_Free(void)
{
    Py_XDECREF(ConstantUnicodeHTTP);
    Py_XDECREF(ConstantUnicodeHTTPS);
    Py_XDECREF(ConstantUnicode_mount);
}

