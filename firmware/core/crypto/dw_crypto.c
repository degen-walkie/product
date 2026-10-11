#include "dw_crypto.h"

#include <string.h>

#include "dw_hal.h"
#include "monocypher-ed25519.h"
#include "secp256k1.h"
#include "secp256k1_recovery.h"

/* ---------------- Keccak-256 ---------------- */

static const uint64_t RC[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL};
static const int ROTC[24] = {1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44};
static const int PILN[24] = {10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20, 14, 22, 9, 6, 1};

#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))

static void keccakf(uint64_t st[25])
{
    uint64_t t, bc[5];
    for (int r = 0; r < 24; r++) {
        for (int i = 0; i < 5; i++) bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];
        for (int i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
            for (int j = 0; j < 25; j += 5) st[j + i] ^= t;
        }
        t = st[1];
        for (int i = 0; i < 24; i++) {
            int j = PILN[i];
            bc[0] = st[j];
            st[j] = ROTL64(t, ROTC[i]);
            t = bc[0];
        }
        for (int j = 0; j < 25; j += 5) {
            for (int i = 0; i < 5; i++) bc[i] = st[j + i];
            for (int i = 0; i < 5; i++) st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }
        st[0] ^= RC[r];
    }
}

static void absorb_block(uint64_t st[25], const uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n / 8; i++) {
        uint64_t lane = 0;
        for (int b = 0; b < 8; b++) lane |= (uint64_t)p[i * 8 + b] << (8 * b);
        st[i] ^= lane;
    }
}

void dw_keccak256(const uint8_t *in, size_t len, uint8_t out[32])
{
    enum { RATE = 136 };
    uint64_t st[25] = {0};
    while (len >= RATE) {
        absorb_block(st, in, RATE);
        keccakf(st);
        in += RATE;
        len -= RATE;
    }
    uint8_t last[RATE] = {0};
    memcpy(last, in, len);
    last[len] = 0x01;
    last[RATE - 1] |= 0x80;
    absorb_block(st, last, RATE);
    keccakf(st);
    for (int i = 0; i < 32; i++) out[i] = (uint8_t)(st[i / 8] >> (8 * (i % 8)));
}

/* ---------------- secp256k1 ---------------- */

static secp256k1_context *secp_ctx(void)
{
    static secp256k1_context *ctx;
    if (!ctx) {
        ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
        uint8_t seed[32];
        hal_random_fill(seed, sizeof seed);
        (void)secp256k1_context_randomize(ctx, seed);  /* side-channel blinding */
    }
    return ctx;
}

bool dw_secp_pubkey_address(const uint8_t priv[32], uint8_t addr[20])
{
    secp256k1_pubkey pk;
    if (!secp256k1_ec_pubkey_create(secp_ctx(), &pk, priv)) return false;
    uint8_t ser[65];
    size_t n = sizeof ser;
    secp256k1_ec_pubkey_serialize(secp_ctx(), ser, &n, &pk, SECP256K1_EC_UNCOMPRESSED);
    uint8_t h[32];
    dw_keccak256(ser + 1, 64, h);
    memcpy(addr, h + 12, 20);
    return true;
}

bool dw_secp_sign(const uint8_t priv[32], const uint8_t digest[32], uint8_t r[32], uint8_t s[32], int *v)
{
    secp256k1_ecdsa_recoverable_signature sig;
    if (!secp256k1_ecdsa_sign_recoverable(secp_ctx(), &sig, digest, priv, NULL, NULL)) return false;
    uint8_t rs[64];
    int recid = 0;
    secp256k1_ecdsa_recoverable_signature_serialize_compact(secp_ctx(), rs, &recid, &sig);
    memcpy(r, rs, 32);
    memcpy(s, rs + 32, 32);
    *v = 27 + recid;
    return true;
}

/* ---------------- Ed25519 ---------------- */

void dw_ed25519_pub(const uint8_t seed[32], uint8_t pub[32])
{
    uint8_t sk[64], s[32];
    memcpy(s, seed, 32);
    crypto_ed25519_key_pair(sk, pub, s);  /* wipes s */
    crypto_wipe(sk, sizeof sk);
}

void dw_ed25519_sign(const uint8_t seed[32], const uint8_t pub[32], const uint8_t *msg, size_t len, uint8_t sig[64])
{
    uint8_t sk[64];
    memcpy(sk, seed, 32);
    memcpy(sk + 32, pub, 32);
    crypto_ed25519_sign(sig, sk, msg, len);
    crypto_wipe(sk, sizeof sk);
}

/* ---------------- Encodings ---------------- */

size_t dw_hex(const uint8_t *in, size_t len, char *out, bool prefix)
{
    static const char H[] = "0123456789abcdef";
    size_t o = 0;
    if (prefix) { out[o++] = '0'; out[o++] = 'x'; }
    for (size_t i = 0; i < len; i++) {
        out[o++] = H[in[i] >> 4];
        out[o++] = H[in[i] & 15];
    }
    out[o] = 0;
    return o;
}

static int nib(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int dw_unhex(const char *in, uint8_t *out, size_t cap)
{
    if (in[0] == '0' && (in[1] == 'x' || in[1] == 'X')) in += 2;
    size_t n = strlen(in);
    if (n % 2 || n / 2 > cap) return -1;
    for (size_t i = 0; i < n / 2; i++) {
        int a = nib(in[2 * i]), b = nib(in[2 * i + 1]);
        if (a < 0 || b < 0) return -1;
        out[i] = (uint8_t)(a << 4 | b);
    }
    return (int)(n / 2);
}

size_t dw_base58(const uint8_t *in, size_t len, char *out, size_t cap)
{
    static const char A[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    uint8_t digits[128] = {0};
    size_t nd = 1, zeros = 0;
    while (zeros < len && in[zeros] == 0) zeros++;
    for (size_t i = 0; i < len; i++) {
        unsigned carry = in[i];
        for (size_t j = 0; j < nd; j++) {
            carry += (unsigned)digits[j] << 8;
            digits[j] = carry % 58;
            carry /= 58;
        }
        while (carry && nd < sizeof digits) {
            digits[nd++] = carry % 58;
            carry /= 58;
        }
    }
    size_t o = 0;
    for (size_t i = 0; i < zeros && o + 1 < cap; i++) out[o++] = '1';
    if (!(len == zeros)) {
        for (size_t i = nd; i > 0 && o + 1 < cap; i--) out[o++] = A[digits[i - 1]];
    }
    out[o] = 0;
    return o;
}

void dw_eth_address_str(const uint8_t addr[20], char out[43])
{
    char lower[41];
    dw_hex(addr, 20, lower, false);
    uint8_t h[32];
    dw_keccak256((const uint8_t *)lower, 40, h);
    out[0] = '0';
    out[1] = 'x';
    for (int i = 0; i < 40; i++) {
        char c = lower[i];
        int bit = (h[i / 2] >> (i % 2 ? 0 : 4)) & 0xF;
        out[2 + i] = (c >= 'a' && bit >= 8) ? (char)(c - 32) : c;
    }
    out[42] = 0;
}

/* ---------------- MessagePack ---------------- */

static void put(dw_mp_t *m, const void *p, size_t n)
{
    if (m->len + n > m->cap) { m->overflow = true; return; }
    memcpy(m->buf + m->len, p, n);
    m->len += n;
}
static void put8(dw_mp_t *m, uint8_t b) { put(m, &b, 1); }
static void putbe(dw_mp_t *m, uint64_t v, int bytes)
{
    for (int i = bytes - 1; i >= 0; i--) put8(m, (uint8_t)(v >> (8 * i)));
}

void dw_mp_init(dw_mp_t *m, uint8_t *buf, size_t cap) { m->buf = buf; m->cap = cap; m->len = 0; m->overflow = false; }

void dw_mp_map(dw_mp_t *m, uint32_t n)
{
    if (n < 16) put8(m, 0x80 | n);
    else if (n < 65536) { put8(m, 0xde); putbe(m, n, 2); }
    else { put8(m, 0xdf); putbe(m, n, 4); }
}

void dw_mp_array(dw_mp_t *m, uint32_t n)
{
    if (n < 16) put8(m, 0x90 | n);
    else if (n < 65536) { put8(m, 0xdc); putbe(m, n, 2); }
    else { put8(m, 0xdd); putbe(m, n, 4); }
}

void dw_mp_str(dw_mp_t *m, const char *s)
{
    size_t n = strlen(s);
    if (n < 32) put8(m, 0xa0 | (uint8_t)n);
    else if (n < 256) { put8(m, 0xd9); put8(m, (uint8_t)n); }
    else if (n < 65536) { put8(m, 0xda); putbe(m, n, 2); }
    else { put8(m, 0xdb); putbe(m, n, 4); }
    put(m, s, n);
}

void dw_mp_uint(dw_mp_t *m, uint64_t v)
{
    if (v < 128) put8(m, (uint8_t)v);
    else if (v < 256) { put8(m, 0xcc); put8(m, (uint8_t)v); }
    else if (v < 65536) { put8(m, 0xcd); putbe(m, v, 2); }
    else if (v <= 0xffffffffULL) { put8(m, 0xce); putbe(m, v, 4); }
    else { put8(m, 0xcf); putbe(m, v, 8); }
}

void dw_mp_bool(dw_mp_t *m, bool v) { put8(m, v ? 0xc3 : 0xc2); }
void dw_mp_nil(dw_mp_t *m) { put8(m, 0xc0); }
