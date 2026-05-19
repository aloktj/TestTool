#ifndef WEBBUILDER_API_DEMO_API_PUBLIC_H
#define WEBBUILDER_API_DEMO_API_PUBLIC_H

#include "mongoose.h"

#include "app.h"

void api_public_health(struct mg_connection *c, const app_ctx_t *ctx);
void api_public_version(struct mg_connection *c);
void api_public_system_info(struct mg_connection *c);

#endif  // WEBBUILDER_API_DEMO_API_PUBLIC_H

