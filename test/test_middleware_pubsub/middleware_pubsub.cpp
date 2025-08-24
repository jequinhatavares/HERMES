#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/network/src/core/middleware/strategies/strategy_pubsub/strategy_pubsub.h"
#include "../lib/network/src/core/table/table.h"

//pio test -e native -f "test_middleware_pubsub" -v


/*** ****************************** Tests ****************************** ***/

void test_init_middleware(){
    uint8_t IP[4]={1,1,1,1};
    initStrategyPubSub(decodeTopic);

    tablePrint(pubsubTable,printPubSubTableHeader, printPubSubStruct);

    tableClean(pubsubTable);
}

void test_add_remove_subscription(){
    int8_t topic = HUMIDITY, topic2 = CAMERA;
    PubSubInfo *myPubSubInfo;
    initStrategyPubSub(decodeTopic);
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

    initStrategyPubSub(decodeTopic);

    // Subscribe to topic
    advertiseTopic(topic1);
    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);

    TEST_ASSERT(myPubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic1));

    // Unsubscribe to topic
    unadvertiseTopic(topic1);
    TEST_ASSERT(!containsTopic(myPubSubInfo->publishedTopics,topic1));

    //Subscribe to two topics
    advertiseTopic(topic1);
    advertiseTopic(topic2);
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic1));
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic2));

    //Unsubscribe to one topic
    unadvertiseTopic(topic1);
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic2));
    TEST_ASSERT(!containsTopic(myPubSubInfo->publishedTopics,topic1));

    //Subscribe again to the same topic to check for duplicates
    advertiseTopic(topic2);
    TEST_ASSERT(containsTopic(myPubSubInfo->publishedTopics,topic2));

    tableClean(pubsubTable);

}


void test_encode_middleware_subscribe_message(){
    char correctEncodedMsg[50];

    int8_t subtopic = HUMIDITY;
    PubSubInfo *myPubSubInfo;

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 1.1.1.1 1.1.1.1 1",MIDDLEWARE_MESSAGE,PUBSUB_SUBSCRIBE);

    initStrategyPubSub(decodeTopic);

    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer) ,PUBSUB_SUBSCRIBE,subtopic);

    printf("Encoded Message:%s\n",smallSendBuffer);
    printf("Correct Message:%s\n",correctEncodedMsg);
    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);
    tableClean(pubsubTable);
}


void test_encode_middleware_advertise_message(){
    char correctEncodedMsg[50];

    int8_t pubtopic = CAMERA;
    PubSubInfo *myPubSubInfo;

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 1.1.1.1 1.1.1.1 2",MIDDLEWARE_MESSAGE,PUBSUB_ADVERTISE);

    initStrategyPubSub(decodeTopic);

    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer) ,PUBSUB_ADVERTISE,pubtopic);

    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);/******/

    tableClean(pubsubTable);
}

void test_encode_middleware_info_update_message(){
    char correctEncodedMsg[50] ;

    int8_t stopic = HUMIDITY, stopic2 = TEMPERATURE, ptopic = CAMERA;
    PubSubInfo *myPubSubInfo;

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 1.1.1.1 1.1.1.1|2 -1 -1 1 0 -1",MIDDLEWARE_MESSAGE,PUBSUB_NODE_UPDATE);

    initStrategyPubSub(decodeTopic);

    subscribeToTopic(stopic);
    subscribeToTopic(stopic2);
    advertiseTopic(ptopic);

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    TEST_ASSERT(myPubSubInfo != nullptr);

    //10 5 [sender IP] [node IP] | [Published Topic List] [Subscribed Topics List]
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer) ,PUBSUB_NODE_UPDATE,stopic);

    //printf("Encoded Message: %s\n",smallSendBuffer);
    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);/******/
    tableClean(pubsubTable);
}

void test_encode_middleware_table_update_message(){
    //13 5 [sender IP] |[node IP] [Published Topic List] [Subscribed Topics List] |[node IP] [Published Topic List] [Subscribed Topics List]...
    char correctEncodedMsg[100];
    char receivedMiddlewareMessage[50] = "11 0 3.3.3.3 2.2.2.2 2";
    int8_t stopic = HUMIDITY, stopic2 = TEMPERATURE, ptopic = CAMERA;
    PubSubInfo *myPubSubInfo;

    initStrategyPubSub(decodeTopic);

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 1.1.1.1 |1.1.1.1 2 -1 -1 1 0 -1|2.2.2.2 2 -1 -1 -1 2 -1 -1",MIDDLEWARE_MESSAGE,PUBSUB_TABLE_UPDATE);

    subscribeToTopic(stopic);
    subscribeToTopic(stopic2);
    advertiseTopic(ptopic);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    TEST_ASSERT(myPubSubInfo != nullptr);

    //10 5 [sender IP] [node IP] | [Published Topic List] [Subscribed Topics List]
    encodeMessageStrategyPubSub(largeSendBuffer, sizeof(largeSendBuffer) ,PUBSUB_TABLE_UPDATE,stopic);

    printf("Correct Message: %s\n",correctEncodedMsg);
    printf("Encoded Message: %s\n",largeSendBuffer);
    //TEST_ASSERT(strcmp(largeSendBuffer,correctEncodedMsg) == 0);/******/
    tableClean(pubsubTable);
}

void test_handle_multiple_subscribe_messages(){
    char receivedMiddlewareMessage[50];
    int8_t topic0=0,topic1=1,topic2=2;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={3,3,3,3};

    initStrategyPubSub(decodeTopic);

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage),"%d %d 3.3.3.3 3.3.3.3 0",MIDDLEWARE_MESSAGE,PUBSUB_SUBSCRIBE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage),"%d %d 3.3.3.3 3.3.3.3 1",MIDDLEWARE_MESSAGE,PUBSUB_ADVERTISE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage),"%d %d 3.3.3.3 3.3.3.3 1",MIDDLEWARE_MESSAGE,PUBSUB_SUBSCRIBE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage),"%d %d 3.3.3.3 3.3.3.3 2",MIDDLEWARE_MESSAGE,PUBSUB_ADVERTISE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));/******/

    tablePrint(pubsubTable,printPubSubTableHeader,printPubSubStruct);

    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(nodePubSubInfo->subscribedTopics,0));

    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(nodePubSubInfo->subscribedTopics,1));

    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(nodePubSubInfo->publishedTopics,2));

    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(nodePubSubInfo->publishedTopics,1));


    tableClean(pubsubTable);
}

void test_extensive_add_and_remove_pubsub_topics() {
    char receivedMiddlewareMessage[50];
    uint8_t nodeIP[4] = {5, 5, 5, 5};
    PubSubInfo *info;

    initStrategyPubSub(decodeTopic);

    // Subscribe to topics 0, 1, 2
    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage), "%d %d 3.3.3.3 5.5.5.5 0", MIDDLEWARE_MESSAGE, PUBSUB_SUBSCRIBE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage), "%d %d 3.3.3.3 5.5.5.5 1", MIDDLEWARE_MESSAGE, PUBSUB_SUBSCRIBE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage), "%d %d 3.3.3.3 5.5.5.5 2", MIDDLEWARE_MESSAGE, PUBSUB_SUBSCRIBE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    // Advertise topics 3, 4
    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage), "%d %d 3.3.3.3 5.5.5.5 3", MIDDLEWARE_MESSAGE, PUBSUB_ADVERTISE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage), "%d %d 3.3.3.3 5.5.5.5 4", MIDDLEWARE_MESSAGE, PUBSUB_ADVERTISE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    // Try adding duplicates (should not change)
    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage), "%d %d 3.3.3.3 5.5.5.5 1", MIDDLEWARE_MESSAGE, PUBSUB_SUBSCRIBE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));
    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage), "%d %d 3.3.3.3 5.5.5.5 3", MIDDLEWARE_MESSAGE, PUBSUB_ADVERTISE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    tablePrint(pubsubTable,printPubSubTableHeader,printPubSubStruct);

    // Check after adds
    info = (PubSubInfo*) tableRead(pubsubTable, nodeIP);
    TEST_ASSERT(info != nullptr);
    TEST_ASSERT(containsTopic(info->subscribedTopics, 0));
    TEST_ASSERT(containsTopic(info->subscribedTopics, 1));
    TEST_ASSERT(containsTopic(info->subscribedTopics, 2));
    TEST_ASSERT(containsTopic(info->publishedTopics, 3));
    TEST_ASSERT(containsTopic(info->publishedTopics, 4));

    // Remove subscription topic 1
    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage), "%d %d 3.3.3.3 5.5.5.5 1", MIDDLEWARE_MESSAGE, PUBSUB_UNSUBSCRIBE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    // Remove advertisement topic 4
    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage), "%d %d 3.3.3.3 5.5.5.5 4", MIDDLEWARE_MESSAGE, PUBSUB_UNADVERTISE);
    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    tablePrint(pubsubTable,printPubSubTableHeader,printPubSubStruct);

    // Check after removals
    info = (PubSubInfo*) tableRead(pubsubTable, nodeIP);
    TEST_ASSERT(info != nullptr);
    TEST_ASSERT(containsTopic(info->subscribedTopics, 0));
    TEST_ASSERT(!containsTopic(info->subscribedTopics, 1)); // removed
    TEST_ASSERT(containsTopic(info->subscribedTopics, 2));

    TEST_ASSERT(containsTopic(info->publishedTopics, 3));
    TEST_ASSERT(!containsTopic(info->publishedTopics, 4)); // removed/******/

    tableClean(pubsubTable);
}

void test_subscribe_and_publish_multiple_topics() {
    char messageBuffer[50];
    uint8_t nodeIP[4] = {1, 1, 1, 1};
    PubSubInfo *info;

    initStrategyPubSub(decodeTopic);

    // Lists of topics to subscribe and publish
    int8_t subscribeList[] = {0, 1, 2};
    int8_t publishList[]   = {3, 4};

    // Call the function to add all subscriptions and publications
    updateNodeTopics(subscribeList, 3, publishList, 2);

    //tablePrint(pubsubTable, printPubSubTableHeader, printPubSubStruct);

    // Read the table entry for this node
    info = (PubSubInfo*) tableRead(pubsubTable, nodeIP);
    TEST_ASSERT(info != nullptr);

    // Verify subscriptions
    TEST_ASSERT(containsTopic(info->subscribedTopics, 0));
    TEST_ASSERT(containsTopic(info->subscribedTopics, 1));
    TEST_ASSERT(containsTopic(info->subscribedTopics, 2));

    // Verify publications
    TEST_ASSERT(containsTopic(info->publishedTopics, 3));
    TEST_ASSERT(containsTopic(info->publishedTopics, 4));

    // Attempt to add duplicate topics
    updateNodeTopics(subscribeList, 3, publishList, 2);

    // Verify duplicates did not appear (still only the same topics)
    info = (PubSubInfo*) tableRead(pubsubTable, myIP);
    int subCount = 0, pubCount = 0;
    for (int i = 0; i < MAX_TOPICS; i++) {
        if(info->subscribedTopics[i] != -1) subCount++;
        if(info->publishedTopics[i] != -1) pubCount++;
    }
    TEST_ASSERT(subCount == 3);
    TEST_ASSERT(pubCount == 2);

}

void test_handle_middleware_subscribe_message(){
    char receivedMiddlewareMessage[50];
    int8_t topic = HUMIDITY, topic2 = TEMPERATURE, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage),"%d %d 3.3.3.3 2.2.2.2 2",MIDDLEWARE_MESSAGE,PUBSUB_SUBSCRIBE);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));
    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(nodePubSubInfo->subscribedTopics,topic3));

    tableClean(pubsubTable);

}

void test_handle_middleware_unsubscribe_message(){
    char receivedMiddlewareMessage2[50] = "10 2 3.3.3.3 2.2.2.2 2";
    int8_t topic = HUMIDITY, topic2 = TEMPERATURE, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    snprintf(receivedMiddlewareMessage2, sizeof(receivedMiddlewareMessage2),"%d %d 3.3.3.3 2.2.2.2 2",MIDDLEWARE_MESSAGE,PUBSUB_UNSUBSCRIBE);

    handleMessageStrategyPubSub(receivedMiddlewareMessage2, sizeof(receivedMiddlewareMessage2));
    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(!containsTopic(nodePubSubInfo->subscribedTopics,topic3));


    tableClean(pubsubTable);
}

void test_handle_middleware_advertise_message(){
    char receivedMiddlewareMessage[50];
    int8_t topic = HUMIDITY, topic2 = TEMPERATURE, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage),"%d %d 3.3.3.3 2.2.2.2 2",MIDDLEWARE_MESSAGE,PUBSUB_ADVERTISE);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
    TEST_ASSERT(nodePubSubInfo != nullptr);
    TEST_ASSERT(containsTopic(nodePubSubInfo->publishedTopics,topic3));

    tableClean(pubsubTable);
}

void test_handle_middleware_unadvertise_message(){
    char receivedMiddlewareMessage[50];
    char receivedMiddlewareMessage2[50];
    int8_t topic = HUMIDITY, topic2 = TEMPERATURE, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage),"%d %d 3.3.3.3 2.2.2.2 2",MIDDLEWARE_MESSAGE,PUBSUB_ADVERTISE);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    snprintf(receivedMiddlewareMessage2, sizeof(receivedMiddlewareMessage2),"%d %d 3.3.3.3 2.2.2.2 2",MIDDLEWARE_MESSAGE,PUBSUB_UNADVERTISE);

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

    initStrategyPubSub(decodeTopic);

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
    char receivedMiddlewareMessage[100];
    int8_t topic = TEMPERATURE, topic2 = HUMIDITY, topic3 = CAMERA;
    PubSubInfo *nodePubSubInfo;
    uint8_t nodeIP[4] ={2,2,2,2};

    initStrategyPubSub(decodeTopic);

    subscribeToTopic(topic);
    subscribeToTopic(topic2);
    advertiseTopic(topic3);

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage),"%d %d 1.1.1.1|1.1.1.1 2 -1 -1 1 0 -1 |2.2.2.2 -1 -1 -1 2 -1 -1",MIDDLEWARE_MESSAGE,PUBSUB_TABLE_UPDATE);

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
    char correctEncodedMsg[50];
    char receivedMiddlewareMessage[50];

    int8_t topic = HUMIDITY;
    PubSubInfo *myPubSubInfo;

    initStrategyPubSub(decodeTopic);

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 1.1.1.1 2.2.2.2 2",MIDDLEWARE_MESSAGE,PUBSUB_ADVERTISE);
    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage),"%d %d 3.3.3.3 2.2.2.2 2",MIDDLEWARE_MESSAGE,PUBSUB_ADVERTISE);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));
    //printf("Encoded Message: %s\n",receivedMiddlewareMessage);

    TEST_ASSERT(strcmp(smallSendBuffer,correctEncodedMsg) == 0);

    tableClean(pubsubTable);
}

void test_message_rewriteIP_info_update_message(){
    char correctEncodedMsg[50];
    char receivedMiddlewareMessage[50];

    int8_t topic = HUMIDITY;
    PubSubInfo *myPubSubInfo;

    initStrategyPubSub(decodeTopic);

    snprintf(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage),"%d %d 3.3.3.3 2.2.2.2 | 2 0 -1 -1 -1 -1",MIDDLEWARE_MESSAGE,PUBSUB_NODE_UPDATE);

    handleMessageStrategyPubSub(receivedMiddlewareMessage, sizeof(receivedMiddlewareMessage));

    snprintf(correctEncodedMsg, sizeof(correctEncodedMsg),"%d %d 1.1.1.1 2.2.2.2 | 2 0 -1 -1 -1 -1",MIDDLEWARE_MESSAGE,PUBSUB_NODE_UPDATE);

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
    enableModule(MIDDLEWARE);

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
    RUN_TEST(test_handle_multiple_subscribe_messages);
    RUN_TEST(test_extensive_add_and_remove_pubsub_topics);/******//******//******/
    RUN_TEST(test_subscribe_and_publish_multiple_topics);

    UNITY_END();
}
