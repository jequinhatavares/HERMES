#include "middleware.h"

Strategy *activeStrategy = nullptr;

void middlewareSelectStrategy(StrategyType strategyType){
    switch (strategyType) {
        case STRATEGY_INJECT:
            activeStrategy = &strategyInject;
            break;
        case STRATEGY_PUBSUB:
            activeStrategy = &strategyPubSub;
            break;

        case STRATEGY_NONE:
            break;

        default:
            break;
    }
}

void initCallbacks(){
    middlewareOnTimerCallback = middlewareOnTimer;
    middlewareHandleMessageCallback = middlewareHandleMessage;
    middlewareInfluenceRoutingCallback = middlewareInfluenceRouting;
    middlewareOnNetworkEventCallback = middlewareOnNetworkEvent;
}

void initMiddlewareStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*),void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *)){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Initialization attempted without a selected strategy\n");
        return;
    }
    initStrategyInject(setValueFunction,metricStruct,metricStructSize,encodeMetricFunction,decodeMetricFunction);
    initCallbacks();
}

void initMiddlewareStrategyPubSub(void (*setValueFunction)(void*,void *),void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,void *)){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Initialization attempted without a selected strategy\n");
        return;
    }
    initStrategyPubSub(setValueFunction,encodeTopicFunction,decodeTopicFunction);
    initCallbacks();
}

void middlewareInfluenceRouting(char* dataMessage){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Trying to influence routing without selecting a middleware strategy\n");
        return;
    }
    activeStrategy->influenceRouting(dataMessage);
}

void middlewareHandleMessage(char* messageBuffer, size_t bufferSize){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Cannot handle middleware messages without a strategy selected.\n");
        return;
    }
    activeStrategy->handleMessage(messageBuffer,bufferSize);
}

void middlewareEncodeMessage(char* messageBuffer, size_t bufferSize, int type){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Cannot encode middleware messages without a strategy selected.\n");
        return;
    }
    activeStrategy->encodeMessage(messageBuffer,bufferSize,type);
}

void middlewareOnTimer(){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Cannot perform middleware periodic tasks, no active strategy selected.\n");
        return;
    }
    activeStrategy->onTimer();
}

void middlewareOnNetworkEvent(int networkEvent, int involvedIP[4]){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Cannot perform middleware periodic tasks, no active strategy selected.\n");
        return;
    }
    activeStrategy->onNetworkEvent(networkEvent, involvedIP);
}

void* middlewareGetStrategyContext(){
    if(activeStrategy == nullptr){
        LOG(NETWORK,ERROR,"ERROR: Cannot get strategy context without a strategy selected.\n");
        return nullptr;
    }
    return activeStrategy->getContext();
}