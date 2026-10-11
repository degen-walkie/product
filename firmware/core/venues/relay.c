#include "relay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dw_hal.h"
#include "dw_venue.h"

static int s_ws = -1;
static bool s_ws_open;
static uint32_t s_rooms_at;

static int venue_from(const char *s)
{
    if (!strcmp(s, "hl")) return V_HL;
    if (!strcmp(s, "pac")) return V_PAC;
    if (!strcmp(s, "lit")) return V_LIT;
    return -1;
}

static void push_msg(const cJSON *m)
{
    if (g_model.nmsgs == DW_MAX_MSG) {
        memmove(g_model.msgs, g_model.msgs + 1, sizeof(dw_msg_t) * (DW_MAX_MSG - 1));
        g_model.nmsgs--;
    }
    dw_msg_t *x = &g_model.msgs[g_model.nmsgs++];
    snprintf(x->who, sizeof x->who, "%s", dw_jstr(m, "who"));
    snprintf(x->lang, sizeof x->lang, "%s", dw_jstr(m, "lang"));
    snprintf(x->text, sizeof x->text, "%s", dw_jstr(m, "text"));
    x->me = cJSON_IsTrue(cJSON_GetObjectItem(m, "me"));
}

static void push_news(const cJSON *n, bool front)
{
    if (front) {
        int k = g_model.nnews < DW_MAX_NEWS ? g_model.nnews : DW_MAX_NEWS - 1;
        memmove(g_model.news + 1, g_model.news, sizeof(dw_news_t) * k);
        g_model.nnews = k + 1;
    } else if (g_model.nnews == DW_MAX_NEWS) {
        return;
    }
    dw_news_t *x = &g_model.news[front ? 0 : g_model.nnews++];
    snprintf(x->tag, sizeof x->tag, "%s", dw_jstr(n, "tag"));
    snprintf(x->text, sizeof x->text, "%s", dw_jstr(n, "text"));
    x->ts = (int64_t)dw_jnum(n, "ts");
}

static void on_rooms(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    cJSON *arr = cJSON_GetObjectItem(root, "rooms");
    if (status != 200 || !cJSON_IsArray(arr)) return;
    int n = 0;
    cJSON *it;
    cJSON_ArrayForEach(it, arr) {
        if (n >= DW_MAX_ROOMS) break;
        dw_room_t *r = &g_model.rooms[n++];
        snprintf(r->id, sizeof r->id, "%s", dw_jstr(it, "id"));
        snprintf(r->name, sizeof r->name, "%s", dw_jstr(it, "name"));
        snprintf(r->coin, sizeof r->coin, "%s", dw_jstr(it, "coin"));
        r->venue = venue_from(dw_jstr(it, "venue"));
        snprintf(r->venue_name, sizeof r->venue_name, "%s", r->venue >= 0 ? dw_venue_name(r->venue) : "Private");
        r->listening = (int)dw_jnum(it, "listening");
    }
    g_model.nrooms = n;
    dw_model_touch();
}

static void on_news(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    cJSON *arr = cJSON_GetObjectItem(root, "items");
    if (status != 200 || !cJSON_IsArray(arr)) return;
    g_model.nnews = 0;
    cJSON *it;
    cJSON_ArrayForEach(it, arr) push_news(it, false);
    dw_model_touch();
}

static void on_history(void *ctx, int status, cJSON *root)
{
    (void)ctx;
    cJSON *arr = cJSON_GetObjectItem(root, "messages");
    if (status != 200 || !cJSON_IsArray(arr)) return;
    g_model.nmsgs = 0;
    cJSON *it;
    cJSON_ArrayForEach(it, arr) push_msg(it);
    dw_model_touch();
}

static void send_join(void)
{
    if (!s_ws_open || !g_model.room_id[0]) return;
    char buf[96];
    snprintf(buf, sizeof buf, "{\"op\":\"join\",\"room\":\"%s\"}", g_model.room_id);
    hal_ws_send(s_ws, buf);
}

static void on_ws(void *ctx, const char *msg, size_t len)
{
    (void)ctx;
    cJSON *root = cJSON_ParseWithLength(msg, len);
    if (!root) return;
    const char *type = dw_jstr(root, "type");
    if (!strcmp(type, "msg") && !strcmp(dw_jstr(root, "room"), g_model.room_id)) {
        push_msg(root);
        dw_model_touch();
    } else if (!strcmp(type, "news")) {
        push_news(cJSON_GetObjectItem(root, "item"), true);
        dw_model_touch();
    } else if (!strcmp(type, "listening")) {
        for (int i = 0; i < g_model.nrooms; i++)
            if (!strcmp(g_model.rooms[i].id, dw_jstr(root, "room"))) g_model.rooms[i].listening = (int)dw_jnum(root, "n");
        dw_model_touch();
    }
    cJSON_Delete(root);
}

static void on_ws_state(void *ctx, int open)
{
    (void)ctx;
    s_ws_open = open;
    if (open) send_join();
}

void dw_relay_start(void)
{
    if (s_ws < 0) s_ws = hal_ws_open(DW_RELAY_WS, on_ws, on_ws_state, NULL);
    s_rooms_at = 0;
    dw_relay_poll();
    dw_http_json("GET", DW_RELAY_API "/news", NULL, NULL, on_news, NULL);
}

void dw_relay_poll(void)
{
    if (!s_rooms_at || hal_millis() - s_rooms_at > 20000) {
        s_rooms_at = hal_millis();
        dw_http_json("GET", DW_RELAY_API "/rooms", NULL, NULL, on_rooms, NULL);
    }
}

void dw_relay_join(const char *room_id)
{
    if (!strcmp(g_model.room_id, room_id) && g_model.nmsgs) return;
    snprintf(g_model.room_id, sizeof g_model.room_id, "%s", room_id);
    g_model.nmsgs = 0;
    char url[160];
    snprintf(url, sizeof url, DW_RELAY_API "/rooms/%s/history", room_id);
    dw_http_json("GET", url, NULL, NULL, on_history, NULL);
    send_join();
}

void dw_relay_say(const char *text)
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "op", "say");
    cJSON_AddStringToObject(o, "room", g_model.room_id);
    cJSON_AddStringToObject(o, "text", text);
    cJSON_AddStringToObject(o, "lang", "EN");
    char *s = cJSON_PrintUnformatted(o);
    if (s_ws_open && s) hal_ws_send(s_ws, s);
    cJSON_free(s);
    cJSON_Delete(o);
}

/* ---------- AI ---------- */

typedef struct { dw_text_cb cb; void *ctx; } ask_ctx_t;

static void on_answer(void *ctx, int status, cJSON *root)
{
    ask_ctx_t *a = ctx;
    const char *ans = status == 200 ? dw_jstr(root, "answer") : "";
    a->cb(a->ctx, *ans ? ans : "The relay is not answering. Try again in a moment.");
    free(a);
}

int dw_relay_ask(const char *question, dw_text_cb cb, void *ctx)
{
    ask_ctx_t *a = malloc(sizeof *a);
    if (!a) return -1;
    a->cb = cb;
    a->ctx = ctx;
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "q", question);
    cJSON_AddStringToObject(o, "lang", "en");
    cJSON *pos = cJSON_AddArrayToObject(o, "positions");
    for (int i = 0; i < g_model.npos; i++) {
        dw_pos_t *p = &g_model.pos[i];
        cJSON *x = cJSON_CreateObject();
        cJSON_AddStringToObject(x, "venue", dw_venue_name(p->venue));
        cJSON_AddStringToObject(x, "coin", p->coin);
        cJSON_AddNumberToObject(x, "size", p->szi);
        cJSON_AddNumberToObject(x, "entry", p->entry);
        cJSON_AddNumberToObject(x, "liq", p->liq);
        cJSON_AddNumberToObject(x, "lev", p->lev);
        cJSON_AddNumberToObject(x, "upnl", p->upnl);
        cJSON_AddNumberToObject(x, "mark", dw_price(p->venue, p->coin));
        cJSON *m = cJSON_AddObjectToObject(x, "market");
        dw_mkt_t *mk = dw_mkt_find(p->venue, p->coin);
        cJSON_AddNumberToObject(m, "funding_h", mk ? mk->funding_h : 0);
        cJSON_AddItemToArray(pos, x);
    }
    double eq = 0;
    for (int v = 0; v < V_COUNT; v++) eq += g_model.acct[v].equity;
    cJSON_AddNumberToObject(o, "equity", eq);
    char *body = cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    int rc = dw_http_json("POST", DW_RELAY_API "/ask", NULL, body, on_answer, a);
    cJSON_free(body);
    if (rc < 0) free(a);
    return rc;
}

/* ---------- linking ---------- */

typedef struct { dw_link_cb cb; void *ctx; } link_ctx_t;

static void on_link(void *ctx, int status, cJSON *root)
{
    link_ctx_t *l = ctx;
    if (status != 200 || !root) l->cb(l->ctx, -1, "", "", "");
    else if (!strcmp(dw_jstr(root, "status"), "done"))
        l->cb(l->ctx, 1, dw_jstr(root, "venue"), dw_jstr(root, "account"), dw_jstr(root, "extra"));
    else
        l->cb(l->ctx, 0, "", "", "");
    free(l);
}

int dw_relay_link_poll(const char *session, dw_link_cb cb, void *ctx)
{
    link_ctx_t *l = malloc(sizeof *l);
    if (!l) return -1;
    l->cb = cb;
    l->ctx = ctx;
    char url[160];
    snprintf(url, sizeof url, DW_RELAY_API "/link/%s", session);
    int rc = dw_http_json("GET", url, NULL, NULL, on_link, l);
    if (rc < 0) free(l);
    return rc;
}
