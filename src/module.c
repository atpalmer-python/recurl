#include <Python.h>
#include "easyadapter.h"
#include "requests.h"
#include "constants.h"

static PyObject *
_CurlEasySession(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *adapter = NULL;
    PyObject *session = NULL;
    PyObject *mresult = NULL;

    adapter = CurlEasyAdapter_New(kwargs);
    if (!adapter)
        goto fail;
    session = RequestsMod_Session_New();
    if (!session)
        goto fail;

    PyObject *httpsargs[] = {session, ConstantUnicodeHTTPS, adapter};
    mresult = PyObject_VectorcallMethod(ConstantUnicode_mount, httpsargs, 3, NULL);
    if (!mresult)
        goto fail;
    Py_DECREF(mresult);

    PyObject *httpargs[] = {session, ConstantUnicodeHTTP, adapter};
    mresult = PyObject_VectorcallMethod(ConstantUnicode_mount, httpargs, 3, NULL);
    if (!mresult)
        goto fail;
    Py_DECREF(mresult);

    return session;

fail:
    Py_XDECREF(adapter);
    Py_XDECREF(session);
    return NULL;
}

static PyObject *
_session_method_call(const char *mname, PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *session = NULL;
    PyObject *emptyargs = PyTuple_New(0);
    if (!emptyargs)
        goto fail;
    session = _CurlEasySession(self, emptyargs, kwargs);
    if (!session)
        goto fail;
    PyObject *method = PyObject_GetAttrString(session, mname);
    if (!method)
        goto fail;
    PyObject *result = PyObject_Call(method, args, kwargs);
    Py_DECREF(session);
    Py_DECREF(emptyargs);
    return result;

fail:
    Py_XDECREF(session);
    Py_XDECREF(emptyargs);
    return NULL;
}

static PyObject *
_request(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return _session_method_call("request", self, args, kwargs);
}

static PyObject *
_head(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return _session_method_call("head", self, args, kwargs);
}

static PyObject *
_get(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return _session_method_call("get", self, args, kwargs);
}

static PyObject *
_post(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return _session_method_call("post", self, args, kwargs);
}

static PyObject *
_put(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return _session_method_call("put", self, args, kwargs);
}

static PyObject *
_patch(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return _session_method_call("patch", self, args, kwargs);
}

static PyObject *
_delete(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return _session_method_call("delete", self, args, kwargs);
}

static PyMethodDef methods[] = {
    {"CurlEasySession", (PyCFunction)_CurlEasySession, METH_VARARGS | METH_KEYWORDS, ""},
    {"request", (PyCFunction)_request, METH_VARARGS | METH_KEYWORDS, ""},
    {"head", (PyCFunction)_head, METH_VARARGS | METH_KEYWORDS, ""},
    {"get", (PyCFunction)_get, METH_VARARGS | METH_KEYWORDS, ""},
    {"post", (PyCFunction)_post, METH_VARARGS | METH_KEYWORDS, ""},
    {"put", (PyCFunction)_put, METH_VARARGS | METH_KEYWORDS, ""},
    {"patch", (PyCFunction)_patch, METH_VARARGS | METH_KEYWORDS, ""},
    {"delete", (PyCFunction)_delete, METH_VARARGS | METH_KEYWORDS, ""},
    {0},
};

static void
_free(void *self)
{
    Constants_Free();
    Py_DECREF(self);
}

struct PyModuleDef module_def = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "requests_curl",
    .m_doc = "",
    .m_size = 0,
    .m_methods = methods,
    .m_free = _free,
};

PyMODINIT_FUNC
PyInit_requests_curl(void)
{
    PyObject *module = PyModule_Create(&module_def);
    if (!module)
        return NULL;

    if (PyType_Ready(&CurlEasyAdapter_Type) < 0)
        goto fail;
    Py_INCREF(&CurlEasyAdapter_Type);
    PyModule_AddObject(module, CurlEasyAdapter_Type.tp_name, (PyObject *)&CurlEasyAdapter_Type);

    if (Constants_Init() < 0)
        goto fail;

    return module;

fail:
    Py_DECREF(module);
    return NULL;
}

