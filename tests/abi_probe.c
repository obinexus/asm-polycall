/*
 * asm-polycall loader probe, driven by tests/loader-errors.sh:
 *
 *   abi_probe expect-ok        the shims reach a library speaking ABI 1
 *   abi_probe expect-mismatch  the library reports ABI 2: the probe sees it
 *                              through asm_polycall_abi_version() and refuses,
 *                              as the README tells callers to do
 */
#include "asm_polycall.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    const int mismatch = argc > 1 && strcmp(argv[1], "expect-mismatch") == 0;
    const int abi = asm_polycall_abi_version();
    int st;
    printf("abi_probe: asm_polycall_abi_version() = %d\n", abi);
    if (abi != POLYCALL_FFI_ABI_VERSION) {
        printf("abi_probe: refusing: libpolycall speaks binding ABI %d, asm-polycall needs %d\n", abi,
               POLYCALL_FFI_ABI_VERSION);
        return mismatch && abi == 2 ? 0 : 1;
    }
    st = asm_polycall_run_config("");
    printf("abi_probe: asm_polycall_run_config(\"\") = %d (%s)\n", st, asm_polycall_strerror(st));
    return !mismatch && st == POLYCALL_E_INVALID_ARGUMENT ? 0 : 1;
}
