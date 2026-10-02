#include "asm_polycall.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    const char *config = argc > 1 ? argv[1] : "asm-polycallrc";
    char version[32];
    char detail[512];
    int status;

    if (asm_polycall_abi_version() != POLYCALL_FFI_ABI_VERSION) {
        fprintf(stderr, "libpolycall speaks binding ABI %d, asm-polycall needs %d\n",
                asm_polycall_abi_version(), POLYCALL_FFI_ABI_VERSION);
        return 1;
    }
    asm_polycall_version(version, (int)sizeof version);
    printf("asm-polycall: libpolycall %s\n", version);

    status = asm_polycall_run_config(config);
    if (status != POLYCALL_OK) {
        asm_polycall_last_error(detail, sizeof detail);
        fprintf(stderr, "asm-polycall: %s: %s (%s)\n", config,
                asm_polycall_strerror(status), detail);
        return 1;
    }
    printf("asm-polycall: '%s' is valid for this build\n", config);
    return 0;
}
