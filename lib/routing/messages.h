#ifndef MESSAGES_H
#define MESSAGES_H

#include "../table/table.h"
//#include <table.h>
#include "routing.h"
#include "logger.h"
#include "transport_hal.h"
//#include "lifecycle.h"
#include "net_viz.h"

typedef struct messageParameters{
    int IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
    int nrOfNodes = 0;
    int IP[10][4];
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
    PARENT_LIST_ADVERTISEMENT,//5
    PARENT_REASSIGNMENT_COMMAND, //6
    TOPOLOGY_BREAK_ALERT, //7
    DEBUG_REGISTRATION_REQUEST, //8
    DEBUG_MESSAGE,//9
    DATA_MESSAGE,//10
    ACK_MESSAGE,//11
}messageType;

extern char receiveBuffer[256];
extern char largeSendBuffer[256];
extern char smallSendBuffer[50];


void encodeMessage(char * msg, size_t bufferSize, messageType type, messageParameters parameters);

bool isMessageValid(int expectedMessageType,char* msg);

void handleParentDiscoveryRequest(char* msg);
void handleParentInfoResponse(char* msg, parentInfo *parents, int i);
void handleChildRegistrationRequest(char * msg);
void handleFullRoutingTableUpdate(char *msg);
void handlePartialRoutingUpdate(char *msg);
void handleTopologyBreakAlert(char *msg);
void handleDebugMessage(char* msg);
void handleDataMessage(char *msg);
void handleAckMessage(char *msg);
void handleDebugRegistrationRequest(char* msg);
void handleDebugMessage2(char* msg, int* nextHopIP);

void propagateMessage(char* message, int* sourceIP);

#endif //MESSAGES_H
