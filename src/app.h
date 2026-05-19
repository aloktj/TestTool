#ifndef WEBBUILDER_API_DEMO_APP_H
#define WEBBUILDER_API_DEMO_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "demo_data.h"

typedef struct {
  int poll_interval_ms;
  bool websocket_enabled;
  bool auth_required;
  bool digest_auth_enabled;
  bool jwt_enabled;
  bool demo_mode;
} app_settings_t;

typedef struct app_ctx {
  uint64_t start_ms;
  app_settings_t settings;
  demo_live_data_t live;
} app_ctx_t;

#endif  // WEBBUILDER_API_DEMO_APP_H
