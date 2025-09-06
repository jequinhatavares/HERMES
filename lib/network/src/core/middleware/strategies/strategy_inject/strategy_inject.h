#ifndef STRATEGY_INJECT_H
#define STRATEGY_INJECT_H

#include "../../../table/table.h"
#include "../../../routing/routing.h"
#include "../../../routing/messages.h"
#include "../../../logger/logger.h"
#include "../../../time_hal/time_hal.h"
#include "../../../transport_hal/transport_hal.h"
#include "../strategy_interface.h"



extern TableInfo* metricsTable;

struct MetricTableEntry{
    int processingCapacity;
};

typedef enum InjectMessageType{
    INJECT_NODE_INFO,   //Contains the metric info about a single node
    INJECT_TABLE_INFO,  //Contains all metrics of all nodes
} InjectMessageType;

// Inject strategy context definition
typedef struct InjectContext{
    void (*injectNodeMetric)(void* metric);
    void (*influenceRouting)(char* messageEncodeBuffer,size_t encodeBufferSize,char* dataMessagePayload,uint8_t* finalDestination);
} InjectContext;

extern Strategy strategyInject;

extern unsigned long lastMiddlewareUpdateTimeInject;

void initStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*),void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *),int(*compareMetricsFunction)(void*,void*),void (*printMetricStruct)(TableEntry*));

void encodeMessageInject(char* messageBuffer, size_t bufferSize, int type);
bool encodeMessageStrategyInject(char* messageBuffer, size_t bufferSize, int type);
void handleMessageStrategyInject(char* messageBuffer, size_t bufferSize);
void onNetworkEventStrategyInject(int networkEvent, uint8_t involvedIP[4]);
void influenceRoutingStrategyInject(char* messageEncodeBuffer,size_t encodeBufferSize,char* dataMessagePayload,uint8_t* finalDestination);
void onTimerStrategyInject();
void* getContextStrategyInject();

void injectNodeMetric(void* metric);

bool encodeMyMetric(char* messageBuffer, size_t bufferSize);
void rewriteSenderIPInject(char* messageBuffer, char* writeBuffer, size_t writeBufferSize, InjectMessageType type);

void registerInjectMetric(uint8_t *nodeIP, char* metricBuffer);

void printInjectTable();
//void encodeMetricEntry(char* buffer, size_t bufferSize, void *metricEntry);
//void decodeMetricEntry(char* buffer, void *metricEntry);
//void setMetricValue(void* av, void*bv);
void printMetricsTableHeader();


void setIP(void* av, void* bv);

#endif //STRATEGY_INJECT_H
