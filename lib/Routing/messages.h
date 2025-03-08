#ifndef MESSAGES_H
#define MESSAGES_H



typedef struct messageParameters{
    int IP[4] = {0,0,0,0};
    int hopDistance = -1;
    int childrenNumber = -1;
}messageParameters;

//a = messageEncode(parentDiscoveryRequest, .ip=1.1.1.1, .hopDistance=2)
//#define encodeMessage(msg, message_type, ...) messageEncode(msg, message_type, (messageParameters){__VA_ARGS__})

typedef enum messageType{
    parentDiscoveryRequest, //0
    parentInfoResponse, //1
    parentRegistrationRequest, //2
}messageType;

typedef struct parentSelectionInfo{
    int parentIP[4];
    int rootHopDistance;
    int childrenNumber;
}parentSelectionInfo;

void encodeMessage(char* msg, messageType type, messageParameters parameters);
int decodeMessage(char* msg);
void decodeParentInfoResponse(char* msg, parentSelectionInfo *parents, int i);

#endif //MESSAGES_H
