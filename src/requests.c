#include <Python.h>
#include "requests.h"

PyObject *
_import_obj(const char *modname, const char *objname)
{
    PyObject *mod = PyImport_ImportModule(modname);
    if (!mod)
        return NULL;
    PyObject *obj = PyObject_GetAttrString(mod, objname);
    if (!obj)
        return NULL;
    return obj;
}

PyObject *
_import_default_instance(const char *modname, const char *typename)
{
    PyObject *typeobj = _import_obj(modname, typename);
    if (!typeobj)
        return NULL;
    PyObject *new = PyObject_CallNoArgs(typeobj);
    if (!new)
        return NULL;
    return new;
}

PyObject *
RequestsMod_Response_InitNew(RequestsMod_ResponseArgs *args)
{
    /*
     * requests.Request attributes:
     * '_content', 'status_code', 'headers', 'url', 'history',
     * 'encoding', 'reason', 'cookies', 'elapsed', 'request'
     */
    PyObject *response = _import_default_instance("requests", "Response");
    if (!response)
        return NULL;

    if (PyObject_SetAttrString(response, "_content", args->content) < 0)
        return NULL;
    if (PyObject_SetAttrString(response, "status_code", args->status_code) < 0)
        return NULL;
    if (PyObject_SetAttrString(response, "url", args->url) < 0)
        return NULL;
    if (PyObject_SetAttrString(response, "request", args->request) < 0)
        return NULL;
    if (PyObject_SetAttrString(response, "headers", args->headers) < 0)
        return NULL;
    if (PyObject_SetAttrString(response, "reason", args->reason) < 0)
        return NULL;

    return response;
}

PyObject *
RequestsMod_CaseInsensitiveDict_New(void)
{
    return _import_default_instance("requests.structures", "CaseInsensitiveDict");
}

const char *
RequestsMod_PreparedRequest_url(PyObject *request)
{
    PyObject *urlobj = PyObject_GetAttrString(request, "url");
    if (!urlobj)
        return NULL;
    if (!PyUnicode_Check(urlobj))
        return NULL;
    return PyUnicode_AsUTF8(urlobj);
}
