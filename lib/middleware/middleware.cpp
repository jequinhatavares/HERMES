#include "middleware.h"

Strategy *activeStrategy = nullptr;
StrategyType activeStrategyType = STRATEGY_NONE;

bool isStrategyInitialized = false;



/**
 * middlewareSelectStrategy
 * Selects and activates a middleware strategy to influence routing behavior.
 *
 * @param strategyType - The strategy to be activated (e.g., STRATEGY_INJECT, STRATEGY_PUBSUB).
 *                       STRATEGY_NONE disables any active strategy.
 * @return void
 */
void middlewareSelectStrategy(StrategyType strategyType){
    switch (strategyType) {
        case STRATEGY_INJECT:
            activeStrategy = &strategyInject;
            activeStrategyType = STRATEGY_INJECT;
            break;
        case STRATEGY_PUBSUB:
            activeStrategy = &strategyPubSub;
            activeStrategyType = STRATEGY_PUBSUB;
            break;

        case STRATEGY_TOPOLOGY:
            activeStrategy = &strategyTopology;
            activeStrategyType = STRATEGY_TOPOLOGY;
            break;

        case STRATEGY_NONE:
            activeStrategyType = STRATEGY_NONE;
            initMiddlewareCallbacks();
            break;

        default:
            break;
    }
}

/**
 * initMiddlewareCallbacks
 * Initializes the middleware callback function pointers defined at lower layers (lifecycle) with their corresponding implementations.
 * Injecting the middleware dependencies into the lower layers.
 * This function is not meant to be called directly; it is managed internally.
 *
 * @return void
 */
void initMiddlewareCallbacks(){
    middlewareOnTimerCallback = middlewareOnTimer;
    middlewareHandleMessageCallback = middlewareHandleMessage;
    middlewareInfluenceRoutingCallback = middlewareInfluenceRouting;
    middlewareOnNetworkEventCallback = middlewareOnNetworkEvent;
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
void initMiddlewareStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*),void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *)){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Initialization attempted without a selected strategy\n");
        return;
    }
    initStrategyInject(metricStruct,metricStructSize,setValueFunction,encodeMetricFunction,decodeMetricFunction);
    initMiddlewareCallbacks();
    isStrategyInitialized = true;
}

/**
 * initMiddlewareStrategyPubSub
 * Initializes the middleware strategy Pub/Sub by assigning the required function pointers.
 * This function must be called only after selecting the STRATEGY_PUBSUB strategy using middlewareSelectStrategy.
 *
 * @param setValueFunction - Pointer to the function responsible for updating values within the pubsub table.
 * @param encodeTopicFunction - Pointer to the function responsible for encoding the topic data into a buffer.
 * @param decodeTopicFunction - Pointer to the function responsible for decoding a buffer into a topic identifier.
 *
 * @return void
 */
void initMiddlewareStrategyPubSub(void (*setValueFunction)(void*,void *),void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,int8_t *)){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Initialization attempted without a selected strategy\n");
        return;
    }
    initStrategyPubSub(setValueFunction,encodeTopicFunction,decodeTopicFunction);
    initMiddlewareCallbacks();
    isStrategyInitialized = true;

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
void initMiddlewareStrategyTopology(void *topologyMetricValues, size_t topologyMetricStructSize,void (*setValueFunction)(void*,void*),void (*encodeTopologyMetricFunction)(char*,size_t,void *),void (*decodeTopologyMetricFunction)(char*,void *), uint8_t * (*selectParentFunction)(uint8_t *, uint8_t (*)[4], uint8_t)){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Initialization attempted without a selected strategy\n");
        return;
    }
    initStrategyTopology(topologyMetricValues, topologyMetricStructSize,*setValueFunction,encodeTopologyMetricFunction,decodeTopologyMetricFunction,selectParentFunction);
    middlewareChooseParentCallback = requestParentFromRoot;
    initMiddlewareCallbacks();
    isStrategyInitialized = true;

}

/**
 * middlewareInfluenceRouting
 * Invokes the active middleware strategy to influence routing decisions
 *
 * @param dataMessage - Pointer to the message data for which the middleware can influence the next routing direction
 * @return void
 */
void middlewareInfluenceRouting(char* dataMessage){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Trying to influence routing without selecting a middleware strategy\n");
        return;
    }
    if(!isStrategyInitialized){
        LOG(NETWORK,ERROR,"ERROR: Trying to influence routing without initializing the middleware strategy\n");
        return;
    }
    if(activeStrategyType == STRATEGY_NONE) return;

    activeStrategy->influenceRouting(dataMessage);
}

/**
 * middlewareHandleMessage
 * Passes an incoming MIDDLEWARE message to the active middleware strategy for processing
 *
 * @param messageBuffer - Pointer to the buffer containing the message data.
 * @param bufferSize - Size of the message buffer in bytes.
 *
 * @return void
 */
void middlewareHandleMessage(char* messageBuffer, size_t bufferSize){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Cannot handle middleware messages without a strategy selected.\n");
        return;
    }
    if(!isStrategyInitialized){
        LOG(NETWORK,ERROR,"ERROR: Cannot handle middleware messages without initializing the selected strategy.\n");
        return;
    }
    if(activeStrategyType == STRATEGY_NONE) return;

    activeStrategy->handleMessage(messageBuffer,bufferSize);
}

/**
 * middlewareEncodeMessage
 * Encodes a message according to the active middleware strategy, modifying the provided buffer.
 *
 * @param messageBuffer - Pointer to the buffer where the encoded message will be stored.
 * @param bufferSize - Size of the message buffer in bytes.
 * @param type - An integer representing the middleware type of the message to encode.
 *
 * @return void
 */
void middlewareEncodeMessage(char* messageBuffer, size_t bufferSize, int type){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Cannot encode middleware messages without a strategy selected.\n");
        return;
    }
    if(!isStrategyInitialized){
        LOG(NETWORK,ERROR,"ERROR: Cannot encode middleware messages without a strategy initialized.\n");
        return;
    }
    if(activeStrategyType == STRATEGY_NONE) return;

    activeStrategy->encodeMessage(messageBuffer,bufferSize,type);
}

/**
 * middlewareOnTimer
 * Executes periodic middleware related tasks defined by the active strategy.
 *
 * @return void
 */
void middlewareOnTimer(){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Cannot perform middleware periodic tasks, no active strategy selected.\n");
        return;
    }
    if(!isStrategyInitialized){
        LOG(NETWORK,ERROR,"ERROR: Cannot perform middleware periodic tasks without the strategy initialized.\n");
        return;
    }
    if(activeStrategyType == STRATEGY_NONE) return;

    activeStrategy->onTimer();
}

/**
 * middlewareOnNetworkEvent
 * Calls the active strategy to respond to a network event triggered by the lower layer.
 *
 * @param networkEvent - Integer representing the type of network event.
 * @param involvedIP - Array of 4 integers representing the IP address involved in the event.
 *
 * @return void
 */
void middlewareOnNetworkEvent(int networkEvent, uint8_t involvedIP[4]){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Cannot perform middleware onNetworkEvent tasks, no active strategy selected.\n");
        return;
    }
    if(!isStrategyInitialized){
        LOG(NETWORK,ERROR,"ERROR: Cannot perform middleware onNetworkEvent tasks without a strategy initialized.\n");
        return;
    }
    if(activeStrategyType == STRATEGY_NONE) return;

    activeStrategy->onNetworkEvent(networkEvent, involvedIP);
}

/**
 * middlewareGetStrategyContext
 * Retrieves the context functions associated with currently active middleware strategy.
 *
 * @return void* - Pointer to the strategy's context functions, or nullptr if no strategy is selected or the strategy has no context.
 */
void* middlewareGetStrategyContext(){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Cannot get strategy context without a strategy selected.\n");
        return nullptr;
    }
    if(!isStrategyInitialized){
        LOG(NETWORK,ERROR,"ERROR: Cannot get strategy context without a strategy initialized.\n");
        return nullptr;
    }
    if(activeStrategyType == STRATEGY_NONE) return nullptr;

    return activeStrategy->getContext();
}