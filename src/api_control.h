#ifndef WEBBUILDER_API_DEMO_API_CONTROL_H
#define WEBBUILDER_API_DEMO_API_CONTROL_H

#include "mongoose.h"

void api_control_start(struct mg_connection *c);
void api_control_stop(struct mg_connection *c);

#endif  // WEBBUILDER_API_DEMO_API_CONTROL_H

