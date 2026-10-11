#include "dw_model.h"

#include <stdlib.h>
#include <string.h>

#include "dw_hal.h"

dw_model_t g_model;

static const char *VENUE_NAMES[V_COUNT] = {"Hyperliquid", "Pacifica", "Lighter"};
static const char *VENUE_CHAINS[V_COUNT] = {"HyperCore", "Solana", "Robinhood Chain"};

const char *dw_venue_name(int v) { return v >= 0 && v < V_COUNT ? VENUE_NAMES[v] : "?"; }
const char *dw_venue_chain(int v) { return v >= 0 && v < V_COUNT ? VENUE_CHAINS[v] : "?"; }

dw_mkt_t *dw_mkt_find(int venue, const char *coin)
{
    if (venue < 0 || venue >= V_COUNT) return NULL;
    for (int i = 0; i < g_model.nmkt[venue]; i++)
        if (!strcmp(g_model.mkt[venue][i].coin, coin)) return &g_model.mkt[venue][i];
    return NULL;
}

dw_pos_t *dw_pos_find(int venue, const char *coin)
{
    for (int i = 0; i < g_model.npos; i++)
        if (g_model.pos[i].venue == venue && !strcmp(g_model.pos[i].coin, coin)) return &g_model.pos[i];
    return NULL;
}

double dw_price(int venue, const char *coin)
{
    dw_mkt_t *m = dw_mkt_find(venue, coin);
    if (m && m->mid > 0) return m->mid;
    for (int v = 0; v < V_COUNT; v++) {
        m = dw_mkt_find(v, coin);
        if (m && m->mid > 0) return m->mid;
    }
    return 0;
}

double dw_change_pct(int venue, const char *coin)
{
    dw_mkt_t *m = dw_mkt_find(venue, coin);
    if (!m || m->prev_day <= 0 || m->mid <= 0) return 0;
    return (m->mid - m->prev_day) / m->prev_day * 100.0;
}

void dw_model_set_positions(int venue, const dw_pos_t *p, int n)
{
    dw_pos_t keep[DW_MAX_POS];
    int k = 0;
    for (int i = 0; i < g_model.npos; i++)
        if (g_model.pos[i].venue != venue) keep[k++] = g_model.pos[i];
    for (int i = 0; i < n && k < DW_MAX_POS; i++) keep[k++] = p[i];
    memcpy(g_model.pos, keep, sizeof(dw_pos_t) * k);
    g_model.npos = k;
    dw_model_touch();
}

void dw_model_set_orders(int venue, const dw_ord_t *o, int n)
{
    dw_ord_t keep[DW_MAX_ORD];
    int k = 0;
    for (int i = 0; i < g_model.nord; i++)
        if (g_model.ord[i].venue != venue) keep[k++] = g_model.ord[i];
    for (int i = 0; i < n && k < DW_MAX_ORD; i++) keep[k++] = o[i];
    memcpy(g_model.ord, keep, sizeof(dw_ord_t) * k);
    g_model.nord = k;
    dw_model_touch();
}

/* ---------------- HTTP + JSON ---------------- */

typedef struct { dw_json_cb cb; void *ctx; } jreq_t;

static void on_http(void *ctx, int status, const char *body, size_t len)
{
    jreq_t *r = ctx;
    cJSON *root = body && len ? cJSON_ParseWithLength(body, len) : NULL;
    r->cb(r->ctx, status, root);
    cJSON_Delete(root);
    free(r);
}

int dw_http_json(const char *method, const char *url, const char *headers, const char *body, dw_json_cb cb, void *ctx)
{
    jreq_t *r = malloc(sizeof *r);
    if (!r) return -1;
    r->cb = cb;
    r->ctx = ctx;
    if (!headers && body) headers = "Content-Type: application/json\n";
    int rc = hal_http(method, url, headers, body, on_http, r);
    if (rc < 0) free(r);
    return rc;
}

double dw_jnum(const cJSON *o, const char *key)
{
    const cJSON *v = key ? cJSON_GetObjectItemCaseSensitive(o, key) : o;
    if (cJSON_IsNumber(v)) return v->valuedouble;
    if (cJSON_IsString(v) && v->valuestring) return atof(v->valuestring);
    if (cJSON_IsBool(v)) return cJSON_IsTrue(v) ? 1 : 0;
    return 0;
}

const char *dw_jstr(const cJSON *o, const char *key)
{
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
    return cJSON_IsString(v) && v->valuestring ? v->valuestring : "";
}
