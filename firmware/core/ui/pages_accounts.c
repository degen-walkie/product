/* Accounts, linking a venue (QR + approval in the main wallet), revoking. */
#include <stdio.h>
#include <string.h>

#include "dw_crypto.h"
#include "dw_pages.h"
#include "dw_venue.h"
#include "relay.h"

int g_link_venue = V_HL;

/* ======================= Accounts ======================= */

static dw_focus_t s_ac_f;
static struct { lv_obj_t *card, *badge, *big, *sub, *note; } s_ac[V_COUNT];

static void ac_open(int i)
{
    if (i < V_COUNT) {
        if (g_model.acct[i].linked) dw_toast("Linked with a trade-only key. Revoke it below, or from your main wallet.", C_NEON);
        else dw_open_link(i);
    } else {
        dw_nav_push(&PG_REVOKE);
    }
}

static const char *venue_note(int v)
{
    switch (v) {
    case V_HL: return "CAN TRADE \xC2\xB7 CAN'T WITHDRAW";
    case V_PAC: return "CAN TRADE \xC2\xB7 NO WITHDRAWALS FROM THIS DEVICE";
    default: return "CAN TRADE \xC2\xB7 WITHDRAW ONLY TO YOUR OWN ADDRESS";
    }
}

static void ac_update(void)
{
    for (int v = 0; v < V_COUNT; v++) {
        dw_acct_t *a = &g_model.acct[v];
        lv_obj_t *bl = lv_obj_get_child(s_ac[v].badge, 0);
        if (a->linked) {
            lv_label_set_text(bl, a->approved ? "TRADE-ONLY" : "CHECKING\xE2\x80\xA6");
            ui_big_set(s_ac[v].big, a->error ? "\xE2\x80\x94" : fmt_num(a->equity, 2), C_INK);
            char s[80];
            snprintf(s, sizeof s, "PERPS \xC2\xB7 %s %s", v == V_LIT ? "" : "AGENT", fmt_short(a->agent, 5, 4));
            lv_label_set_text(s_ac[v].sub, s);
            lv_label_set_text(s_ac[v].note, venue_note(v));
            lv_obj_set_style_text_color(s_ac[v].note, lv_color_hex(C_NEON), 0);
        } else {
            lv_label_set_text(bl, "NOT LINKED");
            ui_big_set(s_ac[v].big, "\xE2\x80\x94", C_DIM);
            lv_label_set_text(s_ac[v].sub, dw_venue_chain(v));
            lv_label_set_text(s_ac[v].note, "TRADE KEY TO LINK");
            lv_obj_set_style_text_color(s_ac[v].note, lv_color_hex(C_MUTE), 0);
        }
    }
}

static void ac_build(lv_obj_t *body)
{
    dw_bar("ACCOUNTS", "", C_NEON);
    dw_hint("move", "menu", "open", false);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_row(body, 8, 0);
    ui_title(body, "Accounts", false);
    ui_sub(body, "FUNDS STAY ON THE VENUE", 0);
    dw_focus_reset(&s_ac_f, ac_open);
    for (int v = 0; v < V_COUNT; v++) {
        lv_obj_t *c = ui_card(body, K_PLAIN);
        s_ac[v].card = c;
        lv_obj_t *r = ui_rowbox(c, 6);
        lv_obj_t *e = ui_eyebrow(r, dw_venue_name(v));
        lv_obj_set_style_text_color(e, lv_color_hex(C_NEON), 0);
        s_ac[v].badge = ui_badge(r, "", K_NEON);
        s_ac[v].big = ui_big(c, "", "USDC", &dw_mono_24);
        s_ac[v].sub = ui_label(c, &dw_mono_9, C_MUTE, "");
        s_ac[v].note = ui_label(c, &dw_mono_9, C_NEON, "");
        dw_focus_add(&s_ac_f, c);
    }
    lv_obj_t *rv = ui_btn(body, "REVOKE ACCESS", K_BAD);
    dw_focus_add(&s_ac_f, rv);
    dw_focus_set(&s_ac_f, 0);
}

static bool ac_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_ac_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_ac_f, 1); return true;
    case DW_IN_TRADE: ac_open(s_ac_f.i); return true;
    default: return false;
    }
}

const dw_page_t PG_ACCOUNTS = {.name = "accounts", .build = ac_build, .update = ac_update, .input = ac_input, .top = true};

/* ======================= Link ======================= */

static char s_session[17];
static char s_qr[200];
static lv_obj_t *s_lk_status, *s_lk_qr;
static lv_timer_t *s_lk_timer;
static int s_lk_state;   /* 0 waiting, 1 approved on relay, 2 confirmed by venue */
static uint32_t s_lk_since;

void dw_open_link(int venue)
{
    g_link_venue = venue;
    dw_nav_push(&PG_LINK);
}

static void lk_done_back(lv_timer_t *t)
{
    (void)t;
    if (dw_nav_top() == &PG_LINK) dw_nav_pop();
}

static void on_link(void *ctx, int status, const char *venue, const char *account, const char *extra)
{
    (void)ctx;
    (void)venue;
    if (status != 1 || s_lk_state != 0 || !s_lk_status) return;
    dw_venue(g_link_venue)->link_done(account, extra);
    s_lk_state = 1;
    s_lk_since = hal_millis();
    lv_label_set_text(s_lk_status, "Approved in your wallet. Checking with the venue\xE2\x80\xA6");
}

static void lk_tick(lv_timer_t *t)
{
    (void)t;
    if (s_lk_state == 0) {
        dw_relay_link_poll(s_session, on_link, NULL);
    } else if (s_lk_state == 1) {
        dw_venue(g_link_venue)->poll();
        dw_acct_t *a = &g_model.acct[g_link_venue];
        if (a->approved || hal_millis() - s_lk_since > 8000) {
            s_lk_state = 2;
            dw_light_ok();
            hal_vibrate(80);
            char m[120];
            snprintf(m, sizeof m, "Linked \xE2\x80\x94 %s. This key can trade, nothing else.", a->account[0] == '#' ? a->account : fmt_short(a->account, 6, 4));
            lv_label_set_text(s_lk_status, m);
            lv_obj_set_style_text_color(s_lk_status, lv_color_hex(C_NEON), 0);
            lv_timer_t *b = lv_timer_create(lk_done_back, 2200, NULL);
            lv_timer_set_repeat_count(b, 1);
        }
    }
}

static void lk_build(lv_obj_t *body)
{
    char t[40];
    int v = g_link_venue;
    dw_bar("LINK", "", C_NEON);
    snprintf(t, sizeof t, "\xC2\xB7 %s", dw_venue_name(v));
    for (char *p = t; *p; p++) if (*p >= 'a' && *p <= 'z') *p -= 32;
    dw_bar("LINK", t, C_NEON);
    dw_hint(NULL, "cancel", NULL, false);
    lv_obj_set_style_pad_row(body, 7, 0);
    snprintf(t, sizeof t, "Link %s", dw_venue_name(v));
    ui_title(body, t, true);
    ui_sub(body, "SCAN \xC2\xB7 APPROVE IN YOUR MAIN WALLET", 0);

    uint8_t rnd[8];
    hal_random_fill(rnd, 8);
    dw_hex(rnd, 8, s_session, false);
    dw_venue(v)->link_begin(s_session, s_qr, sizeof s_qr);
    hal_qr_shown(s_qr);

    lv_obj_t *row = ui_rowbox(body, 10);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t *qbox = lv_obj_create(row);
    lv_obj_remove_style_all(qbox);
    lv_obj_set_size(qbox, 136, 136);
    lv_obj_set_style_bg_color(qbox, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(qbox, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(qbox, 6, 0);
    s_lk_qr = lv_qrcode_create(qbox);
    lv_qrcode_set_size(s_lk_qr, 124);
    lv_qrcode_set_dark_color(s_lk_qr, lv_color_hex(0x000000));
    lv_qrcode_set_light_color(s_lk_qr, lv_color_hex(0xffffff));
    lv_qrcode_update(s_lk_qr, s_qr, (uint32_t)strlen(s_qr));
    lv_obj_center(s_lk_qr);
    lv_obj_t *steps = ui_col(row, 6);
    lv_obj_set_flex_grow(steps, 1);
    lv_obj_set_width(steps, 0);
    static const char *STEPS[] = {"1  Scan with your phone", "2  Open in your wallet", "3  Approve the device key"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *l = ui_label(steps, &dw_sans_12, C_SOFT, STEPS[i]);
        lv_obj_set_width(l, LV_PCT(100));
        lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    }

    lv_obj_t *kv = ui_kv(body);
    const char *agent = g_model.acct[v].agent;
    ui_kv_add(kv, v == V_LIT ? "Device key" : "Agent", v == V_LIT ? "new API key" : fmt_short(agent, 6, 4), 0);
    ui_kv_add(kv, "Trade", "allowed", C_NEON);
    ui_kv_add(kv, "Withdraw", v == V_HL ? "impossible" : v == V_PAC ? "never signed here" : "only to your address", C_BAD);
    ui_kv_add(kv, "Revoke", "any time, main wallet", 0);
    s_lk_status = ui_label(body, &dw_mono_11, C_WARN, "Waiting for approval\xE2\x80\xA6");
    lv_obj_set_width(s_lk_status, LV_PCT(100));
    lv_label_set_long_mode(s_lk_status, LV_LABEL_LONG_WRAP);
    ui_eyebrow(body, "THE KEY WAS MADE ON THIS DEVICE AND NEVER LEAVES IT");
    s_lk_state = 0;
    s_lk_timer = lv_timer_create(lk_tick, 1500, NULL);
}

static void lk_leave(void)
{
    if (s_lk_timer) lv_timer_delete(s_lk_timer);
    s_lk_timer = NULL;
    s_lk_status = NULL;
    hal_qr_shown("");
    /* Abandoned before approval: drop the half-made key. */
    if (s_lk_state == 0 && !g_model.acct[g_link_venue].linked) dw_venue(g_link_venue)->unlink();
}

const dw_page_t PG_LINK = {.name = "link", .build = lk_build, .leave = lk_leave};

/* ======================= Revoke ======================= */

static dw_focus_t s_rv_f;
static int s_rv_v[V_COUNT];
static int s_rv_n;

static void rv_do(int i)
{
    if (i >= s_rv_n) return;
    int v = s_rv_v[i];
    dw_venue(v)->unlink();
    dw_light_ok();
    char m[120];
    snprintf(m, sizeof m, "%s key erased from this device. Your funds didn't move.", dw_venue_name(v));
    dw_toast(m, C_NEON);
    dw_nav_replace(&PG_REVOKE);
}

static void rv_build(lv_obj_t *body)
{
    dw_bar("ACCOUNTS", "\xC2\xB7 REVOKE", C_NEON);
    dw_hint("move", "back", "revoke", true);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    ui_title(body, "Revoke access", true);
    ui_sub(body, "YOUR FUNDS DON'T MOVE", 0);
    dw_focus_reset(&s_rv_f, NULL);
    lv_obj_t *list = ui_col(body, 0);
    s_rv_n = 0;
    for (int v = 0; v < V_COUNT; v++) {
        dw_acct_t *a = &g_model.acct[v];
        if (!a->linked) continue;
        char s[64];
        snprintf(s, sizeof s, "%s", fmt_short(a->agent, 6, 4));
        ui_row_t r = ui_row(list, dw_venue_name(v), s, "revoke", dw_venue_chain(v), false);
        lv_obj_set_style_text_color(r.val, lv_color_hex(C_BAD), 0);
        dw_focus_add(&s_rv_f, r.obj);
        s_rv_v[s_rv_n++] = v;
    }
    if (!s_rv_n) ui_label(list, &dw_sans_12, C_MUTE, "No linked venues.");
    lv_obj_t *w = ui_card(body, K_WARN);
    ui_label(w, &dw_mono_9, C_WARN, "LOST THE DEVICE?");
    lv_obj_t *t = ui_label(w, &dw_sans_12, C_INK,
                           "Revoke from your main wallet or the venue. Your funds stay on the venue either way.");
    lv_obj_set_width(t, LV_PCT(100));
    lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
    if (s_rv_n) ui_sign(body, "PRESS THE TRADE KEY TO REVOKE");
}

static bool rv_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_rv_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_rv_f, 1); return true;
    case DW_IN_TRADE: rv_do(s_rv_f.i); return true;
    default: return false;
    }
}

const dw_page_t PG_REVOKE = {.name = "revoke", .build = rv_build, .input = rv_input};
