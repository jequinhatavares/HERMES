#ifndef STRATEGY_INJECT_H
#define STRATEGY_INJECT_H

#include <../table/table.h>
#include "routing.h"
//#include "../../routing/messages.h"
#include "messages.h"
#include "logger.h"
#include "../../../time_hal/time_hal.h"
#include "../../../transport_hal/transport_hal.h"
#include "../strategy_interface.h"

#define MIDDLEWARE_UPDATE_INTERVAL 120000
extern TableInfo* metricTable;

struct metricTableEntry{
    int processingCapacity;
};

typedef enum InjectMessageType{
    INJECT_NODE_INFO,
    INJECT_TABLE_INFO,
} InjectMessageType;

// Inject strategy API
typedef struct InjectContext{
    void (*injectNodeMetric)(void* metric);
} InjectContext;

extern Strategy strategyInject;

extern unsigned long lastMiddlewareUpdateTime;

void initMiddlewareInject(void (*setValueFunction)(void*,void*), void *metricStruct, size_t metricStructSize,void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *));

void encodeMessageInject(char* messageBuffer, size_t bufferSize, int type);
void encodeMessageStrategyInject(char* messageBuffer, size_t bufferSize, int type);
void handleMessageStrategyInject(char* messageBuffer, size_t bufferSize);
void onNetworkEventStrategyInject(int context, int contextIP[4]);
void influenceRoutingStrategyInject(char* dataMessage);
void onTimerStrategyInject();
void* getContextStrategyInject();

void injectNodeMetric(void* metric);

void encodeMyMetric(char* messageBuffer, size_t bufferSize);
void rewriteSenderIPInject(char* messageBuffer, size_t bufferSize, InjectMessageType type);

void encodeMetricEntry(char* buffer, size_t bufferSize, void *metricEntry);
void decodeMetricEntry(char* buffer, void *metricEntry);
void printMetricStruct(TableEntry* Table);
void setMetricValue(void* av, void*bv);
int compareMetrics(void *metricAv,void*metricBv);

void setIP(void* av, void* bv);

#endif //STRATEGY_INJECT_H
