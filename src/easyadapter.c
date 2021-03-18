#include <Python.h>
#include "easyadapter.h"

PyObject *
CurlEasyAdapter_New(PyTypeObject *tp, PyObject *args, PyObject *kwargs)
{
    CurlEasyAdapter *new = (CurlEasyAdapter *)tp->tp_alloc(tp, 0);
    return (PyObject *)new;
}

void
CurlEasyAdapter_Dealloc(PyObject *self)
{
    Py_TYPE(self)->tp_free(self);
}

PyTypeObject CurlEasyAdapter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "CurlEasyAdapter",
    .tp_doc = "",
    .tp_basicsize = sizeof(CurlEasyAdapter),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = CurlEasyAdapter_New,
    .tp_dealloc = CurlEasyAdapter_Dealloc,
    .tp_members = NULL,
    .tp_methods = NULL,
};

