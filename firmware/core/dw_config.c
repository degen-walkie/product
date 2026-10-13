#include "dw_config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "dw_hal.h"
#include "dw_model.h"

#define DEFAULT_RELAY "https://relay-demo.degenwalkie.trade/v1"

static dw_net_t s_net = DW_NET_MAINNET;
static char s_relay[128] = DEFAULT_RELAY;
static char s_relay_ws[140];
static char s_link[128] = "https://degenwalkie.trade/link";

static const char *URLS[3][6] = {
    /* HL api, HL ws, PAC api, PAC ws, LIT api, LIT ws */
    {"https://api.hyperliquid.xyz", "wss://api.hyperliquid.xyz/ws", "https://api.pacifica.fi/api/v1", "wss://ws.pacifica.fi/ws",
     "https://mainnet.zklighter.elliot.ai/api/v1", "wss://mainnet.zklighter.elliot.ai/stream"},
    {"https://api.hyperliquid-testnet.xyz", "wss://api.hyperliquid-testnet.xyz/ws", "https://test-api.pacifica.fi/api/v1",
     "wss://test-ws.pacifica.fi/ws", "https://testnet.zklighter.elliot.ai/api/v1", "wss://testnet.zklighter.elliot.ai/stream"},
    {"https://api.hyperliquid.xyz", "wss://api.hyperliquid.xyz/ws", "https://api.pacifica.fi/api/v1", "wss://ws.pacifica.fi/ws",
     "https://mainnet.zklighter.elliot.ai/api/v1", "wss://mainnet.zklighter.elliot.ai/stream"},
};

void dw_config_load(void)
{
    char buf[128];
    s_net = DW_NET_MAINNET;   /* a device with nothing stored trades on mainnet */
    if (hal_store_get("net.mode", buf, sizeof buf) > 0) {
        if (!strcmp(buf, "testnet")) s_net = DW_NET_TESTNET;
        else if (!strcmp(buf, "demo")) s_net = DW_NET_DEMO;
    }
    if (hal_store_get("relay.url", buf, sizeof buf) > 0) snprintf(s_relay, sizeof s_relay, "%s", buf);
    if (hal_store_get("link.url", buf, sizeof buf) > 0) snprintf(s_link, sizeof s_link, "%s", buf);
    /* ws(s)://host/v1/ws from http(s)://host/v1 */
    const char *rest = strstr(s_relay, "://");
    snprintf(s_relay_ws, sizeof s_relay_ws, "%s%s/ws", !strncmp(s_relay, "https", 5) ? "wss" : "ws", rest ? rest : "");
}

dw_net_t dw_net(void) { return s_net; }

const char *dw_net_name(void)
{
    return s_net == DW_NET_TESTNET ? "TESTNET" : s_net == DW_NET_MAINNET ? "MAINNET" : "DEMO";
}

const char *dw_url(dw_url_t u)
{
    switch (u) {
    case U_RELAY: return s_relay;
    case U_RELAY_WS: return s_relay_ws;
    case U_LINK: return s_link;
    default: return URLS[s_net][u];
    }
}

const char *dw_urlf(dw_url_t u, const char *fmt, ...)
{
    static char bufs[8][320];
    static int i;
    char *out = bufs[i++ & 7];
    int n = snprintf(out, sizeof bufs[0], "%s", dw_url(u));
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(out + n, sizeof bufs[0] - (size_t)n, fmt, ap);
    va_end(ap);
    return out;
}

bool dw_venue_enabled(int venue) { return venue != V_LIT || s_net == DW_NET_DEMO; }

const char *dw_relay_param(void)
{
    static char out[200];
    static const char H[] = "0123456789ABCDEF";
    if (!strcmp(s_relay, DEFAULT_RELAY)) return "";
    size_t o = (size_t)snprintf(out, sizeof out, "&r=");
    for (const char *p = s_relay; *p && o + 4 < sizeof out; p++) {
        unsigned char c = (unsigned char)*p;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') out[o++] = (char)c;
        else { out[o++] = '%'; out[o++] = H[c >> 4]; out[o++] = H[c & 15]; }
    }
    out[o] = 0;
    return out;
}
