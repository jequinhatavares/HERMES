#include "messages.h"
#include "routing.h"

#include <cstdio>
#include <cstring>


void encodeMessage(char * msg, messageType type, messageParameters parameters){
    char tempMsg[37] = "";//35

    switch (type) {
        case parentDiscoveryRequest:
            //0 [mySTAIP]
            sprintf(msg,"0 %i.%i.%i.%i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3]);
            break;
        case parentInfoResponse:
            //1 [my AP IP] [my hop distance to the root] [my number of children]
            sprintf(msg,"1 %i.%i.%i.%i %i %i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.hopDistance, parameters.childrenNumber);
            break;
        case childRegistrationRequest:
            //2 [my AP IP] [my STA IP]
            sprintf(msg,"2 %i.%i.%i.%i %i.%i.%i.%i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;
        case fullRoutingTableUpdate:
            //3 [node1 IP] [next hop IP] [hopDistance] |[node2 IP] [next hop IP] [hopDistance] |....
            sprintf(msg, "3 |");
            for (int i = 0; i < parameters.routingTable->numberOfItems; i++) {
                sprintf(tempMsg,"%i.%i.%i.%i %i.%i.%i.%i %i |",((int*)parameters.routingTable->table->key)[0],
                        ((int*)parameters.routingTable->table[i].key)[1],((int*)parameters.routingTable->table[i].key)[2],
                        ((int*)parameters.routingTable->table[i].key)[3],((NodeEntry *)parameters.routingTable->table[i].value)->nextHopIP[0],
                        ((NodeEntry *)parameters.routingTable->table[i].value)->nextHopIP[1],((NodeEntry *)parameters.routingTable->table[i].value)->nextHopIP[2],
                        ((NodeEntry *)parameters.routingTable->table[i].value)->nextHopIP[3],((NodeEntry *)parameters.routingTable->table[i].value)->hopDistance);

                strcat(msg, tempMsg);
            }
            break;
        case partialRoutingTableUpdate:
            sprintf(msg,"4 %i.%i.%i.%i %i.%i.%i.%i %i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3],
                    parameters.hopDistance);
            break;
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

void decodeChildRegistrationRequest(char * msg){
    int type;
    int childAPIP[4], childSTAIP[4];
    sscanf(msg, "%d ", &type);

    if (type == childRegistrationRequest){
        sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d", &type, &childAPIP[0],&childAPIP[1],&childAPIP[2],&childAPIP[3],
               &childSTAIP[0],&childSTAIP[1],&childSTAIP[2],&childSTAIP[3] );
        //Add the new children to the children table
        updateChildrenTable(childAPIP, childSTAIP);

        //Add the child node to the routing table
        updateRoutingTable(childAPIP, childAPIP,1);
    }
}

void decodeFullRoutingTableUpdate(char * msg){
    int type;
    int nodeIP[4], nextHopIP[4];
    int hopDistance;
    sscanf(msg, "%d ", &type);

    char* token = strtok(msg, "| ");

    if (type == fullRoutingTableUpdate){
        while (token != NULL) {
            //Serial.printf("% s\n", token);
            sscanf(token, "%d.%d.%d.%d %d.%d.%d.%d %d",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],
                   &nextHopIP[0],&nextHopIP[1],&nextHopIP[2],&nextHopIP[3], &hopDistance);
            token = strtok(NULL, "| ");
            //Update the Routing Table
            updateRoutingTable(nodeIP,nextHopIP,hopDistance);
        }
    }
}

void decodePartialRoutingUpdate(char *msg){
    int type;
    int nodeIP[4], nextHopIP[4];
    int hopDistance;
    sscanf(msg, "%d ", &type);

    if (type == partialRoutingTableUpdate){
        sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d %d",&type, &nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],
               &nextHopIP[0],&nextHopIP[1],&nextHopIP[2],&nextHopIP[3], &hopDistance);

        updateRoutingTable(nodeIP,nextHopIP,hopDistance);
    }

}
