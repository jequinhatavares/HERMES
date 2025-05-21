#ifndef STRATEGY_PUBSUB_H
#define STRATEGY_PUBSUB_H

#include <stdlib.h>
#include "routing.h"
//#include "../../routing/messages.h"
#include "messages.h"
#include "logger.h"
#include "../../../time_hal/time_hal.h"
#include "../../../transport_hal/transport_hal.h"

#define MAX_TOPICS 3

extern TableInfo* pubsubTable;

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
    PUBSUB_INFO_UPDATE,
} PubSubMessageType;

void initMiddlewarePubSub(void (*setValueFunction)(void*,void *),void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,void *) );
void encodeMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize, PubSubMessageType typePubSub, int topic);
void handleMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize);
void middlewareInfluenceRoutingPubSub(char* dataMessage);
void middlewareOnTimerPubSub();

void rewriteSenderIP(char* messageBuffer, size_t bufferSize, PubSubMessageType type);
bool containsTopic(const int* list, int topic);

void printPubSubStruct(TableEntry* Table);
void decodeTopic(char* dataMessage, void *topicType);
void encodeTopic(char*DataMessage,size_t messageSize, void* topic);
void setPubSubInfo(void* av, void* bv);

void subscribeToTopic(int topic);
void unsubscribeToTopic(int topic);
void advertiseTopic(int topic);
void unadvertiseTopic(int topic);


#endif //STRATEGY_PUBSUB_H
