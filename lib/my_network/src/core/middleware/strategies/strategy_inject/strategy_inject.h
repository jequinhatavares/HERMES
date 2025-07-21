#ifndef STRATEGY_INJECT_H
#define STRATEGY_INJECT_H

#include <../table/table.h>
#include "routing.h"
#include "messages.h"
#include "logger.h"
#include "../../../time_hal/time_hal.h"
#include "../../../transport_hal/transport_hal.h"
#include "../strategy_interface.h"
//#include "../../routing/messages.h"



extern TableInfo* metricsTable;

struct MetricTableEntry{
    int processingCapacity;
};

typedef enum InjectMessageType{
    INJECT_NODE_INFO,
    INJECT_TABLE_INFO,
} InjectMessageType;

// Inject strategy context definition
typedef struct InjectContext{
    void (*injectNodeMetric)(void* metric);
} InjectContext;

extern Strategy strategyInject;

extern unsigned long lastMiddlewareUpdateTimeInject;

void initStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*),void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *));

void encodeMessageInject(char* messageBuffer, size_t bufferSize, int type);
void encodeMessageStrategyInject(char* messageBuffer, size_t bufferSize, int type);
void handleMessageStrategyInject(char* messageBuffer, size_t bufferSize);
void onNetworkEventStrategyInject(int networkEvent, uint8_t involvedIP[4]);
void influenceRoutingStrategyInject(char* dataMessage);
void onTimerStrategyInject();
void* getContextStrategyInject();

void injectNodeMetric(void* metric);

void encodeMyMetric(char* messageBuffer, size_t bufferSize);
void rewriteSenderIPInject(char* messageBuffer, char* writeBuffer, size_t writeBufferSize, InjectMessageType type);

void registerInjectMetric(uint8_t *nodeIP, char* metricBuffer);

void encodeMetricEntry(char* buffer, size_t bufferSize, void *metricEntry);
void decodeMetricEntry(char* buffer, void *metricEntry);
void setMetricValue(void* av, void*bv);
void printMetricStruct(TableEntry* Table);
void printMetricsTableHeader();
int compareMetrics(void *metricAv,void*metricBv);

void setIP(void* av, void* bv);

#endif //STRATEGY_INJECT_H
