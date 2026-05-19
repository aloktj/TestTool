#include "websocket.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "auth.h"

static ws_type_t ws_get_type(struct mg_connection *c) {
  return (ws_type_t) (unsigned char) c->data[0];
}

static void ws_set_type(struct mg_connection *c, ws_type_t t) {
  c->data[0] = (char) t;
}

static const char *ws_type_to_string(ws_type_t t) {
  switch (t) {
    case WS_TYPE_LIVE:
      return "live";
    case WS_TYPE_ALARMS:
      return "alarms";
    case WS_TYPE_NONE:
    default:
      return "unknown";
  }
}

void websocket_handle_upgrade(struct mg_connection *c, struct mg_http_message *hm,
                              ws_type_t ws_type) {
  app_ctx_t *ctx = (app_ctx_t *) c->mgr->userdata;
  const user_t *u = NULL;

  if (ctx != NULL && !ctx->settings.websocket_enabled) {
    mg_http_reply(c, 503, "Content-Type: application/json\r\n",
                  "{\"error\":\"unavailable\",\"message\":\"WebSockets disabled\"}");
    return;
  }

  u = auth_require_ws_role(c, hm, ROLE_OPERATOR);
  if (u == NULL) return;

  ws_set_type(c, ws_type);
  mg_ws_upgrade(c, hm, NULL);
}

void websocket_on_open(struct mg_connection *c, struct mg_http_message *hm) {
  (void) hm;
  if (!c->is_websocket) return;
  MG_INFO(("WebSocket connected (type=%s id=%lu)", ws_type_to_string(ws_get_type(c)),
           c->id));
}

void websocket_on_message(struct mg_connection *c, struct mg_ws_message *wm) {
  (void) wm;
  if (!c->is_websocket) return;
  // For PoC, ignore incoming messages. Could be extended later.
}

void websocket_on_close(struct mg_connection *c) {
  if (!c->is_websocket && ws_get_type(c) == WS_TYPE_NONE) return;
  MG_INFO(("WebSocket disconnected (type=%s id=%lu)", ws_type_to_string(ws_get_type(c)),
           c->id));
}

static void broadcast_live(struct mg_connection *c, const demo_live_data_t *d) {
  char msg[256];
  int n = snprintf(msg, sizeof(msg),
                   "{"
                   "\"source\":\"live\","
                   "\"temperatureC\":%.1f,"
                   "\"humidityPercent\":%d,"
                   "\"pressureBar\":%.2f,"
                   "\"sequence\":%u"
                   "}",
                   d->temperature_c, d->humidity_percent, d->pressure_bar,
                   d->sequence);
  if (n > 0) mg_ws_send(c, msg, (size_t) n, WEBSOCKET_OP_TEXT);
}

static void broadcast_alarm(struct mg_connection *c, unsigned int seq) {
  char msg[256];
  int active = (seq % 5 == 0) ? 1 : 2;
  int id = (seq % 2 == 0) ? 1003 : 1001;
  const char *sev = (seq % 2 == 0) ? "INFO" : "WARNING";
  const char *text = (seq % 2 == 0) ? "Demo alarm event" : "High temperature warning";
  int n = snprintf(msg, sizeof(msg),
                   "{"
                   "\"source\":\"alarms\","
                   "\"activeCount\":%d,"
                   "\"latestAlarm\":{"
                   "\"id\":%d,"
                   "\"severity\":\"%s\","
                   "\"message\":\"%s\""
                   "}"
                   "}",
                   active, id, sev, text);
  if (n > 0) mg_ws_send(c, msg, (size_t) n, WEBSOCKET_OP_TEXT);
}

void websocket_broadcast_periodic(struct mg_mgr *mgr) {
  static uint64_t s_last_live_ms = 0;
  static uint64_t s_last_alarm_ms = 0;

  app_ctx_t *ctx = (app_ctx_t *) mgr->userdata;
  uint64_t now_ms = mg_millis();

  if (ctx == NULL || !ctx->settings.websocket_enabled) return;

  if (s_last_live_ms == 0) s_last_live_ms = now_ms;
  if (s_last_alarm_ms == 0) s_last_alarm_ms = now_ms;

  bool send_live = (now_ms - s_last_live_ms) >= 1000;
  bool send_alarm = (now_ms - s_last_alarm_ms) >= 2000;

  if (!send_live && !send_alarm) return;

  if (send_live) s_last_live_ms = now_ms;
  if (send_alarm) s_last_alarm_ms = now_ms;

  for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next) {
    if (!c->is_websocket) continue;
    ws_type_t t = ws_get_type(c);
    if (t == WS_TYPE_LIVE && send_live) {
      broadcast_live(c, &ctx->live);
    } else if (t == WS_TYPE_ALARMS && send_alarm) {
      broadcast_alarm(c, ctx->live.sequence);
    }
  }
}

