#include "auth.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app.h"

static const user_t k_users[] = {
    {"operator", "operator123", "token_operator", ROLE_OPERATOR},
    {"maint", "maint123", "token_maintenance", ROLE_MAINTENANCE},
    {"admin", "admin123", "token_admin", ROLE_ADMIN},
};

static void reply_json(struct mg_connection *c, int status, const char *json) {
  mg_http_reply(c, status, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n", "%s", json);
}

const char *auth_role_to_string(user_role_t role) {
  switch (role) {
    case ROLE_OPERATOR:
      return "operator";
    case ROLE_MAINTENANCE:
      return "maintenance";
    case ROLE_ADMIN:
      return "admin";
    case ROLE_GUEST:
    default:
      return "guest";
  }
}

const char *auth_permissions_json_for_role(user_role_t role) {
  if (role >= ROLE_ADMIN) {
    return "[\"data.read\",\"control.execute\",\"diagnostics.read\",\"settings.read\","
           "\"settings.write\"]";
  } else if (role >= ROLE_MAINTENANCE) {
    return "[\"data.read\",\"control.execute\",\"diagnostics.read\",\"settings.read\"]";
  } else if (role >= ROLE_OPERATOR) {
    return "[\"data.read\",\"control.execute\"]";
  }
  return "[]";
}

const user_t *auth_login(const char *username, const char *password) {
  size_t i = 0;
  for (i = 0; i < (sizeof(k_users) / sizeof(k_users[0])); i++) {
    if (strcmp(k_users[i].username, username) == 0 &&
        strcmp(k_users[i].password, password) == 0) {
      return &k_users[i];
    }
  }
  return NULL;
}

const user_t *auth_get_user_from_token(const char *token) {
  size_t i = 0;
  if (token == NULL || token[0] == '\0') return NULL;
  for (i = 0; i < (sizeof(k_users) / sizeof(k_users[0])); i++) {
    if (strcmp(k_users[i].token, token) == 0) return &k_users[i];
  }
  return NULL;
}

static const user_t *auth_get_user_from_username(const char *username) {
  size_t i = 0;
  if (username == NULL || username[0] == '\0') return NULL;
  for (i = 0; i < (sizeof(k_users) / sizeof(k_users[0])); i++) {
    if (strcmp(k_users[i].username, username) == 0) return &k_users[i];
  }
  return NULL;
}

static bool extract_bearer_token(struct mg_http_message *hm, char *out,
                                 size_t out_len) {
  struct mg_str *h = mg_http_get_header(hm, "Authorization");
  size_t i = 0, j = 0;

  if (out_len == 0) return false;
  out[0] = '\0';
  if (h == NULL) return false;
  if (h->len < 8) return false;
  if (memcmp(h->buf, "Bearer ", 7) != 0) return false;

  // Copy token and trim whitespace
  for (i = 7; i < h->len && j + 1 < out_len; i++) {
    out[j++] = h->buf[i];
  }
  out[j] = '\0';

  // Trim right
  while (j > 0 && isspace((unsigned char) out[j - 1])) {
    out[--j] = '\0';
  }

  // Trim left (rare, but safe)
  while (out[0] != '\0' && isspace((unsigned char) out[0])) {
    memmove(out, out + 1, strlen(out));
  }

  return out[0] != '\0';
}

const user_t *auth_get_user_from_header(struct mg_http_message *hm) {
  char token[128];
  if (!extract_bearer_token(hm, token, sizeof(token))) return NULL;
  return auth_get_user_from_token(token);
}

static bool extract_digest_username(struct mg_http_message *hm, char *out,
                                    size_t out_len) {
  struct mg_str *h = mg_http_get_header(hm, "Authorization");
  const char *p = NULL;
  const char *start = NULL;
  const char *end = NULL;
  size_t len = 0;

  if (out_len == 0) return false;
  out[0] = '\0';
  if (h == NULL || h->len == 0) return false;
  if (h->len < 7 || memcmp(h->buf, "Digest ", 7) != 0) return false;

  p = strstr(h->buf, "username=\"");
  if (p == NULL) return false;
  start = p + strlen("username=\"");
  end = strchr(start, '"');
  if (end == NULL || end <= start) return false;
  len = (size_t) (end - start);
  if (len + 1 > out_len) len = out_len - 1;
  memcpy(out, start, len);
  out[len] = '\0';
  return out[0] != '\0';
}

static const user_t *auth_get_user_from_digest(struct mg_http_message *hm) {
  char username[64];
  if (!extract_digest_username(hm, username, sizeof(username))) return NULL;
  return auth_get_user_from_username(username);
}

const user_t *auth_get_user_from_query(struct mg_http_message *hm) {
  char token[128];
  int rc = mg_http_get_var(&hm->query, "token", token, sizeof(token));
  if (rc <= 0) return NULL;
  token[sizeof(token) - 1] = '\0';
  return auth_get_user_from_token(token);
}

static const user_t *auth_get_user_from_jwt(struct mg_http_message *hm) {
  char token[256];
  char username[64];
  int n = 0;
  if (!extract_bearer_token(hm, token, sizeof(token))) return NULL;
  // Demo-only minimal JWT handling: jwt:<username> form
  n = sscanf(token, "jwt:%63[A-Za-z0-9_]", username);
  if (n != 1) return NULL;
  return auth_get_user_from_username(username);
}

static const user_t *require_role_common(struct mg_connection *c, const user_t *u,
                                        user_role_t required_role,
                                        bool is_ws) {
  (void) is_ws;
  if (u == NULL) {
    MG_INFO(("Unauthorized access attempt"));
    reply_json(c, 401,
               "{\"error\":\"unauthorized\",\"message\":\"Missing or invalid "
               "token\"}");
    return NULL;
  }
  if (u->role < required_role) {
    const char *msg = (required_role >= ROLE_ADMIN) ? "Admin role required"
                                                    : "Insufficient permissions";
    MG_INFO(("Forbidden access attempt (user=%s role=%s required=%s)", u->username,
             auth_role_to_string(u->role), auth_role_to_string(required_role)));
    mg_http_reply(c, 403, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                  "{"
                  "\"error\":\"forbidden\","
                  "\"message\":\"%s\""
                  "}",
                  msg);
    return NULL;
  }
  return u;
}

const user_t *auth_require_role(struct mg_connection *c, struct mg_http_message *hm,
                                user_role_t required_role) {
  app_ctx_t *ctx = (app_ctx_t *) c->mgr->userdata;
  const user_t *u = NULL;
  if (ctx != NULL && !ctx->settings.auth_required) {
    size_t i = 0;
    for (i = 0; i < (sizeof(k_users) / sizeof(k_users[0])); i++) {
      if (k_users[i].role >= required_role) return &k_users[i];
    }
    return NULL;
  }
  if (ctx != NULL && ctx->settings.digest_auth_enabled) {
    u = auth_get_user_from_digest(hm);
  } else if (ctx != NULL && ctx->settings.jwt_enabled) {
    u = auth_get_user_from_jwt(hm);
  } else {
    u = auth_get_user_from_header(hm);
  }
  return require_role_common(c, u, required_role, false);
}

const user_t *auth_require_ws_role(struct mg_connection *c,
                                   struct mg_http_message *hm,
                                   user_role_t required_role) {
  app_ctx_t *ctx = (app_ctx_t *) c->mgr->userdata;
  if (ctx != NULL && !ctx->settings.auth_required) {
    size_t i = 0;
    for (i = 0; i < (sizeof(k_users) / sizeof(k_users[0])); i++) {
      if (k_users[i].role >= required_role) return &k_users[i];
    }
    return NULL;
  }
  const user_t *u = auth_get_user_from_query(hm);
  return require_role_common(c, u, required_role, true);
}
