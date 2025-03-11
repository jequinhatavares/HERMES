#ifndef MESSAGES_H
#define MESSAGES_H


typedef struct messageParameters{
    int IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
    int hopDistance = -1;
    int childrenNumber = -1;
}messageParameters;

//a = messageEncode(parentDiscoveryRequest, .ip=1.1.1.1, .hopDistance=2)
//#define encodeMessage(msg, message_type, ...) messageEncode(msg, message_type, (messageParameters){__VA_ARGS__})

typedef enum messageType{
    parentDiscoveryRequest, //0
    parentInfoResponse, //1
    parentRegistrationRequest, //2
    fullRoutingTableUpdate, //3
    partialRoutingTableUpdate, //4
}messageType;

typedef struct parentInfo{
    char* ssid;
    int parentIP[4];
    int rootHopDistance;
    int nrOfChildren;
}parentInfo;

void encodeMessage(char* msg, messageType type, messageParameters parameters);
int decodeMessage(char* msg);
void decodeParentInfoResponse(char* msg, parentInfo *parents, int i);

#endif //MESSAGES_H
