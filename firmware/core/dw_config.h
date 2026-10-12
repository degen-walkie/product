/* Network mode and endpoints, chosen at runtime.
 *   demo     mainnet hostnames; the simulator answers them with in-page mocks
 *   testnet  Hyperliquid and Pacifica testnets (Lighter locked)
 *   mainnet  real money (Lighter locked)
 * The mode lives in storage ("net.mode"); the relay base URL in "relay.url". */
#pragma once
#include <stdbool.h>

typedef enum { DW_NET_DEMO, DW_NET_TESTNET, DW_NET_MAINNET } dw_net_t;
typedef enum { U_HL, U_HL_WS, U_PAC, U_PAC_WS, U_LIT, U_LIT_WS, U_RELAY, U_RELAY_WS, U_LINK } dw_url_t;

void dw_config_load(void);
dw_net_t dw_net(void);
const char *dw_net_name(void);            /* "DEMO", "TESTNET", "MAINNET" */
const char *dw_url(dw_url_t u);           /* base URL, no trailing slash */
/* base + formatted path, in one of 8 rotating buffers (use it right away) */
const char *dw_urlf(dw_url_t u, const char *fmt, ...);
bool dw_venue_enabled(int venue);         /* false: shown but locked */
/* "&r=<relay>" for the link QR when the relay is not the default one, else "". */
const char *dw_relay_param(void);
