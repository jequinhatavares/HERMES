#include <network.h>
#include <Arduino.h>

//Put Here the MAC of the node you wish to be root
uint8_t rootMAC[6] = {0,0,0,96,230,135};

struct topologyTableEntry{
    int processingCapacity;
};

topologyTableEntry topologyMetrics[10];



uint8_t* chooseParentByProcessingCapacity(uint8_t * targetNodeIP, uint8_t potentialParents[][4], uint8_t nPotentialParents){
    int maxProcessingCapacity = 0;
    int bestParentIndex = -1;
    topologyTableEntry *topologyMetricValue;

    for (int i = 0; i < nPotentialParents; i++) {
        topologyMetricValue = (topologyTableEntry*) getNodeTopologyMetric(potentialParents[i]);
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
    }
    else{ return nullptr;}/******/
    return nullptr;
}

void encodeTopologyMetricEntry(char* buffer, size_t bufferSize, void *metricEntry){
    topologyTableEntry *metric = (topologyTableEntry*) metricEntry;
    snprintf(buffer,bufferSize,"%i", metric->processingCapacity);
}

void decodeTopologyMetricEntry(char* buffer, void *metricEntry){
    topologyTableEntry *metric = (topologyTableEntry*)metricEntry;
    sscanf(buffer,"%i", &metric->processingCapacity);
}


void setTopologyMetricValue(void* av, void*bv){
    if(bv == nullptr)return; //
    topologyTableEntry *a = (topologyTableEntry *) av;
    topologyTableEntry *b = (topologyTableEntry *) bv;

    a->processingCapacity = b->processingCapacity;
}

class Network network;

void setup() {
    uint8_t MAC[6];
    Serial.begin(115200);

    topologyTableEntry myMetric;

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


void loop(){
    network.run();
}