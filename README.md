# asm-polycall

GNU-assembler binding for the [Polycall](https://github.com/obinexus/polycall)
core, published as the npm source package `@obinexusltd/asm-polycall`.

`src/asm_polycall.S` is a set of tail-call shims over the core's **binding
ABI v1** (`<polycall.h>`, `docs/BINDING_ABI.md` in the core repository;
requires **polycall >= 1.1.0**). Each shim leaves the caller's arguments
where the platform C ABI put them and jumps to the real symbol; the two
`run_config` shims additionally load the `run` constant into the
second-argument register. No parsing, sockets or runtime logic live here.

## C-callable API (`include/asm_polycall.h`)

| Shim | Core symbol |
| --- | --- |
| `asm_polycall_run_config(path)` | `polycall_ffi_run_config(path, 1)` (historical entry point) |
| `asm_polycall_validate_config(path)` | `polycall_ffi_run_config(path, 0)` |
| `asm_polycall_run_config_ex(path, run)` | `polycall_ffi_run_config(path, run)` |
| `asm_polycall_abi_version()` / `asm_polycall_version(buf, len)` | `polycall_ffi_abi_version` / `polycall_ffi_version` |
| `asm_polycall_strerror(st)` / `asm_polycall_last_error(buf, cap)` | `polycall_strerror` / `polycall_last_error` |
| `asm_polycall_describe(path, buf, len)` | `polycall_ffi_describe` |
| `asm_polycall_call(...)` | `polycall_call` |
| `asm_polycall_peer_{open,close,endpoint,node_id,register,unregister,list,ping,send,recv,cancel,health}` | `polycall_peer_*` |

Statuses, caller-owned buffers and handle rules are exactly the core's
(`POLYCALL_OK` = 0, negative `POLYCALL_E_*`; nothing to free).

```c
#include <asm_polycall.h>

if (asm_polycall_abi_version() != POLYCALL_FFI_ABI_VERSION) { /* refuse */ }
int status = asm_polycall_run_config("asm-polycallrc");
if (status != POLYCALL_OK) {
    char detail[512];
    asm_polycall_last_error(detail, sizeof detail);
    fprintf(stderr, "%s: %s\n", asm_polycall_strerror(status), detail);
}
```

Assembly callers use the same symbols with the platform C calling
convention (leading underscore on Mach-O and 32-bit Windows).

## Targets

| Target | Argument registers | Status |
| --- | --- | --- |
| x86-64 System V (Linux, ELF) | `RDI, RSI, ...`; `run` in `ESI` | runtime-tested |
| x86-64 Windows (MinGW/clang, COFF) | `RCX, RDX, R8, R9`; `run` in `EDX` | runtime-tested (MSYS2 UCRT64) |
| AArch64 ELF / Mach-O / Windows on ARM64 (COFF) | `X0-X7`; `run` in `W1` | assembles; not runtime-tested |
| x86-32 ELF (PIC, via GOT) / Windows / Mach-O | cdecl stack | assembles; not runtime-tested |
| AArch32 ELF | `R0-R3`; `run` in `R1` | assembles; not runtime-tested |

`make cross-check` assembles every target with clang. Windows on ARM64 has
its own COFF branch (the ELF-only `.type`/`.size` directives previously used
there do not assemble for COFF). MSVC's `cl`/`ml64` cannot assemble GNU `.S`
files: on Windows build with MinGW gcc or clang.

## Build and test

The core is located with pkg-config:

```sh
export PKG_CONFIG_PATH=/opt/polycall/lib/pkgconfig LD_LIBRARY_PATH=/opt/polycall/lib
make            # lib/libasm_polycall.a
make test       # real-core test + interop with the polycall CLI
make example    # examples/basic.c
make verify-dry # thin-adapter audit
```

Windows, from an MSYS2 UCRT64 shell with an installed MinGW build of the
core: set `PKG_CONFIG_PATH=<prefix>/lib/pkgconfig` and put `<prefix>/bin`
(libpolycall.dll, polycall.exe) on `PATH`, then `make test`.

`make test` runs `tests/run-real.sh`, which starts `polycall peer serve` and
`polycall start` and then the C test of every shim; without the CLI it exits
77 (SKIP), never success. See [tests/TESTS.md](tests/TESTS.md).

## npm source package

```sh
npm install @obinexusltd/asm-polycall
```

The CommonJS entry point only exposes absolute paths (`assembly`,
`publicHeader`, `makefile`, `config`, `manifest`); it does not load
libpolycall. The package is not yet published.

## Author

Nnamdi Michael Okpala — <okpalan@protonmail.com>. MIT licensed, see
[LICENSE](LICENSE).
