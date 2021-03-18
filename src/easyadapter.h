#ifndef EASY_ADAPTER_H
#define EASY_ADAPTER_H

typedef struct {
    PyObject_HEAD
} CurlEasyAdapter;

extern PyTypeObject CurlEasyAdapter_Type;

#endif
