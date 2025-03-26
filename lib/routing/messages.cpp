#include "messages.h"

//#include <Arduino.h>

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
            //3 [rootIP] |[node1 IP] [next hop IP] [hopDistance] |[node2 IP] [next hop IP] [hopDistance] |....
            sprintf(msg, "3 %i.%i.%i.%i |", rootIP[0],rootIP[1],rootIP[2],rootIP[3]);

            for (int i = 0; i < parameters.routingTable->numberOfItems; i++) {
                sprintf(tempMsg,"%i.%i.%i.%i %i.%i.%i.%i %i |",((int*)routingTable->table[i].key)[0],
                        ((int*)routingTable->table[i].key)[1],((int*)routingTable->table[i].key)[2],
                        ((int*)routingTable->table[i].key)[3],((routingTableEntry *)parameters.routingTable->table[i].value)->nextHopIP[0],
                        ((routingTableEntry *)parameters.routingTable->table[i].value)->nextHopIP[1],((routingTableEntry *)parameters.routingTable->table[i].value)->nextHopIP[2],
                        ((routingTableEntry *)parameters.routingTable->table[i].value)->nextHopIP[3],((routingTableEntry *)parameters.routingTable->table[i].value)->hopDistance);

                strcat(msg, tempMsg);
                strcpy(tempMsg , "");
            }
            //Serial.printf("Formated msg: %s", msg);
            break;
        case partialRoutingTableUpdate:
            //4 [node1 IP] [next hop IP] [hopDistance]
            sprintf(msg,"4 %i.%i.%i.%i %i.%i.%i.%i %i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3],
                    parameters.hopDistance);
            break;

        case dataMessage:
            //5 [message payload] [source node IP] [destiny node IP]
            sprintf(msg,"5 %s %i.%i.%i.%i %i.%i.%i.%i", parameters.payload,parameters.IP1[0],parameters.IP1[1],
                    parameters.IP1[2],parameters.IP1[3],parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;

        case ackMessage:
            //6 [source node IP] [destiny node IP]
            sprintf(msg,"6 %i.%i.%i.%i %i.%i.%i.%i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],
                    parameters.IP1[3],parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;

        case parentListAdvertisement:
            //7 [myIP] [parent1IP] [parent2IP] [parent3IP] ...
            sprintf(msg, "7 ");
            for (int i = 0; i < parameters.nrOfPossibleParents; i++) {
                sprintf(tempMsg,"%i.%i.%i.%i",parameters.possibleParents[i][0], parameters.possibleParents[i][1],
                        parameters.possibleParents[i][2], parameters.possibleParents[i][3]);

                strcat(msg, tempMsg);
                strcpy(tempMsg , "");
            }
            break;
        case parentReassignmentCommand:
            //8 [parentIP]
            sprintf(msg,"8 %i.%i.%i.%i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3]);
            break;
        default:
            break;
    }
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
    routingTableEntry newNode;
    sscanf(msg, "%d ", &type);

    if (type == childRegistrationRequest){
        sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d", &type, &childAPIP[0],&childAPIP[1],&childAPIP[2],&childAPIP[3],
               &childSTAIP[0],&childSTAIP[1],&childSTAIP[2],&childSTAIP[3] );
        //Add the new children to the children table
        updateChildrenTable(childAPIP, childSTAIP);

        newNode.nextHopIP[0] = childAPIP[0];newNode.nextHopIP[1] = childAPIP[1];
        newNode.nextHopIP[2] = childAPIP[2];newNode.nextHopIP[3] = childAPIP[3];
        newNode.hopDistance = 1;
        //Add the child node to the routing table
        updateRoutingTable(childAPIP, newNode, childAPIP);
        //Increase the number of children
        numberOfChildren++;
    }
}

void decodeFullRoutingTableUpdate(char * msg, int* senderIP){
    int type;
    int nodeIP[4], nextHopIP[4];
    int hopDistance;
    routingTableEntry newNode;

    //Parse Message Type and root node IP
    sscanf(msg, "%d %d.%d.%d.%d", &type, &rootIP[0], &rootIP[1], &rootIP[2], &rootIP[3]);
    char* token = strtok(msg, "|");

    //To discard the message type and ensure the token points to the first routing table update entry
    token = strtok(NULL, "|");

    if (type == fullRoutingTableUpdate){
        while (token != NULL) {
            sscanf(token, "%d.%d.%d.%d %d.%d.%d.%d %d",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],
                   &nextHopIP[0],&nextHopIP[1],&nextHopIP[2],&nextHopIP[3], &hopDistance);
            //Serial.printf("Token: %s\n", token);

            //Serial.printf("Parsed IP values: nodeIP %d.%d.%d.%d nextHopIp %d.%d.%d.%d hopDistance %d\n",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3],
                         // nextHopIP[0],nextHopIP[1],nextHopIP[2],nextHopIP[3], hopDistance);
            newNode.nextHopIP[0] = nextHopIP[0];newNode.nextHopIP[1] = nextHopIP[1];
            newNode.nextHopIP[2] = nextHopIP[2];newNode.nextHopIP[3] = nextHopIP[3];
            newNode.hopDistance = hopDistance;
            //Update the Routing Table
            updateRoutingTable(nodeIP,newNode,senderIP);
            token = strtok(NULL, "|");
        }
    }
}

void decodePartialRoutingUpdate(char *msg, int* senderIP){
    int type;
    int nodeIP[4], nextHopIP[4];
    int hopDistance;
    routingTableEntry newNode;

    sscanf(msg, "%d ", &type);

    if (type == partialRoutingTableUpdate){
        sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d %d",&type, &nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],
               &nextHopIP[0],&nextHopIP[1],&nextHopIP[2],&nextHopIP[3], &hopDistance);

        newNode.nextHopIP[0] = nextHopIP[0];newNode.nextHopIP[1] = nextHopIP[1];
        newNode.nextHopIP[2] = nextHopIP[2];newNode.nextHopIP[3] = nextHopIP[3];
        newNode.hopDistance = hopDistance;

        updateRoutingTable(nodeIP,newNode, senderIP);
    }

}

void decodeDataMessage(char *msg, int* nextHopIP, int* senderIP, int* destinyIP){
    int type;
    int *nextHopPtr = nullptr;
    char payload[50];
    routingTableEntry newNode;
    //Serial.printf("Entered decode Data Message\n");
    sscanf(msg, "%d ", &type);

    if (type == dataMessage){
        sscanf(msg, "%d %s %d.%d.%d.%d %d.%d.%d.%d",&type, payload, &senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3],
            &destinyIP[0],&destinyIP[1],&destinyIP[2],&destinyIP[3]);
        //Serial.printf("Message %s received from %d.%d.%d.%d to %d.%d.%d.%d", payload, senderIP[0],senderIP[1],senderIP[2],senderIP[3],
            //destinyIP[0],destinyIP[1],destinyIP[2],destinyIP[3]);
        nextHopPtr = findRouteToNode(destinyIP);
        if (nextHopPtr != nullptr){
            assignIP(nextHopIP, nextHopPtr);
        }else{
            LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                                "Unable to forward message.\n", destinyIP[0], destinyIP[1],destinyIP[2], destinyIP[3]);
        }
        //Serial.printf("Next Hop IP: %d.%d.%d.%d", nextHopIP[0],nextHopIP[1],nextHopIP[2],nextHopIP[3]);
    }

}
void decodeAckMessage(char *msg, int* nextHopIP, int* senderIP, int* destinyIP){
    int type;
    int *nextHopPtr = nullptr;
    routingTableEntry newNode;
    //Serial.printf("Entered decode Data Message\n");
    sscanf(msg, "%d ", &type);

    if (type == ackMessage){
        sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d",&type, &senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3],
               &destinyIP[0],&destinyIP[1],&destinyIP[2],&destinyIP[3]);
        //Serial.printf("Message %s received from %d.%d.%d.%d to %d.%d.%d.%d", payload, senderIP[0],senderIP[1],senderIP[2],senderIP[3],
        //destinyIP[0],destinyIP[1],destinyIP[2],destinyIP[3]);
        nextHopPtr = findRouteToNode(destinyIP);
        if (nextHopPtr != nullptr){
            assignIP(nextHopIP, nextHopPtr);
        }else{
            LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                                "Unable to forward message.\n", destinyIP[0], destinyIP[1],destinyIP[2], destinyIP[3]);
        }

        //Serial.printf("Next Hop IP: %d.%d.%d.%d", nextHopIP[0],nextHopIP[1],nextHopIP[2],nextHopIP[3]);
    }

}