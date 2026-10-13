/* Voice: hold to talk -> ES8388 capture -> WAV -> the relay's speech-to-text.
 *
 * The MEMS mic sits on the codec's LIN1 (RIN1 carries the speaker echo reference).
 * The codec is an I2S slave; the ESP32-S3 drives MCLK/BCLK/LRCK. While talk is
 * held, a task copies the left channel into a PSRAM buffer. On release the clip
 * goes to POST <relay>/stt as audio/wav and the text comes back through
 * dw_app_voice_result() on the UI thread. */
#include "audio.h"

#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "cJSON.h"
#include "driver/i2s_std.h"
#include "dw_config.h"
#include "dw_hal.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "platform.h"

static const char *TAG = "audio";

#define PIN_I2S_MCLK 5
#define PIN_I2S_BCLK 33
#define PIN_I2S_LRCK 7
#define PIN_I2S_DIN 2     /* codec ASDOUT */
#define PIN_I2S_DOUT 4    /* codec DSDIN (playback, unused for now) */
#define ES8388_ADDR 0x10

#define RATE 16000
#define MAX_SECONDS 15
#define MAX_SAMPLES (RATE * MAX_SECONDS)
#define SKIP_SAMPLES (RATE / 10)    /* the first 100 ms after power-up is a thump */
#define MIN_SAMPLES (RATE * 3 / 10) /* shorter than 300 ms: treat as nothing heard */

static i2c_master_dev_handle_t s_codec;
static i2s_chan_handle_t s_rx;
static TaskHandle_t s_task;
static volatile bool s_rec;
static bool s_ok;

/* ---------------- ES8388 ---------------- */

static esp_err_t cw(uint8_t reg, uint8_t v)
{
    uint8_t b[2] = {reg, v};
    return i2c_master_transmit(s_codec, b, 2, 50);
}

static bool codec_init(void)
{
    static const uint8_t INIT[][2] = {
        {0x19, 0x04},  /* DACCONTROL3: DAC muted */
        {0x01, 0x50},  /* CONTROL2 */
        {0x02, 0x00},  /* CHIPPOWER: all blocks on */
        {0x35, 0xA0},  /* internal DLL off */
        {0x37, 0xD0},
        {0x39, 0xD0},
        {0x08, 0x00},  /* MASTERMODE: codec is the I2S slave */
        {0x04, 0xC0},  /* DACPOWER: DACs and outputs off */
        {0x00, 0x12},  /* CONTROL1: play & record, VMID 500k (as ESP-ADF) */
        {0x2B, 0x80},  /* DACCONTROL21: ADC and DAC share the one LRCK pin */
        {0x03, 0xFF},  /* ADCPOWER: off until talk is held */
        {0x09, 0x88},  /* ADCCONTROL1: mic PGA +24 dB, both channels */
        {0x0A, 0x00},  /* ADCCONTROL2: LIN1 / RIN1 */
        {0x0B, 0x02},  /* ADCCONTROL3 */
        {0x0C, 0x0C},  /* ADCCONTROL4: I2S, 16-bit */
        {0x0D, 0x02},  /* ADCCONTROL5: MCLK / LRCK = 256 */
        {0x10, 0x00},  /* ADC digital volume 0 dB */
        {0x11, 0x00},
    };
    for (size_t i = 0; i < sizeof INIT / sizeof INIT[0]; i++)
        if (cw(INIT[i][0], INIT[i][1]) != ESP_OK) return false;
    return true;
}

static void codec_adc(bool on)
{
    cw(0x03, on ? 0x00 : 0xFF);   /* ADCPOWER */
    if (on) { cw(0x02, 0xF0); cw(0x02, 0x00); }   /* restart the codec state machine */
}

/* ---------------- capture + upload ---------------- */

static void deliver(void *arg)
{
    dw_app_voice_result(arg);   /* NULL = nothing heard */
    free(arg);
}

/* 8-bit mu-law WAV: half the bytes of 16-bit PCM, plenty for speech-to-text. */
static void wav_header(uint8_t *h, uint32_t samples)
{
    uint32_t data = samples, riff = 36 + data, rate = RATE, byterate = RATE, fmt = 16;
    uint16_t mulaw = 7, ch = 1, align = 1, bits = 8;
    memcpy(h, "RIFF", 4); memcpy(h + 4, &riff, 4); memcpy(h + 8, "WAVEfmt ", 8);
    memcpy(h + 16, &fmt, 4); memcpy(h + 20, &mulaw, 2); memcpy(h + 22, &ch, 2); memcpy(h + 24, &rate, 4);
    memcpy(h + 28, &byterate, 4); memcpy(h + 32, &align, 2); memcpy(h + 34, &bits, 2);
    memcpy(h + 36, "data", 4); memcpy(h + 40, &data, 4);
}

static uint8_t mulaw(int16_t v)
{
    int x = v, sign = 0;
    if (x < 0) { sign = 0x80; x = -x; }
    if (x > 32635) x = 32635;
    x += 0x84;
    int exp = 7;
    for (int mask = 0x4000; exp > 0 && !(x & mask); mask >>= 1) exp--;
    return (uint8_t)~(sign | (exp << 4) | ((x >> (exp + 3)) & 0x0F));
}

/* Bring speech up to a usable level: the analog mic is quiet at arm's length. */
static void normalize(int16_t *s, size_t n)
{
    int peak = 1;
    for (size_t i = 0; i < n; i++) { int a = abs(s[i]); if (a > peak) peak = a; }
    int gain = 24000 / peak;
    ESP_LOGI(TAG, "peak %d, gain x%d", peak, gain > 16 ? 16 : gain);
    if (gain < 1) return;
    if (gain > 16) gain = 16;
    for (size_t i = 0; i < n; i++) s[i] = (int16_t)(s[i] * gain);
}

/* clip = 44-byte WAV header + n 16-bit samples, contiguous (in PSRAM). Encoded to
 * mu-law in place, then sent in 4 KB pieces through an internal-RAM buffer. */
static char *transcribe(uint8_t *clip, size_t n)
{
    char lang[8] = "EN", url[200];
    hal_store_get("lang", lang, sizeof lang);
    for (char *p = lang; *p; p++) if (*p >= 'A' && *p <= 'Z') *p += 32;
    snprintf(url, sizeof url, "%s/stt?lang=%s", dw_url(U_RELAY), lang);

    const int16_t *pcm = (const int16_t *)(clip + 44);
    for (size_t i = 0; i < n; i++) clip[44 + i] = mulaw(pcm[i]);   /* byte i is written after sample i is read */
    size_t total = 44 + n, sent = 0;
    wav_header(clip, (uint32_t)n);
    static char chunk[4096], resp[1024];
    esp_http_client_config_t cfg = {.url = url, .method = HTTP_METHOD_POST, .crt_bundle_attach = esp_crt_bundle_attach, .timeout_ms = 30000};
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    esp_http_client_set_header(c, "Content-Type", "audio/wav");
    char *text = NULL;
    int status = -1, len = 0;
    esp_err_t err = esp_http_client_open(c, (int)total);
    if (err == ESP_OK) {
        while (sent < total) {
            size_t k = total - sent > sizeof chunk ? sizeof chunk : total - sent;
            memcpy(chunk, clip + sent, k);
            int w = esp_http_client_write(c, chunk, (int)k);
            if (w <= 0) { ESP_LOGW(TAG, "stt write failed at %u/%u (%d)", (unsigned)sent, (unsigned)total, w); break; }
            sent += (size_t)w;
        }
        if (sent == total && esp_http_client_fetch_headers(c) >= 0) {
            status = esp_http_client_get_status_code(c);
            int r;
            while (len < (int)sizeof resp - 1 && (r = esp_http_client_read(c, resp + len, (int)sizeof resp - 1 - len)) > 0) len += r;
        }
    }
    resp[len] = 0;
    esp_http_client_cleanup(c);
    cJSON *j = status == 200 ? cJSON_Parse(resp) : NULL;
    const cJSON *t = cJSON_GetObjectItem(j, "text");
    if (cJSON_IsString(t) && t->valuestring[strspn(t->valuestring, " .")]) text = strdup(t->valuestring);
    cJSON_Delete(j);
    ESP_LOGI(TAG, "stt %s, sent %u/%u, status %d: %s", esp_err_to_name(err), (unsigned)sent, (unsigned)total, status, text ? text : resp);
    return text;
}

static void voice_task(void *arg)
{
    int16_t frame[512];   /* 256 stereo samples */
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint8_t *clip = heap_caps_malloc(44 + MAX_SAMPLES * 2, MALLOC_CAP_SPIRAM);
        int16_t *buf = clip ? (int16_t *)(clip + 44) : NULL;
        size_t n = 0, skip = SKIP_SAMPLES;
        int pl = 0, pr = 0;
        if (buf) {
            codec_adc(true);
            i2s_channel_enable(s_rx);
            while (s_rec && n < MAX_SAMPLES) {
                size_t got = 0;
                if (i2s_channel_read(s_rx, frame, sizeof frame, &got, 100) != ESP_OK) continue;
                for (size_t i = 0; i + 1 < got / 2 && n < MAX_SAMPLES; i += 2) {
                    if (skip) { skip--; continue; }
                    if (abs(frame[i]) > pl) pl = abs(frame[i]);
                    if (abs(frame[i + 1]) > pr) pr = abs(frame[i + 1]);
                    buf[n++] = frame[i];   /* left = LIN1 = the mic */
                }
            }
            i2s_channel_disable(s_rx);
            codec_adc(false);
        }
        while (s_rec) vTaskDelay(pdMS_TO_TICKS(20));   /* hit the 15 s cap: wait for release */
        char *text = NULL;
        ESP_LOGI(TAG, "recorded %u ms, peak L %d R %d, internal heap %u", (unsigned)(n * 1000 / RATE), pl, pr,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        if (buf && n >= MIN_SAMPLES) {
            normalize(buf, n);
            text = transcribe(clip, n);
        }
        free(clip);
        platform_post(deliver, text);
    }
}

void audio_init(void)
{
    i2c_device_config_t d = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = ES8388_ADDR, .scl_speed_hz = 100000};
    if (i2c_master_probe(g_i2c, ES8388_ADDR, 50) != ESP_OK || i2c_master_bus_add_device(g_i2c, &d, &s_codec) != ESP_OK) {
        ESP_LOGW(TAG, "ES8388 not found at 0x%02x: voice disabled", ES8388_ADDR);
        return;
    }
    i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    if (i2s_new_channel(&cc, NULL, &s_rx) != ESP_OK) return;
    i2s_std_config_t sc = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {.mclk = PIN_I2S_MCLK, .bclk = PIN_I2S_BCLK, .ws = PIN_I2S_LRCK, .dout = I2S_GPIO_UNUSED, .din = PIN_I2S_DIN},
    };
    sc.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    if (i2s_channel_init_std_mode(s_rx, &sc) != ESP_OK) return;
    if (!codec_init()) { ESP_LOGW(TAG, "ES8388 init failed: voice disabled"); return; }
    xTaskCreatePinnedToCore(voice_task, "voice", 8192, NULL, 5, &s_task, 1);
    s_ok = true;
    ESP_LOGI(TAG, "voice ready");
}

void audio_start(void)
{
    if (!s_ok || s_rec) return;
    s_rec = true;
    xTaskNotifyGive(s_task);
}

bool audio_stop(void)
{
    if (!s_ok || !s_rec) return false;
    s_rec = false;   /* the task uploads and posts the result */
    return true;
}
