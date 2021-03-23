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
    new->curl = curl_easy_init();
    return (PyObject *)new;
}

static void
CurlEasyAdapter_Dealloc(PyObject *_self)
{
    CurlEasyAdapter *self = (CurlEasyAdapter *)_self;
    curl_easy_cleanup(self->curl);
    Py_TYPE(self)->tp_free(self);
}

static int
_headers_split(PyObject *bytesobj, PyObject **status_line, PyObject **rest)
{
    const char *bytes = PyBytes_AsString(bytesobj);
    const char *eol = strstr(bytes, "\r\n");
    if (!eol)
        return -1;
    *status_line = PyBytes_FromStringAndSize(bytes, eol - bytes);
    if (!*status_line)
        return -1;
    *rest = PyBytes_FromString(&eol[2]);
    if (!*rest)
        return -1;
    return 0;
}

static PyObject *
_header_fields_to_dict(PyObject *fieldbytes)
{
    PyObject *headerdict = RequestsMod_CaseInsensitiveDict_New();

    const char *fieldstring = PyBytes_AsString(fieldbytes);

    const char *start = fieldstring;
    for (;;) {
        /* TODO: handle continuations (lines starting with whitespace) */
        const char *sep = strchr(start, ':');
        if (!sep)
            break;
        const char *end = strstr(sep, "\r\n");
        if (!end)
            break;

        const char *vstart = &sep[1];
        while (*vstart == ' ')
            ++vstart;
        Py_ssize_t vlen = end - vstart;
        PyObject *value = PyUnicode_FromStringAndSize(vstart, vlen);

        Py_ssize_t klen = sep - start;
        PyObject *key = PyUnicode_FromStringAndSize(start, klen);

        if (PyMapping_HasKey(headerdict, key)) {
            PyObject *existing_val = PyObject_GetItem(headerdict, key);
            PyObject *comma = PyUnicode_FromString(", ");
            PyObject *tmp = PyUnicode_Concat(existing_val, comma);
            Py_DECREF(existing_val);
            Py_DECREF(comma);
            PyObject *newvalue = PyUnicode_Concat(tmp, value);
            Py_DECREF(tmp);
            Py_DECREF(value);
            value = newvalue;
        }

        PyObject_SetItem(headerdict, key, value);
        start = &end[2];
    }

    return headerdict;
}

const char *
_skip_linearwhitespace(const char *p)
{
    while (*p == ' ' || *p == '\t')
        ++p;
    return p;
}

PyObject *
_Py_None_New(void)
{
    Py_RETURN_NONE;
}

PyObject *
_or_Py_None(PyObject *o)
{
    return o ? o : _Py_None_New();
}

PyObject *
_status_line_reason(PyObject *statusbytes)
{
    const char *bytes = PyBytes_AsString(statusbytes);

    const char *protostart = bytes;
    const char *protoend = strchr(protostart, ' ');
    if (!protoend)
        return NULL;

    const char *codestart = _skip_linearwhitespace(&protoend[1]);
    const char *codeend = strchr(codestart, ' ');
    if (!codeend)
        return NULL;

    const char *reason = _skip_linearwhitespace(&codeend[1]);

    return *reason ? PyUnicode_FromString(reason) : _Py_None_New();
}

static PyObject *
CurlEasyAdapter_send(PyObject *_self, PyObject *args, PyObject *kwargs)
{
    CurlEasyAdapter *self = (CurlEasyAdapter *)_self;

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

    _Curl_apply_PreparedRequest(self->curl, request);
    _Curl_set_buffers(self->curl, &headers, &body);
    curl_easy_perform(self->curl);

    PyObject *status_line = NULL;
    PyObject *header_fields = NULL;

    _headers_split(headers, &status_line, &header_fields);
    PyObject *reason = _status_line_reason(status_line);
    Py_DECREF(status_line);

    PyObject *headerdict = _header_fields_to_dict(header_fields);
    Py_DECREF(header_fields);

    Py_INCREF(request);

    RequestsMod_ResponseArgs resp_args = {
        .status_code = _Curl_get_response_code(self->curl),
        .content = _or_Py_None(body),
        .url = _Curl_get_effective_url(self->curl),
        .request = request,
        .headers = headerdict,
        .reason = reason,
    };

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

