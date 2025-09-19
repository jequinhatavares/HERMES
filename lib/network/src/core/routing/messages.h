#ifndef MESSAGES_H
#define MESSAGES_H

/*** Include config.h at the top of every file that uses configurable macros.
 *   This ensures user-defined values take priority at compile time. ***/
//#include "network_config.h"

#include "../table/table.h"
#include "../logger/logger.h"
#include "../network_monitoring/network_monitoring.h"
#include "../routing/routing.h"

#include "../transport_hal/transport_hal.h"
#include "../time_hal/time_hal.h"

#include <cstdio>
#include <cstring>

#ifndef MAX_PAYLOAD_SIZE
#define MAX_PAYLOAD_SIZE 200
#endif

extern void (*onDataMessageCallback)(uint8_t*,uint8_t *,char*);
extern void (*onACKMessageCallback)(uint8_t*,uint8_t *,char*);

extern char receiveBuffer[256];
extern size_t receivePayload;
extern char largeSendBuffer[255];
extern char smallSendBuffer[50];

typedef enum MessageType{
    PARENT_DISCOVERY_REQUEST, //0
    PARENT_INFO_RESPONSE, //1
    CHILD_REGISTRATION_REQUEST, //2
    FULL_ROUTING_TABLE_UPDATE, //3
    PARTIAL_ROUTING_TABLE_UPDATE, //4
    TOPOLOGY_BREAK_ALERT, //5
    TOPOLOGY_RESTORED_NOTICE,//6
    PARENT_RESET_NOTIFICATION,//7
    MONITORING_MESSAGE,//8
    DATA_MESSAGE,//9
    MIDDLEWARE_MESSAGE,//10
}MessageType;



//void encodeMessage(char * msg, size_t bufferSize, MessageType type, messageParameters parameters);

void encodeParentInfoResponse(char* messageBuffer, size_t bufferSize,uint8_t *APIP,int hopDistance,int childrenNumber);
void encodeChildRegistrationRequest(char* messageBuffer, size_t bufferSize,uint8_t *APIP,uint8_t *STAIP,int sequenceNumber);
void encodeFullRoutingTableUpdate(char* messageBuffer, size_t bufferSize);
void encodePartialRoutingUpdate(char* messageBuffer, size_t bufferSize,uint8_t nodeIPs[][4],int nrNodes);
void encodeTopologyBreakAlert(char* messageBuffer, size_t bufferSize);
void encodeTopologyRestoredNotice(char* messageBuffer, size_t bufferSize);
void encodeParentResetNotification(char* messageBuffer, size_t bufferSize);
void encodeParentDiscoveryRequest(char* messageBuffer, size_t bufferSize,uint8_t *STAIP);
void encodeDebugMessage(char* messageBuffer, size_t bufferSize,char* payload);

void encodeACKMessage(char* messageBuffer, size_t bufferSize,const char* payload,uint8_t *sourceIP,uint8_t *destinationIP);
void encodeDataMessage(char* messageBuffer, size_t bufferSize,const char* payload,uint8_t *originatorIP,uint8_t *destinationIP);

bool isMessageValid(int expectedMessageType,char* msg);

void handleParentDiscoveryRequest(char* msg);
void handleParentInfoResponse(char* msg, ParentInfo *parents, int i);
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
bool waitForMessage(MessageType type, uint8_t expectedSenderIP[4], unsigned long timeOut);
void getSenderIP(char* messageBuffer, MessageType type, uint8_t * senderIP);

bool sendMessageToNode(char* messageBuffer,uint8_t *destinationIP);
void sendMessageToChildren(char* messageBuffer);
void sendMessageToParent(char* messageBuffer);

void sendDataMessageToNode(char* messageBuffer,size_t bufferSize,const char* messagePayload,uint8_t *destinationIP);
void sendDataMessageToChildren(char* messageBuffer,size_t bufferSize,const char* messagePayload);
void sendDataMessageToParent(char* messageBuffer,size_t bufferSize,const char* messagePayload);
void sendDataMessageToNode(char* messageBuffer,size_t bufferSize,const char* messagePayload,uint8_t *originatorIP,uint8_t *destinationIP);
void sendACKMessageToNode(char* messageBuffer,size_t bufferSize,const char* ackPayload,uint8_t *destinationIP);

#endif //MESSAGES_H
