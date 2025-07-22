#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_pubsub/strategy_pubsub.h"
#include "table.h"

//pio test -e native -f "test_middleware_pubsub" -v


/*** ****************************** Tests ****************************** ***/

void test_init_middleware(){
    uint8_t IP[4]={1,1,1,1};
    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    tablePrint(pubsubTable,printPubSubTableHeader, printPubSubStruct);

    tableClean(pubsubTable);
}

void test_add_remove_subscription(){
    int8_t topic = HUMIDITY, topic2 = CAMERA;
    PubSubInfo *myPubSubInfo;
    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);
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
    int8_t topic1 = HUMIDITY, topic2 = CAMERA;
    PubSubInfo *myPubSubInfo;

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    // Subscribe to topic
    advertiseTopic(topic1);
    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);

    TEST_ASSERT(myPubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic1));

    // Unsubscribe to topic
    unadvertiseTopic(topic);
    TEST_ASSERT(!containsTopic(myPubSubInfo->publishedTopics,topic1));

    //Subscribe to two topics
    advertiseTopic(topic);
    advertiseTopic(topic2);
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic1));
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


void test_encode_middleware_subscribe_message(){
    char correctEncodedMsg[50] = "10 1 1.1.1.1 1.1.1.1 1";

    int8_t subtopic = HUMIDITY;
    PubSubInfo *myPubSubInfo;

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    topic = subtopic;
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer) ,PUBSUB_SUBSCRIBE);

    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);

    tableClean(pubsubTable);
}


void test_encode_middleware_advertise_message(){
    char correctEncodedMsg[50] = "10 3 1.1.1.1 1.1.1.1 2";

    int8_t pubtopic = CAMERA;
    PubSubInfo *myPubSubInfo;

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    topic = pubtopic;
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer) ,PUBSUB_ADVERTISE);

    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);/******/

    tableClean(pubsubTable);
}

void test_encode_middleware_info_update_message(){
    char correctEncodedMsg[50] = "10 5 1.1.1.1 1.1.1.1 | 2 -1 -1 1 0 -1 ";

    int8_t stopic = HUMIDITY, stopic2 = TEMPERATURE, ptopic = CAMERA;
    PubSubInfo *myPubSubInfo;

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    subscribeToTopic(stopic);
    subscribeToTopic(stopic2);
    advertiseTopic(ptopic);

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    TEST_ASSERT(myPubSubInfo != nullptr);

    //10 5 [sender IP] [node IP] | [Published Topic List] [Subscribed Topics List]
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer) ,PUBSUB_NODE_UPDATE);

    //printf("Encoded Message: %s\n",smallSendBuffer);
    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);/******/
    tableClean(pubsubTable);
}

void test_encode_middleware_table_update_message(){
    //10 6 [sender IP] |[node IP] [Published Topic List] [Subscribed Topics List] |[node IP] [Published Topic List] [Subscribed Topics List]...
    char correctEncodedMsg[100] = "10 6 1.1.1.1 |1.1.1.1 2 -1 -1 1 0 -1 |2.2.2.2 -1 -1 -1 2 -1 -1";
    char receivedMiddlewareMessage[50] = "10 1 3.3.3.3 2.2.2.2 2";
    int8_t stopic = HUMIDITY, stopic2 = TEMPERATURE, ptopic = CAMERA;
    PubSubInfo *myPubSubInfo;

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    subscribeToTopic(stopic);
    subscribeToTopic(stopic2);
    advertiseTopic(ptopic);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    TEST_ASSERT(myPubSubInfo != nullptr);

    //10 5 [sender IP] [node IP] | [Published Topic List] [Subscribed Topics List]
    encodeMessageStrategyPubSub(largeSendBuffer, sizeof(largeSendBuffer) ,PUBSUB_TABLE_UPDATE);

    printf("Encoded Message: %s\n",correctEncodedMsg);
    printf("Encoded Message: %s\n",largeSendBuffer);
    TEST_ASSERT(strcmp(largeSendBuffer,correctEncodedMsg) == 0);/******/
    tableClean(pubsubTable);
}

void test_handle_middleware_subscribe_message(){
    char correctEncodedMsg[50] = "10 5 1.1.1.1 1.1.1.1 | 2 -1 -1 1 0 -1 ";
    char receivedMiddlewareMessage[50] = "10 1 3.3.3.3 2.2.2.2 2";
    int8_t topic = HUMIDITY, topic2 = TEMPERATURE, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));
    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(nodePubSubInfo->subscribedTopics,topic3));


    tableClean(pubsubTable);

}

void test_handle_middleware_unsubscribe_message(){
    char correctEncodedMsg[50] = "10 5 1.1.1.1 1.1.1.1 | 2 -1 -1 1 0 -1 ";
    char receivedMiddlewareMessage[50] = "10 1 3.3.3.3 2.2.2.2 2";
    char receivedMiddlewareMessage2[50] = "10 2 3.3.3.3 2.2.2.2 2";
    int8_t topic = HUMIDITY, topic2 = TEMPERATURE, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    handleMessageStrategyPubSub(receivedMiddlewareMessage2, sizeof(receivedMiddlewareMessage));
    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(!containsTopic(nodePubSubInfo->subscribedTopics,topic3));


    tableClean(pubsubTable);
}

void test_handle_middleware_advertise_message(){
    char correctEncodedMsg[50] = "10 5 1.1.1.1 1.1.1.1 | 2 -1 -1 1 0 -1 ";
    char receivedMiddlewareMessage[50] = "10 3 3.3.3.3 2.2.2.2 2";
    char receivedMiddlewareMessage2[50] = "10 2 3.3.3.3 2.2.2.2 2";
    int8_t topic = HUMIDITY, topic2 = TEMPERATURE, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(nodePubSubInfo->publishedTopics,topic3));

    tableClean(pubsubTable);
}

void test_handle_middleware_unadvertise_message(){
    char correctEncodedMsg[50] = "10 5 1.1.1.1 1.1.1.1 | 2 -1 -1 1 0 -1 ";
    char receivedMiddlewareMessage[50] = "10 3 3.3.3.3 2.2.2.2 2";
    char receivedMiddlewareMessage2[50] = "10 4 3.3.3.3 2.2.2.2 2";
    int8_t topic = HUMIDITY, topic2 = TEMPERATURE, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    handleMessageStrategyPubSub(receivedMiddlewareMessage2, sizeof(receivedMiddlewareMessage));


    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(!containsTopic(nodePubSubInfo->publishedTopics,topic3));

    tableClean(pubsubTable);
}

void test_handle_middleware_info_update_message(){
    //10 5 [sender IP] [node IP] | [Published Topic List] [Subscribed Topics List]
    char receivedMiddlewareMessage[50] = "10 5 3.3.3.3 2.2.2.2 | 2 0 -1 -1 -1 -1";
    char receivedMiddlewareMessage2[50] = "10 4 3.3.3.3 2.2.2.2 2";
    int8_t topic = TEMPERATURE, topic2 = HUMIDITY, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));


    //tablePrint(pubsubTable,printPubSubStruct);

    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(nodePubSubInfo->publishedTopics,topic));
    TEST_ASSERT(!containsTopic(nodePubSubInfo->publishedTopics,topic2));
    TEST_ASSERT(containsTopic(nodePubSubInfo->publishedTopics,topic3));
    TEST_ASSERT(!containsTopic(nodePubSubInfo->subscribedTopics,topic));
    TEST_ASSERT(!containsTopic(nodePubSubInfo->subscribedTopics,topic2));
    TEST_ASSERT(!containsTopic(nodePubSubInfo->subscribedTopics,topic3));/******/

    tableClean(pubsubTable);
}
void test_handle_middleware_table_update_message(){
    //10 5 [sender IP] [node IP] | [Published Topic List] [Subscribed Topics List]
    char receivedMiddlewareMessage[100] = "10 6 1.1.1.1 |1.1.1.1 2 -1 -1 1 0 -1 |2.2.2.2 -1 -1 -1 2 -1 -1";
    int8_t topic = TEMPERATURE, topic2 = HUMIDITY, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));


    tablePrint(pubsubTable,printPubSubTableHeader,printPubSubStruct);

    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(!containsTopic(nodePubSubInfo->publishedTopics,topic));
    TEST_ASSERT(!containsTopic(nodePubSubInfo->publishedTopics,topic2));
    TEST_ASSERT(!containsTopic(nodePubSubInfo->publishedTopics,topic3));
    TEST_ASSERT(!containsTopic(nodePubSubInfo->subscribedTopics,topic));
    TEST_ASSERT(!containsTopic(nodePubSubInfo->subscribedTopics,topic2));
    TEST_ASSERT(containsTopic(nodePubSubInfo->subscribedTopics,topic3));/******/

    tableClean(pubsubTable);
}


void test_message_rewriteIP(){
    char correctEncodedMsg[50] = "10 3 1.1.1.1 2.2.2.2 2";
    char receivedMiddlewareMessage[50] = "10 3 3.3.3.3 2.2.2.2 2";

    int8_t topic = HUMIDITY;
    PubSubInfo *myPubSubInfo;

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    //printf("Encoded Message: %s\n",receivedMiddlewareMessage);

    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);

    tableClean(pubsubTable);
}

void test_message_rewriteIP_info_update_message(){
    char correctEncodedMsg[50] = "10 5 1.1.1.1 2.2.2.2 | 2 0 -1 -1 -1 -1";
    char receivedMiddlewareMessage[50] = "10 5 3.3.3.3 2.2.2.2 | 2 0 -1 -1 -1 -1";

    int8_t topic = HUMIDITY;
    PubSubInfo *myPubSubInfo;

    initStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    //printf("Encoded Message: %s\n",receivedMiddlewareMessage);

    TEST_ASSERT(strcmp(largeSendBuffer,correctEncodedMsg) == 0);

    tableClean(pubsubTable);
}

void test_decode_topic(){
    char topicMsg[50] = "TEMPERATURE";
    int8_t stopic = 0;

    decodeTopic(topicMsg,&stopic);
    printf("Topic: %i\n",stopic);
    TEST_ASSERT(stopic == 0);
}


void setUp(void){
    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(MONITORING_SERVER);
    enableModule(CLI);

    lastModule = NETWORK;
    currentLogLevel = DEBUG;

    uint8_t IP[4]={1,1,1,1};
    assignIP(myIP,IP);
}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_init_middleware);
    RUN_TEST(test_add_remove_subscription);
    RUN_TEST(test_publishing_topic);
    RUN_TEST(test_encode_middleware_subscribe_message);
    RUN_TEST(test_encode_middleware_advertise_message);
    RUN_TEST(test_encode_middleware_info_update_message);
    RUN_TEST(test_encode_middleware_table_update_message);
    RUN_TEST(test_handle_middleware_subscribe_message);
    RUN_TEST(test_handle_middleware_unsubscribe_message);
    RUN_TEST(test_handle_middleware_advertise_message);
    RUN_TEST(test_handle_middleware_unadvertise_message);
    RUN_TEST(test_handle_middleware_table_update_message);
    RUN_TEST(test_message_rewriteIP);
    RUN_TEST(test_message_rewriteIP_info_update_message);
    RUN_TEST(test_decode_topic);
    UNITY_END();
}
