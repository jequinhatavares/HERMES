#include <network.h>
#include <Arduino.h>

//Put Here the MAC of the node you wish to be root
uint8_t rootMAC[6] = {0,0,0,96,230,135};

MetricTableEntry metrics[TABLE_MAX_SIZE];

class Network network;

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

void setup() {
    uint8_t MAC[6];
    Serial.begin(115200);

    MetricTableEntry myMetric;

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

    // Set this node's own metric (e.g., processing capacity) and inject it into the middleware
    myMetric.processingCapacity = MAC[5];
    network.injectMetric(&myMetric);

    network.onPeriodicAppTask(periodicSensorMeasure);
    network.begin();

}

void loop(){
    network.run();
}