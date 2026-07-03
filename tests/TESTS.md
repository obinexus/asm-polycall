# Assembly tests

`asm_polycall_adapter_test.c` links the real assembly object against a mock
libpolycall FFI. It verifies that the adapter forwards the path, sets `run=1`,
and propagates both success and failure statuses unchanged. Run it with
`make test` or `npm test`.
