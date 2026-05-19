#include "api_data.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

static uint64_t unix_time_sec_estimate(void) {
  time_t t = time(NULL);
  if (t < 0) return 0;
  return (uint64_t) t;
}

void api_data_temperature(struct mg_connection *c, const app_ctx_t *ctx) {
  const demo_live_data_t *d = &ctx->live;
  mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{"
                "\"temperatureC\":%.1f,"
                "\"humidityPercent\":%d,"
                "\"status\":\"NORMAL\","
                "\"timestamp\":%llu"
                "}",
                d->temperature_c, d->humidity_percent,
                (unsigned long long) unix_time_sec_estimate());
}

void api_data_sensors(struct mg_connection *c, const app_ctx_t *ctx) {
  const demo_live_data_t *d = &ctx->live;
  double a = d->temperature_c;
  double b = (double) d->humidity_percent;
  double p = d->pressure_bar;
  const char *state_c = (d->sequence % 7 == 0) ? "WARNING" : "OK";

  mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{"
                "\"sensors\":["
                "{"
                "\"id\":1,"
                "\"name\":\"Sensor A\","
                "\"value\":%.1f,"
                "\"unit\":\"C\","
                "\"state\":\"OK\""
                "},"
                "{"
                "\"id\":2,"
                "\"name\":\"Sensor B\","
                "\"value\":%.1f,"
                "\"unit\":\"%s\","
                "\"state\":\"OK\""
                "},"
                "{"
                "\"id\":3,"
                "\"name\":\"Sensor C\","
                "\"value\":%.1f,"
                "\"unit\":\"bar\","
                "\"state\":\"%s\""
                "}"
                "]"
                "}",
                a, b, "%", p, state_c);
}

void api_data_alarms(struct mg_connection *c, const app_ctx_t *ctx) {
  unsigned int seq = ctx->live.sequence;
  int active = (seq % 5 == 0) ? 1 : 2;
  const char *sev_latest = (seq % 2 == 0) ? "INFO" : "WARNING";
  const char *msg_latest =
      (seq % 2 == 0) ? "Demo alarm event" : "High temperature warning";
  int id_latest = (seq % 2 == 0) ? 1003 : 1001;

  mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{"
                "\"activeCount\":%d,"
                "\"alarms\":["
                "{"
                "\"id\":1001,"
                "\"severity\":\"WARNING\","
                "\"message\":\"High temperature warning\","
                "\"acknowledged\":false"
                "},"
                "{"
                "\"id\":1002,"
                "\"severity\":\"INFO\","
                "\"message\":\"System restarted\","
                "\"acknowledged\":true"
                "}"
                "],"
                "\"latestAlarm\":{"
                "\"id\":%d,"
                "\"severity\":\"%s\","
                "\"message\":\"%s\""
                "}"
                "}",
                active, id_latest, sev_latest, msg_latest);
}

void api_data_chart(struct mg_connection *c, const app_ctx_t *ctx) {
  const demo_live_data_t *d = &ctx->live;
  double y0 = d->temperature_c - 0.7;
  double y1 = d->temperature_c - 0.2;
  double y2 = d->temperature_c + 0.4;

  mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{"
                "\"points\":["
                "{\"x\":\"10:00\",\"y\":%.1f},"
                "{\"x\":\"10:01\",\"y\":%.1f},"
                "{\"x\":\"10:02\",\"y\":%.1f}"
                "]"
                "}",
                y0, y1, y2);
}
