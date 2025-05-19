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

void encodeMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize, PubSubMessageType typePubSub, int topic);
void handleMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize);
void middlewareInfluenceRoutingPubSub(char* dataMessage);

void rewriteSenderIP(char* messageBuffer, size_t bufferSize);

void printPubSubStruct(TableEntry* Table);
void decodeTopic(char* dataMessage, int* topicType);

void subscribeToTopic(char topic);
void unsubscribeToTopic(char topic);
void advertiseTopic(char topic);
void unadvertiseTopic(char topic);

#endif //STRATEGY_PUBSUB_H
