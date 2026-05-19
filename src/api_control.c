#include "api_control.h"

void api_control_start(struct mg_connection *c) {
  mg_http_reply(c, 200, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                "{"
                "\"accepted\":true,"
                "\"command\":\"start\","
                "\"message\":\"System started\""
                "}");
}

void api_control_stop(struct mg_connection *c) {
  mg_http_reply(c, 200, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
                "{"
                "\"accepted\":true,"
                "\"command\":\"stop\","
                "\"message\":\"System stopped\""
                "}");
}

