# asm-polycall tests

All functional tests run against the REAL installed Polycall core (no mock);
every call goes through the assembly shims.

- `asm_polycall_real_test.c` -- the checklist of the core's
  `docs/BINDING_ABI.md`: version/ABI; `run_config` / `validate_config` /
  `run_config_ex` (valid, missing, malformed, strict unknown key, unsupported
  TLS -- which also proves the injected `run` constant), also on a non-ASCII
  (UTF-8) config path; `asm_polycall_call` against a live `polycall start`
  and a `polycall daemon` (8 arguments: the out_len pointer is stack-passed
  on both x86-64 ABIs and on x86-32), `timeout_ms` 1 / 600000 accepted and
  600001 / `UINT32_MAX` rejected, a too-small `out` buffer, 4 threads x 10
  concurrent calls; two peers exchanging empty, UTF-8, binary-with-NUL and
  exactly-1-MiB payloads both ways, 1 MiB + 1 rejected; registry ownership;
  duplicate id stored once; auth failure; dead peer; receive timeout and
  `timeout 0` poll; too-small buffer (`payload_len` is the 9th, stack-passed
  argument) keeps the message queued; 63/64-byte ids and snprintf capacity
  edges; cancel/close waking a blocked receive (pthreads); double close /
  use after close / invalid handles; 4 concurrent sender threads; interop
  both ways with the C CLI. No `assert()`. Exit status: 0 all passed, 1 a
  check failed, 77 a check was skipped.
- `run-real.sh` starts `polycall peer serve`, `polycall start` and `polycall
  daemon start` (random `POLYCALL_DEV_TOKEN`, ephemeral ports, private state
  directory) and runs the test; exit 77 = SKIP without the CLI. Any command
  can wrap the test (valgrind, `qemu-aarch64 -L ...`).
- `loader-errors.sh` + `abi_probe.c` (`make test-loader`) -- loader
  behaviour: real library accepted; no library and a 1.0 library (no ABI v1
  symbols) refused by the platform loader, never a crash; a library
  reporting ABI 2 seen through `asm_polycall_abi_version()` and refused.
  `loader/fake_polycall.c` builds the two fake libraries -- TEST FIXTURES,
  not the core.
- `make cross-check` assembles the shims for x86-64/x86-32/AArch64/AArch32
  ELF, Windows (COFF, incl. ARM64) and Mach-O with clang.
- `package.test.js` -- npm entry point paths.
