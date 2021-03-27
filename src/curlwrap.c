#include <Python.h>
#include <curl/curl.h>
#include "requests.h"
#include "util.h"
#include "constants.h"
#include "curlwrap.h"

struct curl_private_data {
    /* not automatically freed by curl_easy_cleanup, so hold onto pointer */
    struct curl_slist *headers;
};

struct curl_private_data *
_Curl_get_private_or_new(CURL *curl)
{
    struct curl_private_data *data = NULL;
    curl_easy_getinfo(curl, CURLINFO_PRIVATE, &data);
    if (data)
        return data;

    data = PyMem_Calloc(sizeof *data, 1);
    curl_easy_setopt(curl, CURLOPT_PRIVATE, data);
    return data;
}

void
CurlWrap_free(CURL *curl)
{
    struct curl_private_data *data = NULL;
    curl_easy_getinfo(curl, CURLINFO_PRIVATE, &data);
    if (data) {
        curl_slist_free_all(data->headers);
        PyMem_Free(data);
    }

    curl_easy_cleanup(curl);
}

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
_Curl_set_body(CURL *curl, PyObject *bodyobj)
{
    if (!util_has_value(bodyobj)) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        return 0;
    }

    if (!util_ensure_type(bodyobj, &PyUnicode_Type, "body"))
        return -1;

    const char *body = PyUnicode_AsUTF8(bodyobj);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    return 0;
}

static int
_Curl_set_url(CURL *curl, PyObject *urlobj)
{
    if (!util_ensure_type(urlobj, &PyUnicode_Type, "url"))
        return -1;
    const char *url = PyUnicode_AsUTF8(urlobj);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    return 0;
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
_Curl_set_headers(CURL *curl, PyObject *headersobj)
{
    if (!util_has_value(headersobj))
        return 0;
    if (!util_ensure_mapping(headersobj, "headers"))
        return -1;

    struct curl_private_data *data = _Curl_get_private_or_new(curl);
    data->headers = NULL;

    /* Accept-Encoding needs to be handled as a special case
     * for curl to automatically decompress the response.
     */
    if (PyMapping_HasKeyString(headersobj, "accept-encoding")) {
        PyObject *encobj = PyMapping_GetItemString(headersobj, "accept-encoding");
        if (!util_ensure_type(encobj, &PyUnicode_Type, "accept-encoding"))
            goto fail;
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, PyUnicode_AsUTF8(encobj));
        PyMapping_DelItemString(headersobj, "accept-encoding");
        Py_DECREF(encobj);
    }

    PyObject *items = PyMapping_Items(headersobj);
    if (!items)
        goto fail;
    if (!util_ensure_type(items, &PyList_Type, NULL))
        goto fail;

    for (int i = 0; i < PyList_GET_SIZE(items); ++i) {
        const char *key, *val;
        if (!PyArg_ParseTuple(PyList_GET_ITEM(items, i), "ss", &key, &val))
            goto fail;

        char tmp[4096];
        snprintf(tmp, 4096, "%s: %s", key, val);

        struct curl_slist *tmplist = curl_slist_append(data->headers, tmp);
        if (!tmplist)
            goto fail;
        data->headers = tmplist;
    }

    Py_DECREF(items);
    return 0;

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, data->headers);

    return 0;

fail:
    curl_slist_free_all(data->headers);
    data->headers = NULL;
    return -1;
}

static int
_Curl_apply_PreparedRequest(CURL *curl, PyObject *prepreq)
{
    /* TODO: handle PreparedRequest.hooks */

    /* TODO: reset each call */

    if (_Curl_set_body(curl, PyObject_GetAttrString(prepreq, "body")) < 0)
        return -1;
    if (_Curl_set_headers(curl, PyObject_GetAttrString(prepreq, "headers")) < 0)
        return -1;
    if (_Curl_set_url(curl, PyObject_GetAttrString(prepreq, "url")) < 0)
        return -1;
    if (_Curl_set_method(curl, PyObject_GetAttrString(prepreq, "method")) < 0)
        return -1;

    return 0;
}

static void
_set_timeout_opts(CURL *curl, double timeout, double connecttimeout)
{
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)(timeout * 1000.0));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, (long)(connecttimeout * 1000.0));
}

static int
_Curl_set_timeout(CURL *curl, PyObject *timeout)
{
    if (!util_has_value(timeout)) {
        _set_timeout_opts(curl, 0.0, 0.0);
        return 0;
    }

    if (PyNumber_Check(timeout)) {
        _set_timeout_opts(curl, PyFloat_AsDouble(timeout), 0.0);
        return 0;
    }

    if (PyTuple_Check(timeout)) {
        double connto, readto;
        if (!PyArg_ParseTuple(timeout, "dd", &connto, &readto)) {
            PyErr_SetString(PyExc_ValueError, "Invalid timeout argument");
            return -1;
        }
        _set_timeout_opts(curl, readto, connto);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Invalid timeout argument");
    return -1;
}

static void
_set_verify_opts(CURL *curl, long verifypeer, const char *cainfo)
{
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifypeer);
    curl_easy_setopt(curl, CURLOPT_CAINFO, cainfo);
}

static int
_Curl_set_verify(CURL *curl, PyObject *verify)
{
    if (!util_has_value(verify)) {
        _set_verify_opts(curl, 1L, NULL);
        return 0;
    }

    if (PyBool_Check(verify)) {
        _set_verify_opts(curl, verify == Py_True, NULL);
        return 0;
    }

    if (PyUnicode_Check(verify)) {
        _set_verify_opts(curl, 1L, PyUnicode_AsUTF8(verify));
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Invalid verify argument");
    return -1;
}

static void
_set_cert_opts(CURL *curl, const char *cert, const char *key, const char *pw)
{
    curl_easy_setopt(curl, CURLOPT_SSLCERT, cert);
    curl_easy_setopt(curl, CURLOPT_SSLKEY, key);
    curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pw);
}

static int
_Curl_set_cert(CURL *curl, PyObject *certobj)
{
    if (!util_has_value(certobj)) {
        _set_cert_opts(curl, NULL, NULL, NULL);
        return 0;
    }

    if (PyUnicode_Check(certobj)) {
        _set_cert_opts(curl, PyUnicode_AsUTF8(certobj), NULL, NULL);
        return 0;
    }

    if (PyTuple_Check(certobj)) {
        const char *cert = NULL;
        const char *key = NULL;
        if (!PyArg_ParseTuple(certobj, "ss", &cert, &key)) {
            PyErr_SetString(PyExc_TypeError, "Invalid cert argument");
            return -1;
        }
        _set_cert_opts(curl, cert, key, NULL);
        return 0;
    }

    /* TODO: Extend API...
     * add CURLOPT_KEYPASSWD in 3-tuple? (or dict)
     * See also: https://github.com/psf/requests/issues/1573
     *
     * Allow PyBytes object to provide literal cert "blob"?
     * CURLOPT_SSLCERT_BLOB, CURLOPT_SSLKEY_BLOB
     *
     * Proxy certificates?
     *
     * cert types? SSL versions? OpenSSL engines? etc., etc.
     */

    PyErr_SetString(PyExc_TypeError, "Invalid cert argument");
    return -1;
}

static int
_Curl_set_proxy(CURL *curl, PyObject *url, PyObject *proxies)
{
    if (!util_has_value(proxies)) {
        curl_easy_setopt(curl, CURLOPT_PROXY, NULL);
        return 0;
    }

    if (!util_has_value(url)) {
        PyErr_SetString(PyExc_SystemError, "url argument must be set");
        return -1;
    }

    PyObject *proxy = RequestsMod_select_proxy(url, proxies);
    if (!proxy)
        return -1;
    if (proxy == Py_None) {
        curl_easy_setopt(curl, CURLOPT_PROXY, NULL);
        Py_DECREF(proxy);
        return 0;
    }

    if (!util_ensure_type(proxy, &PyUnicode_Type, "proxy"))
        return -1;

    curl_easy_setopt(curl, CURLOPT_PROXY, PyUnicode_AsUTF8(proxy));
    return 0;
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

PyObject *
CurlWrap_send(CURL *curl, struct CurlWrap_send_args *args)
{
    if (_Curl_apply_PreparedRequest(curl, args->request) < 0)
        return NULL;
    if (_Curl_set_timeout(curl, args->timeout) < 0)
        return NULL;
    if (_Curl_set_verify(curl, args->verify) < 0)
        return NULL;
    if (_Curl_set_cert(curl, args->cert) < 0)
        return NULL;
    if (_Curl_set_proxy(curl, PyObject_GetAttrString(args->request, "url"), args->proxies) < 0)
        return NULL;

    /* TODO: handle "stream" parameter (args->stream) */

    PyObject *headers = NULL;
    PyObject *body = NULL;

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

CURL *
CurlWrap_new(PyObject *kwargs)
{
    /* requests HTTPAdapter supported arguments:
     * - pool_connections
     * - pool_maxsize
     * - max_retries
     * - pool_block
     */

    /* CURLOPTs to possibly support:
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

    CURL *curl = curl_easy_init();
    if (!curl)
        goto fail;

    if (_Curl_set_http_version(curl, http_version) < 0)
        goto fail;
    if (_Curl_set_maxconnects(curl, maxconnects) < 0)
        goto fail;

    return curl;

fail:
    curl_easy_cleanup(curl);
    return NULL;
}

