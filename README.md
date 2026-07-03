# asm-polycall

Cross-platform assembly binding for libpolycall 1.5.0, published as
`@obinexusltd/asm-polycall`.

The binding is deliberately tiny: it preserves the incoming configuration
path, supplies `run=1` in the platform C ABI's second argument location, and
forwards control to `polycall_ffi_run_config`. It contains no configuration
parser, network stack, or duplicated runtime logic.

## Install from npm

```sh
npm install @obinexusltd/asm-polycall
```

The CommonJS entry point exposes absolute paths for build tooling:

```js
const polycall = require('@obinexusltd/asm-polycall');

console.log(polycall.assembly);
console.log(polycall.publicHeader);
console.log(polycall.ffiHeader);
console.log(polycall.config);
console.log(polycall.manifest);
```

Direct package subpaths are also exported, including
`@obinexusltd/asm-polycall/src/asm_polycall.S` and
`@obinexusltd/asm-polycall/include/asm_polycall.h`.

## C-callable API

```c
#include <asm_polycall.h>

int status = asm_polycall_run_config("asm-polycallrc");
if (status != 0) {
    /* Apply the host application's error policy. */
}
```

The function returns the libpolycall status unchanged. Assembly callers can
call the same symbol using their platform's C calling convention.

## Supported ABIs

| Target | First argument | Injected second argument |
|---|---:|---:|
| Windows x86-64 | `RCX` | `EDX = 1` |
| System V x86-64 | `RDI` | `ESI = 1` |
| Windows/System V x86-32 | cdecl stack | cdecl stack value `1` |
| AArch64 | `X0` | `W1 = 1` |
| AArch32 | `R0` | `R1 = 1` |

macOS underscore-prefixed external symbols are handled for x86-64 and
AArch64. The source uses a preprocessed GNU-compatible `.S` file, so the C
compiler selects the correct branch for the target architecture.

The implementation follows the
[Microsoft x64 calling convention](https://learn.microsoft.com/cpp/build/x64-calling-convention),
[x86-64 System V psABI](https://gitlab.com/x86-psABIs/x86-64-ABI), and
[Arm procedure-call standards](https://github.com/ARM-software/abi-aa).

## Build and test

Build the static adapter archive:

```sh
make
# or
npm run build
```

Run the real assembly shim against the mock libpolycall boundary:

```sh
npm test
npm run verify
```

The test verifies the configuration pointer, `run=1`, and unchanged success
and failure statuses. Platform-specific binaries and archives are excluded
from the npm tarball; source, headers, tests, examples, scripts, manifest, and
configuration are included.

## Link with libpolycall

A real integration requires libpolycall v1.5 exporting
`polycall_ffi_run_config`:

```sh
make example POLYCALL_LDFLAGS="-L/path/to/libpolycall/lib -lpolycall"
```

The example host is [`examples/basic.c`](examples/basic.c). The read-only
shared-schema override is [`asm-polycallrc`](asm-polycallrc).

## Publishing

Inspect the exact package contents and then publish the public scoped package:

```sh
npm pack --dry-run
npm publish --access public
```

Publishing is intentionally not performed by the build or test scripts.

## Author

Nnamdi Michael Okpala — <okpalan@protonmail.com>
