#include <Python.h>
#include "easyadapter.h"

struct PyModuleDef module_def = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "requests_curl",
    .m_doc = "",
    .m_size = 0,
    .m_methods = NULL,
};

PyMODINIT_FUNC
PyInit_requests_curl(void)
{
    PyObject *module = PyModule_Create(&module_def);
    if (!module)
        return NULL;

    if(PyType_Ready(&CurlEasyAdapter_Type) < 0)
        goto fail;
    Py_INCREF(&CurlEasyAdapter_Type);
    PyModule_AddObject(module, CurlEasyAdapter_Type.tp_name, (PyObject *)&CurlEasyAdapter_Type);

    return module;

fail:
    Py_DECREF(module);
    return NULL;
}

