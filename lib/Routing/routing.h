#ifndef ROUTING_H
#define ROUTING_H

//#include "table.h"
#include "../Table/table.h"

typedef struct routingTableEntry {
    int hopDistance;
    int nextHopIP[4];
} routingTableEntry;

typedef struct parentInfo{
    char* ssid;
    int parentIP[4];
    int rootHopDistance;
    int nrOfChildren;
}parentInfo;

extern TableInfo* routingTable;
extern TableInfo* childrenTable;
extern int parent[4];

extern bool iamRoot;
extern int rootHopDistance;
extern int numberOfChildren;
extern bool hasParent;

bool isIPEqual(void* a, void* b);
void printNodeStruct(TableEntry* Table);
void* findNode(TableInfo* Table, int nodeIP[4]);
void initTables();
int* findRouteToNode(int nodeIP[4]);
void updateRoutingTable(int nodeIP[4], int nextHopIP[4], int hopDistance);
parentInfo chooseParent(parentInfo* possibleParents, int n);

#endif //ROUTING_H
