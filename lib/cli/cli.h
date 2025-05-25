
#include "messages.h"
#include "transport_hal.h"
#include "wifi_hal.h"
#include "routing.h"
#include "logger.h"
#include "middleware.h"
//#include "lifecycle.h"
#include "../lib/middleware/strategies/strategy_inject/strategy_inject.h"
#include "../lib/middleware/strategies/strategy_pubsub/strategy_pubsub.h"



void showMenu();
void readIPAddress(int *ip, const char *prompt);
void getDataMessage();
void cliInteraction();

