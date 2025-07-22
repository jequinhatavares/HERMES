#include <network.h>
#include <Arduino.h>

class Network network;

//Put Here the MAC of the node you wish to be root
uint8_t rootMAC[6] = {0,0,0,96,230,135};


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


void setup() {
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

    //To auto initialize the root node has the node with the IP 135.230.96.1
    network.getNodeMAC(MAC);
    if(MAC[5] == rootMAC[5] && MAC[4] == rootMAC[4] && MAC[3] == rootMAC[3])
    {
        network.setAsRoot(true);
    }

    network.middlewareSelectStrategy(STRATEGY_PUBSUB);
    network.initMiddlewareStrategyPubSub(setPubSubInfo,encodeTopic,decodeTopic);

    network.begin();

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


void loop(){
    network.run();
}