#include "demo_data.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static double clamp_double(double v, double lo, double hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static int clamp_int(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void demo_data_init(demo_live_data_t *d) {
  memset(d, 0, sizeof(*d));
  d->temperature_c = 26.4;
  d->humidity_percent = 54;
  d->pressure_bar = 1.2;
  d->sequence = 0;
  d->last_update_ms = 0;
}

void demo_data_update(demo_live_data_t *d, uint64_t now_ms) {
  if (d->last_update_ms == 0) d->last_update_ms = now_ms;
  if (now_ms - d->last_update_ms < 250) return;  // Update at ~4 Hz

  d->last_update_ms = now_ms;
  d->sequence++;

  // Smooth-ish changing values for polling/WebSocket demos
  double t = (double) d->sequence / 12.0;
  double base_temp = 26.0 + 1.2 * sin(t);
  double base_hum = 54.0 + 6.0 * sin(t / 1.8);
  double base_press = 1.2 + 0.15 * sin(t / 2.3);

  d->temperature_c = clamp_double(base_temp, 20.0, 35.0);
  d->humidity_percent = clamp_int((int) (base_hum + 0.5), 20, 95);
  d->pressure_bar = clamp_double(base_press, 0.8, 1.6);
}

