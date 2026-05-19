#include "api_settings.h"

#include <stdbool.h>

void api_settings_read(struct mg_connection *c, const app_ctx_t *ctx) {
  const app_settings_t *s = &ctx->settings;
  mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{"
                "\"pollIntervalMs\":%d,"
                "\"websocketEnabled\":%s,"
                "\"authRequired\":%s,"
                "\"demoMode\":%s"
                "}",
                s->poll_interval_ms, s->websocket_enabled ? "true" : "false",
                s->auth_required ? "true" : "false", s->demo_mode ? "true" : "false");
}

static void apply_bool_if_present(struct mg_str json, const char *path,
                                  bool *field) {
  bool v = false;
  if (mg_json_get_bool(json, path, &v)) {
    *field = v;
  }
}

void api_settings_write(struct mg_connection *c, struct mg_http_message *hm,
                        app_ctx_t *ctx) {
  app_settings_t *s = &ctx->settings;
  long poll_interval = s->poll_interval_ms;

  poll_interval = mg_json_get_long(hm->body, "$.pollIntervalMs", poll_interval);
  if (poll_interval < 50) poll_interval = 50;
  if (poll_interval > 600000) poll_interval = 600000;
  s->poll_interval_ms = (int) poll_interval;

  apply_bool_if_present(hm->body, "$.websocketEnabled", &s->websocket_enabled);
  apply_bool_if_present(hm->body, "$.authRequired", &s->auth_required);
  apply_bool_if_present(hm->body, "$.demoMode", &s->demo_mode);

  mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{"
                "\"saved\":true,"
                "\"message\":\"Settings updated\""
                "}");
}
