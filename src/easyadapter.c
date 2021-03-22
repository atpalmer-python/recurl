#include <Python.h>
#include <curl/curl.h>
#include "requests.h"
#include "easyadapter.h"

static PyObject *
_Curl_get_response_code(CURL *curl)
{
    long result;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result);
    return PyLong_FromLong(result);
}

static PyObject *
_Curl_get_effective_url(CURL *curl)
{
    char *result;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &result);
    return PyUnicode_FromString(result);
}

static void
_Curl_apply_PreparedRequest(CURL *curl, PyObject *prepreq)
{
    curl_easy_setopt(curl, CURLOPT_URL, RequestsMod_PreparedRequest_url(prepreq));
}

static size_t
write_callback(void *contents, size_t size, size_t count, void *_buff)
{
    PyObject **buff = (PyObject **)_buff;
    PyObject *newpart = PyBytes_FromStringAndSize(contents, size * count);
    if (!*buff)
        *buff = newpart;
    else
        PyBytes_ConcatAndDel(buff, newpart);
    return size * count;
}

static void
_Curl_set_buffers(CURL *curl, PyObject **headers, PyObject **body)
{
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, body);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, headers);
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

static int
_headers_split(PyObject *bytesobj, PyObject **status_line, PyObject **rest)
{
    const char *bytes = PyBytes_AsString(bytesobj);
    const char *eol = strchr(bytes, '\n');
    if(!eol)
        return -1;
    *status_line = PyBytes_FromStringAndSize(bytes, eol - bytes);
    if(!*status_line)
        return -1;
    *rest = PyBytes_FromString(&eol[1]);
    if(!*rest)
        return -1;
    return 0;
}

static PyObject *
_header_fields_to_dict(PyObject *fieldbytes)
{
    PyObject *headerdict = PyDict_New();

    const char *fieldstring = PyBytes_AsString(fieldbytes);

    const char *start = fieldstring;
    for(;;) {
        const char *sep = strchr(start, ':');
        if(!sep)
            break;
        const char *end = strstr(sep, "\r\n");
        if(!end)
            break;

        const char *vstart = &sep[1];
        while(*vstart == ' ')
            ++vstart;
        Py_ssize_t vlen = end - vstart;
        PyObject *value = PyUnicode_FromStringAndSize(vstart, vlen);

        Py_ssize_t klen = sep - start;
        PyObject *key = PyUnicode_FromStringAndSize(start, klen);
        PyDict_SetItem(headerdict, key, value);

        start = &end[2];
    }

    return headerdict;
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

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOO", kwlist,
            &request, &stream, &timeout, &verify, &cert, &proxies) < 0) {
        return NULL;
    }

    PyObject *headers = NULL;
    PyObject *body = NULL;

    CURL *curl = curl_easy_init();
    _Curl_apply_PreparedRequest(curl, request);
    _Curl_set_buffers(curl, &headers, &body);
    curl_easy_perform(curl);

    PyObject *status_line = NULL;
    PyObject *header_fields = NULL;

    _headers_split(headers, &status_line, &header_fields);

    printf("STATUS_LINE: %s\n", PyBytes_AsString(status_line));
    printf("HEADER_FIELDS:\n%s\n", PyBytes_AsString(header_fields));

    /* TODO: handle multiple "Set-Cookie" headers */
    PyObject *headerdict = _header_fields_to_dict(header_fields);

    printf("HEADER_FIELDS DICT (size: %d):\n", PyDict_Size(headerdict));
    printf("%s\n", PyUnicode_AsUTF8(PyObject_Repr(headerdict)));
    printf("***END HEADER_FIELDS DICT***\n");

    Py_INCREF(request);

    RequestsMod_ResponseArgs resp_args = {
        .status_code = _Curl_get_response_code(curl),
        .content = body,
        .url = _Curl_get_effective_url(curl),
        .request = request,
    };

    curl_easy_cleanup(curl);

    return RequestsMod_Response_InitNew(&resp_args);
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

