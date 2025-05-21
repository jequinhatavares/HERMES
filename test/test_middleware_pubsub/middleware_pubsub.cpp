#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_pubsub/strategy_pubsub.h"
#include "table.h"

//pio test -e native -f "test_middleware_pubsub" -v

typedef enum topicTypes{
    TEMPERATURE,
    HUMIDITY,
    CAMERA,
}topicTypes;

/*** ****************************** Tests ****************************** ***/

void test_init_middleware(){
    int IP[4]={1,1,1,1};
    initMiddlewarePubSub(setPubSubInfo,encodeTopic,decodeTopic);

    tablePrint(pubsubTable, printPubSubStruct);

    tableClean(pubsubTable);

}

void test_add_remove_subscription(){
    topicTypes topic = HUMIDITY, topic2 = CAMERA;
    PubSubInfo *myPubSubInfo;
    initMiddlewarePubSub(setPubSubInfo,encodeTopic,decodeTopic);
    // Subscribe to topic
    subscribeToTopic(topic);

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    TEST_ASSERT(myPubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(myPubSubInfo->subscribedTopics,topic));

    // Unsubscribe to topic
    unsubscribeToTopic(topic);

    TEST_ASSERT(!containsTopic(myPubSubInfo->subscribedTopics,topic));

    //Subscribe to two topics
    subscribeToTopic(topic);
    subscribeToTopic(topic2);

    TEST_ASSERT(containsTopic(myPubSubInfo->subscribedTopics,topic));
    TEST_ASSERT(containsTopic(myPubSubInfo->subscribedTopics,topic2));

    //Unsubscribe to one topic
    unsubscribeToTopic(topic);

    TEST_ASSERT(!containsTopic(myPubSubInfo->subscribedTopics,topic));

    //Subscribe again to the same topic to check for duplicates
    subscribeToTopic(topic2);
    TEST_ASSERT(containsTopic(myPubSubInfo->subscribedTopics,topic2));

    tableClean(pubsubTable);
}

void test_publishing_topic(){
    topicTypes topic = HUMIDITY, topic2 = CAMERA;
    PubSubInfo *myPubSubInfo;

    initMiddlewarePubSub(setPubSubInfo,encodeTopic,decodeTopic);

    // Subscribe to topic
    advertiseTopic(topic);
    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);

    TEST_ASSERT(myPubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic));

    // Unsubscribe to topic
    unadvertiseTopic(topic);
    TEST_ASSERT(!containsTopic(myPubSubInfo->publishedTopics,topic));

    //Subscribe to two topics
    advertiseTopic(topic);
    advertiseTopic(topic2);
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic));
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic2));

    //Unsubscribe to one topic
    unadvertiseTopic(topic);
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic2));
    TEST_ASSERT(!containsTopic(myPubSubInfo->publishedTopics,topic));

    //Subscribe again to the same topic to check for duplicates
    advertiseTopic(topic2);
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic2));


    tableClean(pubsubTable);

}


void setUp(void){
    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(DEBUG_SERVER);
    enableModule(CLI);

    lastModule = NETWORK;
    currentLogLevel = DEBUG;

    int IP[4]={1,1,1,1};
    assignIP(myIP,IP);
}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_init_middleware);
    RUN_TEST(test_add_remove_subscription);
    RUN_TEST(test_publishing_topic);

    UNITY_END();
}
