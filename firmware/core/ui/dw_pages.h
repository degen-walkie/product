#pragma once
#include "dw_model.h"
#include "dw_ui.h"

extern const dw_page_t PG_BOOT, PG_ROOMS, PG_ROOM, PG_MENU, PG_MARKETS, PG_CHART, PG_POSITIONS, PG_NEWS, PG_TICKET,
    PG_ASK, PG_VOICE, PG_ACCOUNTS, PG_LINK, PG_REVOKE, PG_DEVICE;

/* The order being built. Screens fill it, the ticket edits it, the sheet signs it. */
typedef struct {
    int venue;
    char coin[12];
    bool buy;
    int lev;
    int size_pct;       /* of free margin; ignored when size_base > 0 */
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

/* Confirmation sheet: the only place an order gets signed (trade key only). */
void dw_sheet_open(void);
bool dw_sheet_active(void);
bool dw_sheet_input(dw_input_t in);

/* Voice: whoever is on screen can take the transcript. */
typedef void (*dw_voice_sink)(const char *text);
void dw_voice_sink_set(dw_voice_sink s);
bool dw_voice_listening(void);
/* Parse a spoken order ("long sol five x five hundred usdc"). Returns false if it is not one. */
bool dw_parse_order(const char *text, dw_ticket_t *t);

void dw_ui_start_timers(void);
