#include "wifi_pc.h"
#if defined(NATIVE)

List reachableNetworks;

unsigned long lastParentDisconnectionTime = 0 ;
int parentDisconnectionCount = 0;



void (*parentDisconnectCallback)() = nullptr;
bool (*isChildRegisteredCallback)(uint8_t*) = nullptr;


/**
 * initWifiEventHandlers
 * Sets up Wi-Fi event handlers
 *
 * @return void
 */
void initWifiEventHandlers(){}

const char* getWifiStatus(int Status){return "";}

void startWifiSTA(int* localIP, int* gateway, int* subnet, int* dns){}

void startWifiAP(const char* SSID, const char* Pass, uint8_t* localIP, uint8_t* gateway, uint8_t* subnet){}

void searchAP(const char* SSID){}

void connectToAP(const char * SSID, const char * PASS) {}

void stopWifiAP(){}

void disconnectFromAP(){}

int numberOfSTAConnected(){return 0;}

void getGatewayIP(uint8_t *IP){}

void getMySTAIP(uint8_t *IP){}

void getMyMAC(uint8_t* MAC){}

void getMyAPIP(uint8_t* IP){}

void changeWifiMode(int mode){}


#endif