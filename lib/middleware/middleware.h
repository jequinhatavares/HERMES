#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

//#include <strategies/strategy_interface.h>
#include "strategies/strategy_inject/strategy_inject.h"
#include "strategies/strategy_pubsub/strategy_pubsub.h"
#include <logger.h>

typedef enum Context{
    CONTEXT_JOINED_NETWORK,
    CONTEXT_CHILD_CONNECTED,
    CONTEXT_CHILD_DISCONNECTED,
}Context;

typedef enum StrategyType{
    STRATEGY_INJECT,
    STRATEGY_PUBSUB,
    STRATEGY_NONE,
}StrategyType;


void middlewareSelectStrategy(StrategyType strategyType);
void initStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*),void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *));
void initStrategyPubSub(void (*setValueFunction)(void*,void *),void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,void *));
void middlewareInfluenceRouting(char* dataMessage);
void middlewareHandleMessage(char* messageBuffer, size_t bufferSize);
void middlewareEncodeMessage(char* messageBuffer, size_t bufferSize, int type);

#endif //MIDDLEWARE_H

