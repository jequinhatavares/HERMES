#include "strategy_topology.h"

Strategy strategyTopology = {
        .handleMessage = handleMessageStrategyTopology,
        .encodeMessage = encodeMessageStrategyTopology,
        .influenceRouting = influenceRoutingStrategyTopology,
        .onTimer = onTimerStrategyTopology,
        .onNetworkEvent = onNetworkEventStrategyTopology,
        .getContext = getContextStrategyTopology,

};

int tmpChildIP[4];
int tmpChildSTAIP[4];

unsigned long lastMiddlewareUpdateTimeTopology;

//Init Function Pointers
//void (*encodeTopicValue)(char*,size_t,void *) = nullptr;
//void (*decodeTopicValue)(char*,int8_t *) = nullptr;

void (*chooseParentFunction)(int*) = nullptr;

void initStrategyTopology(){
        //chooseParentFunction = chooseParentFunctionPointer;
}

void encodeMessageStrategyTopology(char* messageBuffer, size_t bufferSize, int typeTopology){
    int nodeIP[4],MAC[6];
    int offset = 0;

    /***if(typeTopology == TOP_PARENT_LIST_ADVERTISEMENT){
        //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
        offset = snprintf(messageBuffer, bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT,rootIP[0],rootIP[1],rootIP[2],rootIP[3],
                myIP[0],myIP[1],myIP[2],myIP[3]);

        for (int i = 0; i < reachableNetworks.len; i++) {
            sscanf(reachableNetworks.item[i], "JessicaNode%i:%i:%i:%i:%i:%i",&MAC[0],&MAC[1],&MAC[2],&MAC[3],&MAC[4],&MAC[5]);
            getIPFromMAC(MAC, nodeIP);

            offset += snprintf(messageBuffer + offset, bufferSize - offset," %i.%i.%i.%i",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
        }

    }else if(typeTopology == TOP_PARENT_REASSIGNMENT_COMMAND){
        //MESSAGE_TYPE TOP_PARENT_REASSIGNMENT_COMMAND [tmpParentIP] [nodeIP] [parentIP]
        snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_REASSIGNMENT_COMMAND,orphanIP[0],orphanIP[1],orphanIP[2],orphanIP[3],
                 newParentIP[0],newParentIP[1],newParentIP[2],newParentIP[3]);
    }***/
}
void encodeParentAssignmentCommand(char* messageBuffer, size_t bufferSize, int* destinationIP, int* chosenParentIP, int* targetNodeIP){
    //MESSAGE_TYPE TOP_PARENT_REASSIGNMENT_COMMAND [destinationIP] [nodeIP] [parentIP]
    snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_REASSIGNMENT_COMMAND,destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3],
             targetNodeIP[0],targetNodeIP[1],targetNodeIP[2],targetNodeIP[3],chosenParentIP[0],chosenParentIP[1],chosenParentIP[2],chosenParentIP[3]);
}
void encodeParentListAdvertisementRequest(char* messageBuffer, size_t bufferSize, parentInfo* possibleParents, int nrOfPossibleParents, int *temporaryParent, int *mySTAIP){
    int offset = 0;

    /*** This is an intermediary message sent to the temporary parent.
         Since the node is not yet integrated into the network and doesn't know the root IP, the destination field is set to the temporary parent’s IP.
         The temporary parent will replace it with the correct root IP before forwarding.
         New Node-> Temporary Parent -> Root Node  ***/

    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT_REQUEST [tmp parent IP] [nodeSTAIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    offset = snprintf(messageBuffer, bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT_REQUEST,temporaryParent[0],temporaryParent[1],temporaryParent[2],temporaryParent[3],
                      mySTAIP[0],mySTAIP[1],mySTAIP[2],mySTAIP[3],myIP[0],myIP[1],myIP[2],myIP[3]);

    for (int i = 0; i < nrOfPossibleParents; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset," %i.%i.%i.%i",
                           possibleParents[i].parentIP[0],possibleParents[i].parentIP[1],possibleParents[i].parentIP[2],possibleParents[i].parentIP[3]);
    }
}

void handleMessageStrategyTopology(char* messageBuffer, size_t bufferSize){
    TopologyMessageType type;
    int destinationNodeIP[4],*nextHopIP, nChars = 0,IP[4], targetNodeIP[4],chosenParentIP[4];
    int possibleParents[TableMaxSize][4],i=0;
    //Extract Inject Message Types
    sscanf(messageBuffer,"%*i %i",&type);

    if(type == TOP_PARENT_LIST_ADVERTISEMENT_REQUEST){
        LOG(MESSAGES,INFO,"Received [PARENT_LIST_ADVERTISEMENT_REQUEST] message: \"%s\"\n", messageBuffer);
        sscanf(messageBuffer,"%*d %*d %d.%d.%d.%d %d.%d.%d.%d %d.%d.%d.%d %n",&destinationNodeIP[0],&destinationNodeIP[1],&destinationNodeIP[2],&destinationNodeIP[3]
               ,&tmpChildSTAIP[0],&tmpChildSTAIP[1],&tmpChildSTAIP[2],&tmpChildSTAIP[3]
               ,&tmpChildIP[0],&tmpChildIP[1],&tmpChildIP[2],&tmpChildIP[3],&nChars);

        // Check whether this node is the intended destination of the message
        if(isIPEqual(destinationNodeIP,myIP)){
            if(!iamRoot){
                //Encode the TOP_PARENT_LIST_ADVERTISEMENT message to be sent to the root
                //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP =root] [tmpParentIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
                snprintf(largeSendBuffer, sizeof(largeSendBuffer),"%i %i %i.%i.%i.%i %i.%i.%i.%i %i.%i.%i.%i %s",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT,rootIP[0],rootIP[1],rootIP[2],rootIP[3],
                         myIP[0],myIP[1],myIP[2],myIP[3],tmpChildIP[0],tmpChildIP[1],tmpChildIP[2],tmpChildIP[3],messageBuffer+nChars);
                //Send the encode message to the root
                nextHopIP = findRouteToNode(rootIP);
                if(nextHopIP != nullptr){
                    sendMessage(nextHopIP,largeSendBuffer);
                }
            }else{
                // If the new node connects directly to the root and sends the TOP_PARENT_LIST_ADVERTISEMENT_REQUEST,
                // the root can process the list of advertised parents and select a parent immediately
                chooseParentStrategyTopology(messageBuffer);
            }

        }else{
            // This message type is intended to be sent only by the new node to its directly connected temporary parent
            LOG(NETWORK, ERROR, "❌ ERROR: This node should be the destination of the TOP_PARENT_LIST_ADVERTISEMENT_REQUEST message\n");
        }
    }
    else if(type == TOP_PARENT_LIST_ADVERTISEMENT){
        LOG(MESSAGES,INFO,"Received [PARENT_LIST_ADVERTISEMENT] message: \"%s\"\n", messageBuffer);
        sscanf(messageBuffer,"%*d %*d %d.%d.%d.%d %n",&destinationNodeIP[0],&destinationNodeIP[1],&destinationNodeIP[2],&destinationNodeIP[3],&nChars);
        // Check if i am the final destination of the message
        if(isIPEqual(destinationNodeIP,myIP)){
            /***char* token = strtok(messageBuffer+nChars, " ");
            while (token != NULL) {
                sscanf(token,"%d.%d.%d.%d",&IP[0],&IP[1],&IP[2],&IP[3]);
                // Check if the nodeIP already exists in the middleware metrics table
                assignIP(possibleParents[i],IP);
                token = strtok(NULL, " ");
                i++;
            }***/
            chooseParentStrategyTopology(messageBuffer);

        }else{ // If not, forward the message to the nextHop to the destination
            nextHopIP = findRouteToNode(destinationNodeIP);
            if (nextHopIP != nullptr){
                sendMessage(nextHopIP,messageBuffer);
            }
        }
    }else if(type == TOP_PARENT_REASSIGNMENT_COMMAND){
        LOG(MESSAGES,INFO,"Received [PARENT_REASSIGNMENT_COMMAND] message: \"%s\"\n", messageBuffer);

        //MESSAGE_TYPE TOP_PARENT_REASSIGNMENT_COMMAND [destinationIP] [nodeIP] [parentIP]
        sscanf(messageBuffer,"%*d %*d %d.%d.%d.%d",&destinationNodeIP[0],&destinationNodeIP[1],&destinationNodeIP[2],&destinationNodeIP[3]);
        // Check if i am the final destination of the message
        if(isIPEqual(destinationNodeIP,myIP)){
            // If this node receives a parent reassignment command concerning the temporary child node, it must forward the message to that child
            sscanf(messageBuffer,"%*d %*d %*d.%*d.%*d.%*d %d.%d.%d.%d",&targetNodeIP[0],&targetNodeIP[1],&targetNodeIP[2],&targetNodeIP[3]);
            if(isIPEqual(targetNodeIP,tmpChildIP)){
                //Change the message destination IP to the childIP
                //snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_REASSIGNMENT_COMMAND,tmpChildIP[0],tmpChildIP[1],tmpChildIP[2],tmpChildIP[3],tmpChildIP[0],tmpChildIP[1],tmpChildIP[2],tmpChildIP[3],);
                sendMessage(tmpChildSTAIP,messageBuffer);
            }
        }else{ // If not, forward the message to the nextHop to the destination
            nextHopIP = findRouteToNode(destinationNodeIP);
            if (nextHopIP != nullptr){
                sendMessage(nextHopIP,messageBuffer);
            }
        }
    }
}
void onNetworkEventStrategyTopology(int networkEvent, int involvedIP[4]){
}
void influenceRoutingStrategyTopology(char* dataMessage){
}
void onTimerStrategyTopology(){
    unsigned long currentTime = getCurrentTime();
    //Periodically send this node's metric to all other nodes in the network
    if( (currentTime - lastMiddlewareUpdateTimeTopology) >= 10000 ) {
    }
}
void* getContextStrategyTopology(){
    return nullptr;
}


parentInfo requestParentFromRoot(parentInfo* possibleParents, int nrOfPossibleParents){
    int mySTAIP[4], temporaryParent[4],assignedParent[4];
    int packetSize=0, receivedMessageType;
    bool isExpectedMessage=false;

    LOG(MESSAGES,INFO,"Strategy Topology\n");


    // Select the first element of the possibleParents List to be the temporary parent
    if(nrOfPossibleParents != 0){
        connectToAP(possibleParents[0].ssid,PASS);
        assignIP(temporaryParent,possibleParents[0].parentIP);
        getMySTAIP(mySTAIP);
    }


    // Encode the PARENT_LIST_ADVERTISEMENT_REQUEST to send to the temporary parent, for in to send a PARENT_LIST_ADVERTISEMENT to the root
    encodeParentListAdvertisementRequest(largeSendBuffer, sizeof(largeSendBuffer), possibleParents, nrOfPossibleParents, temporaryParent, mySTAIP);

    // Send the message to the temporary parent so it can be forwarded toward the root
    sendMessage(temporaryParent,largeSendBuffer);

    LOG(MIDDLEWARE,INFO,"Sending %s to Temporary Parent: %i.%i.%i.%i\n",largeSendBuffer, temporaryParent[0],temporaryParent[1],temporaryParent[2],temporaryParent[3]);


    // Wait for the message from the root assigning me a parent
    unsigned long startTime = getCurrentTime();
    unsigned long currentTime = startTime;
    while( ((currentTime - startTime) <= 6000) && !isExpectedMessage ){
        packetSize = receiveMessage(receiveBuffer, sizeof(receiveBuffer));
        currentTime = getCurrentTime();
        if(packetSize>0){
            sscanf(receiveBuffer, "%*d %d",&receivedMessageType);
            if(receivedMessageType == TOP_PARENT_REASSIGNMENT_COMMAND){
                isExpectedMessage = true;
            }
        }
    }

    LOG(MIDDLEWARE,INFO,"Received: %s\n",receiveBuffer);

    //Disconnect from the temporary parent
    disconnectFromAP();

    //Parse the received TOP_PARENT_REASSIGNMENT_COMMAND
    if(isExpectedMessage == true){
        sscanf(receiveBuffer,"%*d %*d %*d.%*d.%*d.%*d %d.%d.%d.%d",&assignedParent[0],&assignedParent[1],&assignedParent[2],&assignedParent[3]);
        // Search in the possibleParents the chosen one
        for (int i = 0; i < nrOfPossibleParents; i++) {
            if(isIPEqual(possibleParents[i].parentIP, assignedParent)){
                LOG(MIDDLEWARE,INFO,"Chosen Parent: %i.%i.%i.%i\n",assignedParent[0],assignedParent[1],assignedParent[2],assignedParent[3]);
                return possibleParents[i];
            }
        }
    }else{
        LOG(MIDDLEWARE,INFO,"Did not receive the [PARENT_ASSIGNMENT_COMMAND] message\n");
        return chooseParent(possibleParents,nrOfPossibleParents);
    }

    return possibleParents[0];

}

void chooseParentStrategyTopology(char* messageBuffer){
    int sourceIP[4], targetNodeIP[4],possibleParents[TableMaxSize][4],IP[4],chosenParentIP[4],*nextHopIP;
    int middlewareMessageType,nChars=0,parentsCount=0;
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP] [nodeSTAIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP =root] [tmpParentIP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...

    sscanf(messageBuffer,"%*d %d %*d.%*d.%*d.%*d %d.%d.%d.%d %d.%d.%d.%d %n",&middlewareMessageType,
            &sourceIP[0],&sourceIP[1],&sourceIP[2],&sourceIP[3],&targetNodeIP[0],&targetNodeIP[1],&targetNodeIP[2],&targetNodeIP[3],&nChars);

    // Extract the list of possible parents from the message
    char* token = strtok(messageBuffer+nChars, " ");
    while (token != NULL) {
        sscanf(token,"%d.%d.%d.%d",&IP[0],&IP[1],&IP[2],&IP[3]);
        // Check if the nodeIP already exists in the middleware metrics table
        assignIP(possibleParents[parentsCount],IP);
        token = strtok(NULL, " ");
        parentsCount++;
    }

    //Select the node parent from the list of candidates
    assignIP(chosenParentIP, possibleParents[0]);
    //TODO choose parent

    // Encode a TOP_PARENT_REASSIGNMENT_COMMAND to be sent either directly to the new node or to its temporary parent, who will relay the message
    if(middlewareMessageType == TOP_PARENT_LIST_ADVERTISEMENT_REQUEST){
        // If the message is of type TOP_PARENT_LIST_ADVERTISEMENT_REQUEST, it came directly from the new node we can send the new parent command to it directly
        encodeParentAssignmentCommand(smallSendBuffer,sizeof(smallSendBuffer), targetNodeIP, chosenParentIP,targetNodeIP);
        sendMessage(sourceIP,smallSendBuffer);
        LOG(MIDDLEWARE,INFO,"Sending %s to: %i.%i.%i.%i\n",smallSendBuffer, targetNodeIP[0],targetNodeIP[1],targetNodeIP[2],targetNodeIP[3]);

    }else if(middlewareMessageType == TOP_PARENT_LIST_ADVERTISEMENT){
        // If the received message is a TOP_PARENT_LIST_ADVERTISEMENT, it came from the temporary parent.
        //The parent assignment command must be sent to the temporary parent, who will relay it to the target node
        encodeParentAssignmentCommand(smallSendBuffer,sizeof(smallSendBuffer), sourceIP, chosenParentIP,targetNodeIP);
        nextHopIP = findRouteToNode(sourceIP);
        if(nextHopIP != nullptr){
            sendMessage(nextHopIP,smallSendBuffer);
        }
        LOG(MIDDLEWARE,INFO,"Sending %s to: %i.%i.%i.%i\n",smallSendBuffer, sourceIP[0],sourceIP[1],sourceIP[2],sourceIP[3]);

    }

}