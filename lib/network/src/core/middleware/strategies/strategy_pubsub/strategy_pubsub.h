#ifndef STRATEGY_PUBSUB_H
#define STRATEGY_PUBSUB_H


#include "../../../routing/routing.h"
#include "../../../routing/messages.h"
#include "../../../logger/logger.h"
#include "../../../time_hal/time_hal.h"
#include "../../../transport_hal/transport_hal.h"
#include "../strategy_interface.h"

#include <cstdlib>
#include <cstring>
#include <cstdint>

extern Strategy strategyPubSub;


typedef enum TopicTypes{
    TEMPERATURE,
    HUMIDITY,
    CAMERA,
}TopicTypes;

#define MAX_TOPICS 3 //Max topics that a node can publish

extern TableInfo* pubsubTable;

typedef struct PubSubInfo{
    int8_t publishedTopics[MAX_TOPICS]; //Topics that each node publishes
    int8_t subscribedTopics[MAX_TOPICS]; //Topics that the node subscribes
}PubSubInfo;

typedef enum PubSubMessageType{
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


void initStrategyPubSub(void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,int8_t *) );
bool encodeMessageStrategyPubSub(char* messageBuffer, size_t bufferSize, int typePubSub, int8_t topic);
void handleMessageStrategyPubSub(char* messageBuffer, size_t bufferSize);
void onNetworkEventStrategyPubSub(int networkEvent, uint8_t involvedIP[4]);
void influenceRoutingStrategyPubSub(char* messageEncodeBuffer,size_t encodeBufferSize,char* dataMessagePayload);
void onTimerStrategyPubSub();
void* getContextStrategyPubSub();

void rewriteSenderIPPubSub(char* messageBuffer, char* writeBuffer, size_t writeBufferSize, PubSubMessageType type);
bool containsTopic(const int8_t * list, int8_t topic);
void setPubSubInfo(void* av, void* bv);

void printPubSubStruct(TableEntry* Table);
void printPubSubTableHeader();
void decodeTopic(char* dataMessage, int8_t * topicType);
void encodeTopic(char*DataMessage,size_t messageSize, void* topic);

void subscribeToTopic(int8_t topic);
void unsubscribeToTopic(int8_t topic);
void advertiseTopic(int8_t topic);
void unadvertiseTopic(int8_t topic);


#endif //STRATEGY_PUBSUB_H
