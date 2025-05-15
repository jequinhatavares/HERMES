#ifndef STRATEGY_INJECT_H
#define STRATEGY_INJECT_H

#include "table.h"
#include "routing.h"

extern TableInfo* metricTable;


void initMetricTable(void (*setValueFunction)(void*,void*), void *metricStruct, size_t metricStructSize,void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *));
void updateMiddlewareMetric(void* metricStruct, void * nodeIP);
void encodeMiddlewareMessage(char* messageBuffer, size_t bufferSize);
void handleMiddlewareMessage(char* messageBuffer);

void encodeMetricEntry(char* buffer, size_t bufferSize, void *metricEntry);
void decodeMetricEntry(char* buffer, void *metricEntry);

void setIP(void* av, void* bv);

#endif //STRATEGY_INJECT_H
