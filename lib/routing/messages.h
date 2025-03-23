#ifndef MESSAGES_H
#define MESSAGES_H

#include "table.h"
#include "routing.h"
#include "logger.h"

typedef struct messageParameters{
    int IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
    int hopDistance = -1;
    int childrenNumber = -1;
    TableInfo *routingTable;
    char payload[200] = "";
}messageParameters;

//a = messageEncode(parentDiscoveryRequest, .ip=1.1.1.1, .hopDistance=2)
//#define encodeMessage(msg, message_type, ...) messageEncode(msg, message_type, (messageParameters){__VA_ARGS__})

typedef enum messageType{
    parentDiscoveryRequest, //0
    parentInfoResponse, //1
    childRegistrationRequest, //2
    fullRoutingTableUpdate, //3
    partialRoutingTableUpdate, //4
    dataMessage,//5 por enquanto
    ackMessage,//6
}messageType;


void encodeMessage(char* msg, messageType type, messageParameters parameters);
void decodeParentInfoResponse(char* msg, parentInfo *parents, int i);
void decodeChildRegistrationRequest(char * msg);
void decodeFullRoutingTableUpdate(char *msg, int* senderIP);
void decodePartialRoutingUpdate(char *msg, int* senderIP);
void decodeDataMessage(char *msg, int* nextHopIP, int* senderIP, int* destinyIP);
void decodeAckMessage(char *msg, int* nextHopIP, int* senderIP, int* destinyIP);


#endif //MESSAGES_H
