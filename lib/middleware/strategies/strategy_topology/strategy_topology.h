#ifndef STRATEGY_TOPOLOGY_H
#define STRATEGY_TOPOLOGY_H

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "routing.h"
#include "messages.h"
#include "logger.h"
#include "../../../time_hal/time_hal.h"
#include "../../../transport_hal/transport_hal.h"
#include "../../../wifi_hal/wifi_hal.h"
#include "../strategy_interface.h"
#include <lifecycle.h> //This dependency can be removed when the config file is created


extern Strategy strategyTopology;

extern int orphanIP[4];
extern int newParentIP[4];

typedef enum TopologyMessageType{
    TOP_PARENT_LIST_ADVERTISEMENT_REQUEST,//0
    TOP_PARENT_LIST_ADVERTISEMENT,//1
    TOP_PARENT_REASSIGNMENT_COMMAND, //2
    TOP_NODE_METRICS_REPORT, //3
} TopologyMessageType;


// Topology strategy context definition
typedef struct TopologyContext{
    void (*setMetric)(void* metric);
} TopologyContext;

void initStrategyTopology(void *topologyMetricValues, size_t topologyMetricStructSize,void (*setValueFunction)(void*,void*),void (*encodeTopologyMetricFunction)(char*,size_t,void *),void (*decodeTopologyMetricFunction)(char*,void *));
void encodeMessageStrategyTopology(char* messageBuffer, size_t bufferSize, int typeTopology);
void handleMessageStrategyTopology(char* messageBuffer, size_t bufferSize);
void onNetworkEventStrategyTopology(int networkEvent, uint8_t involvedIP[4]);
void influenceRoutingStrategyTopology(char* dataMessage);
void onTimerStrategyTopology();
void* getContextStrategyTopology();

void encodeNodeMetricReport(char* messageBuffer, size_t bufferSize, void* metric);
void encodeParentAssignmentCommand(char* messageBuffer, size_t bufferSize, uint8_t * destinationIP, uint8_t * chosenParentIP, uint8_t * targetNodeIP);
void encodeParentListAdvertisementRequest(char* messageBuffer, size_t bufferSize, parentInfo* possibleParents, int nrOfPossibleParents, uint8_t *temporaryParent, uint8_t *mySTAIP);

parentInfo requestParentFromRoot(parentInfo* possibleParents, int nrOfPossibleParents);
void chooseParentStrategyTopology(char* message);

void topologySetNodeMetric(void* metric);


#endif //STRATEGY_TOPOLOGY_H
