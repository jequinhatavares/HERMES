#ifndef ROUTING_H
#define ROUTING_H

//#include "table.h"
#include <../table/table.h>
#include <logger.h>
#include <../middleware/strategies/strategy_inject.h>

typedef struct parentInfo{
    char* ssid;
    int parentIP[4];
    int rootHopDistance;
    int nrOfChildren;
}parentInfo;

typedef struct routingTableEntry {
    int hopDistance;
    int nextHopIP[4];
    int sequenceNumber;
} routingTableEntry;

extern TableInfo* routingTable;
extern TableInfo* childrenTable;
extern int parent[4];

extern bool iamRoot;
extern int rootHopDistance;
extern int numberOfChildren;
extern bool hasParent;

extern int myIP[4];
extern int rootIP[4];
extern int mySequenceNumber;

extern unsigned long lastRoutingUpdateTime;
#define ROUTING_UPDATE_INTERVAL 60000
//extern int debugServerIP[4];

bool isIPEqual(void* a, void* b);
void assignIP(int destIP[4], int sourceIP[4]);
void setKey(void* av, void* bv);
void setValue(void* av, void* bv);
void initTables();
void printRoutingStruct(TableEntry* Table);
void printChildStruct(TableEntry* Table);
void* findNode(TableInfo* Table, int nodeIP[4]);
int* findRouteToNode(int nodeIP[4]);
//void updateRoutingTable(int nodeIP[4], routingTableEntry newNode, int senderIP[4]);
bool updateRoutingTableSN(int nodeIP[4], int hopDistance, int sequenceNumber, int senderIP[4]);
void updateChildrenTable(int APIP[4], int STAIP[4]);
parentInfo chooseParent(parentInfo* possibleParents, int n);
bool inMySubnet(int* nodeIP);
void updateMySequenceNumber(int newSequenceNumber);

#endif //ROUTING_H
