#include "messages.h"
#include <cstdio>

void encodeMessage(char * msg, messageType type, messageParameters parameters){
    //char * msg = nullptr;
    switch (type) {
        case parentDiscoveryRequest:
            sprintf(msg,"0 %i.%i.%i.%i",parameters.IP[0],parameters.IP[1],parameters.IP[2],parameters.IP[3]);
            break;
        case parentInfoResponse:
            sprintf(msg,"1 %i.%i.%i.%i %i %i",parameters.IP[0],parameters.IP[1],parameters.IP[2],parameters.IP[3],parameters.hopDistance, parameters.childrenNumber);
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

void decodeParentInfoResponse(char* msg, parentSelectionInfo *parents, int i){
    int type;
    int rootDistance, nrChildren;
    int parentIP[4];
    sscanf(msg, "%i %i.%i.%i.%i %i %i", &type, &parentIP[0],&parentIP[1],&parentIP[2],&parentIP[3],&rootDistance,&nrChildren);

    if (type == parentInfoResponse){
        parents[i].rootHopDistance = rootDistance;
        parents[i].childrenNumber = nrChildren;
        parents->parentIP[0]=parentIP[0];
        parents->parentIP[1]=parentIP[1];
        parents->parentIP[2]=parentIP[2];
        parents->parentIP[3]=parentIP[3];
    }
}

