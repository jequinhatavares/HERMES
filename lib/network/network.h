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

        void sendMessageToRoot(const char* messagePayload);

        void sendMessageToParent(const char* messagePayload);

        void sendMessageToChildren(const char* messagePayload);

        void sendMessageToNode(const char* messagePayload,const uint8_t* nodeIP);

        void broadcastMessage(const char* messagePayload);

        void sendACKMessage(const char* messagePayload,const uint8_t* nodeIP);

};



