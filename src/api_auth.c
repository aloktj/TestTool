#include "api_auth.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"

static void reply_invalid_login(struct mg_connection *c) {
  mg_http_reply(c, 401, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                "{"
                "\"success\":false,"
                "\"error\":\"invalid_credentials\","
                "\"message\":\"Invalid username or password\""
                "}");
}

static bool parse_login_body(struct mg_http_message *hm, char *username,
                             size_t username_len, char *password,
                             size_t password_len) {
  bool got_any = false;
  username[0] = '\0';
  password[0] = '\0';

  // JSON body
  if (hm->body.len > 0 && hm->body.buf != NULL) {
    char *u = mg_json_get_str(hm->body, "$.username");
    char *p = mg_json_get_str(hm->body, "$.password");
    if (u != NULL) {
      strncpy(username, u, username_len - 1);
      username[username_len - 1] = '\0';
      free(u);
      got_any = true;
    }
    if (p != NULL) {
      strncpy(password, p, password_len - 1);
      password[password_len - 1] = '\0';
      free(p);
      got_any = true;
    }
  }

  // Form-url-encoded body: username=...&password=...
  if (!got_any) {
    int ru = mg_http_get_var(&hm->body, "username", username, username_len);
    int rp = mg_http_get_var(&hm->body, "password", password, password_len);
    if (ru > 0 || rp > 0) got_any = true;
  }
  if (!got_any) {
    int ru = mg_http_get_var(&hm->query, "username", username, username_len);
    int rp = mg_http_get_var(&hm->query, "password", password, password_len);
    if (ru > 0 || rp > 0) got_any = true;
  }

  return got_any && username[0] != '\0' && password[0] != '\0';
}

void api_auth_login(struct mg_connection *c, struct mg_http_message *hm) {
  char username[64], password[64];
  char token[768];
  app_ctx_t *ctx = (app_ctx_t *) c->mgr->userdata;
  const user_t *user = NULL;

  MG_INFO(("Login attempt"));
  if (!parse_login_body(hm, username, sizeof(username), password,
                        sizeof(password))) {
    return reply_invalid_login(c);
  }

  user = auth_login(username, password);
  if (user == NULL) {
    MG_INFO(("Login failed (user=%s)", username));
    return reply_invalid_login(c);
  }

  MG_INFO(("Login success (user=%s role=%s)", user->username,
           auth_role_to_string(user->role)));

  if (ctx != NULL && ctx->settings.jwt_enabled) {
    if (!auth_generate_jwt_for_user(user, token, sizeof(token))) {
      return mg_http_reply(c, 500,
                           "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                           "{\"success\":false,\"error\":\"jwt_generation_failed\"}");
    }
  } else {
    strncpy(token, user->token, sizeof(token) - 1);
    token[sizeof(token) - 1] = '\0';
  }

  mg_http_reply(c, 200, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                "{"
                "\"success\":true,"
                "\"token\":\"%s\","
                "\"username\":\"%s\","
                "\"role\":\"%s\","
                "\"permissions\":%s"
                "}",
                token, user->username, auth_role_to_string(user->role),
                auth_permissions_json_for_role(user->role));
}

void api_auth_logout(struct mg_connection *c, const user_t *user) {
  (void) user;
  mg_http_reply(c, 200, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                "{"
                "\"success\":true,"
                "\"message\":\"Logged out\""
                "}");
}

void api_auth_me(struct mg_connection *c, const user_t *user) {
  mg_http_reply(c, 200, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                "{"
                "\"authenticated\":true,"
                "\"username\":\"%s\","
                "\"role\":\"%s\","
                "\"permissions\":%s"
                "}",
                user->username, auth_role_to_string(user->role),
                auth_permissions_json_for_role(user->role));
}
