#ifndef MESSAGES_H
#define MESSAGES_H

#include "table.h"
#include "routing.h"
#include "logger.h"
#include "transport_hal.h"
#include "lifecycle.h"
#include "net_viz.h"

typedef struct messageParameters{
    int IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
    int nrOfPossibleParents = 0;
    int** possibleParents;
    int hopDistance = -1;
    int childrenNumber = -1;
    TableInfo *routingTable;
    char payload[200] = "";
}messageParameters;

//a = messageEncode(PARENT_DISCOVERY_REQUEST, .ip=1.1.1.1, .hopDistance=2)
//#define encodeMessage(msg, message_type, ...) messageEncode(msg, message_type, (messageParameters){__VA_ARGS__})

typedef enum messageType{
    PARENT_DISCOVERY_REQUEST, //0
    PARENT_INFO_RESPONSE, //1
    CHILD_REGISTRATION_REQUEST, //2
    FULL_ROUTING_TABLE_UPDATE, //3
    PARTIAL_ROUTING_TABLE_UPDATE, //4
    PARENT_LIST_ADVERTISEMENT,//5
    PARENT_REASSIGNMENT_COMMAND, //6
    DEBUG_REGISTRATION_REQUEST, //7
    DEBUG_MESSAGE,//8
    DATA_MESSAGE,//9
    ACK_MESSAGE,//10
}messageType;


void encodeMessage(char* msg, messageType type, messageParameters parameters);
void handleParentDiscoveryRequest(char* msg);
void handleParentInfoResponse(char* msg, parentInfo *parents, int i);
void handleChildRegistrationRequest(char * msg);
void handleFullRoutingTableUpdate(char *msg);
void handlePartialRoutingUpdate(char *msg);
void handleDebugMessage(char* msg);
void handleDataMessage(char *msg);
void handleAckMessage(char *msg);
void handleDebugRegistrationRequest(char* msg);
void handleDebugMessage2(char* msg, int* nextHopIP);

#endif //MESSAGES_H
