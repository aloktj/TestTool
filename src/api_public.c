#include "api_public.h"

#include <stdint.h>

void api_public_health(struct mg_connection *c, const app_ctx_t *ctx) {
  uint64_t now_ms = mg_millis();
  uint64_t uptime_sec = (now_ms >= ctx->start_ms) ? ((now_ms - ctx->start_ms) / 1000)
                                                  : 0;

  mg_http_reply(c, 200, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                "{"
                "\"status\":\"OK\","
                "\"uptimeSec\":%llu,"
                "\"server\":\"mongoose-c\","
                "\"mode\":\"webbuilder-api-demo\""
                "}",
                (unsigned long long) uptime_sec);
}

void api_public_version(struct mg_connection *c) {
  mg_http_reply(c, 200, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                "{"
                "\"appName\":\"WebBuilder API Demo Server\","
                "\"version\":\"1.0.0\","
                "\"build\":\"linux-x86_64\","
                "\"language\":\"C\","
                "\"webserver\":\"Mongoose\""
                "}");
}

void api_public_system_info(struct mg_connection *c) {
  mg_http_reply(c, 200, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                "{"
                "\"deviceName\":\"Linux VM API Simulator\","
                "\"deviceId\":\"VM_DEMO_001\","
                "\"hostname\":\"localhost\","
                "\"port\":8000"
                "}");
}

