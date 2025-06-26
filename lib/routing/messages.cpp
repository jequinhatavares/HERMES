#include "messages.h"

char receiveBuffer[256] = "";
char largeSendBuffer[255] = "";
char smallSendBuffer[50] = "";


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
void encodeMessage(char * msg, size_t bufferSize, messageType type, messageParameters parameters){
    char tempMsg[40] = "";//35
    routingTableEntry *nodeRoutingEntry;
    switch (type) {
        case PARENT_DISCOVERY_REQUEST:
            //0 [mySTAIP]
            snprintf(msg,bufferSize,"%i %hhu.%hhu.%hhu.%hhu",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3]);
            break;
        case PARENT_INFO_RESPONSE:
            //1 [my AP IP] [my hop distance to the root] [my number of children]
            snprintf(msg,bufferSize,"%i %hhu.%hhu.%hhu.%hhu %i %i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.hopDistance, parameters.childrenNumber);
            break;
        case CHILD_REGISTRATION_REQUEST:
            //2 [my AP IP] [my STA IP] [my sequence number]
            snprintf(msg,bufferSize,"%i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %i",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3], parameters.sequenceNumber);
            break;
        case FULL_ROUTING_TABLE_UPDATE:
            //3 [senderIP] [rootIP] |[node1 IP] [hopDistance] [Sequence Number1]|[node2 IP] [hopDistance] [Sequence Number2]|....
            snprintf(msg,bufferSize,"%i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu |",type,myIP[0],myIP[1],myIP[2],
                    myIP[3],rootIP[0],rootIP[1],rootIP[2],rootIP[3]);

            for (int i = 0; i < routingTable->numberOfItems; i++) {
                snprintf(tempMsg,sizeof(tempMsg),"%hhu.%hhu.%hhu.%hhu %i %i|",((uint8_t *)routingTable->table[i].key)[0],
                        ((uint8_t *)routingTable->table[i].key)[1],((uint8_t *)routingTable->table[i].key)[2],
                        ((uint8_t *)routingTable->table[i].key)[3],((routingTableEntry *)routingTable->table[i].value)->hopDistance,
                        ((routingTableEntry *)routingTable->table[i].value)->sequenceNumber);

                strcat(msg, tempMsg);
                strcpy(tempMsg , "");
            }
            //Serial.printf("Formated msg: %s", msg);
            break;

        case PARTIAL_ROUTING_TABLE_UPDATE:
            //4 [senderIP] |[node1 IP] [hopDistance] [sequenceNumber]| [node2 IP] [hopDistance] [sequenceNumber] ...
            snprintf(msg,bufferSize,"%i %hhu.%hhu.%hhu.%hhu |",type,myIP[0],myIP[1],myIP[2],myIP[3]);
            for (int i = 0; i < parameters.nrOfNodes; i++){
                nodeRoutingEntry = (routingTableEntry*)tableRead(routingTable, parameters.IP[i]);
                if(nodeRoutingEntry != nullptr){
                    snprintf(tempMsg,sizeof(tempMsg),"%hhu.%hhu.%hhu.%hhu %i %i |",parameters.IP[i][0],parameters.IP[i][1],parameters.IP[i][2],parameters.IP[i][3],nodeRoutingEntry->hopDistance, nodeRoutingEntry->sequenceNumber);
                    strcat(msg, tempMsg);
                    strcpy(tempMsg , "");
                }
            }
            break;

        case TOPOLOGY_BREAK_ALERT:
            //7 [senderIP]
            snprintf(msg,bufferSize,"%i %hhu.%hhu.%hhu.%hhu",type,myIP[0],myIP[1],myIP[2],myIP[3]);
            break;

        case PARENT_RESET_NOTIFICATION:
            //8 [senderIP]
            snprintf(msg,bufferSize,"%i",type);
            break;

        case DEBUG_MESSAGE:
            //10 [DEBUG message payload]
            snprintf(msg,bufferSize,"%i %s\n",type,parameters.payload);
            break;

        case DATA_MESSAGE:
            //11 [source node IP] [destination node IP] [message payload]
            snprintf(msg,bufferSize,"%i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %s",type,parameters.IP1[0],parameters.IP1[1],
                    parameters.IP1[2],parameters.IP1[3],parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3],parameters.payload);
            break;

        case ACK_MESSAGE:
            //12 [source node IP] [destination node IP]
            snprintf(msg,bufferSize,"%i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",type,parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],
                    parameters.IP1[3],parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;

        case MIDDLEWARE_MESSAGE:
            snprintf(msg, bufferSize, "%i ",type);
            break;
        default:
            break;
    }
}

void encodeDataMessage(char* messageBuffer, size_t bufferSize,char* payload,uint8_t *sourceIP,uint8_t *destinationIP){
    //11 [source node IP] [destination node IP] [message payload]
    snprintf(messageBuffer,bufferSize,"%i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %s",DATA_MESSAGE,sourceIP[0],sourceIP[1],
             sourceIP[2],sourceIP[3],destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3],payload);
}

bool isMessageValid(int expectedMessageType,char* msg){
    int type, parsedFields=0;
    // First, check if the message starts with an integer, as all valid messages do
    if (sscanf(msg, "%i", &type) != 1) {
        return false; // Cannot even extract the type
    }

    //If the message hasn't from the expected type
    if(type != expectedMessageType){
        return false;
    }

    switch (type) {
        case PARENT_DISCOVERY_REQUEST: {
            uint8_t IP[4];
            parsedFields = sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu", &type, &IP[0], &IP[1], &IP[2], &IP[3]);
            if(parsedFields != 5){
                return false;
            }
            break;
        }
        case PARENT_INFO_RESPONSE: {
            uint8_t IP[4];
            int hopDistance, children;
            parsedFields = sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %d %d", &type, &IP[0], &IP[1], &IP[2], &IP[3], &hopDistance, &children);
            if(parsedFields != 7){
                LOG(MESSAGES, ERROR, "Invalid PARENT_INFO_RESPONSE\n");
                return false;
            }
            return true;
        }
        case CHILD_REGISTRATION_REQUEST: {
            uint8_t IP1[4], IP2[4];
            int seqNum;
            return (sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %d", &type, &IP1[0], &IP1[1], &IP1[2], &IP1[3],
                           &IP2[0], &IP2[1], &IP2[2], &IP2[3], &seqNum) == 10);
            break;
        }
        case FULL_ROUTING_TABLE_UPDATE: {
            uint8_t IP1[4], IP2[4];
            int hopDistance,sequenceNumber;
            char msgCopy[255];
            strcpy(msgCopy, msg);

            if(sscanf(msgCopy, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu", &type, &IP1[0], &IP1[1], &IP1[2], &IP1[3],&IP2[0], &IP2[1], &IP2[2], &IP2[3]) != 9)return false;
            char* token = strtok(msgCopy, "|");

            token = strtok(NULL, "|");

            while (token != NULL) {
                if(sscanf(token, "%hhu.%hhu.%hhu.%hhu %d %d",&IP1[0],&IP1[1],&IP1[2],&IP1[3],&hopDistance,&sequenceNumber) != 6) return false;
                token = strtok(NULL, "|");
            }
            return true;
            break;
        }
        case PARTIAL_ROUTING_TABLE_UPDATE: {
            uint8_t IP1[4];
            int hopDistance,sequenceNumber;
            char msgCopy[255];
            strcpy(msgCopy, msg);
            if(sscanf(msgCopy, "%d %hhu.%hhu.%hhu.%hhu", &type, &IP1[0], &IP1[1], &IP1[2], &IP1[3]) != 5)return false;
            char* token = strtok(msgCopy, "|");

            token = strtok(NULL, "|");

            while (token != NULL) {
                if(sscanf(token, "%hhu.%hhu.%hhu.%hhu %d %d",&IP1[0],&IP1[1],&IP1[2],&IP1[3],&hopDistance,&sequenceNumber) != 6) return false;
                token = strtok(NULL, "|");
            }
            return true;
            break;
        }
        case TOPOLOGY_BREAK_ALERT: {
            /***int IP[4];
            return (sscanf(msg, "%d %d.%d.%d.%d", &type, &IP[0], &IP[1], &IP[2], &IP[3]) == 5);***/
            return true;
            break;
        }
        case PARENT_RESET_NOTIFICATION:
            //8 [senderIP]
            return(sscanf(msg,"%d",&type)== 1);
            break;

        case DEBUG_MESSAGE: {
            /***int dummy;
            char payload[100];
            return (sscanf(msg, "%d %[^\n]", &dummy, payload) == 2);***/
            return true;
            break;
        }
        case DATA_MESSAGE: {
            uint8_t IP1[4], IP2[4];
            char payload[200];
            return (sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %199s", &type, &IP1[0], &IP1[1], &IP1[2], &IP1[3],
                           &IP2[0], &IP2[1], &IP2[2], &IP2[3], payload) == 10);
            break;
        }
        case ACK_MESSAGE: {
            uint8_t IP1[4], IP2[4];
            return (sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu", &type, &IP1[0], &IP1[1], &IP1[2], &IP1[3],
                           &IP2[0], &IP2[1], &IP2[2], &IP2[3]) == 9);
            break;
        }
        case MIDDLEWARE_MESSAGE:
            return true;

        default: // The message is not one of the valid message types
            return false;
    }
    return true;
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
    uint8_t childIP[4];
    messageParameters parameters;
    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu", &messageType, &childIP[0], &childIP[1], &childIP[2], &childIP[3]);

    //Send my information(IP, nr of children and root hop distance) to the node requesting it
    parameters.IP1[0] = myIP[0]; parameters.IP1[1] = myIP[1]; parameters.IP1[2] = myIP[2]; parameters.IP1[3] = myIP[3];
    parameters.childrenNumber = numberOfChildren;
    parameters.hopDistance = rootHopDistance;

    encodeMessage(smallSendBuffer,sizeof(smallSendBuffer),PARENT_INFO_RESPONSE,parameters);
    sendMessage(childIP,smallSendBuffer);

    strcpy(smallSendBuffer , "");

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
    sscanf(msg, "%d", &messageType);

    if (messageType == PARENT_INFO_RESPONSE){
            sscanf(msg, "%d %d.%d.%d.%d %d %d", &messageType, &parentIP[0],&parentIP[1],&parentIP[2],&parentIP[3],&rootDistance,&nrChildren);
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
    uint8_t childAPIP[4], childSTAIP[4];
    int sequenceNumber;
    routingTableEntry newNode;
    messageParameters parameters;

    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %d", &type, &childAPIP[0],&childAPIP[1],&childAPIP[2],&childAPIP[3],
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
    newNode.hopDistance = 0;
    newNode.sequenceNumber = sequenceNumber;
    //updateRoutingTable(childAPIP, newNode, childAPIP);
    updateRoutingTableSN(childAPIP,0,sequenceNumber,childAPIP);

    LOG(NETWORK,INFO, "New child connected.\n");

    //Send my routing table to my child
    //LOG(MESSAGES,INFO,"Sending my routing Table to child:");
    encodeMessage(largeSendBuffer,sizeof(largeSendBuffer),FULL_ROUTING_TABLE_UPDATE,parameters);
    LOG(MESSAGES, INFO, "Sending [Full Routing Update]:%s to: %d.%d.%d.%d\n",largeSendBuffer,childAPIP[0], childAPIP[1], childAPIP[2], childAPIP[3]);
    sendMessage(childSTAIP,largeSendBuffer);

    //Propagate the new node information trough the network
    assignIP(parameters.IP[0], childAPIP);
    parameters.nrOfNodes = 1;

    encodeMessage(smallSendBuffer,sizeof(smallSendBuffer), PARTIAL_ROUTING_TABLE_UPDATE, parameters);
    //LOG(MESSAGES,INFO,"Sending [PARTIAL ROUTING TABLE UPDATE] message: \"%s\"\n", smallSendBuffer);
    propagateMessage(smallSendBuffer,  childAPIP);
    //Sending new node information to the DEBUG visualization program, if enabled
    reportNewNodeToViz(childAPIP, myIP);

    strcpy(smallSendBuffer , "");
    strcpy(largeSendBuffer , "");

}

/**
* handleFullRoutingTableUpdate
* Handles a FULL_ROUTING_TABLE_UPDATE: decodes the message and updates the routing table.
*
* @param msg - The message to decode.
* @return void
*/
void handleFullRoutingTableUpdate(char * msg){
    int type, nrOfChanges = 0;
    uint8_t nodeIP[4], sourceIP[4];
    int hopDistance,sequenceNumber;
    messageParameters parameters;
    bool isRoutingTableChanged = false, isRoutingEntryChanged = false ;
    //Parse Message Type and root node IP
    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu", &type,&sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],&rootIP[0],&rootIP[1],&rootIP[2],&rootIP[3]);

    char* token = strtok(msg, "|");

    //To discard the message type and ensure the token points to the first routing table update entry
    token = strtok(NULL, "|");

    while (token != NULL) {
        sscanf(token, "%hhu.%hhu.%hhu.%hhu %d %d",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&hopDistance,&sequenceNumber);
        //Serial.printf("Token: %s\n", token);

        //Serial.printf("Parsed IP values: nodeIP %d.%d.%d.%d nextHopIp %d.%d.%d.%d hopDistance %d\n",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3],
                     // nextHopIP[0],nextHopIP[1],nextHopIP[2],nextHopIP[3], hopDistance);
        //Update the Routing Table
        //updateRoutingTableSN(nodeIP,newNode,sourceIP);
        isRoutingEntryChanged = updateRoutingTableSN(nodeIP,hopDistance,sequenceNumber,sourceIP);
        // If the node's routing entry was modified, add it to the list of nodes to include in the Partial routing update
        if(isRoutingEntryChanged == true){
            assignIP(parameters.IP[nrOfChanges], nodeIP);
            nrOfChanges ++;
        }
        isRoutingTableChanged = isRoutingTableChanged || isRoutingEntryChanged ;
        token = strtok(NULL, "|");
    }

    if (isRoutingTableChanged){
        parameters.nrOfNodes = nrOfChanges;
        LOG(NETWORK,INFO, "Routing Information has changed-> Propagate new info\n");
        //Propagate the routing table update information trough the network
        encodeMessage(largeSendBuffer,sizeof(largeSendBuffer), PARTIAL_ROUTING_TABLE_UPDATE, parameters);
        propagateMessage(largeSendBuffer, sourceIP);
    }

    strcpy(largeSendBuffer , "");


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
    int type, nrOfChanges = 0;
    uint8_t nodeIP[4], senderIP[4];
    int sequenceNumber;
    int hopDistance;
    bool isRoutingTableChanged = false, isRoutingEntryChanged = false;
    messageParameters parameters;

    //Parse Message Type and senderIP
    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu", &type,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]);

    char* token = strtok(msg, "|");

    //To discard the message type and ensure the token points to the first routing table update entry
    token = strtok(NULL, "|");

    while (token != NULL) {
        sscanf(token, "%hhu.%hhu.%hhu.%hhu %d %d",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&hopDistance,&sequenceNumber);
        //Serial.printf("Token: %s\n", token);

        //Update the Routing Table
        isRoutingEntryChanged = updateRoutingTableSN(nodeIP,hopDistance,sequenceNumber,senderIP);
        // If the node's routing entry was modified, add it to the list of nodes to include in the Partial routing update
        if(isRoutingEntryChanged == true){
            assignIP(parameters.IP[nrOfChanges],nodeIP);
            nrOfChanges ++;
        }
        //updateRoutingTable(nodeIP,newNode,sourceIP);
        isRoutingTableChanged = isRoutingTableChanged || isRoutingEntryChanged ;
        token = strtok(NULL, "|");
    }

    // If the routing update caused a change in my routing table, propagate the updated information to the rest of the network
    if(isRoutingTableChanged){
        LOG(NETWORK,INFO, "Routing Information has changed->propagate new info\n");
        routingTableEntry*nodeEntry = (routingTableEntry*) findNode(routingTable,nodeIP);
        if(nodeEntry != nullptr){
            //Propagate the routing table update information trough the network
            encodeMessage(largeSendBuffer,sizeof(largeSendBuffer), PARTIAL_ROUTING_TABLE_UPDATE, parameters);
            propagateMessage(largeSendBuffer, senderIP);
        }else{
            LOG(NETWORK,ERROR, "❌ Routing Table Update Failed: The node in the routing update was not"
                               " properly added to the routing table. The table lookup returned a null pointer.");
        }
    }
    strcpy(largeSendBuffer , "");

}

void handleTopologyBreakAlert(char *msg){
    //TODO set a variable as to my tree broken
    int type;
    uint8_t senderIP[4];
    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu",&type,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]);
    // Propagate the message to my children
    propagateMessage(msg, senderIP);
}

void handleParentResetNotification(char *msg){

    // Permanently disconnect from the parent node
    //disconnectFromAP();
}
/**
 * handleDebugMessage
 * Handles a DEBUG_MESSAGE: decodes the message, forwards it to the appropriate next hop or debug server.
 *
 * @param msg - The message to decode.
 * @return void
 */
void handleDebugMessage(char* msg){
    uint8_t nextHopIP[4];

    //If this message is not intended for this node, forward it to the next hop leading to its destination.
    if(!iamRoot){
        uint8_t * nextHopPtr = findRouteToNode(rootIP);
        if (nextHopPtr != nullptr){
            assignIP(nextHopIP, nextHopPtr);
        }else{
            LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                                "Unable to forward message.\n", rootIP[0], rootIP[1],rootIP[2], rootIP[3]);
        }
        sendMessage(nextHopIP,receiveBuffer);
    }else{//send message to debug server
        LOG(DEBUG_SERVER,DEBUG,msg);
        //sendMessage(debugServerIP, receiveBuffer);
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
    int type, nChars=0;
    uint8_t sourceIP[4], destinationIP[4], nextHopIP[4];
    uint8_t *nextHopPtr = nullptr;
    char payload[200];
    bool isTunneled = false;
    messageParameters parameters;

    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %n",&type, &sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],
        &destinationIP[0],&destinationIP[1],&destinationIP[2],&destinationIP[3],&nChars);
    //Serial.printf("Message %s received from %d.%d.%d.%d to %d.%d.%d.%d", payload, senderIP[0],senderIP[1],senderIP[2],senderIP[3],
    //destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3]);

    // Copy the rest of the string manually
    strncpy(payload, msg + nChars, sizeof(payload) - 1);

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
        sendMessage(nextHopIP,receiveBuffer);
    }else{// If the message is for this node, process it and send an ACK back to the source

        //TODO process the message
        isTunneled = isMessageTunneled(msg);
        if(isTunneled)LOG(MESSAGES,INFO,"Tunneled Message arrived\n");

        //Send ACK Message back to the source of the message
        assignIP(parameters.IP1,myIP);
        assignIP(parameters.IP2,sourceIP);
        encodeMessage(smallSendBuffer, sizeof(smallSendBuffer),ACK_MESSAGE, parameters);

        nextHopPtr = findRouteToNode(sourceIP);
        if (nextHopPtr != nullptr){
            sendMessage(nextHopPtr,smallSendBuffer);
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
    uint8_t nextHopIP[4], sourceIP[4], destinationIP[4];
    uint8_t *nextHopPtr = nullptr;


    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",&type, &sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],
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
        sendMessage(nextHopIP,receiveBuffer);
    }else{
        //TODO process the ACK
    }

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
 * handleDebugRegistrationRequest
 * Handles a DEBUG_REGISTRATION_REQUEST: decodes the message, ...
 *
 * @param msg - The message to decode.
 * @return void
 */


/**
 * propagateMessage
 * Propagates the message to other nodes in the network, sending it to all connected nodes except the one that
 * sent the message for propagation
 *
 * @param msg - The message to propagate.
 * @param sourceIP - The IP address of the node that sent me the message.
 * @return void
 */
void propagateMessage(char* message, uint8_t * sourceIP){
    // If the message didn't come from the parent and i have a parent, forward it to the parent
    //LOG(MESSAGES, DEBUG, "SourceIP: %i.%i.%i.%i\nParentIP: %i.%i.%i.%i hasParent: %i\n",sourceIP[0], sourceIP[1],sourceIP[2],sourceIP[3],
    //   parent[0],parent[1],parent[2],parent[3],hasParent);
    routingTableEntry *childRoutingEntry;
    routingTableEntry *parentRoutingEntry = (routingTableEntry*)findNode(routingTable, parent);
    //Send the message to my parent only if it exists and is reachable
    if(parentRoutingEntry != nullptr){
        if(!isIPEqual(sourceIP, parent) && parentRoutingEntry->hopDistance != -1){
            LOG(MESSAGES, DEBUG, "Propagating Message to parent: %d.%d.%d.%d\n",parent[0],parent[1],parent[2],parent[3]);
            sendMessage(parent, message);
        }
    }

    //Forward the message to all children except the one that sent it to me
    for(int i = 0; i< childrenTable->numberOfItems; i++){
        childRoutingEntry = (routingTableEntry*)findNode(routingTable, (uint8_t *)childrenTable->table[i].key);
        //LOG(MESSAGES, DEBUG, "SourceIP: %i.%i.%i.%i ChildIP: %i.%i.%i.%i\n",sourceIP[0], sourceIP[1],sourceIP[2],sourceIP[3],
        //        ((int*)childrenTable->table[i].key)[0],((int*)childrenTable->table[i].key)[1],((int*)childrenTable->table[i].key)[2],((int*)childrenTable->table[i].key)[3]);
        if (childRoutingEntry != nullptr){
            if(!isIPEqual((int*)childrenTable->table[i].key, sourceIP) && childRoutingEntry->hopDistance != -1){
                LOG(MESSAGES, DEBUG, "Propagating Message to: %i.%i.%i.%i\n", ((uint8_t *)childrenTable->table[i].key)[0],((uint8_t *)childrenTable->table[i].key)[1],((uint8_t *)childrenTable->table[i].key)[2],((uint8_t *)childrenTable->table[i].key)[3]);
                sendMessage((uint8_t *)childrenTable->table[i].value, message);
            }
        }

    }

}

void encodeTunneledMessage(char* encodedMessage,size_t encodedMessageSize,uint8_t sourceIP[4], uint8_t destinationIP[4], char* encapsulatedMessage){
    messageParameters params;
    assignIP(params.IP1,sourceIP);
    assignIP(params.IP2,destinationIP);
    strncpy(params.payload,encapsulatedMessage,sizeof(params.payload)-1);
    encodeMessage(encodedMessage, encodedMessageSize,DATA_MESSAGE,params);
}

bool isMessageTunneled(char* dataMessage){
    messageType type;
    //int sourceIP[4],originalDestinationIP[4];
    return( sscanf(dataMessage,"%d %*u.%*u.%*u.%*u %*u.%*u.%*u.%*u %d %*u.%*u.%*u.%*u %*u.%*u.%*u.%*u",&type,&type) ==2 );
}

bool waitForMessage(messageType type, uint8_t expectedSenderIP[4], unsigned long timeOut){
    int packetSize;
    uint8_t receivedSenderIP[4];
    bool isExpectedMessage = false;
    //Wait for the parent to respond
    unsigned long startTime = getCurrentTime();
    unsigned long currentTime = startTime;
    while( ((currentTime - startTime) <=timeOut) && !isExpectedMessage ){
        packetSize = receiveMessage(receiveBuffer, sizeof(receiveBuffer));
        currentTime = getCurrentTime();
        if(packetSize>0){
            //Verify if the received message is from the expected type
            if(isMessageValid(type,receiveBuffer)){
                //LOG(MESSAGES,INFO,"Valid Message type\n");
                getSenderIP(receiveBuffer, type,receivedSenderIP);
                //LOG(MESSAGES,INFO,"Parsed Received IP: %i.%i.%i.%i\n",receivedSenderIP[0],receivedSenderIP[1],receivedSenderIP[2],receivedSenderIP[3]);
                //Verify if sender of the message corresponds to the expected sender
                if(isIPEqual(expectedSenderIP,receivedSenderIP)){
                    //LOG(MESSAGES,INFO,"Valid Sender IP\n");
                    isExpectedMessage = true;
                }
            }
        }
    }
    return isExpectedMessage;
}

void getSenderIP(char* messageBuffer, messageType type, uint8_t * senderIP){
    switch (type) {
        case PARENT_INFO_RESPONSE:
            sscanf(messageBuffer, "%*d %hhu.%hhu.%hhu.%hhu",&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]);
            break;
        case FULL_ROUTING_TABLE_UPDATE:
            sscanf(messageBuffer, "%*d %hhu.%hhu.%hhu.%hhu",&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]);
            break;
        default:
            break;
    }
}

void sendMessageToNode(char* messageBuffer,uint8_t *destinationIP){
    uint8_t *nextHopIP = findRouteToNode(destinationIP);
    if(nextHopIP != nullptr){
        sendMessage(destinationIP,messageBuffer);
    }
}


void sendDataMessageToNode(char* messageBuffer,uint8_t *senderIP,uint8_t *destinationIP){
    //encodeDataMessage(la,messageBuffer);
    uint8_t *nextHopIP = findRouteToNode(destinationIP);
    if(nextHopIP != nullptr){
        sendMessage(destinationIP,messageBuffer);
    }
}

