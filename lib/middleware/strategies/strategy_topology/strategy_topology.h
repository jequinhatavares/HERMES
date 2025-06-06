#ifndef STRATEGY_TOPOLOGY_H
#define STRATEGY_TOPOLOGY_H

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "routing.h"
#include "messages.h"
#include "logger.h"
#include "../../../time_hal/time_hal.h"
#include "../../../transport_hal/transport_hal.h"
#include "../../../wifi_hal/wifi_hal.h"
#include "../strategy_interface.h"
#include <lifecycle.h> //This dependency can be removed when the config file is created




extern int orphanIP[4];
extern int newParentIP[4];

typedef enum TopologyMessageType{
    TOP_PARENT_LIST_ADVERTISEMENT_REQUEST,//0
    TOP_PARENT_LIST_ADVERTISEMENT,//1
    TOP_PARENT_REASSIGNMENT_COMMAND, //1
} TopologyMessageType;


// Topology strategy API
typedef struct TopologyContext{

} TopologyContext;

void initStrategyTopology();
void encodeMessageStrategyTopology(char* messageBuffer, size_t bufferSize, int typeTopology);
void handleMessageStrategyTopology(char* messageBuffer, size_t bufferSize);
void onNetworkEventStrategyTopology(int networkEvent, int involvedIP[4]);
void influenceRoutingStrategyTopology(char* dataMessage);
void onTimerStrategyTopology();
void* getContextStrategyTopology();

parentInfo chooseParentProcedure(parentInfo* possibleParents, int nrOfPossibleParents);
void chooseParentStrategyTopology(char* message);


#endif //STRATEGY_TOPOLOGY_H
