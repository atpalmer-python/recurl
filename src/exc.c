#include <curl/curl.h>
#include "requests.h"

void
exc_set_from_CURLcode(CURLcode code, PyObject *request, PyObject *response)
{
    PyObject *msg = NULL;
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *exctype = NULL;

    switch (code) {

    /* TODO: specific exception types per CURLcode */

    default:
        exctype = RequestsMod_exception("RequestException");
        break;
    }

    if (!exctype)
        goto out;

    msg = PyUnicode_FromFormat("%s (CURLcode: %ld)", curl_easy_strerror(code), code);
    if (!msg)
        goto out;
    args = Py_BuildValue("(O)", msg);
    if (!args)
        goto out;
    kwargs = Py_BuildValue("{s:O, s:O}", "request", request, "response", response);
    if (!kwargs)
        goto out;

    PyObject *excobj = PyObject_Call(exctype, args, kwargs);
    if (!excobj)
        goto out;
    PyObject_SetAttrString(excobj, "curl_code", PyLong_FromLong(code));
    PyErr_SetObject(exctype, excobj);
    Py_DECREF(excobj);

out:
    Py_XDECREF(msg);
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
}

