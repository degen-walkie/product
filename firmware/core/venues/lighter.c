/* Lighter client (perps, Robinhood Chain).
 *
 * Lighter accounts accept up to 255 API keys. The device's key is registered from
 * the main wallet (ChangePubKey); with it the device can create and cancel orders
 * and set leverage. Withdrawals from an API key only go to the account's own L1
 * address ("secure withdrawal"), and this client never builds one.
 *
 * NOTE: Lighter signs transactions with Schnorr over ECgFp5 and Poseidon2 hashing
 * (their Go signer). That signer is not ported yet: this client signs the tx_info
 * with an Ed25519 stand-in, which the simulator's mock accepts. Everything else
 * (endpoints, tx types, integer scaling, nonces) follows the real API. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dw_crypto.h"
#include "dw_hal.h"
#include "dw_venue.h"

enum { TX_CREATE_ORDER = 14, TX_UPDATE_LEVERAGE = 20 };
enum { ORDER_LIMIT = 0, ORDER_MARKET = 1 };
enum { TIF_IOC = 0, TIF_GTT = 1 };

static uint8_t s_seed[32], s_pub[32];
static bool s_has_key;
static long s_acct_idx = -1;
static int s_key_idx = -1;
static uint32_t s_books_at;

static void load_key(void)
{
    char buf[80];
    s_has_key = hal_store_get("lit.key", buf, sizeof buf) == 64 && dw_unhex(buf, s_seed, 32) == 32;
    if (s_has_key) dw_ed25519_pub(s_seed, s_pub);
    dw_acct_t *a = &g_model.acct[V_LIT];
    a->linked = false;
    if (s_has_key && hal_store_get("lit.acct", buf, sizeof buf) > 0 && sscanf(buf, "%ld:%d", &s_acct_idx, &s_key_idx) == 2) {
        a->linked = true;
        snprintf(a->account, sizeof a->account, "#%ld", s_acct_idx);
        snprintf(a->agent, sizeof a->agent, "API key #%d", s_key_idx);
    } else {
        a->agent[0] = 0;
    }
    a->approved = a->linked;
}

static void b64(const uint8_t *in, size_t n, char *out)
{
    static const char T[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t o = 0;
    for (size_t i = 0; i < n; i += 3) {
        uint32_t v = (uint32_t)in[i] << 16 | (i + 1 < n ? (uint32_t)in[i + 1] << 8 : 0) | (i + 2 < n ? in[i + 2] : 0);
        out[o++] = T[v >> 18 & 63];
        out[o++] = T[v >> 12 & 63];
        out[o++] = i + 1 < n ? T[v >> 6 & 63] : '=';
        out[o++] = i + 2 < n ? T[v & 63] : '=';
    }
    out[o] = 0;
}

static void urlenc(const char *in, char *out, size_t cap)
{
    static const char H[] = "0123456789ABCDEF";
    size_t o = 0;
    for (; *in && o + 4 < cap; in++) {
        unsigned char c = (unsigned char)*in;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') {
            out[o++] = (char)c;
        } else {
            out[o++] = '%';
            out[o++] = H[c >> 4];
            out[o++] = H[c & 15];
        }
    }
    out[o] = 0;
}

/* ---------- market data ---------- */

static void on_books(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    cJSON *arr = cJSON_GetObjectItem(root, "order_books");
    if (status != 200 || !cJSON_IsArray(arr)) return;
    int n = 0;
    cJSON *it;
    cJSON_ArrayForEach(it, arr) {
        const char *sym = dw_jstr(it, "symbol");
        if (strcmp(sym, "BTC") && strcmp(sym, "ETH") && strcmp(sym, "SOL")) continue;
        if (n >= DW_MAX_MKT) break;
        dw_mkt_t *m = &g_model.mkt[V_LIT][n++];
        double mid = m->mid, prev = m->prev_day;
        memset(m, 0, sizeof *m);
        snprintf(m->coin, sizeof m->coin, "%s", sym);
        m->index = (int)dw_jnum(it, "market_id");
        m->sz_dec = (int)dw_jnum(it, "supported_size_decimals");
        m->px_dec = (int)dw_jnum(it, "supported_price_decimals");
        m->max_lev = 20;
        m->mid = mid;
        m->prev_day = prev;
    }
    g_model.nmkt[V_LIT] = n;
    dw_model_touch();
}

static void on_details(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    cJSON *arr = cJSON_GetObjectItem(root, "order_book_details");
    if (status != 200 || !cJSON_IsArray(arr)) return;
    cJSON *it;
    cJSON_ArrayForEach(it, arr) {
        dw_mkt_t *m = dw_mkt_find(V_LIT, dw_jstr(it, "symbol"));
        if (!m) continue;
        m->mid = dw_jnum(it, "last_trade_price");
        double chg = dw_jnum(it, "daily_price_change");
        m->prev_day = m->mid / (1 + chg / 100.0);
        m->oi = dw_jnum(it, "open_interest");
    }
    dw_model_touch();
}

/* ---------- account ---------- */

static void on_account(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    dw_acct_t *a = &g_model.acct[V_LIT];
    cJSON *acc = cJSON_GetArrayItem(cJSON_GetObjectItem(root, "accounts"), 0);
    if (status != 200 || !acc) { a->error = true; return; }
    a->error = false;
    a->equity = dw_jnum(acc, "total_asset_value");
    a->free = dw_jnum(acc, "available_balance");
    a->margin_used = a->equity - a->free;
    dw_pos_t p[DW_MAX_POS];
    int n = 0;
    cJSON *it;
    cJSON_ArrayForEach(it, cJSON_GetObjectItem(acc, "positions")) {
        double sz = dw_jnum(it, "position");
        if (sz == 0 || n >= DW_MAX_POS) continue;
        dw_pos_t *x = &p[n++];
        memset(x, 0, sizeof *x);
        x->venue = V_LIT;
        snprintf(x->coin, sizeof x->coin, "%s", dw_jstr(it, "symbol"));
        x->szi = dw_jnum(it, "sign") < 0 ? -sz : sz;
        x->entry = dw_jnum(it, "avg_entry_price");
        x->value = dw_jnum(it, "position_value");
        x->upnl = dw_jnum(it, "unrealized_pnl");
        x->liq = dw_jnum(it, "liquidation_price");
        double imf = dw_jnum(it, "initial_margin_fraction");
        x->lev = imf > 0 ? 100.0 / imf : 0;
        x->margin = x->lev > 0 ? x->value / x->lev : 0;
    }
    dw_model_set_positions(V_LIT, p, n);
}

static void lit_poll(void)
{
    if (!s_books_at || hal_millis() - s_books_at > 15000) {
        s_books_at = hal_millis();
        dw_http_json("GET", dw_urlf(U_LIT, "/orderBooks"), NULL, NULL, on_books, NULL);
    }
    dw_http_json("GET", dw_urlf(U_LIT, "/orderBookDetails"), NULL, NULL, on_details, NULL);
    if (!g_model.acct[V_LIT].linked) return;
    char url[160];
    snprintf(url, sizeof url, "%s" "/account?by=index&value=%ld", dw_url(U_LIT), s_acct_idx);
    dw_http_json("GET", url, NULL, NULL, on_account, NULL);
}

static void lit_start(void)
{
    load_key();
    s_books_at = 0;
    lit_poll();
}

/* ---------- transactions ---------- */

typedef struct {
    dw_order_req_t req;
    dw_order_cb cb;
    void *ctx;
    int step;           /* 0 = leverage pending, 1 = order */
    long long nonce;
} ord_ctx_t;

static void finish(ord_ctx_t *c, bool ok, bool resting, const char *msg)
{
    dw_order_res_t r = {.ok = ok, .resting = resting};
    if (ok && !resting) {
        r.avg_px = dw_price(V_LIT, c->req.coin);
        r.filled_sz = c->req.sz;
    }
    snprintf(r.msg, sizeof r.msg, "%s", msg ? msg : "");
    c->cb(c->ctx, &r);
    free(c);
    lit_poll();
}

static void next_step(ord_ctx_t *c);

static void on_sent(void *ctx, int status, cJSON *root)
{
    ord_ctx_t *c = ctx;
    if (status != 200 || dw_jnum(root, "code") != 200) {
        const char *m = root ? dw_jstr(root, "message") : "";
        finish(c, false, false, *m ? m : "Venue error");
        return;
    }
    if (c->step == 0) { c->step = 1; next_step(c); return; }
    finish(c, true, c->req.limit_px > 0, c->req.limit_px > 0 ? "Order placed" : "Filled");
}

/* tx_body: the tx_info fields without Sig, as a JSON object body (no braces). */
static int send_tx(ord_ctx_t *c, int tx_type, const char *tx_body)
{
    char unsigned_json[512];
    snprintf(unsigned_json, sizeof unsigned_json, "{%s}", tx_body);
    uint8_t sig[64];
    dw_ed25519_sign(s_seed, s_pub, (const uint8_t *)unsigned_json, strlen(unsigned_json), sig);
    char sig64[100];
    b64(sig, 64, sig64);
    char tx_info[640];
    snprintf(tx_info, sizeof tx_info, "{%s,\"Sig\":\"%s\"}", tx_body, sig64);
    char enc[1400], body[1500];
    urlenc(tx_info, enc, sizeof enc);
    snprintf(body, sizeof body, "tx_type=%d&tx_info=%s", tx_type, enc);
    return dw_http_json("POST", dw_urlf(U_LIT, "/sendTx"), "Content-Type: application/x-www-form-urlencoded\n", body, on_sent, c);
}

static void on_nonce(void *ctx, int status, cJSON *root)
{
    ord_ctx_t *c = ctx;
    if (status != 200) { finish(c, false, false, "No nonce"); return; }
    c->nonce = (long long)dw_jnum(root, "nonce");
    dw_mkt_t *m = dw_mkt_find(V_LIT, c->req.coin);
    long long expired = (long long)dw_now_ms() + 10 * 60 * 1000;
    char tx[480];
    int rc;
    if (c->step == 0) {
        snprintf(tx, sizeof tx,
                 "\"AccountIndex\":%ld,\"ApiKeyIndex\":%d,\"MarketIndex\":%d,\"InitialMarginFraction\":%d,\"MarginMode\":0,\"ExpiredAt\":%lld,\"Nonce\":%lld",
                 s_acct_idx, s_key_idx, m->index, 10000 / c->req.leverage, expired, c->nonce);
        rc = send_tx(c, TX_UPDATE_LEVERAGE, tx);
    } else {
        bool market = c->req.limit_px <= 0;
        double px = market ? m->mid * (c->req.buy ? 1 + c->req.slippage : 1 - c->req.slippage) : c->req.limit_px;
        long long base = llround(c->req.sz * pow(10, m->sz_dec));
        long long price = llround(px * pow(10, m->px_dec));
        uint8_t r[4];
        hal_random_fill(r, 4);
        long long coi = ((long long)r[0] << 24 | r[1] << 16 | r[2] << 8 | r[3]) & 0x7fffffff;
        snprintf(tx, sizeof tx,
                 "\"AccountIndex\":%ld,\"ApiKeyIndex\":%d,\"MarketIndex\":%d,\"ClientOrderIndex\":%lld,\"BaseAmount\":%lld,\"Price\":%lld,"
                 "\"IsAsk\":%d,\"Type\":%d,\"TimeInForce\":%d,\"ReduceOnly\":%d,\"TriggerPrice\":0,\"OrderExpiry\":%lld,\"ExpiredAt\":%lld,\"Nonce\":%lld",
                 s_acct_idx, s_key_idx, m->index, coi, base, price, c->req.buy ? 0 : 1, market ? ORDER_MARKET : ORDER_LIMIT,
                 market ? TIF_IOC : TIF_GTT, c->req.reduce_only ? 1 : 0,
                 market ? 0LL : (long long)dw_now_ms() + 28LL * 24 * 3600 * 1000, expired, c->nonce);
        rc = send_tx(c, TX_CREATE_ORDER, tx);
    }
    if (rc < 0) finish(c, false, false, "Could not sign");
}

static void next_step(ord_ctx_t *c)
{
    char url[200];
    snprintf(url, sizeof url, "%s" "/nextNonce?account_index=%ld&api_key_index=%d", dw_url(U_LIT), s_acct_idx, s_key_idx);
    if (dw_http_json("GET", url, NULL, NULL, on_nonce, c) < 0) finish(c, false, false, "Network error");
}

static int lit_order(const dw_order_req_t *req, dw_order_cb cb, void *ctx)
{
    if (!s_has_key || !g_model.acct[V_LIT].linked || !dw_mkt_find(V_LIT, req->coin)) return -1;
    ord_ctx_t *c = calloc(1, sizeof *c);
    if (!c) return -1;
    c->req = *req;
    c->cb = cb;
    c->ctx = ctx;
    c->step = req->leverage > 0 ? 0 : 1;
    next_step(c);
    return 0;
}

/* ---------- candles ---------- */

typedef struct { dw_candles_cb cb; void *ctx; } cnd_ctx_t;

static void on_candles(void *ctx, int status, cJSON *root)
{
    cnd_ctx_t *c = ctx;
    dw_candle_t out[64];
    int n = 0;
    cJSON *arr = cJSON_GetObjectItem(root, "candlesticks");
    if (status == 200 && cJSON_IsArray(arr)) {
        int total = cJSON_GetArraySize(arr), skip = total > 64 ? total - 64 : 0, i = 0;
        cJSON *it;
        cJSON_ArrayForEach(it, arr) {
            if (i++ < skip) continue;
            out[n++] = (dw_candle_t){dw_jnum(it, "timestamp"), dw_jnum(it, "open"), dw_jnum(it, "high"), dw_jnum(it, "low"),
                                     dw_jnum(it, "close")};
        }
    }
    c->cb(c->ctx, out, n);
    free(c);
}

static int lit_candles(const char *coin, const char *interval, int count, dw_candles_cb cb, void *ctx)
{
    dw_mkt_t *m = dw_mkt_find(V_LIT, coin);
    if (!m) return -1;
    cnd_ctx_t *c = malloc(sizeof *c);
    if (!c) return -1;
    c->cb = cb;
    c->ctx = ctx;
    uint64_t end = dw_now_ms(), start = end - (uint64_t)dw_interval_ms(interval) * (uint64_t)count;
    char url[220];
    snprintf(url, sizeof url, "%s" "/candlesticks?market_id=%d&resolution=%s&start_timestamp=%llu&end_timestamp=%llu&count_back=%d", dw_url(U_LIT),
             m->index, interval, (unsigned long long)start, (unsigned long long)end, count);
    return dw_http_json("GET", url, NULL, NULL, on_candles, c);
}

/* ---------- linking ---------- */

static int lit_link_begin(const char *session, char *qr, size_t cap)
{
    uint8_t k[32];
    hal_random_fill(k, 32);
    char hex[65], pub[65];
    dw_hex(k, 32, hex, false);
    memset(k, 0, sizeof k);
    hal_store_set("lit.key", hex);
    hal_store_set("lit.acct", NULL);
    load_key();
    dw_hex(s_pub, 32, pub, true);
    snprintf(qr, cap, "%s?v=lit&pubkey=%s&s=%s&n=%s%s", dw_url(U_LINK), pub, session, dw_net_name(), dw_relay_param());
    return 0;
}

/* account = account index, extra = API key index */
static void lit_link_done(const char *account, const char *extra)
{
    char buf[48];
    snprintf(buf, sizeof buf, "%s:%s", account, extra && *extra ? extra : "3");
    hal_store_set("lit.acct", buf);
    load_key();
    lit_poll();
}

static void lit_unlink(void)
{
    hal_store_set("lit.key", NULL);
    hal_store_set("lit.acct", NULL);
    memset(s_seed, 0, sizeof s_seed);
    memset(&g_model.acct[V_LIT], 0, sizeof(dw_acct_t));
    load_key();
    dw_model_set_positions(V_LIT, NULL, 0);
}

const dw_venue_t dw_lit = {
    .name = "Lighter",
    .start = lit_start,
    .poll = lit_poll,
    .order = lit_order,
    .candles = lit_candles,
    .link_begin = lit_link_begin,
    .link_done = lit_link_done,
    .unlink = lit_unlink,
};
