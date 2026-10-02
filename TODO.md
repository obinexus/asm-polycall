# TODO — asm-polycall (Assembly)

Status: supported on x86-64 (System V, Win64) -- tail-call shims over
Polycall binding ABI v1 (polycall >= 1.1.0).

- [x] `asm_polycall_run_config` -> `polycall_ffi_run_config(path, 1)` kept
- [x] Shims for version/ABI, strerror/last_error, describe, call and peer API
- [x] Real `<polycall.h>` + pkg-config instead of a generated stub
- [x] Windows on ARM64 COFF branch (was falling into the ELF branch)
- [x] i386 ELF shims reach the core through the GOT (PIC-safe)
- [x] Real-core tests + C CLI interop on Linux x86-64 and Windows UCRT64
- [ ] Runtime tests on AArch64 / x86-32 / AArch32 (assemble-checked only)
- [ ] Publish `@obinexusltd/asm-polycall` (not published yet)

Do not add config parsing or runtime logic here — adapt the core only.
