#ifndef ROUTING_H
#define ROUTING_H

/*** Include config.h at the top of every file that uses configurable macros.
 *   This ensures user-defined values take priority at compile time. ***/
//#include "network_config.h"


#include "../table/table.h"
#include "../logger/logger.h"
#include "../ip_tools/ip_tools.h"

#include <cstdint>

#ifndef ROUTING_UPDATE_INTERVAL
#define ROUTING_UPDATE_INTERVAL 180000
#endif

// Flag indicating whether this node is currently connected to the main network tree
extern bool connectedToMainTree;


extern void (*onRootUnreachableCallback)();
extern void (*onRootReachableCallback)();

typedef struct ParentInfo{
    char* ssid;
    uint8_t parentIP[4];
    int rootHopDistance;
    int nrOfChildren;
}ParentInfo;

typedef struct RoutingTableEntry {
    int hopDistance;
    uint8_t nextHopIP[4];
    int sequenceNumber;
} RoutingTableEntry;

extern TableInfo* routingTable;
extern TableInfo* childrenTable;

extern bool iamRoot;
extern int rootHopDistance;
extern int numberOfChildren;
extern bool hasParent;

extern uint8_t myIP[4];
extern uint8_t rootIP[4];
extern uint8_t parent[4];

extern int mySequenceNumber;

extern unsigned long lastRoutingUpdateTime;


void setIP(void* av, void* bv);
void setValue(void* av, void* bv);

void initTables();
void printRoutingStruct(TableEntry* Table);
void printRoutingTableHeader();
void printChildStruct(TableEntry* Table);
void printChildrenTableHeader();

void* findNode(TableInfo* Table, uint8_t nodeIP[4]);
uint8_t * findRouteToNode(uint8_t nodeIP[4]);
bool updateRoutingTable(uint8_t nodeIP[4], int hopDistance, int sequenceNumber, uint8_t senderIP[4]);
void updateChildrenTable(uint8_t APIP[4], uint8_t STAIP[4]);
ParentInfo chooseParent(ParentInfo* possibleParents, int n);
bool inMySubnet(uint8_t * nodeIP);
void updateMySequenceNumber(int newSequenceNumber);
int getNumberOfActiveDevices();
bool isNodeReachable(uint8_t *nodeIP);
void getIPFromMAC(uint8_t * MAC, uint8_t * IP);

int getDistanceToNode(uint8_t *nodeIP);

#endif //ROUTING_H
