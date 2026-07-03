# TODO — asm-polycall (Assembly)

Status: implemented thin adapter for libpolycall 1.5.0.

- [x] Folder structure, manifest, and `asm-polycallrc` (shared schema)
- [x] Generate the consumed declaration from `polycall_ffi.h`
- [x] Implement GNU assembly adapters for supported C ABIs
- [x] Add a runnable example under `examples/`
- [x] Add an ABI smoke test under `tests/`
- [x] Add `scripts/verify-dry.sh` (no core duplication)

Do not add config parsing or runtime logic here — adapt the core only.
