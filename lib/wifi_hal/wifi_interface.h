#ifndef WIFIROUTING_WIFI_INTERFACE_H
#define WIFIROUTING_WIFI_INTERFACE_H

#include <time_hal.h>
#include "logger.h"
#include "table.h"
#include "wifi_common.h"

#define disconnectionThreshold 3

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
extern bool (*isChildRegisteredCallback)(int*);

void initWifiEventHandlers();
const char* getWifiStatus(int Status);
void startWifiSTA(int* localIP, int* gateway, int* subnet, int* dns);
void startWifiAP(const char* SSID, const char* Pass, int* localIP, int* gateway, int* subnet);
void searchAP(const char*);
void connectToAP(const char*, const char*);
void stopWifiAP();
void disconnectFromAP();
int numberOfSTAConnected();
void getGatewayIP(int* IP);
void getMySTAIP(int* IP);
void getMyMAC(int* MAC);
void getMyAPIP(int* IP);
void changeWifiMode(int);



#endif //WIFIROUTING_WIFI_INTERFACE_H
