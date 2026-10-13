#pragma once
#include "dw_model.h"
#include "dw_ui.h"

extern const dw_page_t PG_SETUP_LANG, PG_SETUP_WIFI, PG_SETUP_WIFI_PASS, PG_SETUP_PIN, PG_SETUP_LINK, PG_SETUP_DONE, PG_UNLOCK;
bool dw_pin_set(void);
extern const dw_page_t PG_BOOT, PG_ROOMS, PG_ROOM, PG_MENU, PG_MARKETS, PG_CHART, PG_POSITIONS, PG_NEWS, PG_TICKET,
    PG_ASK, PG_VOICE, PG_ACCOUNTS, PG_LINK, PG_REVOKE, PG_DEVICE;

/* The order being built. Screens fill it, the ticket edits it, the sheet signs it. */
typedef struct {
    int venue;
    char coin[12];
    bool buy;
    int lev;
    int size_pct;       /* of free margin; ignored when size_base > 0 */
    int tp_pct, sl_pct; /* take profit / stop loss distance from entry, % (0 = off) */
    double size_base;   /* explicit size in base units (close / half) */
    bool limit;
    double limit_px;
    bool reduce;
    const char *label;  /* "Close", "Half", or NULL */
} dw_ticket_t;
extern dw_ticket_t g_ticket;

extern int g_chart_venue;
extern char g_chart_coin[12];
extern int g_link_venue;

void dw_open_ticket(int venue, const char *coin, bool buy);
void dw_open_chart(int venue, const char *coin);
void dw_open_room(const char *room_id);
void dw_open_link(int venue);

/* Order notional for the ticket, and the size in base units. */
double dw_ticket_notional(void);
double dw_ticket_size(void);
double dw_ticket_liq(void);
/* TP / SL trigger prices from an entry price (0 when off). */
double dw_ticket_tp(double entry);
double dw_ticket_sl(double entry);

/* Confirmation sheet: the only place an order gets signed (trade key only). */
void dw_sheet_open(void);
bool dw_sheet_active(void);
bool dw_sheet_input(dw_input_t in);

/* Voice: whoever is on screen can take the transcript. */
typedef void (*dw_voice_sink)(const char *text);
void dw_voice_sink_set(dw_voice_sink s);
bool dw_voice_listening(void);   /* from talk pressed until the text arrives */
bool dw_voice_held(void);        /* talk is held right now */
/* Parse a spoken order ("long sol five x five hundred usdc"). Returns false if it is not one. */
bool dw_parse_order(const char *text, dw_ticket_t *t);

void dw_ui_start_timers(void);

/* Power, setup and overlays */
void dw_app_power_off(void);
bool dw_app_setup_done(void);
void dw_app_gesture(lv_dir_t dir);
bool dw_page_is_setup(const dw_page_t *p);
void dw_qp_open(void);
void dw_poweroff_open(void);
bool dw_overlays_active(void);
bool dw_overlays_input(dw_input_t in);
void dw_overlays_close(void);
/* A bottom sheet confirmed only by the trade key. */
void dw_confirm_open(const char *title, const char *body, const char *sign, void (*on_yes)(void));
