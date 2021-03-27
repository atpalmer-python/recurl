#ifndef CURLWRAP_H
#define CURLWRAP_H

#include <Python.h>
#include <curl/curl.h>

CURL *CurlWrap_new(PyObject *kwargs);

struct CurlWrap_send_args {
    PyObject *request;
    PyObject *stream;
    PyObject *timeout;
    PyObject *verify;
    PyObject *cert;
    PyObject *proxies;
};

PyObject *CurlWrap_send(CURL *curl, struct CurlWrap_send_args *args);
void CurlWrap_free(CURL *curl);

#endif
