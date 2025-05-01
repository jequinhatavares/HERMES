#ifndef WIFIROUTING_WIFI_INTERFACE_H
#define WIFIROUTING_WIFI_INTERFACE_H

#include <time_hal.h>
#include "logger.h"
#include "table.h"
#include "wifi_common.h"

#define disconnectionThreshold 3

extern bool initializeAP;

typedef struct List {
    char item[10][50];
    int len = 0;
} List;



extern TableInfo* lostChildrenTable;


extern List ssidList;


// These callbacks let Wi-Fi events interact with other components (e.g., enqueueing state machine events)
// without tight coupling, avoiding direct dependencies.
extern void (*parentDisconnectCallback)();
extern bool (*isChildRegisteredCallback)(int*);

void initWifiEventHandlers();
String Get_WiFiStatus(int Status);
void startWifiSTA(const IPAddress& localIP, const IPAddress& gateway, const IPAddress& subnet, const IPAddress& dns);
void startWifiAP(const char* SSID, const char* Pass, const IPAddress& localIP, const IPAddress& gateway, const IPAddress& subnet);
void searchAP(String);
void connectToAP(const char*, const char*);
void stopWifiAP();
void disconnectFromAP();
int numberOfSTAConnected();
IPAddress getGatewayIP();
IPAddress getMySTAIP();
String getMyMAC();
IPAddress getMyAPIP();
void changeWifiMode(int);



#endif //WIFIROUTING_WIFI_INTERFACE_H
