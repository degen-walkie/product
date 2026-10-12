/* Degen Walkie hardware abstraction.
 *
 * The core (UI, venues, signing) only talks to the board through these functions.
 * Two implementations exist:
 *   main/   ESP32-S3 board (ST7796U + GT911, wheel, trade key, AXP2101, Wi-Fi)
 *   sim/    WebAssembly build for the browser simulator
 *
 * Threading: every callback below is delivered on the UI thread (the one running
 * lv_timer_handler), so callbacks may touch LVGL objects directly. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Physical controls (platform -> core) ---- */
typedef enum {
    DW_IN_WHEEL_UP,     /* roll one detent away from the user */
    DW_IN_WHEEL_DOWN,   /* roll one detent towards the user */
    DW_IN_WHEEL_PRESS,  /* press the wheel: back (menu at the top level) */
    DW_IN_TRADE,        /* the trade key: open / enter / SIGN */
    DW_IN_POWER,        /* short press: screen off / on */
    DW_IN_PTT_DOWN,     /* hold-to-talk started (on-screen mic button or key combo) */
    DW_IN_PTT_UP,
    DW_IN_POWER_HOLD,   /* power key held 2 s: boot when off, power-off sheet when on */
} dw_input_t;

/* Implemented by the core; the platform calls it from the UI thread. */
void dw_app_input(dw_input_t in);

/* ---- Status light ---- */
typedef enum { DW_LED_OFF, DW_LED_GREEN, DW_LED_BLUE, DW_LED_RED, DW_LED_YELLOW, DW_LED_PURPLE } dw_led_color_t;
typedef enum { DW_LED_SOLID, DW_LED_BLINK, DW_LED_SLOW } dw_led_mode_t;
void hal_led(dw_led_color_t color, dw_led_mode_t mode);
void hal_vibrate(uint32_t ms);
void hal_backlight(bool on);

/* ---- Time and randomness ---- */
uint32_t hal_millis(void);      /* monotonic */
int64_t hal_unix_ms(void);      /* wall clock (SNTP on the board) */
void hal_random_fill(uint8_t *buf, size_t n);  /* cryptographic RNG */

/* ---- Battery / power ---- */
typedef struct { int percent; bool charging; bool usb; } hal_power_t;
void hal_power(hal_power_t *out);

/* ---- Network ----
 * HTTP: one request, one callback. status < 0 means a transport error.
 * headers: "Name: value\n" lines, may be NULL. body may be NULL. */
typedef void (*hal_http_cb)(void *ctx, int status, const char *body, size_t len);
int hal_http(const char *method, const char *url, const char *headers, const char *body, hal_http_cb cb, void *ctx);

/* WebSocket: on_msg gets every text frame; on_state gets 1 when open, 0 when closed. */
typedef void (*hal_ws_msg_cb)(void *ctx, const char *msg, size_t len);
typedef void (*hal_ws_state_cb)(void *ctx, int open);
int hal_ws_open(const char *url, hal_ws_msg_cb on_msg, hal_ws_state_cb on_state, void *ctx);
void hal_ws_send(int handle, const char *text);
void hal_ws_close(int handle);
bool hal_net_up(void);

/* ---- Encrypted key/value storage (NVS with flash encryption on the board) ---- */
int hal_store_get(const char *key, char *buf, size_t cap);   /* returns length, or -1 if missing */
int hal_store_set(const char *key, const char *val);          /* val == NULL deletes */

/* ---- Voice ----
 * The mic is streamed to the relay for speech-to-text on the board. The result
 * (or an error) comes back through dw_app_voice_result() on the UI thread. */
void hal_voice_start(void);
void hal_voice_stop(void);
void dw_app_voice_result(const char *text);   /* implemented by the core; NULL = nothing heard */

/* Name of the Wi-Fi network the device is on ("" when offline). */
void hal_wifi_ssid(char *out, size_t cap);

/* Text shown as a QR code on the device, exposed so the simulator's "phone" can scan it. */
void hal_qr_shown(const char *payload);

#ifdef __cplusplus
}
#endif
