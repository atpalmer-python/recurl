#include <curl/curl.h>
#include "requests.h"

/* TODO: move to util */
PyObject *
util_obj_BuildAttrString(PyObject *o, const char *name, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    PyObject *value = Py_VaBuildValue(fmt, va);
    va_end(va);

    if (!value)
        return NULL;

    PyObject *result = PyObject_SetAttrString(o, name, value);

    Py_DECREF(value);

    return result;
}

static PyObject *
_exctype_from_CURLcode(CURLcode code)
{
    PyObject *exctype = NULL;

    switch (code) {
    case CURLE_COULDNT_RESOLVE_PROXY:  /* code 5 */
        exctype = RequestsMod_exception("ProxyError");  /* extends ConnectionError */
        break;
    case CURLE_COULDNT_RESOLVE_HOST:  /* code 6 */
        exctype = RequestsMod_exception("ConnectionError");  /* extends RequestException */
        break;
    default:
        exctype = RequestsMod_exception("RequestException");  /* extends IOError */
        break;
    }

    return exctype;
}

static PyObject *
_excobj_new(PyObject *exctype, CURLcode code, const char *errorbuffer, PyObject *request, PyObject *response)
{
    PyObject *msg = NULL;
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *excobj = NULL;

    msg = PyUnicode_FromFormat("%s (CURLcode: %ld)", curl_easy_strerror(code), code);
    if (!msg)
        goto out;
    args = Py_BuildValue("(O)", msg);
    if (!args)
        goto out;
    kwargs = Py_BuildValue("{s:O, s:O}", "request", request, "response", response);
    if (!kwargs)
        goto out;

    excobj = PyObject_Call(exctype, args, kwargs);
    if (!excobj)
        goto out;

    util_obj_BuildAttrString(excobj, "curl_code", "i", code);
    util_obj_BuildAttrString(excobj, "curl_strerror", "s", curl_easy_strerror(code));
    util_obj_BuildAttrString(excobj, "curl_errorbuffer", "s", errorbuffer);

out:
    Py_XDECREF(msg);
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    return excobj;
}

void
exc_set_from_CURLcode(CURLcode code, const char *errorbuffer, PyObject *request, PyObject *response)
{
    PyObject *exctype = _exctype_from_CURLcode(code);
    if (!exctype)
        return;

    PyObject *excobj = _excobj_new(exctype, code, errorbuffer, request, response);
    if (!excobj)
        return;

    PyErr_SetObject(exctype, excobj);
    Py_DECREF(excobj);
}

