#ifndef ROUTING_H
#define ROUTING_H

//#include "table.h"
#include "../Table/table.h"

typedef struct NodeEntry {
    int nodeIP[4];
    int hopDistance;
    int nextHopIP[4];
} NodeEntry;

typedef struct childEntry{
    int APIP[4];
    int STAIP[4];
}childEntry;


extern TableInfo* RoutingTable;
extern TableInfo* ChildrenTable;
extern int parent[4];

extern int rootHopDistance;
extern int numberOfChildren;
extern bool hasParent;

bool isIPEqual(void* a, void* b);
void* findNode(TableInfo* Table, int nodeIP[4]);
int* findRouteToNode(int nodeIP[4]);

#endif //ROUTING_H
