#ifndef STRATEGY_TOPOLOGY_H
#define STRATEGY_TOPOLOGY_H


#include "../../../routing/routing.h"
#include "../../../routing/messages.h"
#include "../../../logger/logger.h"
#include "../../../time_hal/time_hal.h"
#include "../../../transport_hal/transport_hal.h"
#include "../../../wifi_hal/wifi_hal.h"
#include "../strategy_interface.h"


#include <cstdlib>
#include <cstring>
#include <cstdint>

extern Strategy strategyTopology;

extern int orphanIP[4];
extern int newParentIP[4];

extern TableInfo* topologyMetricsTable;


typedef enum TopologyMessageType{
    TOP_PARENT_LIST_ADVERTISEMENT_REQUEST,//0
    TOP_PARENT_LIST_ADVERTISEMENT,//1
    TOP_PARENT_ASSIGNMENT_COMMAND, //2
    TOP_METRICS_REPORT, //3
    TOP_NODE_UPDATE, //4
} TopologyMessageType;


// Topology strategy context definition
typedef struct TopologyContext{
    void (*setParentMetric)(void* metric);
    void* (*getParentMetric)(uint8_t * nodeIP);
} TopologyContext;



void initStrategyTopology(void *topologyMetricValues, size_t topologyMetricStructSize,void (*setValueFunction)(void*,void*),void (*encodeTopologyMetricFunction)(char*,size_t,void *),void (*decodeTopologyMetricFunction)(char*,void *),void (*printMetricFunction)(TableEntry*),uint8_t* (*selectParentFunction)(uint8_t *, uint8_t (*)[4], uint8_t));
void handleMessageStrategyTopology(char* messageBuffer, size_t bufferSize);
void onNetworkEventStrategyTopology(int networkEvent, uint8_t involvedIP[4]);
void influenceRoutingStrategyTopology(char* messageEncodeBuffer,size_t encodeBufferSize,char* dataMessagePayload);
void onTimerStrategyTopology();
void* getContextStrategyTopology();

void registerTopologyMetric(uint8_t *nodeIP, char* metricBuffer);

void encodeNodeMetricReport(char* messageBuffer, size_t bufferSize, void* metric);
void encodeParentAssignmentCommand(char* messageBuffer, size_t bufferSize, uint8_t * destinationIP, uint8_t * chosenParentIP, uint8_t * targetNodeIP);
void encodeParentListAdvertisementRequest(char* messageBuffer, size_t bufferSize, ParentInfo* possibleParents, int nrOfPossibleParents, uint8_t *temporaryParent, uint8_t *mySTAIP);
void encodeNodeUpdateMessage(char* messageBuffer, size_t bufferSize);

ParentInfo requestParentFromRoot(ParentInfo* possibleParents, int nrOfPossibleParents);
void chooseParentStrategyTopology(char* message);

void topologySetNodeMetric(void* metric);

void printTopologyTable();
void printTopologyTableHeader();

void* getNodeTopologyMetric(uint8_t * nodeIP);

/******************************-----------Application Defined Functions----------------********************************/

/***uint8_t * chooseParentByProcessingCapacity(uint8_t * targetNodeIP, uint8_t (*potentialParents)[4], uint8_t nPotentialParents);
void encodeTopologyMetricEntry(char* buffer, size_t bufferSize, void *metricEntry);
void decodeTopologyMetricEntry(char* buffer, void *metricEntry);
void setTopologyMetricValue(void* av, void*bv);
void printTopologyMetricStruct(TableEntry* Table);***/


#endif //STRATEGY_TOPOLOGY_H
