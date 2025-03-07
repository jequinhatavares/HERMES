#include "messages.h"
#include <cstdio>

void encodeMessage(char * msg, messageType type, messageParameters parameters){
    //char * msg = nullptr;
    switch (type) {
        case parentDiscoveryRequest:
            sprintf(msg,"0 %i.%i.%i.%i",parameters.IP[0],parameters.IP[1],parameters.IP[2],parameters.IP[3]);
            break;
        case parentInfoResponse:
            sprintf(msg,"1 %i.%i.%i.%i %i",parameters.IP[0],parameters.IP[1],parameters.IP[2],parameters.IP[3],parameters.hopDistance);
            break;
        case parentRegistrationRequest:
            break;
        default:
            break;
    }
}

int decodeMessage(char* msg){
    messageType type;
    sscanf(msg, "%i", type);
    return type;
}

