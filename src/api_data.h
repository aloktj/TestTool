#ifndef WEBBUILDER_API_DEMO_API_DATA_H
#define WEBBUILDER_API_DEMO_API_DATA_H

#include "mongoose.h"

#include "app.h"

void api_data_temperature(struct mg_connection *c, const app_ctx_t *ctx);
void api_data_sensors(struct mg_connection *c, const app_ctx_t *ctx);
void api_data_alarms(struct mg_connection *c, const app_ctx_t *ctx);
void api_data_chart(struct mg_connection *c, const app_ctx_t *ctx);

#endif  // WEBBUILDER_API_DEMO_API_DATA_H

