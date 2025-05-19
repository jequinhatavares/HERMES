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
    int publishedTopics[MAX_TOPICS]; //Topics that each node publishes
    int subscribedTopics[MAX_TOPICS]; //Topics that the node subscribes
}PubSubInfo;

typedef enum PubSubMessageType{
    PUBSUB_PUBLISH,
    PUBSUB_SUBSCRIBE,
    PUBSUB_UNSUBSCRIBE,
    PUBSUB_ADVERTISE,
    PUBSUB_UNADVERTISE,
} PubSubMessageType;

void encodeMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize);
void handleMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize);
void middlewareInfluenceRoutingPubSub(char* dataMessage);


void decodeTopic(char* dataMessage, int* topicType);



#endif //STRATEGY_PUBSUB_H
