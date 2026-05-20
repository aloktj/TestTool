#include "auth.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app.h"

static const user_t k_users[] = {
    {"operator", "operator123", "token_operator", ROLE_OPERATOR},
    {"maint", "maint123", "token_maintenance", ROLE_MAINTENANCE},
    {"admin", "admin123", "token_admin", ROLE_ADMIN},
};

static const char *k_jwt_secret = "webbuilder_demo_hs256_secret_change_me";

static size_t base64url_encode(const unsigned char *src, size_t src_len,
                               char *dst, size_t dst_len) {
  size_t n = mg_base64_encode(src, src_len, dst, dst_len);
  size_t i = 0;
  if (n == 0 || n >= dst_len) return 0;
  for (i = 0; i < n; i++) {
    if (dst[i] == '+') dst[i] = '-';
    else if (dst[i] == '/') dst[i] = '_';
  }
  while (n > 0 && dst[n - 1] == '=') n--;
  dst[n] = '\0';
  return n;
}

static int base64url_decode(const char *src, size_t src_len, char *dst,
                            size_t dst_len) {
  char tmp[512];
  size_t i = 0, pad = 0, n = 0;
  if (src_len + 4 >= sizeof(tmp)) return -1;
  for (i = 0; i < src_len; i++) {
    char ch = src[i];
    if (ch == '-') ch = '+';
    else if (ch == '_') ch = '/';
    tmp[i] = ch;
  }
  n = src_len;
  pad = (4 - (n % 4)) % 4;
  while (pad-- > 0) tmp[n++] = '=';
  tmp[n] = '\0';
  return (int) mg_base64_decode(tmp, n, dst, dst_len);
}

bool auth_generate_jwt_for_user(const user_t *user, char *out, size_t out_len) {
  char header_b64[128], payload[256], payload_b64[384], signing_input[512];
  uint8_t sig[32];
  char sig_b64[128];
  long now = (long) time(NULL);
  int payload_len = 0, signing_len = 0;

  if (user == NULL || out == NULL || out_len == 0) return false;
  if (base64url_encode((const unsigned char *) "{\"alg\":\"HS256\",\"typ\":\"JWT\"}",
                       strlen("{\"alg\":\"HS256\",\"typ\":\"JWT\"}"),
                       header_b64, sizeof(header_b64)) == 0) return false;

  payload_len = snprintf(payload, sizeof(payload),
                         "{\"sub\":\"%s\",\"role\":\"%s\",\"iat\":%ld,\"exp\":%ld}",
                         user->username, auth_role_to_string(user->role), now,
                         now + 3600);
  if (payload_len <= 0 || (size_t) payload_len >= sizeof(payload)) return false;
  if (base64url_encode((const unsigned char *) payload, (size_t) payload_len,
                       payload_b64, sizeof(payload_b64)) == 0) return false;

  signing_len = snprintf(signing_input, sizeof(signing_input), "%s.%s",
                         header_b64, payload_b64);
  if (signing_len <= 0 || (size_t) signing_len >= sizeof(signing_input)) return false;

  mg_hmac_sha256(sig, (uint8_t *) (void *) k_jwt_secret, strlen(k_jwt_secret),
                 (uint8_t *) (void *) signing_input, (size_t) signing_len);
  if (base64url_encode(sig, sizeof(sig), sig_b64, sizeof(sig_b64)) == 0) return false;

  return snprintf(out, out_len, "%s.%s", signing_input, sig_b64) > 0;
}

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
  char token[512], decoded_payload[512], signing_input[512], expected_sig_b64[128];
  char recv_sig_b64[128], username[64];
  const char *dot1 = NULL, *dot2 = NULL;
  size_t head_len = 0, payload_len = 0, sig_len = 0, signing_len = 0;
  uint8_t expected_sig[32];
  long exp = 0;
  int decoded_len = 0;
  if (!extract_bearer_token(hm, token, sizeof(token))) return NULL;

  dot1 = strchr(token, '.');
  if (dot1 == NULL) return NULL;
  dot2 = strchr(dot1 + 1, '.');
  if (dot2 == NULL) return NULL;
  head_len = (size_t) (dot1 - token);
  payload_len = (size_t) (dot2 - dot1 - 1);
  sig_len = strlen(dot2 + 1);
  if (head_len == 0 || payload_len == 0 || sig_len == 0) return NULL;

  signing_len = head_len + 1 + payload_len;
  if (signing_len + 1 > sizeof(signing_input)) return NULL;
  memcpy(signing_input, token, signing_len);
  signing_input[signing_len] = '\0';

  mg_hmac_sha256(expected_sig, (uint8_t *) (void *) k_jwt_secret,
                 strlen(k_jwt_secret), (uint8_t *) (void *) signing_input,
                 signing_len);
  if (base64url_encode(expected_sig, sizeof(expected_sig), expected_sig_b64,
                       sizeof(expected_sig_b64)) == 0) return NULL;

  if (sig_len + 1 > sizeof(recv_sig_b64)) return NULL;
  memcpy(recv_sig_b64, dot2 + 1, sig_len);
  recv_sig_b64[sig_len] = '\0';
  if (strcmp(expected_sig_b64, recv_sig_b64) != 0) return NULL;

  decoded_len = base64url_decode(dot1 + 1, payload_len, decoded_payload,
                                 sizeof(decoded_payload));
  if (decoded_len <= 0 || (size_t) decoded_len >= sizeof(decoded_payload)) return NULL;
  decoded_payload[decoded_len] = '\0';

  {
    char *sub = mg_json_get_str(mg_str(decoded_payload), "$.sub");
    if (sub == NULL) return NULL;
    strncpy(username, sub, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';
    free(sub);
  }
  exp = mg_json_get_long(mg_str(decoded_payload), "$.exp", 0);
  if (exp <= 0 || (long) time(NULL) >= exp) return NULL;
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
