#ifndef REQUESTS_MOD_H
#define REQUESTS_MOD_H

#include <Python.h>

typedef struct {
    PyObject *status_code;
    PyObject *content;
    PyObject *url;
    PyObject *request;
    PyObject *headers;
} RequestsMod_ResponseArgs;

PyObject *RequestsMod_Response_InitNew(RequestsMod_ResponseArgs *args);
PyObject *RequestsMod_CaseInsensitiveDict_New(void);
const char *RequestsMod_PreparedRequest_url(PyObject *request);

#endif
