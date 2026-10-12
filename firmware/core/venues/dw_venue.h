/* One client per venue. Keys never leave the device: each client generates its own
 * trade-only key, the user approves it from their main wallet, and from then on the
 * client signs orders locally. None of the clients can sign a withdrawal. */
#pragma once
#include <stdbool.h>
#include <stddef.h>

#include "dw_config.h"
#include "dw_model.h"


typedef struct {
    char coin[12];
    bool buy;
    double sz;          /* base units */
    double limit_px;    /* 0 = market (IOC with slippage) */
    double slippage;    /* fraction, e.g. 0.01 */
    int leverage;       /* 0 = leave as is */
    bool reduce_only;
} dw_order_req_t;

typedef struct {
    bool ok;
    bool resting;       /* limit order placed, not filled */
    double avg_px, filled_sz;
    char msg[96];
} dw_order_res_t;

typedef void (*dw_order_cb)(void *ctx, const dw_order_res_t *res);
typedef void (*dw_candles_cb)(void *ctx, const dw_candle_t *c, int n);

typedef struct {
    const char *name;
    void (*start)(void);
    void (*poll)(void);
    int (*order)(const dw_order_req_t *req, dw_order_cb cb, void *ctx);
    int (*candles)(const char *coin, const char *interval, int count, dw_candles_cb cb, void *ctx);
    /* Take profit / stop loss on an open position (0 = none). NULL when the venue lacks it. */
    int (*tpsl)(const char *coin, bool long_pos, double sz, double tp, double sl, dw_order_cb cb, void *ctx);
    int (*cancel)(const char *coin, uint64_t oid, dw_order_cb cb, void *ctx);
    /* Linking: create a fresh device key and return what the QR shows. */
    int (*link_begin)(const char *session, char *qr, size_t cap);
    void (*link_done)(const char *account, const char *extra);
    void (*unlink)(void);
} dw_venue_t;

extern const dw_venue_t dw_hl, dw_pac, dw_lit;
const dw_venue_t *dw_venue(int v);

/* Shared helpers */
uint64_t dw_now_ms(void);
int dw_interval_ms(const char *interval);
/* Round to n significant figures, then to max_dec decimals; returns the wire string. */
void dw_px_str(double px, int sig, int max_dec, char *out, size_t cap);
void dw_num_str(double v, int dec, char *out, size_t cap);
