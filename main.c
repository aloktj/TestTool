#include "mongoose.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "src/app.h"
#include "src/demo_data.h"
#include "src/routes.h"
#include "src/websocket.h"

static volatile int s_signo = 0;

static void signal_handler(int signo) {
  s_signo = signo;
}

static void settings_init_defaults(app_settings_t *s) {
  memset(s, 0, sizeof(*s));
  s->poll_interval_ms = 1000;
  s->websocket_enabled = true;
  s->auth_required = true;
  s->demo_mode = true;
}

int main(void) {
  struct mg_mgr mgr;
  struct mg_connection *listener = NULL;
  app_ctx_t ctx;

  memset(&ctx, 0, sizeof(ctx));
  settings_init_defaults(&ctx.settings);
  demo_data_init(&ctx.live);

  mg_mgr_init(&mgr);
  mgr.userdata = &ctx;
  mg_log_set(MG_LL_INFO);

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  listener = mg_http_listen(&mgr, "http://0.0.0.0:8000", routes_fn, NULL);
  if (listener == NULL) {
    MG_ERROR(("Failed to listen on http://0.0.0.0:8000"));
    mg_mgr_free(&mgr);
    return 1;
  }

  ctx.start_ms = mg_millis();
  printf("WebBuilder API Demo Server started\n");
  printf("Listening on http://0.0.0.0:8000\n");
  printf("Web root: ./web\n");

  while (s_signo == 0) {
    uint64_t now_ms = mg_millis();
    demo_data_update(&ctx.live, now_ms);
    websocket_broadcast_periodic(&mgr);
    mg_mgr_poll(&mgr, 10);
  }

  MG_INFO(("Signal %d received, exiting", s_signo));
  mg_mgr_free(&mgr);
  return 0;
}

