#ifndef STRATEGY_PUBSUB_H
#define STRATEGY_PUBSUB_H

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "routing.h"
//#include "../../routing/messages.h"
#include "messages.h"
#include "logger.h"
#include "../../../time_hal/time_hal.h"
#include "../../../transport_hal/transport_hal.h"
#include "../strategy_interface.h"



#define MAX_TOPICS 3

extern TableInfo* pubsubTable;

typedef struct PubSubInfo{
    int8_t publishedTopics[MAX_TOPICS]; //Topics that each node publishes
    int8_t subscribedTopics[MAX_TOPICS]; //Topics that the node subscribes
}PubSubInfo;

typedef enum PubSubMessageType{
    PUBSUB_PUBLISH,
    PUBSUB_SUBSCRIBE,
    PUBSUB_UNSUBSCRIBE,
    PUBSUB_ADVERTISE,
    PUBSUB_UNADVERTISE,
    PUBSUB_NODE_UPDATE,
    PUBSUB_TABLE_UPDATE,
} PubSubMessageType;

// PubSub strategy API
typedef struct PubSubContext{
    void (*subscribeToTopic)(int8_t topic);
    void (*unsubscribeToTopic)(int8_t topic);
    void (*advertiseTopic)(int8_t topic);
    void (*unadvertiseTopic)(int8_t topic);
} PubSubContext;

extern int8_t topic;

extern Strategy strategyPubSub;

void initMiddlewarePubSub(void (*setValueFunction)(void*,void *),void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,void *) );
void encodeMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize, int typePubSub);
void handleMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize);
void middlewareOnNetworkEventPubSub(int context,int contextIP[4]);
void middlewareInfluenceRoutingPubSub(char* dataMessage);
void middlewareOnTimerPubSub();

void rewriteSenderIPPubSub(char* messageBuffer, size_t bufferSize, PubSubMessageType type);
bool containsTopic(int8_t * list, int8_t topic);

void printPubSubStruct(TableEntry* Table);
void* getContextPubSub();
void decodeTopic(char* dataMessage, void *topicType);
void encodeTopic(char*DataMessage,size_t messageSize, void* topic);
void setPubSubInfo(void* av, void* bv);

void subscribeToTopic(int8_t topic);
void unsubscribeToTopic(int8_t topic);
void advertiseTopic(int8_t topic);
void unadvertiseTopic(int8_t topic);


#endif //STRATEGY_PUBSUB_H
