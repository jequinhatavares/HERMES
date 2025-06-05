#include "net_viz.h"


void encodeVizMessage(char* msg, messageVizType type, messageVizParameters parameters){
    switch (type) {
        case NEW_NODE:
            //0 [nodeIP] [parentIP]
            sprintf(msg,"0 %i.%i.%i.%i %i.%i.%i.%i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;
        case DELETED_NODE:
            //1 [nodeIP]
            sprintf(msg,"1 %i.%i.%i.%i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3]);
            break;
        case CHANGE_PARENT:
            break;
    }
}

void reportNewNodeToViz(int* nodeIP, int* parentIP){
#ifdef VISUALIZATION_ON
    char msg[50];
    messageParameters parameters;
    messageVizParameters vizParameters;
    //If the visualization program is active, pass the new node information to it
    assignIP(vizParameters.IP1, nodeIP);
    assignIP(vizParameters.IP2, parentIP);
    encodeVizMessage(msg,NEW_NODE,vizParameters);

    sprintf(parameters.payload, "%s", msg);
    strcpy(msg , "");
    encodeMessage(msg, sizeof (msg),DEBUG_MESSAGE, parameters);

    if(!iamRoot){
        int *nextHopIP = findRouteToNode(rootIP);
        if(nextHopIP != nullptr){
            sendMessage(rootIP,msg);
        }else{
            LOG(NETWORK, ERROR, "❌ ERROR: No path to the root node was found in the routing table.\n");
        }
    }else LOG(DEBUG_SERVER,DEBUG,msg);

#endif
}

void reportDeletedNodeToViz(int* nodeIP){
#ifdef VISUALIZATION_ON
    char msg[50];
    messageParameters parameters;
    messageVizParameters vizParameters;
    //If the visualization program is active, pass the deleted node information to it
    assignIP(vizParameters.IP1, nodeIP);
    encodeVizMessage(msg,DELETED_NODE,vizParameters);

    sprintf(parameters.payload, "%s", msg);
    strcpy(msg , "");
    encodeMessage(msg, sizeof (msg),DEBUG_MESSAGE, parameters);


    if(!iamRoot){//If i am not the root send the message to the root
        int *nextHopIP = findRouteToNode(rootIP);
        if(nextHopIP != nullptr){
            sendMessage(rootIP,msg);
        }else{
            LOG(NETWORK, ERROR, "❌ No path to the root node was found in the routing table.\n");
        }
    }else LOG(DEBUG_SERVER,DEBUG,msg);//If i am the root print the message to the monitoring server

#endif
}