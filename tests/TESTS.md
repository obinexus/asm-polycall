# asm-polycall tests

All tests run against the REAL installed Polycall core (no mock); every call
goes through the assembly shims.

- `asm_polycall_real_test.c` -- the checklist of the core's
  `docs/BINDING_ABI.md`: version/ABI; `run_config` / `validate_config` /
  `run_config_ex` (valid, missing, malformed, strict unknown key, unsupported
  TLS -- which also proves the injected `run` constant); `asm_polycall_call`
  against a live `polycall start` (8 arguments: the out_len pointer is
  stack-passed on both x86-64 ABIs); two peers exchanging empty, UTF-8,
  binary-with-NUL and exactly-1-MiB payloads both ways, 1 MiB + 1 rejected;
  registry ownership; duplicate id stored once; auth failure; dead peer;
  receive timeout; too-small buffer (`payload_len` is the 9th, stack-passed
  argument) keeps the message queued; cancel/close waking a blocked receive
  (pthreads); double close / use after close / invalid handles; 4 concurrent
  sender threads; interop both ways with the C CLI. No `assert()`.
- `run-real.sh` starts `polycall peer serve` + `polycall start` (random
  `POLYCALL_DEV_TOKEN`) and runs the test; exit 77 = SKIP without the CLI.
- `make cross-check` assembles the shims for x86-64/x86-32/AArch64/AArch32
  ELF, Windows (COFF, incl. ARM64) and Mach-O with clang.
- `package.test.js` -- npm entry point paths.
