#include <Python.h>
#include "easyadapter.h"
#include "requests.h"

static PyObject *
_CurlEasySession(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *adapter = CurlEasyAdapter_New(kwargs);
    PyObject *result = RequestsMod_Session_New();

    PyObject *mount = PyUnicode_FromString("mount");
    PyObject *http = PyUnicode_FromString("http://");
    PyObject *https = PyUnicode_FromString("https://");

    PyObject *mresult = NULL;
    PyObject *httpsargs[] = {result, https, adapter};
    mresult = PyObject_VectorcallMethod(mount, httpsargs, 3, NULL);
    Py_DECREF(mresult);
    PyObject *httpargs[] = {result, http, adapter};
    mresult = PyObject_VectorcallMethod(mount, httpargs, 3, NULL);
    Py_DECREF(mresult);

    Py_DECREF(mount);
    Py_DECREF(http);
    Py_DECREF(https);

    return result;
}

static PyMethodDef methods[] = {
    {"CurlEasySession", (PyCFunction)_CurlEasySession, METH_VARARGS | METH_KEYWORDS, ""},
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

