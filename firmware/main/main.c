/* ESP32-S3 entry point: board, LVGL, the input poller and the UI task. */
#include <string.h>

#include "board.h"
#include "dw_app.h"
#include "dw_hal.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "platform.h"
#include "driver/gpio.h"

static const char *TAG = "main";
static QueueHandle_t s_ui_q;

/* ---------------- UI thread queue ---------------- */

typedef struct { void (*fn)(void *); void *arg; } ui_job_t;

void platform_post(void (*fn)(void *), void *arg)
{
    ui_job_t j = {fn, arg};
    xQueueSend(s_ui_q, &j, portMAX_DELAY);
}

static void post_input(void *arg) { dw_app_input((dw_input_t)(intptr_t)arg); }

/* ---------------- display ---------------- */

static bool on_flush_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *e, void *ctx)
{
    lv_display_flush_ready((lv_display_t *)ctx);
    return false;
}

static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *px)
{
    lv_draw_sw_rgb565_swap(px, lv_area_get_size(a));
    esp_lcd_panel_draw_bitmap(g_panel, a->x1, a->y1, a->x2 + 1, a->y2 + 1, px);
}

static void read_touch(lv_indev_t *in, lv_indev_data_t *data)
{
    uint16_t x, y;
    uint8_t n = 0;
    if (g_touch) {
        esp_lcd_touch_read_data(g_touch);
        esp_lcd_touch_get_coordinates(g_touch, &x, &y, NULL, &n, 1);
    }
    if (n) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static uint32_t tick(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

/* ---------------- physical controls ----------------
 * Wheel A is a GPIO, wheel B, the wheel switch and the trade key sit on the
 * expander, so everything is polled every 4 ms and decoded here. */

static void input_task(void *arg)
{
    int last_a = gpio_get_level(PIN_WHEEL_A);
    bool trade_was = false, sw_was = false;
    uint32_t n = 0;
    for (;;) {
        uint16_t e = exp_read();
        int a = gpio_get_level(PIN_WHEEL_A);
        if (a != last_a && a == 0) {
            bool b = (e & EXP_WHEEL_B) != 0;
            platform_post(post_input, (void *)(intptr_t)(b ? DW_IN_WHEEL_DOWN : DW_IN_WHEEL_UP));
        }
        last_a = a;
        bool trade = !(e & EXP_KEY_TRADE), sw = !(e & EXP_WHEEL_SW);
        if (trade && !trade_was) platform_post(post_input, (void *)(intptr_t)DW_IN_TRADE);
        if (sw && !sw_was) platform_post(post_input, (void *)(intptr_t)DW_IN_WHEEL_PRESS);
        trade_was = trade;
        sw_was = sw;
        if (++n % 25 == 0 && !(e & EXP_AXP_IRQ)) {
            int k = axp_power_key();
            if (k) platform_post(post_input, (void *)(intptr_t)(k == 2 ? DW_IN_POWER_HOLD : DW_IN_POWER));
        }
        vTaskDelay(pdMS_TO_TICKS(4));
    }
}

/* ---------------- UI task ---------------- */

static void ui_task(void *arg)
{
    lv_init();
    lv_tick_set_cb(tick);
    lv_display_t *d = lv_display_create(LCD_W, LCD_H);
    size_t buf_px = LCD_W * 40;
    void *b1 = heap_caps_malloc(buf_px * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void *b2 = heap_caps_malloc(buf_px * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d, b1, b2, buf_px * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d, flush);
    esp_lcd_panel_io_callbacks_t cbs = {.on_color_trans_done = on_flush_done};
    esp_lcd_panel_io_register_event_callbacks(g_lcd_io, &cbs, d);
    lv_indev_t *t = lv_indev_create();
    lv_indev_set_type(t, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(t, read_touch);

    dw_app_init();
    board_backlight(80);
    for (;;) {
        ui_job_t j;
        while (xQueueReceive(s_ui_q, &j, 0) == pdTRUE) j.fn(j.arg);
        uint32_t wait = lv_timer_handler();
        if (wait > 10) wait = 10;
        vTaskDelay(pdMS_TO_TICKS(wait ? wait : 1));
    }
}

void app_main(void)
{
    s_ui_q = xQueueCreate(64, sizeof(ui_job_t));
    board_init();
    platform_init();   /* NVS, Wi-Fi, SNTP, status light */
    xTaskCreatePinnedToCore(ui_task, "ui", 16 * 1024, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(input_task, "input", 4 * 1024, NULL, 6, NULL, 0);
    ESP_LOGI(TAG, "Degen Walkie " DW_FW_VERSION " started");
}
