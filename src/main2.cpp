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

//topologyTableEntry topologyMetrics[TABLE_MAX_SIZE];

//MetricTableEntry metrics[TABLE_MAX_SIZE];

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
    LOG(APP,INFO,"MY MAC: %hhu.%hhu.%hhu.%hhu.%hhu.%hhu\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);

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
    initMiddlewareStrategyTopology(topologyMetrics, sizeof(topologyTableEntry),setTopologyMetricValue,encodeTopologyMetricEntry,decodeTopologyMetricEntry,chooseParentByProcessingCapacity);
    TopologyContext *context = (TopologyContext*) middlewareGetStrategyContext();
    myMetric.processingCapacity = MAC[5];
    if(context != nullptr)context->setParentMetric(&myMetric);***/

    //middlewareSelectStrategy(STRATEGY_NONE);
    //First init the middleware strategy
    network.middlewareSelectStrategy(STRATEGY_PUBSUB);
    network.initMiddlewareStrategyPubSub(decodeTopicWrapper);

    //Then init the callback function for data message receiving
    network.onDataReceived(handleDataMessageWrapper);

    //And then the node can be initialized and integrated in the network
    network.begin();

    LOG(APP,INFO,"MY MAC after begin: %hhu.%hhu.%hhu.%hhu.%hhu.%hhu\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);

    if(MAC[5] == 89 && MAC[4] == 248 && MAC[3] == 169 && MAC[2] == 45){
        LOG(APP,INFO,"1\n");
        worker.registerNodeAsInput();
        worker.registerNodeAsWorker();
    }
    else if(MAC[5] == 12 && MAC[4] == 150 && MAC[3] == 51 && MAC[2] == 26){
        LOG(APP,INFO,"2\n");
        worker.registerNodeAsInput();
        worker.registerNodeAsWorker();
    }else if(MAC[5] == 252 && MAC[4] == 8 && MAC[3] == 107 && MAC[2] == 164){
        LOG(APP,INFO,"3\n");
        worker.registerNodeAsWorker();
    }

}

void loop(){
   network.run();
}
#endif

#ifdef raspberrypi_3b
#include <stdio.h>
#include <unistd.h>

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
#include "middleware.h"

//pio remote --agent raspberrypi run --force-remote -e raspberrypi_3b

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

    initTime();

    Advance(SM, eSuccess);//Init

     middlewareSelectStrategy(STRATEGY_TOPOLOGY);
     initMiddlewareStrategyTopology();

    if(!iamRoot){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Search APs
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Choose Parent
    }

}

int main() {

    printf("Hello, world from Raspberry Pi 12!\n");
    LOG(NETWORK, INFO, "Logs are working\n");
    setup();

    while (1) {
        int packetSize;

        waitForWifiEvent();

        //Wait for incoming requests
        packetSize = receiveMessage(receiveBuffer, sizeof(receiveBuffer));
        if (packetSize > 0){
            insertLast(stateMachineEngine, eMessage);
            if(packetSize >= 255){
                LOG(MESSAGES, ERROR,"Receiving buffer is too small packet has size:%i\n", packetSize);
            }

        }

        handleTimers();

        if(stateMachineEngine->size != 0){
            Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
        }

        //cliInteraction();
    }

    close(sockfd);
    close(hostapd_sockfd);
    return 0;

}
#endif