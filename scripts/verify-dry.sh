#!/usr/bin/env sh
# Thin-adapter audit: the assembly shims only forward to real binding-ABI
# symbols of <polycall.h>; no parsing, sockets or runtime logic.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
src="$root/src/asm_polycall.S"

if grep -E -n 'fopen|CreateFile|sscanf|strtok|socket|connect' "$src"; then
    echo "asm-polycall must not parse configuration or implement runtime logic" >&2
    exit 1
fi
if grep -R -n 'polycall_ffi\.h' "$root/src" "$root/include" "$root/Makefile"; then
    echo "asm-polycall must use the real <polycall.h>, not a generated stub" >&2
    exit 1
fi

# every shim targets a symbol declared by the core's binding ABI
for sym in polycall_ffi_run_config polycall_ffi_abi_version polycall_ffi_version \
           polycall_strerror polycall_last_error polycall_ffi_describe polycall_call \
           polycall_peer_open polycall_peer_close polycall_peer_endpoint polycall_peer_node_id \
           polycall_peer_register polycall_peer_unregister polycall_peer_list polycall_peer_ping \
           polycall_peer_send polycall_peer_recv polycall_peer_cancel polycall_peer_health; do
    grep -F -q "SYM($sym)" "$src"
done
grep -F -q 'movl $\run, %esi' "$src"
grep -F -q 'movl $\run, %edx' "$src"
grep -F -q 'mov w1, #\run' "$src"
grep -F -q '#include <polycall.h>' "$root/include/asm_polycall.h"

echo "asm-polycall thin-adapter check: PASS"
