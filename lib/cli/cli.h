#if defined(ESP32) || defined(ESP8266)

#include "messages.h"
#include "transport_hal.h"
#include "wifi_hal.h"
#include "routing.h"
#include "logger.h"

void showMenu();
void readIPAddress(int *ip, const char *prompt);
void getDataMessage();
void cliInteraction();

#endif