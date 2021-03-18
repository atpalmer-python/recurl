#include <Python.h>
#include <curl/curl.h>
#include "easyadapter.h"

static PyObject *
_Response_New(void)
{
    PyObject *requestsmod = PyImport_ImportModule("requests");
    if(!requestsmod)
        return NULL;
    PyObject *response_class = PyObject_GetAttrString(requestsmod, "Response");
    if(!response_class)
        return NULL;
    PyObject *response = PyObject_CallNoArgs(response_class);
    if(!response)
        return NULL;
    return response;
}

static const char *
_PreparedRequest_url(PyObject *request)
{
    PyObject *urlobj = PyObject_GetAttrString(request, "url");
    if(!urlobj)
        return NULL;
    if(!PyUnicode_Check(urlobj))
        return NULL;
    return PyUnicode_AsUTF8(urlobj);
}

static int
_Curl_apply_PreparedRequest(CURL *curl, PyObject *prepreq)
{
    int r = 0;
    r = curl_easy_setopt(curl, CURLOPT_URL, _PreparedRequest_url(prepreq));
    if(r)
        return r;
    return r;
}

static PyObject *
CurlEasyAdapter_New(PyTypeObject *tp, PyObject *args, PyObject *kwargs)
{
    CurlEasyAdapter *new = (CurlEasyAdapter *)tp->tp_alloc(tp, 0);
    return (PyObject *)new;
}

static void
CurlEasyAdapter_Dealloc(PyObject *self)
{
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
CurlEasyAdapter_send(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *kwlist[] = {
        "request", "stream", "timeout", "verify", "cert", "proxies", NULL
    };

    PyObject *request = NULL;
    PyObject *stream = NULL;
    PyObject *timeout = NULL;
    PyObject *verify = NULL;
    PyObject *cert = NULL;
    PyObject *proxies = NULL;

    if(PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOO", kwlist,
            &request, &stream, &timeout, &verify, &cert, &proxies) < 0) {
        return NULL;
    }

    CURL *curl = curl_easy_init();
    _Curl_apply_PreparedRequest(curl, request);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return _Response_New();
}

PyMethodDef methods[] = {
    {"send", (PyCFunction)CurlEasyAdapter_send, METH_VARARGS | METH_KEYWORDS, ""},
    {0},
};

PyTypeObject CurlEasyAdapter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "CurlEasyAdapter",
    .tp_doc = "",
    .tp_basicsize = sizeof(CurlEasyAdapter),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = CurlEasyAdapter_New,
    .tp_dealloc = CurlEasyAdapter_Dealloc,
    .tp_members = NULL,
    .tp_methods = methods,
};

