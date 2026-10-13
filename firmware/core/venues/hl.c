/* Hyperliquid client.
 *
 * The device holds an "agent wallet" (API wallet): a secp256k1 key that the user
 * approves from their main wallet with an approveAgent action. Agent-signed L1
 * actions (order, cancel, updateLeverage) are accepted for the main account.
 * Withdrawals and transfers need the main wallet's own signature, so this key
 * cannot move funds out. https://hyperliquid.gitbook.io/hyperliquid-docs */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "dw_crypto.h"
#include "dw_hal.h"
#include "dw_venue.h"

static uint8_t s_key[32];
static bool s_has_key;
static char s_agent[43];      /* checksummed agent address */
static int s_ws = -1;
static uint32_t s_meta_at;

/* ---------- keys ---------- */

static void load_key(void)
{
    char hex[80];
    s_has_key = hal_store_get("hl.key", hex, sizeof hex) == 64 && dw_unhex(hex, s_key, 32) == 32;
    if (s_has_key) {
        uint8_t a[20];
        dw_secp_pubkey_address(s_key, a);
        dw_eth_address_str(a, s_agent);
    }
    dw_acct_t *acct = &g_model.acct[V_HL];
    acct->linked = s_has_key && hal_store_get("hl.acct", acct->account, sizeof acct->account) > 0;
    snprintf(acct->agent, sizeof acct->agent, "%s", s_has_key ? s_agent : "");
}

/* ---------- EIP-712 signing of L1 actions ---------- */

static void kstr(const char *s, uint8_t out[32]) { dw_keccak256((const uint8_t *)s, strlen(s), out); }

static void domain_separator(uint8_t out[32])
{
    uint8_t buf[32 * 5] = {0};
    kstr("EIP712Domain(string name,string version,uint256 chainId,address verifyingContract)", buf);
    kstr("Exchange", buf + 32);
    kstr("1", buf + 64);
    buf[96 + 30] = 0x05;  /* chainId 1337 = 0x0539 */
    buf[96 + 31] = 0x39;
    /* verifyingContract = 0x0 */
    dw_keccak256(buf, sizeof buf, out);
}

/* hash = keccak(msgpack(action) || nonce_be64 || 0x00)  (no vault address) */
static bool sign_l1(const uint8_t *mp, size_t mplen, uint64_t nonce, char *sig_json, size_t cap)
{
    uint8_t *data = malloc(mplen + 9);
    if (!data) return false;
    memcpy(data, mp, mplen);
    for (int i = 0; i < 8; i++) data[mplen + i] = (uint8_t)(nonce >> (56 - 8 * i));
    data[mplen + 8] = 0x00;
    uint8_t conn[32];
    dw_keccak256(data, mplen + 9, conn);
    free(data);

    uint8_t st[32 * 3];
    kstr("Agent(string source,bytes32 connectionId)", st);
    kstr(dw_net() == DW_NET_TESTNET ? "b" : "a", st + 32);  /* "a" = mainnet, "b" = testnet */
    memcpy(st + 64, conn, 32);
    uint8_t struct_hash[32];
    dw_keccak256(st, sizeof st, struct_hash);

    uint8_t msg[66];
    msg[0] = 0x19;
    msg[1] = 0x01;
    domain_separator(msg + 2);
    memcpy(msg + 34, struct_hash, 32);
    uint8_t digest[32];
    dw_keccak256(msg, sizeof msg, digest);

    uint8_t r[32], s[32];
    int v;
    if (!dw_secp_sign(s_key, digest, r, s, &v)) return false;
    char rh[67], sh[67];
    dw_hex(r, 32, rh, true);
    dw_hex(s, 32, sh, true);
    snprintf(sig_json, cap, "{\"r\":\"%s\",\"s\":\"%s\",\"v\":%d}", rh, sh, v);
    return true;
}

/* ---------- requests ---------- */

static int info(const char *body, dw_json_cb cb, void *ctx)
{
    return dw_http_json("POST", dw_urlf(U_HL, "/info"), NULL, body, cb, ctx);
}

static int exchange(const char *action_json, const uint8_t *mp, size_t mplen, dw_json_cb cb, void *ctx)
{
    uint64_t nonce = dw_now_ms();
    char sig[200];
    if (!sign_l1(mp, mplen, nonce, sig, sizeof sig)) return -1;
    size_t n = strlen(action_json) + 400;
    char *body = malloc(n);
    if (!body) return -1;
    snprintf(body, n, "{\"action\":%s,\"nonce\":%llu,\"signature\":%s,\"vaultAddress\":null}", action_json,
             (unsigned long long)nonce, sig);
    int rc = dw_http_json("POST", dw_urlf(U_HL, "/exchange"), NULL, body, cb, ctx);
    free(body);
    return rc;
}

/* ---------- market data ---------- */

static void on_meta(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    if (status != 200 || !cJSON_IsArray(root)) return;
    cJSON *universe = cJSON_GetObjectItem(cJSON_GetArrayItem(root, 0), "universe");
    cJSON *ctxs = cJSON_GetArrayItem(root, 1);
    int n = 0, idx = 0;
    cJSON *u;
    cJSON_ArrayForEach(u, universe) {
        const char *name = dw_jstr(u, "name");
        static const char *WANT[] = {"BTC", "ETH", "SOL", "HYPE"};
        for (int w = 0; w < 4; w++) {
            if (strcmp(name, WANT[w]) || n >= DW_MAX_MKT) continue;
            dw_mkt_t *m = &g_model.mkt[V_HL][n++];
            double keep_mid = m->mid;
            memset(m, 0, sizeof *m);
            snprintf(m->coin, sizeof m->coin, "%s", name);
            m->index = idx;
            m->sz_dec = (int)dw_jnum(u, "szDecimals");
            m->max_lev = (int)dw_jnum(u, "maxLeverage");
            cJSON *c = cJSON_GetArrayItem(ctxs, idx);
            m->prev_day = dw_jnum(c, "prevDayPx");
            m->funding_h = dw_jnum(c, "funding");
            m->oi = dw_jnum(c, "openInterest");
            m->mid = keep_mid > 0 ? keep_mid : dw_jnum(c, "midPx");
        }
        idx++;
    }
    g_model.nmkt[V_HL] = n;
    dw_model_touch();
}

static void on_ws(void *ctx, const char *msg, size_t len)
{
    (void)ctx;
    cJSON *root = cJSON_ParseWithLength(msg, len);
    if (!root) return;
    if (!strcmp(dw_jstr(root, "channel"), "allMids")) {
        cJSON *mids = cJSON_GetObjectItem(cJSON_GetObjectItem(root, "data"), "mids");
        for (int i = 0; i < g_model.nmkt[V_HL]; i++) {
            cJSON *v = cJSON_GetObjectItem(mids, g_model.mkt[V_HL][i].coin);
            if (v) g_model.mkt[V_HL][i].mid = dw_jnum(v, NULL);
        }
        dw_model_touch();
    }
    cJSON_Delete(root);
}

static void on_ws_state(void *ctx, int open)
{
    (void)ctx;
    if (open) hal_ws_send(s_ws, "{\"method\":\"subscribe\",\"subscription\":{\"type\":\"allMids\"}}");
}

/* ---------- account ---------- */

static void on_state(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    dw_acct_t *a = &g_model.acct[V_HL];
    if (status != 200 || !root) { a->error = true; return; }
    a->error = false;
    cJSON *ms = cJSON_GetObjectItem(root, "marginSummary");
    a->equity = dw_jnum(ms, "accountValue");
    a->margin_used = dw_jnum(ms, "totalMarginUsed");
    a->free = dw_jnum(root, "withdrawable");
    dw_pos_t p[DW_MAX_POS];
    int n = 0;
    cJSON *ap;
    cJSON_ArrayForEach(ap, cJSON_GetObjectItem(root, "assetPositions")) {
        cJSON *q = cJSON_GetObjectItem(ap, "position");
        if (!q || n >= DW_MAX_POS) continue;
        dw_pos_t *x = &p[n++];
        memset(x, 0, sizeof *x);
        x->venue = V_HL;
        snprintf(x->coin, sizeof x->coin, "%s", dw_jstr(q, "coin"));
        x->szi = dw_jnum(q, "szi");
        x->entry = dw_jnum(q, "entryPx");
        x->liq = dw_jnum(q, "liquidationPx");
        x->upnl = dw_jnum(q, "unrealizedPnl");
        x->margin = dw_jnum(q, "marginUsed");
        x->value = dw_jnum(q, "positionValue");
        x->lev = dw_jnum(cJSON_GetObjectItem(q, "leverage"), "value");
    }
    dw_model_set_positions(V_HL, p, n);
}

static void on_orders(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    if (status != 200 || !cJSON_IsArray(root)) return;
    dw_ord_t o[DW_MAX_ORD];
    int n = 0;
    cJSON *it;
    cJSON_ArrayForEach(it, root) {
        if (n >= DW_MAX_ORD) break;
        dw_ord_t *x = &o[n++];
        memset(x, 0, sizeof *x);
        x->venue = V_HL;
        snprintf(x->coin, sizeof x->coin, "%s", dw_jstr(it, "coin"));
        x->buy = !strcmp(dw_jstr(it, "side"), "B");
        x->sz = dw_jnum(it, "sz");
        x->oid = (uint64_t)dw_jnum(it, "oid");
        bool trig = cJSON_IsTrue(cJSON_GetObjectItem(it, "isTrigger"));
        const char *ot = dw_jstr(it, "orderType");
        x->px = trig ? dw_jnum(it, "triggerPx") : dw_jnum(it, "limitPx");
        snprintf(x->kind, sizeof x->kind, "%s", !trig ? "LIMIT" : strstr(ot, "Take") ? "TP" : "SL");
    }
    dw_model_set_orders(V_HL, o, n);
}

static void on_agents(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    if (status != 200 || !cJSON_IsArray(root)) return;
    bool ok = false;
    cJSON *it;
    cJSON_ArrayForEach(it, root) {
        if (!strcasecmp(dw_jstr(it, "address"), s_agent)) ok = true;
    }
    if (g_model.acct[V_HL].approved != ok) {
        g_model.acct[V_HL].approved = ok;
        dw_model_touch();
    }
}

/* A key without an account: the device was waiting for an approval and missed the
 * relay's answer (it left the link page, lost power, or the relay restarted).
 * Hyperliquid itself says which main wallet approved an agent. */
static uint32_t s_role_at;
static void hl_poll(void);

static void on_role(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    if (status != 200 || !root || strcmp(dw_jstr(root, "role"), "agent")) return;
    const char *user = dw_jstr(cJSON_GetObjectItem(root, "data"), "user");
    if (!*user || g_model.acct[V_HL].linked || !s_has_key) return;
    hal_store_set("hl.acct", user);
    load_key();
    dw_model_touch();
    hl_poll();
}

static void hl_poll(void)
{
    if (!s_meta_at || hal_millis() - s_meta_at > 15000) {
        s_meta_at = hal_millis();
        info("{\"type\":\"metaAndAssetCtxs\"}", on_meta, NULL);
    }
    dw_acct_t *a = &g_model.acct[V_HL];
    char body[160];
    if (!a->linked) {
        if (s_has_key && (!s_role_at || hal_millis() - s_role_at > 4000)) {
            s_role_at = hal_millis();
            snprintf(body, sizeof body, "{\"type\":\"userRole\",\"user\":\"%s\"}", s_agent);
            info(body, on_role, NULL);
        }
        return;
    }
    snprintf(body, sizeof body, "{\"type\":\"clearinghouseState\",\"user\":\"%s\"}", a->account);
    info(body, on_state, NULL);
    snprintf(body, sizeof body, "{\"type\":\"frontendOpenOrders\",\"user\":\"%s\"}", a->account);
    info(body, on_orders, NULL);
    snprintf(body, sizeof body, "{\"type\":\"extraAgents\",\"user\":\"%s\"}", a->account);
    info(body, on_agents, NULL);
}

static void hl_start(void)
{
    load_key();
    if (s_ws < 0) s_ws = hal_ws_open(dw_url(U_HL_WS), on_ws, on_ws_state, NULL);
    s_meta_at = 0;
    hl_poll();
}

/* ---------- orders ---------- */

typedef struct {
    dw_order_req_t req;
    dw_order_cb cb;
    void *ctx;
} ord_ctx_t;

static void finish(ord_ctx_t *c, bool ok, bool resting, double px, double sz, const char *msg)
{
    dw_order_res_t r = {.ok = ok, .resting = resting, .avg_px = px, .filled_sz = sz};
    snprintf(r.msg, sizeof r.msg, "%s", msg ? msg : "");
    c->cb(c->ctx, &r);
    free(c);
    hl_poll();
}

static void on_order(void *ctx, int status, cJSON *root)
{
    ord_ctx_t *c = ctx;
    if (status != 200 || !root) { finish(c, false, false, 0, 0, status < 0 ? "Network error" : "Venue error"); return; }
    if (strcmp(dw_jstr(root, "status"), "ok")) {
        finish(c, false, false, 0, 0, dw_jstr(root, "response"));
        return;
    }
    cJSON *st = cJSON_GetArrayItem(
        cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(root, "response"), "data"), "statuses"), 0);
    cJSON *f = cJSON_GetObjectItem(st, "filled");
    if (f) { finish(c, true, false, dw_jnum(f, "avgPx"), dw_jnum(f, "totalSz"), "Filled"); return; }
    if (cJSON_GetObjectItem(st, "resting")) { finish(c, true, true, c->req.limit_px, 0, "Order placed"); return; }
    finish(c, false, false, 0, 0, dw_jstr(st, "error"));
}

static void send_order(ord_ctx_t *c)
{
    dw_mkt_t *m = dw_mkt_find(V_HL, c->req.coin);
    double mid = m->mid;
    bool market = c->req.limit_px <= 0;
    double px = market ? mid * (c->req.buy ? 1 + c->req.slippage : 1 - c->req.slippage) : c->req.limit_px;
    char ps[32], ss[32];
    dw_px_str(px, 5, 6 - m->sz_dec, ps, sizeof ps);
    dw_num_str(c->req.sz, m->sz_dec, ss, sizeof ss);
    const char *tif = market ? "Ioc" : "Gtc";

    uint8_t buf[256];
    dw_mp_t mp;
    dw_mp_init(&mp, buf, sizeof buf);
    dw_mp_map(&mp, 3);
    dw_mp_str(&mp, "type"); dw_mp_str(&mp, "order");
    dw_mp_str(&mp, "orders"); dw_mp_array(&mp, 1);
    dw_mp_map(&mp, 6);
    dw_mp_str(&mp, "a"); dw_mp_uint(&mp, (uint64_t)m->index);
    dw_mp_str(&mp, "b"); dw_mp_bool(&mp, c->req.buy);
    dw_mp_str(&mp, "p"); dw_mp_str(&mp, ps);
    dw_mp_str(&mp, "s"); dw_mp_str(&mp, ss);
    dw_mp_str(&mp, "r"); dw_mp_bool(&mp, c->req.reduce_only);
    dw_mp_str(&mp, "t"); dw_mp_map(&mp, 1);
    dw_mp_str(&mp, "limit"); dw_mp_map(&mp, 1);
    dw_mp_str(&mp, "tif"); dw_mp_str(&mp, tif);
    dw_mp_str(&mp, "grouping"); dw_mp_str(&mp, "na");

    char json[320];
    snprintf(json, sizeof json,
             "{\"type\":\"order\",\"orders\":[{\"a\":%d,\"b\":%s,\"p\":\"%s\",\"s\":\"%s\",\"r\":%s,\"t\":{\"limit\":{\"tif\":\"%s\"}}}],\"grouping\":\"na\"}",
             m->index, c->req.buy ? "true" : "false", ps, ss, c->req.reduce_only ? "true" : "false", tif);
    if (mp.overflow || exchange(json, mp.buf, mp.len, on_order, c) < 0) finish(c, false, false, 0, 0, "Could not sign");
}

static void on_leverage(void *ctx, int status, cJSON *root)
{
    ord_ctx_t *c = ctx;
    if (status != 200 || strcmp(dw_jstr(root, "status"), "ok")) {
        finish(c, false, false, 0, 0, root ? dw_jstr(root, "response") : "Leverage not set");
        return;
    }
    send_order(c);
}

static int hl_order(const dw_order_req_t *req, dw_order_cb cb, void *ctx)
{
    dw_mkt_t *m = dw_mkt_find(V_HL, req->coin);
    if (!s_has_key || !g_model.acct[V_HL].linked || !m) return -1;
    ord_ctx_t *c = calloc(1, sizeof *c);
    if (!c) return -1;
    c->req = *req;
    c->cb = cb;
    c->ctx = ctx;
    if (req->leverage <= 0) { send_order(c); return 0; }

    uint8_t buf[96];
    dw_mp_t mp;
    dw_mp_init(&mp, buf, sizeof buf);
    dw_mp_map(&mp, 4);
    dw_mp_str(&mp, "type"); dw_mp_str(&mp, "updateLeverage");
    dw_mp_str(&mp, "asset"); dw_mp_uint(&mp, (uint64_t)m->index);
    dw_mp_str(&mp, "isCross"); dw_mp_bool(&mp, true);
    dw_mp_str(&mp, "leverage"); dw_mp_uint(&mp, (uint64_t)req->leverage);
    char json[128];
    snprintf(json, sizeof json, "{\"type\":\"updateLeverage\",\"asset\":%d,\"isCross\":true,\"leverage\":%d}", m->index,
             req->leverage);
    if (exchange(json, mp.buf, mp.len, on_leverage, c) < 0) { free(c); return -1; }
    return 0;
}

/* ---------- TP / SL and cancel ---------- */

/* A trigger order that closes the position at market once triggerPx trades. */
static void trigger_order(dw_mp_t *mp, char *json, size_t cap, int asset, bool close_buy, const char *ps, const char *ss,
                          const char *trig, const char *kind)
{
    dw_mp_map(mp, 6);
    dw_mp_str(mp, "a"); dw_mp_uint(mp, (uint64_t)asset);
    dw_mp_str(mp, "b"); dw_mp_bool(mp, close_buy);
    dw_mp_str(mp, "p"); dw_mp_str(mp, ps);
    dw_mp_str(mp, "s"); dw_mp_str(mp, ss);
    dw_mp_str(mp, "r"); dw_mp_bool(mp, true);
    dw_mp_str(mp, "t"); dw_mp_map(mp, 1);
    dw_mp_str(mp, "trigger"); dw_mp_map(mp, 3);
    dw_mp_str(mp, "isMarket"); dw_mp_bool(mp, true);
    dw_mp_str(mp, "triggerPx"); dw_mp_str(mp, trig);
    dw_mp_str(mp, "tpsl"); dw_mp_str(mp, kind);
    snprintf(json, cap,
             "{\"a\":%d,\"b\":%s,\"p\":\"%s\",\"s\":\"%s\",\"r\":true,\"t\":{\"trigger\":{\"isMarket\":true,\"triggerPx\":\"%s\",\"tpsl\":\"%s\"}}}",
             asset, close_buy ? "true" : "false", ps, ss, trig, kind);
}

static void on_simple(void *ctx, int status, cJSON *root)
{
    ord_ctx_t *c = ctx;
    if (status != 200 || !root) { finish(c, false, false, 0, 0, "Network error"); return; }
    if (strcmp(dw_jstr(root, "status"), "ok")) { finish(c, false, false, 0, 0, dw_jstr(root, "response")); return; }
    cJSON *sts = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(root, "response"), "data"), "statuses");
    cJSON *it;
    cJSON_ArrayForEach(it, sts) {
        const char *e = dw_jstr(it, "error");
        if (*e) { finish(c, false, false, 0, 0, e); return; }
    }
    finish(c, true, false, 0, 0, "Done");
}

static int hl_tpsl(const char *coin, bool long_pos, double sz, double tp, double sl, dw_order_cb cb, void *ctx)
{
    dw_mkt_t *m = dw_mkt_find(V_HL, coin);
    if (!s_has_key || !g_model.acct[V_HL].linked || !m || (tp <= 0 && sl <= 0)) return -1;
    ord_ctx_t *c = calloc(1, sizeof *c);
    if (!c) return -1;
    c->cb = cb;
    c->ctx = ctx;
    bool close_buy = !long_pos;
    char ss[32];
    dw_num_str(sz, m->sz_dec, ss, sizeof ss);
    uint8_t buf[400];
    dw_mp_t mp;
    dw_mp_init(&mp, buf, sizeof buf);
    int n = (tp > 0) + (sl > 0);
    dw_mp_map(&mp, 3);
    dw_mp_str(&mp, "type"); dw_mp_str(&mp, "order");
    dw_mp_str(&mp, "orders"); dw_mp_array(&mp, (uint32_t)n);
    char parts[2][300] = {"", ""};
    int k = 0;
    const double trig[2] = {tp, sl};
    const char *kind[2] = {"tp", "sl"};
    for (int i = 0; i < 2; i++) {
        if (trig[i] <= 0) continue;
        char ts[32], ps[32];
        dw_px_str(trig[i], 5, 6 - m->sz_dec, ts, sizeof ts);
        /* market trigger: p is the worst fill price we accept */
        dw_px_str(trig[i] * (close_buy ? 1.1 : 0.9), 5, 6 - m->sz_dec, ps, sizeof ps);
        trigger_order(&mp, parts[k++], sizeof parts[0], m->index, close_buy, ps, ss, ts, kind[i]);
    }
    dw_mp_str(&mp, "grouping"); dw_mp_str(&mp, "positionTpsl");
    char json[700];
    snprintf(json, sizeof json, "{\"type\":\"order\",\"orders\":[%s%s%s],\"grouping\":\"positionTpsl\"}", parts[0], k > 1 ? "," : "",
             k > 1 ? parts[1] : "");
    if (mp.overflow || exchange(json, mp.buf, mp.len, on_simple, c) < 0) { free(c); return -1; }
    return 0;
}

static int hl_cancel(const char *coin, uint64_t oid, dw_order_cb cb, void *ctx)
{
    dw_mkt_t *m = dw_mkt_find(V_HL, coin);
    if (!s_has_key || !m) return -1;
    ord_ctx_t *c = calloc(1, sizeof *c);
    if (!c) return -1;
    c->cb = cb;
    c->ctx = ctx;
    uint8_t buf[96];
    dw_mp_t mp;
    dw_mp_init(&mp, buf, sizeof buf);
    dw_mp_map(&mp, 2);
    dw_mp_str(&mp, "type"); dw_mp_str(&mp, "cancel");
    dw_mp_str(&mp, "cancels"); dw_mp_array(&mp, 1);
    dw_mp_map(&mp, 2);
    dw_mp_str(&mp, "a"); dw_mp_uint(&mp, (uint64_t)m->index);
    dw_mp_str(&mp, "o"); dw_mp_uint(&mp, oid);
    char json[128];
    snprintf(json, sizeof json, "{\"type\":\"cancel\",\"cancels\":[{\"a\":%d,\"o\":%llu}]}", m->index, (unsigned long long)oid);
    if (exchange(json, mp.buf, mp.len, on_simple, c) < 0) { free(c); return -1; }
    return 0;
}

/* ---------- candles ---------- */

typedef struct { dw_candles_cb cb; void *ctx; } cnd_ctx_t;

static void on_candles(void *ctx, int status, cJSON *root)
{
    cnd_ctx_t *c = ctx;
    dw_candle_t out[64];
    int n = 0;
    if (status == 200 && cJSON_IsArray(root)) {
        int total = cJSON_GetArraySize(root), skip = total > 64 ? total - 64 : 0, i = 0;
        cJSON *it;
        cJSON_ArrayForEach(it, root) {
            if (i++ < skip) continue;
            out[n++] = (dw_candle_t){dw_jnum(it, "t"), dw_jnum(it, "o"), dw_jnum(it, "h"), dw_jnum(it, "l"), dw_jnum(it, "c")};
        }
    }
    c->cb(c->ctx, out, n);
    free(c);
}

static int hl_candles(const char *coin, const char *interval, int count, dw_candles_cb cb, void *ctx)
{
    cnd_ctx_t *c = malloc(sizeof *c);
    if (!c) return -1;
    c->cb = cb;
    c->ctx = ctx;
    uint64_t end = dw_now_ms(), start = end - (uint64_t)dw_interval_ms(interval) * (uint64_t)count;
    char body[200];
    snprintf(body, sizeof body,
             "{\"type\":\"candleSnapshot\",\"req\":{\"coin\":\"%s\",\"interval\":\"%s\",\"startTime\":%llu,\"endTime\":%llu}}",
             coin, interval, (unsigned long long)start, (unsigned long long)end);
    return info(body, on_candles, c);
}

/* ---------- linking ---------- */

static int hl_link_begin(const char *session, char *qr, size_t cap)
{
    load_key();
    if (!s_has_key || g_model.acct[V_HL].linked) {
        /* a fresh key; a pending one (made, not yet approved) is kept, so a QR that was
         * already scanned and signed still links */
        uint8_t k[32];
        uint8_t a[20];
        do hal_random_fill(k, 32); while (!dw_secp_pubkey_address(k, a));
        char hex[65];
        dw_hex(k, 32, hex, false);
        hal_store_set("hl.key", hex);
        hal_store_set("hl.acct", NULL);
        memset(k, 0, sizeof k);
        load_key();
    }
    s_role_at = 0;
    g_model.acct[V_HL].approved = false;
    snprintf(qr, cap, "%s?v=hl&agent=%s&s=%s&n=%s%s", dw_url(U_LINK), s_agent, session, dw_net_name(), dw_relay_param());
    return 0;
}

static void hl_link_done(const char *account, const char *extra)
{
    (void)extra;
    hal_store_set("hl.acct", account);
    load_key();
    hl_poll();
}

static void hl_unlink(void)
{
    hal_store_set("hl.key", NULL);
    hal_store_set("hl.acct", NULL);
    memset(s_key, 0, sizeof s_key);
    memset(&g_model.acct[V_HL], 0, sizeof(dw_acct_t));
    load_key();
    dw_model_set_positions(V_HL, NULL, 0);
    dw_model_set_orders(V_HL, NULL, 0);
}

const dw_venue_t dw_hl = {
    .name = "Hyperliquid",
    .start = hl_start,
    .poll = hl_poll,
    .order = hl_order,
    .candles = hl_candles,
    .tpsl = hl_tpsl,
    .cancel = hl_cancel,
    .link_begin = hl_link_begin,
    .link_done = hl_link_done,
    .unlink = hl_unlink,
};
