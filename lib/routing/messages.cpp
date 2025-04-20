#include "messages.h"

//#include <Arduino.h>

#include <cstdio>
#include <cstring>


char messageBuffer[256] = "";


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
 *                   DATA_MESSAGE - .IP1 contains the source node IP, .IP2 contains the destination hop IP and the .payload the message to be sent
 *                   ACK_MESSAGE - .IP1 contains the source node IP, .IP2 contains the destination hop IP
 *                   PARENT_LIST_ADVERTISEMENT -
 *                   PARENT_REASSIGNMENT_COMMAND -
 * @return void
 */
void encodeMessage(char * msg, messageType type, messageParameters parameters){
    char tempMsg[40] = "";//35
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
            sprintf(msg,"%i %i.%i.%i.%i %i.%i.%i.%i %i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3], parameters.sequenceNumber);
            break;
        case FULL_ROUTING_TABLE_UPDATE:
            //3 [senderIP] [rootIP] |[node1 IP] [hopDistance] [Sequence Number1]|[node2 IP] [hopDistance] [Sequence Number2]|....
            sprintf(msg, "%i %i.%i.%i.%i %i.%i.%i.%i |",type,parameters.senderIP[0],parameters.senderIP[1],parameters.senderIP[2],
                    parameters.senderIP[3],rootIP[0],rootIP[1],rootIP[2],rootIP[3]);

            for (int i = 0; i < routingTable->numberOfItems; i++) {
                sprintf(tempMsg,"%i.%i.%i.%i %i %i|",((int*)routingTable->table[i].key)[0],
                        ((int*)routingTable->table[i].key)[1],((int*)routingTable->table[i].key)[2],
                        ((int*)routingTable->table[i].key)[3],((routingTableEntry *)routingTable->table[i].value)->hopDistance,
                        ((routingTableEntry *)routingTable->table[i].value)->sequenceNumber);

                strcat(msg, tempMsg);
                strcpy(tempMsg , "");
            }
            //Serial.printf("Formated msg: %s", msg);
            break;

        case PARTIAL_ROUTING_TABLE_UPDATE:
            //4 [senderIP] [node1 IP] [hopDistance] [sequenceNumber]
            sprintf(msg,"%i %i.%i.%i.%i %i.%i.%i.%i %i %i",type,parameters.senderIP[0],parameters.senderIP[1],parameters.senderIP[2],
                    parameters.senderIP[3],parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],parameters.hopDistance, parameters.sequenceNumber);
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
            //6 [nodeIP] [parentIP]
            sprintf(msg,"%i %i.%i.%i.%i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3]);
            break;

        case TOPOLOGY_BREAK_ALERT:
            //7 [senderIP]
            sprintf(msg,"%i %i.%i.%i.%i",type,parameters.senderIP[0],parameters.senderIP[1],parameters.senderIP[2],
                    parameters.senderIP[3]);
            break;

        case DEBUG_MESSAGE:
            //9 [DEBUG message payload]
            sprintf(msg,"%i %s\n",type,parameters.payload);
            break;

        case DATA_MESSAGE:
            //9 [message payload] [source node IP] [destination node IP]
            sprintf(msg,"%i %s %i.%i.%i.%i %i.%i.%i.%i",type,parameters.payload,parameters.IP1[0],parameters.IP1[1],
                    parameters.IP1[2],parameters.IP1[3],parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;

        case ACK_MESSAGE:
            //10 [source node IP] [destination node IP]
            sprintf(msg,"%i %i.%i.%i.%i %i.%i.%i.%i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],
                    parameters.IP1[3],parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;
        default:
            break;
    }
}

/**
 * handleParentDiscoveryRequest
 * Handles a PARENT_DISCOVERY_REQUEST message by decoding it and sending parent information.
 *
 * @param msg - The message containing the PDR.
 * @return void
 */
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
 * handleParentInfoResponse
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
* handleChildRegistrationRequest
* Handles a CHILD_REGISTRATION_REQUEST message: decodes the message, updates the children and routing tables,
 * sends the current routing information to the child, propagates the new node details throughout the network,
 * and reports the newly registered node to the visualization server.
*
* @param msg - The message to decode.
* @return void
*/
void handleChildRegistrationRequest(char * msg){
    int type;
    int childAPIP[4], childSTAIP[4],sequenceNumber;
    char messageBuffer1[50] = "", messageBufferLarge[300] = "";
    routingTableEntry newNode;
    messageParameters parameters;

    sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d %d", &type, &childAPIP[0],&childAPIP[1],&childAPIP[2],&childAPIP[3],
           &childSTAIP[0],&childSTAIP[1],&childSTAIP[2],&childSTAIP[3], &sequenceNumber);


    // If the node is already registered as my child, do not increment the number of children.
    if(findNode(childrenTable,childAPIP) == nullptr){
        //Increase my number of children
        numberOfChildren++;
    }

    //Add the new children to the children table
    updateChildrenTable(childAPIP, childSTAIP);

    //Add the child node to the routing table
    newNode.nextHopIP[0] = childAPIP[0];newNode.nextHopIP[1] = childAPIP[1];
    newNode.nextHopIP[2] = childAPIP[2];newNode.nextHopIP[3] = childAPIP[3];
    newNode.hopDistance = 1;
    updateRoutingTable(childAPIP, newNode, childAPIP);

    LOG(NETWORK,INFO, "New child connected.\n");

    //Send my routing table to my child
    //LOG(MESSAGES,INFO,"Sending my routing Table to child:");
    LOG(MESSAGES, INFO, "Sending [Full Routing Update] to: %d.%d.%d.%d\n",childAPIP[0], childAPIP[1], childAPIP[2], childAPIP[3]);
    assignIP(parameters.senderIP,myIP);
    encodeMessage(messageBufferLarge,FULL_ROUTING_TABLE_UPDATE,parameters);
    //LOG(MESSAGES,INFO,"%s to -> %d.%d.%d.%d\n",messageBufferLarge, childSTAIP[0], childSTAIP[1], childSTAIP[2], childSTAIP[3]);
    sendMessage(childSTAIP,messageBufferLarge);

    //Propagate the new node information trough the network
    assignIP(parameters.IP1,childAPIP);
    assignIP(parameters.IP2,childAPIP);
    parameters.hopDistance = 1;
    parameters.sequenceNumber = sequenceNumber;
    assignIP(parameters.senderIP,myIP);
    encodeMessage(messageBuffer1, PARTIAL_ROUTING_TABLE_UPDATE, parameters);
    propagateMessage(messageBuffer1, childAPIP);

    //Sending new node information to the DEBUG visualization program, if enabled
    reportNewNodeToViz(childAPIP, myIP);

}

/**
* handleFullRoutingTableUpdate
* Handles a FULL_ROUTING_TABLE_UPDATE: decodes the message and updates the routing table.
*
* @param msg - The message to decode.
* @return void
*/
void handleFullRoutingTableUpdate(char * msg){
    int type;
    int nodeIP[4], nextHopIP[4], sourceIP[4];
    int hopDistance,sequenceNumber;
    messageParameters parameters;
    char messageBuffer1[300];
    bool hasRoutingChanged = false;
    //Parse Message Type and root node IP
    sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d", &type,&sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],&rootIP[0],&rootIP[1],&rootIP[2],&rootIP[3]);

    char* token = strtok(msg, "|");

    //To discard the message type and ensure the token points to the first routing table update entry
    token = strtok(NULL, "|");

    while (token != NULL) {
        sscanf(token, "%d.%d.%d.%d %d %d",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&hopDistance,&sequenceNumber);
        //Serial.printf("Token: %s\n", token);

        //Serial.printf("Parsed IP values: nodeIP %d.%d.%d.%d nextHopIp %d.%d.%d.%d hopDistance %d\n",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3],
                     // nextHopIP[0],nextHopIP[1],nextHopIP[2],nextHopIP[3], hopDistance);
        //Update the Routing Table
        hasRoutingChanged = hasRoutingChanged || updateRoutingTable2(nodeIP,hopDistance,sequenceNumber,sourceIP);
        token = strtok(NULL, "|");
    }

    if (hasRoutingChanged){
        LOG(NETWORK,INFO, "Routing Information has changed->propagate new info\n");
        //Propagate the routing table update information trough the network
        assignIP(parameters.senderIP,myIP);
        encodeMessage(messageBuffer1, FULL_ROUTING_TABLE_UPDATE, parameters);
        propagateMessage(messageBuffer1, sourceIP);
    }


}

/**
* handlePartialRoutingUpdate
* Handles a PARTIAL_ROUTING_TABLE_UPDATE: decodes the message, updates the routing table and propagates the routing update
* to other network nodes .
*
* @param msg - The message to decode.
* @return void
*/
void handlePartialRoutingUpdate(char *msg){
    int type;
    int nodeIP[4], nextHopIP[4], senderIP[4],sequenceNumber;
    int hopDistance;
    char messageBuffer1[50] = "";
    bool hasRoutingChanged = false;
    routingTableEntry newNode;
    messageParameters parameters;

    sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d %d %d",&type,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3],
           &nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&hopDistance,&sequenceNumber);

    newNode.hopDistance = hopDistance;

    hasRoutingChanged = updateRoutingTable2(nodeIP,hopDistance,sequenceNumber, senderIP);

    // If the routing update caused a change in my routing table, propagate the updated information to the rest of the network
    if(hasRoutingChanged){
        LOG(NETWORK,INFO, "Routing Information has changed->propagate new info\n");
        routingTableEntry*nodeEntry = (routingTableEntry*) findNode(routingTable,nodeIP);
        if(nodeEntry != nullptr){
            //Propagate the routing table update information trough the network
            assignIP(parameters.IP1,nodeIP);
            parameters.hopDistance = nodeEntry->hopDistance;
            assignIP(parameters.senderIP,myIP);
            parameters.sequenceNumber = sequenceNumber;
            encodeMessage(messageBuffer1, PARTIAL_ROUTING_TABLE_UPDATE, parameters);
            propagateMessage(messageBuffer1, senderIP);
        }else{
            LOG(NETWORK,ERROR, "❌ Routing Table Update Failed: The node in the routing update was not"
                               " properly added to the routing table. The table lookup returned a null pointer.");
        }
    }

}

void handleTopologyBreakAlert(char *msg){
    //TODO set a variable as to my tree broken
    int type, senderIP[4];
    sscanf(msg, "%d %d.%d.%d.%d",&type,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]);
    // Propagate the message to my children
    propagateMessage(msg, senderIP);
}
/**
 * handleDebugMessage
 * Handles a DEBUG_MESSAGE: decodes the message, forwards it to the appropriate next hop or debug server.
 *
 * @param msg - The message to decode.
 * @return void
 */
void handleDebugMessage(char* msg){
    int nextHopIP[4];

    //If this message is not intended for this node, forward it to the next hop leading to its destination.
    if(!iamRoot){
        int* nextHopPtr = findRouteToNode(rootIP);
        if (nextHopPtr != nullptr){
            assignIP(nextHopIP, nextHopPtr);
        }else{
            LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                                "Unable to forward message.\n", rootIP[0], rootIP[1],rootIP[2], rootIP[3]);
        }
        sendMessage(nextHopIP,messageBuffer);
    }else{//send message to debug server
        LOG(DEBUG_SERVER,DEBUG,msg);
        //sendMessage(debugServerIP, messageBuffer);
    }

}

/**
* handleDataMessage
* Handles a DATA_MESSAGE: decodes the message, determines the next hop for forwarding, or if it is the final node, processes the message and sends an ACK back to the source node.
*
* @param msg - The message to decode
* @return void
*/
void handleDataMessage(char *msg){
    int type;
    int sourceIP[4], destinationIP[4], nextHopIP[4];
    int *nextHopPtr = nullptr;
    char payload[50];
    routingTableEntry newNode;
    messageParameters parameters;

    sscanf(msg, "%d %s %d.%d.%d.%d %d.%d.%d.%d",&type, payload, &sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],
        &destinationIP[0],&destinationIP[1],&destinationIP[2],&destinationIP[3]);
    //Serial.printf("Message %s received from %d.%d.%d.%d to %d.%d.%d.%d", payload, senderIP[0],senderIP[1],senderIP[2],senderIP[3],
        //destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3]);

    //Find the route to the destination IP of the message
    nextHopPtr = findRouteToNode(destinationIP);
    if (nextHopPtr != nullptr){
        assignIP(nextHopIP, nextHopPtr);
    }else{
        LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                            "Unable to forward message.\n", destinationIP[0], destinationIP[1],destinationIP[2], destinationIP[3]);
    }

    // If this message is not intended for this node, forward it to the next hop leading to its destination.
    if(!isIPEqual(nextHopIP, myIP)){
        sendMessage(nextHopIP,messageBuffer);
    }else{// If the message is for this node, process it and send an ACK back to the source

        //TODO process the message

        //Send ACK Message back to the source of the message
        assignIP(parameters.IP1,sourceIP);
        assignIP(parameters.IP2,destinationIP);
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
 * handleAckMessage
 * Handles an ACK_MESSAGE: decodes the message, determines the next hop for message forwarding, or if it is the final
 * node, processes the ACK.
 *
 * @param msg - The message to decode.
 * @return void
 */
void handleAckMessage(char *msg){
    int type;
    int nextHopIP[4], sourceIP[4], destinationIP[4];
    int *nextHopPtr = nullptr;


    sscanf(msg, "%d %d.%d.%d.%d %d.%d.%d.%d",&type, &sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],
           &destinationIP[0],&destinationIP[1],&destinationIP[2],&destinationIP[3]);
    //Serial.printf("Message %s received from %d.%d.%d.%d to %d.%d.%d.%d", payload, senderIP[0],senderIP[1],senderIP[2],senderIP[3],
    //destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3]);
    nextHopPtr = findRouteToNode(destinationIP);
    if (nextHopPtr != nullptr){
        assignIP(nextHopIP, nextHopPtr);
    }else{
        LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                            "Unable to forward message.\n", destinationIP[0], destinationIP[1],destinationIP[2], destinationIP[3]);
    }

    if(!isIPEqual(nextHopIP, myIP)){
        sendMessage(nextHopIP,messageBuffer);
    }else{
        //TODO process the ACK
    }

}

/**
 * handleParentReassignmentCommand
 * Handles a PARENT_REASSIGNMENT_COMMAND message: decodes the message, ....
 *
 * @param msg - The message to decode.
 */
void handleParentReassignmentCommand(char *msg, int *newParentIP){

    //ToDo put event on the queue to change parent

}

/**
 * handleParentListAdvertisement
 * Handles a PARENT_LIST_ADVERTISEMENT: decodes the message, ...
 *
 * @param msg - The message to decode.
 * @return void
 */
void handleParentListAdvertisement(char *msg){
    int type;

    //ToDo Callback to choose the node new parent

}

/**
 * handleDebugRegistrationRequest
 * Handles a DEBUG_REGISTRATION_REQUEST: decodes the message, ...
 *
 * @param msg - The message to decode.
 * @return void
 */
void handleDebugRegistrationRequest(char* msg){
    int type;
    //sscanf(msg, "%d %d.%d.%d.%d",&type, &debugServerIP[0],&debugServerIP[1],&debugServerIP[2],&debugServerIP[3]);
}

/**
 * propagateMessage
 * Propagates the message to other nodes in the network, sending it to all connected nodes except the one that
 * sent the message for propagation
 *
 * @param msg - The message to propagate.
 * @param sourceIP - The IP address of the node that sent me the message.
 * @return void
 */
void propagateMessage(char* message, int* sourceIP){
    // If the message didn't come from the parent and i have a parent, forward it to the parent
    //LOG(MESSAGES, DEBUG, "SourceIP: %i.%i.%i.%i\nParentIP: %i.%i.%i.%i hasParent: %i\n",sourceIP[0], sourceIP[1],sourceIP[2],sourceIP[3],
    //   parent[0],parent[1],parent[2],parent[3],hasParent);
    routingTableEntry *childRoutingEntry;
    routingTableEntry *parentRoutingEntry = (routingTableEntry*)findNode(routingTable, parent);
    //Send the message to my parent only if it exists and is reachable
    if(parentRoutingEntry != nullptr){
        if(!isIPEqual(sourceIP, parent) && parentRoutingEntry->hopDistance != -1){
            LOG(MESSAGES, DEBUG, "Propagating Message to parent: %i.%i.%i.%i\n", parent[0],parent[1],parent[2],parent[3]);
            sendMessage(parent, message);
        }
    }

    //Forward the message to all children except the one that sent it to me
    for(int i = 0; i< childrenTable->numberOfItems; i++){
        childRoutingEntry = (routingTableEntry*)findNode(routingTable, (int*)childrenTable->table[i].key);
        //LOG(MESSAGES, DEBUG, "SourceIP: %i.%i.%i.%i ChildIP: %i.%i.%i.%i\n",sourceIP[0], sourceIP[1],sourceIP[2],sourceIP[3],
        //        ((int*)childrenTable->table[i].key)[0],((int*)childrenTable->table[i].key)[1],((int*)childrenTable->table[i].key)[2],((int*)childrenTable->table[i].key)[3]);
        if (childRoutingEntry != nullptr){
            if(!isIPEqual((int*)childrenTable->table[i].key, sourceIP) && childRoutingEntry->hopDistance != -1){
                LOG(MESSAGES, DEBUG, "Propagating Message to: %i.%i.%i.%i\n", ((int*)childrenTable->table[i].key)[0],((int*)childrenTable->table[i].key)[1],((int*)childrenTable->table[i].key)[2],((int*)childrenTable->table[i].key)[3]);
                sendMessage((int*)childrenTable->table[i].value, message);
            }
        }

    }

}