package main

import "testing"

func TestTheModuleDrivesThroughTheContractTable(t *testing.T) {
	if failed := Probe(); failed != 0 {
		t.Fatalf("probe check %d failed", failed)
	}
}

func TestTheIdentityContractHoldsEverything(t *testing.T) {
	if why := Judge("any", []byte{0, 1, 0xff}); why != "" {
		t.Fatalf("held nothing: %q", why)
	}
}
