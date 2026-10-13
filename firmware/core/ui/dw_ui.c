#include "dw_ui.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dw_config.h"
#include "dw_model.h"
#include "dw_pages.h"

#define NAV_MAX 10

static lv_obj_t *s_bar_l, *s_bar_s, *s_bar_r, *s_bar_mode, *s_page, *s_hint[3][2], *s_hint_box[3];
static bool s_hint_tap[3];   /* which hint boxes act as their key when tapped */
static void hint_click(lv_event_t *e);
static const dw_page_t *s_stack[NAV_MAX];
static int s_depth;
static lv_style_t s_focus, s_chip_on, s_tab_on;
static lv_timer_t *s_light_timer;

static lv_color_t C(uint32_t hex) { return lv_color_hex(hex); }

/* ---------------- formatting ---------------- */

static char *fbuf(void)
{
    static char bufs[8][40];
    static int i;
    return bufs[i++ & 7];
}

const char *fmt_num(double v, int dec)
{
    char raw[40], *out = fbuf();
    snprintf(raw, sizeof raw, "%.*f", dec, fabs(v));
    char *dot = strchr(raw, '.');
    int ilen = dot ? (int)(dot - raw) : (int)strlen(raw);
    int o = 0;
    if (v < 0 && fabs(v) >= 0.5 * pow(10, -dec)) out[o++] = '-';
    for (int i = 0; i < ilen; i++) {
        out[o++] = raw[i];
        int left = ilen - i - 1;
        if (left > 0 && left % 3 == 0) out[o++] = ',';
    }
    if (dot) { strcpy(out + o, dot); } else out[o] = 0;
    return out;
}

const char *fmt_signed(double v, int dec)
{
    char *out = fbuf();
    snprintf(out, 40, "%s%s", v >= 0 ? "+" : "", fmt_num(v, dec));
    return out;
}

const char *fmt_px(double v)
{
    double a = fabs(v);
    return fmt_num(v, a >= 10000 ? 1 : a >= 100 ? 2 : a >= 1 ? 3 : 5);
}

const char *fmt_pct(double v)
{
    char *out = fbuf();
    snprintf(out, 40, "%s%.1f%%", v >= 0 ? "+" : "", v);
    return out;
}

const char *fmt_short(const char *s, int head, int tail)
{
    char *out = fbuf();
    int n = (int)strlen(s);
    if (n <= head + tail + 1) snprintf(out, 40, "%s", s);
    else snprintf(out, 40, "%.*s\xE2\x80\xA6%s", head, s, s + n - tail);
    return out;
}

/* ---------------- base layout ---------------- */

static lv_obj_t *box(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

lv_obj_t *ui_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *text)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, C(color), 0);
    lv_label_set_text(l, text ? text : "");
    return l;
}

static void bar_click(lv_event_t *e) { (void)e; dw_qp_open(); }
static void on_gesture(lv_event_t *e)
{
    (void)e;
    dw_app_gesture(lv_indev_get_gesture_dir(lv_indev_active()));
}

void dw_ui_init(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, C(C_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_style_init(&s_focus);
    lv_style_set_border_color(&s_focus, C(C_NEON));
    lv_style_set_border_width(&s_focus, 2);
    lv_style_set_border_opa(&s_focus, LV_OPA_COVER);
    lv_style_set_border_side(&s_focus, LV_BORDER_SIDE_FULL);
    lv_style_set_bg_color(&s_focus, C(C_NEON));
    lv_style_set_bg_opa(&s_focus, 16);
    lv_style_set_radius(&s_focus, 5);
    lv_style_set_shadow_color(&s_focus, C(C_NEON));
    lv_style_set_shadow_width(&s_focus, 14);
    lv_style_set_shadow_opa(&s_focus, 40);

    lv_style_init(&s_chip_on);
    lv_style_set_border_color(&s_chip_on, C(C_NEON));
    lv_style_set_text_color(&s_chip_on, C(C_NEON));
    lv_style_set_bg_color(&s_chip_on, C(C_NEON));
    lv_style_set_bg_opa(&s_chip_on, 22);

    lv_style_init(&s_tab_on);
    lv_style_set_text_color(&s_tab_on, C(C_INK));
    lv_style_set_border_color(&s_tab_on, C(C_NEON));
    lv_style_set_border_width(&s_tab_on, 2);
    lv_style_set_border_side(&s_tab_on, LV_BORDER_SIDE_BOTTOM);

    /* status bar */
    lv_obj_t *bar = box(scr);
    lv_obj_set_size(bar, SCR_W, BAR_H);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_color(bar, C(C_LINE), 0);
    lv_obj_set_style_pad_hor(bar, 14, 0);
    s_bar_l = ui_label(bar, &dw_monob_11, C_NEON, "");
    lv_obj_align(s_bar_l, LV_ALIGN_LEFT_MID, 0, 0);
    s_bar_s = ui_label(bar, &dw_mono_11, C_MUTE, "");
    lv_obj_align_to(s_bar_s, s_bar_l, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
    s_bar_r = ui_label(bar, &lv_font_montserrat_14, C_MUTE, "");
    lv_obj_align(s_bar_r, LV_ALIGN_RIGHT_MID, 0, 0);
    s_bar_mode = ui_label(bar, &dw_monob_9, C_WARN, "");
    /* Tap or pull down on the status bar: quick panel. Swipes anywhere: pages. */
    lv_obj_add_flag(bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(bar, bar_click, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(scr, on_gesture, LV_EVENT_GESTURE, NULL);

    /* page area */
    s_page = box(scr);
    lv_obj_set_pos(s_page, 0, BAR_H);
    lv_obj_set_size(s_page, SCR_W, SCR_H - BAR_H - HINT_H);

    /* hint bar */
    lv_obj_t *hint = box(scr);
    lv_obj_set_pos(hint, 0, SCR_H - HINT_H);
    lv_obj_set_size(hint, SCR_W, HINT_H);
    lv_obj_set_style_border_side(hint, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(hint, 1, 0);
    lv_obj_set_style_border_color(hint, C(C_LINE), 0);
    static const char *TOP[3] = {"ROLL", "PRESS", "TRADE KEY"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *b = box(hint);
        s_hint_box[i] = b;
        lv_obj_set_size(b, i == 2 ? SCR_W - 2 * 106 : 106, HINT_H - 1);
        lv_obj_set_pos(b, i * 106, 1);
        if (i < 2) {
            lv_obj_set_style_border_side(b, LV_BORDER_SIDE_RIGHT, 0);
            lv_obj_set_style_border_width(b, 1, 0);
            lv_obj_set_style_border_color(b, C(C_LINE), 0);
        }
        s_hint[i][0] = ui_label(b, &dw_mono_9, 0x5c6470, TOP[i]);
        lv_obj_align(s_hint[i][0], LV_ALIGN_TOP_MID, 0, 4);
        s_hint[i][1] = ui_label(b, &dw_monob_11, i == 2 ? C_NEON : 0xc5ccd6, "");
        lv_obj_align(s_hint[i][1], LV_ALIGN_BOTTOM_MID, 0, -3);
        lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
        if (i > 0) lv_obj_add_event_cb(b, hint_click, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

/* Tapping a hint box does what its key does, so a touch-only device can get through every page.
   Signing never: a hint marked `sign` (and "cancel all") still needs the physical TRADE key. */
static void hint_click(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (s_hint_tap[i]) dw_app_input(i == 1 ? DW_IN_WHEEL_PRESS : DW_IN_TRADE);
}

void dw_bar(const char *label, const char *sub, uint32_t color)
{
    lv_label_set_text(s_bar_l, label ? label : "");
    lv_obj_set_style_text_color(s_bar_l, C(color ? color : C_NEON), 0);
    lv_label_set_text(s_bar_s, sub ? sub : "");
    lv_obj_align_to(s_bar_s, s_bar_l, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
}

static void bar_power_update(lv_timer_t *t)
{
    (void)t;
    hal_power_t p;
    hal_power(&p);
    const char *bat = p.percent > 80 ? LV_SYMBOL_BATTERY_FULL : p.percent > 55 ? LV_SYMBOL_BATTERY_3
                    : p.percent > 30 ? LV_SYMBOL_BATTERY_2 : p.percent > 10 ? LV_SYMBOL_BATTERY_1 : LV_SYMBOL_BATTERY_EMPTY;
    static char buf[48];
    snprintf(buf, sizeof buf, "%s  %s %d", p.charging ? LV_SYMBOL_CHARGE : (hal_net_up() ? LV_SYMBOL_WIFI : ""), bat, p.percent);
    lv_label_set_text(s_bar_r, buf);
    lv_obj_set_style_text_color(s_bar_r, C(p.percent <= 10 && !p.charging ? C_BAD : C_MUTE), 0);
    dw_net_t n = dw_net();
    lv_label_set_text(s_bar_mode, n == DW_NET_DEMO ? "" : dw_net_name());
    lv_obj_set_style_text_color(s_bar_mode, C(n == DW_NET_MAINNET ? C_BAD : C_WARN), 0);
    lv_obj_align_to(s_bar_mode, s_bar_r, LV_ALIGN_OUT_LEFT_MID, -8, 0);
}

void dw_hint(const char *roll, const char *press, const char *trade, bool sign)
{
    const char *v[3] = {roll, press, trade};
    for (int i = 0; i < 3; i++) {
        lv_label_set_text(s_hint[i][1], v[i] ? v[i] : "\xE2\x80\x94");
        lv_obj_set_style_opa(s_hint_box[i], v[i] ? LV_OPA_COVER : 80, 0);
    }
    lv_obj_set_style_bg_color(s_hint_box[2], C(C_NEON), 0);
    lv_obj_set_style_bg_opa(s_hint_box[2], sign ? 40 : 0, 0);
    s_hint_tap[1] = press && *press;
    s_hint_tap[2] = trade && *trade && !sign && strcmp(trade, "cancel all");
}

/* ---------------- navigation ---------------- */

static void show(const dw_page_t *p)
{
    lv_obj_clean(s_page);
    lv_obj_t *body = box(s_page);
    lv_obj_set_size(body, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_hor(body, 14, 0);
    lv_obj_set_style_pad_ver(body, 12, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 10, 0);
    dw_hint(NULL, "back", NULL, false);
    p->build(body);
    if (p->update) p->update();
}

lv_obj_t *dw_body(void) { return lv_obj_get_child(s_page, 0); }
const dw_page_t *dw_nav_top(void) { return s_depth ? s_stack[s_depth - 1] : NULL; }

static void leave_top(void)
{
    const dw_page_t *t = dw_nav_top();
    if (t && t->leave) t->leave();
}

void dw_nav_push(const dw_page_t *p)
{
    leave_top();
    if (s_depth == NAV_MAX) {
        memmove(s_stack, s_stack + 1, sizeof(s_stack[0]) * (NAV_MAX - 1));
        s_depth--;
    }
    s_stack[s_depth++] = p;
    show(p);
}

void dw_nav_replace(const dw_page_t *p)
{
    leave_top();
    if (s_depth) s_depth--;
    s_stack[s_depth++] = p;
    show(p);
}

void dw_nav_pop(void)
{
    if (s_depth <= 1) return;
    leave_top();
    s_depth--;
    show(s_stack[s_depth - 1]);
}

void dw_nav_root(const dw_page_t *p)
{
    leave_top();
    s_depth = 0;
    s_stack[s_depth++] = p;
    show(p);
}

static void model_tick(lv_timer_t *t)
{
    (void)t;
    static uint32_t seen;
    if (seen == g_model.version) return;
    seen = g_model.version;
    const dw_page_t *p = dw_nav_top();
    if (p && p->update) p->update();
}

void dw_ui_start_timers(void)
{
    lv_timer_create(model_tick, 200, NULL);
    lv_timer_t *pw = lv_timer_create(bar_power_update, 2000, NULL);
    bar_power_update(pw);
}

/* ---------------- toast and light ---------------- */

static void toast_del(lv_timer_t *t)
{
    lv_obj_delete(lv_timer_get_user_data(t));
    lv_timer_delete(t);
}

void dw_toast(const char *text, uint32_t color)
{
    lv_obj_t *t = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(t);
    lv_obj_set_size(t, SCR_W - 24, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(t, C(0x161b21), 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(t, C(color), 0);
    lv_obj_set_style_border_width(t, 1, 0);
    lv_obj_set_style_radius(t, 6, 0);
    lv_obj_set_style_pad_all(t, 10, 0);
    lv_obj_t *l = ui_label(t, &dw_monob_11, color, text);
    lv_obj_set_width(l, LV_PCT(100));
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_align(t, LV_ALIGN_BOTTOM_MID, 0, -HINT_H - 10);
    lv_timer_create(toast_del, 2800, t);
}

static void light_back(lv_timer_t *t)
{
    (void)t;
    s_light_timer = NULL;
    dw_light_idle();
}

static void light_after(uint32_t ms)
{
    if (s_light_timer) lv_timer_delete(s_light_timer);
    s_light_timer = lv_timer_create(light_back, ms, NULL);
    lv_timer_set_repeat_count(s_light_timer, 1);
}

void dw_light_sending(void)
{
    if (s_light_timer) { lv_timer_delete(s_light_timer); s_light_timer = NULL; }
    hal_led(DW_LED_BLUE, DW_LED_BLINK);
}
void dw_light_ok(void) { hal_led(DW_LED_GREEN, DW_LED_SOLID); light_after(3000); }
void dw_light_fail(void) { hal_led(DW_LED_RED, DW_LED_BLINK); light_after(1500); }
void dw_light_idle(void)
{
    hal_power_t p;
    hal_power(&p);
    if (p.charging) hal_led(p.percent >= 100 ? DW_LED_GREEN : DW_LED_YELLOW, DW_LED_SOLID);
    else if (p.percent < 10) hal_led(DW_LED_RED, DW_LED_SLOW);
    else hal_led(DW_LED_OFF, DW_LED_SOLID);
}

/* ---------------- focus ---------------- */

void dw_style_focusable(lv_obj_t *obj) { lv_obj_add_style(obj, &s_focus, LV_STATE_USER_1); }

void dw_style_focus(lv_obj_t *obj, bool on)
{
    if (on) lv_obj_add_state(obj, LV_STATE_USER_1);
    else lv_obj_remove_state(obj, LV_STATE_USER_1);
}

static void focus_click(lv_event_t *e)
{
    dw_focus_t *f = lv_event_get_user_data(e);
    lv_obj_t *t = lv_event_get_current_target(e);
    /* A tap on a chip inside a focusable group only moves the focus; the chip's own
       handler picks the value. A tap on the item itself opens it. */
    bool direct = lv_event_get_target(e) == t;
    for (int i = 0; i < f->n; i++) {
        if (f->items[i] != t) continue;
        dw_focus_set(f, i);
        if (direct && f->on_open) f->on_open(i);
        return;
    }
}

void dw_focus_reset(dw_focus_t *f, dw_focus_cb on_open)
{
    f->n = 0;
    f->i = 0;
    f->on_open = on_open;
}

int dw_focus_add(dw_focus_t *f, lv_obj_t *obj)
{
    if (f->n >= 24) return -1;
    f->items[f->n] = obj;
    lv_obj_add_style(obj, &s_focus, LV_STATE_USER_1);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(obj, focus_click, LV_EVENT_CLICKED, f);
    dw_style_focus(obj, f->n == f->i);
    return f->n++;
}

void dw_focus_set(dw_focus_t *f, int i)
{
    if (!f->n) return;
    if (i < 0) i = 0;
    if (i >= f->n) i = f->n - 1;
    for (int k = 0; k < f->n; k++) dw_style_focus(f->items[k], k == i);
    f->i = i;
    lv_obj_update_layout(f->items[i]);
    lv_obj_scroll_to_view_recursive(f->items[i], LV_ANIM_ON);
}

void dw_focus_move(dw_focus_t *f, int d) { dw_focus_set(f, f->i + d); }

/* ---------------- widgets ---------------- */

lv_obj_t *ui_col(lv_obj_t *parent, int gap)
{
    lv_obj_t *o = box(parent);
    lv_obj_set_size(o, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(o, gap, 0);
    return o;
}

lv_obj_t *ui_rowbox(lv_obj_t *parent, int gap)
{
    lv_obj_t *o = box(parent);
    lv_obj_set_size(o, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(o, gap, 0);
    return o;
}

lv_obj_t *ui_title(lv_obj_t *parent, const char *text, bool small)
{
    return ui_label(parent, small ? &dw_disp_20 : &dw_disp_24, C_INK, text);
}

lv_obj_t *ui_sub(lv_obj_t *parent, const char *text, uint32_t color)
{
    lv_obj_t *l = ui_label(parent, &dw_mono_11, color ? color : C_MUTE, text);
    lv_obj_set_style_margin_top(l, -6, 0);
    return l;
}

lv_obj_t *ui_eyebrow(lv_obj_t *parent, const char *text)
{
    lv_obj_t *l = ui_label(parent, &dw_mono_9, C_DIM, text);
    lv_obj_set_style_text_letter_space(l, 1, 0);
    return l;
}

static void kind_border(lv_obj_t *o, dw_kind_t k)
{
    uint32_t c = k == K_NEON ? C_NEON : k == K_HOT ? C_HOT : k == K_WARN ? C_WARN : k == K_BAD ? C_BAD : C_LINE2;
    lv_obj_set_style_border_color(o, C(c), 0);
    lv_obj_set_style_border_opa(o, k == K_WARN ? 150 : LV_OPA_COVER, 0);
}

lv_obj_t *ui_card(lv_obj_t *parent, dw_kind_t kind)
{
    lv_obj_t *o = ui_col(parent, 4);
    lv_obj_set_style_border_width(o, 1, 0);
    kind_border(o, kind);
    lv_obj_set_style_radius(o, 6, 0);
    lv_obj_set_style_pad_hor(o, 12, 0);
    lv_obj_set_style_pad_ver(o, 10, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(o, C(kind == K_HOT ? 0x1a1229 : C_CARD), 0);
    return o;
}

lv_obj_t *ui_btn(lv_obj_t *parent, const char *text, dw_kind_t kind)
{
    lv_obj_t *o = box(parent);
    lv_obj_set_size(o, LV_PCT(100), 40);
    lv_obj_set_style_radius(o, 6, 0);
    lv_obj_set_style_border_width(o, 1, 0);
    uint32_t border = C_BORDER, fg = C_INK, bg = 0;
    lv_opa_t bgo = 0;
    switch (kind) {
    case K_PRIMARY: border = C_NEON; fg = C_NEON_INK; bg = C_NEON; bgo = LV_OPA_COVER; break;
    case K_HOT: border = C_HOT; fg = 0xffffff; bg = C_HOT; bgo = LV_OPA_COVER; break;
    case K_BAD: border = C_BAD; fg = C_BAD; break;
    case K_TRADE: border = C_NEON; fg = C_NEON; bg = 0x0f1a15; bgo = LV_OPA_COVER; break;
    default: break;
    }
    lv_obj_set_style_border_color(o, C(border), 0);
    lv_obj_set_style_bg_color(o, C(bg), 0);
    lv_obj_set_style_bg_opa(o, bgo, 0);
    lv_obj_t *l = ui_label(o, &dw_disp_14, fg, text);
    lv_obj_set_style_text_letter_space(l, 1, 0);
    lv_obj_center(l);
    lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

lv_obj_t *ui_badge(lv_obj_t *parent, const char *text, dw_kind_t kind)
{
    lv_obj_t *o = box(parent);
    lv_obj_set_size(o, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_radius(o, 3, 0);
    lv_obj_set_style_pad_hor(o, 6, 0);
    lv_obj_set_style_pad_ver(o, 3, 0);
    uint32_t c = kind == K_NEON ? C_NEON : kind == K_HOT ? C_HOT : kind == K_WARN ? C_WARN : kind == K_BAD ? C_BAD : C_BORDER;
    lv_obj_set_style_border_color(o, C(c), 0);
    uint32_t fg = kind == K_NEON ? C_NEON : kind == K_HOT ? C_HOT_TXT : kind == K_WARN ? C_WARN : kind == K_BAD ? C_BAD : C_SOFT;
    lv_obj_t *l = ui_label(o, &dw_mono_9, fg, text);
    lv_obj_set_style_text_letter_space(l, 1, 0);
    return o;
}

lv_obj_t *ui_big(lv_obj_t *parent, const char *text, const char *unit, const lv_font_t *font)
{
    lv_obj_t *o = box(parent);
    lv_obj_set_size(o, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(o, 5, 0);
    ui_label(o, font ? font : &dw_mono_30, C_INK, text);
    if (unit) {
        lv_obj_t *u = ui_label(o, &dw_mono_13, C_MUTE, unit);
        lv_obj_set_style_pad_bottom(u, 3, 0);
    }
    return o;
}

void ui_big_set(lv_obj_t *big, const char *text, uint32_t color)
{
    lv_obj_t *l = lv_obj_get_child(big, 0);
    lv_label_set_text(l, text);
    if (color) lv_obj_set_style_text_color(l, C(color), 0);
}

ui_row_t ui_row(lv_obj_t *parent, const char *title, const char *sub, const char *val, const char *small, bool mono)
{
    ui_row_t r = {0};
    r.obj = ui_rowbox(parent, 8);
    lv_obj_set_style_min_height(r.obj, 46, 0);
    lv_obj_set_style_pad_hor(r.obj, 10, 0);
    lv_obj_set_style_pad_ver(r.obj, 7, 0);
    lv_obj_set_style_border_side(r.obj, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(r.obj, 1, 0);
    lv_obj_set_style_border_color(r.obj, C(C_LINE), 0);
    lv_obj_t *left = ui_col(r.obj, 2);
    lv_obj_set_flex_grow(left, 1);
    lv_obj_set_width(left, LV_SIZE_CONTENT);
    r.title = ui_label(left, mono ? &dw_mono_15 : &dw_sansb_15, C_INK, title);
    if (sub) r.sub = ui_label(left, &dw_mono_11, C_MUTE, sub);
    if (val || small) {
        lv_obj_t *right = ui_col(r.obj, 1);
        lv_obj_set_width(right, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
        r.val = ui_label(right, &dw_mono_13, C_INK, val ? val : "");
        if (small) r.small = ui_label(right, &dw_mono_9, C_MUTE, small);
    }
    return r;
}

lv_obj_t *ui_chips(lv_obj_t *parent, const char *const *items, int n, int on)
{
    lv_obj_t *o = box(parent);
    lv_obj_set_size(o, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(o, 6, 0);
    lv_obj_set_style_pad_row(o, 6, 0);
    lv_obj_set_style_pad_all(o, 3, 0);
    lv_obj_set_style_radius(o, 6, 0);
    for (int i = 0; i < n; i++) {
        lv_obj_t *c = box(o);
        lv_obj_set_size(c, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_hor(c, 9, 0);
        lv_obj_set_style_pad_ver(c, 6, 0);
        lv_obj_set_style_border_width(c, 1, 0);
        lv_obj_set_style_border_color(c, C(C_BORDER), 0);
        lv_obj_set_style_radius(c, 4, 0);
        lv_obj_set_style_text_color(c, C(C_SOFT), 0);
        lv_obj_add_style(c, &s_chip_on, LV_STATE_CHECKED);
        lv_obj_t *l = lv_label_create(c);
        lv_obj_set_style_text_font(l, &dw_mono_11, 0);
        lv_label_set_text(l, items[i]);
        lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(c, LV_OBJ_FLAG_EVENT_BUBBLE);
    }
    ui_chips_set(o, on);
    return o;
}

void ui_chips_set(lv_obj_t *chips, int on)
{
    uint32_t n = lv_obj_get_child_count(chips);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *c = lv_obj_get_child(chips, (int32_t)i);
        if ((int)i == on) lv_obj_add_state(c, LV_STATE_CHECKED);
        else lv_obj_remove_state(c, LV_STATE_CHECKED);
    }
}

lv_obj_t *ui_seg(lv_obj_t *parent, const char *a, const char *b, int on)
{
    const char *items[2] = {a, b};
    lv_obj_t *o = ui_chips(parent, items, 2, on);
    lv_obj_set_style_pad_column(o, 6, 0);
    for (int i = 0; i < 2; i++) {
        lv_obj_t *c = lv_obj_get_child(o, i);
        lv_obj_set_width(c, LV_PCT(48));
        lv_obj_set_style_text_align(c, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(lv_obj_get_child(c, 0), LV_PCT(100));
    }
    return o;
}

lv_obj_t *ui_kv(lv_obj_t *parent)
{
    lv_obj_t *o = ui_col(parent, 6);
    lv_obj_set_style_pad_all(o, 3, 0);
    lv_obj_set_style_radius(o, 6, 0);
    return o;
}

lv_obj_t *ui_kv_add(lv_obj_t *kv, const char *k, const char *v, uint32_t color)
{
    lv_obj_t *r = ui_rowbox(kv, 8);
    ui_label(r, &dw_sans_12, C_MUTE, k);
    return ui_label(r, &dw_mono_13, color ? color : C_INK, v);
}

lv_obj_t *ui_tabs(lv_obj_t *parent, const char *const *items, int n, int on)
{
    lv_obj_t *o = ui_rowbox(parent, 14);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_side(o, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_border_color(o, C(C_LINE2), 0);
    for (int i = 0; i < n; i++) {
        lv_obj_t *l = ui_label(o, &dw_monob_11, C_MUTE, items[i]);
        lv_obj_set_style_pad_bottom(l, 5, 0);
        lv_obj_add_style(l, &s_tab_on, LV_STATE_CHECKED);
        if (i == on) lv_obj_add_state(l, LV_STATE_CHECKED);
        lv_obj_add_flag(l, LV_OBJ_FLAG_CLICKABLE);
    }
    char pg[16];
    snprintf(pg, sizeof pg, "%d / %d", on + 1, n);
    lv_obj_t *p = ui_label(o, &dw_mono_9, C_DIM, pg);
    lv_obj_set_flex_grow(p, 1);
    lv_obj_set_style_text_align(p, LV_TEXT_ALIGN_RIGHT, 0);
    return o;
}

lv_obj_t *ui_sign(lv_obj_t *parent, const char *text)
{
    lv_obj_t *o = ui_rowbox(parent, 10);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(o, C(C_NEON), 0);
    lv_obj_set_style_bg_opa(o, 30, 0);
    lv_obj_set_style_radius(o, 6, 0);
    lv_obj_set_style_pad_all(o, 9, 0);
    lv_obj_t *k = box(o);
    lv_obj_set_size(k, 6, 16);
    lv_obj_set_style_bg_color(k, C(C_NEON), 0);
    lv_obj_set_style_bg_opa(k, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(k, 1, 0);
    lv_obj_t *l = ui_label(o, &dw_monob_11, C_NEON, text);
    lv_obj_set_flex_grow(l, 1);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    return o;
}

/* ---- push to talk ---- */

/* The talk button: idle (HOLD), held (LIVE, recording) and released while the
 * clip is being transcribed (SENDING). */
typedef struct { lv_obj_t *o, *bars[9], *h, *s; lv_timer_t *t; bool active; int state; char sub[24]; } ptt_t;

static void ptt_tick(lv_timer_t *t)
{
    ptt_t *p = lv_timer_get_user_data(t);
    uint32_t ms = lv_tick_get();
    int state = p->active || dw_voice_held() ? 1 : dw_voice_listening() ? 2 : 0;
    if (state != p->state) {
        p->state = state;
        static const char *H[] = {"HOLD", "LIVE", "SENDING"};
        lv_label_set_text(p->h, H[state]);
        lv_label_set_text(p->s, state == 1 ? "RELEASE TO SEND" : state == 2 ? "TRANSCRIBING\xE2\x80\xA6" : p->sub);
        lv_obj_set_style_text_color(p->h, C(state == 2 ? C_SOFT : C_NEON), 0);
        lv_obj_set_style_border_color(p->o, C(state == 2 ? C_LINE2 : C_NEON), 0);
        lv_obj_set_style_shadow_opa(p->o, state == 2 ? 0 : state == 1 ? 120 : 50, 0);
    }
    for (int i = 0; i < 9; i++) {
        /* held: loud; sending: a travelling dot; idle: breathing */
        int h = state == 1 ? 4 + (int)(16 * fabs(sin(ms / 140.0 + i * 1.3)))
              : state == 2 ? ((int)(ms / 90) % 9 == i ? 14 : 4)
                           : 4 + (int)(7 * fabs(sin(ms / 330.0 + i * 0.9)));
        lv_obj_set_height(p->bars[i], h);
    }
}

static void ptt_event(lv_event_t *e)
{
    ptt_t *p = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *o = lv_event_get_current_target(e);
    if (code == LV_EVENT_PRESSED) {
        p->active = true;
        lv_obj_set_style_bg_opa(o, 60, 0);
        dw_app_input(DW_IN_PTT_DOWN);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        p->active = false;
        lv_obj_set_style_bg_opa(o, 0, 0);
        dw_app_input(DW_IN_PTT_UP);
    } else if (code == LV_EVENT_DELETE) {
        lv_timer_delete(p->t);
        free(p);
    }
}

lv_obj_t *ui_ptt(lv_obj_t *parent, const char *sub)
{
    lv_obj_t *wrap = box(parent);
    lv_obj_set_size(wrap, LV_PCT(100), 126);
    lv_obj_t *o = box(wrap);
    lv_obj_set_size(o, 110, 110);
    lv_obj_center(o);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(o, 3, 0);
    lv_obj_set_style_border_color(o, C(C_NEON), 0);
    lv_obj_set_style_bg_color(o, C(C_NEON), 0);
    lv_obj_set_style_shadow_color(o, C(C_NEON), 0);
    lv_obj_set_style_shadow_width(o, 26, 0);
    lv_obj_set_style_shadow_spread(o, 4, 0);
    lv_obj_set_style_shadow_opa(o, 50, 0);
    lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
    ptt_t *p = calloc(1, sizeof *p);
    p->o = o;
    snprintf(p->sub, sizeof p->sub, "%s", sub);
    lv_obj_t *bars = box(o);
    lv_obj_set_size(bars, 60, 22);
    lv_obj_align(bars, LV_ALIGN_CENTER, 0, -18);
    lv_obj_set_flex_flow(bars, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bars, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bars, 3, 0);
    for (int i = 0; i < 9; i++) {
        lv_obj_t *b = box(bars);
        lv_obj_set_size(b, 3, 8);
        lv_obj_set_style_bg_color(b, C(C_NEON), 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        p->bars[i] = b;
    }
    p->h = ui_label(o, &dw_disp_20, C_NEON, "HOLD");
    lv_obj_align(p->h, LV_ALIGN_CENTER, 0, 10);
    p->s = ui_label(o, &dw_mono_9, C_MUTE, sub);
    lv_obj_align(p->s, LV_ALIGN_CENTER, 0, 30);
    p->t = lv_timer_create(ptt_tick, 60, p);
    lv_obj_add_event_cb(o, ptt_event, LV_EVENT_ALL, p);
    return o;
}

lv_obj_t *ui_liqbar(lv_obj_t *parent, double safe)
{
    lv_obj_t *b = lv_bar_create(parent);
    lv_obj_set_size(b, LV_PCT(100), 6);
    lv_bar_set_range(b, 0, 100);
    lv_bar_set_value(b, (int32_t)(fmax(0, fmin(1, safe)) * 100), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(b, C(0x232931), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(b, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(b, 2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(b, C(safe < 0.27 ? C_WARN : C_NEON), LV_PART_INDICATOR);
    return b;
}

/* ---- candlestick chart ---- */

typedef struct {
    dw_candle_t c[64];
    int n;
    double live, entry;
} chart_t;

static void chart_draw(lv_event_t *e)
{
    lv_obj_t *o = lv_event_get_current_target(e);
    chart_t *ch = lv_obj_get_user_data(o);
    lv_layer_t *layer = lv_event_get_layer(e);
    lv_area_t a;
    lv_obj_get_content_coords(o, &a);
    int x0 = a.x1, x1 = a.x2 - 48, y0 = a.y1 + 4, y1 = a.y2 - 4;
    if (!ch || ch->n < 2) {
        lv_draw_label_dsc_t ld;
        lv_draw_label_dsc_init(&ld);
        ld.font = &dw_mono_11;
        ld.color = C(C_DIM);
        ld.text = "Loading chart\xE2\x80\xA6";
        ld.align = LV_TEXT_ALIGN_CENTER;
        lv_area_t ta = {a.x1, (a.y1 + a.y2) / 2 - 8, a.x2, (a.y1 + a.y2) / 2 + 8};
        lv_draw_label(layer, &ld, &ta);
        return;
    }
    double hi = -1e300, lo = 1e300;
    for (int i = 0; i < ch->n; i++) {
        double h = ch->c[i].h, l = ch->c[i].l;
        if (i == ch->n - 1 && ch->live > 0) { h = fmax(h, ch->live); l = fmin(l, ch->live); }
        hi = fmax(hi, h);
        lo = fmin(lo, l);
    }
    if (ch->entry > 0 && ch->entry < hi * 1.15 && ch->entry > lo * 0.85) { hi = fmax(hi, ch->entry); lo = fmin(lo, ch->entry); }
    double pad = (hi - lo) * 0.06 + 1e-9;
    hi += pad;
    lo -= pad;
#define CY(v) (y0 + (int)((hi - (v)) / (hi - lo) * (y1 - y0)))

    lv_draw_line_dsc_t gl;
    lv_draw_line_dsc_init(&gl);
    gl.color = C(0x1d2229);
    gl.width = 1;
    lv_draw_label_dsc_t ld;
    lv_draw_label_dsc_init(&ld);
    ld.font = &dw_mono_9;
    ld.color = C(0x56606c);
    for (int i = 0; i <= 4; i++) {
        int yy = y0 + i * (y1 - y0) / 4;
        gl.p1.x = x0; gl.p1.y = yy; gl.p2.x = x1; gl.p2.y = yy;
        lv_draw_line(layer, &gl);
        char t[16];
        double v = hi - i * (hi - lo) / 4;
        snprintf(t, sizeof t, "%s", fmt_px(v));
        ld.text = t;
        ld.text_local = 1;
        lv_area_t ta = {x1 + 3, yy - 6, a.x2, yy + 6};
        lv_draw_label(layer, &ld, &ta);
    }
    double step = (double)(x1 - x0) / ch->n;
    for (int i = 0; i < ch->n; i++) {
        dw_candle_t *k = &ch->c[i];
        double cl = (i == ch->n - 1 && ch->live > 0) ? ch->live : k->c;
        bool up = cl >= k->o;
        lv_color_t col = C(up ? C_NEON : C_BAD);
        int cx = x0 + (int)(step * (i + 0.5));
        lv_draw_line_dsc_t wl;
        lv_draw_line_dsc_init(&wl);
        wl.color = col;
        wl.width = 1;
        wl.p1.x = cx; wl.p1.y = CY(fmax(k->h, cl));
        wl.p2.x = cx; wl.p2.y = CY(fmin(k->l, cl));
        lv_draw_line(layer, &wl);
        lv_draw_rect_dsc_t rd;
        lv_draw_rect_dsc_init(&rd);
        rd.bg_color = col;
        int top = CY(fmax(k->o, cl)), bot = CY(fmin(k->o, cl));
        int hw = (int)fmax(1, step * 0.32);
        lv_area_t ra = {cx - hw, top, cx + hw, bot > top ? bot : top + 1};
        lv_draw_rect(layer, &rd, &ra);
    }
    if (ch->entry > lo && ch->entry < hi) {
        int ey = CY(ch->entry);
        lv_draw_line_dsc_t el;
        lv_draw_line_dsc_init(&el);
        el.color = C(C_HOT_TXT);
        el.width = 1;
        el.dash_width = 4;
        el.dash_gap = 3;
        el.p1.x = x0; el.p1.y = ey; el.p2.x = x1; el.p2.y = ey;
        lv_draw_line(layer, &el);
        char t[32];
        snprintf(t, sizeof t, "ENTRY %s", fmt_px(ch->entry));
        ld.color = C(C_HOT_TXT);
        ld.text = t;
        lv_area_t ta = {x0 + 2, ey - 13, x1, ey - 1};
        lv_draw_label(layer, &ld, &ta);
    }
    if (ch->live > 0) {
        int ly = CY(ch->live);
        lv_draw_rect_dsc_t rd;
        lv_draw_rect_dsc_init(&rd);
        rd.bg_color = C(C_NEON);
        rd.radius = 2;
        lv_area_t ra = {x1 + 1, ly - 7, a.x2, ly + 7};
        lv_draw_rect(layer, &rd, &ra);
        char t[16];
        snprintf(t, sizeof t, "%s", fmt_px(ch->live));
        ld.color = C(C_NEON_INK);
        ld.text = t;
        lv_area_t ta = {x1 + 3, ly - 6, a.x2, ly + 6};
        lv_draw_label(layer, &ld, &ta);
    }
#undef CY
}

static void chart_del(lv_event_t *e) { free(lv_obj_get_user_data(lv_event_get_current_target(e))); }

lv_obj_t *ui_chart(lv_obj_t *parent, int h)
{
    lv_obj_t *o = box(parent);
    lv_obj_set_size(o, LV_PCT(100), h);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_border_color(o, C(C_LINE2), 0);
    lv_obj_set_style_radius(o, 6, 0);
    lv_obj_set_style_pad_all(o, 4, 0);
    lv_obj_set_style_bg_color(o, C(0x0c1512), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_user_data(o, calloc(1, sizeof(chart_t)));
    lv_obj_add_event_cb(o, chart_draw, LV_EVENT_DRAW_MAIN, NULL);
    lv_obj_add_event_cb(o, chart_del, LV_EVENT_DELETE, NULL);
    return o;
}

void ui_chart_set(lv_obj_t *chart, const void *candles, int n, double live, double entry)
{
    chart_t *ch = lv_obj_get_user_data(chart);
    if (!ch) return;
    if (candles) {
        if (n > 64) n = 64;
        memcpy(ch->c, candles, sizeof(dw_candle_t) * n);
        ch->n = n;
    }
    ch->live = live;
    ch->entry = entry;
    lv_obj_invalidate(chart);
}
