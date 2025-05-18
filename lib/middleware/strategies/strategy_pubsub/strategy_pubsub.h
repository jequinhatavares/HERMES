#ifndef STRATEGY_PUBSUB_H
#define STRATEGY_PUBSUB_H

#include "routing.h"
//#include "../../routing/messages.h"
#include "messages.h"
#include "logger.h"
#include "../../../time_hal/time_hal.h"
#include "../../../transport_hal/transport_hal.h"

#define MAX_TOPICS 3

typedef struct PubSubInfo{
    int publishedTopics[MAX_TOPICS];
    int subscribedTopics[MAX_TOPICS];
}PubSubInfo;

#endif //STRATEGY_PUBSUB_H
