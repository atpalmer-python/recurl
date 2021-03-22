#include <Python.h>
#include "requests.h"

PyObject *
RequestsMod_Response_InitNew(RequestsMod_ResponseArgs *args)
{
    /*
     * requests.Request attributes:
     * '_content', 'status_code', 'headers', 'url', 'history',
     * 'encoding', 'reason', 'cookies', 'elapsed', 'request'
     */
    PyObject *requestsmod = PyImport_ImportModule("requests");
    if (!requestsmod)
        return NULL;
    PyObject *response_class = PyObject_GetAttrString(requestsmod, "Response");
    if (!response_class)
        return NULL;
    PyObject *response = PyObject_CallNoArgs(response_class);
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
    if (PyObject_SetAttrString(response, "headers", args->headers) < 0)     /* TODO: ensure case-insensitive */
        return NULL;

    return response;
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

