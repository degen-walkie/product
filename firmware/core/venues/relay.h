/* Degen Walkie relay: live rooms, trading news, the AI and the account-linking
 * handshake. The relay never holds keys or funds; for the AI the device sends only
 * the position summary needed to answer. */
#pragma once
#include "dw_model.h"

void dw_relay_start(void);
void dw_relay_poll(void);
void dw_relay_join(const char *room_id);
void dw_relay_say(const char *text);

typedef void (*dw_text_cb)(void *ctx, const char *text);
int dw_relay_ask(const char *question, dw_text_cb cb, void *ctx);

/* status: 0 pending, 1 done, -1 error */
typedef void (*dw_link_cb)(void *ctx, int status, const char *venue, const char *account, const char *extra);
int dw_relay_link_poll(const char *session, dw_link_cb cb, void *ctx);
