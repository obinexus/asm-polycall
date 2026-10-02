/*
 * asm-polycall tests against the REAL installed libpolycall (no mock): every
 * call goes through the assembly shims in src/asm_polycall.S.
 *
 *   asm_polycall_real_test <repo-root>
 *
 * Covers the checklist of the core's docs/BINDING_ABI.md. The 8- and 9-
 * argument functions (asm_polycall_call, asm_polycall_peer_recv) prove that
 * the tail calls keep stack-passed arguments intact on both x86-64 ABIs.
 * Checks needing the C CLI read POLYCALL_CLI, POLYCALL_CLI_PEER,
 * POLYCALL_RPC_ENDPOINT and POLYCALL_DEV_TOKEN (tests/run-real.sh); without
 * them they print SKIP, never PASS. No assert(): checks also run with NDEBUG.
 */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include "asm_polycall.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
static void sleep_ms(unsigned ms) { Sleep(ms); }
static double now_ms(void) { return (double)GetTickCount64(); }
#else
static void sleep_ms(unsigned ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000u;
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    nanosleep(&ts, NULL);
}
static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}
#endif

static int g_pass, g_fail, g_skip;

static void check(int ok, const char *name, const char *fmt, ...)
{
    if (ok) {
        ++g_pass;
        printf("PASS %s\n", name);
    } else {
        va_list ap;
        ++g_fail;
        printf("FAIL %s -- ", name);
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
        printf("\n");
    }
    fflush(stdout);
}

static void skip(const char *name, const char *reason)
{
    ++g_skip;
    printf("SKIP %s -- %s\n", name, reason);
    fflush(stdout);
}

#define EXPECT(expr, want, name)                                                   \
    do {                                                                           \
        int got_ = (expr);                                                         \
        check(got_ == (want), name, "expected %d got %d (%s)", (want), got_,       \
              asm_polycall_strerror(got_));                                        \
    } while (0)

static const char *env(const char *n)
{
    const char *v = getenv(n);
    return (v && *v) ? v : NULL;
}

static void base64(const unsigned char *in, size_t n, char *out)
{
    static const char t[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t i = 0, o = 0;
    for (; i + 2 < n; i += 3) {
        unsigned v = ((unsigned)in[i] << 16) | ((unsigned)in[i + 1] << 8) | in[i + 2];
        out[o++] = t[(v >> 18) & 63];
        out[o++] = t[(v >> 12) & 63];
        out[o++] = t[(v >> 6) & 63];
        out[o++] = t[v & 63];
    }
    if (n - i == 1) {
        unsigned v = (unsigned)in[i] << 16;
        out[o++] = t[(v >> 18) & 63];
        out[o++] = t[(v >> 12) & 63];
        out[o++] = '=';
        out[o++] = '=';
    } else if (n - i == 2) {
        unsigned v = ((unsigned)in[i] << 16) | ((unsigned)in[i + 1] << 8);
        out[o++] = t[(v >> 18) & 63];
        out[o++] = t[(v >> 12) & 63];
        out[o++] = t[(v >> 6) & 63];
        out[o++] = '=';
    }
    out[o] = '\0';
}

static int run_command(const char *cmd)
{
#if defined(_WIN32)
    char buf[4096];
    snprintf(buf, sizeof buf, "\"%s\"", cmd); /* cmd.exe strips outer quotes */
    return system(buf);
#else
    return system(cmd);
#endif
}

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *buf;
    long n;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = calloc((size_t)n + 1, 1);
    if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n) buf[0] = '\0';
    fclose(f);
    return buf;
}

static polycall_peer_t open_node(const char *id, const char *bind, const char *token)
{
    polycall_peer_t h = 0;
    int st = asm_polycall_peer_open(id, bind, token, &h);
    if (st != POLYCALL_OK) {
        char d[256];
        asm_polycall_last_error(d, sizeof d);
        printf("FATAL open %s: %s (%s)\n", id, asm_polycall_strerror(st), d);
        exit(1);
    }
    return h;
}

static void endpoint_of(polycall_peer_t h, char *ep)
{
    if (asm_polycall_peer_endpoint(h, ep, POLYCALL_ENDPOINT_MAX) != POLYCALL_OK) ep[0] = '\0';
}

/* recv into a fresh 1 MiB buffer; returns status */
typedef struct {
    char sender[POLYCALL_PEER_ID_MAX];
    char id[POLYCALL_MESSAGE_ID_MAX];
    unsigned char *data;
    size_t len;
} msg_t;

static int recv_msg(polycall_peer_t h, uint32_t timeout, msg_t *m)
{
    if (!m->data) m->data = malloc(POLYCALL_PEER_MAX_PAYLOAD);
    m->len = 0;
    m->sender[0] = m->id[0] = '\0';
    return asm_polycall_peer_recv(h, timeout, m->sender, sizeof m->sender, m->id, sizeof m->id,
                                  m->data, POLYCALL_PEER_MAX_PAYLOAD, &m->len);
}

static void exchange(polycall_peer_t from, const char *from_id, polycall_peer_t to,
                     const unsigned char *p, size_t n, const char *mid, const char *label)
{
    char ep[POLYCALL_ENDPOINT_MAX];
    msg_t m = {0};
    int s, r;
    endpoint_of(to, ep);
    s = asm_polycall_peer_send(from, ep, p, n, mid, 5000);
    r = s == POLYCALL_OK ? recv_msg(to, 5000, &m) : s;
    check(s == POLYCALL_OK && r == POLYCALL_OK && strcmp(m.sender, from_id) == 0 &&
              strcmp(m.id, mid) == 0 && m.len == n && (n == 0 || memcmp(m.data, p, n) == 0),
          label, "send=%d recv=%d sender=%s id=%s len=%zu", s, r, m.sender, m.id, m.len);
    free(m.data);
}

/* ------------------------------------------------------------------------ */

static void test_version(void)
{
    char v[32], small[2];
    int n = asm_polycall_version(v, (int)sizeof v);
    check(asm_polycall_abi_version() == 1, "abi: asm_polycall_abi_version() == 1", "got %d",
          asm_polycall_abi_version());
    check(n == (int)strlen(v) && strncmp(v, "1.", 2) == 0 && strcmp(v, "1.1.0") >= 0,
          "version: library >= 1.1.0", "%s", v);
    EXPECT(asm_polycall_version(small, -1), POLYCALL_E_INVALID_ARGUMENT,
           "version: negative length -> E_INVALID_ARGUMENT");
    check(asm_polycall_version(small, 2) == n && small[1] == '\0', "version: snprintf rules", "");
    check(strncmp(asm_polycall_strerror(POLYCALL_E_TIMEOUT), "POLYCALL_E_TIMEOUT", 18) == 0,
          "strerror names E_TIMEOUT", "%s", asm_polycall_strerror(POLYCALL_E_TIMEOUT));
    check(strncmp(asm_polycall_strerror(-999), "POLYCALL_E_UNKNOWN", 18) == 0,
          "strerror of an unknown code", "");
}

static void test_run_config(const char *root)
{
    char p[1024], d[512], js[8192];
    int n;
    snprintf(p, sizeof p, "%s/asm-polycallrc", root);
    EXPECT(asm_polycall_run_config(p), POLYCALL_OK, "run_config: shipped asm-polycallrc (run=1)");
    EXPECT(asm_polycall_validate_config(p), POLYCALL_OK, "run_config: valid, validate-only");
    n = asm_polycall_describe(p, js, (int)sizeof js);
    check(n > 0 && js[0] == '{' && strstr(js, "log_level") != NULL, "describe: JSON", "%d %.80s", n, js);
    snprintf(p, sizeof p, "%s/examples/asm-polycallrc", root);
    EXPECT(asm_polycall_run_config(p), POLYCALL_OK, "run_config: examples/asm-polycallrc");
    snprintf(p, sizeof p, "%s/tests/fixtures/does-not-exist", root);
    EXPECT(asm_polycall_run_config(p), POLYCALL_E_NOT_FOUND, "run_config: missing -> E_NOT_FOUND");
    asm_polycall_last_error(d, sizeof d);
    check(strstr(d, "does-not-exist") != NULL, "last_error: detail names the file", "%s", d);
    snprintf(p, sizeof p, "%s/tests/fixtures/invalid-polycallrc", root);
    EXPECT(asm_polycall_validate_config(p), POLYCALL_E_CONFIG, "run_config: malformed -> E_CONFIG (validate)");
    EXPECT(asm_polycall_run_config(p), POLYCALL_E_CONFIG, "run_config: malformed -> E_CONFIG (strict)");
    snprintf(p, sizeof p, "%s/tests/fixtures/unknown-key-polycallrc", root);
    EXPECT(asm_polycall_validate_config(p), POLYCALL_OK, "run_config: unknown key warns when validating");
    EXPECT(asm_polycall_run_config(p), POLYCALL_E_CONFIG, "run_config: unknown key fails when strict");
    EXPECT(asm_polycall_run_config_ex(p, 0), POLYCALL_OK, "run_config_ex(path, 0) forwards run=0");
    snprintf(p, sizeof p, "%s/tests/fixtures/tls-polycallrc", root);
    EXPECT(asm_polycall_validate_config(p), POLYCALL_OK, "run_config: tls_enabled=true validates");
    EXPECT(asm_polycall_run_config(p), POLYCALL_E_UNSUPPORTED, "run_config: tls strict -> E_UNSUPPORTED");
    EXPECT(asm_polycall_run_config(""), POLYCALL_E_INVALID_ARGUMENT, "run_config: empty path");
    EXPECT(asm_polycall_run_config(NULL), POLYCALL_E_INVALID_ARGUMENT, "run_config: NULL path");
}

static void test_call(void)
{
    static char out[POLYCALL_CALL_MAX_OUTPUT + 1];
    size_t len = 0;
    const char *ep = env("POLYCALL_RPC_ENDPOINT");
    EXPECT(asm_polycall_call("127.0.0.1:1", "debug", "echo", "{}", 1500, out, sizeof out, &len),
           POLYCALL_E_TRANSPORT, "call: no runtime -> E_TRANSPORT");
    EXPECT(asm_polycall_call("127.0.0.1:1", "debug", "echo", "{}", 0, out, sizeof out, &len),
           POLYCALL_E_INVALID_ARGUMENT, "call: timeout 0 -> E_INVALID_ARGUMENT");
    if (!ep) {
        skip("call: success / unknown op / deadline / invalid input",
             "POLYCALL_RPC_ENDPOINT not set (run via tests/run-real.sh)");
        return;
    }
    EXPECT(asm_polycall_call(ep, "inventory", "get", "{\"item_id\":\"widget-a\"}", 3000, out,
                             sizeof out, &len), POLYCALL_OK, "call: inventory.get");
    check(strcmp(out, "{\"item_id\":\"widget-a\",\"quantity\":42,\"in_stock\":true}") == 0 &&
              len == strlen(out), "call: exact output + out_len (8th arg, stack-passed)", "%s %zu", out, len);
    EXPECT(asm_polycall_call(ep, "debug", "echo", "{\"s\":\"h\xc3\xa9\"}", 3000, out, sizeof out, &len),
           POLYCALL_OK, "call: debug.echo");
    check(strcmp(out, "{\"echo\":{\"s\":\"h\xc3\xa9\"}}") == 0, "call: echo exact UTF-8 output", "%s", out);
    EXPECT(asm_polycall_call(ep, "inventory", "teleport", "{}", 3000, out, sizeof out, &len),
           POLYCALL_E_NOT_FOUND, "call: unknown operation -> E_NOT_FOUND");
    check(strstr(out, "operation.unknown") != NULL, "call: unknown operation error object", "%s", out);
    EXPECT(asm_polycall_call(ep, "inventory", "get", "{\"item_id\":\"nope\"}", 3000, out, sizeof out, &len),
           POLYCALL_E_REMOTE, "call: unknown item -> E_REMOTE");
    EXPECT(asm_polycall_call(ep, "debug", "sleep", "{\"ms\":3000}", 200, out, sizeof out, &len),
           POLYCALL_E_TIMEOUT, "call: deadline -> E_TIMEOUT");
    EXPECT(asm_polycall_call(ep, "debug", "echo", "{not json", 1000, out, sizeof out, &len),
           POLYCALL_E_INVALID_ARGUMENT, "call: invalid input JSON");
}

static void test_peers(void)
{
    static const unsigned char bin[] = {0x00, 0x01, 0x00, 0xff, 0x7f, 0x00, 0x80};
    static const char utf8[] = "h\xc3\xa9llo w\xc3\xb6rld \xe2\x9c\x93 \xf0\x9f\x9a\x80";
    unsigned char *mib = malloc(POLYCALL_PEER_MAX_PAYLOAD + 1);
    polycall_peer_t a = open_node("asm-a", "127.0.0.1:0", NULL);
    polycall_peer_t b = open_node("asm-b", "127.0.0.1:0", NULL);
    char ea[POLYCALL_ENDPOINT_MAX], eb[POLYCALL_ENDPOINT_MAX], buf[4096], id[64];
    size_t i, len = 0;
    msg_t m = {0};

    endpoint_of(a, ea);
    endpoint_of(b, eb);
    check(strncmp(ea, "127.0.0.1:", 10) == 0 && strcmp(ea, "127.0.0.1:0") != 0, "peer: ephemeral endpoint", "%s", ea);
    check(asm_polycall_peer_node_id(a, id, sizeof id) == 0 && strcmp(id, "asm-a") == 0, "peer: node_id", "%s", id);
    check(asm_polycall_peer_health(a, buf, sizeof buf, &len) == 0 && strstr(buf, "\"node_id\":\"asm-a\""),
          "peer: health JSON", "%s", buf);
    for (i = 0; i < POLYCALL_PEER_MAX_PAYLOAD + 1; ++i) mib[i] = (unsigned char)((i * 31u + 7u) & 0xffu);
    exchange(a, "asm-a", b, (const unsigned char *)"", 0, "m-empty-ab", "peer a->b: empty payload");
    exchange(b, "asm-b", a, (const unsigned char *)"", 0, "m-empty-ba", "peer b->a: empty payload");
    exchange(a, "asm-a", b, (const unsigned char *)utf8, sizeof utf8 - 1, "m-utf8-ab", "peer a->b: UTF-8");
    exchange(b, "asm-b", a, (const unsigned char *)utf8, sizeof utf8 - 1, "m-utf8-ba", "peer b->a: UTF-8");
    exchange(a, "asm-a", b, bin, sizeof bin, "m-bin-ab", "peer a->b: binary with NUL");
    exchange(b, "asm-b", a, bin, sizeof bin, "m-bin-ba", "peer b->a: binary with NUL");
    exchange(a, "asm-a", b, mib, POLYCALL_PEER_MAX_PAYLOAD, "m-mib-ab", "peer a->b: exactly 1 MiB");
    exchange(b, "asm-b", a, mib, POLYCALL_PEER_MAX_PAYLOAD, "m-mib-ba", "peer b->a: exactly 1 MiB");
    EXPECT(asm_polycall_peer_send(a, eb, mib, POLYCALL_PEER_MAX_PAYLOAD + 1, "m-over", 5000),
           POLYCALL_E_TOO_LARGE, "peer: 1 MiB + 1 -> E_TOO_LARGE");
    EXPECT(recv_msg(b, 200, &m), POLYCALL_E_TIMEOUT, "peer: oversize payload never delivered");

    /* registry ownership */
    EXPECT(asm_polycall_peer_register(a, "asm-b", eb), POLYCALL_OK, "registry: register on a");
    asm_polycall_peer_list(a, buf, sizeof buf, &len);
    check(strstr(buf, "\"asm-b\":") != NULL, "registry: a lists asm-b", "%s", buf);
    asm_polycall_peer_list(b, buf, sizeof buf, &len);
    check(strcmp(buf, "{}") == 0 && len == 2, "registry: b's registry is its own (empty)", "%s", buf);
    EXPECT(asm_polycall_peer_send(a, "asm-b", "by-id", 5, "m-by-id", 5000), POLYCALL_OK,
           "registry: send by registered id");
    EXPECT(recv_msg(b, 5000, &m), POLYCALL_OK, "registry: received by id");
    asm_polycall_peer_list(b, buf, sizeof buf, &len);
    check(strcmp(buf, "{}") == 0, "registry: receiving does not register the sender", "%s", buf);
    EXPECT(asm_polycall_peer_ping(a, "asm-b", 3000), POLYCALL_OK, "ping: registered id");
    EXPECT(asm_polycall_peer_ping(a, eb, 3000), POLYCALL_OK, "ping: host:port");
    asm_polycall_peer_register(a, "impostor", eb);
    EXPECT(asm_polycall_peer_ping(a, "impostor", 3000), POLYCALL_E_PROTOCOL,
           "ping: id answered by another node -> E_PROTOCOL");
    EXPECT(asm_polycall_peer_unregister(a, "impostor"), POLYCALL_OK, "registry: unregister");
    EXPECT(asm_polycall_peer_unregister(a, "impostor"), POLYCALL_E_NOT_FOUND, "registry: unregister unknown");
    EXPECT(asm_polycall_peer_register(a, "bad id!", eb), POLYCALL_E_INVALID_ARGUMENT,
           "registry: invalid id -> E_INVALID_ARGUMENT");
    EXPECT(asm_polycall_peer_list(a, buf, 4, &len), POLYCALL_E_TOO_LARGE, "list: small buffer -> E_TOO_LARGE");
    check(len > 4, "list: needed size reported", "%zu", len);

    /* duplicate message id */
    asm_polycall_peer_send(a, eb, "dup", 3, "m-dup", 5000);
    EXPECT(asm_polycall_peer_send(a, eb, "dup", 3, "m-dup", 5000), POLYCALL_OK, "duplicate: resend acknowledged");
    EXPECT(recv_msg(b, 5000, &m), POLYCALL_OK, "duplicate: first copy delivered");
    EXPECT(recv_msg(b, 300, &m), POLYCALL_E_TIMEOUT, "duplicate: second copy dropped");

    /* generated id, timeout, too-small buffer */
    asm_polycall_peer_send(a, eb, "gen", 3, NULL, 5000);
    EXPECT(recv_msg(b, 5000, &m), POLYCALL_OK, "send: NULL message id generated");
    check(m.id[0] != '\0', "send: generated id is non-empty", "%s", m.id);
    {
        double t0 = now_ms();
        EXPECT(recv_msg(b, 150, &m), POLYCALL_E_TIMEOUT, "recv: timeout -> E_TIMEOUT");
        check(now_ms() - t0 >= 100, "recv: waited for the timeout", "%.0f ms", now_ms() - t0);
    }
    asm_polycall_peer_send(a, eb, "0123456789", 10, "m-small", 5000);
    {
        char s[64], mid[64];
        unsigned char tiny[4];
        size_t need = 0;
        EXPECT(asm_polycall_peer_recv(b, 5000, s, sizeof s, mid, sizeof mid, tiny, sizeof tiny, &need),
               POLYCALL_E_TOO_LARGE, "recv: too-small buffer -> E_TOO_LARGE");
        check(need == 10, "recv: needed size reported (9th arg, stack-passed)", "%zu", need);
        EXPECT(recv_msg(b, 1000, &m), POLYCALL_OK, "recv: message stayed queued");
        check(m.len == 10 && memcmp(m.data, "0123456789", 10) == 0 && strcmp(m.id, "m-small") == 0,
              "recv: queued message intact", "");
    }
    {
        polycall_peer_t s = open_node("asm-sendonly", NULL, NULL);
        char e[POLYCALL_ENDPOINT_MAX] = "x";
        asm_polycall_peer_endpoint(s, e, sizeof e);
        check(e[0] == '\0', "send-only: empty endpoint", "%s", e);
        EXPECT(asm_polycall_peer_send(s, eb, "so", 2, "m-so", 5000), POLYCALL_OK, "send-only: can send");
        EXPECT(recv_msg(b, 5000, &m), POLYCALL_OK, "send-only: delivered");
        asm_polycall_peer_close(s);
    }
    asm_polycall_peer_close(a);
    asm_polycall_peer_close(b);
    free(m.data);
    free(mib);
}

static void test_auth_transport(void)
{
    polycall_peer_t sec = open_node("asm-secured", "127.0.0.1:0", "qa-secret-1");
    polycall_peer_t anon = open_node("asm-anon", NULL, NULL);
    polycall_peer_t wrong = open_node("asm-wrong", NULL, "nope");
    polycall_peer_t member = open_node("asm-member", NULL, "qa-secret-1");
    polycall_peer_t gone, h = 0;
    char es[POLYCALL_ENDPOINT_MAX], dead[POLYCALL_ENDPOINT_MAX];
    msg_t m = {0};
    endpoint_of(sec, es);
    EXPECT(asm_polycall_peer_send(anon, es, "x", 1, "m-a1", 5000), POLYCALL_E_AUTH, "auth: no token -> E_AUTH");
    EXPECT(asm_polycall_peer_send(wrong, es, "x", 1, "m-a2", 5000), POLYCALL_E_AUTH, "auth: wrong token -> E_AUTH");
    EXPECT(asm_polycall_peer_send(member, es, "x", 1, "m-a3", 5000), POLYCALL_OK, "auth: right token delivers");
    EXPECT(recv_msg(sec, 5000, &m), POLYCALL_OK, "auth: received");
    EXPECT(asm_polycall_peer_open("asm-open", "0.0.0.0:0", NULL, &h), POLYCALL_E_CONFIG,
           "open: non-loopback without token -> E_CONFIG");
    EXPECT(asm_polycall_peer_open("bad id", "127.0.0.1:0", NULL, &h), POLYCALL_E_INVALID_ARGUMENT,
           "open: invalid node id");
    gone = open_node("asm-gone", "127.0.0.1:0", NULL);
    endpoint_of(gone, dead);
    asm_polycall_peer_close(gone);
    EXPECT(asm_polycall_peer_send(member, dead, "x", 1, "m-dead", 3000), POLYCALL_E_TRANSPORT,
           "send to a dead peer -> E_TRANSPORT");
    asm_polycall_peer_close(sec);
    asm_polycall_peer_close(anon);
    asm_polycall_peer_close(wrong);
    asm_polycall_peer_close(member);
    free(m.data);
}

typedef struct {
    polycall_peer_t h;
    int status;
} blocked_t;

static void *blocked_recv(void *arg)
{
    blocked_t *b = arg;
    char s[64], mid[64];
    unsigned char p[16];
    size_t n = 0;
    b->status = asm_polycall_peer_recv(b->h, UINT32_MAX, s, sizeof s, mid, sizeof mid, p, sizeof p, &n);
    return NULL;
}

typedef struct {
    int idx;
    const char *ep;
    int failures;
} sender_t;

static void *sender_thread(void *arg)
{
    sender_t *s = arg;
    char id[64], mid[64], payload[64];
    polycall_peer_t h = 0;
    int i;
    snprintf(id, sizeof id, "asm-tx%d", s->idx);
    if (asm_polycall_peer_open(id, NULL, NULL, &h) != POLYCALL_OK) {
        s->failures = 25;
        return NULL;
    }
    for (i = 0; i < 25; ++i) {
        snprintf(mid, sizeof mid, "m-%d-%d", s->idx, i);
        snprintf(payload, sizeof payload, "t%d-%d", s->idx, i);
        if (asm_polycall_peer_send(h, s->ep, payload, strlen(payload), mid, 10000) != POLYCALL_OK) {
            s->failures++;
        }
    }
    asm_polycall_peer_close(h);
    return NULL;
}

static void test_threads(void)
{
    polycall_peer_t r = open_node("asm-blocked", "127.0.0.1:0", NULL);
    blocked_t b = {r, 1};
    pthread_t t, pool[4];
    sender_t snd[4];
    char ep[POLYCALL_ENDPOINT_MAX], seen[100][32];
    int i, unique = 0, failures = 0;
    msg_t m = {0};

    pthread_create(&t, NULL, blocked_recv, &b);
    sleep_ms(300);
    EXPECT(asm_polycall_peer_cancel(r), POLYCALL_OK, "cancel: returns OK");
    pthread_join(t, NULL);
    EXPECT(b.status, POLYCALL_E_CANCELLED, "cancel wakes a blocked recv -> E_CANCELLED");
    b.status = 1;
    pthread_create(&t, NULL, blocked_recv, &b);
    sleep_ms(300);
    EXPECT(asm_polycall_peer_close(r), POLYCALL_OK, "close: returns OK");
    pthread_join(t, NULL);
    EXPECT(b.status, POLYCALL_E_CLOSED, "close wakes a blocked recv -> E_CLOSED");
    EXPECT(asm_polycall_peer_close(r), POLYCALL_E_INVALID_HANDLE, "double close -> E_INVALID_HANDLE");
    EXPECT(asm_polycall_peer_endpoint(r, ep, sizeof ep), POLYCALL_E_INVALID_HANDLE, "endpoint after close");
    EXPECT(asm_polycall_peer_send(r, "127.0.0.1:1", "x", 1, NULL, 100), POLYCALL_E_INVALID_HANDLE,
           "send after close");
    EXPECT(recv_msg(r, 0, &m), POLYCALL_E_INVALID_HANDLE, "recv after close");
    EXPECT(asm_polycall_peer_close(0), POLYCALL_E_INVALID_HANDLE, "close(0)");
    EXPECT(asm_polycall_peer_close(-7), POLYCALL_E_INVALID_HANDLE, "close(-7)");
    EXPECT(asm_polycall_peer_cancel(987654), POLYCALL_E_INVALID_HANDLE, "cancel(unknown handle)");

    r = open_node("asm-rx", "127.0.0.1:0", NULL);
    endpoint_of(r, ep);
    for (i = 0; i < 4; ++i) {
        snd[i].idx = i;
        snd[i].ep = ep;
        snd[i].failures = 0;
        pthread_create(&pool[i], NULL, sender_thread, &snd[i]);
    }
    for (i = 0; i < 100; ++i) {
        int j, dup = 0;
        if (recv_msg(r, 10000, &m) != POLYCALL_OK) break;
        snprintf(seen[unique], sizeof seen[unique], "%s/%s", m.sender, m.id);
        for (j = 0; j < unique; ++j) dup |= strcmp(seen[j], seen[unique]) == 0;
        if (!dup) ++unique;
    }
    for (i = 0; i < 4; ++i) {
        pthread_join(pool[i], NULL);
        failures += snd[i].failures;
    }
    check(unique == 100 && failures == 0, "concurrent senders: 4 threads x 25, all delivered once",
          "%d unique, %d failures", unique, failures);
    asm_polycall_peer_close(r);
    free(m.data);
}

static void test_interop(void)
{
    static const char raw[] = "asm\0bin\x01\xff \xc3\xa9\xe2\x9c\x93 end";
    const size_t n = sizeof raw - 1;
    const char *cli = env("POLYCALL_CLI"), *cli_peer = env("POLYCALL_CLI_PEER");
    const char *token = env("POLYCALL_DEV_TOKEN");
    char ep[POLYCALL_ENDPOINT_MAX], cmd[2048], b64[64], want[128];
    polycall_peer_t node;
    msg_t m = {0};
    FILE *f;
    int rc, st;
    char *got;

    if (!cli || !cli_peer) {
        skip("interop: polycall CLI peer -> asm", "POLYCALL_CLI/POLYCALL_CLI_PEER not set");
        skip("interop: asm -> polycall CLI peer", "POLYCALL_CLI/POLYCALL_CLI_PEER not set");
        return;
    }
    node = open_node("asm-node", "127.0.0.1:0", token);
    endpoint_of(node, ep);
    f = fopen("asm_cli_payload.bin", "wb");
    fwrite(raw, 1, n, f);
    fclose(f);
    snprintf(cmd, sizeof cmd,
             "\"%s\" peer send --to %s --payload-file asm_cli_payload.bin --from cli-node "
             "--id cli-to-asm-1 -t 5000 > asm_cli_send.log 2>&1", cli, ep);
    rc = run_command(cmd);
    st = rc == 0 ? recv_msg(node, 5000, &m) : -1;
    check(rc == 0 && st == 0 && strcmp(m.sender, "cli-node") == 0 && strcmp(m.id, "cli-to-asm-1") == 0 &&
              m.len == n && memcmp(m.data, raw, n) == 0,
          "interop: polycall CLI peer send -> asm peer (bytes, sender, id)", "rc=%d st=%d", rc, st);

    asm_polycall_peer_register(node, "cli-node", cli_peer);
    EXPECT(asm_polycall_peer_ping(node, "cli-node", 3000), POLYCALL_OK, "interop: ping the CLI node by id");
    st = asm_polycall_peer_send(node, "cli-node", raw, n, "asm-to-cli-1", 5000);
    snprintf(cmd, sizeof cmd, "\"%s\" --format json peer recv --to %s -t 5000 > asm_cli_recv.json 2>&1",
             cli, cli_peer);
    rc = run_command(cmd);
    got = slurp("asm_cli_recv.json");
    base64((const unsigned char *)raw, n, b64);
    snprintf(want, sizeof want, "\"payload_b64\":\"%s\"", b64);
    check(st == 0 && rc == 0 && got && strstr(got, "\"from\":\"asm-node\"") && strstr(got, "\"id\":\"asm-to-cli-1\"") &&
              strstr(got, want),
          "interop: asm peer -> polycall peer serve, read by polycall peer recv", "st=%d rc=%d %s", st, rc,
          got ? got : "");
    free(got);
    remove("asm_cli_payload.bin");
    remove("asm_cli_send.log");
    remove("asm_cli_recv.json");
    asm_polycall_peer_close(node);
    free(m.data);
}

int main(int argc, char **argv)
{
    char v[32];
    const char *root = argc > 1 ? argv[1] : ".";
    asm_polycall_version(v, (int)sizeof v);
    printf("asm-polycall real-core tests: libpolycall %s, ABI %d\n", v, asm_polycall_abi_version());
    if (asm_polycall_abi_version() != 1) {
        printf("FAIL abi: library speaks ABI %d, asm-polycall requires 1\n", asm_polycall_abi_version());
        return 1;
    }
    test_version();
    test_run_config(root);
    test_call();
    test_peers();
    test_auth_transport();
    test_threads();
    test_interop();
    printf("SUMMARY pass=%d fail=%d skip=%d\n", g_pass, g_fail, g_skip);
    return g_fail == 0 ? 0 : 1;
}
