#include "network_monitoring.h"

char monitoringBuffer[100];

void encodeMonitoringMessage(char* msg, MonitoringMessageType type, messageVizParameters parameters){
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

void reportNewNodeToMonitoringServer (uint8_t * nodeIP, uint8_t * parentIP){
#ifdef MONITORING_ON
    char msg[50];
    messageVizParameters vizParameters;
    //If the visualization program is active, pass the new node information to it
    assignIP(vizParameters.IP1, nodeIP);
    assignIP(vizParameters.IP2, parentIP);
    encodeMonitoringMessage(msg,NEW_NODE,vizParameters);

    encodeDebugMessage(monitoringBuffer, sizeof (monitoringBuffer), msg);

    if(!iamRoot){
        uint8_t *nextHopIP = findRouteToNode(rootIP);
        if(nextHopIP != nullptr){
            sendMessage(rootIP,monitoringBuffer);
        }else{
            LOG(NETWORK, ERROR, "❌ ERROR: No path to the root node was found in the routing table.\n");
        }
    }else LOG(MONITORING_SERVER,DEBUG,"%s",monitoringBuffer);

#endif
}

void reportDeletedNodeToMonitoringServer (uint8_t * nodeIP){
#ifdef MONITORING_ON
    char msg[50];
    messageVizParameters vizParameters;
    //If the visualization program is active, pass the deleted node information to it
    assignIP(vizParameters.IP1, nodeIP);
    encodeMonitoringMessage(msg,DELETED_NODE,vizParameters);

    encodeDebugMessage(smallSendBuffer, sizeof (smallSendBuffer),msg);

    if(!iamRoot){//If i am not the root send the message to the root
        uint8_t *nextHopIP = findRouteToNode(rootIP);
        if(nextHopIP != nullptr){
            sendMessage(rootIP,smallSendBuffer);
        }else{
            LOG(NETWORK, ERROR, "❌ No path to the root node was found in the routing table.\n");
        }
    }else LOG(MONITORING_SERVER,DEBUG,"%s",smallSendBuffer);//If i am the root print the message to the monitoring server

#endif
}

void reportLifecycleTimesToMonitoringServer(unsigned long initTime, unsigned long searchTime, unsigned long joinNetworkTime){
#ifdef MONITORING_ON
#if defined(ESP8266)
    sprintf(monitoringBuffer,"%d 1 %lu %lu %lu",LIFECYCLE_TIMES,initTime,searchTime,joinNetworkTime);
#endif

#if defined(ESP32)
    sprintf(monitoringBuffer,"%d 2 %lu %lu %lu",LIFECYCLE_TIMES,initTime,searchTime,joinNetworkTime);
#endif

#if defined(raspberrypi_3b)
    sprintf(monitoringBuffer,"%d 3 %lu %lu %lu",LIFECYCLE_TIMES,initTime,searchTime,joinNetworkTime);
#endif
    if(!iamRoot){//If i am not the root send the message to the root
        uint8_t *nextHopIP = findRouteToNode(rootIP);
        if(nextHopIP != nullptr){
            sendMessage(rootIP,monitoringBuffer);
        }else{
            LOG(NETWORK, ERROR, "❌ No path to the root node was found in the routing table.\n");
        }
    }else LOG(MONITORING_SERVER,DEBUG,"%s",monitoringBuffer);//If i am the root print the message to the monitoring server

#endif
}


void reportParentRecoveryTimeToMonitoringServer(unsigned long parentRecoveryTime){
#ifdef MONITORING_ON
#if defined(ESP8266)
    sprintf(monitoringBuffer,"%d 1 %lu",PARENT_RECOVERY_TIME,parentRecoveryTime);
#endif

#if defined(ESP32)
    sprintf(monitoringBuffer,"%d 2 %lu",PARENT_RECOVERY_TIME,parentRecoveryTime);
#endif

#if defined(raspberrypi_3b)
    sprintf(monitoringBuffer,"%d 3 %lu",PARENT_RECOVERY_TIME,parentRecoveryTime);
#endif
    if(!iamRoot){//If i am not the root send the message to the root
        uint8_t *nextHopIP = findRouteToNode(rootIP);
        if(nextHopIP != nullptr){
            sendMessage(rootIP,monitoringBuffer);
        }else{
            LOG(NETWORK, ERROR, "❌ No path to the root node was found in the routing table.\n");
        }
    }else LOG(MONITORING_SERVER,DEBUG,"%s",monitoringBuffer);//If i am the root print the message to the monitoring server

#endif
}