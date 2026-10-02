$ErrorActionPreference = 'Stop'

# Thin-adapter audit (see verify-dry.sh).
$root = Split-Path -Parent $PSScriptRoot
$sourcePath = Join-Path $root 'src/asm_polycall.S'
$source = Get-Content -Raw $sourcePath
$found = Select-String -Path $sourcePath -Pattern 'fopen|CreateFile|sscanf|strtok|socket|connect'

if ($found) {
    $found | ForEach-Object { Write-Error $_.Line }
    throw 'asm-polycall must not parse configuration or implement runtime logic'
}

$required = @(
    'SYM(polycall_ffi_run_config)',
    'SYM(polycall_ffi_abi_version)',
    'SYM(polycall_ffi_version)',
    'SYM(polycall_peer_recv)',
    'movl $\run, %esi',
    'movl $\run, %edx',
    'mov w1, #\run'
)

foreach ($token in $required) {
    if (-not $source.Contains($token)) {
        throw "asm-polycall is missing required ABI forwarding token: $token"
    }
}

Write-Output 'asm-polycall thin-adapter check: PASS'
