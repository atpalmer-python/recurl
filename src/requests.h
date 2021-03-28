#ifndef REQUESTS_MOD_H
#define REQUESTS_MOD_H

#include <Python.h>

typedef struct {
    PyObject *status_code;
    PyObject *content;
    PyObject *url;
    PyObject *request;
    PyObject *headers;
    PyObject *reason;
    PyObject *encoding;
} RequestsMod_ResponseArgs;

PyObject *RequestsMod_Response_InitNew(RequestsMod_ResponseArgs *args);
PyObject *RequestsMod_CaseInsensitiveDict_New(void);
PyObject *RequestsMod_Session_New(void);
PyObject *RequestsMod_get_encoding_from_headers(PyObject *headers);
PyObject *RequestsMod_select_proxy(PyObject *url, PyObject *proxies);
PyObject *RequestsMod_exception(const char *name);

#endif
