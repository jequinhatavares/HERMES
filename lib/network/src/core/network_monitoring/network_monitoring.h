#ifndef NETWORK_MONITORING_H
#define NETWORK_MONITORING_H

#include <cstdio>
#include <cstdint>

#define MONITORING_ON

typedef enum MonitoringMessageType{
    NEW_NODE,               // 0 - Notifies monitoring system of new node joining the network
    DELETED_NODE,           // 1 - Alerts when a node leaves or becomes unreachable
    CHANGE_PARENT,          // 2 - Indicates node parent reassignment in network topology
    LIFECYCLE_TIMES,        // 3 - Reports node startup state duration metrics
    PARENT_RECOVERY_TIME,   // 4 - Measures time to reconnect after parent node failure
    MESSAGES_BATCHED,       // 5 - Tracks message count statistics for traffic analysis
    MESSAGES_CONTINUOUS,    // 6 - Tracks message count statistics for traffic analysis
    END_TO_END_DELAY,       // 7 - Measures round-trip latency between nodes
    APP_LEVEL,              // 8 - Carries application-specific performance metrics
}MonitoringMessageType;

typedef struct messageVizParameters{
    uint8_t IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
}messageVizParameters;


class NetworkMonitoring{
public:
    //Buffer used to encode the monitoring messages
    char monitoringBuffer[100];

    void handleMonitoringMessage(char* messageBuffer);
    void handleTimersNetworkMonitoring();
    void encodeMessage(char* msg, MonitoringMessageType type, messageVizParameters parameters);
    void encodeAppLevelMessage(char*appLevelMonitoringMessage, char*encodeMessageBuffer, size_t encodeBufferSize);

    void reportAppLevelMonitoringMessage(char* appMonitoringMessage);

    void reportNewNode(uint8_t * nodeIP, uint8_t * parentIP);
    void reportDeletedNode(uint8_t* nodeIP);
    void reportLifecycleTimes(unsigned long initTime, unsigned long searchTime, unsigned long joinNetworkTime);
    void reportParentRecoveryTime(unsigned long parentRecoveryTime);
    void reportMessagesReceived();

    void reportRoutingMessageReceived(size_t nBytes,int messageType);
    void reportLifecycleMessageReceived(size_t nBytes,int messageType);
    void reportMiddlewareMessageReceived(size_t nBytes,int messageType,int strategyType,int messageSubType);
    void reportDataMessageReceived(size_t nBytes,int messageType, int messageSubType);
    void reportMonitoringMessageReceived(size_t nBytes,int messageType);

    void sampleEndToEndDelay();
    void sampleMessageMetrics(unsigned long sampleTime);

private:
    //TimeStamp that the server started to take the volume of messages that the node sends
    unsigned long messageMonitoringStartTime;
    //If the monitoring messages has already started
    bool messageMonitoringStarted=false;
    //The time interval that the node monitors the volume of messages sent by the node
    unsigned long messageMonitoringTime = 300000; //5 minutes
    // If a sample of the number of messages has already been taken
    bool messagesMonitored=false;

    // Routing protocol message metrics
    int nRoutingMessages=0;     // Count of routing messages
    int nRoutingBytes=0;        // Total bytes routing messages received

    // Node lifecycle message metrics
    int nLifecycleMessages=0;   // Count of lifecycle messages
    int nLifecycleBytes=0;      // Total bytes lifecycle messages received

    // Middleware infrastructure message metrics
    int nMiddlewareMessages=0;  // Count of middleware messages
    int nMiddlewareBytes=0;     // Total bytes middleware messages received

    // Application Level message metrics
    int nDataMessages=0;        // Count of data messages
    int nDataBytes=0;           // Total bytes of actual data received

    // Monitoring Level message metrics
    int nMonitoringMessages=0;  // Count of monitoring messages
    int nMonitoringBytes=0;     // Total bytes of actual monitoring received

    void markEndToEndDelayReceivedByDestinationNode(char*encodeMessageBuffer,size_t encodeBufferSize,uint8_t destinationIP[4]);
    static void encodeEndToEndDelayMessageToNode(char* encodeMessageBuffer,size_t encodeBufferSize,uint8_t *nodeIP);
    int encodeNodeEndToEndDelayToServer(char*encodeBuffer,size_t encodeBufferSize,unsigned long delay, int numberOfHops, uint8_t nodeIP[4]);
    void encodeMessageContinuousToServer(char *encodeBuffer, size_t encodeBufferSize, int messageType, int strategyType,int messageSubType, int nBytes);

    void handleEndToEndDelayMessage(char* messageBuffer);
};

extern NetworkMonitoring monitoring;


#endif //NETWORK_MONITORING_H
