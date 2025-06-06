#include "strategy_topology.h"

int orphanIP[4];
int newParentIP[4];

unsigned long lastMiddlewareUpdateTimeTopology;


//Function Pointers Initializers
void (*encodeTopicValue)(char*,size_t,void *) = nullptr;
void (*decodeTopicValue)(char*,int8_t *) = nullptr;

void (*chooseParentFunction)(int*) = nullptr;

void initStrategyTopology(void (*chooseParentFunctionPointer)(int*)){
        chooseParentFunction = chooseParentFunctionPointer;
}

void encodeMessageStrategyTopology(char* messageBuffer, size_t bufferSize, int typeTopology){
    int nodeIP[4],MAC[6];
    int offset = 0;

    if(typeTopology == TOP_PARENT_LIST_ADVERTISEMENT){
        //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP] [nodeIP] [Possible Parent 1] [Possible Parent 2] ...
        offset = snprintf(messageBuffer, bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT,rootIP[0],rootIP[1],rootIP[2],rootIP[3],
                myIP[0],myIP[1],myIP[2],myIP[3]);

        for (int i = 0; i < reachableNetworks.len; i++) {
            sscanf(reachableNetworks.item[i], "JessicaNode%i:%i:%i:%i:%i:%i",&MAC[0],&MAC[1],&MAC[2],&MAC[3],&MAC[4],&MAC[5]);
            getIPFromMAC(MAC, nodeIP);

            offset += snprintf(messageBuffer + offset, bufferSize - offset," %i.%i.%i.%i",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
        }

    }else if(typeTopology == TOP_PARENT_REASSIGNMENT_COMMAND){
        //MESSAGE_TYPE TOP_PARENT_REASSIGNMENT_COMMAND [nodeIP] [parentIP]
        snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_REASSIGNMENT_COMMAND,orphanIP[0],orphanIP[1],orphanIP[2],orphanIP[3],
                 newParentIP[0],newParentIP[1],newParentIP[2],newParentIP[3]);
    }
}

void encodeMessageParentListAdvertisement(char* messageBuffer, size_t bufferSize, parentInfo* possibleParents, int nrOfPossibleParents, int *temporaryParent, int *mySTAIP){
    int offset = 0;

    // This is an intermediary message sent to the temporary parent.
    // Since the node is not yet integrated into the network and doesn't know the root IP, the destination field is set to the temporary parent’s IP.
    // The temporary parent will replace it with the correct root IP before forwarding.

    //MESSAGE_TYPE TOP_PARENT_LIST_ADVERTISEMENT [destination IP] [nodeIP] [nodeSTAIP] [Possible Parent 1] [Possible Parent 2] ...
    offset = snprintf(messageBuffer, bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_LIST_ADVERTISEMENT,temporaryParent[0],temporaryParent[1],temporaryParent[2],temporaryParent[3],
                      myIP[0],myIP[1],myIP[2],myIP[3],mySTAIP[0],mySTAIP[1],mySTAIP[2],mySTAIP[3]);

    for (int i = 0; i < nrOfPossibleParents; i++) {
        offset += snprintf(messageBuffer + offset, bufferSize - offset," %i.%i.%i.%i",
                           possibleParents->parentIP[0],possibleParents->parentIP[1],possibleParents->parentIP[2],possibleParents->parentIP[3]);
    }
}

void handleMessageStrategyTopology(char* messageBuffer, size_t bufferSize){
    TopologyMessageType type;
    int destinationNodeIP[4],*nextHopIP,newParent[4], nChars = 0,IP[4];
    int possibleParents[TableMaxSize][4],i=0;
    //Extract Inject Message Types
    sscanf(messageBuffer,"%*i %i",&type);

    if(type == TOP_PARENT_LIST_ADVERTISEMENT){
        sscanf(messageBuffer,"%*d %*d %d.%d.%d.%d %n",&destinationNodeIP[0],&destinationNodeIP[1],&destinationNodeIP[2],&destinationNodeIP[3],&nChars);
        // Check if i am the final destination of the message
        if(isIPEqual(destinationNodeIP,myIP)){
            char* token = strtok(messageBuffer+nChars, " ");
            while (token != NULL) {
                sscanf(token,"%d.%d.%d.%d",&IP[0],&IP[1],&IP[2],&IP[3]);
                // Check if the nodeIP already exists in the middleware metrics table
                assignIP(possibleParents[i],IP);
                token = strtok(NULL, " ");
                i++;
            }
        }else{ // If not, forward the message to the nextHop to the destination
            nextHopIP = findRouteToNode(destinationNodeIP);
            if (nextHopIP != nullptr){
                sendMessage(nextHopIP,messageBuffer);
            }
        }
    }else if(type == TOP_PARENT_REASSIGNMENT_COMMAND){
        sscanf(messageBuffer,"%*d %*d %d.%d.%d.%d",&destinationNodeIP[0],&destinationNodeIP[1],&destinationNodeIP[2],&destinationNodeIP[3]);
        // Check if i am the final destination of the message
        if(isIPEqual(destinationNodeIP,myIP)){
            // In this strategy, PARENT_REASSIGNMENT_COMMAND messages are only expected during the join phase, not during normal operation when this function runs
            LOG(NETWORK, ERROR, "❌ ERROR: Received a TOP_PARENT_REASSIGNMENT_COMMAND unexpectedly\n");
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

void rewriteSenderIPPubSub(char* messageBuffer, size_t bufferSize, TopologyMessageType type){

    if(type == TOP_PARENT_LIST_ADVERTISEMENT){

    }else if(type ==TOP_PARENT_REASSIGNMENT_COMMAND){

    }
}

parentInfo chooseParentProcedure(parentInfo* possibleParents, int nrOfPossibleParents){
    int mySTAIP[4], temporaryParent[4],assignedParent[4];
    int packetSize=0, receivedMessageType;
    bool isExpectedMessage=false;

    // Select the first element of the possibleParents List to be the temporary parent
    if(nrOfPossibleParents != 0){
        connectToAP(possibleParents[0].ssid,PASS);
        assignIP(temporaryParent,possibleParents[0].parentIP);
        getMySTAIP(mySTAIP);
    }
    // Encode the PARENT_LIST_ADVERTISEMENT to send to the root of the network
    encodeMessageParentListAdvertisement(largeSendBuffer, sizeof(largeSendBuffer), possibleParents, nrOfPossibleParents, temporaryParent, mySTAIP);

    // Send the message to the temporary parent so it can be forwarded toward the root
    sendMessage(temporaryParent,largeSendBuffer);

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

    //Parse the received TOP_PARENT_REASSIGNMENT_COMMAND
    if(isExpectedMessage == true){
        sscanf(receiveBuffer,"%*d %*d %*d.%*d.%*d.%*d %d.%d.%d.%d",&assignedParent[0],&assignedParent[1],&assignedParent[2],&assignedParent[3]);
        // Search in the possibleParents the chosen one
        for (int i = 0; i < nrOfPossibleParents; i++) {
            if(isIPEqual(possibleParents[i].parentIP, assignedParent)){
                return possibleParents[i];
            }
        }
    }else{
        return chooseParent(possibleParents,nrOfPossibleParents);
    }

    return possibleParents[0];

}