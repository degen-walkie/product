/* WebAssembly entry point: the same firmware core, with the browser as the board.
 * Display: a 320 x 480 RGB565 framebuffer that the page blits to a canvas.
 * Input: pointer events from the canvas, the wheel / trade / power keys from the shell.
 * Everything else goes through hal_web.c. */
#include <emscripten.h>
#include <stdint.h>
#include <stdlib.h>

#include "dw_app.h"
#include "dw_hal.h"
#include "lvgl.h"

#define W 320
#define H 480

static uint16_t s_fb[W * H];
static int32_t s_tx, s_ty;
static bool s_down;

EM_JS(void, js_present, (void), { if (Module.dwHost) Module.dwHost.present(); });

static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *px)
{
    (void)a;
    (void)px;
    if (lv_display_flush_is_last(d)) js_present();
    lv_display_flush_ready(d);
}

static void read_touch(lv_indev_t *in, lv_indev_data_t *data)
{
    (void)in;
    data->point.x = s_tx;
    data->point.y = s_ty;
    data->state = s_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static uint32_t tick(void) { return (uint32_t)emscripten_get_now(); }

EMSCRIPTEN_KEEPALIVE uint16_t *dw_sim_fb(void) { return s_fb; }

EMSCRIPTEN_KEEPALIVE void dw_sim_touch(int x, int y, int down)
{
    s_tx = x;
    s_ty = y;
    s_down = down != 0;
}

/* 0 wheel up, 1 wheel down, 2 wheel press, 3 trade, 4 power, 5 ptt down, 6 ptt up */
EMSCRIPTEN_KEEPALIVE void dw_sim_input(int code) { dw_app_input((dw_input_t)code); }

static void loop(void) { lv_timer_handler(); }

int main(void)
{
    lv_init();
    lv_tick_set_cb(tick);
    lv_display_t *d = lv_display_create(W, H);
    lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d, s_fb, NULL, sizeof s_fb, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(d, flush);
    lv_indev_t *t = lv_indev_create();
    lv_indev_set_type(t, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(t, read_touch);
    dw_app_init();
    emscripten_set_main_loop(loop, 0, 0);
    return 0;
}
