#include "strategy_topology.h"

int nodeIP[4];
int newParentIP[4];

void initStrategyTopology(){

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

            offset += snprintf(messageBuffer + offset, bufferSize - offset,"%i.%i.%i.%i",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
        }

    }else if(typeTopology == TOP_PARENT_REASSIGNMENT_COMMAND){
        //MESSAGE_TYPE TOP_PARENT_REASSIGNMENT_COMMAND [nodeIP] [parentIP]
        snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i",MIDDLEWARE_MESSAGE,TOP_PARENT_REASSIGNMENT_COMMAND,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3],
                 newParentIP[0],newParentIP[1],newParentIP[2],newParentIP[3]);
    }
}
void handleMessageStrategyTopology(char* messageBuffer, size_t bufferSize){

}
void onNetworkEventStrategyTopology(int networkEvent, int involvedIP[4]){

}
void influenceRoutingStrategyTopology(char* dataMessage){

}
void onTimerStrategyTopology(){

}
void* getContextStrategyTopology(){
    return nullptr;
}

void rewriteSenderIPPubSub(char* messageBuffer, size_t bufferSize, TopologyMessageType type){

    if(type == TOP_PARENT_LIST_ADVERTISEMENT){

    }else if(type ==TOP_PARENT_REASSIGNMENT_COMMAND){

    }
}