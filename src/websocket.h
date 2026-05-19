#ifndef WEBBUILDER_API_DEMO_WEBSOCKET_H
#define WEBBUILDER_API_DEMO_WEBSOCKET_H

#include "mongoose.h"

typedef enum {
  WS_TYPE_NONE = 0,
  WS_TYPE_LIVE = 1,
  WS_TYPE_ALARMS = 2
} ws_type_t;

void websocket_handle_upgrade(struct mg_connection *c, struct mg_http_message *hm,
                              ws_type_t ws_type);

void websocket_on_open(struct mg_connection *c, struct mg_http_message *hm);
void websocket_on_message(struct mg_connection *c, struct mg_ws_message *wm);
void websocket_on_close(struct mg_connection *c);

void websocket_broadcast_periodic(struct mg_mgr *mgr);

#endif  // WEBBUILDER_API_DEMO_WEBSOCKET_H

