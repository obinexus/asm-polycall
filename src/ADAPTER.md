# Assembly adapter

The adapter calls across the FFI boundary only:

    status = polycall_ffi_run_config("asm-polycallrc", /*run=*/1)

`asm_polycall.S` preserves the incoming configuration pointer, places `1` in
the ABI-specific second C argument location, and transfers control to
`polycall_ffi_run_config`. Its integer return status is propagated unchanged.
Callers decide whether a nonzero status is fatal. The adapter contains no
configuration parser or runtime logic.
