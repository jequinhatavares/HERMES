#include "messages.h"

void (*onDataMessageCallback)(uint8_t*,uint8_t *,char*) = nullptr;
void (*onACKMessageCallback)(uint8_t*,uint8_t *,char*) = nullptr;

char receiveBuffer[256] = "";
size_t receivePayload=0;

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
/***void encodeMessage(char * msg, size_t bufferSize, MessageType type, messageParameters parameters){
    char tempMsg[40] = "";//35
    RoutingTableEntry *nodeRoutingEntry;
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
                        ((uint8_t *)routingTable->table[i].key)[3],((RoutingTableEntry *)routingTable->table[i].value)->hopDistance,
                        ((RoutingTableEntry *)routingTable->table[i].value)->sequenceNumber);

                strcat(msg, tempMsg);
                strcpy(tempMsg , "");
            }
            //Serial.printf("Formated msg: %s", msg);
            break;

        case PARTIAL_ROUTING_TABLE_UPDATE:
            //4 [senderIP] |[node1 IP] [hopDistance] [sequenceNumber]| [node2 IP] [hopDistance] [sequenceNumber] ...
            snprintf(msg,bufferSize,"%i %hhu.%hhu.%hhu.%hhu |",type,myIP[0],myIP[1],myIP[2],myIP[3]);
            for (int i = 0; i < parameters.nrOfNodes; i++){
                nodeRoutingEntry = (RoutingTableEntry*)tableRead(routingTable, parameters.IP[i]);
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

        case TOPOLOGY_RESTORED_NOTICE:
            //7 [senderIP]
            snprintf(msg,bufferSize,"%i %hhu.%hhu.%hhu.%hhu",type,myIP[0],myIP[1],myIP[2],myIP[3]);
            break;

        case PARENT_RESET_NOTIFICATION:
            //8 [senderIP]
            snprintf(msg,bufferSize,"%i",type);
            break;

        case MONITORING_MESSAGE:
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
}***/

void encodeParentDiscoveryRequest(char* messageBuffer, size_t bufferSize,uint8_t *STAIP) {
    //PARENT_DISCOVERY_REQUEST [mySTAIP]
    snprintf(messageBuffer,bufferSize,"%i %hhu.%hhu.%hhu.%hhu",PARENT_DISCOVERY_REQUEST,STAIP[0],STAIP[1],STAIP[2],STAIP[3]);
}

void encodeParentInfoResponse(char* messageBuffer, size_t bufferSize,uint8_t *APIP,int hopDistance,int childrenNumber){
    //PARENT_INFO_RESPONSE [my AP IP] [my hop distance to the root] [my number of children]
    snprintf(messageBuffer,bufferSize,"%i %hhu.%hhu.%hhu.%hhu %i %i",PARENT_INFO_RESPONSE,APIP[0],APIP[1],APIP[2],APIP[3],
             hopDistance, childrenNumber);
}

void encodeChildRegistrationRequest(char* messageBuffer, size_t bufferSize,uint8_t *APIP,uint8_t *STAIP,int sequenceNumber) {
    //CHILD_REGISTRATION_REQUEST [my AP IP] [my STA IP] [my sequence number]
    snprintf(messageBuffer,bufferSize,"%i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %i",CHILD_REGISTRATION_REQUEST,APIP[0],APIP[1],APIP[2],APIP[3],
             STAIP[0],STAIP[1],STAIP[2],STAIP[3],sequenceNumber);
}

void encodeFullRoutingTableUpdate(char* messageBuffer, size_t bufferSize){
    int offset=0;
    //Message Size: Header Max-33 Each Routing Entry Max-22
    //FULL_ROUTING_TABLE_UPDATE [senderIP] [rootIP]|[node1 IP] [hopDistance] [Sequence Number1]|[node2 IP] [hopDistance] [Sequence Number2]|....
    offset+=snprintf(messageBuffer+offset,bufferSize-offset,"%i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu|",FULL_ROUTING_TABLE_UPDATE,myIP[0],myIP[1],myIP[2],
             myIP[3],rootIP[0],rootIP[1],rootIP[2],rootIP[3]);


    for (int i = 0; i < routingTable->numberOfItems; i++) {
        offset+=snprintf(messageBuffer+offset,bufferSize-offset,"%hhu.%hhu.%hhu.%hhu %i %i|",((uint8_t *)routingTable->table[i].key)[0],
                 ((uint8_t *)routingTable->table[i].key)[1],((uint8_t *)routingTable->table[i].key)[2],
                 ((uint8_t *)routingTable->table[i].key)[3],((RoutingTableEntry *)routingTable->table[i].value)->hopDistance,
                 ((RoutingTableEntry *)routingTable->table[i].value)->sequenceNumber);

    }
}

void encodePartialRoutingUpdate(char* messageBuffer, size_t bufferSize,uint8_t nodeIPs[][4],int nrNodes){
    int offset=0;
    RoutingTableEntry *nodeRoutingEntry;
    //Message Size: Header Max-18 Each routing Entry Max-22
    //PARTIAL_ROUTING_TABLE_UPDATE [senderIP]|[node1 IP] [hopDistance] [sequenceNumber]|[node2 IP] [hopDistance] [sequenceNumber] ...
    offset += snprintf(messageBuffer+offset,bufferSize-offset,"%i %hhu.%hhu.%hhu.%hhu|",PARTIAL_ROUTING_TABLE_UPDATE,myIP[0],myIP[1],myIP[2],myIP[3]);

    for (int i = 0; i < nrNodes; i++){
        nodeRoutingEntry = (RoutingTableEntry*)tableRead(routingTable, nodeIPs[i]);
        if(nodeRoutingEntry != nullptr){
            offset += snprintf(messageBuffer+offset,bufferSize-offset,"%hhu.%hhu.%hhu.%hhu %i %i|",nodeIPs[i][0],nodeIPs[i][1],nodeIPs[i][2],nodeIPs[i][3],nodeRoutingEntry->hopDistance, nodeRoutingEntry->sequenceNumber);
        }
    }
}

void encodeTopologyBreakAlert(char* messageBuffer, size_t bufferSize){
    //TOPOLOGY_BREAK_ALERT [senderIP]
    snprintf(messageBuffer,bufferSize,"%i %hhu.%hhu.%hhu.%hhu",TOPOLOGY_BREAK_ALERT,myIP[0],myIP[1],myIP[2],myIP[3]);
}

void encodeTopologyRestoredNotice(char* messageBuffer, size_t bufferSize){
    //TOPOLOGY_RESTORED_NOTICE [senderIP]
    snprintf(messageBuffer,bufferSize,"%i %hhu.%hhu.%hhu.%hhu",TOPOLOGY_RESTORED_NOTICE,myIP[0],myIP[1],myIP[2],myIP[3]);
}

void encodeParentResetNotification(char* messageBuffer, size_t bufferSize){
    //PARENT_RESET_NOTIFICATION [senderIP]
    snprintf(messageBuffer,bufferSize,"%i %hhu.%hhu.%hhu.%hhu",PARENT_RESET_NOTIFICATION,myIP[0],myIP[1],myIP[2],myIP[3]);
}

void encodeDebugMessage(char* messageBuffer, size_t bufferSize,char* payload){
    //10 [DEBUG message payload]
    snprintf(messageBuffer,bufferSize,"%i %s\n",MONITORING_MESSAGE,payload);
}


void encodeACKMessage(char* messageBuffer, size_t bufferSize,const char* payload,uint8_t *sourceIP,uint8_t *destinationIP){
    //ACK_MESSAGE [source node IP] [destination node IP] [ACK payload]
    //snprintf(messageBuffer,bufferSize,"%i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %s",ACK_MESSAGE,sourceIP[0],sourceIP[1],
      //       sourceIP[2],sourceIP[3],destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3],payload);
}


void encodeDataMessage(char* messageBuffer, size_t bufferSize,const char* payload,uint8_t *originatorIP,uint8_t *destinationIP){
    uint8_t broadcastIP[4]={255,255,255,255};
    if(!isIPEqual(destinationIP,broadcastIP)){
        //DATA_MESSAGE [originator node IP] [destination node IP] [message payload]
        snprintf(messageBuffer,bufferSize,"%i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %s",DATA_MESSAGE,originatorIP[0],originatorIP[1],
                 originatorIP[2],originatorIP[3],destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3],payload);
    }else{
        //DATA_MESSAGE [originator node IP] [destination node IP=broadcastIP] [sender IP] [message payload]
        snprintf(messageBuffer,bufferSize,"%i %hhu.%hhu.%hhu.%hhu 255.255.255.255 %hhu.%hhu.%hhu.%hhu %s",DATA_MESSAGE,originatorIP[0],originatorIP[1],
                 originatorIP[2],originatorIP[3],myIP[0],myIP[1],myIP[2],myIP[3],payload);
    }

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

            char* token = strtok(msgCopy, "|");

            if(sscanf(msgCopy, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu", &type, &IP1[0], &IP1[1], &IP1[2], &IP1[3],&IP2[0], &IP2[1], &IP2[2], &IP2[3]) != 9)return false;

            token = strtok(nullptr, "|");

            while (token != nullptr) {
                if(sscanf(token, "%hhu.%hhu.%hhu.%hhu %d %d",&IP1[0],&IP1[1],&IP1[2],&IP1[3],&hopDistance,&sequenceNumber) != 6) return false;
                token = strtok(nullptr, "|");
            }
            return true;
            break;
        }
        case PARTIAL_ROUTING_TABLE_UPDATE: {
            uint8_t IP1[4];
            int hopDistance,sequenceNumber;
            char msgCopy[255];
            strcpy(msgCopy, msg);

            char* token = strtok(msgCopy, "|");

            if(sscanf(msgCopy, "%d %hhu.%hhu.%hhu.%hhu", &type, &IP1[0], &IP1[1], &IP1[2], &IP1[3]) != 5)return false;

            token = strtok(nullptr, "|");

            while (token != nullptr) {
                if(sscanf(token, "%hhu.%hhu.%hhu.%hhu %d %d",&IP1[0],&IP1[1],&IP1[2],&IP1[3],&hopDistance,&sequenceNumber) != 6) return false;
                token = strtok(nullptr, "|");
            }
            return true;
            break;
        }
        case TOPOLOGY_BREAK_ALERT: {
            /******/int IP[4];
            return (sscanf(msg, "%d %d.%d.%d.%d", &type, &IP[0], &IP[1], &IP[2], &IP[3]) == 5);
            break;
        }

        case TOPOLOGY_RESTORED_NOTICE: {
            /******/int IP[4];
            return (sscanf(msg, "%d %d.%d.%d.%d", &type, &IP[0], &IP[1], &IP[2], &IP[3]) == 5);
            break;
        }
        case PARENT_RESET_NOTIFICATION:
            //8 [senderIP]
            return(sscanf(msg,"%d",&type)== 1);
            break;

        case MONITORING_MESSAGE: {
            /***int dummy;
            char payload[100];
            return (sscanf(msg, "%d %[^\n]", &dummy, payload) == 2);***/
            return true;
            break;
        }
        case DATA_MESSAGE: {
            uint8_t IP1[4], IP2[4];
            return (sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu", &type, &IP1[0], &IP1[1], &IP1[2], &IP1[3],
                           &IP2[0], &IP2[1], &IP2[2], &IP2[3]) == 9);
            break;
        }
        /***case ACK_MESSAGE: {
            uint8_t IP1[4], IP2[4];
            return (sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu", &type, &IP1[0], &IP1[1], &IP1[2], &IP1[3],
                           &IP2[0], &IP2[1], &IP2[2], &IP2[3]) == 9);
            break;
        }***/
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
    int MessageType;
    uint8_t childIP[4];

    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu", &MessageType, &childIP[0], &childIP[1], &childIP[2], &childIP[3]);

    encodeParentInfoResponse(smallSendBuffer,sizeof(smallSendBuffer),myIP,rootHopDistance,numberOfChildren);
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
void handleParentInfoResponse(char* msg, ParentInfo *parents, int i){
    int MessageType;
    int rootDistance, nrChildren;
    int parentIP[4];
    sscanf(msg, "%d", &MessageType);

    if (MessageType == PARENT_INFO_RESPONSE){
            sscanf(msg, "%d %d.%d.%d.%d %d %d", &MessageType, &parentIP[0],&parentIP[1],&parentIP[2],&parentIP[3],&rootDistance,&nrChildren);
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
    RoutingTableEntry newNode;

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
    updateRoutingTable(childAPIP,0,sequenceNumber,childAPIP);

    LOG(NETWORK,INFO, "New child connected.\n");

    //Send my routing table to my child
    //LOG(MESSAGES,INFO,"Sending my routing Table to child:");
    //tablePrint(routingTable,printRoutingTableHeader,printRoutingStruct);
    encodeFullRoutingTableUpdate(largeSendBuffer,sizeof(largeSendBuffer));
    LOG(MESSAGES, INFO, "Sending [Full Routing Update]:%s to: %d.%d.%d.%d\n",largeSendBuffer,childAPIP[0], childAPIP[1], childAPIP[2], childAPIP[3]);
    sendMessage(childSTAIP,largeSendBuffer);


    encodePartialRoutingUpdate(smallSendBuffer,sizeof(smallSendBuffer), &childAPIP,1);
    //LOG(MESSAGES,INFO,"Sending [PARTIAL ROUTING TABLE UPDATE] message: \"%s\"\n", smallSendBuffer);
    propagateMessage(smallSendBuffer,  childAPIP);
    //Sending new node information to the DEBUG visualization program, if enabled
    monitoring.reportNewNode(childAPIP, myIP);

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
    int type, nrOfChanges = 0,childIndex=-1;
    uint8_t nodeIP[4], sourceIP[4],changedNodes[TABLE_MAX_SIZE][4];
    int hopDistance,sequenceNumber;
    bool isRoutingTableChanged = false, isRoutingEntryChanged = false ;
    RoutingTableEntry *entry;

    char* token = strtok(msg, "|");

    //Parse Message Type and root node IP
    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",&type,&sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],
           &rootIP[0],&rootIP[1],&rootIP[2],&rootIP[3]);

    //To discard the message type and ensure the token points to the first routing table update entry
    token = strtok(nullptr, "|");

    while (token != nullptr) {
        sscanf(token, "%hhu.%hhu.%hhu.%hhu %d %d",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&hopDistance,&sequenceNumber);
        //Serial.printf("Token: %s\n", token);

        //Update the Routing Table
        isRoutingEntryChanged = updateRoutingTable(nodeIP,hopDistance,sequenceNumber,sourceIP);
        // If the node's routing entry was modified, add it to the list of nodes to include in the Partial routing update
        if(isRoutingEntryChanged == true){
            assignIP(changedNodes[nrOfChanges], nodeIP);
            nrOfChanges ++;
            // If the node's own routing entry changed, it was previously unreachable and is now returning.
            // It must notify the sender so they can update their routing table accordingly.
            if(isIPEqual(nodeIP,myIP)){
                encodePartialRoutingUpdate(smallSendBuffer,sizeof(smallSendBuffer),&myIP,1);
                // Find the nextHopIP that leads to the sender(if it is a child then its the child STA IP)
                uint8_t *nextHopIP = (uint8_t *) findRouteToNode(sourceIP);
                if(nextHopIP != nullptr) sendMessage(nextHopIP,smallSendBuffer);
            }
        }
        /***
         * If a received update does not significantly change the local routing table, it will not be propagated.
         * However, if that update marks a node as unreachable when it is actually reachable
         * (i.e., that node has a stored sequence number greater than the odd one in the update),
         * the false unreachability would otherwise persist until the next FRTU
         * To avoid stale information remaining that long, the node immediately sends a correction
         * back to the sender, informing it that the advertised node is in fact reachable.
         ***/
        entry = (RoutingTableEntry*)tableRead(routingTable,nodeIP);
        if(!isRoutingEntryChanged && (sequenceNumber % 2 != 0) && entry->sequenceNumber>sequenceNumber){
            encodePartialRoutingUpdate(smallSendBuffer,sizeof(smallSendBuffer),&nodeIP,1);
            // Find the nextHopIP that leads to the sender(if it is a child then its the child STA IP)
            uint8_t *nextHopIP = (uint8_t *) findRouteToNode(sourceIP);
            if(nextHopIP != nullptr) sendMessage(nextHopIP,smallSendBuffer);
        }

        isRoutingTableChanged = isRoutingTableChanged || isRoutingEntryChanged ;
        token = strtok(nullptr, "|");
    }

    // If the routing update caused a change in my routing table, propagate the updated information to the rest of the network
    if (isRoutingTableChanged){
        //Propagate the routing table update information trough the network
        encodePartialRoutingUpdate(largeSendBuffer,sizeof(largeSendBuffer),changedNodes,nrOfChanges);
        propagateMessage(largeSendBuffer, sourceIP);
    }

    strcpy(largeSendBuffer , "");

    /*** If the routing update is flagged as coming from a node in a detached subtree,
       * it indicates that this node is also part of a detached subtree. If the node is not yet
       * aware of its disconnected status, update its lifecycle information (states, variables) accordingly ***/
    // If i am not the root, verify whether the root's routing information has been updated
    if(!iamRoot && isIPinList(rootIP,changedNodes,nrOfChanges)){
        RoutingTableEntry *rootEntry = (RoutingTableEntry*) findNode(routingTable,rootIP);
        if(rootEntry != nullptr){
            if(rootEntry->sequenceNumber%2!=0){
                /***
                 * If the root routing information indicates that the root is unreachable, two situations are possible:
                 * either the information is outdated, or the node is no longer connected to the main tree.
                 * In either case, the node transitions to the recovery-await state (if not already there) and waits for
                 * reconnection or, in the case of outdated information, waits for a new update confirming that it is still part of the tree.
                ***/
                if(onRootUnreachableCallback != nullptr)onRootUnreachableCallback();
            }else{
                /***
                 * If the root routing information indicates that the root is reachable, and the node was previously in
                 * the recovery-wait state, it can now transition to the active state (if not already there), since the update
                 * confirms that the subtree has rejoined the main network.
                 ***/
                if(onRootReachableCallback != nullptr)onRootReachableCallback();
            }

        }
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
    int type, nrOfChanges = 0, childIndex;
    uint8_t *childSTAIP;
    uint8_t nodeIP[4], senderIP[4],changedNodes[TABLE_MAX_SIZE][4];
    int sequenceNumber;
    int hopDistance;
    bool isRoutingTableChanged = false, isRoutingEntryChanged = false;
    RoutingTableEntry *entry;
    char* token = strtok(msg, "|");

    //Parse Message Type and senderIP
    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu",&type,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]);

    //To discard the message type and ensure the token points to the first routing table update entry
    token = strtok(nullptr, "|");

    while (token != NULL) {
        sscanf(token, "%hhu.%hhu.%hhu.%hhu %d %d",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&hopDistance,&sequenceNumber);
        //Serial.printf("Token: %s\n", token);

        //Update the Routing Table
        isRoutingEntryChanged = updateRoutingTable(nodeIP,hopDistance,sequenceNumber,senderIP);
        LOG(MESSAGES,DEBUG,"NodeEntry: %hhu.%hhu.%hhu.%hhu isChanged:%d\n",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3],isRoutingEntryChanged);
        // If the node's routing entry was modified, add it to the list of nodes to include in the Partial routing update
        if(isRoutingEntryChanged == true){
            assignIP(changedNodes[nrOfChanges],nodeIP);
            nrOfChanges ++;

            // If the node's own routing entry changed, it was previously unreachable and is now returning.
            // It must notify the sender so they can update their routing table accordingly.
            if(isIPEqual(nodeIP,myIP)){
                encodePartialRoutingUpdate(smallSendBuffer,sizeof(smallSendBuffer),&myIP,1);
                childIndex = tableFind(childrenTable, senderIP) ;
                if(childIndex != -1){ // If the sender is one of my children, the update must be sent to its childSTAIP
                    childSTAIP = (uint8_t *)tableValueAtIndex(childrenTable,childIndex);
                    if(childSTAIP!= nullptr)sendMessage(childSTAIP,smallSendBuffer);
                }else sendMessage(senderIP,smallSendBuffer);
            }
        }

        /***
         * If a received update does not significantly change the local routing table, it will not be propagated.
         * However, if that update marks a node as unreachable when it is actually reachable
         * (i.e., that node has a stored sequence number greater than the odd one in the update),
         * the false unreachability would otherwise persist until the next FRTU
         * To avoid stale information remaining that long, the node immediately sends a correction
         * back to the sender, informing it that the advertised node is in fact reachable.
         ***/
        entry = (RoutingTableEntry*)tableRead(routingTable,nodeIP);
        if(!isRoutingEntryChanged && (sequenceNumber % 2 != 0) && entry->sequenceNumber>sequenceNumber){
            encodePartialRoutingUpdate(smallSendBuffer,sizeof(smallSendBuffer),&nodeIP,1);
            // Find the nextHopIP that leads to the sender(if it is a child then its the child STA IP)
            uint8_t *nextHopIP = (uint8_t *) findRouteToNode(senderIP);
            if(nextHopIP != nullptr) sendMessage(nextHopIP,smallSendBuffer);
        }

        //updateRoutingTable(nodeIP,newNode,sourceIP);
        isRoutingTableChanged = isRoutingTableChanged || isRoutingEntryChanged;
        token = strtok(nullptr, "|");
    }


    // If the routing update caused a change in my routing table, propagate the updated information to the rest of the network
    if(isRoutingTableChanged){
        //Propagate the routing table update information trough the network
        encodePartialRoutingUpdate(largeSendBuffer,sizeof(largeSendBuffer), changedNodes,nrOfChanges);
        propagateMessage(largeSendBuffer, senderIP);

    }
    strcpy(largeSendBuffer , "");

    // Verify whether the root's routing information has been updated
    if(isIPinList(rootIP,changedNodes,nrOfChanges)){
        LOG(NETWORK,INFO, "Root on changed nodes\n");
        RoutingTableEntry *rootEntry = (RoutingTableEntry*) findNode(routingTable,rootIP);
        if(rootEntry != nullptr){
            if(rootEntry->sequenceNumber%2!=0){
                /***
                 * If the root routing information indicates that the root is unreachable, two situations are possible:
                 * either the information is outdated, or the node is no longer connected to the main tree.
                 * In either case, the node transitions to the recovery-await state (if not already there) and waits for
                 * reconnection or, in the case of outdated information, waits for a new update confirming that it is still part of the tree.
                ***/
                if(onRootUnreachableCallback != nullptr)onRootUnreachableCallback();
            }else{
                /***
                 * If the root routing information indicates that the root is reachable, and the node was previously in
                 * the recovery-wait state, it can now transition to the active state (if not already there), since the update
                 * confirms that the subtree has rejoined the main network.
                 ***/
                if(onRootReachableCallback != nullptr)onRootReachableCallback();
            }

        }
    }

}

void handleTopologyBreakAlert(char *msg){
    int type;
    uint8_t senderIP[4];
    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu",&type,&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]);
    //Encode a TOPOLOGY_BREAK_ALERT message where the sender IP is this node IP
    encodeTopologyBreakAlert(smallSendBuffer, sizeof(smallSendBuffer));
    // Notify child nodes of a tree disconnection so they can initiate recovery procedures
    propagateMessage(smallSendBuffer, senderIP);
}

void handleTopologyRestoredNotice(char *msg){
    uint8_t senderIP[4];
    sscanf(msg, "%*d %hhu.%hhu.%hhu.%hhu",&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3]);
    //Encode a TOPOLOGY_RESTORED_NOTICE message where the sender IP is this node IP
    encodeTopologyRestoredNotice(smallSendBuffer, sizeof(smallSendBuffer));
    // Notify child nodes that the tree has been restored and they can transition to the active state.
    propagateMessage(smallSendBuffer, senderIP);
}

void handleParentResetNotification(char *msg){

    // Permanently disconnect from the parent node
    //disconnectFromAP();
}
/**
 * handleDebugMessage
 * Handles a MONITORING_MESSAGE: decodes the message, forwards it to the appropriate next hop or debug server.
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
        LOG(MONITORING_SERVER,DEBUG,msg);
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
    int type, nChars=0,nChars2=0;
    uint8_t originatorIP[4],destinationIP[4],nextHopIP[4],broadcastIP[4]={255,255,255,255};
    uint8_t *nextHopPtr = nullptr,senderIP[4];
    char payload[MAX_PAYLOAD_SIZE];
    bool isTunneled = false,isBroadcast=false;

    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %n",&type, &originatorIP[0],&originatorIP[1],&originatorIP[2],&originatorIP[3],
        &destinationIP[0],&destinationIP[1],&destinationIP[2],&destinationIP[3],&nChars);
    //Serial.printf("Message %s received from %d.%d.%d.%d to %d.%d.%d.%d", payload, senderIP[0],senderIP[1],senderIP[2],senderIP[3],
    //destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3]);

    if(isIPEqual(destinationIP,broadcastIP)){
        sscanf(msg+nChars, "%hhu.%hhu.%hhu.%hhu %n",&senderIP[0],&senderIP[1],&senderIP[2],&senderIP[3],&nChars2);
        nChars += nChars2;
        isBroadcast=true;
    }
    // Copy the rest of the string manually
    strncpy(payload, msg + nChars, sizeof(payload) - 1);


    if(isBroadcast){ //if the message is a broadcast message
        //Process the message with the user provided callback
        LOG(NETWORK, DEBUG, "Receive the Broadcast Message: %s\n",msg);

        if(onDataMessageCallback) onDataMessageCallback(originatorIP,destinationIP,payload);

        //Propagate the message to the rest of the message
        encodeDataMessage(largeSendBuffer, sizeof(largeSendBuffer),payload,originatorIP,broadcastIP);
        propagateMessage(largeSendBuffer,senderIP);

    }else if(!isIPEqual(destinationIP, myIP)){ // If this message is not intended for this node, forward it to the next hop leading to its destination.
        //LOG(NETWORK, DEBUG, "DATA Message as arrived for forwarding to other node:%hhu.%hhu.%hhu.%hhu.\n",destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3]);

        monitoring.reportDataMessageReceived(receivePayload,DATA_MESSAGE,-1);

        //Find the route to the destination IP of the message
        nextHopPtr = findRouteToNode(destinationIP);
        if (nextHopPtr != nullptr){
            assignIP(nextHopIP, nextHopPtr);
            //LOG(NETWORK, DEBUG, "DATA Message forwarded to node:%hhu.%hhu.%hhu.%hhu.\n",nextHopPtr[0],nextHopPtr[1],nextHopPtr[2],nextHopPtr[3]);
            sendMessage(nextHopIP,receiveBuffer);
        }else{
            LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                                "Unable to forward message.\n", destinationIP[0], destinationIP[1],destinationIP[2], destinationIP[3]);
        }
    }else{// If the message is addressed to this node, handle it using the user-defined callback
        //LOG(NETWORK, DEBUG, "DATA Message as arrived for this node\n");
        if(onDataMessageCallback) onDataMessageCallback(originatorIP,destinationIP,payload);

        //isTunneled = isMessageTunneled(msg);
        //if(isTunneled)LOG(MESSAGES,INFO,"Tunneled Message arrived\n");

        //Send ACK Message back to the source of the message
        //assignIP(parameters.IP1,myIP);
        //assignIP(parameters.IP2,originatorIP);
        //encodeMessage(smallSendBuffer, sizeof(smallSendBuffer),ACK_MESSAGE, parameters);

        /***nextHopPtr = findRouteToNode(originatorIP);
        if (nextHopPtr != nullptr){
            sendMessage(nextHopPtr,smallSendBuffer);
        }else{
            LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %d.%d.%d.%d. "
                                "Unable to forward message.\n", originatorIP[0], originatorIP[1],originatorIP[2], originatorIP[3]);
        }***/
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
    int type,nChars=0;
    uint8_t nextHopIP[4], sourceIP[4], destinationIP[4];
    uint8_t *nextHopPtr = nullptr;
    char payload[MAX_PAYLOAD_SIZE];

    sscanf(msg, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %n",&type, &sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],
           &destinationIP[0],&destinationIP[1],&destinationIP[2],&destinationIP[3],&nChars);


    //If the node is not the final destination of the ACK forward the message to the next hop
    if(!isIPEqual(destinationIP, myIP)){
        nextHopPtr = findRouteToNode(destinationIP);
        if (nextHopPtr != nullptr){
            assignIP(nextHopIP, nextHopPtr);
            sendMessage(nextHopIP,receiveBuffer);
        }else{
            LOG(NETWORK, ERROR, "❌Routing failed: No route found to node %hhu.%hhu.%hhu.%hhu. "
                                "Unable to forward message.\n", destinationIP[0], destinationIP[1],destinationIP[2], destinationIP[3]);
        }
    }else{
        // Extract the ACK payload from the message and pass it to the user-defined callback
        strncpy(payload, msg + nChars, sizeof(payload) - 1);

        if(onACKMessageCallback) onACKMessageCallback(sourceIP,destinationIP,payload);
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
void propagateMessage(char* message, uint8_t * sourceIP){
    // If the message didn't come from the parent and i have a parent, forward it to the parent
    uint8_t *childSTAIP,*childAPIP;
    bool messageSent=false;

    //Send the message to my parent only if it exists and is reachable
    if(!isIPEqual(sourceIP, parent) && hasParent){
        sendMessage(parent, message);
        messageSent = true;
        LOG(MESSAGES, INFO, "Sending the Message:\"%s\" to %hhu.%hhu.%hhu.%hhu",message,parent[0],parent[1],parent[2],parent[3]);
    }

    //Forward the message to all children except the one that sent it to me
    for(int i = 0; i< childrenTable->numberOfItems; i++){
        childAPIP = (uint8_t *)tableKey(childrenTable, i);
        childSTAIP = (uint8_t *)tableValueAtIndex(childrenTable, i);
        if (childSTAIP != nullptr && childAPIP != nullptr){
            if(!isIPEqual(childAPIP, sourceIP)){
                sendMessage(childSTAIP, message);
                if (!messageSent) {
                    LOG(MESSAGES, INFO, "Sending the Message:\"%s\" to %hhu.%hhu.%hhu.%hhu",message,childAPIP[0],childAPIP[1],childAPIP[2],childAPIP[3]);
                    messageSent = true;
                } else {
                    LOG(MESSAGES, INFO, ", %hhu.%hhu.%hhu.%hhu",childAPIP[0],childAPIP[1],childAPIP[2],childAPIP[3]);
                }
            }
        }

    }

    if(messageSent)LOG(MESSAGES, INFO, "\n");

}

void encodeTunneledMessage(char* encodedMessage,size_t encodedMessageSize,uint8_t sourceIP[4], uint8_t destinationIP[4], char* encapsulatedMessage){
    //char payload[MAX_PAYLOAD_SIZE];
    //strncpy(payload,encapsulatedMessage,sizeof(payload)-1);
    encodeDataMessage(encodedMessage, encodedMessageSize,encapsulatedMessage,sourceIP,destinationIP);
}

bool isMessageTunneled(char* dataMessage){
    MessageType type;
    //int sourceIP[4],originalDestinationIP[4];
    return( sscanf(dataMessage,"%d %*u.%*u.%*u.%*u %*u.%*u.%*u.%*u %d %*u.%*u.%*u.%*u %*u.%*u.%*u.%*u",&type,&type) ==2 );
}

bool waitForMessage(MessageType type, uint8_t expectedSenderIP[4], unsigned long timeOut){
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

void getSenderIP(char* messageBuffer, MessageType type, uint8_t * senderIP){
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

bool sendMessageToNode(char* messageBuffer,uint8_t *destinationIP){
    uint8_t *nextHopIP = findRouteToNode(destinationIP);
    if(nextHopIP != nullptr){
        sendMessage(nextHopIP,messageBuffer);
        return true;
    }
    return false;
}

void sendMessageToChildren(char* messageBuffer){
    uint8_t *childSTAIP,*childAPIP;
    for (int i = 0; i < childrenTable->numberOfItems; i++) {
        // When broadcasting a message to all children, we can directly use as the nextHopIp the child's STA IP from the childrenTable.
        childAPIP = (uint8_t *)tableKey(childrenTable,i);
        childSTAIP = (uint8_t *)tableValueAtIndex(childrenTable,i);
        if(childSTAIP != nullptr){
            LOG(MESSAGES, INFO, "Sending the Message:\"%s\" to %hhu.%hhu.%hhu.%hhu\n",messageBuffer,childAPIP[0],childAPIP[1],childAPIP[2],childAPIP[3]);
            sendMessage(childSTAIP,messageBuffer);
        }
    }

}

void sendMessageToParent(char* messageBuffer){
    if(hasParent){
        sendMessage(parent,messageBuffer);
    }
}





void sendDataMessageToChildren(char* messageBuffer,size_t bufferSize,const char* messagePayload){
    uint8_t *childSTAIP,*childAPIP;
    for (int i = 0; i < childrenTable->numberOfItems; i++) {
        // When broadcasting a message to all children, we can directly use as the nextHopIp the child's STA IP from the childrenTable.
        childAPIP = (uint8_t *)tableKey(childrenTable,i);
        childSTAIP = (uint8_t *)tableValueAtIndex(childrenTable,i);
        if(childSTAIP != nullptr){
            encodeDataMessage(messageBuffer,bufferSize ,messagePayload,myIP,childAPIP);
            LOG(MESSAGES, INFO, "Sending Data Message:\"%s\" to %hhu.%hhu.%hhu.%hhu\n",messageBuffer,childAPIP[0],childAPIP[1],childAPIP[2],childAPIP[3]);
            sendMessage(childSTAIP,messageBuffer);
        }
    }

}

void sendDataMessageToParent(char* messageBuffer,size_t bufferSize,const char* messagePayload){
    if(hasParent){
        encodeDataMessage(messageBuffer, bufferSize,messagePayload,myIP,parent);
        sendMessage(parent,messageBuffer);
    }
}


void sendDataMessageToNode(char* messageBuffer,size_t bufferSize,const char* messagePayload,uint8_t *destinationIP){
    uint8_t broadcastIP[4]={255,255,255,255};

    encodeDataMessage(messageBuffer, bufferSize,messagePayload,myIP,destinationIP);

    // If it's a broadcast data message, forward it to all neighbors
    if (isIPEqual(destinationIP,broadcastIP)){ //If the data message is meant to be broadcast
        propagateMessage(messageBuffer,myIP);
    }else{
        uint8_t *nextHopIP = findRouteToNode(destinationIP);
        if(nextHopIP != nullptr){
            sendMessage(nextHopIP,messageBuffer);
            LOG(MESSAGES,DEBUG,"Sending the message: %s to node: %hhu.%hhu.%hhu.%hhu (nextHopIP:%hhu.%hhu.%hhu.%hhu)\n"
                ,messageBuffer,destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3]
                    ,nextHopIP[0],nextHopIP[1],nextHopIP[2],nextHopIP[3]);
        }
    }

}

void sendACKMessageToNode(char* messageBuffer,size_t bufferSize,const char* ackPayload,uint8_t *destinationIP){
    encodeACKMessage(messageBuffer, bufferSize,ackPayload,myIP,destinationIP);
    uint8_t *nextHopIP = findRouteToNode(destinationIP);
    if(nextHopIP != nullptr){
        sendMessage(nextHopIP,messageBuffer);
    }
}


void onPeriodicRoutingUpdate(){
    uint8_t changedNodes[TABLE_MAX_SIZE][4];
    uint8_t nChanges=0;
    RoutingTableEntry *entry;
    uint8_t *currentIP;

    // Construct a list of all nodes that have undergone relevant changes since the last FRTU.
    for (int i = 0; i < routingTable->numberOfItems; i++){
        currentIP = (uint8_t *) tableKey(routingTable,i);
        entry = (RoutingTableEntry*) tableRead(routingTable,currentIP);
        // If the current node has relevant changes that were not sent in the last routing update, include them in this update.
        //LOG(MESSAGES,DEBUG,"Is Node Entry changed since last update?: %hhu.%hhu.%hhu.%hhu isEntryChanged:%i\n",currentIP[0],currentIP[1],currentIP[2],currentIP[3],entry->isChangeRelevant);
        if(entry != nullptr && entry->isChangeRelevant){
            assignIP(changedNodes[nChanges],currentIP);
            nChanges++;
        }
    }

    // If more than 50% of the routing table entries have changed since the last update, send a FRTU
    if(nChanges>=0.5*routingTable->numberOfItems){
        //Update my sequence number
        updateMySequenceNumber(mySequenceNumber+2);
        encodeFullRoutingTableUpdate(largeSendBuffer, sizeof(largeSendBuffer));
        propagateMessage(largeSendBuffer,myIP);
        lastFullRoutingUpdateTime=getCurrentTime(); // Update the time of the last FRTU
        clearRelevantFlag();//Clear the relevant flags on each routing entry
    }else if(nChanges>0){
        //Encode the periodic routing message with the changes since the last update ON A PRTU
        encodePartialRoutingUpdate(largeSendBuffer, sizeof(largeSendBuffer),changedNodes,nChanges);
        propagateMessage(largeSendBuffer,myIP);
    }
}

void onPeriodicFullRoutingUpdate(){
    // The node increments its sequence number with each periodic FRTU to ensure the information remains fresh.
    updateMySequenceNumber(mySequenceNumber+2);
    encodeFullRoutingTableUpdate(largeSendBuffer, sizeof(largeSendBuffer));
    propagateMessage(largeSendBuffer,myIP);
    // Update the time of the last FRTU
    lastFullRoutingUpdateTime=getCurrentTime();
    clearRelevantFlag();//Clear the relevant flags on each routing entry
}