#ifndef MESSAGES_H
#define MESSAGES_H

//#include "table.h"
#include "../table/table.h"
#include "routing.h"
//#include "strategy.h"
//#include <wifi_hal.h>
#include <transport_hal.h>
//#include <../time_hal/time_hal.h>
//#include <time_hal.h>
#include <../middleware/strategies/strategy_inject/strategy_inject.h>
//#include "../wifi_hal/wifi_hal.h"
//#include <../transport_hal/transport_hal.h>
//#include "transport_hal.h"
//#include "lifecycle.h"
#include "logger.h"
#include "../net_viz/net_viz.h"
#include <cstdio>
#include <cstring>

typedef struct messageParameters{
    uint8_t IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
    int nrOfNodes = 0;
    uint8_t IP[10][4];
    int hopDistance = -1;
    int sequenceNumber = 0;
    int childrenNumber = -1;
    char payload[200] = "";
}messageParameters;


typedef enum messageType{
    PARENT_DISCOVERY_REQUEST, //0
    PARENT_INFO_RESPONSE, //1
    CHILD_REGISTRATION_REQUEST, //2
    FULL_ROUTING_TABLE_UPDATE, //3
    PARTIAL_ROUTING_TABLE_UPDATE, //4
    TOPOLOGY_BREAK_ALERT, //5
    TOPOLOGY_RESTORED_NOTICE,//6
    //CHILD_RELEASE_NOTICE,
    PARENT_RESET_NOTIFICATION,//7
    DEBUG_MESSAGE,//8
    DATA_MESSAGE,//9
    ACK_MESSAGE,//10
    MIDDLEWARE_MESSAGE,//11
}messageType;

extern char receiveBuffer[256];
extern char largeSendBuffer[255];
extern char smallSendBuffer[50];


void encodeMessage(char * msg, size_t bufferSize, messageType type, messageParameters parameters);

void encodeParentInfoResponse(char* messageBuffer, size_t bufferSize,uint8_t *APIP,int hopDistance,int childrenNumber);
void encodeChildRegistrationRequest(char* messageBuffer, size_t bufferSize,uint8_t *APIP,uint8_t *STAIP,int sequenceNumber);
void encodeFullRoutingTableUpdate(char* messageBuffer, size_t bufferSize);
void encodePartialRoutingUpdate(char* messageBuffer, size_t bufferSize,uint8_t nodeIPs[][4],int nrNodes);
void encodeTopologyBreakAlert(char* messageBuffer, size_t bufferSize);
void encodeTopologyRestoredNotice(char* messageBuffer, size_t bufferSize);
void encodeParentResetNotification(char* messageBuffer, size_t bufferSize);
void encodeParentDiscoveryRequest(char* messageBuffer, size_t bufferSize,uint8_t *STAIP);

void encodeDataMessage(char* messageBuffer, size_t bufferSize,char* payload,uint8_t *sourceIP,uint8_t *destinationIP);

bool isMessageValid(int expectedMessageType,char* msg);

void handleParentDiscoveryRequest(char* msg);
void handleParentInfoResponse(char* msg, parentInfo *parents, int i);
void handleChildRegistrationRequest(char * msg);
void handleFullRoutingTableUpdate(char *msg);
void handlePartialRoutingUpdate(char *msg);
void handleTopologyBreakAlert(char *msg);
void handleParentResetNotification(char *msg);
void handleDebugMessage(char* msg);
void handleDataMessage(char *msg);
void handleAckMessage(char *msg);
void handleDebugRegistrationRequest(char* msg);
void handleDebugMessage2(char* msg, int* nextHopIP);

void propagateMessage(char* message, uint8_t * sourceIP);
void encodeTunneledMessage(char* encodedMessage,size_t encodedMessageSize,uint8_t sourceIP[4], uint8_t destinationIP[4], char* encapsulatedMessage);
bool isMessageTunneled(char* dataMessage);
bool waitForMessage(messageType type, uint8_t expectedSenderIP[4], unsigned long timeOut);
void getSenderIP(char* messageBuffer, messageType type, uint8_t * senderIP);

void sendMessageToNode(char* messageBuffer,uint8_t *destinationIP);
void sendDataMessageToNode(char* messageBuffer,uint8_t *senderIP,uint8_t *destinationIP);
void sendMessageToChildren(char* messageBuffer);

#endif //MESSAGES_H
