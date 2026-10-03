/*
 * asm-polycall: GNU-assembler shims over the Polycall binding ABI v1
 * (<polycall.h>, polycall >= 1.1.0).
 *
 *   asm_polycall_run_config(path)       -> polycall_ffi_run_config(path, 1)
 *   asm_polycall_validate_config(path)  -> polycall_ffi_run_config(path, 0)
 *   asm_polycall_<fn>(args...)          -> polycall_<fn>(args...)   (tail call)
 *
 * Every shim is a tail call: the caller's arguments stay exactly where the
 * platform C ABI put them (registers and, beyond them, the caller's stack),
 * so the core sees the original call. The two run_config shims additionally
 * write the `run` constant into the second-argument slot. No shim touches
 * memory, the stack pointer or a callee-saved register, so none needs
 * unwind information.
 *
 * Targets (branch selected by the C preprocessor):
 *   x86-64 System V (ELF, Mach-O)    runtime-tested on Linux
 *   x86-64 Windows (COFF, MinGW)     runtime-tested with MSYS2 UCRT64
 *   AArch64 ELF                      runtime-tested on Linux under qemu-user
 *   AArch64 Mach-O / Windows         assembles; not runtime-tested here
 *   x86-32 ELF / Windows             runtime-tested (Linux i386, Windows WoW64)
 *   x86-32 Mach-O                    assembles; not runtime-tested here
 *   AArch32 ELF                      runtime-tested on Linux armhf under qemu-user
 * MSVC's cl/ml64 cannot assemble GNU .S files: on Windows build with MinGW
 * gcc or clang.
 */

#if defined(__APPLE__) || (defined(_WIN32) && defined(__i386__))
#define SYM(x) _##x
#else
#define SYM(x) x
#endif

/* ------------------------------------------------------------------------ */
#if defined(__x86_64__) && defined(_WIN32)
/* Microsoft x64: args RCX, RDX, R8, R9, then caller stack (+shadow space). */
    .text
.macro PC_FUNC name
    .globl \name
    .def \name; .scl 2; .type 32; .endef
    .p2align 4, 0x90
\name:
.endm
.macro PC_END name
.endm
.macro PC_TAIL name, target
    PC_FUNC \name
    jmp \target
.endm
.macro PC_RUN name, target, run
    PC_FUNC \name
    movl $\run, %edx
    jmp \target
.endm

/* ------------------------------------------------------------------------ */
#elif defined(__x86_64__) && defined(__APPLE__)
    .text
.macro PC_TAIL name, target
    .globl \name
    .p2align 4, 0x90
\name:
    jmp \target
.endm
.macro PC_RUN name, target, run
    .globl \name
    .p2align 4, 0x90
\name:
    movl $\run, %esi
    jmp \target
.endm

/* ------------------------------------------------------------------------ */
#elif defined(__x86_64__)
/* System V AMD64: args RDI, RSI, RDX, RCX, R8, R9, then caller stack. */
    .text
.macro PC_TAIL name, target
    .globl \name
    .type \name, @function
    .p2align 4, 0x90
\name:
    jmp \target@PLT
    .size \name, .-\name
.endm
.macro PC_RUN name, target, run
    .globl \name
    .type \name, @function
    .p2align 4, 0x90
\name:
    movl $\run, %esi
    jmp \target@PLT
    .size \name, .-\name
.endm

/* ------------------------------------------------------------------------ */
#elif defined(__i386__) && (defined(_WIN32) || defined(__APPLE__))
/* cdecl: every argument is on the caller's stack; a jmp leaves it intact. */
    .text
.macro PC_TAIL name, target
    .globl \name
    .p2align 4, 0x90
\name:
    jmp \target
.endm
.macro PC_RUN name, target, run
    .globl \name
    .p2align 4, 0x90
\name:
    pushl %ebp
    movl %esp, %ebp
    andl $-16, %esp
    subl $8, %esp
    pushl $\run
    pushl 8(%ebp)
    call \target
    leave
    ret
.endm

/* ------------------------------------------------------------------------ */
#elif defined(__i386__)
/* i386 ELF: position-independent, so go through the GOT explicitly (a PLT
 * call from PIC code would need %ebx = GOT, which a tail call cannot set). */
    .text
.macro PC_TAIL name, target
    .globl \name
    .type \name, @function
    .p2align 4, 0x90
\name:
    call 1f
1:  popl %ecx
    addl $_GLOBAL_OFFSET_TABLE_+(.-1b), %ecx
    jmp *\target@GOT(%ecx)
    .size \name, .-\name
.endm
.macro PC_RUN name, target, run
    .globl \name
    .type \name, @function
    .p2align 4, 0x90
\name:
    pushl %ebp
    movl %esp, %ebp
    andl $-16, %esp
    subl $8, %esp
    call 1f
1:  popl %ecx
    addl $_GLOBAL_OFFSET_TABLE_+(.-1b), %ecx
    pushl $\run
    pushl 8(%ebp)
    call *\target@GOT(%ecx)
    leave
    ret
    .size \name, .-\name
.endm

/* ------------------------------------------------------------------------ */
#elif defined(__aarch64__) && defined(__APPLE__)
    .text
.macro PC_TAIL name, target
    .globl \name
    .p2align 2
\name:
    b \target
.endm
.macro PC_RUN name, target, run
    .globl \name
    .p2align 2
\name:
    mov w1, #\run
    b \target
.endm

/* ------------------------------------------------------------------------ */
#elif defined(__aarch64__) && defined(_WIN32)
/* Windows on ARM64 uses AAPCS64 argument registers (X0-X7, then stack) for
 * these non-variadic functions, but COFF object files: no ELF .type/.size
 * directives. The branch reaches the import thunk of polycall.dll. */
    .text
.macro PC_TAIL name, target
    .globl \name
    .def \name; .scl 2; .type 32; .endef
    .p2align 2
\name:
    b \target
.endm
.macro PC_RUN name, target, run
    .globl \name
    .def \name; .scl 2; .type 32; .endef
    .p2align 2
\name:
    mov w1, #\run
    b \target
.endm

/* ------------------------------------------------------------------------ */
#elif defined(__aarch64__)
/* AAPCS64 ELF: args X0-X7, then caller stack. */
    .text
.macro PC_TAIL name, target
    .globl \name
    .type \name, %function
    .p2align 2
\name:
    b \target
    .size \name, .-\name
.endm
.macro PC_RUN name, target, run
    .globl \name
    .type \name, %function
    .p2align 2
\name:
    mov w1, #\run
    b \target
    .size \name, .-\name
.endm

/* ------------------------------------------------------------------------ */
#elif defined(__arm__)
/* AAPCS32: args R0-R3, then caller stack. */
    .text
.macro PC_TAIL name, target
    .globl \name
    .type \name, %function
    .p2align 2
\name:
    b \target
    .size \name, .-\name
.endm
.macro PC_RUN name, target, run
    .globl \name
    .type \name, %function
    .p2align 2
\name:
    mov r1, #\run
    b \target
    .size \name, .-\name
.endm

#else
#error "asm-polycall: unsupported target architecture"
#endif

/* ---- the shims (one per line) ------------------------------------------- */

    PC_RUN  SYM(asm_polycall_run_config),      SYM(polycall_ffi_run_config), 1
    PC_RUN  SYM(asm_polycall_validate_config), SYM(polycall_ffi_run_config), 0
    PC_TAIL SYM(asm_polycall_run_config_ex),   SYM(polycall_ffi_run_config)
    PC_TAIL SYM(asm_polycall_abi_version),     SYM(polycall_ffi_abi_version)
    PC_TAIL SYM(asm_polycall_version),         SYM(polycall_ffi_version)
    PC_TAIL SYM(asm_polycall_strerror),        SYM(polycall_strerror)
    PC_TAIL SYM(asm_polycall_last_error),      SYM(polycall_last_error)
    PC_TAIL SYM(asm_polycall_describe),        SYM(polycall_ffi_describe)
    PC_TAIL SYM(asm_polycall_call),            SYM(polycall_call)
    PC_TAIL SYM(asm_polycall_peer_open),       SYM(polycall_peer_open)
    PC_TAIL SYM(asm_polycall_peer_close),      SYM(polycall_peer_close)
    PC_TAIL SYM(asm_polycall_peer_endpoint),   SYM(polycall_peer_endpoint)
    PC_TAIL SYM(asm_polycall_peer_node_id),    SYM(polycall_peer_node_id)
    PC_TAIL SYM(asm_polycall_peer_register),   SYM(polycall_peer_register)
    PC_TAIL SYM(asm_polycall_peer_unregister), SYM(polycall_peer_unregister)
    PC_TAIL SYM(asm_polycall_peer_list),       SYM(polycall_peer_list)
    PC_TAIL SYM(asm_polycall_peer_ping),       SYM(polycall_peer_ping)
    PC_TAIL SYM(asm_polycall_peer_send),       SYM(polycall_peer_send)
    PC_TAIL SYM(asm_polycall_peer_recv),       SYM(polycall_peer_recv)
    PC_TAIL SYM(asm_polycall_peer_cancel),     SYM(polycall_peer_cancel)
    PC_TAIL SYM(asm_polycall_peer_health),     SYM(polycall_peer_health)

#if defined(__ELF__) && (defined(__x86_64__) || defined(__i386__))
    .section .note.GNU-stack,"",@progbits
#elif defined(__ELF__) && (defined(__aarch64__) || defined(__arm__))
    .section .note.GNU-stack,"",%progbits
#endif
