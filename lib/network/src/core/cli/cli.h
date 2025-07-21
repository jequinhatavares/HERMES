
#include "../routing/messages.h"
#include "../transport_hal/transport_hal.h"
#include "../wifi_hal/wifi_hal.h"
#include "../routing/routing.h"
#include "../logger/logger.h"
#include "../middleware/middleware.h"
//#include "lifecycle.h"
#include "../middleware/strategies/strategy_inject/strategy_inject.h"
#include "../middleware/strategies/strategy_pubsub/strategy_pubsub.h"



void showMenu();
void readIPAddress(uint8_t *ip, const char *prompt);
void getDataMessage();
void cliInteraction();

