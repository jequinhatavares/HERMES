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
}MonitoringMessageType;

typedef struct messageVizParameters{
    uint8_t IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
}messageVizParameters;

void encodeMonitoringMessage(char* msg, MonitoringMessageType type, messageVizParameters parameters);

void reportNewNodeToMonitoringServer (uint8_t * nodeIP, uint8_t * parentIP);
void reportDeletedNodeToMonitoringServer (uint8_t* nodeIP);
void reportLifecycleTimesToMonitoringServer(unsigned long initTime, unsigned long searchTime, unsigned long joinNetworkTime);
void reportParentRecoveryTimeToMonitoringServer(unsigned long parentRecoveryTime);
void reportMessagesReceived();

void reportRoutingMessage(size_t nBytes);
void reportLifecycleMessage(size_t nBytes);
void reportMiddlewareMessage(size_t nBytes);
void reportAPPMessage(size_t nBytes);

void handleTimersNetworkMonitoring();

#endif //NETWORK_MONITORING_H
