#ifndef WEBBUILDER_API_DEMO_DEMO_DATA_H
#define WEBBUILDER_API_DEMO_DEMO_DATA_H

#include <stdint.h>

typedef struct {
  double temperature_c;
  int humidity_percent;
  double pressure_bar;
  unsigned int sequence;
  uint64_t last_update_ms;
} demo_live_data_t;

void demo_data_init(demo_live_data_t *d);
void demo_data_update(demo_live_data_t *d, uint64_t now_ms);

#endif  // WEBBUILDER_API_DEMO_DEMO_DATA_H

