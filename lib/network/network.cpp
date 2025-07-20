#include "network.h"


void network::setAsRoot(bool isRoot) {
    iamRoot = isRoot;
}

void network::begin() {

    uint8_t MAC[6];
    //Serial.begin(115200);

    #if defined(raspberrypi_3b)
        initTime();
    #endif

    //To auto initialize the root node has the node with the IP 135.230.96.1
    getMyMAC(MAC);

    LOG(NETWORK,INFO,"Code uploaded through multi_upload_tool.py V1\n");
    LOG(NETWORK,INFO,"My MAC addr: %i.%i.%i.%i.%i.%i\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);

    Advance(SM, eSuccess);//Init

    if(!iamRoot){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//State Search APs
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//State Join Network
    }

}


void network::run() {
    int packetSize;

    #if defined(raspberrypi_3b)
        // Raspberry Pi Wi-Fi events are manually implemented and require polling in the main loop
        waitForWifiEvent();
    #endif

    //Wait for incoming requests
    packetSize = receiveMessage(receiveBuffer, sizeof(receiveBuffer));
    if (packetSize > 0){
        insertLast(stateMachineEngine, eMessage);
        if(packetSize >= 255){
            LOG(MESSAGES, ERROR,"Receiving buffer is too small packet has size:%i\n", packetSize);
        }

    }

    // Handle all timers: routing periodic updates, child disconnection timeouts, middleware timer callbacks and any application-level periodic tasks
    handleTimers();

    while(stateMachineEngine->size != 0){
        printSnake((CircularBuffer *)stateMachineEngine);
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
    }

    #if defined(ESP32) || defined(ESP8266)
        // Command Line Interface not yet implemented for Raspberry Pi
        cliInteraction();
    #endif
}

void network::middlewareSelectStrategy(StrategyType strategyType){
    ::middlewareSelectStrategy(strategyType);
}


void network::middlewareInfluenceRouting(char *dataMessage) {
    ::middlewareInfluenceRouting(dataMessage);
}


void network::initMiddlewareStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*),void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *)){
    ::initMiddlewareStrategyInject(metricStruct, metricStructSize,setValueFunction,encodeMetricFunction,decodeMetricFunction);
}

void network::injectMetric(void *metric) {
    if (!::isMiddlewareStrategyActive(STRATEGY_INJECT)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method that is not permitted by the active middleware strategy. \n");
        return;
    }

    InjectContext* context = (InjectContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->injectNodeMetric(metric);
}


void network::initMiddlewareStrategyPubSub(void (*setValueFunction)(void*,void *),void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,int8_t *)){
    ::initMiddlewareStrategyPubSub(setValueFunction,encodeTopicFunction,decodeTopicFunction);
}

void network::subscribeToTopic(int8_t topic) {
    if (!::isMiddlewareStrategyActive( STRATEGY_PUBSUB)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method that is not permitted by the active middleware strategy. \n");
        return;
    }

    PubSubContext* context = (PubSubContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->subscribeToTopic(topic);
}

void network::unsubscribeToTopic(int8_t topic){
    if (!::isMiddlewareStrategyActive( STRATEGY_PUBSUB)) return;{
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method that is not permitted by the active middleware strategy. \n");
        return;
    }

    PubSubContext* context = (PubSubContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->unsubscribeToTopic(topic);
}

void network::advertiseTopic(int8_t topic){
    if (!::isMiddlewareStrategyActive( STRATEGY_PUBSUB)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method that is not permitted by the active middleware strategy. \n");
        return;
    }

    PubSubContext* context = (PubSubContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->advertiseTopic(topic);
}

void network::unadvertiseTopic(int8_t topic){
    if (!::isMiddlewareStrategyActive( STRATEGY_PUBSUB)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method that is not permitted by the active middleware strategy. \n");
        return;
    }

    PubSubContext* context = (PubSubContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->unadvertiseTopic(topic);
}

void network::initMiddlewareStrategyTopology(void *topologyMetricValues, size_t topologyMetricStructSize,void (*setValueFunction)(void*,void*),void (*encodeTopologyMetricFunction)(char*,size_t,void *),void (*decodeTopologyMetricFunction)(char*,void *), uint8_t * (*selectParentFunction)(uint8_t *, uint8_t (*)[4], uint8_t)){
    ::initMiddlewareStrategyTopology(topologyMetricValues, topologyMetricStructSize,setValueFunction,encodeTopologyMetricFunction,decodeTopologyMetricFunction, selectParentFunction);
}
void network::setMetric(void *metric) {
    if (!::isMiddlewareStrategyActive( STRATEGY_TOPOLOGY)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method that is not permitted by the active middleware strategy. \n");
        return;
    }

    TopologyContext* context = (TopologyContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->setMetric(metric);
}



void network::sendMessageToRoot(const char *messagePayload) {
    sendDataMessageToNode(messagePayload,rootIP);
}

void network::sendMessageToParent(const char *messagePayload) {
    sendDataMessageToParent(messagePayload);
}

void network::sendMessageToChildren(const char *messagePayload) {
    sendDataMessageToChildren(messagePayload);
}

void network::sendMessageToNode(const char *messagePayload,uint8_t *nodeIP) {
    sendDataMessageToNode(messagePayload,nodeIP);
}


void network::broadcastMessage(const char *messagePayload) {
    uint8_t broadcastIP[4]={255,255,255,255};
    sendDataMessageToNode(messagePayload,broadcastIP);
}

void network::sendACKMessage(const char *ackPayload, uint8_t *destinationIP) {
    sendACKMessageToNode(ackPayload,destinationIP);
}

void network::onDataReceived(void (*callback)(uint8_t *, uint8_t *, char *)) {
    onDataMessageCallback = callback;
}

void network::onACKReceived(void (*callback)(uint8_t *, uint8_t *, char *)) {
    onACKMessageCallback = callback;
}

void network::onPeriodicAppTask(void (*callback)()) {
    onAppPeriodicTaskCallback = callback;
}


