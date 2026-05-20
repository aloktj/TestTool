#ifndef WEBBUILDER_API_DEMO_AUTH_H
#define WEBBUILDER_API_DEMO_AUTH_H

#include "mongoose.h"

typedef enum {
  ROLE_GUEST = 0,
  ROLE_OPERATOR = 1,
  ROLE_MAINTENANCE = 2,
  ROLE_ADMIN = 3
} user_role_t;

typedef struct {
  const char *username;
  const char *password;
  const char *token;
  user_role_t role;
} user_t;

const user_t *auth_login(const char *username, const char *password);
const user_t *auth_get_user_from_token(const char *token);
bool auth_generate_jwt_for_user(const user_t *user, char *out, size_t out_len);

const user_t *auth_get_user_from_header(struct mg_http_message *hm);
const user_t *auth_get_user_from_query(struct mg_http_message *hm);

const user_t *auth_require_role(struct mg_connection *c, struct mg_http_message *hm,
                                user_role_t required_role);
const user_t *auth_require_ws_role(struct mg_connection *c,
                                   struct mg_http_message *hm,
                                   user_role_t required_role);

const char *auth_role_to_string(user_role_t role);
const char *auth_permissions_json_for_role(user_role_t role);

#endif  // WEBBUILDER_API_DEMO_AUTH_H
