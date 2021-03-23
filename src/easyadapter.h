#ifndef EASY_ADAPTER_H
#define EASY_ADAPTER_H

#include <curl/curl.h>

typedef struct {
    PyObject_HEAD
    CURL *curl;
} CurlEasyAdapter;

extern PyTypeObject CurlEasyAdapter_Type;

PyObject *CurlEasyAdapter_New(PyObject *args, PyObject *kwargs);

#endif
