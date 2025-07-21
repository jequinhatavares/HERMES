#ifndef NETWORK_MONITORING_H
#define NETWORK_MONITORING_H

#include <cstdio>
#include <messages.h>
#include <transport_hal.h>
#include <logger.h>


typedef enum MonitoringMessageType{
    NEW_NODE, //0
    DELETED_NODE, //1
    CHANGE_PARENT, //2
}MonitoringMessageType;

typedef struct messageVizParameters{
    uint8_t IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
}messageVizParameters;

void encodeMonitoringMessage(char* msg, MonitoringMessageType type, messageVizParameters parameters);

void reportNewNodeToMonitoringServer (uint8_t * nodeIP, uint8_t * parentIP);
void reportDeletedNodeToMonitoringServer (uint8_t* nodeIP);


#endif //NETWORK_MONITORING_H
