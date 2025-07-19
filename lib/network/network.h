#pragma once

#include <wifi_hal.h>
#include <transport_hal.h>
#include "lifecycle.h"
#include "cli.h"
#include "logger.h"
#include "../lib/middleware/strategies/strategy_inject/strategy_inject.h"
#include "../lib/middleware/strategies/strategy_pubsub/strategy_pubsub.h"
#include "middleware.h"
#include "../lib/circular_buffer/snake_queue.h"

class network {

    public:

        // Called in setup before begin()
        void configure(bool isRoot);

        // Called in setup()
        void begin();

        // Called in loop()
        void run();

        /*************  Middleware related methods  ************/
        void middlewareSelectStrategy(StrategyType strategyType);

        void initMiddlewareStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*),void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *));
        void initMiddlewareStrategyPubSub(void (*setValueFunction)(void*,void *),void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,int8_t *));
        void initMiddlewareStrategyTopology(void *topologyMetricValues, size_t topologyMetricStructSize,void (*setValueFunction)(void*,void*),void (*encodeTopologyMetricFunction)(char*,size_t,void *),void (*decodeTopologyMetricFunction)(char*,void *), uint8_t * (*selectParentFunction)(uint8_t *, uint8_t (*)[4], uint8_t));
        void middlewareInfluenceRouting(char* dataMessage);
        void* middlewareGetStrategyContext();


        /*************  Message related methods  ************/
        void sendMessageToRoot(const char* messagePayload);
        void sendMessageToParent(const char* messagePayload);
        void sendMessageToChildren(const char* messagePayload);
        void sendMessageToNode(const char* messagePayload, uint8_t* nodeIP);
        void broadcastMessage(const char* messagePayload);
        void sendACKMessage(const char* messagePayload, uint8_t* nodeIP);





};



