#include "asm_polycall.h"

#include <stdio.h>

int main(void) {
    int status = asm_polycall_run_config("asm-polycallrc");

    if (status != 0) {
        fprintf(stderr, "libpolycall failed with status %d\n", status);
        return status;
    }

    puts("asm-polycall: configuration started successfully");
    return 0;
}
