#include <Python.h>

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

    return module;
}

