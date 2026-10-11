/* Hashing, signing and encodings used by the venue clients. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Keccak-256 (Ethereum flavour, not SHA3-256). */
void dw_keccak256(const uint8_t *in, size_t len, uint8_t out[32]);

/* secp256k1 (Hyperliquid agent wallet). */
bool dw_secp_pubkey_address(const uint8_t priv[32], uint8_t addr[20]);
/* Signs a 32-byte digest; r, s and v (27/28) as Ethereum expects. */
bool dw_secp_sign(const uint8_t priv[32], const uint8_t digest[32], uint8_t r[32], uint8_t s[32], int *v);

/* Ed25519 (Pacifica agent key, Solana style). */
void dw_ed25519_pub(const uint8_t seed[32], uint8_t pub[32]);
void dw_ed25519_sign(const uint8_t seed[32], const uint8_t pub[32], const uint8_t *msg, size_t len, uint8_t sig[64]);

/* Encodings. Return the number of chars written (excluding NUL). */
size_t dw_hex(const uint8_t *in, size_t len, char *out, bool prefix);
int dw_unhex(const char *in, uint8_t *out, size_t cap);
size_t dw_base58(const uint8_t *in, size_t len, char *out, size_t cap);
/* EIP-55 checksummed address string, "0x" + 40 chars. */
void dw_eth_address_str(const uint8_t addr[20], char out[43]);

/* Minimal MessagePack writer (enough for Hyperliquid actions). */
typedef struct { uint8_t *buf; size_t len, cap; bool overflow; } dw_mp_t;
void dw_mp_init(dw_mp_t *m, uint8_t *buf, size_t cap);
void dw_mp_map(dw_mp_t *m, uint32_t n);
void dw_mp_array(dw_mp_t *m, uint32_t n);
void dw_mp_str(dw_mp_t *m, const char *s);
void dw_mp_uint(dw_mp_t *m, uint64_t v);
void dw_mp_bool(dw_mp_t *m, bool v);
void dw_mp_nil(dw_mp_t *m);
