#include <Python.h>
#include <curl/curl.h>
#include "requests.h"
#include "easyadapter.h"
#include "util.h"
#include "constants.h"

static CURLcode
_Curl_invoke(CURL *curl)
{
    CURLcode ecode = curl_easy_perform(curl);
    if (ecode != CURLE_OK) {
        /* TODO: raise specific exception types */
        PyErr_Format(PyExc_Exception,
            "CURL error (%d): %s", ecode, curl_easy_strerror(ecode));
    }
    return ecode;
}

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

static int
_Curl_set_method(CURL *curl, PyObject *methodobj)
{
    if (!util_ensure_type(methodobj, &PyUnicode_Type, "method"))
        return -1;
    const char *method = PyUnicode_AsUTF8(methodobj);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, strcmp(method, "GET") == 0);
    curl_easy_setopt(curl, CURLOPT_NOBODY, strcmp(method, "HEAD") == 0);
    return 0;
}

static int
_Curl_apply_PreparedRequest(CURL *curl, PyObject *prepreq)
{
    /*
     * PreparedRequest properties:
     * - body
     * - headers
     * - hooks
     * - method
     * - path_url
     * - url
     */

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, RequestsMod_PreparedRequest_body(prepreq));

    curl_easy_setopt(curl, CURLOPT_URL, RequestsMod_PreparedRequest_url(prepreq));

    if (_Curl_set_method(curl, PyObject_GetAttrString(prepreq, "method")) < 0)
        return -1;

    return 0;
}

static int
_Curl_set_timeout(CURL *curl, PyObject *timeout)
{
    if (!util_has_value(timeout))
        return 0;

    if (PyNumber_Check(timeout)) {
        double to = PyFloat_AsDouble(timeout);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)(to * 1000.0));
        return 0;
    }

    if (PyTuple_Check(timeout)) {
        double connto, readto;
        if (!PyArg_ParseTuple(timeout, "dd", &connto, &readto)) {
            PyErr_SetString(PyExc_TypeError, "Invalid timeout argument");
            return -1;
        }
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, (long)(connto * 1000.0));
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)(readto * 1000.0));
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Invalid timeout argument");
    return -1;
}

static int
_Curl_set_verify(CURL *curl, PyObject *verify)
{
    if (PyBool_Check(verify)) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verify == Py_True);
        return 0;
    }

    if (PyUnicode_Check(verify)) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, PyUnicode_AsUTF8(verify));
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Invalid verify argument");
    return -1;
}

static int
_Curl_set_cert(CURL *curl, PyObject *certobj)
{
    if (!util_has_value(certobj))
        return 0;

    if (PyUnicode_Check(certobj)) {
        curl_easy_setopt(curl, CURLOPT_SSLCERT, PyUnicode_AsUTF8(certobj));
        return 0;
    }

    if (PyTuple_Check(certobj)) {
        const char *cert;
        const char *key;
        if (!PyArg_ParseTuple(certobj, "ss", &cert, &key)) {
            PyErr_SetString(PyExc_TypeError, "Invalid cert argument");
            return -1;
        }
        curl_easy_setopt(curl, CURLOPT_SSLCERT, cert);
        curl_easy_setopt(curl, CURLOPT_SSLKEY, key);
        return 0;
    }

    /* TODO: extend API for key password?
     * curl_easy_setopt(curl, CURLOPT_KEYPASSWD, ???);
     * See also: https://github.com/psf/requests/issues/1573
     *
     * Extend API for cert "blobs"? cert types? proxy certs?
     * SSL versions? OpenSSL engines? etc., etc.
     */

    PyErr_SetString(PyExc_TypeError, "Invalid cert argument");
    return -1;
}

static int
_Curl_set_http_version(CURL *curl, PyObject *http_version)
{
    if (!util_has_value(http_version))
        return 0;
    if (!util_ensure_type(http_version, &PyUnicode_Type, "http_version"))
        return -1;

    const char *verstr = PyUnicode_AsUTF8(http_version);

    long verval = CURL_HTTP_VERSION_NONE;

    if (strcmp("1.0", verstr) == 0) {
        verval = CURL_HTTP_VERSION_1_0;
    } else if (strcmp("1.1", verstr) == 0) {
        verval = CURL_HTTP_VERSION_1_0;
    } else if (strcmp("2", verstr) == 0) {
        verval = CURL_HTTP_VERSION_2_0;
#ifdef CURL_HTTP_VERSION_3
    } else if (strcmp("3", verstr) == 0) {
        verval = CURL_HTTP_VERSION_3;
#endif
    } else {
        PyErr_Format(PyExc_ValueError, "Unsupported HTTP version: %s\n", verstr);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, verval);

    return 0;
}

static int
_Curl_set_maxconnects(CURL *curl, PyObject *maxconnects)
{
    if (!util_has_value(maxconnects))
        return 0;
    if (!util_ensure_type(maxconnects, &PyLong_Type, "maxconnects"))
        return -1;

    long val = PyLong_AsLong(maxconnects);
    curl_easy_setopt(curl, CURLOPT_MAXCONNECTS, val);
    return 0;
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

        const char *vstart = util_skip_linearwhitespace(&sep[1]);

        Py_ssize_t vlen = end - vstart;
        PyObject *value = PyUnicode_FromStringAndSize(vstart, vlen);

        Py_ssize_t klen = sep - start;
        PyObject *key = PyUnicode_FromStringAndSize(start, klen);

        if (PyMapping_HasKey(headerdict, key)) {
            PyObject *existing_val = PyObject_GetItem(headerdict, key);
            PyObject *tmp = PyUnicode_Concat(existing_val, ConstantUnicodeComma);
            Py_DECREF(existing_val);
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

PyObject *
_status_line_reason(PyObject *statusbytes)
{
    const char *bytes = PyBytes_AsString(statusbytes);

    const char *protostart = bytes;
    const char *protoend = strchr(protostart, ' ');
    if (!protoend)
        return NULL;

    const char *codestart = util_skip_linearwhitespace(&protoend[1]);
    const char *codeend = strchr(codestart, ' ');
    if (!codeend)
        return NULL;

    const char *reason = util_skip_linearwhitespace(&codeend[1]);

    return *reason ? PyUnicode_FromString(reason) : util_Py_None_New();
}

static PyObject *
_parse_response_headers(PyObject *headerbytes, PyObject **reason)
{
    PyObject *status_line = NULL;
    PyObject *header_fields = NULL;

    _headers_split(headerbytes, &status_line, &header_fields);
    *reason = _status_line_reason(status_line);
    Py_DECREF(status_line);

    PyObject *headerdict = _header_fields_to_dict(header_fields);
    Py_DECREF(header_fields);

    return headerdict;
}

struct send_args {
    PyObject *request;
    PyObject *stream;
    PyObject *timeout;
    PyObject *verify;
    PyObject *cert;
    PyObject *proxies;
};

PyObject *
_Curl_send(CURL *curl, struct send_args *args)
{
    PyObject *headers = NULL;
    PyObject *body = NULL;

    if (_Curl_apply_PreparedRequest(curl, args->request) < 0)
        return NULL;
    if (_Curl_set_timeout(curl, args->timeout) < 0)
        return NULL;
    if (_Curl_set_verify(curl, args->verify) < 0)
        return NULL;
    if (_Curl_set_cert(curl, args->cert) < 0)
        return NULL;

    _Curl_set_buffers(curl, &headers, &body);

    if (_Curl_invoke(curl) != CURLE_OK)
        return NULL;

    PyObject *reason = NULL;
    PyObject *headerdict = _parse_response_headers(headers, &reason);

    /* Add to Response object?
     * HTTP_VERSION
     * timings: NAMELOOKUP, CONNECT, APPCONNECT, PRETRANSFER, STARTTRANSFER, TOTAL, REDIRECT
     * PRIMARY_IP, PRIMARY_PORT
     * CERTINFO
     * other...?
     */

    RequestsMod_ResponseArgs resp_args = {
        .status_code = _Curl_get_response_code(curl),
        .content = util_or_Py_None(body),
        .url = _Curl_get_effective_url(curl),
        .request = util_incref(args->request),
        .headers = headerdict,
        .reason = reason,
        .encoding = RequestsMod_get_encoding_from_headers(headerdict),
    };

    return RequestsMod_Response_InitNew(&resp_args);
}

static PyObject *
CurlEasyAdapter_send(PyObject *_self, PyObject *args, PyObject *kwargs)
{
    CurlEasyAdapter *self = (CurlEasyAdapter *)_self;
    struct send_args send_args = {0};

    char *kwlist[] = {
        "request",      /* partial impl */
        "stream",       /* TODO */
        "timeout",      /* done */
        "verify",       /* done */
        "cert",         /* done */
        "proxies",      /* TODO */
        NULL
    };

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOO", kwlist,
            &send_args.request, &send_args.stream, &send_args.timeout,
            &send_args.verify, &send_args.cert, &send_args.proxies) < 0) {
        return NULL;
    }

    return _Curl_send(self->curl, &send_args);
}

static PyObject *
CurlEasyAdapter_close(PyObject *_self, PyObject *args)
{
    CurlEasyAdapter *self = (CurlEasyAdapter *)_self;
    curl_easy_reset(self->curl);
    Py_RETURN_NONE;
}

PyMethodDef methods[] = {
    {"send", (PyCFunction)CurlEasyAdapter_send, METH_VARARGS | METH_KEYWORDS, ""},
    {"close", CurlEasyAdapter_close, METH_NOARGS, ""},
    {0},
};

static PyObject *
_CurlEasyAdapter_New(PyTypeObject *tp, PyObject *args, PyObject *kwargs)
{
    /*
     * requests HTTPAdapter supported arguments:
     * - pool_connections
     * - pool_maxsize
     * - max_retries
     * - pool_block
     */

    /*
     * CURLOPTs to possibly support:
     * - DNS_SERVERS
     * - DNS_CACHE_TIMEOUT
     * - VERBOSE
     * - USERAGENT
     * - REFERER
     * - INTERFACE
     * - MAXREDIRS? (handled by requests.Session)
     */

    /* Keyword names match CURLOPT_* name */
    const char *kwlist[] = {
        "http_version", "maxconnects", NULL
    };

    PyObject *http_version = NULL;
    PyObject *maxconnects = NULL;

    util_pick_off_keywords(kwargs, kwlist, &http_version, &maxconnects);

    CurlEasyAdapter *new = (CurlEasyAdapter *)tp->tp_alloc(tp, 0);
    new->curl = curl_easy_init();

    if (_Curl_set_http_version(new->curl, http_version) < 0)
        goto fail;
    if (_Curl_set_maxconnects(new->curl, maxconnects) < 0)
        goto fail;

    return (PyObject *)new;

fail:
    Py_DECREF(new);
    return NULL;
}

PyObject *
CurlEasyAdapter_New(PyObject *kwargs)
{
    PyObject *args = PyTuple_New(0);
    PyObject *new = _CurlEasyAdapter_New(&CurlEasyAdapter_Type, args, kwargs);
    Py_DECREF(args);
    return new;
}

static void
CurlEasyAdapter_Dealloc(PyObject *_self)
{
    CurlEasyAdapter *self = (CurlEasyAdapter *)_self;
    curl_easy_cleanup(self->curl);
    Py_TYPE(self)->tp_free(self);
}

PyTypeObject CurlEasyAdapter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "CurlEasyAdapter",
    .tp_doc = "",
    .tp_basicsize = sizeof(CurlEasyAdapter),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = _CurlEasyAdapter_New,
    .tp_dealloc = CurlEasyAdapter_Dealloc,
    .tp_members = NULL,
    .tp_methods = methods,
};

