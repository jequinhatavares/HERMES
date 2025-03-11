#include "messages.h"
#include <cstdio>


void encodeMessage(char * msg, messageType type, messageParameters parameters){
    //char * msg = nullptr;
    switch (type) {
        case parentDiscoveryRequest:
            sprintf(msg,"0 %i.%i.%i.%i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3]);
            break;
        case parentInfoResponse:
            //Serial.printf("Parameters- hopDistance: %i numberChildren: %i IP1: %i.%i.%i.%i\n", parameters.hopDistance,parameters.childrenNumber,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3]);
            sprintf(msg,"1 %i.%i.%i.%i %i %i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.hopDistance, parameters.childrenNumber);
            break;
        case parentRegistrationRequest:
            sprintf(msg,"2 %i.%i.%i.%i %i.%i.%i.%i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;
        case fullRoutingTableUpdate:

        case partialRoutingTableUpdate:
        default:
            break;
    }
}

int decodeMessage(char* msg){
    messageType type;
    sscanf(msg, "%d", type);
    return type;
}

void decodeParentInfoResponse(char* msg, parentInfo *parents, int i){
    int type;
    int rootDistance, nrChildren;
    int parentIP[4];
    sscanf(msg, "%d %d.%d.%d.%d %d %d", &type, &parentIP[0],&parentIP[1],&parentIP[2],&parentIP[3],&rootDistance,&nrChildren);

    if (type == parentInfoResponse){
        parents[i].rootHopDistance = rootDistance;
        parents[i].nrOfChildren = nrChildren;
        parents[i].parentIP[0]=parentIP[0];
        parents[i].parentIP[1]=parentIP[1];
        parents[i].parentIP[2]=parentIP[2];
        parents[i].parentIP[3]=parentIP[3];
    }
}

