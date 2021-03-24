#include <Python.h>
#include "easyadapter.h"
#include "requests.h"

static PyObject *
_CurlEasySession(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *adapter = NULL;
    PyObject *session = NULL;
    PyObject *mount = NULL;
    PyObject *http = NULL;
    PyObject *https = NULL;
    PyObject *mresult = NULL;

    adapter = CurlEasyAdapter_New(kwargs);
    if (!adapter)
        goto fail;
    session = RequestsMod_Session_New();
    if (!session)
        goto fail;

    mount = PyUnicode_FromString("mount");
    if (!mount)
        goto fail;
    http = PyUnicode_FromString("http://");
    if (!http)
        goto fail;
    https = PyUnicode_FromString("https://");
    if (!https)
        goto fail;

    PyObject *httpsargs[] = {session, https, adapter};
    mresult = PyObject_VectorcallMethod(mount, httpsargs, 3, NULL);
    if (!mresult)
        goto fail;
    Py_DECREF(mresult);

    PyObject *httpargs[] = {session, http, adapter};
    mresult = PyObject_VectorcallMethod(mount, httpargs, 3, NULL);
    if (!mresult)
        goto fail;
    Py_DECREF(mresult);

    Py_DECREF(mount);
    Py_DECREF(http);
    Py_DECREF(https);

    return session;

fail:
    Py_XDECREF(adapter);
    Py_XDECREF(session);
    Py_XDECREF(mount);
    Py_XDECREF(http);
    Py_XDECREF(https);
    return NULL;
}

static PyObject *
_request(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *session = NULL;
    PyObject *emptyargs = PyTuple_New(0);

    if (!emptyargs)
        goto fail;

    session = _CurlEasySession(self, emptyargs, kwargs);
    if (!session)
        goto fail;

    PyObject *method = PyObject_GetAttrString(session, "request");
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

static PyMethodDef methods[] = {
    {"CurlEasySession", (PyCFunction)_CurlEasySession, METH_VARARGS | METH_KEYWORDS, ""},
    {"request", (PyCFunction)_request, METH_VARARGS | METH_KEYWORDS, ""},
    {0},
};

struct PyModuleDef module_def = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "requests_curl",
    .m_doc = "",
    .m_size = 0,
    .m_methods = methods,
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

    return module;

fail:
    Py_DECREF(module);
    return NULL;
}

