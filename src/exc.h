#ifndef EXC_H
#define EXC_H

#include <curl/curl.h>

void exc_set_from_CURLcode(CURLcode code, PyObject *request, PyObject *response);

#endif
