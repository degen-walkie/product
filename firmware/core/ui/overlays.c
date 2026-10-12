/* Overlays above the page: the pull-down quick panel and the power-off sheet. */
#include <stdio.h>
#include <string.h>

#include "dw_config.h"
#include "dw_pages.h"

static lv_obj_t *s_qp, *s_off, *s_cf;
static void (*s_cf_cb)(void);
static dw_focus_t s_qp_f;
static lv_obj_t *s_off_slider;
static bool s_mute;
static int s_bright = 80;

bool dw_overlays_active(void) { return s_qp || s_off || s_cf; }

void dw_overlays_close(void)
{
    bool was = s_qp || s_off || s_cf;
    if (s_qp) lv_obj_delete(s_qp);
    if (s_off) lv_obj_delete(s_off);
    if (s_cf) lv_obj_delete(s_cf);
    s_qp = s_off = s_cf = NULL;
    if (was && dw_nav_top()) dw_nav_replace(dw_nav_top());   /* restores the page's hint bar */
}

static lv_obj_t *veil(void)
{
    lv_obj_t *v = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(v);
    lv_obj_set_size(v, SCR_W, SCR_H);
    lv_obj_set_style_bg_color(v, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(v, 170, 0);
    lv_obj_add_flag(v, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(v, LV_OBJ_FLAG_SCROLLABLE);
    return v;
}

static lv_obj_t *panel(lv_obj_t *parent, lv_align_t align)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_remove_style_all(p);
    lv_obj_set_size(p, SCR_W, LV_SIZE_CONTENT);
    lv_obj_align(p, align, 0, 0);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x12161b), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(p, 12, 0);
    lv_obj_set_style_pad_all(p, 14, 0);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(p, 10, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);
    return p;
}

/* ---------------- quick panel ---------------- */

static lv_obj_t *tile(lv_obj_t *grid, const char *title, const char *sub, bool on)
{
    lv_obj_t *t = ui_card(grid, on ? K_NEON : K_PLAIN);
    lv_obj_set_width(t, LV_PCT(48));
    ui_label(t, &dw_sansb_15, on ? C_NEON : C_INK, title);
    ui_label(t, &dw_mono_9, C_MUTE, sub);
    return t;
}

static void qp_build(void);

static void qp_open_item(int i)
{
    switch (i) {
    case 0: /* radio mode: screen off, rooms keep playing */
        dw_overlays_close();
        dw_app_input(DW_IN_POWER);
        return;
    case 1: s_mute = !s_mute; dw_toast(s_mute ? "Rooms muted." : "Rooms unmuted.", C_MUTE); break;
    case 2: s_bright = s_bright >= 100 ? 30 : s_bright + 35; if (s_bright > 100) s_bright = 100; break;
    case 3: dw_overlays_close(); dw_nav_root(&PG_DEVICE); return;
    case 4: dw_overlays_close(); dw_poweroff_open(); return;
    default: break;
    }
    int keep = s_qp_f.i;
    lv_obj_delete(s_qp);
    s_qp = NULL;
    qp_build();
    dw_focus_set(&s_qp_f, keep);
}

static void qp_veil_click(lv_event_t *e)
{
    if (lv_event_get_target(e) == s_qp) dw_overlays_close();
}

static void qp_gesture(lv_event_t *e)
{
    (void)e;
    if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_TOP) dw_overlays_close();
}

static void qp_build(void)
{
    s_qp = veil();
    lv_obj_add_event_cb(s_qp, qp_veil_click, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_qp, qp_gesture, LV_EVENT_GESTURE, NULL);
    lv_obj_t *p = panel(s_qp, LV_ALIGN_TOP_MID);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_t *top = ui_rowbox(p, 8);
    hal_power_t pw;
    hal_power(&pw);
    char b[64], ssid[40];
    hal_wifi_ssid(ssid, sizeof ssid);
    snprintf(b, sizeof b, "%s \xC2\xB7 %d%%%s", *ssid ? ssid : "Offline", pw.percent, pw.charging ? " \xC2\xB7 CHARGING" : "");
    ui_label(top, &dw_mono_11, C_SOFT, b);
    dw_net_t n = dw_net();
    lv_obj_t *badge = ui_badge(top, dw_net_name(), n == DW_NET_MAINNET ? K_BAD : n == DW_NET_TESTNET ? K_WARN : K_PLAIN);
    (void)badge;
    lv_obj_t *grid = ui_rowbox(p, 8);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(grid, 8, 0);
    dw_focus_reset(&s_qp_f, qp_open_item);
    dw_focus_add(&s_qp_f, tile(grid, "Radio mode", "SCREEN OFF, ROOM ON", false));
    dw_focus_add(&s_qp_f, tile(grid, s_mute ? "Muted" : "Sound on", "ROOM AUDIO", !s_mute));
    snprintf(b, sizeof b, "BRIGHTNESS %d%%", s_bright);
    dw_focus_add(&s_qp_f, tile(grid, "Brightness", b, false));
    dw_focus_add(&s_qp_f, tile(grid, "Device", "BATTERY \xC2\xB7 LIGHT", false));
    lv_obj_t *off = ui_btn(p, "POWER OFF\xE2\x80\xA6", K_BAD);
    dw_focus_add(&s_qp_f, off);
    lv_obj_t *handle = lv_obj_create(p);
    lv_obj_remove_style_all(handle);
    lv_obj_set_size(handle, 40, 4);
    lv_obj_set_style_radius(handle, 2, 0);
    lv_obj_set_style_bg_color(handle, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_bg_opa(handle, LV_OPA_COVER, 0);
    lv_obj_set_style_align(handle, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_left(handle, 126, 0);
    dw_hint("move", "close", "toggle", false);
}

void dw_qp_open(void)
{
    if (dw_overlays_active()) return;
    qp_build();
    dw_focus_set(&s_qp_f, 0);
}

/* ---------------- power-off sheet ---------------- */

static void do_off(lv_timer_t *t)
{
    (void)t;
    if (s_off) lv_obj_delete(s_off);
    s_off = NULL;
    dw_app_power_off();
}

static void off_confirm(void)
{
    lv_obj_clean(s_off);
    lv_obj_t *l = ui_label(s_off, &dw_disp_20, C_INK, "POWERING OFF\xE2\x80\xA6");
    lv_obj_center(l);
    lv_timer_t *t = lv_timer_create(do_off, 900, NULL);
    lv_timer_set_repeat_count(t, 1);
}

static void off_slide(lv_event_t *e)
{
    lv_obj_t *s = lv_event_get_target(e);
    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        if (lv_slider_get_value(s) > 90) off_confirm();
        else lv_slider_set_value(s, 0, LV_ANIM_ON);
    }
}

void dw_poweroff_open(void)
{
    if (s_off) return;
    if (s_qp) { lv_obj_delete(s_qp); s_qp = NULL; }
    s_off = veil();
    lv_obj_t *p = panel(s_off, LV_ALIGN_CENTER);
    lv_obj_set_width(p, SCR_W - 32);
    ui_title(p, "Power off?", true);
    lv_obj_t *t = ui_label(p, &dw_sans_12, C_MUTE, "Your keys stay encrypted on the device. Rooms stop until you power on.");
    lv_obj_set_width(t, LV_PCT(100));
    lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
    s_off_slider = lv_slider_create(p);
    lv_obj_set_size(s_off_slider, LV_PCT(100), 40);
    lv_slider_set_range(s_off_slider, 0, 100);
    lv_obj_set_style_bg_color(s_off_slider, lv_color_hex(0x1f242b), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_off_slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s_off_slider, 20, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_off_slider, lv_color_hex(C_BAD), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_off_slider, 20, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_off_slider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_off_slider, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(s_off_slider, off_slide, LV_EVENT_RELEASED, NULL);
    lv_obj_t *l = ui_label(p, &dw_mono_9, C_DIM, "SLIDE TO POWER OFF \xC2\xB7 OR PRESS THE TRADE KEY");
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(l, LV_PCT(100));
    dw_hint(NULL, "cancel", "power off", false);
}

/* ---------------- confirm ---------------- */

void dw_confirm_open(const char *title, const char *body, const char *sign, void (*on_yes)(void))
{
    if (dw_overlays_active()) return;
    s_cf = veil();
    s_cf_cb = on_yes;
    lv_obj_t *p = panel(s_cf, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_style_border_side(p, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(p, 2, 0);
    lv_obj_set_style_border_color(p, lv_color_hex(C_NEON), 0);
    lv_obj_align(p, LV_ALIGN_BOTTOM_MID, 0, -HINT_H);
    ui_title(p, title, true);
    lv_obj_t *t = ui_label(p, &dw_sans_14, C_SOFT, body);
    lv_obj_set_width(t, LV_PCT(100));
    lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
    ui_sign(p, sign);
    dw_hint(NULL, "cancel", "confirm", true);
}

/* ---------------- input ---------------- */

bool dw_overlays_input(dw_input_t in)
{
    if (s_cf) {
        if (in == DW_IN_TRADE) {
            void (*cb)(void) = s_cf_cb;
            dw_overlays_close();
            if (cb) cb();
        } else if (in == DW_IN_WHEEL_PRESS) {
            dw_overlays_close();
        }
        return true;
    }
    if (s_off) {
        if (in == DW_IN_TRADE) off_confirm();
        else if (in == DW_IN_WHEEL_PRESS) dw_overlays_close();
        return true;
    }
    if (s_qp) {
        switch (in) {
        case DW_IN_WHEEL_UP: dw_focus_move(&s_qp_f, -1); break;
        case DW_IN_WHEEL_DOWN: dw_focus_move(&s_qp_f, 1); break;
        case DW_IN_TRADE: qp_open_item(s_qp_f.i); break;
        case DW_IN_WHEEL_PRESS: dw_overlays_close(); break;
        default: break;
        }
        return true;
    }
    return false;
}
