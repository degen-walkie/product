/* Shared state: markets, positions, accounts, rooms, news. Venue clients write it,
 * screens read it. Every write bumps g_model.version so screens know to redraw. */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "cJSON.h"

typedef enum { V_HL, V_PAC, V_LIT, V_COUNT } dw_venue_id_t;

#define DW_MAX_MKT 8
#define DW_MAX_POS 16
#define DW_MAX_ORD 16
#define DW_MAX_ROOMS 8
#define DW_MAX_NEWS 10
#define DW_MAX_MSG 24

typedef struct {
    char coin[12];
    int index;          /* venue asset index / market id */
    int sz_dec;         /* size decimals */
    int px_dec;         /* price decimals (Lighter integer scaling) */
    int max_lev;
    double mid, prev_day, funding_h, oi;
} dw_mkt_t;

typedef struct {
    int venue;
    char coin[12];
    double szi;         /* signed size, base units */
    double entry, liq, upnl, margin, lev, value;
} dw_pos_t;

typedef struct {
    int venue;
    char coin[12];
    bool buy;
    double px, sz;
    uint64_t oid;
    char kind[8];       /* "LIMIT", "TP", "SL" */
} dw_ord_t;

typedef struct {
    bool linked;        /* we hold a key and know the account */
    bool approved;      /* the venue confirms the key is approved */
    char account[64];   /* main account (address / pubkey / index) */
    char agent[64];     /* the device key, as the venue shows it */
    double equity, free, margin_used;
    bool error;
} dw_acct_t;

typedef struct {
    char id[12], name[16], venue_name[16], coin[12];
    int venue, listening;
} dw_room_t;

typedef struct {
    char who[20], lang[16], text[160];
    bool me;
} dw_msg_t;

typedef struct {
    char tag[12], text[120];
    int64_t ts;
} dw_news_t;

typedef struct {
    double t, o, h, l, c;
} dw_candle_t;

typedef struct {
    dw_mkt_t mkt[V_COUNT][DW_MAX_MKT];
    int nmkt[V_COUNT];
    dw_pos_t pos[DW_MAX_POS];
    int npos;
    dw_ord_t ord[DW_MAX_ORD];
    int nord;
    dw_acct_t acct[V_COUNT];
    dw_room_t rooms[DW_MAX_ROOMS];
    int nrooms;
    dw_msg_t msgs[DW_MAX_MSG];  /* messages of the room we are in, oldest first */
    int nmsgs;
    char room_id[12];
    dw_news_t news[DW_MAX_NEWS];
    int nnews;
    uint32_t version;
} dw_model_t;

extern dw_model_t g_model;

static inline void dw_model_touch(void) { g_model.version++; }

const char *dw_venue_name(int v);
const char *dw_venue_chain(int v);
dw_mkt_t *dw_mkt_find(int venue, const char *coin);
dw_pos_t *dw_pos_find(int venue, const char *coin);
/* Best price for a coin on any venue (falls back across venues). */
double dw_price(int venue, const char *coin);
double dw_change_pct(int venue, const char *coin);

/* Replace all positions / orders of one venue. */
void dw_model_set_positions(int venue, const dw_pos_t *p, int n);
void dw_model_set_orders(int venue, const dw_ord_t *o, int n);

/* ---- HTTP + JSON helper ---- */
typedef void (*dw_json_cb)(void *ctx, int status, cJSON *root);  /* root may be NULL; freed after the callback */
int dw_http_json(const char *method, const char *url, const char *headers, const char *body, dw_json_cb cb, void *ctx);

/* JSON accessors that accept numbers or numeric strings. */
double dw_jnum(const cJSON *o, const char *key);
const char *dw_jstr(const cJSON *o, const char *key);
