#pragma once

#include "core/middleware/strategies/strategy_interface.h"
#include "core/logger/logger.h"
#include "core/table/table.h"
#include "core/time_hal/time_hal.h"
#include "core/ip_tools/ip_tools.h"
#include <cstdint>
#include <cstdio>



/**
 * @class Network
 * @brief Integrates the node into a multi-hop, self-healing, and self-managed network.
 *
 * This class handles all Wi-Fi, routing, and middleware processes behind the scenes,
 * providing a simple interface for the application to:
 *  - Send messages across the network seamlessly,
 *  - Perform periodic tasks,
 *  - Choose and apply middleware strategies to influence routing behavior.
 *
 * It abstracts complex network logic, enabling the application to perform specific tasks
 * without worrying about underlying networking details.
 */
class Network {

    public:

    bool iamRoot=false;

    // ==================== NETWORK CORE =====================
    void setAsRoot(bool isRoot);        // Set this node as root (call before begin())
    void begin();                       // Initialize network components (call in setup()) and integrate the node into the network
    void run();                         // Called in loop() to the node to run the program

    // ==================== CALLBACKS ========================
    void onDataReceived(void (*callback)(uint8_t*, uint8_t*, char*));   // Set the function that handles incoming data messages
    void onACKReceived(void (*callback)(uint8_t*, uint8_t*, char*));    // Set the function that handles incoming ACK messages
    void onPeriodicAppTask(void (*callback)());                         // Registers a periodic application task callback

    void onNetworkJoin(void (*callback)(uint8_t* parentIP));

    // ================== MIDDLEWARE  =================
    StrategyType getActivemiddlewareStrategy();
    void middlewareSelectStrategy(StrategyType strategyType);   // Sets the middleware routing strategy to use
    void middlewareInfluenceRouting(char* messageEncodeBuffer,size_t encodeBufferSize,char* dataMessagePayload);         // Allows application layer to influence routing decisions (uses the currently selected middleware strategy)

    // ================ STRATEGY: INJECT =====================
    // Configures the Inject strategy
    void initMiddlewareStrategyInject(void *metricStruct, size_t metricStructSize,void (*setValueFunction)(void*,void*)
                                      ,void (*encodeMetricFunction)(char*,size_t,void *),void (*decodeMetricFunction)(char*,void *));
    void injectMetric(void*metric); // Inject new node metric into network

    // ================ STRATEGY: PUB/SUB ====================
    void initMiddlewareStrategyPubSub(void (*encodeTopicFunction)(char*,size_t,void *)
                                      ,void (*decodeTopicFunction)(char*,int8_t *)); // Configures the PubSub strategy
    void subscribeToTopic(int8_t topic); // Subscribe to a topic
    void unsubscribeToTopic(int8_t topic); // Remove topic subscription
    void advertiseTopic(int8_t topic); // Advertise that this node publishes a certain topic
    void unadvertiseTopic(int8_t topic); // Stops advertising a topic


    // ================ STRATEGY: TOPOLOGY ===================
    // Configures the Topology strategy
    void initMiddlewareStrategyTopology(void *topologyMetricValues, size_t topologyMetricStructSize
                                        ,void (*setValueFunction)(void*,void*),void (*encodeTopologyMetricFunction)(char*,size_t,void *)
                                        ,void (*decodeTopologyMetricFunction)(char*,void *), uint8_t * (*selectParentFunction)(uint8_t *, uint8_t (*)[4], uint8_t));
    void setParentMetric(void*metric); // Sets this node metric and sends it to the root for it to be able to discriminate between parent candidates
    void* getParentMetric(uint8_t *nodeIP); //Gets the metric associated to a node as parent this method is only available at the root node

    // ================== MESSAGE ROUTING ====================
    void sendMessageToRoot(char* messageBuffer,size_t bufferSize,const char* messagePayload); // Sends a message to the root node
    void sendMessageToParent(char* messageBuffer,size_t bufferSize,const char* messagePayload);// Sends a message to the parent node
    void sendMessageToChildren(char* messageBuffer,size_t bufferSize,const char* messagePayload);// Sends a message to the children nodes
    void sendMessageToNode(char* messageBuffer,size_t bufferSize,const char* messagePayload, uint8_t* nodeIP);// Sends a message to a specific node in the network
    void broadcastMessage(char* messageBuffer,size_t bufferSize,const char* messagePayload); // Broadcasts a message to all nodes in the network
    void sendACKMessage(char* messageBuffer,size_t bufferSize,const char* ackPayload, uint8_t* destinationIP); // Sends an ACK message to a node

    // ================== NETWORK INFORMATION ==================
    void getNodeMAC(uint8_t *MAC);  // Retrieves the MAC address of this node and stores it in the provided MAC[6] array
    void getNodeIP(uint8_t *IP);    // Retrieves the IP address of this node and stores it in the provided IP[6] array
    void getParentIP(uint8_t *IP);  // Retrieves the IP address of the parent and stores it in the provided IP[6] array
    void getRootIP(uint8_t *IP);    // Retrieves the IP address of the root and stores it in the provided IP[6] array
};


extern Network network;


