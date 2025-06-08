#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

//#include <strategies/strategy_interface.h>
#include "strategies/strategy_inject/strategy_inject.h"
#include "strategies/strategy_pubsub/strategy_pubsub.h"
#include "strategies/strategy_topology/strategy_topology.h"
#include <logger.h>
#include <lifecycle.h>


typedef enum StrategyType{
    STRATEGY_INJECT,
    STRATEGY_PUBSUB,
    STRATEGY_TOPOLOGY,
    STRATEGY_NONE,
}StrategyType;

extern StrategyType activeStrategyType;



void middlewareSelectStrategy(StrategyType strategyType);
void initMiddlewareStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*),void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *));
void initMiddlewareStrategyPubSub(void (*setValueFunction)(void*,void *),void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,int8_t *));
void initMiddlewareStrategyTopology();
void middlewareInfluenceRouting(char* dataMessage);
void middlewareHandleMessage(char* messageBuffer, size_t bufferSize);
void middlewareEncodeMessage(char* messageBuffer, size_t bufferSize, int type);
void middlewareOnTimer();
void middlewareOnNetworkEvent(int networkEvent, int involvedIP[4]);
void* middlewareGetStrategyContext();


#endif //MIDDLEWARE_H

