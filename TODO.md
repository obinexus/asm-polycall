# TODO — asm-polycall (Assembly)

Status: supported -- tail-call shims over Polycall binding ABI v1
(polycall >= 1.1.0). Runtime-tested: x86-64 Linux and Windows (UCRT64),
x86-32 Linux and Windows, AArch64 and AArch32 Linux under qemu-user.

- [x] `asm_polycall_run_config` -> `polycall_ffi_run_config(path, 1)` kept
- [x] Shims for version/ABI, strerror/last_error, describe, call and peer API
- [x] Real `<polycall.h>` + pkg-config instead of a generated stub
- [x] Windows on ARM64 COFF branch (was falling into the ELF branch)
- [x] i386 ELF shims reach the core through the GOT (PIC-safe)
- [x] Real-core tests + C CLI interop on Linux x86-64 and Windows UCRT64
- [x] Runtime tests on AArch64 / AArch32 Linux (qemu-user, emulated) and x86-32 Linux / Windows (native)
- [x] Daemon calls, non-ASCII config path, limits, concurrent calls, loader errors
- [ ] Runtime tests on macOS (x86-64, arm64) and Windows on ARM64 (no hardware)
- [ ] Publish `asm-polycall` (not published yet)

Do not add config parsing or runtime logic here — adapt the core only.
