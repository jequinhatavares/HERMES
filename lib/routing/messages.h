#ifndef MESSAGES_H
#define MESSAGES_H

#include "table.h"
#include "routing.h"
#include "logger.h"

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
    DATA_MESSAGE,//5 por enquanto
    ACK_MESSAGE,//6
    PARENT_LIST_ADVERTISEMENT,//7
    PARENT_REASSIGNMENT_COMMAND, //8
}messageType;


void encodeMessage(char* msg, messageType type, messageParameters parameters);
void decodeParentInfoResponse(char* msg, parentInfo *parents, int i);
void decodeChildRegistrationRequest(char * msg);
void decodeFullRoutingTableUpdate(char *msg, int* senderIP);
void decodePartialRoutingUpdate(char *msg, int* senderIP);
void decodeDataMessage(char *msg, int* nextHopIP, int* senderIP, int* destinyIP);
void decodeAckMessage(char *msg, int* nextHopIP, int* senderIP, int* destinyIP);


#endif //MESSAGES_H
