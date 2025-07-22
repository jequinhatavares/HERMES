#include <network.h>

#include <stdio.h>
#include <unistd.h>

class Network network;


void periodicSensorMeasure(){
    char dataPayload[10];
    float data;
    // Generate or read some data (e.g., from a sensor)
    data = 5.0;

    // Format the message to be sent to an intermediate node for processing
    snprintf(dataPayload, sizeof(dataPayload),"HUMIDITY:%f",data);

    /***
     * If this node has registered as a publisher for the "HUMIDITY" topic,
     * the middleware will influence routing by identifying all subscribed nodes
     * and forwarding the message directly to them.
     * This offloads the processing from the root node, allowing intermediate or
     * leaf nodes to handle topic-specific operations more efficiently.
    ***/
    network.middlewareInfluenceRouting(dataPayload);

}

void setup();

void setup(){
    uint8_t MAC[8];

    enableModule(APP);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    network.getNodeMAC(MAC);

    network.begin();

    network.middlewareSelectStrategy(STRATEGY_PUBSUB);
    network.initMiddlewareStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);


    if(MAC[5] == 135){
        network.subscribeToTopic(TEMPERATURE);
        network.advertiseTopic(HUMIDITY);
    }else if(MAC[5] == 12){
        network.subscribeToTopic(HUMIDITY);
        network.advertiseTopic(TEMPERATURE);
    }else if(MAC[5] == 252){
        network.subscribeToTopic(HUMIDITY);
        network.advertiseTopic(CAMERA);
    }

    network.onPeriodicAppTask(periodicSensorMeasure);


}

int main() {

    LOG(APP, INFO, "Hello, world from Raspberry Pi\n");

    setup();

    while (1) {
        network.run();
    }

}