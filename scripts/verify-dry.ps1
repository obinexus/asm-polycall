$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$sourcePath = Join-Path $root 'src/asm_polycall.S'
$source = Get-Content -Raw $sourcePath
$forbidden = 'fopen|CreateFile|sscanf|strtok|socket|connect'
$matches = Select-String -Path $sourcePath -Pattern $forbidden

if ($matches) {
    $matches | ForEach-Object { Write-Error $_.Line }
    throw 'asm-polycall must not parse configuration or implement runtime logic'
}

$required = @(
    'polycall_ffi_run_config',
    'pushl $1',
    'movl $1, %esi',
    'movl $1, %edx',
    'mov w1, #1'
)

foreach ($token in $required) {
    if (-not $source.Contains($token)) {
        throw "asm-polycall is missing required ABI forwarding token: $token"
    }
}

Write-Output 'asm-polycall thin-adapter check: PASS'
