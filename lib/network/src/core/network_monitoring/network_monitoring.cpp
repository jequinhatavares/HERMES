#include "network_monitoring.h"

NetworkMonitoring monitoring;

void NetworkMonitoring::handleMonitoringMessage(char *messageBuffer) {
    uint8_t destinationNode[4];
    MonitoringMessageType type;

    sscanf(messageBuffer, "%*d %d", &type);

    /*** All monitoring messages are sent to the root node, except for the message that measures end-to-end delay
      * (END_TO_END_DELAY). This message is first sent to a node and then forwarded to the root. ***/
    if(type == END_TO_END_DELAY){
        sscanf(messageBuffer, "%*d %*d %hhu.%hhu.%hhu.%hhu",&destinationNode[0],&destinationNode[1],&destinationNode[2],&destinationNode[3]);
        //If the destination of this message is this then reroute the message to the root node
        if(isIPEqual(destinationNode,myIP)){
            uint8_t * nextHopPtr = findRouteToNode(rootIP);
            if (nextHopPtr != nullptr){
                assignIP(nextHopPtr, nextHopPtr);
                sendMessage(nextHopPtr,messageBuffer);
            }else{
                LOG(NETWORK, ERROR, "❌-Monitoring Server-Routing failed: No route found to node %d.%d.%d.%d. "
                                    "Unable to forward message.\n", rootIP[0], rootIP[1],rootIP[2], rootIP[3]);
            }

        }else{ //If the message is not destined to this node forward it to the destination node
            uint8_t * nextHopPtr = findRouteToNode(destinationNode);
            if (nextHopPtr != nullptr){
                sendMessage(nextHopPtr,messageBuffer);
            }else{
                LOG(NETWORK, ERROR, "❌-Monitoring Server-Routing failed: No route found to node %d.%d.%d.%d. "
                                    "Unable to forward message.\n", destinationNode[0], destinationNode[1],destinationNode[2], destinationNode[3]);
            }
        }
    }else{
        //If this message is not intended for this node, forward it to the next hop leading to its destination.
        if(!iamRoot){
            uint8_t * nextHopPtr = findRouteToNode(rootIP);
            if (nextHopPtr != nullptr){
                sendMessage(nextHopPtr,messageBuffer);
            }else{
                LOG(NETWORK, ERROR, "❌-Monitoring Server-Routing failed: No route found to node %d.%d.%d.%d. "
                                    "Unable to forward message.\n", rootIP[0], rootIP[1],rootIP[2], rootIP[3]);
            }
        }else{//send message to debug server
            LOG(MONITORING_SERVER,DEBUG,messageBuffer);
            //sendMessage(debugServerIP, receiveBuffer);
        }
    }

}

void NetworkMonitoring::encodeMessage(char* msg, MonitoringMessageType type, messageVizParameters parameters){
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

void encodeEndToEndDelayMessage(){
    //MONITORING_MESSAGE END_TO_END_DELAY [bool=is the message round tripped] [destinationIP] sendTime
}

void NetworkMonitoring::reportNewNode(uint8_t * nodeIP, uint8_t * parentIP){
#ifdef MONITORING_ON
    char msg[50];
    messageVizParameters vizParameters;
    //If the visualization program is active, pass the new node information to it
    assignIP(vizParameters.IP1, nodeIP);
    assignIP(vizParameters.IP2, parentIP);
    encodeMessage(msg,NEW_NODE,vizParameters);

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

void NetworkMonitoring::reportDeletedNode(uint8_t * nodeIP){
#ifdef MONITORING_ON
    char msg[50];
    messageVizParameters vizParameters;
    //If the visualization program is active, pass the deleted node information to it
    assignIP(vizParameters.IP1, nodeIP);
    encodeMessage(msg,DELETED_NODE,vizParameters);

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

void NetworkMonitoring::reportLifecycleTimes(unsigned long initTime, unsigned long searchTime, unsigned long joinNetworkTime){
#ifdef MONITORING_ON

#if defined(ESP8266)
    sprintf(monitoringBuffer,"%d %d 1 %lu %lu %lu\n",MONITORING_MESSAGE,LIFECYCLE_TIMES,initTime,searchTime,joinNetworkTime);
#endif

#if defined(ESP32)
    sprintf(monitoringBuffer,"%d %d 2 %lu %lu %lu\n",MONITORING_MESSAGE,LIFECYCLE_TIMES,initTime,searchTime,joinNetworkTime);
#endif

#if defined(raspberrypi_3b)
    sprintf(monitoringBuffer,"%d %d 3 %lu %lu %lu\n",MONITORING_MESSAGE,LIFECYCLE_TIMES,initTime,searchTime,joinNetworkTime);
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


void NetworkMonitoring::reportParentRecoveryTime(unsigned long parentRecoveryTime){
#ifdef MONITORING_ON
#if defined(ESP8266)
    sprintf(monitoringBuffer,"%d %d 1 %lu\n",MONITORING_MESSAGE,PARENT_RECOVERY_TIME,parentRecoveryTime);
#endif

#if defined(ESP32)
    sprintf(monitoringBuffer,"%d %d 2 %lu\n",MONITORING_MESSAGE,PARENT_RECOVERY_TIME,parentRecoveryTime);
#endif

#if defined(raspberrypi_3b)
    sprintf(monitoringBuffer,"%d %d 3 %lu\n",MONITORING_MESSAGE,PARENT_RECOVERY_TIME,parentRecoveryTime);
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


void NetworkMonitoring::reportMessagesReceived(){
#ifdef MONITORING_ON
    //MONITORING_MESSAGE MESSAGES_SENT [N Routing Messages Sent] [N Bytes sent] [N Lifecycle Messages Sent] [N Bytes sent]
    // [N Middleware Messages Sent] [N Bytes sent] [N App Messages Sent] [N Bytes sent]
#if defined(ESP8266)
    sprintf(monitoringBuffer,"%d %d 1 %d %d %d %d %d %d %d %d\n",MONITORING_MESSAGE,MESSAGES_RECEIVED,nRoutingMessages,nRoutingBytes,nLifecycleMessages,nLifecycleBytes,
            nMiddlewareMessages,nMiddlewareBytes,nDataMessages,nDataBytes);
#endif

#if defined(ESP32)
    sprintf(monitoringBuffer,"%d %d 2 %d %d %d %d %d %d %d %d\n",MONITORING_MESSAGE,MESSAGES_RECEIVED,nRoutingMessages,nRoutingBytes,nLifecycleMessages,nLifecycleBytes,
            nMiddlewareMessages,nMiddlewareBytes,nDataMessages,nDataBytes);
#endif

#if defined(raspberrypi_3b)
    sprintf(monitoringBuffer,"%d %d 3 %d %d %d %d %d %d %d %d\n",MONITORING_MESSAGE,MESSAGES_RECEIVED,nRoutingMessages,nRoutingBytes,nLifecycleMessages,nLifecycleBytes,
            nMiddlewareMessages,nMiddlewareBytes,nDataMessages,nDataBytes);
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


void NetworkMonitoring::reportRoutingMessageReceived(size_t nBytes){
    // If the message functionality has already been sampled return
    if(messagesMonitored) return;

    // If the message monitoring functionality isn’t started, start it
    if(!messageMonitoringStarted){
        messageMonitoringStarted=true;
        messageMonitoringStartTime=getCurrentTime();
    }
    nRoutingMessages++;
    nRoutingBytes+=nBytes;
}

void NetworkMonitoring::reportLifecycleMessageReceived(size_t nBytes){
    // If the message functionality has already been sampled return
    if(messagesMonitored) return;

    // If the message monitoring functionality isn’t started, start it
    if(!messageMonitoringStarted){
        messageMonitoringStarted=true;
        messageMonitoringStartTime=getCurrentTime();
    }
    nLifecycleMessages++;
    nLifecycleBytes+=nBytes;
}

void NetworkMonitoring::reportMiddlewareMessageReceived(size_t nBytes){
    // If the message functionality has already been sampled return
    if(messagesMonitored) return;

    // If the message monitoring functionality isn’t started, start it
    if(!messageMonitoringStarted){
        messageMonitoringStarted=true;
        messageMonitoringStartTime=getCurrentTime();
    }
    nMiddlewareMessages++;
    nMiddlewareBytes+=nBytes;
}

void NetworkMonitoring::reportDataMessageReceived(size_t nBytes){
    // If the message functionality has already been sampled return
    if(messagesMonitored) return;

    // If the message monitoring functionality isn’t started, start it
    if(!messageMonitoringStarted){
        messageMonitoringStarted=true;
        messageMonitoringStartTime=getCurrentTime();
    }
    nDataMessages++;
    nDataBytes+=nBytes;
}

void NetworkMonitoring::handleTimersNetworkMonitoring(){
    unsigned long currentTime = getCurrentTime();

    if(messageMonitoringStarted && (currentTime-messageMonitoringStartTime)>=MESSAGE_MONITORING_TIME){
        reportMessagesReceived();
        messagesMonitored=true;
        //Reset the variables
        messageMonitoringStarted=false;
        nRoutingMessages=0;
        nRoutingBytes=0;
        nLifecycleMessages=0;
        nLifecycleBytes=0;
        nMiddlewareMessages=0;
        nMiddlewareBytes=0;
        nDataMessages=0;
        nDataBytes=0;
    }
}


