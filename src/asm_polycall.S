/*
 * Thin assembly adapter for:
 *
 *   int polycall_ffi_run_config(const char *config_path, int run);
 *
 * The incoming config_path remains in the first C argument location. This
 * shim supplies 1 in the second argument location and forwards the return
 * value unchanged.
 */

#if defined(__x86_64__) && defined(_WIN32)

    .text
    .p2align 4, 0x90
    .globl asm_polycall_run_config
    .def asm_polycall_run_config; .scl 2; .type 32; .endef
asm_polycall_run_config:
    movl $1, %edx
    jmp polycall_ffi_run_config

#elif defined(__x86_64__) && defined(__APPLE__)

    .text
    .p2align 4, 0x90
    .globl _asm_polycall_run_config
_asm_polycall_run_config:
    movl $1, %esi
    jmp _polycall_ffi_run_config

#elif defined(__x86_64__)

    .text
    .p2align 4, 0x90
    .globl asm_polycall_run_config
    .type asm_polycall_run_config, @function
asm_polycall_run_config:
    movl $1, %esi
    jmp polycall_ffi_run_config@PLT
    .size asm_polycall_run_config, .-asm_polycall_run_config

#elif defined(__i386__) && defined(_WIN32)

    .text
    .p2align 2, 0x90
    .globl _asm_polycall_run_config
    .def _asm_polycall_run_config; .scl 2; .type 32; .endef
_asm_polycall_run_config:
    pushl %ebp
    movl %esp, %ebp
    andl $-16, %esp
    subl $8, %esp
    pushl $1
    pushl 8(%ebp)
    call _polycall_ffi_run_config
    leave
    ret

#elif defined(__i386__) && defined(__APPLE__)

    .text
    .p2align 2, 0x90
    .globl _asm_polycall_run_config
_asm_polycall_run_config:
    pushl %ebp
    movl %esp, %ebp
    andl $-16, %esp
    subl $8, %esp
    pushl $1
    pushl 8(%ebp)
    call _polycall_ffi_run_config
    leave
    ret

#elif defined(__i386__)

    .text
    .p2align 2, 0x90
    .globl asm_polycall_run_config
    .type asm_polycall_run_config, @function
asm_polycall_run_config:
    pushl %ebp
    movl %esp, %ebp
    andl $-16, %esp
    subl $8, %esp
    pushl $1
    pushl 8(%ebp)
    call polycall_ffi_run_config@PLT
    leave
    ret
    .size asm_polycall_run_config, .-asm_polycall_run_config

#elif defined(__aarch64__) && defined(__APPLE__)

    .text
    .p2align 2
    .globl _asm_polycall_run_config
_asm_polycall_run_config:
    mov w1, #1
    b _polycall_ffi_run_config

#elif defined(__aarch64__)

    .text
    .p2align 2
    .globl asm_polycall_run_config
    .type asm_polycall_run_config, %function
asm_polycall_run_config:
    mov w1, #1
    b polycall_ffi_run_config
    .size asm_polycall_run_config, .-asm_polycall_run_config

#elif defined(__arm__)

    .text
    .p2align 2
    .globl asm_polycall_run_config
    .type asm_polycall_run_config, %function
asm_polycall_run_config:
    mov r1, #1
    b polycall_ffi_run_config
    .size asm_polycall_run_config, .-asm_polycall_run_config

#else
#error "asm-polycall: unsupported target architecture"
#endif

#if defined(__ELF__) && (defined(__x86_64__) || defined(__i386__))
    .section .note.GNU-stack,"",@progbits
#elif defined(__ELF__) && (defined(__aarch64__) || defined(__arm__))
    .section .note.GNU-stack,"",%progbits
#endif
