#include "wifi_pc.h"
#if defined(NATIVE)

List reachableNetworks;

unsigned long lastParentDisconnectionTime = 0 ;
int parentDisconnectionCount = 0;



void (*parentDisconnectCallback)() = nullptr;
bool (*isChildRegisteredCallback)(int*) = nullptr;


/**
 * initWifiEventHandlers
 * Sets up Wi-Fi event handlers
 *
 * @return void
 */
void initWifiEventHandlers(){}

const char* getWifiStatus(int Status){}


void startWifiSTA(int* localIP, int* gateway, int* subnet, int* dns){}

void startWifiAP(const char* SSID, const char* Pass, int* localIP, int* gateway, int* subnet){}

void searchAP(const char* SSID){}

void connectToAP(const char * SSID, const char * PASS) {}

void stopWifiAP(){}

void disconnectFromAP(){}

int numberOfSTAConnected(){}

void getGatewayIP(int *IP){}

void getMySTAIP(int *IP){}

void getMyMAC(int* MAC){}

void getMyAPIP(int* IP){}

void changeWifiMode(int mode){}


#endif