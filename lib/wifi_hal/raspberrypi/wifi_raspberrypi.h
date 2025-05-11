#ifdef raspberrypi_3b

#ifndef WIFI_RASPBERRYPI_H
#define WIFI_RASPBERRYPI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/wireless.h>
#include <errno.h>

#include "../wifi_interface.h"

#define EVENTS_BUFFER_SIZE 1024

#define MAX_WIFI_EVENTS 2

typedef enum wifi_event_t{
    WIFI_EVENT_AP_STACONNECTED,
    WIFI_EVENT_AP_STADISCONNECTED,
    //WIFI_EVENT_STA_CONNECTED,
    //WIFI_EVENT_STA_DISCONNECTED,
} wifi_event_t;

typedef struct wifi_event_info__t{
    int IP[4];
    int MAC[6];
}wifi_event_info__t;

typedef void (*wifi_event_handler_t)(wifi_event_info__t *info);

extern int hostapd_sockfd;


void registerWifiEventHandler(wifi_event_t event, wifi_event_handler_t handler);
//void initWifiEventHandlers();

void startWifiEventListener();
ssize_t waitForWifiEvent(char *buffer);
void parseWifiEventInfo(char *msg);

void onAPModeStationConnectedHandler(wifi_event_info__t *info);
void onAPModeStationDisconnectedHandler(wifi_event_info__t *info);

//void getMyAPIP(int*IP);
//void getMySTAIP(int*IP);
//void getMyMAC(int* MAC);
//void getGatewayIP(int*IP);
void parseWifiScanResults(char *buffer, size_t length);
void searchAP2();
//void searchAP();
//int numberOfSTAConnected();



#endif //WIFI_RASPBERRYPI_H
#endif
