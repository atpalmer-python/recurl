#include <Python.h>
#include <curl/curl.h>
#include "curlwrap.h"
#include "easyadapter.h"

static PyObject *
CurlEasyAdapter_send(PyObject *_self, PyObject *args, PyObject *kwargs)
{
    CurlEasyAdapter *self = (CurlEasyAdapter *)_self;
    struct CurlWrap_send_args send_args = {0};

    char *kwlist[] = {
        "request", "stream", "timeout", "verify", "cert", "proxies", NULL
    };

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOO", kwlist,
            &send_args.request, &send_args.stream, &send_args.timeout,
            &send_args.verify, &send_args.cert, &send_args.proxies) < 0) {
        return NULL;
    }

    return CurlWrap_send(self->curl, &send_args);
}

static PyObject *
CurlEasyAdapter_close(PyObject *_self, PyObject *args)
{
    CurlEasyAdapter *self = (CurlEasyAdapter *)_self;
    curl_easy_reset(self->curl);  /* TODO: wrap curl API */
    Py_RETURN_NONE;
}

PyMethodDef methods[] = {
    {"send", (PyCFunction)CurlEasyAdapter_send, METH_VARARGS | METH_KEYWORDS, ""},
    {"close", CurlEasyAdapter_close, METH_NOARGS, ""},
    {0},
};

static PyObject *
_CurlEasyAdapter_New(PyTypeObject *tp, PyObject *args, PyObject *kwargs)
{
    CurlEasyAdapter *new = (CurlEasyAdapter *)tp->tp_alloc(tp, 0);
    if (!new)
        return NULL;

    new->curl = CurlWrap_new(kwargs);
    if (!new->curl)
        goto fail;

    return (PyObject *)new;

fail:
    Py_DECREF(new);
    return NULL;
}

PyObject *
CurlEasyAdapter_New(PyObject *kwargs)
{
    PyObject *args = PyTuple_New(0);
    PyObject *new = _CurlEasyAdapter_New(&CurlEasyAdapter_Type, args, kwargs);
    Py_DECREF(args);
    return new;
}

static void
CurlEasyAdapter_Dealloc(PyObject *_self)
{
    CurlEasyAdapter *self = (CurlEasyAdapter *)_self;
    CurlWrap_free(self->curl);
    Py_TYPE(self)->tp_free(self);
}

PyTypeObject CurlEasyAdapter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "CurlEasyAdapter",
    .tp_doc = "",
    .tp_basicsize = sizeof(CurlEasyAdapter),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = _CurlEasyAdapter_New,
    .tp_dealloc = CurlEasyAdapter_Dealloc,
    .tp_members = NULL,
    .tp_methods = methods,
};

