/* First-boot setup: language → Wi-Fi → device PIN → link a venue → done.
 * Runs once (until "setup.done" is cleared by a factory reset). */
#include <stdio.h>
#include <string.h>

#include "dw_config.h"
#include "dw_crypto.h"
#include "dw_pages.h"
#include "dw_venue.h"

bool dw_page_is_setup(const dw_page_t *p)
{
    return p == &PG_SETUP_LANG || p == &PG_SETUP_WIFI || p == &PG_SETUP_WIFI_PASS || p == &PG_SETUP_PIN || p == &PG_SETUP_LINK || p == &PG_SETUP_DONE || p == &PG_UNLOCK;
}

static lv_obj_t *step(lv_obj_t *body, int n, const char *title, const char *sub)
{
    char s[24];
    snprintf(s, sizeof s, "SETUP \xC2\xB7 %d OF 4", n);
    dw_bar(s, "", C_NEON);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *dots = ui_rowbox(body, 6);
    lv_obj_set_flex_align(dots, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    for (int i = 1; i <= 4; i++) {
        lv_obj_t *d = lv_obj_create(dots);
        lv_obj_remove_style_all(d);
        lv_obj_set_size(d, i == n ? 22 : 8, 4);
        lv_obj_set_style_radius(d, 2, 0);
        lv_obj_set_style_bg_color(d, lv_color_hex(i <= n ? C_NEON : C_BORDER), 0);
        lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
    }
    ui_title(body, title, false);
    lv_obj_t *l = ui_label(body, &dw_sans_14, C_MUTE, sub);
    lv_obj_set_width(l, LV_PCT(100));
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    return l;
}

/* ======================= 1. Language ======================= */

static const struct { const char *name, *code, *note; } LANGS[] = {
    {"English", "EN", "Rooms heard in English"},       {"Chinese", "ZH", "Rooms heard in Chinese"},
    {"Japanese", "JA", "Rooms heard in Japanese"},     {"Korean", "KO", "Rooms heard in Korean"},
    {"Spanish", "ES", "Rooms heard in Spanish"},       {"Portuguese", "PT", "Rooms heard in Portuguese"},
    {"Turkish", "TR", "Rooms heard in Turkish"},       {"Vietnamese", "VI", "Rooms heard in Vietnamese"},
};
#define NLANG (int)(sizeof LANGS / sizeof LANGS[0])
static dw_focus_t s_lang_f;

static void lang_pick(int i)
{
    hal_store_set("lang", LANGS[i].code);
    dw_nav_replace(&PG_SETUP_WIFI);
}

static void lang_build(lv_obj_t *body)
{
    step(body, 1, "Hello.", "Pick the language you want to hear the rooms in. Everyone else hears you in theirs. The device itself is in English for now.");
    dw_hint("move", NULL, "select", false);
    dw_focus_reset(&s_lang_f, lang_pick);
    lv_obj_t *list = ui_col(body, 0);
    for (int i = 0; i < NLANG; i++) dw_focus_add(&s_lang_f, ui_row(list, LANGS[i].name, LANGS[i].note, LANGS[i].code, NULL, false).obj);
    dw_focus_set(&s_lang_f, 0);
}

static bool lang_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_lang_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_lang_f, 1); return true;
    case DW_IN_TRADE: lang_pick(s_lang_f.i); return true;
    default: return true;   /* nothing to go back to */
    }
}

const dw_page_t PG_SETUP_LANG = {.name = "setup-lang", .build = lang_build, .input = lang_input};

/* ======================= 2. Wi-Fi ======================= */
/* Scan, pick a network (wheel or tap), type the password on the on-screen keyboard. */

static lv_obj_t *s_wifi_state, *s_wifi_list, *s_wifi_btn;
static lv_timer_t *s_wifi_t;
static dw_focus_t s_wifi_f;
static hal_wifi_ap_t s_aps[12];
static int s_nap;           /* -1 while scanning */
static char s_pick[33];     /* network waiting for its password */

static void wifi_pick(int i);

static void wifi_list_build(void)
{
    lv_obj_clean(s_wifi_list);
    dw_focus_reset(&s_wifi_f, wifi_pick);
    char ssid[33], sig[8];
    hal_wifi_ssid(ssid, sizeof ssid);
    for (int i = 0; i < s_nap; i++) {
        int bars = s_aps[i].rssi > -55 ? 4 : s_aps[i].rssi > -65 ? 3 : s_aps[i].rssi > -75 ? 2 : 1;
        snprintf(sig, sizeof sig, "%.*s", bars, "||||");
        const char *sub = !strcmp(s_aps[i].ssid, ssid) ? "Connected" : s_aps[i].secure ? "Password" : "Open";
        dw_focus_add(&s_wifi_f, ui_row(s_wifi_list, s_aps[i].ssid, sub, sig, NULL, false).obj);
    }
    lv_obj_t *again = ui_btn(s_wifi_list, "SCAN AGAIN", K_PLAIN);
    dw_focus_add(&s_wifi_f, again);
    dw_focus_set(&s_wifi_f, 0);
}

static void wifi_scan(void)
{
    s_nap = -1;
    lv_obj_clean(s_wifi_list);
    hal_wifi_scan_start();
}

static void wifi_pick(int i)
{
    if (i >= s_nap) { wifi_scan(); return; }   /* "SCAN AGAIN" */
    if (!s_aps[i].secure) { hal_wifi_join(s_aps[i].ssid, ""); return; }
    snprintf(s_pick, sizeof s_pick, "%s", s_aps[i].ssid);
    dw_nav_push(&PG_SETUP_WIFI_PASS);
}

static void wifi_update(void)
{
    if (s_nap < 0 && (s_nap = hal_wifi_scan_results(s_aps, 12)) >= 0) wifi_list_build();
    char target[33], t[80];
    hal_wifi_target(target, sizeof target);
    hal_wifi_state_t st = hal_wifi_state();
    uint32_t col = C_WARN;
    if (st == HAL_WIFI_UP) { snprintf(t, sizeof t, "Connected to %s", target); col = C_NEON; }
    else if (st == HAL_WIFI_CONNECTING) snprintf(t, sizeof t, "Joining %s\xE2\x80\xA6", target);
    else if (st == HAL_WIFI_FAILED) { snprintf(t, sizeof t, "Couldn't join %s. Check the password.", target); col = C_BAD; }
    else if (s_nap < 0) snprintf(t, sizeof t, "Looking for Wi-Fi\xE2\x80\xA6");
    else if (s_nap == 0) snprintf(t, sizeof t, "No networks found.");
    else snprintf(t, sizeof t, "Pick your network.");
    lv_label_set_text(s_wifi_state, t);
    lv_obj_set_style_text_color(s_wifi_state, lv_color_hex(col), 0);
    dw_hint(s_nap > 0 ? "move" : NULL, "back", st == HAL_WIFI_UP ? "continue" : s_nap > 0 ? "join" : "skip", false);
}

static void wifi_tick(lv_timer_t *t)
{
    (void)t;
    wifi_update();
}

static void wifi_continue(lv_event_t *e)
{
    (void)e;
    dw_nav_replace(&PG_SETUP_PIN);
}

static void wifi_build(lv_obj_t *body)
{
    step(body, 2, "Get online.", "The device talks to the venues and the rooms over Wi-Fi. A 4G back-clip works too.");
    lv_obj_t *c = ui_card(body, K_PLAIN);
    ui_label(c, &dw_mono_9, C_DIM, "NETWORK");
    s_wifi_state = ui_label(c, &dw_sansb_15, C_NEON, "");
    lv_obj_set_width(s_wifi_state, LV_PCT(100));
    lv_label_set_long_mode(s_wifi_state, LV_LABEL_LONG_WRAP);
    s_wifi_list = ui_col(body, 6);
    s_wifi_btn = ui_btn(body, "CONTINUE", K_TRADE);
    lv_obj_add_event_cb(s_wifi_btn, wifi_continue, LV_EVENT_CLICKED, NULL);
    dw_focus_reset(&s_wifi_f, wifi_pick);
    if (s_nap > 0 && dw_nav_top() == &PG_SETUP_WIFI) wifi_list_build();   /* back from the password page */
    else wifi_scan();
    s_wifi_t = lv_timer_create(wifi_tick, 250, NULL);
    wifi_update();
}

static void wifi_leave(void)
{
    if (s_wifi_t) lv_timer_delete(s_wifi_t);
    s_wifi_t = NULL;
}

static bool wifi_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_wifi_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_wifi_f, 1); return true;
    case DW_IN_TRADE:
        if (hal_wifi_state() != HAL_WIFI_UP && s_nap > 0) wifi_pick(s_wifi_f.i);
        else dw_nav_replace(&PG_SETUP_PIN);
        return true;
    case DW_IN_WHEEL_PRESS: dw_nav_replace(&PG_SETUP_LANG); return true;
    default: return true;
    }
}

const dw_page_t PG_SETUP_WIFI = {.name = "setup-wifi", .build = wifi_build, .update = wifi_update, .input = wifi_input, .leave = wifi_leave};

/* Password: a text field and LVGL's on-screen keyboard. */

static lv_obj_t *s_pw_ta;

static void pw_close(void *join)
{
    static char pw[65];
    if (join) {
        snprintf(pw, sizeof pw, "%s", lv_textarea_get_text(s_pw_ta));
        hal_wifi_join(s_pick, pw);
    }
    dw_nav_pop();
}

/* The keyboard still sends READY/CANCEL on to the text field after this returns,
   so the page (and the field) must outlive this call: close it on the next tick. */
static void pw_done(lv_event_t *e)
{
    lv_async_call(pw_close, (void *)(intptr_t)(lv_event_get_code(e) == LV_EVENT_READY));
}

static void pw_build(lv_obj_t *body)
{
    dw_bar("WI-FI", "", C_NEON);
    dw_hint(NULL, "cancel", NULL, false);
    ui_title(body, s_pick, true);
    s_pw_ta = lv_textarea_create(body);
    lv_textarea_set_one_line(s_pw_ta, true);
    lv_textarea_set_password_mode(s_pw_ta, false);   /* typed on the device itself: show it so typos are visible */
    lv_textarea_set_placeholder_text(s_pw_ta, "Password");
    lv_obj_set_width(s_pw_ta, LV_PCT(100));
    lv_obj_t *kb = lv_keyboard_create(lv_layer_top());
    lv_obj_set_size(kb, LV_PCT(100), 210);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, s_pw_ta);
    lv_obj_add_event_cb(kb, pw_done, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, pw_done, LV_EVENT_CANCEL, NULL);
    lv_obj_set_user_data(s_pw_ta, kb);
}

static void pw_leave(void)
{
    lv_obj_t *kb = lv_obj_get_user_data(s_pw_ta);
    if (kb) lv_obj_delete_async(kb);
}

static bool pw_input(dw_input_t in)
{
    if (in == DW_IN_WHEEL_PRESS) dw_nav_pop();
    return true;
}

const dw_page_t PG_SETUP_WIFI_PASS = {.name = "setup-wifi-pass", .build = pw_build, .input = pw_input, .leave = pw_leave};

/* ======================= 3. PIN ======================= */

static char s_pin[7];
static int s_pin_n, s_digit;
static bool s_pin_check;   /* false: set the PIN (setup); true: unlock at power-on */
static lv_obj_t *s_pin_boxes[6], *s_pin_note;

static void pin_render(void)
{
    for (int i = 0; i < 6; i++) {
        lv_obj_t *l = lv_obj_get_child(s_pin_boxes[i], 0);
        char t[4] = "";
        if (i < s_pin_n) strcpy(t, "\xE2\x80\xA2");
        else if (i == s_pin_n) snprintf(t, sizeof t, "%d", s_digit);
        lv_label_set_text(l, t);
        dw_style_focus(s_pin_boxes[i], i == s_pin_n);
    }
}

static void pin_push(int d)
{
    if (s_pin_n >= 6) return;
    s_pin[s_pin_n++] = (char)('0' + d);
    s_pin[s_pin_n] = 0;
    s_digit = 0;
    if (s_pin_n == 6) {
        uint8_t h[32];
        dw_keccak256((const uint8_t *)s_pin, 6, h);
        char hex[65], want[80];
        dw_hex(h, 32, hex, false);
        memset(s_pin, 0, sizeof s_pin);
        if (!s_pin_check) {
            hal_store_set("pin", hex);   /* hashed; NVS is encrypted on production builds */
            dw_nav_replace(&PG_SETUP_LINK);
        } else if (hal_store_get("pin", want, sizeof want) > 0 && !strcmp(want, hex)) {
            dw_light_ok();
            dw_nav_root(&PG_ROOMS);
        } else {
            dw_light_fail();
            hal_vibrate(120);
            dw_toast("Wrong PIN.", C_BAD);
            s_pin_n = 0;
            pin_render();
        }
        return;
    }
    pin_render();
}

static void key_click(lv_event_t *e)
{
    lv_obj_t *k = lv_event_get_current_target(e);
    int v = (int)(intptr_t)lv_obj_get_user_data(k);
    if (v == 10) { if (s_pin_n) s_pin_n--; s_digit = 0; pin_render(); }
    else pin_push(v);
}

static void pin_body(lv_obj_t *body)
{
    lv_obj_set_style_pad_row(body, 8, 0);
    s_pin_n = 0;
    s_digit = 0;
    lv_obj_t *row = ui_rowbox(body, 6);
    for (int i = 0; i < 6; i++) {
        lv_obj_t *b = ui_card(row, K_PLAIN);
        lv_obj_set_size(b, 40, 46);
        lv_obj_set_style_pad_all(b, 0, 0);
        lv_obj_t *l = ui_label(b, &dw_mono_24, C_INK, "");
        lv_obj_center(l);
        lv_obj_set_style_align(l, LV_ALIGN_CENTER, 0);
        s_pin_boxes[i] = b;
        dw_style_focusable(b);
    }
    lv_obj_t *pad = ui_rowbox(body, 6);
    lv_obj_set_flex_flow(pad, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(pad, 6, 0);
    static const int KEYS[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, -1};
    for (int i = 0; i < 12; i++) {
        if (KEYS[i] < 0) continue;
        char t[8];
        snprintf(t, sizeof t, KEYS[i] == 10 ? "\xE2\x86\x90" : "%d", KEYS[i]);
        lv_obj_t *k = ui_btn(pad, t, K_PLAIN);
        lv_obj_set_size(k, LV_PCT(31), 38);
        lv_obj_set_user_data(k, (void *)(intptr_t)KEYS[i]);
        lv_obj_add_event_cb(k, key_click, LV_EVENT_CLICKED, NULL);
    }
    s_pin_note = ui_label(body, &dw_mono_9, C_DIM, "ROLL THE WHEEL \xC2\xB7 TRADE KEY FOR THE NEXT DIGIT \xC2\xB7 OR TAP");
    pin_render();
}

static void pin_build(lv_obj_t *body)
{
    s_pin_check = false;
    step(body, 3, "Lock it.", "Six digits. You enter it every time the device powers on.");
    dw_hint("digit", "back", "next", false);
    pin_body(body);
}

static void unlock_build(lv_obj_t *body)
{
    s_pin_check = true;
    dw_bar("LOCKED", "", C_NEON);
    lv_obj_add_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);
    ui_title(body, "Enter PIN", false);
    dw_hint("digit", "delete", "next", false);
    pin_body(body);
}

bool dw_pin_set(void)
{
    char b[80];
    return hal_store_get("pin", b, sizeof b) > 0;
}

static bool pin_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: s_digit = (s_digit + 1) % 10; pin_render(); return true;
    case DW_IN_WHEEL_DOWN: s_digit = (s_digit + 9) % 10; pin_render(); return true;
    case DW_IN_TRADE: pin_push(s_digit); return true;
    case DW_IN_WHEEL_PRESS:
        if (s_pin_n) { s_pin_n--; s_digit = 0; pin_render(); }
        else if (!s_pin_check) dw_nav_replace(&PG_SETUP_WIFI);
        return true;
    default: return true;
    }
}

const dw_page_t PG_SETUP_PIN = {.name = "setup-pin", .build = pin_build, .input = pin_input};
const dw_page_t PG_UNLOCK = {.name = "unlock", .build = unlock_build, .input = pin_input};

/* ======================= 4. Link ======================= */

static dw_focus_t s_ln_f;
static ui_row_t s_ln_rows[V_COUNT];

static void ln_open(int i)
{
    if (i < V_COUNT) {
        if (!dw_venue_enabled(i)) { dw_toast("Lighter is coming soon.", C_MUTE); return; }
        if (g_model.acct[i].linked) { dw_toast("Already linked with a trade-only key.", C_NEON); return; }
        dw_open_link(i);
        return;
    }
    dw_nav_replace(&PG_SETUP_DONE);
}

static void ln_update(void)
{
    for (int v = 0; v < V_COUNT; v++) {
        bool on = g_model.acct[v].linked;
        lv_label_set_text(s_ln_rows[v].val, !dw_venue_enabled(v) ? "soon" : on ? "linked" : "link");
        lv_obj_set_style_text_color(s_ln_rows[v].val, lv_color_hex(!dw_venue_enabled(v) ? C_DIM : on ? C_NEON : C_INK), 0);
    }
}

static void ln_build(lv_obj_t *body)
{
    step(body, 4, "Link a venue.", "Your main wallet approves a key that lives on this device. It can trade. It can't withdraw. Funds stay on the venue.");
    dw_hint("move", "back", "open", false);
    dw_focus_reset(&s_ln_f, ln_open);
    lv_obj_t *list = ui_col(body, 0);
    for (int v = 0; v < V_COUNT; v++) {
        s_ln_rows[v] = ui_row(list, dw_venue_name(v), dw_venue_chain(v), "", NULL, false);
        if (!dw_venue_enabled(v)) lv_obj_set_style_opa(s_ln_rows[v].obj, 120, 0);
        dw_focus_add(&s_ln_f, s_ln_rows[v].obj);
    }
    dw_focus_add(&s_ln_f, ui_btn(body, "CONTINUE", K_TRADE));
    dw_focus_set(&s_ln_f, 0);
    ln_update();
}

static bool ln_input(dw_input_t in)
{
    switch (in) {
    case DW_IN_WHEEL_UP: dw_focus_move(&s_ln_f, -1); return true;
    case DW_IN_WHEEL_DOWN: dw_focus_move(&s_ln_f, 1); return true;
    case DW_IN_TRADE: ln_open(s_ln_f.i); return true;
    case DW_IN_WHEEL_PRESS: dw_nav_replace(&PG_SETUP_PIN); return true;
    default: return true;
    }
}

const dw_page_t PG_SETUP_LINK = {.name = "setup-link", .build = ln_build, .update = ln_update, .input = ln_input};

/* ======================= Done ======================= */

static void done_build(lv_obj_t *body)
{
    dw_bar("READY", "", C_NEON);
    dw_hint(NULL, NULL, "start", false);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    ui_title(body, "You're set.", false);
    static const char *TIPS[] = {
        "ROLL the wheel to move. PRESS it to go back.",
        "The TRADE KEY opens things, and it is the only way to sign an order.",
        "HOLD THE MIC to talk in a room or ask the AI.",
        "SWIPE left and right between rooms, positions, markets and news. Pull down for quick settings.",
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *c = ui_card(body, K_PLAIN);
        lv_obj_t *l = ui_label(c, &dw_sans_14, C_SOFT, TIPS[i]);
        lv_obj_set_width(l, LV_PCT(100));
        lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    }
    ui_sign(body, "PRESS THE TRADE KEY TO START");
}

static bool done_input(dw_input_t in)
{
    if (in == DW_IN_TRADE) {
        hal_store_set("setup.done", "1");
        dw_light_ok();
        dw_nav_root(&PG_ROOMS);
    }
    return true;
}

const dw_page_t PG_SETUP_DONE = {.name = "setup-done", .build = done_build, .input = done_input};
