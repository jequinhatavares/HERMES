#ifndef NETWORK_MONITORING_H
#define NETWORK_MONITORING_H

#include <cstdio>
#include "../routing/messages.h"
#include "../transport_hal/transport_hal.h"
#include "../logger/logger.h"

#define MONITORING_ON

typedef enum MonitoringMessageType{
    NEW_NODE,               //0
    DELETED_NODE,           //1
    CHANGE_PARENT,          //2
    LIFECYCLE_TIMES,        //3
    PARENT_RECOVERY_TIME,   //4
    MESSAGES_RECEIVED,      //5
    END_TO_END_DELAY,       //6
}MonitoringMessageType;

typedef struct messageVizParameters{
    uint8_t IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
}messageVizParameters;



class NetworkMonitoring{
public:

    void handleMonitoringMessage(char* messageBuffer);
    void handleTimersNetworkMonitoring();
    void encodeMessage(char* msg, MonitoringMessageType type, messageVizParameters parameters);

    void reportNewNode(uint8_t * nodeIP, uint8_t * parentIP);
    void reportDeletedNode(uint8_t* nodeIP);
    void reportLifecycleTimes(unsigned long initTime, unsigned long searchTime, unsigned long joinNetworkTime);
    void reportParentRecoveryTime(unsigned long parentRecoveryTime);
    void reportMessagesReceived();

    void reportRoutingMessageReceived(size_t nBytes);
    void reportLifecycleMessageReceived(size_t nBytes);
    void reportMiddlewareMessageReceived(size_t nBytes);
    void reportDataMessageReceived(size_t nBytes);
    void reportMonitoringMessageReceived(size_t nBytes);

    void sampleEndToEndDelay();


private:
    //Buffer used to encode the monitoring messages
    char monitoringBuffer[100];
    //TimeStamp that the server started to take the volume of messages that the node sends
    unsigned long messageMonitoringStartTime;
    //If the monitoring messages has already started
    bool messageMonitoringStarted=false;
    //The time interval that the node monitors the volume of messages sent by the node
    #define MESSAGE_MONITORING_TIME 300000 //5 minutes
    // If a sample of the number of messages has already been taken
    bool messagesMonitored=false;

    // Routing protocol message metrics
    int nRoutingMessages=0; // Count of routing messages
    int nRoutingBytes=0;    // Total bytes routing messages received

    // Node lifecycle message metrics
    int nLifecycleMessages=0;   // Count of lifecycle messages
    int nLifecycleBytes=0;      // Total bytes lifecycle messages received

    // Middleware infrastructure message metrics
    int nMiddlewareMessages=0;  // Count of middleware messages
    int nMiddlewareBytes=0;     // Total bytes middleware messages received

    // Application Level message metrics
    int nDataMessages=0; // Count of data messages
    int nDataBytes=0;    // Total bytes of actual data received

    // Monitoring Level message metrics
    int nMonitoringMessages=0; // Count of monitoring messages
    int nMonitoringBytes=0;    // Total bytes of actual monitoring received

    void markEndToEndDelayReceivedByDestinationNode(char*encodeMessageBuffer,size_t encodeBufferSize,uint8_t destinationIP[4]);
    static static void encodeEndToEndDelayMessageToNode(char* encodeMessageBuffer,size_t encodeBufferSize,uint8_t *nodeIP);
    int encodeNodeEndToEndDelayToServer(char*encodeBuffer,size_t encodeBufferSize,unsigned long delay, int numberOfHops, uint8_t nodeIP[4]);
};

extern NetworkMonitoring monitoring;


#endif //NETWORK_MONITORING_H
