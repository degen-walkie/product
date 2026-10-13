/* Boot, menu, rooms, live room, markets, positions, news, device. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "dw_app.h"
#include "dw_pages.h"
#include "dw_venue.h"
#include "relay.h"

static uint32_t chg_color(double v) { return v >= 0 ? C_NEON : C_BAD; }

/* A "sub" line made of a muted part and a coloured part (one text run, two colours). */
static lv_obj_t *two_tone(lv_obj_t *parent, const char *a, const char *b, uint32_t bc)
{
    lv_obj_t *sg = lv_spangroup_create(parent);
    lv_obj_set_style_text_font(sg, &dw_mono_11, 0);
    lv_spangroup_set_mode(sg, LV_SPAN_MODE_EXPAND);
    lv_span_t *sa = lv_spangroup_new_span(sg);
    lv_style_set_text_color(lv_span_get_style(sa), lv_color_hex(C_MUTE));
    lv_span_set_text(sa, a);
    lv_span_t *sb = lv_spangroup_new_span(sg);
    lv_style_set_text_color(lv_span_get_style(sb), lv_color_hex(bc));
    lv_span_set_text(sb, b);
    return sg;
}
static void two_tone_set(lv_obj_t *sg, const char *a, const char *b, uint32_t bc)
{
    lv_span_set_text(lv_spangroup_get_child(sg, 0), a);
    lv_span_t *sb = lv_spangroup_get_child(sg, 1);
    lv_span_set_text(sb, b);
    lv_style_set_text_color(lv_span_get_style(sb), lv_color_hex(bc));
    lv_spangroup_refr_mode(sg);
}

/* ======================= Boot ======================= */

static void boot_done(lv_timer_t *t)
{
    (void)t;
    if (dw_nav_top() != &PG_BOOT) return;   /* powered off during the animation */
    dw_nav_root(!dw_app_setup_done() ? &PG_SETUP_LANG : dw_pin_set() ? &PG_UNLOCK : &PG_ROOMS);
}

static void boot_build(lv_obj_t *body)
{
    dw_bar("DEGEN WALKIE", "", C_NEON);
    dw_hint(NULL, NULL, NULL, false);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *t = ui_label(body, &dw_disp_32, C_INK, "DEGEN");
    (void)t;
    ui_label(body, &dw_disp_32, C_NEON, "WALKIE");
    ui_label(body, &dw_mono_11, C_MUTE, "GENESIS \xC2\xB7 FW " DW_FW_VERSION);
    ui_label(body, &dw_mono_9, C_DIM, "TALK YOUR BOOK. THEN TRADE IT.");
    lv_timer_t *tm = lv_timer_create(boot_done, 1600, NULL);
    lv_timer_set_repeat_count(tm, 1);
}

const dw_page_t PG_BOOT = {.name = "boot", .build = boot_build};

/* ======================= Menu ======================= */

static dw_focus_t s_menu_f;
static const struct { const char *t, *s; const dw_page_t *p; } MENU[] = {
    {"Rooms", "LIVE ROOMS \xC2\xB7 ONE PER TICKER", &PG_ROOMS},
    {"Positions", "EQUITY \xC2\xB7 3 VENUES", &PG_POSITIONS},
    {"Markets", "PERPS \xC2\xB7 BY VENUE", &PG_MARKETS},
    {"News", "TRADING NEWS \xC2\xB7 YOUR MARKETS", &PG_NEWS},
    {"Ask AI", "ABOUT YOUR BOOK \xC2\xB7 READ-ONLY", &PG_ASK},
    {"Accounts", "TRADE-ONLY KEYS \xC2\xB7 LINK \xC2\xB7 REVOKE", &PG_ACCOUNTS},
    {"Device", "BATTERY \xC2\xB7 STATUS LIGHT", &PG_DEVICE},
};

static void menu_open(int i) { dw_nav_root(MENU[i].p); }

static void menu_build(lv_obj_t *body)
{
    dw_bar("MENU", "", C_NEON);
    dw_hint("move", "close", "open", false);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    ui_title(body, "Menu", false);
    dw_focus_reset(&s_menu_f, menu_open);
    lv_obj_t *list = ui_col(body, 0);
    for (size_t i = 0; i < sizeof MENU / sizeof MENU[0]; i++)
        dw_focus_add(&s_menu_f, ui_row(list, MENU[i].t, MENU[i].s, NULL, NULL, false).obj);
    const dw_page_t *under = NULL;
    (void)under;
}

static bool menu_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_menu_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_menu_f, 1); return true;
    case DW_IN_TRADE: menu_open(s_menu_f.i); return true;
    default: return false;
    }
}

const dw_page_t PG_MENU = {.name = "menu", .build = menu_build, .input = menu_input};

/* ======================= Rooms ======================= */

static dw_focus_t s_rooms_f;
static lv_obj_t *s_rooms_list;
static int s_rooms_built = -1;
static ui_row_t s_room_rows[DW_MAX_ROOMS];
static lv_obj_t *s_room_sub[DW_MAX_ROOMS];

static void rooms_open(int i)
{
    if (i < g_model.nrooms) dw_open_room(g_model.rooms[i].id);
    else dw_toast("Private rooms: type the 4-digit number on the room's host device.", C_MUTE);
}

static void rooms_rebuild(void)
{
    lv_obj_clean(s_rooms_list);
    int keep = s_rooms_f.i;
    dw_focus_reset(&s_rooms_f, rooms_open);
    s_rooms_f.i = keep;
    for (int i = 0; i < g_model.nrooms; i++) {
        dw_room_t *r = &g_model.rooms[i];
        char t[20];
        snprintf(t, sizeof t, "#%s", r->name);
        s_room_rows[i] = ui_row(s_rooms_list, t, NULL, "", "listening", true);
        lv_obj_t *left = lv_obj_get_child(s_room_rows[i].obj, 0);
        s_room_sub[i] = two_tone(left, "", "", C_NEON);
        dw_focus_add(&s_rooms_f, s_room_rows[i].obj);
    }
    dw_focus_add(&s_rooms_f, ui_btn(s_rooms_list, "+ Join by number", K_PLAIN));
    lv_obj_set_style_margin_top(s_rooms_f.items[s_rooms_f.n - 1], 10, 0);
    s_rooms_built = g_model.nrooms;
    dw_focus_set(&s_rooms_f, keep);
}

static void rooms_update(void)
{
    if (s_rooms_built != g_model.nrooms) rooms_rebuild();
    for (int i = 0; i < g_model.nrooms; i++) {
        dw_room_t *r = &g_model.rooms[i];
        char a[48];
        double px = r->venue >= 0 ? dw_price(r->venue, r->coin) : 0;
        if (px > 0) {
            snprintf(a, sizeof a, "%s \xC2\xB7 %s \xC2\xB7 ", r->venue_name, fmt_px(px));
            double ch = dw_change_pct(r->venue, r->coin);
            two_tone_set(s_room_sub[i], a, fmt_pct(ch), chg_color(ch));
        } else {
            two_tone_set(s_room_sub[i], r->venue >= 0 ? r->venue_name
                                        : strcmp(r->id, "world") ? "Private \xC2\xB7 your crew" : "Everyone \xC2\xB7 every market", "", C_MUTE);
        }
        lv_label_set_text(s_room_rows[i].val, fmt_num(r->listening, 0));
    }
}

static void rooms_build(lv_obj_t *body)
{
    dw_bar("ROOMS", "", C_NEON);
    dw_hint("move", "menu", "open room", false);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    ui_title(body, "Rooms", false);
    ui_sub(body, "ROLL TO PICK \xC2\xB7 TRADE KEY TO JOIN", 0);
    s_rooms_list = ui_col(body, 0);
    s_rooms_built = -1;
    rooms_rebuild();
}

static bool rooms_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_rooms_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_rooms_f, 1); return true;
    case DW_IN_TRADE: rooms_open(s_rooms_f.i); return true;
    default: return false;
    }
}

const dw_page_t PG_ROOMS = {.name = "rooms", .build = rooms_build, .update = rooms_update, .input = rooms_input, .top = true};

/* ======================= Room ======================= */

static dw_room_t s_room;
static int s_tab;
static lv_obj_t *s_room_px, *s_room_chg, *s_room_area, *s_room_feed, *s_room_chart, *s_room_big, *s_room_bigsub, *s_room_orders;
static int s_feed_seen = -1;
static uint32_t s_cnd_req;
static int room_open_orders(void);

void dw_open_room(const char *room_id)
{
    for (int i = 0; i < g_model.nrooms; i++) {
        if (strcmp(g_model.rooms[i].id, room_id)) continue;
        s_room = g_model.rooms[i];
        s_tab = 0;
        dw_relay_join(room_id);
        dw_nav_push(&PG_ROOM);
        return;
    }
}

static void room_say(const char *text)
{
    if (text && *text) dw_relay_say(text);
    else dw_toast("Didn't catch that. Hold the mic while you talk.", C_MUTE);
}

static void feed_render(void)
{
    if (!s_room_feed) return;
    lv_obj_clean(s_room_feed);
    int first = g_model.nmsgs > 6 ? g_model.nmsgs - 6 : 0;
    for (int i = first; i < g_model.nmsgs; i++) {
        dw_msg_t *m = &g_model.msgs[i];
        lv_obj_t *c = ui_col(s_room_feed, 1);
        lv_obj_t *who = ui_rowbox(c, 6);
        lv_obj_set_flex_align(who, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        ui_label(who, &dw_monob_11, m->me ? C_HOT_TXT : C_NEON, m->who);
        ui_label(who, &dw_mono_9, C_MUTE, m->lang);
        lv_obj_t *t = ui_label(c, &dw_sans_14, C_INK, m->text);
        lv_obj_set_width(t, LV_PCT(100));
        lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
    }
    s_feed_seen = g_model.nmsgs;
    lv_obj_update_layout(s_room_feed);
    lv_obj_scroll_to_y(s_room_feed, LV_COORD_MAX, LV_ANIM_OFF);
}

static int s_cnd_tries;
static void room_candles_load(void);

static void room_candles_retry(lv_timer_t *t)
{
    if ((uint32_t)(uintptr_t)lv_timer_get_user_data(t) == s_cnd_req && s_room_chart) room_candles_load();
}

static void on_room_candles(void *ctx, const dw_candle_t *c, int n)
{
    if ((uint32_t)(uintptr_t)ctx != s_cnd_req || !s_room_chart) return;
    if (n == 0 && s_cnd_tries < 3) {   /* a dropped request or an empty answer: try again shortly */
        lv_timer_t *t = lv_timer_create(room_candles_retry, 1500, (void *)(uintptr_t)s_cnd_req);
        lv_timer_set_repeat_count(t, 1);
        return;
    }
    ui_chart_set(s_room_chart, c, n, dw_price(s_room.venue, s_room.coin), 0);
}

static void room_candles_load(void)
{
    s_cnd_tries++;
    const dw_venue_t *v = dw_venue(s_room.venue);
    if (v) v->candles(s_room.coin, "15m", 48, on_room_candles, (void *)(uintptr_t)s_cnd_req);
}

static void room_tab_build(void)
{
    lv_obj_clean(s_room_area);
    s_room_feed = s_room_chart = s_room_big = s_room_bigsub = s_room_orders = NULL;
    static const char *TABS[] = {"Chat", "Chart", "Orders"};
    ui_tabs(s_room_area, TABS, s_room.venue >= 0 ? 3 : 1, s_tab);   /* no market: chat only */
    if (s_tab == 0) {
        s_room_feed = ui_col(s_room_area, 9);
        lv_obj_set_flex_grow(s_room_feed, 1);
        lv_obj_set_height(s_room_feed, 0);
        lv_obj_add_flag(s_room_feed, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(s_room_feed, LV_SCROLLBAR_MODE_OFF);
        s_feed_seen = -1;
        feed_render();
        ui_ptt(s_room_area, "TO TALK");
    } else if (s_tab == 1) {
        s_room_big = ui_big(s_room_area, "", "USDC", &dw_mono_30);
        s_room_bigsub = ui_sub(s_room_area, "", C_NEON);
        s_room_chart = ui_chart(s_room_area, 190);
        s_cnd_req++;
        s_cnd_tries = 0;
        room_candles_load();
        ui_eyebrow(s_room_area, "15 MIN CANDLES \xC2\xB7 LIVE");
    } else {
        ui_eyebrow(s_room_area, "YOUR OPEN ORDERS & POSITION");
        s_room_orders = ui_col(s_room_area, 0);
        ui_eyebrow(s_room_area, "FILLS LIGHT THE STATUS LIGHT GREEN FOR 3 S");
    }
    dw_hint("page", "back", "", false);
    char t[24];
    snprintf(t, sizeof t, "trade %s", s_room.coin);
    dw_hint("page", "back", s_room.venue >= 0 ? t : NULL, false);
}

static void room_update(void)
{
    double px = dw_price(s_room.venue, s_room.coin), ch = dw_change_pct(s_room.venue, s_room.coin);
    if (s_room_px) {
        lv_label_set_text(s_room_px, px > 0 ? fmt_px(px) : "");
        lv_label_set_text(s_room_chg, px > 0 ? fmt_pct(ch) : "");
        lv_obj_set_style_text_color(s_room_chg, lv_color_hex(chg_color(ch)), 0);
    }
    char sub[40];
    int listening = s_room.listening;
    for (int i = 0; i < g_model.nrooms; i++)
        if (!strcmp(g_model.rooms[i].id, s_room.id)) listening = g_model.rooms[i].listening;
    snprintf(sub, sizeof sub, "\xC2\xB7 %s", fmt_num(listening, 0));
    char lab[20];
    snprintf(lab, sizeof lab, "#%s", s_room.name);
    dw_bar(lab, sub, C_NEON);
    if (s_room_feed && s_feed_seen != g_model.nmsgs) feed_render();
    if (s_room_big) {
        ui_big_set(s_room_big, fmt_px(px), 0);
        char s[48];
        snprintf(s, sizeof s, "%s \xC2\xB7 24 H \xC2\xB7 %s", fmt_pct(ch), dw_venue_name(s_room.venue));
        lv_label_set_text(s_room_bigsub, s);
        lv_obj_set_style_text_color(s_room_bigsub, lv_color_hex(chg_color(ch)), 0);
        ui_chart_set(s_room_chart, NULL, 0, px, 0);
        dw_pos_t *p = dw_pos_find(s_room.venue, s_room.coin);
        if (p) ui_chart_set(s_room_chart, NULL, 0, px, p->entry);
    }
    if (s_room_orders) {
        lv_obj_clean(s_room_orders);
        int n = 0;
        dw_pos_t *p = dw_pos_find(s_room.venue, s_room.coin);
        if (p) {
            char t[48], s[64];
            snprintf(t, sizeof t, "%s %s %s", p->szi > 0 ? "Long" : "Short", fmt_num(fabs(p->szi), 4), p->coin);
            snprintf(s, sizeof s, "Entry %s \xC2\xB7 %.0f\xC3\x97", fmt_px(p->entry), p->lev);
            ui_row_t r = ui_row(s_room_orders, t, s, fmt_signed(p->upnl, 2), "USDC PnL", true);
            lv_obj_set_style_text_color(r.val, lv_color_hex(chg_color(p->upnl)), 0);
            n++;
        }
        for (int i = 0; i < g_model.nord; i++) {
            dw_ord_t *o = &g_model.ord[i];
            if (o->venue != s_room.venue || strcmp(o->coin, s_room.coin)) continue;
            char t[48], s[64];
            bool trig = strcmp(o->kind, "LIMIT") && o->kind[0];
            snprintf(t, sizeof t, "%s%s %s %s", trig ? o->kind : "", trig ? " \xC2\xB7" : "", o->buy ? "Buy" : "Sell", fmt_num(o->sz, 4));
            snprintf(s, sizeof s, "%s %s \xC2\xB7 %s", trig ? "Trigger" : "Limit", fmt_px(o->px), dw_venue_name(o->venue));
            ui_row_t rr = ui_row(s_room_orders, t, s, fmt_num(o->px * o->sz, 2), trig ? "waiting" : "open", true);
            if (trig) lv_obj_set_style_text_color(rr.title, lv_color_hex(!strcmp(o->kind, "TP") ? C_NEON : C_BAD), 0);
            n++;
        }
        if (!n) ui_label(s_room_orders, &dw_sans_12, C_MUTE, "Nothing open in this market yet.");
        char tr[24];
        snprintf(tr, sizeof tr, "trade %s", s_room.coin);
        dw_hint("page", "back", room_open_orders() ? "cancel all" : (s_room.venue >= 0 ? tr : NULL), false);
    }
}

static int room_open_orders(void)
{
    int n = 0;
    for (int i = 0; i < g_model.nord; i++)
        if (g_model.ord[i].venue == s_room.venue && !strcmp(g_model.ord[i].coin, s_room.coin)) n++;
    return n;
}

static void on_cancelled(void *ctx, const dw_order_res_t *r)
{
    (void)ctx;
    if (r->ok) dw_light_ok();
    else { dw_light_fail(); dw_toast(r->msg, C_BAD); }
}

static void cancel_all(void)
{
    const dw_venue_t *v = dw_venue(s_room.venue);
    int n = 0;
    for (int i = 0; i < g_model.nord; i++) {
        dw_ord_t *o = &g_model.ord[i];
        if (o->venue != s_room.venue || strcmp(o->coin, s_room.coin) || !v->cancel) continue;
        if (v->cancel(o->coin, o->oid, on_cancelled, NULL) == 0) n++;
    }
    dw_light_sending();
    char t[64];
    snprintf(t, sizeof t, "Cancelling %d order%s\xE2\x80\xA6", n, n == 1 ? "" : "s");
    dw_toast(t, C_BLUE);
}

static void tab_switch(void *arg)
{
    (void)arg;
    if (!s_room_area) return;
    room_tab_build();
    room_update();
}

static void tab_click(lv_event_t *e)
{
    lv_obj_t *t = lv_event_get_target(e);
    lv_obj_t *tabs = lv_obj_get_parent(t);
    int idx = (int)lv_obj_get_index(t);
    if (lv_obj_get_parent(tabs) != s_room_area || idx > 2 || idx == s_tab) return;
    if (s_room.venue < 0 && idx > 0) return;   /* the world room has chat only */
    s_tab = idx;
    lv_async_call(tab_switch, NULL);   /* rebuilding deletes the tab that was tapped: not inside its event */
}

static void room_build(lv_obj_t *body)
{
    lv_obj_set_style_pad_row(body, 8, 0);
    lv_obj_t *strip = ui_rowbox(body, 8);
    lv_obj_set_style_pad_all(strip, 8, 0);
    lv_obj_set_style_radius(strip, 6, 0);
    lv_obj_set_style_border_width(strip, 1, 0);
    lv_obj_set_style_border_color(strip, lv_color_hex(C_LINE2), 0);
    char t[24];
    snprintf(t, sizeof t, "ROOM #%s", s_room.name);
    lv_obj_t *tl = ui_label(strip, &dw_disp_20, C_INK, t);
    lv_obj_set_flex_grow(tl, 1);
    s_room_px = ui_label(strip, &dw_mono_13, C_INK, "");
    s_room_chg = ui_label(strip, &dw_mono_13, C_NEON, "");
    s_room_area = ui_col(body, 8);
    lv_obj_set_flex_grow(s_room_area, 1);
    lv_obj_set_height(s_room_area, 0);
    lv_obj_add_event_cb(s_room_area, tab_click, LV_EVENT_CLICKED, NULL);
    dw_voice_sink_set(room_say);
    room_tab_build();
}

static bool room_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP:
    case DW_IN_WHEEL_DOWN:
        if (s_room.venue < 0) return true;
        s_tab = (s_tab + (in == DW_IN_WHEEL_DOWN ? 1 : 2)) % 3;
        room_tab_build();
        room_update();
        return true;
    case DW_IN_TRADE:
        if (s_tab == 2 && room_open_orders()) {
            char b[120];
            snprintf(b, sizeof b, "%d open order%s on %s-PERP, %s. Positions stay open.", room_open_orders(),
                     room_open_orders() == 1 ? "" : "s", s_room.coin, dw_venue_name(s_room.venue));
            dw_confirm_open("Cancel orders?", b, "PRESS THE TRADE KEY TO CANCEL", cancel_all);
        } else if (s_room.venue >= 0) {
            dw_open_ticket(s_room.venue, s_room.coin, true);
        }
        return true;
    case DW_IN_PTT_DOWN:
        if (s_tab != 0) { s_tab = 0; room_tab_build(); room_update(); }
        dw_voice_sink_set(room_say);
        return true;
    case DW_IN_PTT_UP: return true;
    default: return false;
    }
}

static void room_leave(void)
{
    s_room_px = s_room_chg = s_room_feed = s_room_chart = s_room_big = s_room_orders = NULL;
    dw_voice_sink_set(NULL);
}

const dw_page_t PG_ROOM = {.name = "room", .build = room_build, .update = room_update, .input = room_input, .leave = room_leave};

/* ======================= Markets ======================= */

static dw_focus_t s_mk_f;
static struct { int venue; char coin[12]; ui_row_t row; } s_mk[DW_MAX_MKT * V_COUNT];
static int s_mk_n = -1;
static lv_obj_t *s_mk_list;

static void mk_open(int i) { if (i < s_mk_n) dw_open_chart(s_mk[i].venue, s_mk[i].coin); }

static void mk_update(void)
{
    int total = 0;
    for (int v = 0; v < V_COUNT; v++) total += g_model.nmkt[v];
    if (total != s_mk_n) {
        lv_obj_clean(s_mk_list);
        dw_focus_reset(&s_mk_f, mk_open);
        s_mk_n = 0;
        for (int v = 0; v < V_COUNT; v++) {
            if (!g_model.nmkt[v]) continue;
            lv_obj_t *e = ui_eyebrow(s_mk_list, dw_venue_name(v));
            lv_obj_set_style_pad_top(e, 8, 0);
            for (int i = 0; i < g_model.nmkt[v]; i++) {
                char t[20];
                snprintf(t, sizeof t, "%s-PERP", g_model.mkt[v][i].coin);
                s_mk[s_mk_n].venue = v;
                snprintf(s_mk[s_mk_n].coin, 12, "%s", g_model.mkt[v][i].coin);
                s_mk[s_mk_n].row = ui_row(s_mk_list, t, "", "", "", true);
                dw_focus_add(&s_mk_f, s_mk[s_mk_n].row.obj);
                s_mk_n++;
            }
        }
        dw_focus_set(&s_mk_f, 0);
    }
    for (int i = 0; i < s_mk_n; i++) {
        dw_mkt_t *m = dw_mkt_find(s_mk[i].venue, s_mk[i].coin);
        if (!m) continue;
        char s[48];
        snprintf(s, sizeof s, "%d\xC3\x97 MAX \xC2\xB7 FUNDING %+.4f%%/H", m->max_lev, m->funding_h * 100);
        lv_label_set_text(s_mk[i].row.sub, s);
        lv_label_set_text(s_mk[i].row.val, m->mid > 0 ? fmt_px(m->mid) : "\xE2\x80\x94");
        double ch = dw_change_pct(s_mk[i].venue, m->coin);
        lv_label_set_text(s_mk[i].row.small, fmt_pct(ch));
        lv_obj_set_style_text_color(s_mk[i].row.small, lv_color_hex(chg_color(ch)), 0);
    }
}

static void mk_say(lv_event_t *e) { (void)e; dw_open_voice(-1, NULL); }

static void mk_build(lv_obj_t *body)
{
    dw_bar("MARKETS", "\xC2\xB7 PERPS", C_NEON);
    dw_hint("move", "menu", "open chart", false);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    ui_title(body, "Markets", false);
    ui_sub(body, "THREE VENUES \xC2\xB7 FUNDS STAY ON EACH", 0);
    lv_obj_t *say = ui_btn(body, "SAY AN ORDER", K_PLAIN);   /* no talk key on the device */
    lv_obj_set_width(say, LV_PCT(100));
    lv_obj_add_event_cb(say, mk_say, LV_EVENT_CLICKED, NULL);
    s_mk_list = ui_col(body, 0);
    s_mk_n = -1;
}

static bool mk_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_mk_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_mk_f, 1); return true;
    case DW_IN_TRADE: mk_open(s_mk_f.i); return true;
    default: return false;
    }
}

const dw_page_t PG_MARKETS = {.name = "markets", .build = mk_build, .update = mk_update, .input = mk_input, .top = true};

/* ======================= Positions ======================= */

static dw_focus_t s_pos_f;
static lv_obj_t *s_pos_eq, *s_pos_pnl, *s_pos_list;
static int s_pos_n = -1;
static struct { lv_obj_t *pnl, *sub, *bar, *lbl; int venue; char coin[12]; } s_pr[DW_MAX_POS];

static void pos_open(int i) { if (i < s_pos_n) dw_open_chart(s_pr[i].venue, s_pr[i].coin); }

static double liq_safety(const dw_pos_t *p)
{
    double mark = dw_price(p->venue, p->coin);
    if (p->liq <= 0 || mark <= 0) return 1;
    double dist = fabs(mark - p->liq) / mark;
    return fmin(1, dist / 0.3);  /* < 0.27 (8% away) reads as "near" */
}

static void pos_update(void)
{
    double eq = 0, upnl = 0;
    int venues = 0;
    for (int v = 0; v < V_COUNT; v++) if (g_model.acct[v].linked) { eq += g_model.acct[v].equity; venues++; }
    for (int i = 0; i < g_model.npos; i++) upnl += g_model.pos[i].upnl;
    ui_big_set(s_pos_eq, fmt_num(eq, 2), 0);
    char s[48];
    snprintf(s, sizeof s, "%s UNREALIZED \xC2\xB7 %d VENUE%s", fmt_signed(upnl, 2), venues, venues == 1 ? "" : "S");
    lv_label_set_text(s_pos_pnl, s);
    lv_obj_set_style_text_color(s_pos_pnl, lv_color_hex(chg_color(upnl)), 0);
    char bs[24];
    snprintf(bs, sizeof bs, "\xC2\xB7 %d OPEN", g_model.npos);
    dw_bar("POSITIONS", bs, C_NEON);

    if (s_pos_n != g_model.npos) {
        lv_obj_clean(s_pos_list);
        dw_focus_reset(&s_pos_f, pos_open);
        for (int i = 0; i < g_model.npos; i++) {
            dw_pos_t *p = &g_model.pos[i];
            lv_obj_t *c = ui_card(s_pos_list, K_PLAIN);
            lv_obj_t *top = ui_rowbox(c, 6);
            char t[20];
            snprintf(t, sizeof t, "%s-PERP", p->coin);
            ui_label(top, &dw_monob_13, C_INK, t);
            s_pr[i].pnl = ui_label(top, &dw_monob_13, C_NEON, "");
            lv_obj_t *mid = ui_rowbox(c, 6);
            s_pr[i].sub = ui_label(mid, &dw_mono_9, C_MUTE, "");
            s_pr[i].lbl = ui_label(mid, &dw_mono_9, 0x56606c, "LIQ. DISTANCE");
            s_pr[i].bar = ui_liqbar(c, 1);
            s_pr[i].venue = p->venue;
            snprintf(s_pr[i].coin, 12, "%s", p->coin);
            dw_focus_add(&s_pos_f, c);
        }
        if (!g_model.npos) {
            lv_obj_t *c = ui_card(s_pos_list, K_PLAIN);
            ui_label(c, &dw_sansb_15, C_INK, "No open positions");
            lv_obj_t *l = ui_label(c, &dw_sans_12, C_MUTE, "Open a room or a market and press the trade key.");
            lv_obj_set_width(l, LV_PCT(100));
            lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
        }
        s_pos_n = g_model.npos;
        dw_focus_set(&s_pos_f, 0);
    }
    for (int i = 0; i < g_model.npos && i < s_pos_n; i++) {
        dw_pos_t *p = &g_model.pos[i];
        lv_label_set_text(s_pr[i].pnl, fmt_signed(p->upnl, 2));
        lv_obj_set_style_text_color(s_pr[i].pnl, lv_color_hex(chg_color(p->upnl)), 0);
        char sb[64];
        snprintf(sb, sizeof sb, "%s %.0f\xC3\x97 \xC2\xB7 %s", p->szi > 0 ? "LONG" : "SHORT", p->lev, dw_venue_name(p->venue));
        for (char *q = sb; *q; q++) if (*q >= 'a' && *q <= 'z') *q -= 32;
        lv_label_set_text(s_pr[i].sub, sb);
        double safe = liq_safety(p);
        lv_bar_set_value(s_pr[i].bar, (int32_t)(safe * 100), LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_pr[i].bar, lv_color_hex(safe < 0.27 ? C_WARN : C_NEON), LV_PART_INDICATOR);
        lv_label_set_text(s_pr[i].lbl, safe < 0.27 ? "NEAR LIQ." : "LIQ. DISTANCE");
        lv_obj_set_style_text_color(s_pr[i].lbl, lv_color_hex(safe < 0.27 ? C_WARN : 0x56606c), 0);
    }
}

static void pos_build(lv_obj_t *body)
{
    dw_hint("move", "menu", "open", false);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_row(body, 6, 0);
    ui_eyebrow(body, "EQUITY \xC2\xB7 ALL LINKED VENUES");
    s_pos_eq = ui_big(body, "", "USDC", &dw_mono_30);
    s_pos_pnl = ui_label(body, &dw_monob_11, C_NEON, "");
    s_pos_list = ui_col(body, 8);
    lv_obj_set_style_pad_top(s_pos_list, 6, 0);
    s_pos_n = -1;
}

static bool pos_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_pos_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_pos_f, 1); return true;
    case DW_IN_TRADE: pos_open(s_pos_f.i); return true;
    default: return false;
    }
}

const dw_page_t PG_POSITIONS = {.name = "positions", .build = pos_build, .update = pos_update, .input = pos_input, .top = true};

/* ======================= News ======================= */

static dw_focus_t s_news_f;
static lv_obj_t *s_news_list;
static int64_t s_news_top = -1;
static lv_obj_t *s_news_ago[DW_MAX_NEWS];

static uint32_t tag_color(const char *tag)
{
    if (!strcmp(tag, "MACRO")) return C_HOT_TXT;
    if (!strcmp(tag, "LIQ") || strstr(tag, "LIQ")) return C_BAD;
    if (!strcmp(tag, "HYPE")) return C_WARN;
    return C_NEON;
}

static void news_open(int i)
{
    if (i >= g_model.nnews) return;
    for (int r = 0; r < g_model.nrooms; r++)
        if (!strcasecmp(g_model.rooms[r].coin, g_model.news[i].tag)) { dw_open_room(g_model.rooms[r].id); return; }
    dw_toast("No room for this one yet.", C_MUTE);
}

static void ago(int64_t ts, char *out, size_t cap)
{
    int64_t s = (hal_unix_ms() - ts) / 1000;
    if (s < 60) snprintf(out, cap, "now");
    else if (s < 3600) snprintf(out, cap, "%dm", (int)(s / 60));
    else if (s < 86400) snprintf(out, cap, "%dh", (int)(s / 3600));
    else snprintf(out, cap, "%dd", (int)(s / 86400));
}

static void news_update(void)
{
    int64_t top = g_model.nnews ? g_model.news[0].ts : 0;
    if (top != s_news_top) {
        lv_obj_clean(s_news_list);
        dw_focus_reset(&s_news_f, news_open);
        for (int i = 0; i < g_model.nnews; i++) {
            dw_news_t *n = &g_model.news[i];
            lv_obj_t *c = ui_card(s_news_list, K_PLAIN);
            lv_obj_t *r = ui_rowbox(c, 6);
            uint32_t col = tag_color(n->tag);
            lv_obj_t *b = ui_badge(r, n->tag, K_PLAIN);
            lv_obj_set_style_border_color(b, lv_color_hex(col), 0);
            lv_obj_set_style_text_color(lv_obj_get_child(b, 0), lv_color_hex(col), 0);
            s_news_ago[i] = ui_label(r, &dw_mono_9, C_MUTE, "");
            lv_obj_t *t = ui_label(c, &dw_sans_14, C_INK, n->text);
            lv_obj_set_width(t, LV_PCT(100));
            lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
            dw_focus_add(&s_news_f, c);
        }
        s_news_top = top;
        dw_focus_set(&s_news_f, 0);
    }
    for (int i = 0; i < g_model.nnews; i++) {
        char a[16], b[24];
        ago(g_model.news[i].ts, a, sizeof a);
        snprintf(b, sizeof b, i == 0 ? "NEW \xC2\xB7 %s" : "%s", a);
        lv_label_set_text(s_news_ago[i], b);
        lv_obj_set_style_text_color(s_news_ago[i], lv_color_hex(i == 0 ? C_NEON : C_MUTE), 0);
    }
}

static void news_build(lv_obj_t *body)
{
    dw_bar("NEWS", "\xC2\xB7 LIVE", C_NEON);
    dw_hint("scroll", "menu", "open room", false);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    ui_eyebrow(body, "TRADING NEWS \xC2\xB7 YOUR MARKETS");
    s_news_list = ui_col(body, 8);
    s_news_top = -1;
}

static bool news_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_news_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_news_f, 1); return true;
    case DW_IN_TRADE: news_open(s_news_f.i); return true;
    default: return false;
    }
}

const dw_page_t PG_NEWS = {.name = "news", .build = news_build, .update = news_update, .input = news_input, .top = true};

/* ======================= Device ======================= */

static lv_obj_t *s_dev_big, *s_dev_bar, *s_dev_sub;

static void dev_update(void)
{
    hal_power_t p;
    hal_power(&p);
    char t[16];
    snprintf(t, sizeof t, "%d%%", p.percent);
    ui_big_set(s_dev_big, t, 0);
    lv_bar_set_value(s_dev_bar, p.percent, LV_ANIM_OFF);
    lv_label_set_text(s_dev_sub, p.charging ? "CHARGING \xC2\xB7 USB-C" : "ON BATTERY \xC2\xB7 ABOUT 2 DAYS OF ROOMS");
    dw_bar(p.charging ? "CHARGING" : "DEVICE", "", p.charging ? C_WARN : C_NEON);
}

static void dev_build(lv_obj_t *body)
{
    dw_hint(NULL, "menu", NULL, false);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    s_dev_big = ui_big(body, "", "BATTERY", &dw_mono_30);
    s_dev_bar = ui_liqbar(body, 0.7);
    lv_obj_set_style_bg_color(s_dev_bar, lv_color_hex(C_WARN), LV_PART_INDICATOR);
    s_dev_sub = ui_sub(body, "", 0);
    lv_obj_set_style_margin_top(s_dev_sub, 0, 0);
    ui_eyebrow(body, "STATUS LIGHT");
    static const struct { uint32_t c; const char *b, *t; } L[] = {
        {C_BLUE, "Blue blink", "sending \xC2\xB7 awaiting confirmation"},
        {C_NEON, "Green 3 s", "confirmed"},
        {C_BAD, "Red \xC3\x97" "3", "failed"},
        {C_BAD, "Red every 5 s", "battery under 10%"},
        {C_WARN, "Yellow", "charging"},
        {C_NEON, "Green", "full"},
        {C_HOT, "Purple slow", "AI proposal waiting"},
    };
    for (size_t i = 0; i < sizeof L / sizeof L[0]; i++) {
        lv_obj_t *r = ui_rowbox(body, 8);
        lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_t *dot = lv_obj_create(r);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 10, 10);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(L[i].c), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_shadow_color(dot, lv_color_hex(L[i].c), 0);
        lv_obj_set_style_shadow_width(dot, 8, 0);
        ui_label(r, &dw_sansb_15, L[i].c, L[i].b);
        ui_label(r, &dw_sans_12, C_SOFT, L[i].t);
    }
    ui_eyebrow(body, "FIRMWARE " DW_FW_VERSION " \xC2\xB7 OPEN SOURCE");
}

const dw_page_t PG_DEVICE = {.name = "device", .build = dev_build, .update = dev_update, .top = true};
