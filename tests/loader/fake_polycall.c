/*
 * TEST FIXTURE -- NOT THE POLYCALL CORE.
 *
 * Stand-ins for the shared library (libpolycall.so.1 / libpolycall.dll) used
 * ONLY by tests/loader-errors.sh:
 *
 *   FAKE_ABI2  every ABI v1 symbol exists, but polycall_ffi_abi_version()
 *              reports 2; every other entry point returns POLYCALL_E_INTERNAL.
 *   FAKE_OLD   a 1.0-era library: only the 1.0 symbols, no ABI v1 surface.
 *
 * No functional check uses these; they run against the real installed core
 * (tests/asm_polycall_real_test.c).
 */
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#define FAKE_API __declspec(dllexport)
#else
#define FAKE_API __attribute__((visibility("default")))
#endif

#define FAKE_INTERNAL (-18) /* POLYCALL_E_INTERNAL */

FAKE_API const char *polycall_get_version(void) { return "1.0.0-fake"; }

#if defined(FAKE_ABI2)
FAKE_API int polycall_ffi_abi_version(void) { return 2; }
FAKE_API int polycall_ffi_version(char *buf, int len)
{
    static const char v[] = "2.0.0-fake";
    int i = 0;
    if (len < 0) return -1;
    if (len > 0) {
        for (; i < (int)sizeof v - 1 && i < len - 1; ++i) buf[i] = v[i];
        buf[i] = '\0';
    }
    return (int)sizeof v - 1;
}
FAKE_API const char *polycall_strerror(int status) { (void)status; return "POLYCALL_E_INTERNAL: fake library"; }
FAKE_API int polycall_last_error(char *buf, size_t cap) { if (buf && cap) buf[0] = '\0'; return 0; }
FAKE_API int polycall_ffi_run_config(const char *p, int r) { (void)p; (void)r; return FAKE_INTERNAL; }
FAKE_API int polycall_ffi_describe(const char *p, char *b, int l) { (void)p; (void)b; (void)l; return FAKE_INTERNAL; }
FAKE_API int polycall_call(const char *e, const char *s, const char *o, const char *i, uint32_t t, char *out,
                           size_t cap, size_t *len)
{ (void)e; (void)s; (void)o; (void)i; (void)t; (void)out; (void)cap; (void)len; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_open(const char *n, const char *b, const char *t, int32_t *h)
{ (void)n; (void)b; (void)t; (void)h; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_close(int32_t h) { (void)h; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_endpoint(int32_t h, char *b, size_t c) { (void)h; (void)b; (void)c; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_node_id(int32_t h, char *b, size_t c) { (void)h; (void)b; (void)c; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_register(int32_t h, const char *p, const char *e) { (void)h; (void)p; (void)e; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_unregister(int32_t h, const char *p) { (void)h; (void)p; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_list(int32_t h, char *b, size_t c, size_t *n) { (void)h; (void)b; (void)c; (void)n; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_ping(int32_t h, const char *p, uint32_t t) { (void)h; (void)p; (void)t; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_send(int32_t h, const char *p, const void *d, size_t n, const char *m, uint32_t t)
{ (void)h; (void)p; (void)d; (void)n; (void)m; (void)t; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_recv(int32_t h, uint32_t t, char *s, size_t sc, char *m, size_t mc, void *p, size_t pc,
                                size_t *n)
{ (void)h; (void)t; (void)s; (void)sc; (void)m; (void)mc; (void)p; (void)pc; (void)n; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_cancel(int32_t h) { (void)h; return FAKE_INTERNAL; }
FAKE_API int polycall_peer_health(int32_t h, char *b, size_t c, size_t *n) { (void)h; (void)b; (void)c; (void)n; return FAKE_INTERNAL; }
#elif !defined(FAKE_OLD)
#error "define FAKE_ABI2 or FAKE_OLD"
#endif
