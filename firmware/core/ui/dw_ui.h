/* UI kit for the 320 x 480 screen. Mirrors the on-device UI shown on
 * degenwalkie.trade (same palette, type and spacing; 1 em there = 10 px here). */
#pragma once
#include <stdbool.h>

#include "dw_fonts.h"
#include "dw_hal.h"
#include "lvgl.h"

#define C_BG 0x0b0d10
#define C_CARD 0x0f1216
#define C_LINE 0x1a1e24
#define C_LINE2 0x1f242b
#define C_BORDER 0x2c323b
#define C_INK 0xe9ecf0
#define C_SOFT 0xb6bcc6
#define C_MUTE 0x8a929e
#define C_DIM 0x6c7480
#define C_NEON 0x14f195
#define C_NEON_INK 0x03140b
#define C_HOT 0x9945ff
#define C_HOT_TXT 0xc89bff
#define C_BAD 0xff4d4d
#define C_WARN 0xffb020
#define C_BLUE 0x4da3ff

#define SCR_W 320
#define SCR_H 480
#define BAR_H 32
#define HINT_H 34

typedef enum { K_PLAIN, K_NEON, K_HOT, K_WARN, K_BAD, K_PRIMARY, K_TRADE } dw_kind_t;

/* ---- pages and navigation ---- */
typedef struct {
    const char *name;
    void (*build)(lv_obj_t *body);
    void (*update)(void);           /* model changed (max ~5 Hz) */
    bool (*input)(dw_input_t in);   /* return true when handled */
    void (*leave)(void);
    bool top;                       /* top level: wheel press opens the menu */
} dw_page_t;

void dw_ui_init(void);
void dw_nav_push(const dw_page_t *p);
void dw_nav_replace(const dw_page_t *p);
void dw_nav_pop(void);
void dw_nav_root(const dw_page_t *p);
const dw_page_t *dw_nav_top(void);
lv_obj_t *dw_body(void);

void dw_bar(const char *label, const char *sub, uint32_t color);
void dw_hint(const char *roll, const char *press, const char *trade, bool sign);
void dw_toast(const char *text, uint32_t color);

/* Status light patterns from the website legend */
void dw_light_sending(void);   /* blue blink */
void dw_light_ok(void);        /* green 3 s */
void dw_light_fail(void);      /* red x3 */
void dw_light_idle(void);

/* ---- focus (the wheel moves it, the trade key opens it, touch sets it) ---- */
typedef void (*dw_focus_cb)(int index);
typedef struct {
    lv_obj_t *items[24];
    int n, i;
    dw_focus_cb on_open;   /* trade key or tap */
} dw_focus_t;
void dw_focus_reset(dw_focus_t *f, dw_focus_cb on_open);
int dw_focus_add(dw_focus_t *f, lv_obj_t *obj);
void dw_focus_set(dw_focus_t *f, int i);
void dw_focus_move(dw_focus_t *f, int d);
void dw_style_focus(lv_obj_t *obj, bool on);

/* ---- widgets ---- */
lv_obj_t *ui_col(lv_obj_t *parent, int gap);
lv_obj_t *ui_rowbox(lv_obj_t *parent, int gap);
lv_obj_t *ui_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *text);
lv_obj_t *ui_title(lv_obj_t *parent, const char *text, bool small);
lv_obj_t *ui_sub(lv_obj_t *parent, const char *text, uint32_t color);
lv_obj_t *ui_eyebrow(lv_obj_t *parent, const char *text);
lv_obj_t *ui_card(lv_obj_t *parent, dw_kind_t kind);
lv_obj_t *ui_btn(lv_obj_t *parent, const char *text, dw_kind_t kind);
lv_obj_t *ui_badge(lv_obj_t *parent, const char *text, dw_kind_t kind);
lv_obj_t *ui_big(lv_obj_t *parent, const char *text, const char *unit, const lv_font_t *font);
void ui_big_set(lv_obj_t *big, const char *text, uint32_t color);

/* List row: title / sub on the left, value / small on the right. */
typedef struct { lv_obj_t *obj, *title, *sub, *val, *small; } ui_row_t;
ui_row_t ui_row(lv_obj_t *parent, const char *title, const char *sub, const char *val, const char *small, bool mono);

/* Chips: one selected. Returns the container; chips are its children. */
lv_obj_t *ui_chips(lv_obj_t *parent, const char *const *items, int n, int on);
void ui_chips_set(lv_obj_t *chips, int on);
lv_obj_t *ui_seg(lv_obj_t *parent, const char *a, const char *b, int on);

/* Key / value grid. ui_kv_add returns the value label. */
lv_obj_t *ui_kv(lv_obj_t *parent);
lv_obj_t *ui_kv_add(lv_obj_t *kv, const char *k, const char *v, uint32_t color);

lv_obj_t *ui_tabs(lv_obj_t *parent, const char *const *items, int n, int on);
lv_obj_t *ui_sign(lv_obj_t *parent, const char *text);
lv_obj_t *ui_ptt(lv_obj_t *parent, const char *sub);
lv_obj_t *ui_liqbar(lv_obj_t *parent, double safe);

/* Candlestick chart */
lv_obj_t *ui_chart(lv_obj_t *parent, int h);
void ui_chart_set(lv_obj_t *chart, const void *candles, int n, double live, double entry);

/* Formatting: returns a pointer to one of 8 rotating static buffers. */
const char *fmt_num(double v, int dec);            /* 6,184.20 */
const char *fmt_signed(double v, int dec);         /* +212.48 */
const char *fmt_px(double v);                      /* sensible decimals for a price */
const char *fmt_pct(double v);                     /* +3.1% */
const char *fmt_short(const char *s, int head, int tail);  /* 0x8f3…c21 */
