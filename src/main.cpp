//#define raspberrypi_3b
#if defined(ESP32) || defined(ESP8266)
#include <wifi_hal.h>
#include <transport_hal.h>
#include "lifecycle.h"
#include "cli.h"
#include "logger.h"
#include "../lib/middleware/strategies/strategy_inject/strategy_inject.h"
#include "../lib/middleware/strategies/strategy_pubsub/strategy_pubsub.h"
#include "middleware.h"

//#include "../lib/wifi_hal/wifi_hal.h"
//#include "../lib/transport_hal/esp32/udp_esp32.h"

//227:96:230:135 root
//227:96:237:119

topologyTableEntry topologyMetrics[TableMaxSize];

metricTableEntry metrics[TableMaxSize];

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

void setup(){
    uint8_t MAC[6];
    //metricTableEntry myMetric;
    Serial.begin(115200);


    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(DEBUG_SERVER);
    enableModule(CLI);
    enableModule(MIDDLEWARE);
    enableModule(APP);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;


    //To auto initialize the root node has the node with the IP 135.230.96.1
    getMyMAC(MAC);
    if(MAC[5] == 135 && MAC[4] == 230 && MAC[3] == 96)
    {
        iamRoot = true;
    }


    #ifdef ESP32
        LOG(NETWORK,INFO,"ESP32\n");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        LOG(NETWORK,INFO,"ESP8266\n");
    #endif
    LOG(NETWORK,INFO,"Code uploaded through multi_upload_tool.py V1\n");
    LOG(NETWORK,INFO,"My MAC addr: %i.%i.%i.%i.%i.%i\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);

    waitForEnter();


    Advance(SM, eSuccess);//Init

    //Select and initialize the middleware strategy
    //middlewareSelectStrategy(STRATEGY_INJECT);
    //initMiddlewareStrategyInject((void*) metrics,sizeof(metricTableEntry),setMetricValue,encodeMetricEntry,decodeMetricEntry);
    //InjectContext *context = (InjectContext*) middlewareGetStrategyContext();
    //myMetric.processingCapacity = MAC[5];
    //context->injectNodeMetric(&myMetric);

    /***middlewareSelectStrategy(STRATEGY_PUBSUB);
    initMiddlewareStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);
    //advertise and subscribe to topics
    PubSubContext *context = (PubSubContext*) middlewareGetStrategyContext();
    if(MAC[5] == 135){
        context->subscribeToTopic(TEMPERATURE);
        context->advertiseTopic(HUMIDITY);
    }else if(MAC[5] == 12){
        context->subscribeToTopic(HUMIDITY);
        context->advertiseTopic(TEMPERATURE);
    }else if(MAC[5] == 252){
        context->subscribeToTopic(HUMIDITY);
        context->advertiseTopic(CAMERA);
    }***/


    /***middlewareSelectStrategy(STRATEGY_TOPOLOGY);
    initMiddlewareStrategyTopology(topologyMetrics, sizeof(topologyTableEntry),setTopologyMetricValue,encodeTopologyMetricEntry,decodeTopologyMetricEntry,chooseParentByProcessingCapacity);
    TopologyContext *context = (TopologyContext*) middlewareGetStrategyContext();
    myMetric.processingCapacity = MAC[5];
    if(context != nullptr)context->setMetric(&myMetric);***/

    //middlewareSelectStrategy(STRATEGY_NONE);

    if(!iamRoot){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Search APs
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Choose Parent
    }

    //startWifiAP(ssid,PASS, localIP, gateway, subnet);
    //changeWifiMode(3);
    //LOG(NETWORK,INFO,"My SoftAP IP: %s\nMy STA IP %s\nGateway IP: %s\n", getMyAPIP().toString().c_str(), getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
}

//WiFiClient client;
//bool client_defined = false;


void loop(){
    int packetSize;
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

    if(stateMachineEngine->size != 0){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
    }

    cliInteraction();


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
    enableModule(DEBUG_SERVER);
    enableModule(CLI);
    enableModule(MIDDLEWARE);
    enableModule(APP);


    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    initTime();

    //iamRoot = true;

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