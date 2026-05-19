#ifndef WEBBUILDER_API_DEMO_API_AUTH_H
#define WEBBUILDER_API_DEMO_API_AUTH_H

#include "mongoose.h"

#include "auth.h"

void api_auth_login(struct mg_connection *c, struct mg_http_message *hm);
void api_auth_logout(struct mg_connection *c, const user_t *user);
void api_auth_me(struct mg_connection *c, const user_t *user);

#endif  // WEBBUILDER_API_DEMO_API_AUTH_H

