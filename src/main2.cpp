//#define raspberrypi_3b
#if defined(ESP32) || defined(ESP8266)

#include <network.h>
#include <neural_network_dispatcher.h>
#include <Arduino.h>

//227:96:230:135 root
//227:96:237:119
#ifdef ROOT
NeuralNetworkCoordinator worker;
#endif
#ifndef ROOT
NeuronWorker worker;
#endif


//MetricTableEntry metrics[TABLE_MAX_SIZE];

struct topologyMetric{
    int processingCapacity;
};

topologyMetric topologyMetrics[TABLE_MAX_SIZE];


uint8_t* chooseParentByProcessingCapacity(uint8_t * targetNodeIP, uint8_t potentialParents[][4], uint8_t nPotentialParents){
    int maxProcessingCapacity = 0;
    int bestParentIndex = -1;
    topologyMetric *topologyMetricValue;

    for (int i = 0; i < nPotentialParents; i++) {
        topologyMetricValue = (topologyMetric*) network.getParentMetric(potentialParents[i]);
        if(topologyMetricValue != nullptr){
            LOG(MIDDLEWARE,DEBUG,"Potential Parent: %hhu.%hhu.%hhu.%hhu metric:%d\n",potentialParents[i][0],potentialParents[i][1],potentialParents[i][2],potentialParents[i][3],topologyMetricValue->processingCapacity);
            if(topologyMetricValue->processingCapacity >= maxProcessingCapacity){
                bestParentIndex = i;
                maxProcessingCapacity = topologyMetricValue->processingCapacity;
                LOG(MIDDLEWARE,DEBUG,"Parent Selected\n");
            }
        }
    }

    if(bestParentIndex != -1){
        LOG(MIDDLEWARE,DEBUG,"Chosen Parent: %hhu.%hhu.%hhu.%hhu metric:%d\n",potentialParents[bestParentIndex][0],potentialParents[bestParentIndex][1],potentialParents[bestParentIndex][2],potentialParents[bestParentIndex][3]);
        return potentialParents[bestParentIndex];
    }// If no parent has been selected, return nullptr
    else{ return nullptr;}/******/
    return nullptr;
}

void encodeTopologyMetricEntry(char* buffer, size_t bufferSize, void *metricEntry){
    topologyMetric *metric = (topologyMetric*) metricEntry;
    snprintf(buffer,bufferSize,"%i", metric->processingCapacity);
}

void decodeTopologyMetricEntry(char* buffer, void *metricEntry){
    topologyMetric *metric = (topologyMetric*)metricEntry;
    sscanf(buffer,"%i", &metric->processingCapacity);
}


void setTopologyMetricValue(void* av, void*bv){
    if(bv == nullptr)return; //
    topologyMetric *a = (topologyMetric *) av;
    topologyMetric *b = (topologyMetric *) bv;

    a->processingCapacity = b->processingCapacity;
}

void printTopologyMetricStruct(TableEntry* Table){
    LOG(MIDDLEWARE,INFO,"Node[%hhu.%hhu.%hhu.%hhu] → (Topology Metric: %d) \n",
        ((uint8_t *)Table->key)[0],((uint8_t *)Table->key)[1],((uint8_t *)Table->key)[2],((uint8_t *)Table->key)[3],
        ((topologyMetric *)Table->value)->processingCapacity);
}

void waitForEnter() {
    // Wait for Enter
    while (Serial.available() <= 0) {
        delay(10);
    }
    // Flush anything else left in the serial buffer
    while (Serial.available()) {
        Serial.read();
    }

    Serial.println("Starting program...");
}

void decodeTopicWrapper(char* dataMessage, int8_t* topicType){
    worker.decodeNeuronTopic(dataMessage,topicType);
}
void handleDataMessageWrapper(uint8_t * senderIP,uint8_t *destinationIP,char* dataMessage){
#ifdef ROOT
    worker.handleNeuralNetworkMessage(senderIP,destinationIP,dataMessage);
#endif
#ifndef ROOT
    worker.handleNeuronMessage(senderIP,destinationIP,dataMessage);
#endif
}

void setup(){
    uint8_t MAC[6];
    Serial.begin(115200);
    topologyMetric myMetric;

    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(MONITORING_SERVER);
    enableModule(CLI);
    enableModule(MIDDLEWARE);
    enableModule(APP);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;


    #ifdef ESP32
        LOG(NETWORK,INFO,"ESP32\n");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        LOG(NETWORK,INFO,"ESP8266\n");
    #endif

    //To auto initialize the root node has the node with the IP 135.230.96.1
    network.getNodeMAC(MAC);
    //LOG(APP,INFO,"MY MAC: %hhu.%hhu.%hhu.%hhu.%hhu.%hhu\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);

    //To auto initialize the root node has the node with the IP 135.230.96.1
    if(MAC[5] == 135 && MAC[4] == 230 && MAC[3] == 96)
    {
        network.setAsRoot(true);
    }
    waitForEnter();


    //Select and initialize the middleware strategy
    //middlewareSelectStrategy(STRATEGY_INJECT);
    //initMiddlewareStrategyInject((void*) metrics,sizeof(MetricTableEntry),setMetricValue,encodeMetricEntry,decodeMetricEntry);
    //InjectContext *context = (InjectContext*) middlewareGetStrategyContext();
    //myMetric.processingCapacity = MAC[5];
    //context->injectNodeMetric(&myMetric);

    /***middlewareSelectStrategy(STRATEGY_TOPOLOGY);
    initMiddlewareStrategyTopology(topologyMetrics, sizeof(topologyMetric),setTopologyMetricValue,encodeTopologyMetricEntry,decodeTopologyMetricEntry,chooseParentByProcessingCapacity);
    TopologyContext *context = (TopologyContext*) middlewareGetStrategyContext();
    myMetric.processingCapacity = MAC[5];
    if(context != nullptr)context->setParentMetric(&myMetric);***/

    //middlewareSelectStrategy(STRATEGY_NONE);

    /************* Middleware Strategy: PubSub ************/
    //First init the middleware strategy
    /***network.middlewareSelectStrategy(STRATEGY_PUBSUB);
    network.initMiddlewareStrategyPubSub(decodeTopicWrapper);***/

    /************* Middleware Strategy: Topology *************/
    network.middlewareSelectStrategy(STRATEGY_TOPOLOGY);
    network.initMiddlewareStrategyTopology(topologyMetrics, sizeof(topologyMetric),setTopologyMetricValue,
                                           encodeTopologyMetricEntry, decodeTopologyMetricEntry,
                                           printTopologyMetricStruct,
                                           chooseParentByProcessingCapacity);

    // Assign a topology metric based on device type: NodeMCU = 1, ESP32 = 2, Raspberry Pi = 3
    if(MAC[5] == 89 && MAC[4] == 248 && MAC[3] == 169 && MAC[2] == 45){
        myMetric.processingCapacity=2;
        network.setParentMetric(&myMetric);
    }else if(MAC[5] == 12 && MAC[4] == 150 && MAC[3] == 51 && MAC[2] == 26){
        myMetric.processingCapacity=2;
        network.setParentMetric(&myMetric);
    }else if(MAC[5] == 252 && MAC[4] == 8 && MAC[3] == 107 && MAC[2] == 164){
        myMetric.processingCapacity=2;
        network.setParentMetric(&myMetric);
    }else if(MAC[5] == 135 && MAC[4] == 230 && MAC[3] == 96){
        myMetric.processingCapacity=1;
        network.setParentMetric(&myMetric);
    }

    //Then init the callback function for data message receiving
    network.onDataReceived(handleDataMessageWrapper);

    //And then the node can be initialized and integrated in the network
    network.begin();

    //LOG(APP,INFO,"MY MAC after begin: %hhu.%hhu.%hhu.%hhu.%hhu.%hhu\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);

    // Register each device in the network along with its assigned role
    if(MAC[5] == 89 && MAC[4] == 248 && MAC[3] == 169 && MAC[2] == 45){
        worker.registerNodeAsInput();
        worker.registerNodeAsWorker();
    }
    else if(MAC[5] == 12 && MAC[4] == 150 && MAC[3] == 51 && MAC[2] == 26){
        worker.registerNodeAsInput();
        worker.registerNodeAsWorker();
    }else if(MAC[5] == 252 && MAC[4] == 8 && MAC[3] == 107 && MAC[2] == 164){
        worker.registerNodeAsWorker();
    }

}

void loop(){
   network.run();
   worker.manageNeuron();
#ifdef ROOT
   worker.manageNeuralNetwork();
#endif
}

#endif

#ifdef raspberrypi_3b
#include <stdio.h>
#include <unistd.h>
/***
#include <../lib/wifi_hal/raspberrypi/wifi_raspberrypi.h>
//#include <../lib/transport_hal/raspberrypi/udp_raspberrypi.h>
#include "transport_hal.h"
#include <../lib/time_hal/raspberrypi/time_raspberrypi.h>

//#include <wifi_hal.h>
//#include <transport_hal.h>
//#include "lifecycle.h"
#include "lifecycle.h"
#include "cli.h"
#include "logger.h"
#include "../lib/middleware/strategies/strategy_inject/strategy_inject.h"
#include "../lib/middleware/strategies/strategy_pubsub/strategy_pubsub.h"
#include "middleware.h" ***/


#include <network.h>
#include <neural_network_dispatcher.h>

//pio remote --agent raspberrypi run --force-remote -e raspberrypi_3b

NeuronWorker worker;

void decodeTopicWrapper(char* dataMessage, int8_t* topicType){
    worker.decodeNeuronTopic(dataMessage,topicType);
}
void handleDataMessageWrapper(uint8_t * senderIP,uint8_t *destinationIP,char* dataMessage){
#ifdef ROOT
    worker.handleNeuralNetworkMessage(senderIP,destinationIP,dataMessage);
#endif
#ifndef ROOT
    worker.handleNeuronMessage(senderIP,destinationIP,dataMessage);
#endif
}


void setup();

void setup(){

    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(MONITORING_SERVER);
    enableModule(CLI);
    enableModule(MIDDLEWARE);
    enableModule(APP);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    network.middlewareSelectStrategy(STRATEGY_PUBSUB);
    network.initMiddlewareStrategyPubSub(decodeTopicWrapper);

    //Then init the callback function for data message receiving
    network.onDataReceived(handleDataMessageWrapper);

    //And then the node can be initialized and integrated in the network
    network.begin();

    // Register the RPi device as a worker device
    worker.registerNodeAsWorker();

}

int main() {

    printf("Hello, world from Raspberry Pi 12!\n");
    LOG(NETWORK, INFO, "Logs are working\n");
    setup();

    while (1) {
        network.run();
        worker.manageNeuron();
        //cliInteraction();
    }

    close(sockfd);
    close(hostapd_sockfd);
    return 0;

}
#endif