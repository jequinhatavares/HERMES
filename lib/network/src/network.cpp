#include "network.h"

#include "core/wifi_hal/wifi_hal.h" //nao faz sentido incluir por agora
#include "core/transport_hal/transport_hal.h" //nao faz sentido incluir
#include "core/lifecycle/lifecycle.h"
#include "core/cli/cli.h"
#include "core/logger/logger.h"
#include "core/middleware/middleware.h"

//network instance declaration
Network network;

/**
 * setAsRoot
 *
 * Sets whether this node should act as the root of the network.
 * Must be called before `begin()`. Only one root is allowed per network.
 *
 * @param isRoot - True if the node should be the root, false otherwise.
 * @return void
 */
void Network::setAsRoot(bool isRoot){
    ::iamRoot = isRoot;
    this->iamRoot = isRoot;
}


/**
 * begin
 * Initializes the node and integrates it into the multi-hop network.
 * Must be called once in the setup phase.
 *
 * @return void
 */
void Network::begin() {

    uint8_t MAC[6];

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


    //startWifiAP(ssid,PASS, localIP, gateway, subnet);
    //changeWifiMode(3);
    //LOG(NETWORK,INFO,"My SoftAP IP: %s\nMy STA IP %s\nGateway IP: %s\n", getMyAPIP().toString().c_str(), getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
}


/**
 * run
 * Main execution loop of the network layer. Handles Wi-Fi events, receives incoming messages,
 * manages timers, and drives the state machine to maintain and operate the Network.
 * Should be called continuously in the main loop to ensure proper Network operation.
 *
 * @return void
 */
void Network::run() {
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
        //printSnake((CircularBuffer *)stateMachineEngine);
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
    }

    #if defined(ESP32) || defined(ESP8266)
        // Command Line Interface not yet implemented for Raspberry Pi
        cliInteraction();
    #endif
}
/**
 * getMiddlewareActiveStrategy
 * Returns the active middleware strategy
 *
 * @return strategyType - The active strategy
 */
StrategyType Network::getActiveMiddlewareStrategy() {
    return middlewareActiveStrategy();
}

/**
 * middlewareSelectStrategy
 * Selects and activates a middleware strategy to influence routing behavior.
 *
 * @param strategyType - The strategy to be activated (e.g., STRATEGY_INJECT, STRATEGY_PUBSUB, STRATEGY_TOPOLOGY).
 *                       STRATEGY_NONE disables any active strategy.
 * @return void
 */
void Network::middlewareSelectStrategy(StrategyType strategyType){
    ::middlewareSelectStrategy(strategyType);
}


/**
 * middlewareInfluenceRouting
 * Invokes the active middleware strategy to influence routing decisions
 *
 * @param dataMessage - Pointer to the message data for which the middleware can influence the next routing direction
 * @return void
 */

void Network::middlewarePrintInfo() {
    ::middlewarePrintInfo(::middlewareActiveStrategy());
}



/**
 * initMiddlewareStrategyInject
 * Initializes the middleware strategy Inject by setting up function pointers and metric structures.
 * This should only be called after selecting the STRATEGY_INJECT strategy using middlewareSelectStrategy.
 *
 * @param metricStruct - Pointer to the memory where the metricStruct table values are allocated.
 * @param metricStructSize - Size of the metric structure in bytes.
 * @param setValueFunction - Pointer to the function responsible for updating values within the metric table.
 * @param encodeMetricFunction - Pointer to the function responsible for encoding the metric structure value into a buffer.
 * @param decodeMetricFunction - Pointer to the function responsible for decoding the buffer into a metric structure value.
 *
 * @return void
 */
void Network::initMiddlewareStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*),void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *)){
    ::initMiddlewareStrategyInject(metricStruct, metricStructSize,setValueFunction,encodeMetricFunction,decodeMetricFunction);
}

//todo correct header
/**
 * influenceRoutingStrategyInject
 * Invokes the active middleware strategy to influence routing decisions
 *
 * @param dataMessage - Pointer to the message data for which the middleware can influence the next routing direction
 * @return void
 */
void Network::influenceRoutingStrategyInject(char *messageEncodeBuffer, size_t encodeBufferSize, char *dataMessagePayload,uint8_t *destinationIP) {
    if (!::isMiddlewareStrategyActive(STRATEGY_INJECT)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method (injectMetric) that is not permitted by the active middleware strategy. \n");
        return;
    }

    InjectContext* context = (InjectContext*)::middlewareGetStrategyContext();
    if(context != nullptr)context->influenceRouting(messageEncodeBuffer,encodeBufferSize,dataMessagePayload,destinationIP);
}


/**
 * injectNodeMetric
 * Inserts/updates the metric data for this node.
 *
 * Note: This method works only when the active middleware strategy is STRATEGY_INJECT and has been initialized.
 *
 * @param metric - Pointer to the metric data to be injected or updated.
 * @return void
 */
void Network::injectMetric(void *metric) {
    if (!::isMiddlewareStrategyActive(STRATEGY_INJECT)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method (injectMetric) that is not permitted by the active middleware strategy. \n");
        return;
    }

    InjectContext* context = (InjectContext*)::middlewareGetStrategyContext();
    if(context != nullptr)context->injectNodeMetric(metric);
}


/**
 * initMiddlewareStrategyPubSub
 * Initializes the middleware strategy Pub/Sub by assigning the required function pointers.
 * This function must be called only after selecting the STRATEGY_PUBSUB strategy using middlewareSelectStrategy.
 *
 * @param decodeTopicFunction - Pointer to the function responsible for decoding a buffer into a topic identifier.
 *
 * @return void
 */
void Network::initMiddlewareStrategyPubSub(void (*decodeTopicFunction)(char*,int8_t *)){
    ::initMiddlewareStrategyPubSub(decodeTopicFunction);
}

void Network::influenceRoutingStrategyPubSub(char *messageEncodeBuffer, size_t encodeBufferSize, char *dataMessagePayload) {
    if (!::isMiddlewareStrategyActive( STRATEGY_PUBSUB)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method (subscribeToTopic) that is not permitted by the active middleware strategy. \n");
        return;
    }

    PubSubContext* context = (PubSubContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->influenceRouting(messageEncodeBuffer,encodeBufferSize,dataMessagePayload);
}

/**
 * subscribeToTopic
 * Registers a subscription to the specified topic on the local node and propagates
 * the subscription information to the rest of the network.
 *
 * Note: This method works only when the active middleware strategy is STRATEGY_PUBSUB and has been initialized.
 *
 * @param subTopic - Identifier of the topic to subscribe to
 * @return void
 */
void Network::subscribeToTopic(int8_t topic) {
    if (!::isMiddlewareStrategyActive( STRATEGY_PUBSUB)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method (subscribeToTopic) that is not permitted by the active middleware strategy. \n");
        return;
    }

    PubSubContext* context = (PubSubContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->subscribeToTopic(topic);
}


/**
 * unsubscribeFromTopic
 * Removes the local subscription to the specified topic and propagates
 * the unsubscription information to the rest of the network.
 *
 * Note: This method works only when the active middleware strategy is STRATEGY_PUBSUB and has been initialized.
 *
 * @param subTopic - Identifier of the topic to unsubscribe from
 * @return void
 */
void Network::unsubscribeToTopic(int8_t topic){
    if (!::isMiddlewareStrategyActive( STRATEGY_PUBSUB)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method (unsubscribeToTopic) that is not permitted by the active middleware strategy. \n");
        return;
    }

    PubSubContext* context = (PubSubContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->unsubscribeToTopic(topic);
}

/**
 * advertiseTopic
 * Announces that this node is publishing a specific topic, making this
 * information available to other nodes in the network.
 *
 * Note: This method works only when the active middleware strategy is STRATEGY_PUBSUB and has been initialized.
 *
 * @param pubTopic - Identifier of the topic being advertised for publication
 * @return void
 */
void Network::advertiseTopic(int8_t topic){
    if (!::isMiddlewareStrategyActive( STRATEGY_PUBSUB)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method (advertiseTopic) that is not permitted by the active middleware strategy. \n");
        return;
    }

    PubSubContext* context = (PubSubContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->advertiseTopic(topic);
}


/**
 * unadvertiseTopic
 * Informs the network that this node is no longer publishing the specified topic,
 * removing it from the advertised publication list.
 *
 * Note: This method works only when the active middleware strategy is STRATEGY_PUBSUB and has been initialized.
 *
 * @param pubTopic - Identifier of the topic that is no longer being published
 * @return void
 */
void Network::unadvertiseTopic(int8_t topic){
    if (!::isMiddlewareStrategyActive( STRATEGY_PUBSUB)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method (unadvertiseTopic) that is not permitted by the active middleware strategy. \n");
        return;
    }

    PubSubContext* context = (PubSubContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->unadvertiseTopic(topic);
}

//TODO HEADER
void Network::subscribeAndPublishTopics(int8_t *subscribeList, int subCount, int8_t *publishList, int pubCount) {
    if (!::isMiddlewareStrategyActive( STRATEGY_PUBSUB)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method (subscribeAndPublishTopics) that is not permitted by the active middleware strategy. \n");
        return;
    }

    PubSubContext* context = (PubSubContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->subscribeAndPublishTopics(subscribeList,subCount,publishList,pubCount);
}
/**
 * initMiddlewareStrategyTopology
 * Initializes the middleware strategy Topology by assigning the required function pointers.
 * This function must be called only after selecting the STRATEGY_TOPOLOGY using middlewareSelectStrategy.
 *
 * @param topologyMetricValues - Pointer to the memory where the topology metric table values are allocated.
 * @param topologyMetricStructSize - Size of the topology metric structure in bytes.
 * @param setValueFunction - Pointer to the function responsible for updating values within the topology table.
 * @param encodeTopicFunction - Pointer to the function responsible for encoding the topology data into a buffer.
 * @param decodeTopicFunction - Pointer to the function responsible for decoding a buffer into a topology metric struct.
 * @param selectParentFunction - Pointer to the function that selects the parent for a new node from a list of potential parents
 *
 * @return void
 */
void Network::initMiddlewareStrategyTopology(void *topologyMetricValues, size_t topologyMetricStructSize,void (*setValueFunction)(void*,void*),void (*encodeTopologyMetricFunction)(char*,size_t,void *),void (*decodeTopologyMetricFunction)(char*,void *),void (*printMetricFunction)(TableEntry*),uint8_t * (*selectParentFunction)(uint8_t *, uint8_t (*)[4], uint8_t)){
    ::initMiddlewareStrategyTopology(topologyMetricValues, topologyMetricStructSize,setValueFunction,encodeTopologyMetricFunction,decodeTopologyMetricFunction,printMetricFunction,selectParentFunction);
}

/**
 * setParentMetric
 *
 * Sets the metric value for this node and sends it to the root when a new node joins the network.
 * This allows the root to select the best parent node based on specific criteria.
 *
 * Note: This method is effective only when the active middleware strategy is STRATEGY_TOPOLOGY and has been initialized.
 *
 * @param metric - The metric value to assign to this node.
 * @return void
 */
void Network::setParentMetric(void *metric) {
    if (!::isMiddlewareStrategyActive( STRATEGY_TOPOLOGY)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method (setParentMetric) that is not permitted by the active middleware strategy. \n");
        return;
    }

    TopologyContext* context = (TopologyContext*) ::middlewareGetStrategyContext();
    if(context != nullptr)context->setParentMetric(metric);
}


/**
 * sendMessageToRoot
 * Sends a data message to the root node of the network.
 *
 * @param messageBuffer - Application-provided buffer for message encoding
 * @param bufferSize - Size of the message buffer.
 * @param messagePayload - Application-specific payload to be included in the message.
 * @return void
 */
void Network::sendMessageToRoot(char* messageBuffer,size_t bufferSize,const char* messagePayload) {
    sendDataMessageToNode(messageBuffer,bufferSize,messagePayload,rootIP);
}


/**
 * sendMessageToParent
 * Sends a data message to this node's parent in the routing tree.
 *
 * @param messageBuffer - Application-provided buffer for message encoding
 * @param bufferSize - Size of the message buffer.
 * @param messagePayload - Application-specific payload.
 * @return void
 */
void Network::sendMessageToParent(char* messageBuffer,size_t bufferSize,const char *messagePayload) {
    sendDataMessageToParent(messageBuffer,bufferSize,messagePayload);
}


/**
 * sendMessageToChildren
 * Sends a data message to all child nodes connected to this node.
 *
 * @param messageBuffer - Application-provided buffer for the message for message encoding
 * @param bufferSize - Size of the message buffer.
 * @param messagePayload - Application-specific payload.
 * @return void
 */
void Network::sendMessageToChildren(char* messageBuffer,size_t bufferSize,const char *messagePayload) {
    sendDataMessageToChildren(messageBuffer,bufferSize,messagePayload);
}


/**
 * sendMessageToNode
 * Sends a data message to a specific node identified by its IP address.
 *
 * @param messageBuffer - Application-provided buffer for the message for message encoding
 * @param bufferSize - Size of the message buffer.
 * @param messagePayload - Application-specific payload.
 * @param nodeIP - Target node's IP address.
 * @return void
 */
void Network::sendMessageToNode(char* messageBuffer,size_t bufferSize,const char *messagePayload,uint8_t *nodeIP) {
    sendDataMessageToNode(messageBuffer,bufferSize,messagePayload,nodeIP);
}


/**
 * broadcastMessage
 * Broadcasts a data message to all nodes in the network.
 *
 * @param messageBuffer - Application-provided buffer for the message for message encoding
 * @param bufferSize - Size of the message buffer.
 * @param messagePayload - Application-specific payload.
 * @return void
 */
void Network::broadcastMessage(char* messageBuffer,size_t bufferSize,const char *messagePayload) {
    uint8_t broadcastIP[4]={255,255,255,255};
    sendDataMessageToNode(messageBuffer,bufferSize,messagePayload,broadcastIP);
}



void Network::encodeDataMessage(char *encodeBuffer, size_t bufferSize, const char *messagePayload, uint8_t* destinationIP) {
    ::encodeDataMessage(encodeBuffer,bufferSize,messagePayload,myIP,destinationIP);
}



/**
 * onDataReceived
 * Registers a callback to be invoked when a data message is received.
 *
 * @param callback - Function pointer to the callback with the signature (senderIP, receiverIP, payload).
 * @return void
 */
void Network::onDataReceived(void (*callback)(uint8_t *, uint8_t *, char *)) {
    onDataMessageCallback = callback;
}



/**
 * onPeriodicAppTask
 * Registers a callback to be called periodically for application-specific tasks.
 *
 * @param callback - Function pointer to a no-argument callback function.
 * @return void
 */
void Network::onPeriodicAppTask(void (*callback)()) {
    onAppPeriodicTaskCallback = callback;
}

void Network::onNetworkJoin(void (*callback)(uint8_t *)) {
    onNodeJoinNetworkAppCallback = callback;
}

void Network::onChildConnect(void (*callback)(uint8_t *)) {

}



void Network::getNodeMAC(uint8_t *MAC) {
    if(MAC == nullptr)return;
    getMyMAC(MAC);
}

void Network::getNodeIP(uint8_t *IP) {
    if(IP == nullptr)return;
    assignIP(IP,myIP);
}

void Network::getParentIP(uint8_t *IP) {
    if(IP == nullptr)return;
    getGatewayIP(IP);
}

void Network::getRootIP(uint8_t *IP) {
    if(IP == nullptr)return;
    assignIP(IP,rootIP);
}

void *Network::getParentMetric(uint8_t *nodeIP) {
    if(!iamRoot)return nullptr;
    if (!::isMiddlewareStrategyActive( STRATEGY_TOPOLOGY)){
        LOG(MIDDLEWARE, INFO, "⚠️ Warning: Attempted to access a method that is not permitted by the active middleware strategy. \n");
        return nullptr;
    }

    TopologyContext* context = (TopologyContext*) ::middlewareGetStrategyContext();
    if(context != nullptr) return context->getParentMetric(nodeIP);

    return nullptr;
}

int Network::getHopDistanceToNode(uint8_t *nodeIP) {
    return getDistanceToNode(nodeIP);
}

int Network::getHopDistanceToRoot() {
    return rootHopDistance;
}

int Network::getNumberOfChildren() {
    return numberOfChildren;
}









