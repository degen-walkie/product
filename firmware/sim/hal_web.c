/* Browser implementation of dw_hal.h. Each call forwards to Module.dwHost, which the
 * simulator page provides (sim/web/host.js). Network calls go to whatever endpoint
 * the URL names; the simulator routes the venue and relay hosts to in-page mocks. */
#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dw_hal.h"

#define MAX_REQ 64
#define MAX_WS 8

static struct { hal_http_cb cb; void *ctx; } s_req[MAX_REQ];
static struct { hal_ws_msg_cb msg; hal_ws_state_cb state; void *ctx; bool used; } s_ws[MAX_WS];

EM_JS(void, js_led, (int c, int m), { Module.dwHost.led(c, m); });
EM_JS(void, js_vibrate, (int ms), { Module.dwHost.vibrate(ms); });
EM_JS(void, js_backlight, (int on), { Module.dwHost.backlight(on); });
EM_JS(int, js_power, (void), { return Module.dwHost.power(); });
EM_JS(void, js_http, (int id, const char *m, const char *u, const char *h, const char *b), {
    Module.dwHost.http(id, UTF8ToString(m), UTF8ToString(u), h ? UTF8ToString(h) : "", b ? UTF8ToString(b) : null);
});
EM_JS(void, js_ws_open, (int id, const char *u), { Module.dwHost.wsOpen(id, UTF8ToString(u)); });
EM_JS(void, js_ws_send, (int id, const char *t), { Module.dwHost.wsSend(id, UTF8ToString(t)); });
EM_JS(void, js_ws_close, (int id), { Module.dwHost.wsClose(id); });
EM_JS(char *, js_store_get, (const char *k), {
    const v = Module.dwHost.storeGet(UTF8ToString(k));
    return v === null || v === undefined ? 0 : stringToNewUTF8(v);
});
EM_JS(void, js_store_set, (const char *k, const char *v), {
    Module.dwHost.storeSet(UTF8ToString(k), v ? UTF8ToString(v) : null);
});
EM_JS(void, js_voice, (int on), { Module.dwHost.voice(on); });
EM_JS(void, js_qr, (const char *p), { Module.dwHost.qr(UTF8ToString(p)); });

void hal_led(dw_led_color_t c, dw_led_mode_t m) { js_led(c, m); }
void hal_vibrate(uint32_t ms) { js_vibrate((int)ms); }
void hal_backlight(bool on) { js_backlight(on); }
uint32_t hal_millis(void) { return (uint32_t)emscripten_get_now(); }
int64_t hal_unix_ms(void) { return (int64_t)emscripten_date_now(); }
bool hal_net_up(void) { return true; }

void hal_random_fill(uint8_t *buf, size_t n)
{
    while (n) {
        size_t k = n > 256 ? 256 : n;
        if (getentropy(buf, k) != 0) abort();
        buf += k;
        n -= k;
    }
}

void hal_power(hal_power_t *out)
{
    int v = js_power();
    out->percent = v & 0xff;
    out->charging = (v >> 8) & 1;
    out->usb = (v >> 9) & 1;
}

int hal_http(const char *method, const char *url, const char *headers, const char *body, hal_http_cb cb, void *ctx)
{
    for (int i = 0; i < MAX_REQ; i++) {
        if (s_req[i].cb) continue;
        s_req[i].cb = cb;
        s_req[i].ctx = ctx;
        js_http(i, method, url, headers, body);
        return i;
    }
    return -1;
}

EMSCRIPTEN_KEEPALIVE void dw_sim_http_done(int id, int status, char *body, int len)
{
    if (id < 0 || id >= MAX_REQ || !s_req[id].cb) { free(body); return; }
    hal_http_cb cb = s_req[id].cb;
    void *ctx = s_req[id].ctx;
    s_req[id].cb = NULL;
    cb(ctx, status, body, body ? (size_t)len : 0);
    free(body);
}

int hal_ws_open(const char *url, hal_ws_msg_cb on_msg, hal_ws_state_cb on_state, void *ctx)
{
    for (int i = 0; i < MAX_WS; i++) {
        if (s_ws[i].used) continue;
        s_ws[i].used = true;
        s_ws[i].msg = on_msg;
        s_ws[i].state = on_state;
        s_ws[i].ctx = ctx;
        js_ws_open(i, url);
        return i;
    }
    return -1;
}

void hal_ws_send(int h, const char *text) { if (h >= 0 && h < MAX_WS && s_ws[h].used) js_ws_send(h, text); }

void hal_ws_close(int h)
{
    if (h < 0 || h >= MAX_WS || !s_ws[h].used) return;
    js_ws_close(h);
    s_ws[h].used = false;
}

EMSCRIPTEN_KEEPALIVE void dw_sim_ws_msg(int h, char *msg, int len)
{
    if (h >= 0 && h < MAX_WS && s_ws[h].used && s_ws[h].msg) s_ws[h].msg(s_ws[h].ctx, msg, (size_t)len);
    free(msg);
}

EMSCRIPTEN_KEEPALIVE void dw_sim_ws_state(int h, int open)
{
    if (h >= 0 && h < MAX_WS && s_ws[h].used && s_ws[h].state) s_ws[h].state(s_ws[h].ctx, open);
}

int hal_store_get(const char *key, char *buf, size_t cap)
{
    char *v = js_store_get(key);
    if (!v) return -1;
    size_t n = strlen(v);
    if (n >= cap) n = cap - 1;
    memcpy(buf, v, n);
    buf[n] = 0;
    free(v);
    return (int)n;
}

int hal_store_set(const char *key, const char *val)
{
    js_store_set(key, val);
    return 0;
}

void hal_voice_start(void) { js_voice(1); }
void hal_voice_stop(void) { js_voice(0); }
void hal_qr_shown(const char *payload) { js_qr(payload); }

EMSCRIPTEN_KEEPALIVE void dw_sim_voice(char *text)
{
    dw_app_voice_result(text && *text ? text : NULL);
    free(text);
}
