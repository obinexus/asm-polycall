#ifndef ASM_POLYCALL_H
#define ASM_POLYCALL_H

/*
 * C prototypes of the asm-polycall shims (src/asm_polycall.S). Each shim is
 * a tail call into the Polycall binding ABI v1 with the identical signature
 * (see <polycall.h> and docs/BINDING_ABI.md of the core), except the two
 * run_config shims, which supply the `run` argument. Statuses, buffer rules
 * and handle semantics are exactly the core's: POLYCALL_OK (0) or a
 * negative POLYCALL_E_* code; the core never returns memory to free.
 *
 * Assembly callers use the same symbols with the platform C calling
 * convention (cdecl on 32-bit Windows; leading underscore on Mach-O and
 * 32-bit Windows).
 */

#include <polycall.h>

#ifdef __cplusplus
extern "C" {
#endif

/* polycall_ffi_run_config(config_path, 1): validate for running with this build. */
int asm_polycall_run_config(const char *config_path);
/* polycall_ffi_run_config(config_path, 0): validate only (unknown keys warn). */
int asm_polycall_validate_config(const char *config_path);
/* polycall_ffi_run_config(config_path, run). */
int asm_polycall_run_config_ex(const char *config_path, int run);

int asm_polycall_abi_version(void);
int asm_polycall_version(char *buf, int len);
const char *asm_polycall_strerror(int status);
int asm_polycall_last_error(char *buf, size_t cap);
int asm_polycall_describe(const char *config_path, char *buf, int len);

int asm_polycall_call(const char *endpoint, const char *service,
                      const char *operation, const char *input_json,
                      uint32_t timeout_ms, char *out, size_t out_cap,
                      size_t *out_len);

int asm_polycall_peer_open(const char *node_id, const char *bind_endpoint,
                           const char *auth_token, polycall_peer_t *out_handle);
int asm_polycall_peer_close(polycall_peer_t h);
int asm_polycall_peer_endpoint(polycall_peer_t h, char *buf, size_t cap);
int asm_polycall_peer_node_id(polycall_peer_t h, char *buf, size_t cap);
int asm_polycall_peer_register(polycall_peer_t h, const char *peer_id,
                               const char *endpoint);
int asm_polycall_peer_unregister(polycall_peer_t h, const char *peer_id);
int asm_polycall_peer_list(polycall_peer_t h, char *buf, size_t cap,
                           size_t *out_len);
int asm_polycall_peer_ping(polycall_peer_t h, const char *peer,
                           uint32_t timeout_ms);
int asm_polycall_peer_send(polycall_peer_t h, const char *peer,
                           const void *payload, size_t len,
                           const char *message_id, uint32_t timeout_ms);
int asm_polycall_peer_recv(polycall_peer_t h, uint32_t timeout_ms,
                           char *sender, size_t sender_cap,
                           char *message_id, size_t message_id_cap,
                           void *payload, size_t payload_cap,
                           size_t *payload_len);
int asm_polycall_peer_cancel(polycall_peer_t h);
int asm_polycall_peer_health(polycall_peer_t h, char *buf, size_t cap,
                             size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* ASM_POLYCALL_H */
