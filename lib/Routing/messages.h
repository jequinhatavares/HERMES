#ifndef MESSAGES_H
#define MESSAGES_H


//a = messageEncode(parentDiscoveryRequest, .ip=1.1.1.1, .hopDistance=2)
#define messageEncode(msg, message_type, ...) encodeMessage(msg, message_type, (messageParameters) {__VA_ARGS__})

typedef struct messageParameters{
    int IP[4] = {0,0,0,0};
    int hopDistance = -1;
}messageParameters;

typedef enum messageType{
    parentDiscoveryRequest, //0
    parentInfoResponse, //1
    parentRegistrationRequest, //2
}messageType;


void encodeMessage(char* msg, messageType type, messageParameters parameters);
int decodeMessage(char* msg);

#endif //MESSAGES_H
