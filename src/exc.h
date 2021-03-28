#ifndef EXC_H
#define EXC_H

#include <curl/curl.h>

void exc_set_from_CURLcode(
    CURLcode code,
    const char *errorbuffer,
    PyObject *request,
    PyObject *response);

#endif
