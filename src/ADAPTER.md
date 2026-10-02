# Assembly adapter

The adapter calls across the binding ABI only (`<polycall.h>`):

    asm_polycall_run_config(path)      ->  polycall_ffi_run_config(path, /*run=*/1)
    asm_polycall_validate_config(path) ->  polycall_ffi_run_config(path, /*run=*/0)
    asm_polycall_<fn>(...)             ->  polycall_<fn>(...)

Every shim is a tail call (`jmp` / `b`): arguments stay in the registers and
caller stack slots the platform C ABI assigned, so functions with more
arguments than registers (`polycall_call`, `polycall_peer_recv`) are
forwarded intact. The run_config shims only write the `run` constant into
the second-argument location. Statuses come back unchanged; callers decide
what is fatal. The adapter contains no configuration parser or runtime logic.
