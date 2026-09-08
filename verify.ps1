<#
    .SYNOPSIS
    Vets, tests and builds the Go contract module as a C shared library.

    .DESCRIPTION
    The gate xgit runs (Test-XmipSelfVerifyingModule). cgo needs a C compiler
    and uses zig cc, so this repository needs both the `go` and the `c`
    prerequisites. The ABI header comes from xmip-core-abi, found in the estate
    when mounted there and through XMIP_ABI_INCLUDE otherwise.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot

function Find-Tool([string] $Name, [string[]] $Known) {
    $found = Get-Command $Name -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    foreach ($candidate in $Known) { if (Test-Path -LiteralPath $candidate) { return $candidate } }
    return $null
}

[string] $go = Find-Tool 'go' @('C:\Program Files\Go\bin\go.exe', '/usr/local/go/bin/go', '/opt/homebrew/bin/go')
if (-not $go) { Write-Host 'FAILED. go is not installed; prerequisite.toml declares it.'; exit 2 }
if (-not (Get-Command zig -ErrorAction SilentlyContinue)) {
    Write-Host 'FAILED. zig is not installed; cgo needs it as the C compiler (prerequisite c).'
    exit 2
}

[string] $include = $env:XMIP_ABI_INCLUDE
if (-not $include) { $include = Join-Path $PSScriptRoot '..' '..' '..' 'foundation' 'abi' 'include' }
if (-not (Test-Path -LiteralPath (Join-Path $include 'xmip_module.h'))) {
    Write-Host "FAILED. xmip_module.h not found under $include; set XMIP_ABI_INCLUDE."
    exit 2
}

$env:CGO_ENABLED = '1'
$env:CC = 'zig cc'
# zig cc enables the UBSan runtime by default and cgo ships none of it.
$env:CGO_CFLAGS = "-O2 -fno-sanitize=undefined -I$((Resolve-Path $include).Path)"
$env:CGO_LDFLAGS = '-fno-sanitize=undefined'
New-Item -ItemType Directory -Force -Path build | Out-Null
[string] $library = if ($IsWindows) { 'xmip_core_contract_go.dll' }
    elseif ($IsMacOS) { 'libxmip_core_contract_go.dylib' } else { 'libxmip_core_contract_go.so' }

Write-Host '   go vet'
& $go vet ./...
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host '   go test'
& $go test ./...
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "   go build -buildmode=c-shared -> build/$library"
& $go build -buildmode=c-shared -o (Join-Path build $library) .
exit $LASTEXITCODE
