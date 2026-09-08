/*
 * The C side of the Go contract: the entrypoint, the contract table and the
 * lifecycle, as the C contract has them, with the judgement delegated to Go
 * through the cgo export xmipGoJudge. cgo compiles this file into the shared
 * library beside the Go code.
 */

#include "xmip_module.h"
#include "_cgo_export.h"

#include <stdlib.h>
#include <string.h>

#define XMIP_GO_MESSAGE_MAX 256

typedef struct {
    char   *descriptor;
    size_t  descriptor_len;
} Contract;

typedef struct {
    XmipDiagnostic diagnostic;
    char           message[XMIP_GO_MESSAGE_MAX];
    char           error[XMIP_GO_MESSAGE_MAX];
} State;

static XmipStr str_of(const char *text, size_t len) {
    XmipStr s;
    s.ptr = (const uint8_t *)text;
    s.len = len;
    return s;
}

static XmipStatus configure(void *state, XmipStr toml) { (void)state; (void)toml; return XMIP_OK; }
static XmipStatus start(void *state) { (void)state; return XMIP_OK; }
static XmipStatus stop(void *state) { (void)state; return XMIP_OK; }

static XmipStatus load(void *state, XmipStr descriptor, void **out_contract) {
    Contract *contract;
    (void)state;
    if (out_contract == NULL) return XMIP_E_INVALID;
    contract = (Contract *)calloc(1, sizeof *contract);
    if (contract == NULL) return XMIP_E_CAPACITY;
    if (descriptor.len > 0) {
        contract->descriptor = (char *)malloc(descriptor.len);
        if (contract->descriptor == NULL) { free(contract); return XMIP_E_CAPACITY; }
        memcpy(contract->descriptor, descriptor.ptr, descriptor.len);
        contract->descriptor_len = descriptor.len;
    }
    *out_contract = contract;
    return XMIP_OK;
}

static void release(void *state, void *contract) {
    Contract *c = (Contract *)contract;
    (void)state;
    if (c == NULL) return;
    free(c->descriptor);
    free(c);
}

static XmipStatus validate(void *state, void *contract, const XmipReader *in,
                           const XmipDiagnostic **out, size_t *out_len) {
    State *s = (State *)state;
    Contract *c = (Contract *)contract;
    uint8_t *bytes = NULL;
    size_t len = 0, cap = 0;
    char *why;
    if (s == NULL || c == NULL || in == NULL || in->read == NULL || out == NULL || out_len == NULL) {
        return XMIP_E_INVALID;
    }
    *out = NULL;
    *out_len = 0;
    for (;;) {
        int64_t got;
        if (cap - len < 4096) {
            size_t grown = cap == 0 ? 8192 : cap * 2;
            uint8_t *bigger = (uint8_t *)realloc(bytes, grown);
            if (bigger == NULL) { free(bytes); return XMIP_E_CAPACITY; }
            bytes = bigger;
            cap = grown;
        }
        got = in->read(in->ctx, bytes + len, cap - len);
        if (got < 0) { free(bytes); return (XmipStatus)got; }
        if (got == 0) break;
        len += (size_t)got;
    }
    why = xmipGoJudge(c->descriptor, c->descriptor_len, bytes, len);
    free(bytes);
    if (why == NULL) return XMIP_OK;
    strncpy(s->message, why, XMIP_GO_MESSAGE_MAX - 1);
    s->message[XMIP_GO_MESSAGE_MAX - 1] = '\0';
    free(why);
    s->diagnostic.code = XMIP_E_CONTRACT;
    s->diagnostic.message = str_of(s->message, strlen(s->message));
    s->diagnostic.location = str_of("", 0);
    s->diagnostic.offset = UINT64_MAX;
    *out = &s->diagnostic;
    *out_len = 1;
    return XMIP_E_CONTRACT;
}

static XmipStatus implies(void *state, void *contract, XmipStr key, XmipStr *out) {
    const Contract *c = (const Contract *)contract;
    (void)state;
    if (c == NULL || out == NULL) return XMIP_E_INVALID;
    if (key.len == 10 && memcmp(key.ptr, "descriptor", 10) == 0 && c->descriptor_len > 0) {
        *out = str_of(c->descriptor, c->descriptor_len);
        return XMIP_OK;
    }
    return XMIP_E_NOT_FOUND;
}

static XmipStr last_error(void *state) {
    State *s = (State *)state;
    return str_of(s->error, strlen(s->error));
}

static void destroy(void *state) { free(state); }

static const XmipContractVtable VTABLE = {
    { 1u, 0u, configure, start, stop },
    load, release, validate, implies
};

XMIP_EXPORT XmipStatus xmip_create_module_v1(const XmipHost *host, XmipModule *out) {
    State *state;
    if (host == NULL || out == NULL) return XMIP_E_INVALID;
    if (host->abi_version != XMIP_ABI_VERSION) return XMIP_E_UNSUPPORTED;
    state = (State *)calloc(1, sizeof *state);
    if (state == NULL) return XMIP_E_CAPACITY;
    out->descriptor.abi_version = XMIP_ABI_VERSION;
    out->descriptor.provider = str_of("core", 4);
    out->descriptor.module = str_of("contract", 8);
    out->descriptor.standard = str_of("go", 2);
    out->descriptor.trait_major = 1u;
    out->descriptor.trait_minor = 0u;
    out->descriptor.module_major = 0u;
    out->descriptor.module_minor = 1u;
    out->descriptor.module_patch = 0u;
    out->state = state;
    out->vtable = &VTABLE;
    out->last_error = last_error;
    out->destroy = destroy;
    return XMIP_OK;
}
