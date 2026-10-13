/* App entry: power states, input routing and the voice path.
 *
 * Power: OFF (black, nothing runs on screen) → hold the power key → boot animation →
 * first-boot setup (once) → rooms. Short press toggles the screen ("radio mode":
 * rooms keep playing). Hold while on opens the slide-to-power-off sheet. */
#include "dw_app.h"

#include <string.h>

#include "dw_config.h"
#include "dw_hal.h"
#include "dw_model.h"
#include "dw_pages.h"
#include "dw_venue.h"
#include "relay.h"

static bool s_powered, s_started, s_screen_off;
static lv_obj_t *s_off_layer;
static dw_voice_sink s_sink;
static bool s_listening, s_held;

static void poll_tick(lv_timer_t *t)
{
    (void)t;
    if (!s_powered || !hal_net_up()) return;
    for (int v = 0; v < V_COUNT; v++)
        if (dw_venue_enabled(v)) dw_venue(v)->poll();
    dw_relay_poll();
}

static void black(bool on)
{
    if (on && !s_off_layer) {
        s_off_layer = lv_obj_create(lv_layer_sys());
        lv_obj_remove_style_all(s_off_layer);
        lv_obj_set_size(s_off_layer, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(s_off_layer, lv_color_hex(0x020304), 0);
        lv_obj_set_style_bg_opa(s_off_layer, LV_OPA_COVER, 0);
        lv_obj_add_flag(s_off_layer, LV_OBJ_FLAG_CLICKABLE);
    } else if (!on && s_off_layer) {
        lv_obj_delete(s_off_layer);
        s_off_layer = NULL;
    }
    hal_backlight(!on);
}

static void boot(void)
{
    s_powered = true;
    s_screen_off = false;
    black(false);
    if (!s_started) {
        s_started = true;
        for (int v = 0; v < V_COUNT; v++)
            if (dw_venue_enabled(v)) dw_venue(v)->start();
        dw_relay_start();
        lv_timer_create(poll_tick, 2500, NULL);
    }
    dw_light_idle();
    dw_nav_root(&PG_BOOT);
}

void dw_app_power_off(void)
{
    s_powered = false;
    s_screen_off = false;
    dw_overlays_close();
    hal_led(DW_LED_OFF, DW_LED_SOLID);
    black(true);
}

bool dw_app_setup_done(void)
{
    char b[4];
    return hal_store_get("setup.done", b, sizeof b) > 0;
}

void dw_app_init(void)
{
    memset(&g_model, 0, sizeof g_model);
    dw_config_load();
    dw_ui_init();
    dw_ui_start_timers();
    char b[4];
    if (hal_store_get("boot.off", b, sizeof b) > 0) {
        /* The simulator asks for a powered-off start to show the full boot. */
        hal_store_set("boot.off", NULL);
        dw_app_power_off();
    } else {
        boot();
    }
}

void dw_voice_sink_set(dw_voice_sink s) { s_sink = s; }
bool dw_voice_listening(void) { return s_listening; }
bool dw_voice_held(void) { return s_held; }

void dw_app_voice_result(const char *text)
{
    s_listening = false;
    if (s_sink) s_sink(text);
}

static void screen(bool off)
{
    s_screen_off = off;
    black(off);
}

/* Top-level pages in swipe order. */
static const dw_page_t *const TOP[] = {&PG_ROOMS, &PG_POSITIONS, &PG_MARKETS, &PG_NEWS, &PG_ASK, &PG_ACCOUNTS};
#define NTOP (int)(sizeof TOP / sizeof TOP[0])

void dw_app_gesture(lv_dir_t dir)
{
    if (!s_powered || s_screen_off || dw_sheet_active() || dw_overlays_active()) return;
    const dw_page_t *p = dw_nav_top();
    if (!p || p == &PG_BOOT || dw_page_is_setup(p)) return;
    int i = -1;
    for (int k = 0; k < NTOP; k++) if (TOP[k] == p) i = k;
    if (dir == LV_DIR_LEFT && i >= 0) dw_nav_root(TOP[(i + 1) % NTOP]);
    else if (dir == LV_DIR_RIGHT && i >= 0) dw_nav_root(TOP[(i + NTOP - 1) % NTOP]);
    else if (dir == LV_DIR_RIGHT && i < 0) dw_nav_pop();
    else if (dir == LV_DIR_BOTTOM) dw_qp_open();
}

void dw_app_input(dw_input_t in)
{
    if (!s_powered) {
        if (in == DW_IN_POWER_HOLD) boot();
        return;
    }
    if (in == DW_IN_POWER_HOLD) {
        if (s_screen_off) screen(false);
        dw_poweroff_open();
        return;
    }
    if (in == DW_IN_POWER) {
        if (dw_overlays_active()) dw_overlays_close();
        screen(!s_screen_off);
        return;
    }
    if (s_screen_off) {
        if (in == DW_IN_WHEEL_PRESS || in == DW_IN_TRADE) screen(false);
        return;
    }
    if (dw_overlays_active()) {
        dw_overlays_input(in);
        return;
    }
    if (dw_sheet_active()) {
        dw_sheet_input(in);
        return;
    }
    if (in == DW_IN_PTT_DOWN || in == DW_IN_PTT_UP) {
        const dw_page_t *p = dw_nav_top();
        if (p == &PG_BOOT || dw_page_is_setup(p)) return;
        bool handled = p && p->input && p->input(in);
        if (!handled && in == DW_IN_PTT_DOWN) {
            /* Anywhere else, the mic is the voice shortcut. */
            dw_nav_push(&PG_VOICE);
            handled = PG_VOICE.input(in);
        }
        if (in == DW_IN_PTT_DOWN && handled) { s_listening = s_held = true; hal_voice_start(); }
        if (in == DW_IN_PTT_UP && s_held) { s_held = false; hal_voice_stop(); }
        return;
    }
    const dw_page_t *p = dw_nav_top();
    if (p && p->input && p->input(in)) return;
    if (in == DW_IN_WHEEL_PRESS) {
        if (p && p->top) dw_nav_push(&PG_MENU);
        else dw_nav_pop();
    }
}
