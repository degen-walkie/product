/* ESP32-S3 implementation of dw_hal.h.
 *
 * Network calls run on a worker task (esp_http_client, esp_websocket_client with the
 * certificate bundle); results are handed back to the UI task with platform_post(),
 * so the core only ever runs on one thread. Keys live in NVS; with flash encryption
 * and NVS encryption enabled (see sdkconfig.defaults.secure) they are encrypted at rest.
 *
 * NOT YET VALIDATED ON HARDWARE. */
#include "platform.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "board.h"
#include "dw_hal.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_websocket_client.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "platform";
static nvs_handle_t s_nvs;
static volatile bool s_net_up;

/* ---------------- time, randomness, power ---------------- */

uint32_t hal_millis(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

int64_t hal_unix_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void hal_random_fill(uint8_t *buf, size_t n) { esp_fill_random(buf, n); }

void hal_power(hal_power_t *out)
{
    static int64_t at;
    static hal_power_t cache = {.percent = 100};
    if (esp_timer_get_time() - at > 2000000) {   /* the gauge changes slowly; spare the I2C bus */
        at = esp_timer_get_time();
        cache.percent = axp_battery_percent();
        cache.usb = axp_vbus();
        cache.charging = axp_charging();
    }
    *out = cache;
}

bool hal_net_up(void) { return s_net_up; }
void hal_backlight(bool on) { board_backlight(on ? 80 : 0); }
void hal_qr_shown(const char *payload) { (void)payload; }


/* ---------------- status light and motor ----------------
 * Rev 7 has red and green on the expander; blue (AXP2101 CHGLED) is not fitted. */

static esp_timer_handle_t s_led_t, s_vibe_t;
static dw_led_color_t s_led_c;
static dw_led_mode_t s_led_m;
static bool s_led_on;

static void led_apply(bool on)
{
    bool r = on && (s_led_c == DW_LED_RED || s_led_c == DW_LED_YELLOW || s_led_c == DW_LED_PURPLE || s_led_c == DW_LED_BLUE);
    bool g = on && (s_led_c == DW_LED_GREEN || s_led_c == DW_LED_YELLOW || s_led_c == DW_LED_BLUE);
    exp_write(EXP_LED_R, !r);   /* active low */
    exp_write(EXP_LED_G, !g);
}

static void led_tick(void *arg)
{
    s_led_on = !s_led_on;
    led_apply(s_led_on);
}

void hal_led(dw_led_color_t c, dw_led_mode_t m)
{
    s_led_c = c;
    s_led_m = m;
    esp_timer_stop(s_led_t);
    s_led_on = c != DW_LED_OFF;
    led_apply(s_led_on);
    if (c != DW_LED_OFF && m != DW_LED_SOLID) esp_timer_start_periodic(s_led_t, m == DW_LED_BLINK ? 250000 : 1200000);
}

static void vibe_off(void *arg) { exp_write(EXP_VIBE, false); }

void hal_vibrate(uint32_t ms)
{
    exp_write(EXP_VIBE, true);
    esp_timer_stop(s_vibe_t);
    esp_timer_start_once(s_vibe_t, (uint64_t)ms * 1000);
}

/* ---------------- storage ---------------- */

int hal_store_get(const char *key, char *buf, size_t cap)
{
    size_t n = cap;
    if (nvs_get_str(s_nvs, key, buf, &n) != ESP_OK) return -1;
    return (int)strlen(buf);
}

int hal_store_set(const char *key, const char *val)
{
    esp_err_t e = val ? nvs_set_str(s_nvs, key, val) : nvs_erase_key(s_nvs, key);
    nvs_commit(s_nvs);
    return e == ESP_OK || e == ESP_ERR_NVS_NOT_FOUND ? 0 : -1;
}

/* ---------------- voice ----------------
 * The ES8388 capture path and the relay's speech-to-text stream are not in this
 * build yet; report "nothing heard" so the UI falls back to touch and the wheel. */

static void voice_none(void *arg) { dw_app_voice_result(NULL); }
void hal_voice_start(void) {}
void hal_voice_stop(void) { platform_post(voice_none, NULL); }

/* ---------------- HTTP ---------------- */

typedef struct {
    char *method, *url, *headers, *body;
    hal_http_cb cb;
    void *ctx;
    int status;
    char *resp;
    size_t len;
} http_job_t;

static QueueHandle_t s_http_q;

static char *sdup(const char *s) { return s ? strdup(s) : NULL; }

static void http_deliver(void *arg)
{
    http_job_t *j = arg;
    j->cb(j->ctx, j->status, j->resp, j->len);
    free(j->method); free(j->url); free(j->headers); free(j->body); free(j->resp);
    free(j);
}

static void http_run(http_job_t *j)
{
    esp_http_client_config_t cfg = {.url = j->url, .crt_bundle_attach = esp_crt_bundle_attach, .timeout_ms = 10000};
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    esp_http_client_set_method(c, !strcmp(j->method, "POST") ? HTTP_METHOD_POST : HTTP_METHOD_GET);
    for (char *line = j->headers, *nl; line && *line; line = nl ? nl + 1 : NULL) {
        nl = strchr(line, '\n');
        if (nl) *nl = 0;
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = 0;
            esp_http_client_set_header(c, line, colon + 1 + (colon[1] == ' '));
        }
        if (!nl) break;
    }
    int blen = j->body ? (int)strlen(j->body) : 0;
    j->status = -1;
    if (esp_http_client_open(c, blen) == ESP_OK) {
        if (blen) esp_http_client_write(c, j->body, blen);
        esp_http_client_fetch_headers(c);
        j->status = esp_http_client_get_status_code(c);
        size_t cap = 4096;
        j->resp = malloc(cap);
        int r;
        while (j->resp && (r = esp_http_client_read(c, j->resp + j->len, (int)(cap - j->len - 1))) > 0) {
            j->len += (size_t)r;
            if (cap - j->len < 1024) { cap *= 2; j->resp = realloc(j->resp, cap); }
        }
        if (j->resp) j->resp[j->len] = 0;
    }
    esp_http_client_cleanup(c);
    platform_post(http_deliver, j);
}

static void http_task(void *arg)
{
    for (;;) {
        http_job_t *j;
        if (xQueueReceive(s_http_q, &j, portMAX_DELAY) == pdTRUE) http_run(j);
    }
}

int hal_http(const char *method, const char *url, const char *headers, const char *body, hal_http_cb cb, void *ctx)
{
    http_job_t *j = calloc(1, sizeof *j);
    if (!j) return -1;
    j->method = sdup(method);
    j->url = sdup(url);
    j->headers = sdup(headers);
    j->body = sdup(body);
    j->cb = cb;
    j->ctx = ctx;
    if (xQueueSend(s_http_q, &j, 0) != pdTRUE) {
        free(j->method); free(j->url); free(j->headers); free(j->body); free(j);
        return -1;
    }
    return 0;
}

/* ---------------- WebSocket ---------------- */

#define MAX_WS 6
typedef struct {
    esp_websocket_client_handle_t c;
    hal_ws_msg_cb on_msg;
    hal_ws_state_cb on_state;
    void *ctx;
    char *frag;
    size_t frag_len;
} ws_t;
static ws_t s_ws[MAX_WS];

typedef struct { int h; char *msg; size_t len; int state; } ws_evt_t;

static void ws_deliver(void *arg)
{
    ws_evt_t *e = arg;
    ws_t *w = &s_ws[e->h];
    if (e->msg && w->on_msg) w->on_msg(w->ctx, e->msg, e->len);
    else if (e->state >= 0 && w->on_state) w->on_state(w->ctx, e->state);
    free(e->msg);
    free(e);
}

static void ws_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    int h = (int)(intptr_t)arg;
    ws_t *w = &s_ws[h];
    esp_websocket_event_data_t *d = data;
    ws_evt_t *e = calloc(1, sizeof *e);
    if (!e) return;
    e->h = h;
    e->state = -1;
    if (id == WEBSOCKET_EVENT_CONNECTED) e->state = 1;
    else if (id == WEBSOCKET_EVENT_DISCONNECTED || id == WEBSOCKET_EVENT_CLOSED) e->state = 0;
    else if (id == WEBSOCKET_EVENT_DATA && (d->op_code == 0x01 || d->op_code == 0x00)) {
        if (d->payload_offset == 0) { free(w->frag); w->frag = malloc((size_t)d->payload_len + 1); w->frag_len = 0; }
        if (w->frag) { memcpy(w->frag + w->frag_len, d->data_ptr, (size_t)d->data_len); w->frag_len += (size_t)d->data_len; }
        if (d->payload_offset + d->data_len < d->payload_len || !w->frag) { free(e); return; }
        w->frag[w->frag_len] = 0;
        e->msg = w->frag;
        e->len = w->frag_len;
        w->frag = NULL;
    } else {
        free(e);
        return;
    }
    platform_post(ws_deliver, e);
}

int hal_ws_open(const char *url, hal_ws_msg_cb on_msg, hal_ws_state_cb on_state, void *ctx)
{
    for (int i = 0; i < MAX_WS; i++) {
        if (s_ws[i].c) continue;
        esp_websocket_client_config_t cfg = {.uri = url, .crt_bundle_attach = esp_crt_bundle_attach, .reconnect_timeout_ms = 5000};
        s_ws[i].c = esp_websocket_client_init(&cfg);
        s_ws[i].on_msg = on_msg;
        s_ws[i].on_state = on_state;
        s_ws[i].ctx = ctx;
        esp_websocket_register_events(s_ws[i].c, WEBSOCKET_EVENT_ANY, ws_event, (void *)(intptr_t)i);
        esp_websocket_client_start(s_ws[i].c);
        return i;
    }
    return -1;
}

void hal_ws_send(int h, const char *text)
{
    if (h < 0 || h >= MAX_WS || !s_ws[h].c || !esp_websocket_client_is_connected(s_ws[h].c)) return;
    esp_websocket_client_send_text(s_ws[h].c, text, (int)strlen(text), pdMS_TO_TICKS(1000));
}

void hal_ws_close(int h)
{
    if (h < 0 || h >= MAX_WS || !s_ws[h].c) return;
    esp_websocket_client_destroy(s_ws[h].c);
    memset(&s_ws[h], 0, sizeof s_ws[h]);
}

/* ---------------- Wi-Fi + SNTP ----------------
 * The network is picked on the device (setup step 2) and saved in NVS. A menuconfig
 * network (Degen Walkie → Wi-Fi) is only the fallback for development builds. */

#define WIFI_RETRIES 5
static char s_ssid[33];
static volatile hal_wifi_state_t s_wifi = HAL_WIFI_OFF;
static volatile bool s_scanning;
static volatile int s_nap = -1;
static hal_wifi_ap_t s_aps[16];
static wifi_ap_record_t s_rec[24];   /* static: too big for the event task stack */
static int s_retries;
static esp_timer_handle_t s_retry_t;

static void wifi_retry(void *arg)
{
    (void)arg;
    if (*s_ssid && !s_net_up && !s_scanning) { s_retries = 0; s_wifi = HAL_WIFI_CONNECTING; esp_wifi_connect(); }
}

static void scan_done(void)
{
    uint16_t n = sizeof s_rec / sizeof s_rec[0];
    if (esp_wifi_scan_get_ap_records(&n, s_rec) != ESP_OK) n = 0;
    int k = 0;
    for (int i = 0; i < n && k < (int)(sizeof s_aps / sizeof s_aps[0]); i++) {   /* records come strongest first */
        const char *id = (const char *)s_rec[i].ssid;
        if (!*id) continue;
        bool dup = false;
        for (int j = 0; j < k && !dup; j++) dup = !strcmp(s_aps[j].ssid, id);
        if (dup) continue;
        strlcpy(s_aps[k].ssid, id, sizeof s_aps[k].ssid);
        s_aps[k].rssi = s_rec[i].rssi;
        s_aps[k].secure = s_rec[i].authmode != WIFI_AUTH_OPEN;
        k++;
    }
    s_nap = k;
    s_scanning = false;
    wifi_retry(NULL);
}

static void on_wifi(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        wifi_retry(NULL);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_net_up = false;
        if (s_scanning || !*s_ssid) return;
        if (++s_retries <= WIFI_RETRIES) { s_wifi = HAL_WIFI_CONNECTING; esp_wifi_connect(); }
        else { s_wifi = HAL_WIFI_FAILED; esp_timer_start_once(s_retry_t, 30 * 1000000); }
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE) {
        scan_done();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_net_up = true;
        s_retries = 0;
        s_wifi = HAL_WIFI_UP;
        ESP_LOGI(TAG, "online on %s", s_ssid);
    }
}

void hal_wifi_ssid(char *out, size_t cap) { snprintf(out, cap, "%s", s_net_up ? s_ssid : ""); }
void hal_wifi_target(char *out, size_t cap) { snprintf(out, cap, "%s", s_ssid); }
hal_wifi_state_t hal_wifi_state(void) { return s_wifi; }

void hal_wifi_scan_start(void)
{
    if (s_scanning) return;
    s_nap = -1;
    s_scanning = true;
    if (!s_net_up) esp_wifi_disconnect();   /* a connect attempt in flight blocks the scan */
    if (esp_wifi_scan_start(NULL, false) != ESP_OK) { s_nap = 0; s_scanning = false; }
}

int hal_wifi_scan_results(hal_wifi_ap_t *out, int max)
{
    if (s_nap < 0) return -1;
    int n = s_nap < max ? s_nap : max;
    memcpy(out, s_aps, n * sizeof *out);
    return n;
}

void hal_wifi_join(const char *ssid, const char *password)
{
    strlcpy(s_ssid, ssid, sizeof s_ssid);
    hal_store_set("wifi.ssid", ssid);
    hal_store_set("wifi.pass", password);
    wifi_config_t wc = {0};
    strlcpy((char *)wc.sta.ssid, ssid, sizeof wc.sta.ssid);
    strlcpy((char *)wc.sta.password, password, sizeof wc.sta.password);
    esp_timer_stop(s_retry_t);
    s_retries = 0;
    s_wifi = HAL_WIFI_CONNECTING;
    esp_wifi_disconnect();
    esp_wifi_set_config(WIFI_IF_STA, &wc);
    esp_wifi_connect();
}

static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t ic = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&ic));
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_wifi, NULL);
    esp_timer_create_args_t r = {.callback = wifi_retry, .name = "wifi"};
    esp_timer_create(&r, &s_retry_t);
    char pass[65] = "";
    if (hal_store_get("wifi.ssid", s_ssid, sizeof s_ssid) <= 0) {
        strlcpy(s_ssid, CONFIG_DW_WIFI_SSID, sizeof s_ssid);
        strlcpy(pass, CONFIG_DW_WIFI_PASSWORD, sizeof pass);
    } else {
        hal_store_get("wifi.pass", pass, sizeof pass);
    }
    wifi_config_t wc = {0};
    strlcpy((char *)wc.sta.ssid, s_ssid, sizeof wc.sta.ssid);
    strlcpy((char *)wc.sta.password, pass, sizeof wc.sta.password);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wc);
    esp_wifi_start();
    esp_sntp_config_t sc = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&sc);
}

void platform_init(void)
{
    esp_err_t e = nvs_flash_init();
    if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_open("dw", NVS_READWRITE, &s_nvs));
    esp_timer_create_args_t a = {.callback = led_tick, .name = "led"};
    esp_timer_create(&a, &s_led_t);
    esp_timer_create_args_t v = {.callback = vibe_off, .name = "vibe"};
    esp_timer_create(&v, &s_vibe_t);
    s_http_q = xQueueCreate(16, sizeof(http_job_t *));
    xTaskCreatePinnedToCore(http_task, "http", 8 * 1024, NULL, 4, NULL, 0);
    wifi_init();
}
