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
    PUBSUB_SUBSCRIBE,               // 0 Request to subscribe to a topic
    PUBSUB_UNSUBSCRIBE,             // 1 Request to unsubscribe from a topic
    PUBSUB_ADVERTISE,               // 2 Announce a topic is available for publication
    PUBSUB_UNADVERTISE,             // 3 Withdraw announcement of a topic
    PUBSUB_NODE_TOPICS_UPDATE,     // 4 Update containing a single node’s metrics
    PUBSUB_NETWORK_TOPICS_UPDATE,  // 5 Update containing metrics of multiple nodes
} PubSubMessageType;

// PubSub strategy API
typedef struct PubSubContext{
    void (*influenceRouting)(char* messageEncodeBuffer,size_t encodeBufferSize,char* dataMessagePayload);
    void (*subscribeToTopic)(int8_t topic);
    void (*unsubscribeToTopic)(int8_t topic);
    void (*advertiseTopic)(int8_t topic);
    void (*unadvertiseTopic)(int8_t topic);
    void (*subscribeAndPublishTopics)(int8_t *subscribeList, int subCount, int8_t *publishList, int pubCount);
} PubSubContext;


void initStrategyPubSub(void (*decodeTopicFunction)(char*,int8_t *));
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
void updateNodeTopics(int8_t *subTopics, int numSubTopics, int8_t *pubTopics, int numPubTopics);


#endif //STRATEGY_PUBSUB_H
