package main

/*
#include "xmip_module.h"
#include <string.h>

XmipStatus xmip_create_module_v1(const XmipHost *host, XmipModule *out);

typedef struct { const uint8_t *bytes; size_t len; size_t at; } Source;

static int64_t read_source(void *ctx, uint8_t *buf, size_t len) {
    Source *s = (Source *)ctx;
    size_t left = s->len - s->at;
    size_t n = left < len ? left : len;
    if (n > 3) n = 3;
    memcpy(buf, s->bytes + s->at, n);
    s->at += n;
    return (int64_t)n;
}

// The probe in C, driven from Go's test: it exercises the table the way a
// host would. Returns the number of the first check that failed, 0 for none.
static int probe(void) {
    XmipHost host = { XMIP_ABI_VERSION, NULL, NULL, NULL, NULL };
    XmipHost foreign = { 99u, NULL, NULL, NULL, NULL };
    XmipModule module;
    const XmipContractVtable *table;
    void *contract = NULL;
    const XmipDiagnostic *diagnostics = NULL;
    size_t count = 7;
    XmipStr descriptor = { (const uint8_t *)"any", 3 };
    XmipStr key = { (const uint8_t *)"descriptor", 10 };
    XmipStr implied = { NULL, 0 };
    Source source = { (const uint8_t *)"xmip round-trip", 15, 0 };
    XmipReader reader = { &source, read_source };

    memset(&module, 0, sizeof module);
    if (xmip_create_module_v1(&foreign, &module) != XMIP_E_UNSUPPORTED) return 1;
    if (module.vtable != NULL) return 2;
    if (xmip_create_module_v1(&host, &module) != XMIP_OK) return 3;
    if (module.descriptor.standard.len != 2 || memcmp(module.descriptor.standard.ptr, "go", 2)) return 4;
    table = (const XmipContractVtable *)module.vtable;
    if (table->header.start(module.state) != XMIP_OK) return 5;
    if (table->load(module.state, descriptor, &contract) != XMIP_OK || !contract) return 6;
    if (table->validate(module.state, contract, &reader, &diagnostics, &count) != XMIP_OK) return 7;
    if (count != 0 || source.at != source.len) return 8;
    if (table->implies(module.state, contract, key, &implied) != XMIP_OK || implied.len != 3) return 9;
    key.ptr = (const uint8_t *)"nothing"; key.len = 7;
    if (table->implies(module.state, contract, key, &implied) != XMIP_E_NOT_FOUND) return 10;
    table->release(module.state, contract);
    if (table->header.stop(module.state) != XMIP_OK) return 11;
    if (module.last_error(module.state).len != 0) return 12;
    module.destroy(module.state);
    return 0;
}
*/
import "C"

// Probe drives the module through the C header's own types and returns the
// number of the first check that failed, or 0.
func Probe() int {
	return int(C.probe())
}
