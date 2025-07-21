#ifndef WIFIROUTING_WIFI_INTERFACE_H
#define WIFIROUTING_WIFI_INTERFACE_H

#include "../time_hal/time_hal.h"
#include "../logger/logger.h"
#include "../table/table.h"
#include "wifi_common.h"

#include <cstdint>

#ifndef PARENT_DISCONNECTION_THRESHOLD
#define PARENT_DISCONNECTION_THRESHOLD 3
#endif

#ifndef WIFI_CONNECTION_TIMEOUT
#define WIFI_CONNECTION_TIMEOUT 3000
#endif

#ifndef AP_DISCONNECTION_GRACE_PERIOD
#define AP_DISCONNECTION_GRACE_PERIOD 3000
#endif


extern bool initializeAP;


typedef struct List{
    char item[10][50];
    int len = 0;
} List;


extern TableInfo* lostChildrenTable;


extern List reachableNetworks;


// These callbacks let Wi-Fi events interact with other components (e.g., enqueueing state machine events)
// without tight coupling, avoiding direct dependencies.
extern void (*parentDisconnectCallback)();
extern bool (*isChildRegisteredCallback)(uint8_t *);

void initWifiEventHandlers();
const char* getWifiStatus(int Status);
void startWifiSTA(int* localIP, int* gateway, int* subnet, int* dns);
void startWifiAP(const char* SSID, const char* Pass, uint8_t* localIP, uint8_t* gateway, uint8_t* subnet);
void searchAP(const char*);
bool connectToAP(const char*, const char*);
void stopWifiAP();
void disconnectFromAP();
int numberOfSTAConnected();
void getGatewayIP(uint8_t* IP);
void getMySTAIP(uint8_t* IP);
void getMyMAC(uint8_t* MAC);
void getMyAPIP(uint8_t* IP);
void changeWifiMode(int);



#endif //WIFIROUTING_WIFI_INTERFACE_H
