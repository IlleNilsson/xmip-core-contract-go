// The Go content contract - a technology of xmip-core-contract, in Go.
//
// ADR-0042 decision 3: a contract may be authored in any declared language
// over the C ABI. ADR-0012: the header is the boundary. Go cannot lay out a C
// vtable of its own functions, so bridge.c holds the entrypoint, the table and
// the lifecycle exactly as the C contract does, and calls into Go for the one
// thing that is the contract's: judging the bytes. Built with
// -buildmode=c-shared, the result is one loadable library like every other.
//
// What it claims: well-formedness is bytes (ADR-0042 decision 1). A Go
// contract with a real standard replaces Judge and nothing else.
package main

/*
#include <stdint.h>
#include <stdlib.h>
*/
import "C"

import "unsafe"

// Judge a whole stream against the bound descriptor. Empty holds; anything
// else is the message the diagnostic carries. The identity contract holds
// everything.
func Judge(descriptor string, bytes []byte) string {
	_ = descriptor
	_ = bytes
	return ""
}

//export xmipGoJudge
func xmipGoJudge(descriptor *C.char, descriptorLen C.size_t, bytes *C.uint8_t, bytesLen C.size_t) *C.char {
	var content []byte
	if bytesLen > 0 {
		content = C.GoBytes(unsafe.Pointer(bytes), C.int(bytesLen))
	}
	var bound string
	if descriptorLen > 0 {
		bound = C.GoStringN(descriptor, C.int(descriptorLen))
	}
	why := Judge(bound, content)
	if why == "" {
		return nil
	}
	// Freed by the C side, which copies it into the borrowed diagnostic.
	return C.CString(why)
}

func main() {}
