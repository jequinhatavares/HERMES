#ifndef LIFECYCLE_H
#define LIFECYCLE_H

#include "globals.h"
#include <transport_hal.h>
#include <wifi_hal.h>
#include <messages.h>

#define SSID_PREFIX      		"JessicaNode"
#define PASS      		        "123456789"



void joinNetwork();
parentInfo chooseParent(parentInfo* possibleParents, int n);

#endif //LIFECYCLE_H
