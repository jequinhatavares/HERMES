#include "network_monitoring.h"

NetworkMonitoring monitoring;

void NetworkMonitoring::handleMonitoringMessage(char *messageBuffer) {
    uint8_t destinationNode[4];
    MonitoringMessageType type;
    int forwardToRoot=0;

    sscanf(messageBuffer, "%*d %d", &type);

    /*** All monitoring messages are sent to the root node, except for the message that measures end-to-end delay
      * (END_TO_END_DELAY). This message is first sent to a node and then forwarded to the root. ***/
    if(type == END_TO_END_DELAY){
        sscanf(messageBuffer, "%*d %*d %d %hhu.%hhu.%hhu.%hhu",&forwardToRoot,&destinationNode[0],&destinationNode[1],&destinationNode[2],&destinationNode[3]);

        if(forwardToRoot == 1){ // This message has been echoed back by the destination node and is now returning to the root.

            /*** Root node received a delayed echo message, discarding outdated measurement.This message was expected
             * during the active monitoring window but arrived after the timeout period. ***/
            if(!iamRoot)return;

            if(!sendMessageToNode(messageBuffer,rootIP)){
                LOG(NETWORK, ERROR, "❌-Monitoring Server-Routing failed: No route found to node %d.%d.%d.%d. "
                                    "Unable to forward message.\n", rootIP[0], rootIP[1],rootIP[2], rootIP[3]);
            }
        }else{ //If the message didn't arrive yet to the final destination
            //If the destination of this message is this then reroute the message to the root node
            if(isIPEqual(destinationNode,myIP)){
                markEndToEndDelayReceivedByDestinationNode(monitoringBuffer, sizeof(monitoringBuffer),myIP);
                //Send it to the root node
                if(!sendMessageToNode(monitoringBuffer,rootIP)){
                    LOG(NETWORK, ERROR, "❌-Monitoring Server-Routing failed: No route found to node %d.%d.%d.%d. "
                                        "Unable to forward message.\n", rootIP[0], rootIP[1],rootIP[2], rootIP[3]);
                }
            }else{ //If the message is not destined to this node forward it to the destination node
                if(!sendMessageToNode(messageBuffer,destinationNode)){
                    LOG(NETWORK, ERROR, "❌-Monitoring Server-Routing failed: No route found to node %d.%d.%d.%d. "
                                        "Unable to forward message.\n", destinationNode[0], destinationNode[1],destinationNode[2], destinationNode[3]);
                }
            }
        }
    }else{ //Other MONITORING_MESSAGE sub types are handled were
        //If this message is not intended for this node, forward it to the next hop leading to its destination.
        if(!iamRoot){
            if(!sendMessageToNode(messageBuffer,rootIP)){
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

void NetworkMonitoring::encodeAppLevelMessage(char*appLevelMonitoringMessage, char*encodeMessageBuffer, size_t encodeBufferSize){
    snprintf(encodeMessageBuffer, encodeBufferSize,"%d %d %s\n", MONITORING_MESSAGE,APP_LEVEL,appLevelMonitoringMessage);
}

void  NetworkMonitoring::encodeEndToEndDelayMessageToNode(char* encodeMessageBuffer,size_t encodeBufferSize,uint8_t *nodeIP){
    //MONITORING_MESSAGE END_TO_END_DELAY [bool=is the message round tripped] [destinationIP]
    snprintf(encodeMessageBuffer,encodeBufferSize,"%d %d 0 %hhu.%hhu.%hhu.%hhu\n",MONITORING_MESSAGE,END_TO_END_DELAY,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
}

void NetworkMonitoring::markEndToEndDelayReceivedByDestinationNode(char*encodeMessageBuffer,size_t encodeBufferSize,uint8_t destinationIP[4]){
    snprintf(encodeMessageBuffer,encodeBufferSize,"%d %d 1 %hhu.%hhu.%hhu.%hhu\n",MONITORING_MESSAGE,END_TO_END_DELAY,destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3]);
}

int NetworkMonitoring::encodeNodeEndToEndDelayToServer(char *encodeMessageBuffer, size_t encodeBufferSize, unsigned long delay,int numberOfHops,uint8_t nodeIP[4]){
    int nChars=0;
    //MONITORING_MESSAGE END_TO_END_DELAY [delay value] [number of Hops]
    snprintf(encodeMessageBuffer,encodeBufferSize," %hhu.%hhu.%hhu.%hhu %lu %d %n",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3],delay,numberOfHops,&nChars);
    return nChars;
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
        if(!sendMessageToNode(monitoringBuffer,rootIP)){
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
        if(!sendMessageToNode(monitoringBuffer,rootIP)){
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
        if(!sendMessageToNode(monitoringBuffer,rootIP)){
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
        if(!sendMessageToNode(monitoringBuffer,rootIP)){
            LOG(NETWORK, ERROR, "❌ No path to the root node was found in the routing table.\n");
        }
    }else LOG(MONITORING_SERVER,DEBUG,"%s",monitoringBuffer);//If i am the root print the message to the monitoring server

#endif
}


void NetworkMonitoring::reportMessagesReceived(){
#ifdef MONITORING_ON
    //MONITORING_MESSAGE MESSAGES_SENT [N Routing Messages Sent] [N Bytes sent] [N Lifecycle Messages Sent] [N Bytes sent]
    // [N Middleware Messages Sent] [N Bytes sent] [N App Messages Sent] [N Bytes sent] [N Monitoring Messages Sent] [N Bytes sent]
#if defined(ESP8266)
    sprintf(monitoringBuffer,"%d %d 1 %d %d %d %d %d %d %d %d %d %d\n",MONITORING_MESSAGE,MESSAGES_RECEIVED,nRoutingMessages,nRoutingBytes,nLifecycleMessages,nLifecycleBytes,
            nMiddlewareMessages,nMiddlewareBytes,nDataMessages,nDataBytes,nMonitoringMessages,nMonitoringBytes);
#endif

#if defined(ESP32)
    sprintf(monitoringBuffer,"%d %d 2 %d %d %d %d %d %d %d %d %d %d\n",MONITORING_MESSAGE,MESSAGES_RECEIVED,nRoutingMessages,nRoutingBytes,nLifecycleMessages,nLifecycleBytes,
            nMiddlewareMessages,nMiddlewareBytes,nDataMessages,nDataBytes,nMonitoringMessages,nMonitoringBytes);
#endif

#if defined(raspberrypi_3b)
    sprintf(monitoringBuffer,"%d %d 3 %d %d %d %d %d %d %d %d %d %d\n",MONITORING_MESSAGE,MESSAGES_RECEIVED,nRoutingMessages,nRoutingBytes,nLifecycleMessages,nLifecycleBytes,
            nMiddlewareMessages,nMiddlewareBytes,nDataMessages,nDataBytes,nMonitoringMessages,nMonitoringBytes);
#endif
    if(!iamRoot){//If i am not the root send the message to the root
        if(!sendMessageToNode(monitoringBuffer,rootIP)){
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

void NetworkMonitoring::reportMonitoringMessageReceived(size_t nBytes){
    // If the message functionality has already been sampled return
    if(messagesMonitored) return;

    // If the message monitoring functionality isn’t started, start it
    if(!messageMonitoringStarted){
        messageMonitoringStarted=true;
        messageMonitoringStartTime=getCurrentTime();
    }
    nMonitoringMessages++;
    nMonitoringBytes+=nBytes;
}

void NetworkMonitoring::reportAppLevelMonitoringMessage(char *appMonitoringMessage) {
    //If the node is the root himself print the message for the monitoring server
    if(iamRoot) LOG(MONITORING_SERVER,INFO,"%s",appMonitoringMessage);
    else{
        //Send the monitoring message to the root node
        sendMessageToNode(appMonitoringMessage,rootIP);
    }
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

void NetworkMonitoring::sampleEndToEndDelay(){
    uint8_t* nodeIP,*nextHopIP,receivedMessageIP[4];
    bool isExpectedMessage=false;
    char tmpBuffer[20];
    MessageType type;
    MonitoringMessageType subType;
    unsigned long sendTime,currentTime;
    int nEncodedChars=0,packetSize=0;

    //Encode the message header
    nEncodedChars = snprintf(monitoringBuffer, sizeof(monitoringBuffer),"%d %d",MONITORING_MESSAGE,END_TO_END_DELAY);

    /***
     * Measures end-to-end network latency by performing a round-trip time (RTT) test
     * to each node in the network.
     * Operation:
     * 1. Sends an END_TO_END_DELAY message to each node sequentially
     * 2. Each recipient node immediately echoes the message back to the root (source) node
     * 3. Measures round-trip time between sending and receiving the response
     * 4. Calculates one-way delay assuming symmetric paths (RTT/2)
     * 5. Logs the computed latency metrics to the Monitoring Server
    ***/
    for (int i = 0; i < routingTable->numberOfItems; ++i) {
        nodeIP = (uint8_t*) tableKey(routingTable,i);
        nextHopIP = findRouteToNode(nodeIP);
        //If one of the addresses its nullptr continue to the next node
        if(!nodeIP||!nextHopIP) continue;

        //Encode the end to end delay message destined to the current node
        encodeEndToEndDelayMessageToNode(tmpBuffer, sizeof(tmpBuffer),nodeIP);
        sendTime=getCurrentTime();//Sample the send time
        sendMessage(nextHopIP,tmpBuffer);

        //Waits for a message of type MONITORING_MESSAGE from that node
        while(((currentTime - sendTime) <=100000) && !isExpectedMessage){
            packetSize = receiveMessage(receiveBuffer, sizeof(receiveBuffer));
            currentTime = getCurrentTime();
            if(packetSize>0){
                //Verify is the MONITORING_MESSAGE subtype is the expected END_TO_END_DELAY and the message is from the intended node
                sscanf(receiveBuffer, "%d %d %hhu.%hhu.%hhu.%hhu",&type,&subType,&receivedMessageIP[0],&receivedMessageIP[1],&receivedMessageIP[2],&receivedMessageIP[3]);
                if(type==MONITORING_MESSAGE && subType==END_TO_END_DELAY && isIPEqual(nodeIP,receivedMessageIP)) isExpectedMessage=true;
            }
        }

        if(isExpectedMessage){
            nEncodedChars += encodeNodeEndToEndDelayToServer(monitoringBuffer+nEncodedChars, sizeof(monitoringBuffer)-nEncodedChars,
                                            currentTime-sendTime,getDistanceToNode(nodeIP),nodeIP);
        }
        isExpectedMessage=false;
    }

    //Print the information to the serial so the monitoring server
    LOG(MONITORING_SERVER,INFO,"%s\n", monitoringBuffer);

}