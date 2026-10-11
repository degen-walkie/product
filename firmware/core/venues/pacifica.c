/* Pacifica client (perps on Solana).
 *
 * The device holds an Ed25519 agent key. The user binds it to their Solana account
 * from the main wallet (bind_agent_wallet). Requests are signed by serialising
 * {type, timestamp, expiry_window, data} as compact JSON with sorted keys and
 * signing the bytes with Ed25519; the signature travels base58-encoded.
 * https://docs.pacifica.fi/api-documentation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dw_crypto.h"
#include "dw_hal.h"
#include "dw_venue.h"

#define EXPIRY 5000

static uint8_t s_seed[32], s_pub[32];
static bool s_has_key;
static char s_agent[64];      /* base58 public key */
static uint32_t s_info_at;
static int s_ws = -1;

static void load_key(void)
{
    char hex[80];
    s_has_key = hal_store_get("pac.key", hex, sizeof hex) == 64 && dw_unhex(hex, s_seed, 32) == 32;
    if (s_has_key) {
        dw_ed25519_pub(s_seed, s_pub);
        dw_base58(s_pub, 32, s_agent, sizeof s_agent);
    }
    dw_acct_t *a = &g_model.acct[V_PAC];
    a->linked = s_has_key && hal_store_get("pac.acct", a->account, sizeof a->account) > 0;
    snprintf(a->agent, sizeof a->agent, "%s", s_has_key ? s_agent : "");
    a->approved = a->linked;
}

/* data_sorted: the "data" object, already compact with keys in sorted order.
 * fields: the same fields without braces, appended to the request body. */
static int signed_post(const char *path, const char *type, const char *data_sorted, const char *fields, dw_json_cb cb,
                       void *ctx)
{
    long long ts = (long long)dw_now_ms();
    size_t n = strlen(data_sorted) + 160;
    char *msg = malloc(n);
    if (!msg) return -1;
    snprintf(msg, n, "{\"data\":%s,\"expiry_window\":%d,\"timestamp\":%lld,\"type\":\"%s\"}", data_sorted, EXPIRY, ts, type);
    uint8_t sig[64];
    dw_ed25519_sign(s_seed, s_pub, (const uint8_t *)msg, strlen(msg), sig);
    free(msg);
    char sig58[100];
    dw_base58(sig, 64, sig58, sizeof sig58);

    size_t bn = strlen(fields) + 400;
    char *body = malloc(bn);
    if (!body) return -1;
    snprintf(body, bn,
             "{\"account\":\"%s\",\"agent_wallet\":\"%s\",\"signature\":\"%s\",\"timestamp\":%lld,\"expiry_window\":%d,%s}",
             g_model.acct[V_PAC].account, s_agent, sig58, ts, EXPIRY, fields);
    char url[160];
    snprintf(url, sizeof url, DW_PAC_API "%s", path);
    int rc = dw_http_json("POST", url, NULL, body, cb, ctx);
    free(body);
    return rc;
}

static int decimals_of(const char *step)
{
    const char *dot = strchr(step, '.');
    if (!dot) return 0;
    int n = (int)strlen(dot + 1);
    while (n > 0 && dot[n] == '0') n--;
    return n;
}

/* ---------- market data ---------- */

static void on_info(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (status != 200 || !cJSON_IsArray(data)) return;
    int n = 0;
    cJSON *it;
    cJSON_ArrayForEach(it, data) {
        const char *sym = dw_jstr(it, "symbol");
        if (strcmp(sym, "BTC") && strcmp(sym, "ETH") && strcmp(sym, "SOL")) continue;
        if (n >= DW_MAX_MKT) break;
        dw_mkt_t *m = &g_model.mkt[V_PAC][n];
        double keep_mid = m->mid, keep_prev = m->prev_day;
        memset(m, 0, sizeof *m);
        snprintf(m->coin, sizeof m->coin, "%s", sym);
        m->index = n;
        m->sz_dec = decimals_of(dw_jstr(it, "lot_size"));
        m->px_dec = decimals_of(dw_jstr(it, "tick_size"));
        m->max_lev = (int)dw_jnum(it, "max_leverage");
        m->mid = keep_mid;
        m->prev_day = keep_prev;
        n++;
    }
    g_model.nmkt[V_PAC] = n;
    dw_model_touch();
}

static void apply_prices(cJSON *arr)
{
    cJSON *it;
    cJSON_ArrayForEach(it, arr) {
        dw_mkt_t *m = dw_mkt_find(V_PAC, dw_jstr(it, "symbol"));
        if (!m) continue;
        m->mid = dw_jnum(it, "mid");
        m->prev_day = dw_jnum(it, "yesterday_price");
        m->funding_h = dw_jnum(it, "funding");
        m->oi = dw_jnum(it, "open_interest");
    }
    dw_model_touch();
}

static void on_ws(void *ctx, const char *msg, size_t len)
{
    (void)ctx;
    cJSON *root = cJSON_ParseWithLength(msg, len);
    if (root && !strcmp(dw_jstr(root, "channel"), "prices")) apply_prices(cJSON_GetObjectItem(root, "data"));
    cJSON_Delete(root);
}

static void on_ws_state(void *ctx, int open)
{
    (void)ctx;
    if (open) hal_ws_send(s_ws, "{\"method\":\"subscribe\",\"params\":{\"source\":\"prices\"}}");
}

/* ---------- account ---------- */

static void on_account(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    dw_acct_t *a = &g_model.acct[V_PAC];
    cJSON *d = cJSON_GetObjectItem(root, "data");
    if (status != 200 || !d) { a->error = true; return; }
    a->error = false;
    a->equity = dw_jnum(d, "account_equity");
    a->free = dw_jnum(d, "available_to_spend");
    a->margin_used = dw_jnum(d, "total_margin_used");
    dw_model_touch();
}

static void on_positions(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    cJSON *d = cJSON_GetObjectItem(root, "data");
    if (status != 200 || !cJSON_IsArray(d)) return;
    dw_pos_t p[DW_MAX_POS];
    int n = 0;
    cJSON *it;
    cJSON_ArrayForEach(it, d) {
        if (n >= DW_MAX_POS) break;
        dw_pos_t *x = &p[n++];
        memset(x, 0, sizeof *x);
        x->venue = V_PAC;
        snprintf(x->coin, sizeof x->coin, "%s", dw_jstr(it, "symbol"));
        double amt = dw_jnum(it, "amount");
        x->szi = !strcmp(dw_jstr(it, "side"), "ask") ? -amt : amt;
        x->entry = dw_jnum(it, "entry_price");
        x->margin = dw_jnum(it, "margin");
        x->liq = dw_jnum(it, "liquidation_price");
        double px = dw_price(V_PAC, x->coin);
        x->value = amt * px;
        x->upnl = (px - x->entry) * x->szi;
        x->lev = x->margin > 0 ? x->value / x->margin : 0;
    }
    dw_model_set_positions(V_PAC, p, n);
}

static void on_orders(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    cJSON *d = cJSON_GetObjectItem(root, "data");
    if (status != 200 || !cJSON_IsArray(d)) return;
    dw_ord_t o[DW_MAX_ORD];
    int n = 0;
    cJSON *it;
    cJSON_ArrayForEach(it, d) {
        if (n >= DW_MAX_ORD) break;
        dw_ord_t *x = &o[n++];
        x->venue = V_PAC;
        snprintf(x->coin, sizeof x->coin, "%s", dw_jstr(it, "symbol"));
        x->buy = !strcmp(dw_jstr(it, "side"), "bid");
        x->px = dw_jnum(it, "price");
        x->sz = dw_jnum(it, "initial_amount") - dw_jnum(it, "filled_amount");
        x->oid = (uint64_t)dw_jnum(it, "order_id");
    }
    dw_model_set_orders(V_PAC, o, n);
}

static void pac_poll(void)
{
    if (!s_info_at || hal_millis() - s_info_at > 15000) {
        s_info_at = hal_millis();
        dw_http_json("GET", DW_PAC_API "/info", NULL, NULL, on_info, NULL);
    }
    dw_acct_t *a = &g_model.acct[V_PAC];
    if (!a->linked) return;
    char url[200];
    snprintf(url, sizeof url, DW_PAC_API "/account?account=%s", a->account);
    dw_http_json("GET", url, NULL, NULL, on_account, NULL);
    snprintf(url, sizeof url, DW_PAC_API "/positions?account=%s", a->account);
    dw_http_json("GET", url, NULL, NULL, on_positions, NULL);
    snprintf(url, sizeof url, DW_PAC_API "/orders?account=%s", a->account);
    dw_http_json("GET", url, NULL, NULL, on_orders, NULL);
}

static void pac_start(void)
{
    load_key();
    if (s_ws < 0) s_ws = hal_ws_open(DW_PAC_WS, on_ws, on_ws_state, NULL);
    s_info_at = 0;
    pac_poll();
}

/* ---------- orders ---------- */

typedef struct {
    dw_order_req_t req;
    dw_order_cb cb;
    void *ctx;
} ord_ctx_t;

static void finish(ord_ctx_t *c, bool ok, bool resting, const char *msg)
{
    dw_order_res_t r = {.ok = ok, .resting = resting};
    if (ok && !resting) {
        r.avg_px = dw_price(V_PAC, c->req.coin);
        r.filled_sz = c->req.sz;
    }
    snprintf(r.msg, sizeof r.msg, "%s", msg ? msg : "");
    c->cb(c->ctx, &r);
    free(c);
    pac_poll();
}

static void on_order(void *ctx, int status, cJSON *root)
{
    ord_ctx_t *c = ctx;
    if (status == 200 && cJSON_IsTrue(cJSON_GetObjectItem(root, "success"))) {
        finish(c, true, c->req.limit_px > 0, c->req.limit_px > 0 ? "Order placed" : "Filled");
        return;
    }
    const char *err = root ? dw_jstr(root, "error") : "";
    finish(c, false, false, *err ? err : (status < 0 ? "Network error" : "Venue error"));
}

static void uuid4(char out[37])
{
    uint8_t b[16];
    hal_random_fill(b, 16);
    b[6] = (b[6] & 0x0f) | 0x40;
    b[8] = (b[8] & 0x3f) | 0x80;
    char h[33];
    dw_hex(b, 16, h, false);
    snprintf(out, 37, "%.8s-%.4s-%.4s-%.4s-%.12s", h, h + 8, h + 12, h + 16, h + 20);
}

static void send_order(ord_ctx_t *c)
{
    dw_mkt_t *m = dw_mkt_find(V_PAC, c->req.coin);
    char amt[32], coid[37], data[320], fields[320];
    dw_num_str(c->req.sz, m->sz_dec, amt, sizeof amt);
    uuid4(coid);
    const char *side = c->req.buy ? "bid" : "ask";
    const char *ro = c->req.reduce_only ? "true" : "false";
    int rc;
    if (c->req.limit_px > 0) {
        char px[32];
        dw_num_str(c->req.limit_px, m->px_dec, px, sizeof px);
        snprintf(data, sizeof data,
                 "{\"amount\":\"%s\",\"client_order_id\":\"%s\",\"price\":\"%s\",\"reduce_only\":%s,\"side\":\"%s\",\"symbol\":\"%s\",\"tif\":\"GTC\"}",
                 amt, coid, px, ro, side, m->coin);
        snprintf(fields, sizeof fields,
                 "\"symbol\":\"%s\",\"price\":\"%s\",\"amount\":\"%s\",\"side\":\"%s\",\"tif\":\"GTC\",\"reduce_only\":%s,\"client_order_id\":\"%s\"",
                 m->coin, px, amt, side, ro, coid);
        rc = signed_post("/orders/create", "create_order", data, fields, on_order, c);
    } else {
        char slip[16];
        dw_num_str(c->req.slippage * 100, 2, slip, sizeof slip);
        snprintf(data, sizeof data,
                 "{\"amount\":\"%s\",\"client_order_id\":\"%s\",\"reduce_only\":%s,\"side\":\"%s\",\"slippage_percent\":\"%s\",\"symbol\":\"%s\"}",
                 amt, coid, ro, side, slip, m->coin);
        snprintf(fields, sizeof fields,
                 "\"symbol\":\"%s\",\"reduce_only\":%s,\"amount\":\"%s\",\"side\":\"%s\",\"slippage_percent\":\"%s\",\"client_order_id\":\"%s\"",
                 m->coin, ro, amt, side, slip, coid);
        rc = signed_post("/orders/create_market", "create_market_order", data, fields, on_order, c);
    }
    if (rc < 0) finish(c, false, false, "Could not sign");
}

static void on_leverage(void *ctx, int status, cJSON *root)
{
    ord_ctx_t *c = ctx;
    if (status != 200 || !cJSON_IsTrue(cJSON_GetObjectItem(root, "success"))) {
        finish(c, false, false, root && *dw_jstr(root, "error") ? dw_jstr(root, "error") : "Leverage not set");
        return;
    }
    send_order(c);
}

static int pac_order(const dw_order_req_t *req, dw_order_cb cb, void *ctx)
{
    dw_mkt_t *m = dw_mkt_find(V_PAC, req->coin);
    if (!s_has_key || !g_model.acct[V_PAC].linked || !m) return -1;
    ord_ctx_t *c = calloc(1, sizeof *c);
    if (!c) return -1;
    c->req = *req;
    c->cb = cb;
    c->ctx = ctx;
    if (req->leverage <= 0) { send_order(c); return 0; }
    char data[96], fields[96];
    snprintf(data, sizeof data, "{\"leverage\":%d,\"symbol\":\"%s\"}", req->leverage, m->coin);
    snprintf(fields, sizeof fields, "\"symbol\":\"%s\",\"leverage\":%d", m->coin, req->leverage);
    if (signed_post("/account/leverage", "update_leverage", data, fields, on_leverage, c) < 0) { free(c); return -1; }
    return 0;
}

/* ---------- candles ---------- */

typedef struct { dw_candles_cb cb; void *ctx; } cnd_ctx_t;

static void on_candles(void *ctx, int status, cJSON *root)
{
    cnd_ctx_t *c = ctx;
    dw_candle_t out[64];
    int n = 0;
    cJSON *d = cJSON_GetObjectItem(root, "data");
    if (status == 200 && cJSON_IsArray(d)) {
        int total = cJSON_GetArraySize(d), skip = total > 64 ? total - 64 : 0, i = 0;
        cJSON *it;
        cJSON_ArrayForEach(it, d) {
            if (i++ < skip) continue;
            out[n++] = (dw_candle_t){dw_jnum(it, "t"), dw_jnum(it, "o"), dw_jnum(it, "h"), dw_jnum(it, "l"), dw_jnum(it, "c")};
        }
    }
    c->cb(c->ctx, out, n);
    free(c);
}

static int pac_candles(const char *coin, const char *interval, int count, dw_candles_cb cb, void *ctx)
{
    cnd_ctx_t *c = malloc(sizeof *c);
    if (!c) return -1;
    c->cb = cb;
    c->ctx = ctx;
    uint64_t end = dw_now_ms(), start = end - (uint64_t)dw_interval_ms(interval) * (uint64_t)count;
    char url[200];
    snprintf(url, sizeof url, DW_PAC_API "/kline?symbol=%s&interval=%s&start_time=%llu&end_time=%llu", coin, interval,
             (unsigned long long)start, (unsigned long long)end);
    return dw_http_json("GET", url, NULL, NULL, on_candles, c);
}

/* ---------- linking ---------- */

static int pac_link_begin(const char *session, char *qr, size_t cap)
{
    uint8_t k[32];
    hal_random_fill(k, 32);
    char hex[65];
    dw_hex(k, 32, hex, false);
    memset(k, 0, sizeof k);
    hal_store_set("pac.key", hex);
    hal_store_set("pac.acct", NULL);
    load_key();
    snprintf(qr, cap, "https://degenwalkie.trade/link?v=pac&agent=%s&s=%s", s_agent, session);
    return 0;
}

static void pac_link_done(const char *account, const char *extra)
{
    (void)extra;
    hal_store_set("pac.acct", account);
    load_key();
    pac_poll();
}

static void pac_unlink(void)
{
    hal_store_set("pac.key", NULL);
    hal_store_set("pac.acct", NULL);
    memset(s_seed, 0, sizeof s_seed);
    memset(&g_model.acct[V_PAC], 0, sizeof(dw_acct_t));
    load_key();
    dw_model_set_positions(V_PAC, NULL, 0);
    dw_model_set_orders(V_PAC, NULL, 0);
}

const dw_venue_t dw_pac = {
    .name = "Pacifica",
    .start = pac_start,
    .poll = pac_poll,
    .order = pac_order,
    .candles = pac_candles,
    .link_begin = pac_link_begin,
    .link_done = pac_link_done,
    .unlink = pac_unlink,
};
