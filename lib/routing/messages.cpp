#include "messages.h"

//#include <Arduino.h>

#include <cstdio>
#include <cstring>


char messageBuffer[256] = "";
int senderIP[4];


/**
 * encodeMessage
 * Encodes a message according to the specified message type and parameters.
 *
 * @param msg - The buffer to store the encoded message.
 * @param type - The type of message to encode.
 * @param parameters - The message parameters containing data to encode.
 *                   PARENT_DISCOVERY_REQUEST - .IP1 contains my STA IP
 *                   PARENT_INFO_RESPONSE - .IP1 contains the my AP IP, .hopDistance my root hop distance and .childrenNumber my number of children
 *                   CHILD_REGISTRATION_REQUEST - .IP1 contains the my AP IP and .IP2 contains the my STA IP
 *                   FULL_ROUTING_TABLE_UPDATE - nothing
 *                   PARTIAL_ROUTING_TABLE_UPDATE - .IP1 contains the my node IP, .IP2 contains the next hop IP and .hopDistance contains my hop distance to that node
 *                   DATA_MESSAGE - .IP1 contains the source node IP, .IP2 contains the destiny hop IP and the .payload the message to be sent
 *                   ACK_MESSAGE - .IP1 contains the source node IP, .IP2 contains the destiny hop IP
 *                   PARENT_LIST_ADVERTISEMENT -
 *                   PARENT_REASSIGNMENT_COMMAND -
 * @return void
 */
void encodeMessage(char * msg, messageType type, messageParameters parameters){
    char tempMsg[37] = "";//35
    switch (type) {
        case PARENT_DISCOVERY_REQUEST:
            //0 [mySTAIP]
            sprintf(msg,"%i %i.%i.%i.%i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3]);
            break;
        case PARENT_INFO_RESPONSE:
            //1 [my AP IP] [my hop distance to the root] [my number of children]
            sprintf(msg,"%i %i.%i.%i.%i %i %i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.hopDistance, parameters.childrenNumber);
            break;
        case CHILD_REGISTRATION_REQUEST:
            //2 [my AP IP] [my STA IP]
            sprintf(msg,"%i %i.%i.%i.%i %i.%i.%i.%i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;
        case FULL_ROUTING_TABLE_UPDATE:
            //3 [rootIP] |[node1 IP] [next hop IP] [hopDistance] |[node2 IP] [next hop IP] [hopDistance] |....
            sprintf(msg, "%i %i.%i.%i.%i |",type, rootIP[0],rootIP[1],rootIP[2],rootIP[3]);

            for (int i = 0; i < parameters.routingTable->numberOfItems; i++) {
                sprintf(tempMsg,"%i.%i.%i.%i %i.%i.%i.%i %i |",((int*)routingTable->table[i].key)[0],
                        ((int*)routingTable->table[i].key)[1],((int*)routingTable->table[i].key)[2],
                        ((int*)routingTable->table[i].key)[3],((routingTableEntry *)routingTable->table[i].value)->nextHopIP[0],
                        ((routingTableEntry *)routingTable->table[i].value)->nextHopIP[1],((routingTableEntry *)routingTable->table[i].value)->nextHopIP[2],
                        ((routingTableEntry *)routingTable->table[i].value)->nextHopIP[3],((routingTableEntry *)routingTable->table[i].value)->hopDistance);

                strcat(msg, tempMsg);
                strcpy(tempMsg , "");
            }
            //Serial.printf("Formated msg: %s", msg);
            break;

        case PARTIAL_ROUTING_TABLE_UPDATE:
            //4 [node1 IP] [next hop IP] [hopDistance]
            sprintf(msg,"%i %i.%i.%i.%i %i.%i.%i.%i %i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3],
                    parameters.hopDistance);
            break;

        case PARENT_LIST_ADVERTISEMENT:
            //5 [myIP] [rootIP] [parent1IP] [parent2IP] [parent3IP] ...
            sprintf(msg, "%i %i.%i.%i.%i %i.%i.%i.%i",type,myIP[0],myIP[1],myIP[2],myIP[3],rootIP[0],rootIP[1],rootIP[2],rootIP[3]);
            for (int i = 0; i < parameters.nrOfPossibleParents; i++) {
                sprintf(tempMsg,"%i.%i.%i.%i",parameters.possibleParents[i][0], parameters.possibleParents[i][1],
                        parameters.possibleParents[i][2], parameters.possibleParents[i][3]);

                strcat(msg, tempMsg);
                strcpy(tempMsg , "");
            }
            break;

        case PARENT_REASSIGNMENT_COMMAND:
            //6 [parentIP]
            sprintf(msg,"%i %i.%i.%i.%i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3]);
            break;

        case DEBUG_MESSAGE:
            //8 [DEBUG message payload]
            sprintf(msg,"%i %s\n",type,parameters.payload);
            break;

        case DATA_MESSAGE:
            //9 [message payload] [source node IP] [destiny node IP]
            sprintf(msg,"%i %s %i.%i.%i.%i %i.%i.%i.%i",type,parameters.payload,parameters.IP1[0],parameters.IP1[1],
                    parameters.IP1[2],parameters.IP1[3],parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;

        case ACK_MESSAGE:
            //10 [source node IP] [destiny node IP]
            sprintf(msg,"%i %i.%i.%i.%i %i.%i.%i.%i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],
                    parameters.IP1[3],parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;
        default:
            break;
    }
}

void handleParentDiscoveryRequest(char* msg){
    int messageType;
    int childIP[4];
    messageParameters parameters;
    sscanf(msg, "%d %i.%i.%i.%i", &messageType, &childIP[0], &childIP[1], &childIP[2], &childIP[3]);

    //Send my information(IP, nr of children and root hop distance) to the node requesting it
    parameters.IP1[0] = myIP[0]; parameters.IP1[1] = myIP[1]; parameters.IP1[2] = myIP[2]; parameters.IP1[3] = myIP[3];
    parameters.childrenNumber = numberOfChildren;
    parameters.hopDistance = rootHopDistance;

    encodeMessage(msg,PARENT_INFO_RESPONSE,parameters);
    sendMessage(childIP,msg);
}
/**
 * decodeParentInfoResponse
 * Decodes a PARENT_INFO_RESPONSE message and stores the parent information.
 *
 * @param msg - The message to decode.
 * @param parents - The array to store parent information.
 * @param i - The index in the parents array to store the decoded information.
 * @return void
 */
void handleParentInfoResponse(char* msg, parentInfo *parents, int i){
    int messageType;
    int rootDistance, nrChildren;
    int parentIP[4];
    sscanf(msg, "%d %d.%d.%d.%d %d %d", &messageType, &parentIP[0],&parentIP[1],&parentIP[2],&parentIP[3],&rootDistance,&nrChildren);

    if (messageType == PARENT_INFO_RESPONSE){
        parents[i].rootHopDistance = rootDistance;
        parents[i].nrOfChildren = nrChildren;
        parents[i].parentIP[0]=parentIP[0];
        parents[i].parentIP[1]=parentIP[1];
        parents[i].parentIP[2]=parentIP[2];
        parents[i].parentIP[3]=parentIP[3];
    }
}

/**
 * decodeChildRegistrationRequest
 * Decodes a CHILD_REGISTRATION_REQUEST message and updates children and routing tables.
 *
 * @param msg - The message to decode.
 * @return void
 */
void handleChildRegistrationRequest(char * msg){
    int type;
    int childAPIP[4], childSTAIP[4];
    char messageBuffer1[50] = "", messageBufferLarge[300] = "";
    routingTableEntry newNode;
    messageParameters parameters;

    sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d", &type, &childAPIP[0],&childAPIP[1],&childAPIP[2],&childAPIP[3],
           &childSTAIP[0],&childSTAIP[1],&childSTAIP[2],&childSTAIP[3] );

    //Increase my number of children
    numberOfChildren++;

    //Add the new children to the children table
    updateChildrenTable(childAPIP, childSTAIP);

    //Add the child node to the routing table
    newNode.nextHopIP[0] = childAPIP[0];newNode.nextHopIP[1] = childAPIP[1];
    newNode.nextHopIP[2] = childAPIP[2];newNode.nextHopIP[3] = childAPIP[3];
    newNode.hopDistance = 1;
    updateRoutingTable(childAPIP, newNode, childAPIP);

    //Send my routing table to my child
    parameters.routingTable = routingTable;
    LOG(MESSAGES,INFO,"Sending my routing Table to child:");
    encodeMessage(messageBufferLarge,FULL_ROUTING_TABLE_UPDATE,parameters);
    LOG(MESSAGES,INFO,"%s to -> %d.%d.%d.%d\n",messageBufferLarge, childSTAIP[0], childSTAIP[1], childSTAIP[2], childSTAIP[3]);
    sendMessage(childSTAIP,messageBufferLarge);

    //Propagate the new node information trough the network
    assignIP(parameters.IP1,childAPIP);
    assignIP(parameters.IP2,childAPIP);
    parameters.hopDistance = 1;
    encodeMessage(messageBuffer1, PARTIAL_ROUTING_TABLE_UPDATE, parameters);
    propagateMessage(messageBuffer1, senderIP);

    //Sending new node information to the DEBUG visualization program, if enabled
    reportNewNodeToViz(childAPIP, myIP);

}

/**
 * decodeFullRoutingTableUpdate
 * Decodes a FULL_ROUTING_TABLE_UPDATE message and updates the routing table.
 *
 * @param msg - The message to decode.
 * @param senderIP - The IP address of the message sender.
 * @return void
 */
void handleFullRoutingTableUpdate(char * msg){
    int type;
    int nodeIP[4], nextHopIP[4];
    int hopDistance;
    routingTableEntry newNode;


    //Parse Message Type and root node IP
    sscanf(msg, "%d %d.%d.%d.%d", &type, &rootIP[0], &rootIP[1], &rootIP[2], &rootIP[3]);
    char* token = strtok(msg, "|");

    //To discard the message type and ensure the token points to the first routing table update entry
    token = strtok(NULL, "|");

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

/**
 * decodePartialRoutingUpdate
 * Decodes a PARTIAL_ROUTING_TABLE_UPDATE message and updates the routing table.
 *
 * @param msg - The message to decode.
 * @param senderIP - The IP address of the message sender.
 * @return void
 */
void handlePartialRoutingUpdate(char *msg){
    int type;
    int nodeIP[4], nextHopIP[4];
    int hopDistance;
    char messageBuffer1[50] = "";
    routingTableEntry newNode;
    messageParameters parameters;

    sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d %d",&type, &nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],
           &nextHopIP[0],&nextHopIP[1],&nextHopIP[2],&nextHopIP[3], &hopDistance);

    newNode.nextHopIP[0] = nextHopIP[0];newNode.nextHopIP[1] = nextHopIP[1];
    newNode.nextHopIP[2] = nextHopIP[2];newNode.nextHopIP[3] = nextHopIP[3];
    newNode.hopDistance = hopDistance;

    updateRoutingTable(nodeIP,newNode, senderIP);

    //TODO propagate the routing table update correctly in the network
    //Propagate the new node information trough the network
    //assignIP(parameters.IP1,childAPIP);
    //assignIP(parameters.IP2,childAPIP);
    //parameters.hopDistance = 1;
    //encodeMessage(messageBuffer1, PARTIAL_ROUTING_TABLE_UPDATE, parameters);
    //// If the message didn't come from the parent, forward it to the parent
    //if(!isIPEqual(senderIP, parent)){
    //    sendMessage(parent, messageBuffer1);
    //}
    ////Forward the message to all children except the one that sent it to me
    //for(int i = 0; i< childrenTable->numberOfItems; i++){
    //    if(!isIPEqual((int*)childrenTable->table[i].key, senderIP)){
    //        sendMessage((int*)childrenTable->table[i].key, messageBuffer1);
    //    }
    //}
}
void handleDebugMessage(char* msg){
    int nextHopIP[4], destinyIP[4];

    //If this message is not intended for this node, forward it to the next hop leading to its destination.
    if(!iamRoot){
        int* nextHopPtr = findRouteToNode(rootIP);
        if (nextHopPtr != nullptr){
            assignIP(nextHopIP, nextHopPtr);
        }else{
            LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                                "Unable to forward message.\n", destinyIP[0], destinyIP[1],destinyIP[2], destinyIP[3]);
        }
        sendMessage(nextHopIP,messageBuffer);
    }else{//send message to debug server
        //sendMessage(debugServerIP, messageBuffer);
    }

}
/**
 * decodeDataMessage
 * Decodes a DATA_MESSAGE and determines the next hop for message forwarding.
 *
 * @param msg - The message to decode.
 * @param nextHopIP - Output parameter for the next hop IP address.
 * @param senderIP - Output parameter for the sender's IP address.
 * @param destinyIP - Output parameter for the destination IP address.
 * @return void
 */
void handleDataMessage(char *msg){
    int type;
    int sourceIP[4], destinyIP[4], nextHopIP[4];
    int *nextHopPtr = nullptr;
    char payload[50];
    routingTableEntry newNode;
    messageParameters parameters;


    sscanf(msg, "%d %s %d.%d.%d.%d %d.%d.%d.%d",&type, payload, &sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],
        &destinyIP[0],&destinyIP[1],&destinyIP[2],&destinyIP[3]);
    //Serial.printf("Message %s received from %d.%d.%d.%d to %d.%d.%d.%d", payload, senderIP[0],senderIP[1],senderIP[2],senderIP[3],
        //destinyIP[0],destinyIP[1],destinyIP[2],destinyIP[3]);

    //Find the route to the destination IP of the message
    nextHopPtr = findRouteToNode(destinyIP);
    if (nextHopPtr != nullptr){
        assignIP(nextHopIP, nextHopPtr);
    }else{
        LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                            "Unable to forward message.\n", destinyIP[0], destinyIP[1],destinyIP[2], destinyIP[3]);
    }

    // If this message is not intended for this node, forward it to the next hop leading to its destination.
    if(!isIPEqual(nextHopIP, myIP)){
        sendMessage(nextHopIP,messageBuffer);
    }else{// If the message is for this node, process it and send an ACK back to the source

        //TODO process the message

        //Send ACK Message back to the source of the message
        assignIP(parameters.IP1,sourceIP);
        assignIP(parameters.IP2,destinyIP);
        encodeMessage(msg, ACK_MESSAGE, parameters);

        nextHopPtr = findRouteToNode(sourceIP);
        if (nextHopPtr != nullptr){
            sendMessage(nextHopPtr,msg);
        }else{
            LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                                "Unable to forward message.\n", sourceIP[0], sourceIP[1],sourceIP[2], sourceIP[3]);
        }
    }


}

/**
 * decodeAckMessage
 * Decodes an ACK_MESSAGE and determines the next hop for message forwarding.
 *
 * @param msg - The message to decode.
 * @param nextHopIP - Output parameter for the next hop IP address.
 * @param senderIP - Output parameter for the sender's IP address.
 * @param destinyIP - Output parameter for the destination IP address.
 * @return void
 */
void handleAckMessage(char *msg){
    int type;
    int nextHopIP[4], sourceIP[4], destinyIP[4];
    int *nextHopPtr = nullptr;


    sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d",&type, &sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],
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

    if(!isIPEqual(nextHopIP, myIP)){
        sendMessage(nextHopIP,messageBuffer);
    }else{
        //TODO process the ACK
    }

}

/**
 * decodeParentReassignmentCommand
 * Decodes a PARENT_REASSIGNMENT_COMMAND message.
 *
 * @param msg - The message to decode.
 * @param newParentIP - Output parameter for the new parent IP address.
 * @return void
 */
void handleParentReassignmentCommand(char *msg, int *newParentIP){

    //ToDo put event on the queue to change parent

}

/**
 * decodeParentListAdvertisement
 * Decodes a PARENT_LIST_ADVERTISEMENT message.
 *
 * @param msg - The message to decode.
 * @return void
 */
void handleParentListAdvertisement(char *msg){
    int type;

    //ToDo Callback to choose the node new parent

}

void handleDebugRegistrationRequest(char* msg){
    int type;
    //sscanf(msg, "%d %d.%d.%d.%d",&type, &debugServerIP[0],&debugServerIP[1],&debugServerIP[2],&debugServerIP[3]);
}

void handleDebugMessage2(char* msg, int* nextHopIP){
    int type;
    int senderIP[4], destinyIP[4];
    int *nextHopPtr = nullptr;
    char payload[50];

    sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d %s",&type, &senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3],
           &destinyIP[0],&destinyIP[1],&destinyIP[2],&destinyIP[3], payload);

    nextHopPtr = findRouteToNode(destinyIP);
    if (nextHopPtr != nullptr){
        assignIP(nextHopIP, nextHopPtr);
    }else{
        LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                            "Unable to forward message.\n", destinyIP[0], destinyIP[1],destinyIP[2], destinyIP[3]);
    }

}

void propagateMessage(char* message, int* sourceIP){
    // If the message didn't come from the parent, forward it to the parent
    if(!isIPEqual(sourceIP, parent)){
        sendMessage(parent, message);
    }
    //Forward the message to all children except the one that sent it to me
    for(int i = 0; i< childrenTable->numberOfItems; i++){
        if(!isIPEqual((int*)childrenTable->table[i].key, sourceIP)){
            sendMessage((int*)childrenTable->table[i].key, message);
        }
    }

}