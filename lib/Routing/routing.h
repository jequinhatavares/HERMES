#ifndef ROUTING_H
#define ROUTING_H

#include "table.h"

typedef struct NodeEntry {
    int nodeIP[4];
    int hopDistance;
    int nextHopIP[4];
} NodeEntry;

typedef struct childrenEntry{
    int APIP[4];
    int STAIP[4];
}childrenEntry;


extern TableInfo* RoutingTable;
extern TableInfo* ChildrenTable;
extern int numberOfChildren;
//extern int parent[4];

bool isIPEqual(void* a, void* b);
NodeEntry* findNode(TableInfo* Table, int nodeIP[4]);
int* findRouteToNode(int nodeIP[4]);

#endif //ROUTING_H
