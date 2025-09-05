#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H


#include "strategies/strategy_inject/strategy_inject.h"
#include "strategies/strategy_pubsub/strategy_pubsub.h"
#include "strategies/strategy_topology/strategy_topology.h"
#include "strategies/strategy_interface.h"
#include "../lifecycle/lifecycle.h"



void initMiddlewareCallbacks();

void middlewareSelectStrategy(StrategyType strategyType);
void initMiddlewareStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*),void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *),void (*printMetricStruct)(TableEntry*));
void initMiddlewareStrategyPubSub(void (*decodeTopicFunction)(char*,int8_t *));
void initMiddlewareStrategyTopology(void *topologyMetricValues, size_t topologyMetricStructSize,void (*setValueFunction)(void*,void*),void (*encodeTopologyMetricFunction)(char*,size_t,void *),void (*decodeTopologyMetricFunction)(char*,void *),void (*printMetricFunction)(TableEntry*),uint8_t * (*selectParentFunction)(uint8_t *, uint8_t (*)[4], uint8_t));
//void middlewareInfluenceRouting(char* messageEncodeBuffer,size_t encodeBufferSize,char* dataMessagePayload);

void middlewareHandleMessage(char* messageBuffer, size_t bufferSize);
void middlewareOnTimer();
void middlewareOnNetworkEvent(int networkEvent, uint8_t involvedIP[4]);
void* middlewareGetStrategyContext();
bool isMiddlewareStrategyActive(StrategyType strategyType);
StrategyType middlewareActiveStrategy();
void middlewarePrintInfo(StrategyType strategyType);



#endif //MIDDLEWARE_H

