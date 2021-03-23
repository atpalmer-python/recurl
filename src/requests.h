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
} RequestsMod_ResponseArgs;

PyObject *RequestsMod_Response_InitNew(RequestsMod_ResponseArgs *args);
PyObject *RequestsMod_CaseInsensitiveDict_New(void);
PyObject *RequestsMod_Session_New(void);
const char *RequestsMod_PreparedRequest_url(PyObject *request);
const char *RequestsMod_PreparedRequest_method(PyObject *request);

#endif
