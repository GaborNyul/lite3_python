#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lite3_context_api.h"

#define TRON_MODULE_VERSION "0.1.0"

typedef struct {
    PyObject_HEAD
    lite3_ctx *ctx;
} TronObject;

static PyObject *TronError;

static PyObject *tron_raise_errno(const char *msg)
{
    int err = errno;
    if (err != 0) {
        PyErr_Format(TronError, "%s: %s", msg, strerror(err));
    } else {
        PyErr_Format(TronError, "%s failed", msg);
    }
    return NULL;
}

static TronObject *tron_create_with_ctx(PyTypeObject *type, lite3_ctx *ctx)
{
    TronObject *self = (TronObject *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    self->ctx = ctx;
    return self;
}

static int tron_ctx_set_obj(lite3_ctx *ctx, size_t ofs, const char *key, size_t *out_ofs)
{
    int ret = _lite3_verify_obj_set(ctx->buf, &ctx->buflen, ofs, ctx->bufsz);
    if (ret < 0) {
        return ret;
    }

    errno = 0;
    while ((ret = lite3_set_obj_impl(
                    ctx->buf,
                    &ctx->buflen,
                    ofs,
                    ctx->bufsz,
                    key,
                    lite3_get_key_data(key),
                    out_ofs)) < 0) {
        if (errno == ENOBUFS && (lite3_ctx_grow_impl(ctx) == 0)) {
            continue;
        }
        return ret;
    }

    return ret;
}

static int tron_ctx_set_arr(lite3_ctx *ctx, size_t ofs, const char *key, size_t *out_ofs)
{
    int ret = _lite3_verify_obj_set(ctx->buf, &ctx->buflen, ofs, ctx->bufsz);
    if (ret < 0) {
        return ret;
    }

    errno = 0;
    while ((ret = lite3_set_arr_impl(
                    ctx->buf,
                    &ctx->buflen,
                    ofs,
                    ctx->bufsz,
                    key,
                    lite3_get_key_data(key),
                    out_ofs)) < 0) {
        if (errno == ENOBUFS && (lite3_ctx_grow_impl(ctx) == 0)) {
            continue;
        }
        return ret;
    }

    return ret;
}

static int tron_ctx_get(lite3_ctx *ctx, size_t ofs, const char *key, lite3_val **out)
{
    int ret = _lite3_verify_obj_get(ctx->buf, ctx->buflen, ofs);
    if (ret < 0) {
        return ret;
    }
    return lite3_get_impl(ctx->buf, ctx->buflen, ofs, key, lite3_get_key_data(key), out);
}

static int Tron_init(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *root = "object";
    Py_ssize_t bufsz = 0;
    static char *kwlist[] = {"root", "bufsz", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sn", kwlist, &root, &bufsz)) {
        return -1;
    }

    if (bufsz > 0) {
        self->ctx = lite3_ctx_create_with_size((size_t)bufsz);
    } else {
        self->ctx = lite3_ctx_create();
    }

    if (!self->ctx) {
        tron_raise_errno("lite3_ctx_create");
        return -1;
    }

    int ret = 0;
    if (strcmp(root, "object") == 0) {
        ret = lite3_ctx_init_obj(self->ctx);
    } else if (strcmp(root, "array") == 0) {
        ret = lite3_ctx_init_arr(self->ctx);
    } else {
        lite3_ctx_destroy(self->ctx);
        self->ctx = NULL;
        PyErr_SetString(PyExc_ValueError, "root must be 'object' or 'array'");
        return -1;
    }

    if (ret < 0) {
        tron_raise_errno("lite3_ctx_init");
        lite3_ctx_destroy(self->ctx);
        self->ctx = NULL;
        return -1;
    }

    return 0;
}

static void Tron_dealloc(TronObject *self)
{
    if (self->ctx) {
        lite3_ctx_destroy(self->ctx);
        self->ctx = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *Tron_init_obj(TronObject *self, PyObject *Py_UNUSED(args))
{
    if (lite3_ctx_init_obj(self->ctx) < 0) {
        return tron_raise_errno("lite3_ctx_init_obj");
    }
    Py_RETURN_NONE;
}

static PyObject *Tron_init_arr(TronObject *self, PyObject *Py_UNUSED(args))
{
    if (lite3_ctx_init_arr(self->ctx) < 0) {
        return tron_raise_errno("lite3_ctx_init_arr");
    }
    Py_RETURN_NONE;
}

static PyObject *Tron_set_null(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    if (lite3_ctx_set_null(self->ctx, (size_t)ofs, key) < 0) {
        return tron_raise_errno("lite3_ctx_set_null");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_set_bool(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    PyObject *value_obj = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "value", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|n", kwlist, &key, &value_obj, &ofs)) {
        return NULL;
    }

    int truth = PyObject_IsTrue(value_obj);
    if (truth < 0) {
        return NULL;
    }

    if (lite3_ctx_set_bool(self->ctx, (size_t)ofs, key, truth != 0) < 0) {
        return tron_raise_errno("lite3_ctx_set_bool");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_set_i64(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    PyObject *value_obj = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "value", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|n", kwlist, &key, &value_obj, &ofs)) {
        return NULL;
    }

    long long value = PyLong_AsLongLong(value_obj);
    if (PyErr_Occurred()) {
        return NULL;
    }

    if (lite3_ctx_set_i64(self->ctx, (size_t)ofs, key, (int64_t)value) < 0) {
        return tron_raise_errno("lite3_ctx_set_i64");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_set_f64(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    double value = 0.0;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "value", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sd|n", kwlist, &key, &value, &ofs)) {
        return NULL;
    }

    if (lite3_ctx_set_f64(self->ctx, (size_t)ofs, key, value) < 0) {
        return tron_raise_errno("lite3_ctx_set_f64");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_set_bytes(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    PyObject *value_obj = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "value", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|n", kwlist, &key, &value_obj, &ofs)) {
        return NULL;
    }

    Py_buffer view;
    if (PyObject_GetBuffer(value_obj, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }

    int ret = lite3_ctx_set_bytes(self->ctx, (size_t)ofs, key, (const unsigned char *)view.buf, (size_t)view.len);
    PyBuffer_Release(&view);
    if (ret < 0) {
        return tron_raise_errno("lite3_ctx_set_bytes");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_set_str(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    PyObject *value_obj = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "value", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|n", kwlist, &key, &value_obj, &ofs)) {
        return NULL;
    }

    Py_ssize_t value_len = 0;
    const char *value = PyUnicode_AsUTF8AndSize(value_obj, &value_len);
    if (!value) {
        return NULL;
    }

    if (lite3_ctx_set_str_n(self->ctx, (size_t)ofs, key, value, (size_t)value_len) < 0) {
        return tron_raise_errno("lite3_ctx_set_str_n");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_set_obj(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    size_t out_ofs = 0;
    if (tron_ctx_set_obj(self->ctx, (size_t)ofs, key, &out_ofs) < 0) {
        return tron_raise_errno("lite3_set_obj_impl");
    }

    return PyLong_FromSize_t(out_ofs);
}

static PyObject *Tron_set_arr(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    size_t out_ofs = 0;
    if (tron_ctx_set_arr(self->ctx, (size_t)ofs, key, &out_ofs) < 0) {
        return tron_raise_errno("lite3_set_arr_impl");
    }

    return PyLong_FromSize_t(out_ofs);
}

static PyObject *Tron_get_bool(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    bool value = false;
    if (lite3_ctx_get_bool(self->ctx, (size_t)ofs, key, &value) < 0) {
        return tron_raise_errno("lite3_ctx_get_bool");
    }

    if (value) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *Tron_get_i64(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    int64_t value = 0;
    if (lite3_ctx_get_i64(self->ctx, (size_t)ofs, key, &value) < 0) {
        return tron_raise_errno("lite3_ctx_get_i64");
    }

    return PyLong_FromLongLong((long long)value);
}

static PyObject *Tron_get_f64(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    double value = 0.0;
    if (lite3_ctx_get_f64(self->ctx, (size_t)ofs, key, &value) < 0) {
        return tron_raise_errno("lite3_ctx_get_f64");
    }

    return PyFloat_FromDouble(value);
}

static PyObject *Tron_get_bytes(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    lite3_bytes value;
    if (lite3_ctx_get_bytes(self->ctx, (size_t)ofs, key, &value) < 0) {
        return tron_raise_errno("lite3_ctx_get_bytes");
    }

    const unsigned char *bytes = LITE3_BYTES(self->ctx->buf, value);
    if (!bytes) {
        PyErr_SetString(TronError, "stale bytes reference");
        return NULL;
    }

    return PyBytes_FromStringAndSize((const char *)bytes, (Py_ssize_t)value.len);
}

static PyObject *Tron_get_str(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    lite3_str value;
    if (lite3_ctx_get_str(self->ctx, (size_t)ofs, key, &value) < 0) {
        return tron_raise_errno("lite3_ctx_get_str");
    }

    const char *str = LITE3_STR(self->ctx->buf, value);
    if (!str) {
        PyErr_SetString(TronError, "stale string reference");
        return NULL;
    }

    return PyUnicode_FromStringAndSize(str, (Py_ssize_t)value.len);
}

static PyObject *Tron_get_obj(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    size_t out_ofs = 0;
    if (lite3_ctx_get_obj(self->ctx, (size_t)ofs, key, &out_ofs) < 0) {
        return tron_raise_errno("lite3_ctx_get_obj");
    }

    return PyLong_FromSize_t(out_ofs);
}

static PyObject *Tron_get_arr(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    size_t out_ofs = 0;
    if (lite3_ctx_get_arr(self->ctx, (size_t)ofs, key, &out_ofs) < 0) {
        return tron_raise_errno("lite3_ctx_get_arr");
    }

    return PyLong_FromSize_t(out_ofs);
}

static const char *tron_type_name(enum lite3_type type)
{
    switch (type) {
    case LITE3_TYPE_NULL:
        return "null";
    case LITE3_TYPE_BOOL:
        return "bool";
    case LITE3_TYPE_I64:
        return "i64";
    case LITE3_TYPE_F64:
        return "f64";
    case LITE3_TYPE_BYTES:
        return "bytes";
    case LITE3_TYPE_STRING:
        return "string";
    case LITE3_TYPE_OBJECT:
        return "object";
    case LITE3_TYPE_ARRAY:
        return "array";
    default:
        return "unknown";
    }
}

static PyObject *Tron_get_type(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    enum lite3_type type = lite3_ctx_get_type(self->ctx, (size_t)ofs, key);
    if (type == LITE3_TYPE_INVALID) {
        return tron_raise_errno("lite3_ctx_get_type");
    }

    return PyUnicode_FromString(tron_type_name(type));
}

static PyObject *Tron_get(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    lite3_val *val = NULL;
    if (tron_ctx_get(self->ctx, (size_t)ofs, key, &val) < 0) {
        return tron_raise_errno("lite3_get_impl");
    }

    enum lite3_type type = lite3_val_type(val);
    switch (type) {
    case LITE3_TYPE_NULL:
        Py_RETURN_NONE;
    case LITE3_TYPE_BOOL:
        return PyBool_FromLong(lite3_val_bool(val) ? 1 : 0);
    case LITE3_TYPE_I64:
        return PyLong_FromLongLong((long long)lite3_val_i64(val));
    case LITE3_TYPE_F64:
        return PyFloat_FromDouble(lite3_val_f64(val));
    case LITE3_TYPE_STRING: {
        size_t len = 0;
        const char *str = lite3_val_str_n(val, &len);
        return PyUnicode_FromStringAndSize(str, (Py_ssize_t)len);
    }
    case LITE3_TYPE_BYTES: {
        size_t len = 0;
        const unsigned char *bytes = lite3_val_bytes(val, &len);
        return PyBytes_FromStringAndSize((const char *)bytes, (Py_ssize_t)len);
    }
    case LITE3_TYPE_OBJECT: {
        size_t out_ofs = 0;
        if (lite3_ctx_get_obj(self->ctx, (size_t)ofs, key, &out_ofs) < 0) {
            return tron_raise_errno("lite3_ctx_get_obj");
        }
        return PyLong_FromSize_t(out_ofs);
    }
    case LITE3_TYPE_ARRAY: {
        size_t out_ofs = 0;
        if (lite3_ctx_get_arr(self->ctx, (size_t)ofs, key, &out_ofs) < 0) {
            return tron_raise_errno("lite3_ctx_get_arr");
        }
        return PyLong_FromSize_t(out_ofs);
    }
    default:
        PyErr_SetString(TronError, "unknown value type");
        return NULL;
    }
}

static PyObject *Tron_exists(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *key = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"key", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|n", kwlist, &key, &ofs)) {
        return NULL;
    }

    bool exists = lite3_exists(self->ctx->buf, self->ctx->buflen, (size_t)ofs, key);
    if (exists) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *Tron_arr_append_null(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|n", kwlist, &ofs)) {
        return NULL;
    }

    if (lite3_ctx_arr_append_null(self->ctx, (size_t)ofs) < 0) {
        return tron_raise_errno("lite3_ctx_arr_append_null");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_arr_append_bool(TronObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *value_obj = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"value", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|n", kwlist, &value_obj, &ofs)) {
        return NULL;
    }

    int truth = PyObject_IsTrue(value_obj);
    if (truth < 0) {
        return NULL;
    }

    if (lite3_ctx_arr_append_bool(self->ctx, (size_t)ofs, truth != 0) < 0) {
        return tron_raise_errno("lite3_ctx_arr_append_bool");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_arr_append_i64(TronObject *self, PyObject *args, PyObject *kwargs)
{
    long long value = 0;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"value", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "L|n", kwlist, &value, &ofs)) {
        return NULL;
    }

    if (lite3_ctx_arr_append_i64(self->ctx, (size_t)ofs, (int64_t)value) < 0) {
        return tron_raise_errno("lite3_ctx_arr_append_i64");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_arr_append_f64(TronObject *self, PyObject *args, PyObject *kwargs)
{
    double value = 0.0;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"value", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d|n", kwlist, &value, &ofs)) {
        return NULL;
    }

    if (lite3_ctx_arr_append_f64(self->ctx, (size_t)ofs, value) < 0) {
        return tron_raise_errno("lite3_ctx_arr_append_f64");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_arr_append_bytes(TronObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *value_obj = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"value", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|n", kwlist, &value_obj, &ofs)) {
        return NULL;
    }

    Py_buffer view;
    if (PyObject_GetBuffer(value_obj, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }

    int ret = lite3_ctx_arr_append_bytes(self->ctx, (size_t)ofs, (const unsigned char *)view.buf, (size_t)view.len);
    PyBuffer_Release(&view);
    if (ret < 0) {
        return tron_raise_errno("lite3_ctx_arr_append_bytes");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_arr_append_str(TronObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *value_obj = NULL;
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"value", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|n", kwlist, &value_obj, &ofs)) {
        return NULL;
    }

    Py_ssize_t value_len = 0;
    const char *value = PyUnicode_AsUTF8AndSize(value_obj, &value_len);
    if (!value) {
        return NULL;
    }

    if (lite3_ctx_arr_append_str_n(self->ctx, (size_t)ofs, value, (size_t)value_len) < 0) {
        return tron_raise_errno("lite3_ctx_arr_append_str_n");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_arr_append_obj(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|n", kwlist, &ofs)) {
        return NULL;
    }

    size_t out_ofs = 0;
    if (lite3_ctx_arr_append_obj(self->ctx, (size_t)ofs, &out_ofs) < 0) {
        return tron_raise_errno("lite3_ctx_arr_append_obj");
    }

    return PyLong_FromSize_t(out_ofs);
}

static PyObject *Tron_arr_append_arr(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    static char *kwlist[] = {"ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|n", kwlist, &ofs)) {
        return NULL;
    }

    size_t out_ofs = 0;
    if (lite3_ctx_arr_append_arr(self->ctx, (size_t)ofs, &out_ofs) < 0) {
        return tron_raise_errno("lite3_ctx_arr_append_arr");
    }

    return PyLong_FromSize_t(out_ofs);
}

static PyObject *Tron_arr_get_bool(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    unsigned long index = 0;
    static char *kwlist[] = {"index", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k|n", kwlist, &index, &ofs)) {
        return NULL;
    }

    bool value = false;
    if (lite3_ctx_arr_get_bool(self->ctx, (size_t)ofs, (uint32_t)index, &value) < 0) {
        return tron_raise_errno("lite3_ctx_arr_get_bool");
    }

    return PyBool_FromLong(value ? 1 : 0);
}

static PyObject *Tron_arr_get_i64(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    unsigned long index = 0;
    static char *kwlist[] = {"index", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k|n", kwlist, &index, &ofs)) {
        return NULL;
    }

    int64_t value = 0;
    if (lite3_ctx_arr_get_i64(self->ctx, (size_t)ofs, (uint32_t)index, &value) < 0) {
        return tron_raise_errno("lite3_ctx_arr_get_i64");
    }

    return PyLong_FromLongLong((long long)value);
}

static PyObject *Tron_arr_get_f64(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    unsigned long index = 0;
    static char *kwlist[] = {"index", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k|n", kwlist, &index, &ofs)) {
        return NULL;
    }

    double value = 0.0;
    if (lite3_ctx_arr_get_f64(self->ctx, (size_t)ofs, (uint32_t)index, &value) < 0) {
        return tron_raise_errno("lite3_ctx_arr_get_f64");
    }

    return PyFloat_FromDouble(value);
}

static PyObject *Tron_arr_get_bytes(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    unsigned long index = 0;
    static char *kwlist[] = {"index", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k|n", kwlist, &index, &ofs)) {
        return NULL;
    }

    lite3_bytes value;
    if (lite3_ctx_arr_get_bytes(self->ctx, (size_t)ofs, (uint32_t)index, &value) < 0) {
        return tron_raise_errno("lite3_ctx_arr_get_bytes");
    }

    const unsigned char *bytes = LITE3_BYTES(self->ctx->buf, value);
    if (!bytes) {
        PyErr_SetString(TronError, "stale bytes reference");
        return NULL;
    }

    return PyBytes_FromStringAndSize((const char *)bytes, (Py_ssize_t)value.len);
}

static PyObject *Tron_arr_get_str(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    unsigned long index = 0;
    static char *kwlist[] = {"index", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k|n", kwlist, &index, &ofs)) {
        return NULL;
    }

    lite3_str value;
    if (lite3_ctx_arr_get_str(self->ctx, (size_t)ofs, (uint32_t)index, &value) < 0) {
        return tron_raise_errno("lite3_ctx_arr_get_str");
    }

    const char *str = LITE3_STR(self->ctx->buf, value);
    if (!str) {
        PyErr_SetString(TronError, "stale string reference");
        return NULL;
    }

    return PyUnicode_FromStringAndSize(str, (Py_ssize_t)value.len);
}

static PyObject *Tron_arr_get_obj(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    unsigned long index = 0;
    static char *kwlist[] = {"index", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k|n", kwlist, &index, &ofs)) {
        return NULL;
    }

    size_t out_ofs = 0;
    if (lite3_ctx_arr_get_obj(self->ctx, (size_t)ofs, (uint32_t)index, &out_ofs) < 0) {
        return tron_raise_errno("lite3_ctx_arr_get_obj");
    }

    return PyLong_FromSize_t(out_ofs);
}

static PyObject *Tron_arr_get_arr(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    unsigned long index = 0;
    static char *kwlist[] = {"index", "ofs", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k|n", kwlist, &index, &ofs)) {
        return NULL;
    }

    size_t out_ofs = 0;
    if (lite3_ctx_arr_get_arr(self->ctx, (size_t)ofs, (uint32_t)index, &out_ofs) < 0) {
        return tron_raise_errno("lite3_ctx_arr_get_arr");
    }

    return PyLong_FromSize_t(out_ofs);
}

static PyObject *Tron_to_bytes(TronObject *self, PyObject *Py_UNUSED(args))
{
    return PyBytes_FromStringAndSize((const char *)self->ctx->buf, (Py_ssize_t)self->ctx->buflen);
}

static PyObject *Tron_buflen(TronObject *self, PyObject *Py_UNUSED(args))
{
    return PyLong_FromSize_t(self->ctx->buflen);
}

static PyObject *Tron_bufsz(TronObject *self, PyObject *Py_UNUSED(args))
{
    return PyLong_FromSize_t(self->ctx->bufsz);
}

static PyObject *Tron_to_json(TronObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t ofs = 0;
    int pretty = 0;
    static char *kwlist[] = {"ofs", "pretty", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|np", kwlist, &ofs, &pretty)) {
        return NULL;
    }

    size_t out_len = 0;
    char *json = NULL;
    if (pretty) {
        json = lite3_ctx_json_enc_pretty(self->ctx, (size_t)ofs, &out_len);
    } else {
        json = lite3_ctx_json_enc(self->ctx, (size_t)ofs, &out_len);
    }

    if (!json) {
        return tron_raise_errno("lite3_ctx_json_enc");
    }

    PyObject *result = PyUnicode_FromStringAndSize(json, (Py_ssize_t)out_len);
    free(json);
    return result;
}

static PyObject *Tron_save(TronObject *self, PyObject *args, PyObject *kwargs)
{
    const char *path = NULL;
    static char *kwlist[] = {"path", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &path)) {
        return NULL;
    }

    FILE *fp = fopen(path, "wb");
    if (!fp) {
        return tron_raise_errno("fopen");
    }

    size_t written = fwrite(self->ctx->buf, 1, self->ctx->buflen, fp);
    int saved_errno = errno;
    fclose(fp);

    if (written != self->ctx->buflen) {
        errno = saved_errno;
        return tron_raise_errno("fwrite");
    }

    Py_RETURN_NONE;
}

static PyObject *Tron_debug_fill(TronObject *self, PyObject *args)
{
    unsigned int value = 0;
    if (!PyArg_ParseTuple(args, "I", &value)) {
        return NULL;
    }

    memset(self->ctx->buf, (int)(value & 0xFF), self->ctx->bufsz);
    Py_RETURN_NONE;
}

static PyObject *Tron_from_bytes(PyTypeObject *type, PyObject *args)
{
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "y*", &view)) {
        return NULL;
    }

    lite3_ctx *ctx = lite3_ctx_create_from_buf((const unsigned char *)view.buf, (size_t)view.len);
    PyBuffer_Release(&view);

    if (!ctx) {
        return tron_raise_errno("lite3_ctx_create_from_buf");
    }

    TronObject *self = tron_create_with_ctx(type, ctx);
    if (!self) {
        lite3_ctx_destroy(ctx);
        return NULL;
    }

    return (PyObject *)self;
}

static PyObject *Tron_from_json(PyTypeObject *type, PyObject *args)
{
    PyObject *json_obj = NULL;
    if (!PyArg_ParseTuple(args, "O", &json_obj)) {
        return NULL;
    }

    Py_ssize_t json_len = 0;
    const char *json_str = PyUnicode_AsUTF8AndSize(json_obj, &json_len);
    if (!json_str) {
        return NULL;
    }

    lite3_ctx *ctx = lite3_ctx_create();
    if (!ctx) {
        return tron_raise_errno("lite3_ctx_create");
    }

    if (lite3_ctx_json_dec(ctx, json_str, (size_t)json_len) < 0) {
        tron_raise_errno("lite3_ctx_json_dec");
        lite3_ctx_destroy(ctx);
        return NULL;
    }

    TronObject *self = tron_create_with_ctx(type, ctx);
    if (!self) {
        lite3_ctx_destroy(ctx);
        return NULL;
    }

    return (PyObject *)self;
}

static PyObject *Tron_from_json_file(PyTypeObject *type, PyObject *args)
{
    const char *path = NULL;
    if (!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }

    lite3_ctx *ctx = lite3_ctx_create();
    if (!ctx) {
        return tron_raise_errno("lite3_ctx_create");
    }

    if (lite3_ctx_json_dec_file(ctx, path) < 0) {
        tron_raise_errno("lite3_ctx_json_dec_file");
        lite3_ctx_destroy(ctx);
        return NULL;
    }

    TronObject *self = tron_create_with_ctx(type, ctx);
    if (!self) {
        lite3_ctx_destroy(ctx);
        return NULL;
    }

    return (PyObject *)self;
}

static PyObject *Tron_from_file(PyTypeObject *type, PyObject *args)
{
    const char *path = NULL;
    if (!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return tron_raise_errno("fopen");
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        int saved_errno = errno;
        fclose(fp);
        errno = saved_errno;
        return tron_raise_errno("fseek");
    }

    long size = ftell(fp);
    if (size < 0) {
        int saved_errno = errno;
        fclose(fp);
        errno = saved_errno;
        return tron_raise_errno("ftell");
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        int saved_errno = errno;
        fclose(fp);
        errno = saved_errno;
        return tron_raise_errno("fseek");
    }

    if (size == 0) {
        fclose(fp);
        PyErr_SetString(TronError, "file is empty");
        return NULL;
    }

    unsigned char *buf = (unsigned char *)malloc((size_t)size);
    if (!buf) {
        fclose(fp);
        return tron_raise_errno("malloc");
    }

    size_t read = fread(buf, 1, (size_t)size, fp);
    int saved_errno = errno;
    fclose(fp);

    if (read != (size_t)size) {
        free(buf);
        errno = saved_errno;
        return tron_raise_errno("fread");
    }

    lite3_ctx *ctx = lite3_ctx_create_from_buf(buf, (size_t)size);
    free(buf);

    if (!ctx) {
        return tron_raise_errno("lite3_ctx_create_from_buf");
    }

    TronObject *self = tron_create_with_ctx(type, ctx);
    if (!self) {
        lite3_ctx_destroy(ctx);
        return NULL;
    }

    return (PyObject *)self;
}

static PyMethodDef Tron_methods[] = {
    {"init_obj", (PyCFunction)Tron_init_obj, METH_NOARGS, "Initialize root as object."},
    {"init_arr", (PyCFunction)Tron_init_arr, METH_NOARGS, "Initialize root as array."},
    {"set_null", (PyCFunction)Tron_set_null, METH_VARARGS | METH_KEYWORDS, "Set null value in object."},
    {"set_bool", (PyCFunction)Tron_set_bool, METH_VARARGS | METH_KEYWORDS, "Set boolean value in object."},
    {"set_i64", (PyCFunction)Tron_set_i64, METH_VARARGS | METH_KEYWORDS, "Set int64 value in object."},
    {"set_f64", (PyCFunction)Tron_set_f64, METH_VARARGS | METH_KEYWORDS, "Set float value in object."},
    {"set_bytes", (PyCFunction)Tron_set_bytes, METH_VARARGS | METH_KEYWORDS, "Set bytes value in object."},
    {"set_str", (PyCFunction)Tron_set_str, METH_VARARGS | METH_KEYWORDS, "Set string value in object."},
    {"set_obj", (PyCFunction)Tron_set_obj, METH_VARARGS | METH_KEYWORDS, "Set nested object and return its offset."},
    {"set_arr", (PyCFunction)Tron_set_arr, METH_VARARGS | METH_KEYWORDS, "Set nested array and return its offset."},
    {"delete", (PyCFunction)Tron_set_null, METH_VARARGS | METH_KEYWORDS, "Delete a key by setting null."},
    {"get_bool", (PyCFunction)Tron_get_bool, METH_VARARGS | METH_KEYWORDS, "Get boolean value by key."},
    {"get_i64", (PyCFunction)Tron_get_i64, METH_VARARGS | METH_KEYWORDS, "Get int64 value by key."},
    {"get_f64", (PyCFunction)Tron_get_f64, METH_VARARGS | METH_KEYWORDS, "Get float value by key."},
    {"get_bytes", (PyCFunction)Tron_get_bytes, METH_VARARGS | METH_KEYWORDS, "Get bytes value by key."},
    {"get_str", (PyCFunction)Tron_get_str, METH_VARARGS | METH_KEYWORDS, "Get string value by key."},
    {"get_obj", (PyCFunction)Tron_get_obj, METH_VARARGS | METH_KEYWORDS, "Get nested object offset by key."},
    {"get_arr", (PyCFunction)Tron_get_arr, METH_VARARGS | METH_KEYWORDS, "Get nested array offset by key."},
    {"get_type", (PyCFunction)Tron_get_type, METH_VARARGS | METH_KEYWORDS, "Get value type by key."},
    {"get", (PyCFunction)Tron_get, METH_VARARGS | METH_KEYWORDS, "Get value by key and return a Python type."},
    {"exists", (PyCFunction)Tron_exists, METH_VARARGS | METH_KEYWORDS, "Check if a key exists."},
    {"arr_append_null", (PyCFunction)Tron_arr_append_null, METH_VARARGS | METH_KEYWORDS, "Append null to array."},
    {"arr_append_bool", (PyCFunction)Tron_arr_append_bool, METH_VARARGS | METH_KEYWORDS, "Append boolean to array."},
    {"arr_append_i64", (PyCFunction)Tron_arr_append_i64, METH_VARARGS | METH_KEYWORDS, "Append int64 to array."},
    {"arr_append_f64", (PyCFunction)Tron_arr_append_f64, METH_VARARGS | METH_KEYWORDS, "Append float to array."},
    {"arr_append_bytes", (PyCFunction)Tron_arr_append_bytes, METH_VARARGS | METH_KEYWORDS, "Append bytes to array."},
    {"arr_append_str", (PyCFunction)Tron_arr_append_str, METH_VARARGS | METH_KEYWORDS, "Append string to array."},
    {"arr_append_obj", (PyCFunction)Tron_arr_append_obj, METH_VARARGS | METH_KEYWORDS, "Append object to array and return its offset."},
    {"arr_append_arr", (PyCFunction)Tron_arr_append_arr, METH_VARARGS | METH_KEYWORDS, "Append array to array and return its offset."},
    {"arr_get_bool", (PyCFunction)Tron_arr_get_bool, METH_VARARGS | METH_KEYWORDS, "Get boolean from array by index."},
    {"arr_get_i64", (PyCFunction)Tron_arr_get_i64, METH_VARARGS | METH_KEYWORDS, "Get int64 from array by index."},
    {"arr_get_f64", (PyCFunction)Tron_arr_get_f64, METH_VARARGS | METH_KEYWORDS, "Get float from array by index."},
    {"arr_get_bytes", (PyCFunction)Tron_arr_get_bytes, METH_VARARGS | METH_KEYWORDS, "Get bytes from array by index."},
    {"arr_get_str", (PyCFunction)Tron_arr_get_str, METH_VARARGS | METH_KEYWORDS, "Get string from array by index."},
    {"arr_get_obj", (PyCFunction)Tron_arr_get_obj, METH_VARARGS | METH_KEYWORDS, "Get object offset from array by index."},
    {"arr_get_arr", (PyCFunction)Tron_arr_get_arr, METH_VARARGS | METH_KEYWORDS, "Get array offset from array by index."},
    {"to_bytes", (PyCFunction)Tron_to_bytes, METH_NOARGS, "Return raw buffer bytes."},
    {"buflen", (PyCFunction)Tron_buflen, METH_NOARGS, "Return used buffer length."},
    {"bufsz", (PyCFunction)Tron_bufsz, METH_NOARGS, "Return total buffer size."},
    {"to_json", (PyCFunction)Tron_to_json, METH_VARARGS | METH_KEYWORDS, "Convert to JSON string."},
    {"save", (PyCFunction)Tron_save, METH_VARARGS | METH_KEYWORDS, "Save raw buffer to file."},
    {"debug_fill", (PyCFunction)Tron_debug_fill, METH_VARARGS, "Fill buffer with a byte value (testing)."},
    {"from_bytes", (PyCFunction)Tron_from_bytes, METH_VARARGS | METH_CLASS, "Create Tron from raw bytes."},
    {"from_json", (PyCFunction)Tron_from_json, METH_VARARGS | METH_CLASS, "Create Tron from JSON string."},
    {"from_json_file", (PyCFunction)Tron_from_json_file, METH_VARARGS | METH_CLASS, "Create Tron from JSON file."},
    {"from_file", (PyCFunction)Tron_from_file, METH_VARARGS | METH_CLASS, "Create Tron from raw buffer file."},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject TronType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "tron.Tron",
    .tp_basicsize = sizeof(TronObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "TRON (Lite3) context wrapper",
    .tp_methods = Tron_methods,
    .tp_init = (initproc)Tron_init,
    .tp_new = PyType_GenericNew,
    .tp_dealloc = (destructor)Tron_dealloc,
};

static PyModuleDef tronmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "tron._tron",
    .m_doc = "Python bindings for TRON (Lite3)",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit__tron(void)
{
    if (PyType_Ready(&TronType) < 0) {
        return NULL;
    }

    PyObject *module = PyModule_Create(&tronmodule);
    if (!module) {
        return NULL;
    }

    TronError = PyErr_NewException("tron.TronError", NULL, NULL);
    if (!TronError) {
        Py_DECREF(module);
        return NULL;
    }

    Py_INCREF(TronError);
    if (PyModule_AddObject(module, "TronError", TronError) < 0) {
        Py_DECREF(TronError);
        Py_DECREF(module);
        return NULL;
    }

    Py_INCREF(&TronType);
    if (PyModule_AddObject(module, "Tron", (PyObject *)&TronType) < 0) {
        Py_DECREF(&TronType);
        Py_DECREF(module);
        return NULL;
    }

    PyModule_AddStringConstant(module, "__version__", TRON_MODULE_VERSION);
    PyModule_AddIntConstant(module, "LITE3_NODE_SIZE", (long)LITE3_NODE_SIZE);
    PyModule_AddIntConstant(module, "LITE3_NODE_ALIGNMENT", (long)LITE3_NODE_ALIGNMENT);
    PyModule_AddIntConstant(module, "LITE3_ZERO_MEM_8", (long)LITE3_ZERO_MEM_8);
    PyModule_AddIntConstant(module, "DJB2_HASH_SEED", (long)LITE3_DJB2_HASH_SEED);

    return module;
}
