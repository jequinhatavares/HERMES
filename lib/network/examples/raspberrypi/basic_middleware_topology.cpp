#include <network.h>

#include <stdio.h>
#include <unistd.h>

class Network network;

topologyTableEntry topologyMetrics[TABLE_MAX_SIZE];


void setup();

void setup(){
    uint8_t MAC[6];
    topologyTableEntry myMetric;

    enableModule(APP);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    network.begin();

    // Select the middleware strategy Topology
    network.middlewareSelectStrategy(STRATEGY_TOPOLOGY);

    // Initialize the Topology strategy
    network.initMiddlewareStrategyTopology(
            topologyMetrics,// - topologyMetrics: a pointer to the array holding the topology metrics for known nodes
            sizeof(topologyTableEntry), // - sizeof(topologyTableEntry): the size of each metric entry in the array
            setTopologyMetricValue,// - setTopologyMetricValue: a function to update an individual metric entry
            encodeTopologyMetricEntry, // - encodeTopologyMetricEntry: function to serialize a metric entry for transmission
            decodeTopologyMetricEntry, // - decodeTopologyMetricEntry: function to deserialize received metric data
            chooseParentByProcessingCapacity // - chooseParentByProcessingCapacity: function used by the root to select the best parent for a new node based on metrics
    );

    /***
     * In the Topology strategy, there is no need to explicitly call network.middlewareInfluenceRouting(),
     * because routing influence happens during the network formation phase. When a new node joins the network,
     * the root selects its parent based on a predefined metric (e.g., processing capacity). By determining the parent-child
     * relationships during network setup, the routing paths are implicitly influenced.
    ***/
    myMetric.processingCapacity = MAC[5];

    // Each node can initialize its own metric (e.g., processing capacity),
    // which will later be used by the root to determine this node's suitability
    // to act as a parent when new nodes join the network
    network.setParentMetric(&myMetric);

}

int main() {

    LOG(APP, INFO, "Hello, world from Raspberry Pi\n");

    setup();

    while (1) {
        network.run();
    }

}