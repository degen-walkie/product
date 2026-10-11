/* App entry: starts the venue clients, owns input routing and the voice path. */
#include "dw_app.h"

#include <string.h>

#include "dw_hal.h"
#include "dw_model.h"
#include "dw_pages.h"
#include "relay.h"
#include "dw_venue.h"

static bool s_screen_off;
static lv_obj_t *s_off_layer;
static dw_voice_sink s_sink;
static bool s_listening;

static void poll_tick(lv_timer_t *t)
{
    (void)t;
    for (int v = 0; v < V_COUNT; v++) dw_venue(v)->poll();
    dw_relay_poll();
}

void dw_app_init(void)
{
    memset(&g_model, 0, sizeof g_model);
    dw_ui_init();
    dw_ui_start_timers();
    for (int v = 0; v < V_COUNT; v++) dw_venue(v)->start();
    dw_relay_start();
    lv_timer_create(poll_tick, 2500, NULL);
    dw_light_idle();
    dw_nav_root(&PG_BOOT);
}

void dw_voice_sink_set(dw_voice_sink s) { s_sink = s; }
bool dw_voice_listening(void) { return s_listening; }

void dw_app_voice_result(const char *text)
{
    s_listening = false;
    if (s_sink) s_sink(text);
}

static void screen_power(bool off)
{
    s_screen_off = off;
    hal_backlight(!off);
    if (off && !s_off_layer) {
        /* "Radio mode": screen off, rooms keep playing. */
        s_off_layer = lv_obj_create(lv_layer_sys());
        lv_obj_remove_style_all(s_off_layer);
        lv_obj_set_size(s_off_layer, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(s_off_layer, lv_color_hex(0x020304), 0);
        lv_obj_set_style_bg_opa(s_off_layer, LV_OPA_COVER, 0);
        lv_obj_add_flag(s_off_layer, LV_OBJ_FLAG_CLICKABLE);
    } else if (!off && s_off_layer) {
        lv_obj_delete(s_off_layer);
        s_off_layer = NULL;
    }
}

void dw_app_input(dw_input_t in)
{
    if (in == DW_IN_POWER) { screen_power(!s_screen_off); return; }
    if (s_screen_off) {
        if (in == DW_IN_WHEEL_PRESS || in == DW_IN_TRADE) screen_power(false);
        return;
    }
    if (dw_sheet_active()) {
        dw_sheet_input(in);
        return;
    }
    if (in == DW_IN_PTT_DOWN || in == DW_IN_PTT_UP) {
        const dw_page_t *p = dw_nav_top();
        bool handled = p && p->input && p->input(in);
        if (!handled && in == DW_IN_PTT_DOWN && p != &PG_BOOT) {
            /* Anywhere else, the mic is the voice shortcut. */
            dw_nav_push(&PG_VOICE);
            handled = PG_VOICE.input(in);
        }
        if (in == DW_IN_PTT_DOWN && handled) { s_listening = true; hal_voice_start(); }
        if (in == DW_IN_PTT_UP && s_listening) hal_voice_stop();
        return;
    }
    const dw_page_t *p = dw_nav_top();
    if (p && p->input && p->input(in)) return;
    if (in == DW_IN_WHEEL_PRESS) {
        if (p && p->top) dw_nav_push(&PG_MENU);
        else dw_nav_pop();
    }
}
