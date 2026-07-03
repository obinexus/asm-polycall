#!/usr/bin/env sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

if grep -E -n 'fopen|CreateFile|sscanf|strtok|socket|connect' \
    "$root/src/asm_polycall.S"; then
    echo "asm-polycall must not parse configuration or implement runtime logic" >&2
    exit 1
fi

grep -F -q 'polycall_ffi_run_config' "$root/src/asm_polycall.S"
grep -F -q 'pushl $1' "$root/src/asm_polycall.S"
grep -F -q 'movl $1, %esi' "$root/src/asm_polycall.S"
grep -F -q 'movl $1, %edx' "$root/src/asm_polycall.S"
grep -F -q 'mov w1, #1' "$root/src/asm_polycall.S"

echo "asm-polycall thin-adapter check: PASS"
