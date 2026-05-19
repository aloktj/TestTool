#ifndef WEBBUILDER_API_DEMO_API_SETTINGS_H
#define WEBBUILDER_API_DEMO_API_SETTINGS_H

#include "mongoose.h"

#include "app.h"

void api_settings_read(struct mg_connection *c, const app_ctx_t *ctx);
void api_settings_write(struct mg_connection *c, struct mg_http_message *hm,
                        app_ctx_t *ctx);

#endif  // WEBBUILDER_API_DEMO_API_SETTINGS_H

