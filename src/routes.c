#include "routes.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "app.h"
#include "api_auth.h"
#include "api_control.h"
#include "api_data.h"
#include "api_public.h"
#include "api_settings.h"
#include "auth.h"
#include "websocket.h"

static bool method_is(struct mg_http_message *hm, const char *method) {
  return mg_strcasecmp(hm->method, mg_str(method)) == 0;
}

static bool uri_is(struct mg_http_message *hm, const char *uri) {
  return mg_strcmp(hm->uri, mg_str(uri)) == 0;
}

static bool uri_starts_with(struct mg_http_message *hm, const char *prefix) {
  size_t n = strlen(prefix);
  return hm->uri.len >= n && memcmp(hm->uri.buf, prefix, n) == 0;
}

static void reply_json(struct mg_connection *c, int status, const char *json) {
  mg_http_reply(c, status,
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                "Access-Control-Allow-Headers: Content-Type, Authorization\r\n",
                "%s", json);
}

static void reply_method_not_allowed(struct mg_connection *c) {
  reply_json(c, 405,
             "{\"error\":\"method_not_allowed\",\"message\":\"Method not "
             "allowed\"}");
}

static void log_http_request(struct mg_http_message *hm) {
  MG_INFO(("%.*s %.*s", (int) hm->method.len, hm->method.buf, (int) hm->uri.len,
           hm->uri.buf));
}

static void route_redirect_index(struct mg_connection *c) {
  mg_http_reply(c, 302, "Location: /index.html\r\n", "");
}


static bool file_exists(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (fp == NULL) return false;
  fclose(fp);
  return true;
}

static void serve_index_with_fallback(struct mg_connection *c,
                                      struct mg_http_message *hm) {
  if (file_exists("web/index.html")) {
    mg_http_serve_file(c, hm, "web/index.html", NULL);
    return;
  }

  if (file_exists("web/index_fallback.html")) {
    mg_http_serve_file(c, hm, "web/index_fallback.html", NULL);
    return;
  }

  mg_http_reply(c, 404, "Content-Type: text/plain\r\n",
                "Missing web/index.html and web/index_fallback.html\n");
}
static void serve_static(struct mg_connection *c, struct mg_http_message *hm) {
  struct mg_http_serve_opts opts;
  memset(&opts, 0, sizeof(opts));
  opts.root_dir = "web";
  mg_http_serve_dir(c, hm, &opts);
}

static void routes_handle_http(struct mg_connection *c,
                               struct mg_http_message *hm) {
  app_ctx_t *ctx = (app_ctx_t *) c->mgr->userdata;
  const user_t *user = NULL;

  if (method_is(hm, "OPTIONS") && uri_starts_with(hm, "/api/")) {
    return mg_http_reply(c, 204,
                         "Access-Control-Allow-Origin: *\r\n"
                         "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                         "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                         "Access-Control-Max-Age: 86400\r\n",
                         "");
  }

  if (uri_starts_with(hm, "/api/") || uri_starts_with(hm, "/ws/")) {
    log_http_request(hm);
  }

  if (uri_is(hm, "/")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    return route_redirect_index(c);
  }

  if (uri_is(hm, "/index.html")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    return serve_index_with_fallback(c, hm);
  }

  // Public APIs
  if (uri_is(hm, "/api/public/health")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    return api_public_health(c, ctx);
  }
  if (uri_is(hm, "/api/public/version")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    return api_public_version(c);
  }
  if (uri_is(hm, "/api/public/system-info")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    return api_public_system_info(c);
  }

  // Auth APIs
  if (uri_is(hm, "/api/auth/login")) {
    if (!method_is(hm, "POST") && !method_is(hm, "GET")) {
      return reply_method_not_allowed(c);
    }
    return api_auth_login(c, hm);
  }
  if (uri_is(hm, "/api/auth/logout")) {
    if (!method_is(hm, "POST") && !method_is(hm, "GET")) {
      return reply_method_not_allowed(c);
    }
    user = auth_require_role(c, hm, ROLE_OPERATOR);
    if (user == NULL) return;
    return api_auth_logout(c, user);
  }
  if (uri_is(hm, "/api/auth/me")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    user = auth_require_role(c, hm, ROLE_OPERATOR);
    if (user == NULL) return;
    return api_auth_me(c, user);
  }

  // Data APIs (Operator+)
  if (uri_is(hm, "/api/data/temperature")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    user = auth_require_role(c, hm, ROLE_OPERATOR);
    if (user == NULL) return;
    return api_data_temperature(c, ctx);
  }
  if (uri_is(hm, "/api/data/sensors")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    user = auth_require_role(c, hm, ROLE_OPERATOR);
    if (user == NULL) return;
    return api_data_sensors(c, ctx);
  }
  if (uri_is(hm, "/api/data/alarms")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    user = auth_require_role(c, hm, ROLE_OPERATOR);
    if (user == NULL) return;
    return api_data_alarms(c, ctx);
  }
  if (uri_is(hm, "/api/data/chart")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    user = auth_require_role(c, hm, ROLE_OPERATOR);
    if (user == NULL) return;
    return api_data_chart(c, ctx);
  }

  // Control APIs (Operator+)
  if (uri_is(hm, "/api/control/start")) {
    if (!method_is(hm, "POST")) return reply_method_not_allowed(c);
    user = auth_require_role(c, hm, ROLE_OPERATOR);
    if (user == NULL) return;
    return api_control_start(c);
  }
  if (uri_is(hm, "/api/control/stop")) {
    if (!method_is(hm, "POST")) return reply_method_not_allowed(c);
    user = auth_require_role(c, hm, ROLE_OPERATOR);
    if (user == NULL) return;
    return api_control_stop(c);
  }

  // Settings APIs
  if (uri_is(hm, "/api/settings/read")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    user = auth_require_role(c, hm, ROLE_MAINTENANCE);
    if (user == NULL) return;
    return api_settings_read(c, ctx);
  }
  if (uri_is(hm, "/api/settings/write")) {
    if (!method_is(hm, "POST")) return reply_method_not_allowed(c);
    user = auth_require_role(c, hm, ROLE_ADMIN);
    if (user == NULL) return;
    return api_settings_write(c, hm, ctx);
  }

  // WebSockets
  if (uri_is(hm, "/ws/live")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    return websocket_handle_upgrade(c, hm, WS_TYPE_LIVE);
  }
  if (uri_is(hm, "/ws/alarms")) {
    if (!method_is(hm, "GET")) return reply_method_not_allowed(c);
    return websocket_handle_upgrade(c, hm, WS_TYPE_ALARMS);
  }

  // Unknown API path
  if (uri_starts_with(hm, "/api/")) {
    return reply_json(c, 404,
                      "{\"error\":\"not_found\",\"message\":\"Unknown API "
                      "path\"}");
  }

  serve_static(c, hm);
}

void routes_fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    routes_handle_http(c, hm);
  } else if (ev == MG_EV_WS_OPEN) {
    websocket_on_open(c, (struct mg_http_message *) ev_data);
  } else if (ev == MG_EV_WS_MSG) {
    websocket_on_message(c, (struct mg_ws_message *) ev_data);
  } else if (ev == MG_EV_CLOSE) {
    websocket_on_close(c);
  }
}
