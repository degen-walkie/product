/* Chart, order ticket, confirmation sheet, voice shortcut, Ask AI. */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "dw_config.h"
#include "dw_pages.h"
#include "dw_venue.h"
#include "relay.h"

dw_ticket_t g_ticket = {.venue = V_HL, .coin = "SOL", .buy = true, .lev = 5, .size_pct = 25};
int g_chart_venue = V_HL;
char g_chart_coin[12] = "SOL";
/* The market a voice order defaults to (opened from its chart); empty = say the coin. */
static int s_vo_venue = -1;
static char s_vo_coin[12];

static uint32_t chg_color(double v) { return v >= 0 ? C_NEON : C_BAD; }
static void upper(char *s) { for (; *s; s++) *s = (char)toupper((unsigned char)*s); }

/* ======================= ticket maths ======================= */

double dw_ticket_size(void)
{
    double px = g_ticket.limit && g_ticket.limit_px > 0 ? g_ticket.limit_px : dw_price(g_ticket.venue, g_ticket.coin);
    if (g_ticket.size_base > 0) return g_ticket.size_base;
    if (px <= 0) return 0;
    double free = g_model.acct[g_ticket.venue].free;
    double notional = free * g_ticket.size_pct / 100.0 * g_ticket.lev;
    dw_mkt_t *m = dw_mkt_find(g_ticket.venue, g_ticket.coin);
    int dec = m ? m->sz_dec : 3;
    double sz = notional / px;
    double f = pow(10, dec);
    return floor(sz * f) / f;
}

double dw_ticket_notional(void)
{
    double px = g_ticket.limit && g_ticket.limit_px > 0 ? g_ticket.limit_px : dw_price(g_ticket.venue, g_ticket.coin);
    return dw_ticket_size() * px;
}

double dw_ticket_tp(double entry)
{
    if (!g_ticket.tp_pct || g_ticket.reduce || entry <= 0) return 0;
    return entry * (1 + (g_ticket.buy ? 1 : -1) * g_ticket.tp_pct / 100.0);
}

double dw_ticket_sl(double entry)
{
    if (!g_ticket.sl_pct || g_ticket.reduce || entry <= 0) return 0;
    return entry * (1 - (g_ticket.buy ? 1 : -1) * g_ticket.sl_pct / 100.0);
}

double dw_ticket_liq(void)
{
    if (g_ticket.reduce) return 0;
    double px = g_ticket.limit && g_ticket.limit_px > 0 ? g_ticket.limit_px : dw_price(g_ticket.venue, g_ticket.coin);
    if (px <= 0 || g_ticket.lev <= 0) return 0;
    double mm = 0.5 / g_ticket.lev;  /* maintenance margin ~ half the initial margin */
    return g_ticket.buy ? px * (1 - 1.0 / g_ticket.lev + mm) : px * (1 + 1.0 / g_ticket.lev - mm);
}

void dw_open_ticket(int venue, const char *coin, bool buy)
{
    g_ticket.venue = venue;
    snprintf(g_ticket.coin, sizeof g_ticket.coin, "%s", coin);
    g_ticket.buy = buy;
    g_ticket.size_base = 0;
    g_ticket.reduce = false;
    g_ticket.label = NULL;
    g_ticket.limit = false;
    g_ticket.limit_px = 0;
    g_ticket.tp_pct = g_ticket.sl_pct = 0;
    if (g_ticket.lev <= 0) g_ticket.lev = 5;
    if (!g_ticket.size_pct) g_ticket.size_pct = 25;
    dw_nav_push(&PG_TICKET);
}

/* ======================= Chart ======================= */

static lv_obj_t *s_ch_px, *s_ch_sub, *s_ch_chart, *s_ch_card, *s_ch_side, *s_ch_pnl, *s_ch_meta, *s_ch_tf;
static dw_focus_t s_ch_f;
static int s_tf = 0;
static uint32_t s_ch_req;
static const char *TF[] = {"15m", "1h", "4h", "1d"};
static const char *TF_LABEL[] = {"15M", "1H", "4H", "1D"};

void dw_open_chart(int venue, const char *coin)
{
    g_chart_venue = venue;
    snprintf(g_chart_coin, sizeof g_chart_coin, "%s", coin);
    dw_nav_push(&PG_CHART);
}

static int s_ch_tries;
static void ch_fetch(void);

static void ch_retry(lv_timer_t *t)
{
    if ((uint32_t)(uintptr_t)lv_timer_get_user_data(t) == s_ch_req && s_ch_chart) ch_fetch();
}

static void on_ch_candles(void *ctx, const dw_candle_t *c, int n)
{
    if ((uint32_t)(uintptr_t)ctx != s_ch_req || !s_ch_chart) return;
    if (n == 0 && s_ch_tries < 3) {
        lv_timer_t *t = lv_timer_create(ch_retry, 1500, (void *)(uintptr_t)s_ch_req);
        lv_timer_set_repeat_count(t, 1);
        return;
    }
    dw_pos_t *p = dw_pos_find(g_chart_venue, g_chart_coin);
    ui_chart_set(s_ch_chart, c, n, dw_price(g_chart_venue, g_chart_coin), p ? p->entry : 0);
}

static void ch_fetch(void)
{
    s_ch_tries++;
    const dw_venue_t *v = dw_venue(g_chart_venue);
    v->candles(g_chart_coin, TF[s_tf], 48, on_ch_candles, (void *)(uintptr_t)s_ch_req);
}

static void ch_load(void)
{
    s_ch_req++;
    s_ch_tries = 0;
    ch_fetch();
    ui_chips_set(s_ch_tf, s_tf);
}

static void ch_action(int i)
{
    dw_pos_t *p = dw_pos_find(g_chart_venue, g_chart_coin);
    if (i == 4) { dw_open_voice(g_chart_venue, g_chart_coin); return; }
    if (i == 0) { s_tf = (s_tf + 1) % 4; ch_load(); return; }
    if (!p) { dw_open_ticket(g_chart_venue, g_chart_coin, i == 1); return; }
    if (i == 1) { dw_open_ticket(g_chart_venue, g_chart_coin, p->szi > 0); return; }  /* ADD */
    /* HALF / CLOSE go straight to the confirmation sheet */
    g_ticket.venue = g_chart_venue;
    snprintf(g_ticket.coin, sizeof g_ticket.coin, "%s", g_chart_coin);
    g_ticket.buy = p->szi < 0;
    g_ticket.reduce = true;
    g_ticket.limit = false;
    g_ticket.lev = 0;
    dw_mkt_t *m = dw_mkt_find(g_chart_venue, g_chart_coin);
    double f = pow(10, m ? m->sz_dec : 3);
    g_ticket.size_base = i == 2 ? floor(fabs(p->szi) / 2 * f) / f : fabs(p->szi);
    g_ticket.label = i == 2 ? "Close half" : "Close";
    dw_sheet_open();
}

static void ch_tf_click(lv_event_t *e)
{
    lv_obj_t *t = lv_event_get_target(e);
    if (lv_obj_get_parent(t) != s_ch_tf) return;
    s_tf = (int)lv_obj_get_index(t);
    ch_load();
}

static lv_obj_t *s_ch_btn[3];

static void ch_update(void)
{
    double px = dw_price(g_chart_venue, g_chart_coin), ch = dw_change_pct(g_chart_venue, g_chart_coin);
    lv_label_set_text(s_ch_px, fmt_px(px));
    char sub[64];
    snprintf(sub, sizeof sub, "%s \xC2\xB7 %s \xC2\xB7 %s", dw_venue_name(g_chart_venue), TF_LABEL[s_tf], fmt_pct(ch));
    upper(sub);
    lv_label_set_text(s_ch_sub, sub);
    dw_pos_t *p = dw_pos_find(g_chart_venue, g_chart_coin);
    ui_chart_set(s_ch_chart, NULL, 0, px, p ? p->entry : 0);
    if (p) {
        lv_obj_remove_flag(s_ch_card, LV_OBJ_FLAG_HIDDEN);
        char t[48];
        snprintf(t, sizeof t, "%s %.0f\xC3\x97 \xC2\xB7 %s USDC", p->szi > 0 ? "LONG" : "SHORT", p->lev, fmt_num(fabs(p->value), 0));
        lv_label_set_text(s_ch_side, t);
        lv_label_set_text(s_ch_pnl, fmt_signed(p->upnl, 2));
        lv_obj_set_style_text_color(s_ch_pnl, lv_color_hex(chg_color(p->upnl)), 0);
        double dist = p->liq > 0 && px > 0 ? fabs(px - p->liq) / px * 100 : 0;
        dw_mkt_t *m = dw_mkt_find(g_chart_venue, g_chart_coin);
        snprintf(t, sizeof t, "LIQ %s \xC2\xB7 %.0f%% AWAY\nFUNDING %+.4f%%/H", p->liq > 0 ? fmt_px(p->liq) : "\xE2\x80\x94", dist,
                 m ? m->funding_h * 100 : 0);
        lv_label_set_text(s_ch_meta, t);
        lv_label_set_text(lv_obj_get_child(s_ch_btn[0], 0), "ADD");
        lv_label_set_text(lv_obj_get_child(s_ch_btn[1], 0), "HALF");
        lv_label_set_text(lv_obj_get_child(s_ch_btn[2], 0), "CLOSE");
        lv_obj_set_style_text_color(lv_obj_get_child(s_ch_btn[2], 0), lv_color_hex(C_BAD), 0);
        lv_obj_remove_flag(s_ch_btn[2], LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_ch_card, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(lv_obj_get_child(s_ch_btn[0], 0), "LONG");
        lv_label_set_text(lv_obj_get_child(s_ch_btn[1], 0), "SHORT");
        lv_obj_set_style_text_color(lv_obj_get_child(s_ch_btn[1], 0), lv_color_hex(C_BAD), 0);
        lv_obj_add_flag(s_ch_btn[2], LV_OBJ_FLAG_HIDDEN);
    }
}

static void ch_build(lv_obj_t *body)
{
    char t[24];
    snprintf(t, sizeof t, "%s-PERP", g_chart_coin);
    dw_bar(t, "\xC2\xB7 CHART", C_NEON);
    dw_hint("move", "back", "open", false);
    lv_obj_set_style_pad_row(body, 7, 0);
    lv_obj_t *hdr = ui_rowbox(body, 6);
    ui_label(hdr, &dw_disp_24, C_INK, t);
    s_ch_px = ui_label(hdr, &dw_monob_18, C_NEON, "");
    s_ch_sub = ui_sub(body, "", 0);
    s_ch_chart = ui_chart(body, 150);
    dw_focus_reset(&s_ch_f, ch_action);
    s_ch_tf = ui_chips(body, TF_LABEL, 4, s_tf);
    lv_obj_add_event_cb(s_ch_tf, ch_tf_click, LV_EVENT_CLICKED, NULL);
    dw_focus_add(&s_ch_f, s_ch_tf);

    s_ch_card = ui_card(body, K_NEON);
    lv_obj_set_style_bg_color(s_ch_card, lv_color_hex(0x0c1612), 0);
    lv_obj_t *r = ui_rowbox(s_ch_card, 6);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t *l = ui_col(r, 2);
    lv_obj_set_width(l, LV_SIZE_CONTENT);
    s_ch_side = ui_label(l, &dw_monob_11, C_NEON, "");
    s_ch_pnl = ui_label(l, &dw_monob_18, C_NEON, "");
    s_ch_meta = ui_label(r, &dw_mono_9, C_MUTE, "");
    lv_obj_set_style_text_align(s_ch_meta, LV_TEXT_ALIGN_RIGHT, 0);

    lv_obj_t *btns = ui_rowbox(body, 6);
    for (int i = 0; i < 3; i++) {
        s_ch_btn[i] = ui_btn(btns, "", i == 0 ? K_TRADE : K_PLAIN);
        lv_obj_set_width(s_ch_btn[i], LV_PCT(32));
        lv_obj_set_flex_grow(s_ch_btn[i], 1);
        dw_focus_add(&s_ch_f, s_ch_btn[i]);
    }
    lv_obj_t *say = ui_btn(body, "SAY AN ORDER", K_PLAIN);   /* no talk key on the device */
    lv_obj_set_width(say, LV_PCT(100));
    dw_focus_add(&s_ch_f, say);
    dw_focus_set(&s_ch_f, 1);
    ch_load();
}

static bool ch_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP:
    case DW_IN_WHEEL_DOWN: {
        int d = in == DW_IN_WHEEL_DOWN ? 1 : -1, i = s_ch_f.i + d;
        if (i == 3 && lv_obj_has_flag(s_ch_btn[2], LV_OBJ_FLAG_HIDDEN)) i = d > 0 ? 2 : 1;
        dw_focus_set(&s_ch_f, i);
        dw_hint("move", "back", s_ch_f.i == 0 ? "timeframe" : "open", false);
        return true;
    }
    case DW_IN_TRADE: ch_action(s_ch_f.i); return true;
    case DW_IN_PTT_DOWN:   /* the app opens the voice page next: default to this market */
        s_vo_venue = g_chart_venue;
        snprintf(s_vo_coin, sizeof s_vo_coin, "%s", g_chart_coin);
        return false;
    default: return false;
    }
}

static void ch_leave(void) { s_ch_chart = NULL; }

const dw_page_t PG_CHART = {.name = "chart", .build = ch_build, .update = ch_update, .input = ch_input, .leave = ch_leave};

/* ======================= Ticket ======================= */

enum { F_SIDE, F_TYPE, F_LEV, F_SIZE, F_PRICE, F_TP, F_SL, F_REVIEW };
static const int LEV[] = {2, 3, 5, 10, 20};
static const int PCT[] = {10, 25, 50, 100};
static const int TPS[] = {0, 2, 5, 10};
static const int SLS[] = {0, 2, 3, 5};
static dw_focus_t s_tk_f;
static int s_tk_field[8];
static bool s_tk_edit;
static lv_obj_t *s_tk_tp, *s_tk_sl, *s_tk_tpsl;
static lv_obj_t *s_tk_title, *s_tk_side, *s_tk_type, *s_tk_lev, *s_tk_pct, *s_tk_pxrow, *s_tk_pxval, *s_tk_size, *s_tk_px,
    *s_tk_liq, *s_tk_freelbl, *s_tk_warn;

static int idx_of(const int *a, int n, int v)
{
    for (int i = 0; i < n; i++) if (a[i] == v) return i;
    return 0;
}

static void tk_hint(void)
{
    int f = s_tk_field[s_tk_f.i];
    if (s_tk_edit) dw_hint("adjust", "done", "done", false);
    else if (f == F_REVIEW) dw_hint("move", "back", "review", false);
    else dw_hint("move", "back", "edit", false);
}

static void tk_refresh(void)
{
    char t[32];
    snprintf(t, sizeof t, "%s %s", g_ticket.buy ? "Long" : "Short", g_ticket.coin);
    upper(t);
    lv_label_set_text(s_tk_title, t);
    lv_obj_set_style_text_color(s_tk_title, lv_color_hex(g_ticket.buy ? C_NEON : C_BAD), 0);
    ui_chips_set(s_tk_side, g_ticket.buy ? 0 : 1);
    ui_chips_set(s_tk_type, g_ticket.limit ? 1 : 0);
    ui_chips_set(s_tk_lev, idx_of(LEV, 5, g_ticket.lev));
    ui_chips_set(s_tk_pct, idx_of(PCT, 4, g_ticket.size_pct));
    ui_chips_set(s_tk_tp, idx_of(TPS, 4, g_ticket.tp_pct));
    ui_chips_set(s_tk_sl, idx_of(SLS, 4, g_ticket.sl_pct));
    if (g_ticket.limit) lv_obj_remove_flag(s_tk_pxrow, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(s_tk_pxrow, LV_OBJ_FLAG_HIDDEN);
    double mark = dw_price(g_ticket.venue, g_ticket.coin);
    if (g_ticket.limit && g_ticket.limit_px <= 0) g_ticket.limit_px = mark;
    lv_label_set_text(s_tk_pxval, fmt_px(g_ticket.limit_px));
    char s[48];
    snprintf(s, sizeof s, "%s USDC", fmt_num(dw_ticket_notional(), 2));
    lv_label_set_text(s_tk_size, s);
    lv_label_set_text(s_tk_px, g_ticket.limit ? fmt_px(g_ticket.limit_px) : fmt_px(mark));
    double liq = dw_ticket_liq();
    lv_label_set_text(s_tk_liq, liq > 0 ? fmt_px(liq) : "\xE2\x80\x94");
    double entry = g_ticket.limit ? g_ticket.limit_px : mark, tp = dw_ticket_tp(entry), sl = dw_ticket_sl(entry);
    char tps[48];
    snprintf(tps, sizeof tps, "%s / %s", tp > 0 ? fmt_px(tp) : "off", sl > 0 ? fmt_px(sl) : "off");
    lv_label_set_text(s_tk_tpsl, tps);
    snprintf(s, sizeof s, "SIZE \xC2\xB7 OF %s USDC FREE MARGIN", fmt_num(g_model.acct[g_ticket.venue].free, 0));
    lv_label_set_text(s_tk_freelbl, s);
    bool linked = g_model.acct[g_ticket.venue].linked;
    if (linked) lv_obj_add_flag(s_tk_warn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(s_tk_warn, LV_OBJ_FLAG_HIDDEN);
}

static void tk_adjust(int d)
{
    switch (s_tk_field[s_tk_f.i]) {
    case F_SIDE: g_ticket.buy = !g_ticket.buy; break;
    case F_TYPE: g_ticket.limit = !g_ticket.limit; g_ticket.limit_px = 0; break;
    case F_LEV: {
        int i = idx_of(LEV, 5, g_ticket.lev) + d;
        g_ticket.lev = LEV[i < 0 ? 0 : i > 4 ? 4 : i];
        break;
    }
    case F_SIZE: {
        int i = idx_of(PCT, 4, g_ticket.size_pct) + d;
        g_ticket.size_pct = PCT[i < 0 ? 0 : i > 3 ? 3 : i];
        break;
    }
    case F_TP: {
        int i = idx_of(TPS, 4, g_ticket.tp_pct) + d;
        g_ticket.tp_pct = TPS[i < 0 ? 0 : i > 3 ? 3 : i];
        break;
    }
    case F_SL: {
        int i = idx_of(SLS, 4, g_ticket.sl_pct) + d;
        g_ticket.sl_pct = SLS[i < 0 ? 0 : i > 3 ? 3 : i];
        break;
    }
    case F_PRICE: {
        double mark = dw_price(g_ticket.venue, g_ticket.coin);
        g_ticket.limit_px += d * mark * 0.001;
        break;
    }
    default: break;
    }
    tk_refresh();
}

static void tk_open(int i)
{
    int f = s_tk_field[i];
    if (f == F_REVIEW) {
        if (!g_model.acct[g_ticket.venue].linked) { dw_open_link(g_ticket.venue); return; }
        if (dw_ticket_size() <= 0) { dw_toast("Size is zero. Add funds on the venue or pick a bigger size.", C_WARN); return; }
        dw_sheet_open();
        return;
    }
    if (f == F_SIDE || f == F_TYPE) { tk_adjust(1); return; }
    s_tk_edit = !s_tk_edit;
    tk_hint();
}

static void chip_pick(lv_event_t *e)
{
    lv_obj_t *chip = lv_event_get_target(e);
    lv_obj_t *box = lv_obj_get_parent(chip);
    int idx = (int)lv_obj_get_index(chip);
    if (box == s_tk_side) g_ticket.buy = idx == 0;
    else if (box == s_tk_type) { g_ticket.limit = idx == 1; g_ticket.limit_px = 0; }
    else if (box == s_tk_lev && idx < 5) g_ticket.lev = LEV[idx];
    else if (box == s_tk_pct && idx < 4) g_ticket.size_pct = PCT[idx];
    else if (box == s_tk_tp && idx < 4) g_ticket.tp_pct = TPS[idx];
    else if (box == s_tk_sl && idx < 4) g_ticket.sl_pct = SLS[idx];
    else return;
    tk_refresh();
}

static void tk_add(lv_obj_t *obj, int field)
{
    int i = dw_focus_add(&s_tk_f, obj);
    if (i >= 0) s_tk_field[i] = field;
}

static void tk_build(lv_obj_t *body)
{
    char t[32];
    snprintf(t, sizeof t, "%s-PERP", g_ticket.coin);
    dw_bar(t, "\xC2\xB7 ORDER", C_NEON);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_row(body, 6, 0);
    lv_obj_set_style_pad_top(body, 10, 0);
    s_tk_edit = false;
    if (g_ticket.lev <= 0) g_ticket.lev = 5;
    lv_obj_t *hdr = ui_rowbox(body, 6);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t *hl = ui_col(hdr, 2);
    lv_obj_set_width(hl, LV_SIZE_CONTENT);
    s_tk_title = ui_title(hl, "", true);
    snprintf(t, sizeof t, "PERPETUAL \xC2\xB7 %s", dw_venue_name(g_ticket.venue));
    upper(t);
    ui_label(hl, &dw_mono_11, C_MUTE, t);
    ui_badge(hdr, "trade-only key", K_NEON);

    dw_focus_reset(&s_tk_f, tk_open);
    lv_obj_t *segs = ui_rowbox(body, 6);
    s_tk_side = ui_seg(segs, "Long", "Short", 0);
    lv_obj_set_width(s_tk_side, LV_PCT(49));
    s_tk_type = ui_seg(segs, "Market", "Limit", 0);
    lv_obj_set_width(s_tk_type, LV_PCT(49));
    tk_add(s_tk_side, F_SIDE);
    tk_add(s_tk_type, F_TYPE);
    ui_eyebrow(body, "LEVERAGE");
    static const char *LEVS[] = {"2\xC3\x97", "3\xC3\x97", "5\xC3\x97", "10\xC3\x97", "20\xC3\x97"};
    s_tk_lev = ui_chips(body, LEVS, 5, 2);
    tk_add(s_tk_lev, F_LEV);
    s_tk_freelbl = ui_eyebrow(body, "SIZE \xC2\xB7 OF FREE MARGIN");
    static const char *PCTS[] = {"10%", "25%", "50%", "100%"};
    s_tk_pct = ui_chips(body, PCTS, 4, 1);
    tk_add(s_tk_pct, F_SIZE);
    s_tk_pxrow = ui_rowbox(body, 6);
    lv_obj_set_style_pad_all(s_tk_pxrow, 6, 0);
    lv_obj_set_style_radius(s_tk_pxrow, 6, 0);
    ui_label(s_tk_pxrow, &dw_sans_12, C_MUTE, "Limit price");
    s_tk_pxval = ui_label(s_tk_pxrow, &dw_mono_15, C_INK, "");
    tk_add(s_tk_pxrow, F_PRICE);
    lv_obj_add_event_cb(s_tk_side, chip_pick, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_tk_type, chip_pick, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_tk_lev, chip_pick, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_tk_pct, chip_pick, LV_EVENT_CLICKED, NULL);
    lv_obj_t *tpsl = ui_rowbox(body, 6);
    lv_obj_set_flex_align(tpsl, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t *tpc = ui_col(tpsl, 3), *slc = ui_col(tpsl, 3);
    lv_obj_set_width(tpc, LV_PCT(49));
    lv_obj_set_width(slc, LV_PCT(49));
    ui_eyebrow(tpc, "TAKE PROFIT +");
    ui_eyebrow(slc, "STOP LOSS -");
    static const char *TPL[] = {"off", "2%", "5%", "10%"}, *SLL[] = {"off", "2%", "3%", "5%"};
    s_tk_tp = ui_chips(tpc, TPL, 4, 0);
    s_tk_sl = ui_chips(slc, SLL, 4, 0);
    lv_obj_set_style_pad_column(s_tk_tp, 3, 0);
    lv_obj_set_style_pad_column(s_tk_sl, 3, 0);
    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_pad_hor(lv_obj_get_child(s_tk_tp, i), 5, 0);
        lv_obj_set_style_pad_hor(lv_obj_get_child(s_tk_sl, i), 5, 0);
    }
    tk_add(s_tk_tp, F_TP);
    tk_add(s_tk_sl, F_SL);
    lv_obj_add_event_cb(s_tk_tp, chip_pick, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_tk_sl, chip_pick, LV_EVENT_CLICKED, NULL);

    lv_obj_t *kv = ui_kv(body);
    s_tk_size = ui_kv_add(kv, "Size", "", 0);
    s_tk_px = ui_kv_add(kv, "Price", "", 0);
    s_tk_liq = ui_kv_add(kv, "Liq. price (est.)", "", C_BAD);
    s_tk_tpsl = ui_kv_add(kv, "TP / SL", "", C_SOFT);
    s_tk_warn = ui_card(body, K_WARN);
    lv_obj_t *w = ui_label(s_tk_warn, &dw_sans_12, C_WARN, "This venue isn't linked yet. The trade key opens linking.");
    lv_obj_set_width(w, LV_PCT(100));
    lv_label_set_long_mode(w, LV_LABEL_LONG_WRAP);
    lv_obj_t *rv = ui_btn(body, "REVIEW \xE2\x86\x92 SIGN", K_TRADE);
    tk_add(rv, F_REVIEW);
    tk_refresh();
    dw_focus_set(&s_tk_f, s_tk_f.n - 1);
    lv_obj_scroll_to_y(body, 0, LV_ANIM_OFF);
    tk_hint();
}

static bool tk_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP:
    case DW_IN_WHEEL_DOWN: {
        int d = in == DW_IN_WHEEL_DOWN ? 1 : -1;
        if (s_tk_edit) { tk_adjust(d); return true; }
        int i = s_tk_f.i + d;
        if (i >= 0 && i < s_tk_f.n && s_tk_field[i] == F_PRICE && !g_ticket.limit) i += d;
        dw_focus_set(&s_tk_f, i);
        tk_hint();
        return true;
    }
    case DW_IN_TRADE: tk_open(s_tk_f.i); return true;
    case DW_IN_WHEEL_PRESS:
        if (s_tk_edit) { s_tk_edit = false; tk_hint(); return true; }
        return false;
    default: return false;
    }
}

const dw_page_t PG_TICKET = {.name = "ticket", .build = tk_build, .update = tk_refresh, .input = tk_input};

/* ======================= Confirmation sheet ======================= */

static lv_obj_t *s_sheet, *s_sh_sign, *s_sh_status;
static enum { SH_IDLE, SH_SENDING, SH_DONE } s_sh_state;
static uint32_t s_sh_token;

bool dw_sheet_active(void) { return s_sheet != NULL; }

static void sheet_close(void)
{
    if (s_sheet) lv_obj_delete(s_sheet);
    s_sheet = NULL;
    s_sh_state = SH_IDLE;
    s_sh_token++;
}

static void sheet_finish(lv_timer_t *t)
{
    bool ok = (bool)(uintptr_t)lv_timer_get_user_data(t);
    sheet_close();
    if (ok && dw_nav_top() == &PG_TICKET) dw_nav_pop();
}

static char s_fill_text[140];

static void sheet_done(bool ok, const char *text, const char *status)
{
    s_sh_state = SH_DONE;
    lv_label_set_text(lv_obj_get_child(s_sh_sign, 1), text);
    lv_obj_set_style_bg_color(s_sh_sign, lv_color_hex(ok ? C_NEON : C_BAD), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(s_sh_sign, 1), lv_color_hex(ok ? C_NEON : C_BAD), 0);
    lv_obj_set_style_bg_color(lv_obj_get_child(s_sh_sign, 0), lv_color_hex(ok ? C_NEON : C_BAD), 0);
    lv_label_set_text(s_sh_status, status);
    lv_timer_t *tm = lv_timer_create(sheet_finish, ok ? 2200 : 3400, (void *)(uintptr_t)ok);
    lv_timer_set_repeat_count(tm, 1);
}

static void on_tpsl(void *ctx, const dw_order_res_t *r)
{
    if ((uint32_t)(uintptr_t)ctx != s_sh_token || !s_sheet) return;
    char t[200];
    snprintf(t, sizeof t, "%s \xC2\xB7 TP/SL %s", s_fill_text, r->ok ? "SET" : "FAILED");
    sheet_done(true, t, r->ok ? "Position open. TP/SL orders wait on the venue." : r->msg);
}

static void on_result(void *ctx, const dw_order_res_t *r)
{
    if ((uint32_t)(uintptr_t)ctx != s_sh_token || !s_sheet) return;
    char t[140];
    if (r->ok && !r->resting && !g_ticket.reduce) {
        double avg = r->avg_px > 0 ? r->avg_px : dw_price(g_ticket.venue, g_ticket.coin);
        double tp = dw_ticket_tp(avg), sl = dw_ticket_sl(avg);
        const dw_venue_t *v = dw_venue(g_ticket.venue);
        if ((tp > 0 || sl > 0) && v->tpsl) {
            dw_light_ok();
            hal_vibrate(60);
            snprintf(s_fill_text, sizeof s_fill_text, "FILLED %s %s @ %s", fmt_num(r->filled_sz, 4), g_ticket.coin, fmt_px(avg));
            upper(s_fill_text);
            snprintf(t, sizeof t, "%s \xC2\xB7 SETTING TP/SL\xE2\x80\xA6", s_fill_text);
            lv_label_set_text(lv_obj_get_child(s_sh_sign, 1), t);
            double sz = r->filled_sz > 0 ? r->filled_sz : dw_ticket_size();
            if (v->tpsl(g_ticket.coin, g_ticket.buy, sz, tp, sl, on_tpsl, (void *)(uintptr_t)s_sh_token) == 0) return;
            dw_order_res_t f = {.ok = false};
            snprintf(f.msg, sizeof f.msg, "Could not sign TP/SL");
            on_tpsl((void *)(uintptr_t)s_sh_token, &f);
            return;
        }
    }
    if (r->ok) {
        dw_light_ok();
        hal_vibrate(60);
        if (r->resting) snprintf(t, sizeof t, "ORDER PLACED \xC2\xB7 RESTING ON %s", dw_venue_name(g_ticket.venue));
        else snprintf(t, sizeof t, "FILLED %s %s @ %s", fmt_num(r->filled_sz, 4), g_ticket.coin, fmt_px(r->avg_px));
        upper(t);
    } else {
        dw_light_fail();
        snprintf(t, sizeof t, "REJECTED: %s", r->msg);
    }
    sheet_done(r->ok, t, r->ok ? "Confirmed by the venue." : "Nothing was executed.");
}

static void sheet_sign(void)
{
    const dw_venue_t *v = dw_venue(g_ticket.venue);
    dw_order_req_t req = {0};
    snprintf(req.coin, sizeof req.coin, "%s", g_ticket.coin);
    req.buy = g_ticket.buy;
    req.sz = dw_ticket_size();
    req.limit_px = g_ticket.limit ? g_ticket.limit_px : 0;
    req.slippage = 0.01;
    req.leverage = g_ticket.reduce ? 0 : g_ticket.lev;
    req.reduce_only = g_ticket.reduce;
    s_sh_state = SH_SENDING;
    dw_light_sending();
    lv_label_set_text(lv_obj_get_child(s_sh_sign, 1), "SIGNED ON DEVICE \xC2\xB7 SENDING\xE2\x80\xA6");
    lv_obj_set_style_text_color(lv_obj_get_child(s_sh_sign, 1), lv_color_hex(C_BLUE), 0);
    lv_obj_set_style_bg_color(s_sh_sign, lv_color_hex(C_BLUE), 0);
    lv_obj_set_style_bg_color(lv_obj_get_child(s_sh_sign, 0), lv_color_hex(C_BLUE), 0);
    if (!v || v->order(&req, on_result, (void *)(uintptr_t)s_sh_token) < 0) {
        dw_order_res_t r = {.ok = false};
        snprintf(r.msg, sizeof r.msg, "%s is not linked", dw_venue_name(g_ticket.venue));
        on_result((void *)(uintptr_t)s_sh_token, &r);
    }
}

/* The sheet covers the hint bar, so touch needs its own way out: a tap outside the
 * panel cancels (the panel itself swallows taps). Signing stays on the trade key. */
static void sheet_cancel_async(void *arg)
{
    (void)arg;
    if (s_sheet && s_sh_state != SH_SENDING) dw_sheet_input(DW_IN_WHEEL_PRESS);
}

static void sheet_scrim_click(lv_event_t *e)
{
    if (lv_event_get_target(e) == s_sheet) lv_async_call(sheet_cancel_async, NULL);
}

void dw_sheet_open(void)
{
    sheet_close();
    s_sheet = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_sheet);
    lv_obj_set_size(s_sheet, SCR_W, SCR_H);
    lv_obj_set_style_bg_color(s_sheet, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_sheet, 150, 0);
    lv_obj_add_flag(s_sheet, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_sheet, sheet_scrim_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *p = lv_obj_create(s_sheet);
    lv_obj_remove_style_all(p);
    lv_obj_set_size(p, SCR_W, LV_SIZE_CONTENT);
    lv_obj_align(p, LV_ALIGN_BOTTOM_MID, 0, -HINT_H);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x12161b), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(p, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(p, 2, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(C_NEON), 0);
    lv_obj_set_style_radius(p, 10, 0);
    lv_obj_set_style_pad_hor(p, 14, 0);
    lv_obj_set_style_pad_ver(p, 14, 0);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(p, 8, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);   /* a tap on the panel is not a cancel */

    ui_eyebrow(p, "CONFIRM \xC2\xB7 SIGNED ON THIS DEVICE");
    double sz = dw_ticket_size();
    char t[64];
    if (g_ticket.label) snprintf(t, sizeof t, "%s %s %s", g_ticket.label, fmt_num(sz, 4), g_ticket.coin);
    else snprintf(t, sizeof t, "%s %s %s", g_ticket.buy ? "Long" : "Short", fmt_num(sz, 4), g_ticket.coin);
    upper(t);
    lv_obj_t *title = ui_title(p, t, true);
    lv_obj_set_style_text_color(title, lv_color_hex(g_ticket.reduce ? C_INK : g_ticket.buy ? C_NEON : C_BAD), 0);
    lv_obj_t *kv = ui_kv(p);
    char v[64];
    snprintf(v, sizeof v, "%s-PERP \xC2\xB7 %s", g_ticket.coin, dw_venue_name(g_ticket.venue));
    ui_kv_add(kv, "Market", v, 0);
    if (!g_ticket.reduce) {
        snprintf(v, sizeof v, "%d\xC3\x97 cross", g_ticket.lev);
        ui_kv_add(kv, "Leverage", v, 0);
    } else {
        ui_kv_add(kv, "Reduce only", "yes", C_NEON);
    }
    snprintf(v, sizeof v, "%s USDC", fmt_num(dw_ticket_notional(), 2));
    ui_kv_add(kv, "Notional", v, 0);
    if (g_ticket.limit) snprintf(v, sizeof v, "limit %s", fmt_px(g_ticket.limit_px));
    else snprintf(v, sizeof v, "market \xE2\x89\x88 %s \xC2\xB7 1%% max", fmt_px(dw_price(g_ticket.venue, g_ticket.coin)));
    ui_kv_add(kv, "Price", v, 0);
    double liq = dw_ticket_liq();
    if (liq > 0) ui_kv_add(kv, "Liq. price (est.)", fmt_px(liq), C_BAD);
    double ep = g_ticket.limit ? g_ticket.limit_px : dw_price(g_ticket.venue, g_ticket.coin);
    if (dw_ticket_tp(ep) > 0 || dw_ticket_sl(ep) > 0) {
        snprintf(v, sizeof v, "%s / %s", dw_ticket_tp(ep) > 0 ? fmt_px(dw_ticket_tp(ep)) : "off", dw_ticket_sl(ep) > 0 ? fmt_px(dw_ticket_sl(ep)) : "off");
        ui_kv_add(kv, "TP / SL", v, C_SOFT);
    }
    const char *agent = g_model.acct[g_ticket.venue].agent;
    snprintf(v, sizeof v, "%s \xC2\xB7 trade-only", *agent ? fmt_short(agent, 6, 4) : "none");
    ui_kv_add(kv, "Signing key", v, C_MUTE);
    s_sh_sign = ui_sign(p, "PRESS THE TRADE KEY TO SIGN");
    s_sh_status = ui_label(p, &dw_mono_9, C_DIM, "TOUCH CAN'T SIGN \xC2\xB7 TAP OUTSIDE OR PRESS THE WHEEL TO CANCEL");
    lv_obj_set_style_text_align(s_sh_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_sh_status, LV_PCT(100));
    s_sh_state = SH_IDLE;
    dw_hint(NULL, "cancel", "SIGN", true);
}

bool dw_sheet_input(dw_input_t in)
{
    if (!s_sheet) return false;
    if (s_sh_state == SH_IDLE) {
        if (in == DW_IN_TRADE) { sheet_sign(); dw_hint(NULL, NULL, NULL, false); }
        else if (in == DW_IN_WHEEL_PRESS) {
            sheet_close();
            const dw_page_t *p = dw_nav_top();
            if (p == &PG_TICKET) tk_hint();
            else dw_nav_replace(p);
        }
    } else if (s_sh_state == SH_DONE && (in == DW_IN_WHEEL_PRESS || in == DW_IN_TRADE)) {
        sheet_close();
        dw_nav_replace(dw_nav_top());
    }
    return true;
}

/* ======================= Voice shortcut ======================= */

static lv_obj_t *s_vo_said, *s_vo_kv, *s_vo_sign;
static bool s_vo_ok;

static int num_word(const char *w)
{
    static const char *N[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
                              "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen",
                              "nineteen", "twenty"};
    for (int i = 0; i <= 20; i++) if (!strcmp(w, N[i])) return i;
    if (!strcmp(w, "thirty")) return 30;
    if (!strcmp(w, "forty")) return 40;
    if (!strcmp(w, "fifty")) return 50;
    if (!strcmp(w, "a")) return 1;
    return -1;
}

bool dw_parse_order(const char *text, dw_ticket_t *t)
{
    char buf[200];
    snprintf(buf, sizeof buf, "%s", text);
    for (char *p = buf; *p; p++) *p = (char)(isalnum((unsigned char)*p) || *p == '.' ? tolower((unsigned char)*p) : ' ');
    int side = -1, lev = 0, venue = -1;
    bool close = false;
    double size = 0, acc = 0, cur = 0;
    bool in_num = false;
    char coin[12] = "";
    char *save = NULL;
    for (char *w = strtok_r(buf, " ", &save); w; w = strtok_r(NULL, " ", &save)) {
        int nw = num_word(w);
        bool digit = isdigit((unsigned char)w[0]);
        size_t L = strlen(w);
        if (digit && w[L - 1] == 'x') { lev = atoi(w); continue; }
        if (digit || nw >= 0 || !strcmp(w, "hundred") || !strcmp(w, "thousand") || !strcmp(w, "k")) {
            in_num = true;
            if (digit) cur += atof(w);
            else if (nw >= 0) cur += nw;
            else if (!strcmp(w, "hundred")) cur = (cur ? cur : 1) * 100;
            else { acc += (cur ? cur : 1) * 1000; cur = 0; }
            continue;
        }
        double val = acc + cur;
        if (in_num) {
            if (!strcmp(w, "x") || !strcmp(w, "times") || !strcmp(w, "leverage")) lev = (int)val;
            else if (!strcmp(w, "usdc") || !strcmp(w, "usd") || !strcmp(w, "dollars") || !strcmp(w, "bucks")) size = val;
            acc = cur = 0;
            in_num = false;
            if (lev && !strcmp(w, "x")) continue;
        }
        if (!strcmp(w, "long") || !strcmp(w, "buy")) side = 1;
        else if (!strcmp(w, "short") || !strcmp(w, "sell")) side = 0;
        else if (!strcmp(w, "close") || !strcmp(w, "flatten")) close = true;
        else if (!strcmp(w, "btc") || !strcmp(w, "bitcoin")) strcpy(coin, "BTC");
        else if (!strcmp(w, "eth") || !strcmp(w, "ether") || !strcmp(w, "ethereum")) strcpy(coin, "ETH");
        else if (!strcmp(w, "sol") || !strcmp(w, "solana")) strcpy(coin, "SOL");
        else if (!strcmp(w, "hype") || !strcmp(w, "hyperliquid")) {
            if (!coin[0] && strcmp(w, "hyperliquid")) strcpy(coin, "HYPE");
            if (!strcmp(w, "hyperliquid")) venue = V_HL;
        } else if (!strcmp(w, "pacifica")) venue = V_PAC;
        else if (!strcmp(w, "lighter")) venue = V_LIT;
        else if (!coin[0] && L >= 2 && L < sizeof coin && !strstr(" my me to the at on of and for in it is with position ", w)) {
            char up[12];   /* any coin listed on a venue: "long doge", "short pump" */
            for (size_t i = 0; i <= L; i++) up[i] = (char)toupper((unsigned char)w[i]);
            for (int v = 0; v < V_COUNT && !coin[0]; v++)
                if (dw_venue_enabled(v) && dw_mkt_find(v, up)) snprintf(coin, sizeof coin, "%s", up);
        }
    }
    if (!coin[0] && s_vo_coin[0]) {   /* "long, five x, fifty" on a market's own page */
        snprintf(coin, sizeof coin, "%s", s_vo_coin);
        if (venue < 0) venue = s_vo_venue;
    }
    if (in_num && size == 0 && acc + cur >= 10) size = acc + cur;
    if (!coin[0] || (side < 0 && !close)) return false;
    memset(t, 0, sizeof *t);
    t->venue = venue >= 0 ? venue : V_HL;
    if (!dw_mkt_find(t->venue, coin)) {
        for (int v = 0; v < V_COUNT; v++) if (dw_mkt_find(v, coin) && g_model.acct[v].linked) { t->venue = v; break; }
    }
    snprintf(t->coin, sizeof t->coin, "%s", coin);
    if (close) {
        dw_pos_t *p = dw_pos_find(t->venue, coin);
        if (!p) for (int v = 0; v < V_COUNT && !p; v++) if ((p = dw_pos_find(v, coin))) t->venue = v;
        if (!p) return false;
        t->buy = p->szi < 0;
        t->reduce = true;
        t->size_base = fabs(p->szi);
        t->label = "Close";
        return true;
    }
    t->buy = side == 1;
    t->lev = lev > 0 ? lev : 5;
    double px = dw_price(t->venue, coin);
    if (size > 0 && px > 0) {
        dw_mkt_t *m = dw_mkt_find(t->venue, coin);
        double f = pow(10, m ? m->sz_dec : 3);
        t->size_base = floor(size / px * f) / f;
    } else {
        t->size_pct = 25;
    }
    return true;
}

static void vo_show(const char *said)
{
    lv_label_set_text(s_vo_said, said);
    lv_obj_clean(s_vo_kv);
    if (!s_vo_ok) {
        ui_label(s_vo_kv, &dw_sans_12, C_MUTE, "Try: \"Long SOL, five x, five hundred USDC\" or \"Close my ETH\".");
        lv_obj_add_flag(s_vo_sign, LV_OBJ_FLAG_HIDDEN);
        dw_hint(NULL, "back", NULL, false);
        return;
    }
    char v[64];
    lv_obj_t *side = ui_kv_add(s_vo_kv, "Side", g_ticket.reduce ? "CLOSE" : g_ticket.buy ? "LONG" : "SHORT",
                               g_ticket.reduce ? C_INK : g_ticket.buy ? C_NEON : C_BAD);
    lv_obj_set_style_text_font(side, &dw_monob_13, 0);
    snprintf(v, sizeof v, "%s-PERP \xC2\xB7 %s", g_ticket.coin, dw_venue_name(g_ticket.venue));
    ui_kv_add(s_vo_kv, "Market", v, 0);
    if (!g_ticket.reduce) {
        snprintf(v, sizeof v, "%d\xC3\x97", g_ticket.lev);
        ui_kv_add(s_vo_kv, "Leverage", v, 0);
    }
    snprintf(v, sizeof v, "%s USDC", fmt_num(dw_ticket_notional(), 2));
    ui_kv_add(s_vo_kv, "Size", v, 0);
    double liq = dw_ticket_liq();
    if (liq > 0) ui_kv_add(s_vo_kv, "Liq. price", fmt_px(liq), C_BAD);
    lv_obj_remove_flag(s_vo_sign, LV_OBJ_FLAG_HIDDEN);
    dw_hint(NULL, "cancel", "review", false);   /* review opens the sheet; only signing needs the key */
}

static void vo_result(const char *text)
{
    if (!text || !*text) {
        s_vo_ok = false;
        vo_show("Didn't catch that.");
        return;
    }
    char q[200];
    snprintf(q, sizeof q, "\xE2\x80\x9C%s\xE2\x80\x9D", text);
    bool question = strchr(text, '?') || !strncasecmp(text, "how", 3) || !strncasecmp(text, "what", 4) ||
                    !strncasecmp(text, "why", 3) || !strncasecmp(text, "should", 6) || !strncasecmp(text, "is ", 3);
    dw_ticket_t t;
    s_vo_ok = !question && dw_parse_order(text, &t);
    if (s_vo_ok) g_ticket = t;
    if (question) {
        extern void dw_ask(const char *q);
        dw_nav_replace(&PG_ASK);
        dw_ask(text);
        return;
    }
    vo_show(q);
}

static void vo_review_click(lv_event_t *e)
{
    (void)e;
    if (s_vo_ok) dw_sheet_open();
}

static void vo_build(lv_obj_t *body)
{
    dw_bar("VOICE", "\xC2\xB7 SHORTCUT", C_NEON);
    dw_hint(NULL, "back", NULL, false);
    ui_eyebrow(body, "YOU SAID");
    lv_obj_t *c = ui_card(body, K_PLAIN);
    char hello[120];
    if (s_vo_coin[0]) snprintf(hello, sizeof hello, "Hold the mic and say an order for %s: \"long, five x, fifty USDC\".", s_vo_coin);
    else snprintf(hello, sizeof hello, "Hold the mic and say an order: \"long SOL, five x, fifty USDC\".");
    s_vo_said = ui_label(c, &dw_sans_14, C_INK, hello);
    lv_obj_set_width(s_vo_said, LV_PCT(100));
    lv_label_set_long_mode(s_vo_said, LV_LABEL_LONG_WRAP);
    ui_eyebrow(body, "TICKET");
    s_vo_kv = ui_kv(body);
    lv_obj_set_style_border_width(s_vo_kv, 1, 0);
    lv_obj_set_style_border_color(s_vo_kv, lv_color_hex(C_LINE2), 0);
    lv_obj_set_style_pad_all(s_vo_kv, 8, 0);
    s_vo_sign = ui_sign(body, "TAP OR PRESS THE TRADE KEY TO REVIEW");
    lv_obj_add_flag(s_vo_sign, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_vo_sign, vo_review_click, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_vo_sign, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *sp = lv_obj_create(body);
    lv_obj_remove_style_all(sp);
    lv_obj_set_flex_grow(sp, 1);
    ui_ptt(body, "TO SPEAK");
    dw_voice_sink_set(vo_result);
    s_vo_ok = false;
}

static bool vo_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_PTT_DOWN:
        dw_voice_sink_set(vo_result);
        lv_label_set_text(s_vo_said, "Listening\xE2\x80\xA6");
        return true;
    case DW_IN_PTT_UP: return true;
    case DW_IN_TRADE:
        if (s_vo_ok) dw_sheet_open();
        return true;
    default: return false;
    }
}

static void vo_leave(void)
{
    dw_voice_sink_set(NULL);
    s_vo_coin[0] = 0;
    s_vo_venue = -1;
}

void dw_open_voice(int venue, const char *coin)
{
    s_vo_venue = venue;
    snprintf(s_vo_coin, sizeof s_vo_coin, "%s", coin ? coin : "");
    dw_nav_push(&PG_VOICE);
}

const dw_page_t PG_VOICE = {.name = "voice", .build = vo_build, .input = vo_input, .leave = vo_leave};

/* ======================= Ask AI ======================= */

static lv_obj_t *s_ask_q, *s_ask_a;
static dw_focus_t s_ask_f;
static uint32_t s_ask_token;
static const char *SUGGEST[] = {
    "How far is my SOL long from liquidation?",
    "What is funding doing on my positions?",
    "Summarise my book in one line.",
};

static void on_answer(void *ctx, const char *text)
{
    if ((uint32_t)(uintptr_t)ctx != s_ask_token || !s_ask_a) return;
    lv_label_set_text(s_ask_a, text);
    hal_led(DW_LED_OFF, DW_LED_SOLID);
}

void dw_ask(const char *q)
{
    if (!s_ask_q) return;
    char t[220];
    snprintf(t, sizeof t, "\xE2\x80\x9C%s\xE2\x80\x9D", q);
    lv_label_set_text(s_ask_q, t);
    lv_label_set_text(s_ask_a, "Thinking\xE2\x80\xA6");
    s_ask_token++;
    dw_relay_ask(q, on_answer, (void *)(uintptr_t)s_ask_token);
}

static void ask_open(int i)
{
    if (i < 3) dw_ask(SUGGEST[i]);
}

static void ask_voice(const char *text)
{
    if (text && *text) dw_ask(text);
    else lv_label_set_text(s_ask_a, "Didn't catch that. Hold the mic while you ask.");
}

static void ask_build(lv_obj_t *body)
{
    dw_bar("ASK", "\xC2\xB7 AI", C_HOT_TXT);
    dw_hint("move", "menu", "ask", false);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_row(body, 8, 0);
    ui_eyebrow(body, "YOU ASKED");
    lv_obj_t *c = ui_card(body, K_PLAIN);
    s_ask_q = ui_label(c, &dw_sans_14, C_INK, "Hold the mic, or pick a question below.");
    lv_obj_set_width(s_ask_q, LV_PCT(100));
    lv_label_set_long_mode(s_ask_q, LV_LABEL_LONG_WRAP);
    lv_obj_t *a = ui_card(body, K_HOT);
    ui_label(a, &dw_mono_9, C_HOT_TXT, "ANSWER \xC2\xB7 READ-ONLY, IT CAN'T TRADE");
    s_ask_a = ui_label(a, &dw_sans_14, C_INK, "\xE2\x80\x94");
    lv_obj_set_width(s_ask_a, LV_PCT(100));
    lv_label_set_long_mode(s_ask_a, LV_LABEL_LONG_WRAP);
    ui_eyebrow(body, "SUGGESTED");
    dw_focus_reset(&s_ask_f, ask_open);
    for (int i = 0; i < 3; i++) {
        lv_obj_t *r = ui_card(body, K_PLAIN);
        lv_obj_t *l = ui_label(r, &dw_mono_11, C_SOFT, SUGGEST[i]);
        lv_obj_set_width(l, LV_PCT(100));
        lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
        dw_focus_add(&s_ask_f, r);
    }
    ui_ptt(body, "TO ASK");
    dw_voice_sink_set(ask_voice);
}

static bool ask_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_ask_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_ask_f, 1); return true;
    case DW_IN_TRADE: ask_open(s_ask_f.i); return true;
    case DW_IN_PTT_DOWN:
        dw_voice_sink_set(ask_voice);
        lv_label_set_text(s_ask_q, "Listening\xE2\x80\xA6");
        return true;
    case DW_IN_PTT_UP: return true;
    default: return false;
    }
}

static void ask_leave(void)
{
    s_ask_q = s_ask_a = NULL;
    dw_voice_sink_set(NULL);
}

const dw_page_t PG_ASK = {.name = "ask", .build = ask_build, .input = ask_input, .leave = ask_leave, .top = true};
