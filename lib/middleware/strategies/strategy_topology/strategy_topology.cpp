#include "strategy_topology.h"

int nodeIP[4];
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
    char tmpBuffer[20], tmpBuffer2[50];

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
        snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_REASSIGNMENT_COMMAND,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3],
                 newParentIP[0],newParentIP[1],newParentIP[2],newParentIP[3]);
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
            sscanf(messageBuffer,"%*d %*d %*d.%*d.%*d.%*d %d.%d.%d.%d",&newParent[0],&newParent[1],&newParent[2],&newParent[3]);
            //Change parent to the assigned one
            //TODO Change parent Function
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
        /***encodeMessageStrategyPubSub(largeSendBuffer, sizeof(largeSendBuffer), PUBSUB_NODE_UPDATE);
        propagateMessage(largeSendBuffer, myIP);
        LOG(NETWORK,DEBUG,"Sending periodic [MIDDLEWARE/PUBSUB_NODE_INFO] Message: %s\n",largeSendBuffer);
        lastMiddlewareUpdateTimeTopology = currentTime;***/
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