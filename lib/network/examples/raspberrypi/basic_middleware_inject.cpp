#include <network.h>

#include <stdio.h>
#include <unistd.h>

class Network network;

MetricTableEntry metrics[TABLE_MAX_SIZE];

void periodicSensorMeasure(){
    char dataPayload[10];
    float data;
    // Generate or read some data (e.g., from a sensor)
    data = 5.0;

    // Format the message to be sent to an intermediate node for processing
    snprintf(dataPayload, sizeof(dataPayload),"SENSOR_MEASURE:%f",data);

    /*** If certain conditions are met (e.g., data is valid or requires extra processing),
     * call the middleware to influence the routing decision.
     * For example, with the Inject strategy, the middleware will analyze available nodes,
     * select the one with the highest processing capacity, and route the message through it
     * for processing before forwarding it to the final destination.***/
    if(data>0){
        network.middlewareInfluenceRouting(dataPayload);
    }
}


void setup();

void setup(){
    uint8_t MAC[6];
    MetricTableEntry myMetric;

    enableModule(APP);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    //To auto initialize the root node has the node with the IP 135.230.96.1
    network.getNodeMAC(MAC);

    // Select and activate the middleware strategy to be used
    network.middlewareSelectStrategy(STRATEGY_INJECT);

    // Initialize the selected strategy (Inject) with the metric structure and related handlers
    network.initMiddlewareStrategyInject(
            (void*) metrics,                        // Pointer to the full metric table
            sizeof(MetricTableEntry),            // Size of each metric entry
            setMetricValue,                     // Function to update a metric
            encodeMetricEntry,               // Function to serialize a metric entry
            decodeMetricEntry                // Function to deserialize a metric entry
    );

    network.onPeriodicAppTask(periodicSensorMeasure);

    network.begin();

}

int main() {

    LOG(APP, INFO, "Hello, world from Raspberry Pi\n");

    setup();

    while (1) {
        network.run();
    }

}